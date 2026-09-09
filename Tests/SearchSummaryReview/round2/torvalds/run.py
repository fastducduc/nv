#!/usr/bin/env python3
"""Run minimum-window native action checks in an isolated Intel app copy."""
import hashlib
import json
import os
from pathlib import Path
import plistlib
import subprocess
import sys

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]
OUT = REPO / 'build/SearchSummaryReview/round2/torvalds'
OUT.mkdir(parents=True, exist_ok=True)
CHECKPOINT = '72c668cc5329ce6e48850377b35ec7d6d5b27d5b'
APP = REPO / 'build/DerivedData/Build/Products/Development/nvALT.app'
info = plistlib.loads((APP / 'Contents/Info.plist').read_bytes())
BINARY = APP / 'Contents/MacOS' / info['CFBundleExecutable']
EXPECTED_BINARY = 'efc6fce32c2647e78909efe32d699737a58b3987cdc8b0ddb5fde753d67fdb99'
PATHS = ['Sources/Browser/AppController_BrowserUI.m', 'Sources/Browser/AppController_Search.m',
         'Sources/Browser/NVBrowserSession.m', 'Tests/ViewControlsReview/run-probe.py',
         'Tests/Regression/native-controls/probes.m', 'Tests/Regression/native-controls/checks.inc']
PATHS += [str((HERE / name).relative_to(REPO)) for name in ('checks.inc', 'prefix.h', 'run.py')]


def hashes():
    values = {path: hashlib.sha256((REPO / path).read_bytes()).hexdigest() for path in PATHS}
    values['Development executable'] = hashlib.sha256(BINARY.read_bytes()).hexdigest()
    return values


before = hashes()
assert before['Development executable'] == EXPECTED_BINARY, 'Unexpected app build; review the new build identity before running'
for path in PATHS[:3]:
    committed = subprocess.check_output(['git', 'show', CHECKPOINT + ':' + path], cwd=REPO)
    assert hashlib.sha256(committed).hexdigest() == before[path], path + ' differs from checkpoint'
record = {'checkpoint': CHECKPOINT,
          'head': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=REPO, text=True).strip(),
          'source_sha256_before': before,
          'os': subprocess.check_output(['sw_vers'], text=True).strip(),
          'toolchain': subprocess.check_output(['xcodebuild', '-version'], text=True).strip(),
          'runs': []}
command = [sys.executable, str(REPO / 'Tests/ViewControlsReview/run-probe.py'), '--probe', str(HERE / 'checks.inc'),
           '--prefix', str(HERE / 'prefix.h'), '--timeout', '60']
for mode in ('production', 'negative'):
    environment = dict(os.environ)
    if mode == 'negative': environment['NV_SUMMARY_COMPATIBILITY_NEGATIVE'] = '1'
    print('Starting', mode, 'copied-app run with shared GUI lock.', flush=True)
    result = subprocess.run(command, cwd=REPO, env=environment, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (OUT / (mode + '.log')).write_text(result.stdout)
    print(result.stdout, end='', flush=True)
    expected = result.returncode == 0 and 'TORVALDS_SUMMARY_R2_PASS' in result.stdout if mode == 'production' else (
        result.returncode == 1 and 'FAIL: Retry: native hit point 0 reaches the action' in result.stdout)
    record['runs'].append({'mode': mode, 'command': command, 'exit_code': result.returncode,
                           'pass_count': result.stdout.count('PASS:'), 'expected': expected})
    record['source_sha256_after'] = hashes()
    record['changed_inputs'] = [path for path, digest in before.items() if record['source_sha256_after'][path] != digest]
    (OUT / 'results.json').write_text(json.dumps(record, indent=2) + '\n')
    assert expected, mode + ' did not produce its expected result'
    assert not record['changed_inputs'], record['changed_inputs']
print('PASS: production and native hit-testing negative control produced expected results.')
