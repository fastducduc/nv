#!/usr/bin/env python3
"""Measure actual copied-app list viewports and detect an unreclaimed status strip."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[4]
here = Path(__file__).resolve().parent
out = root / 'build/SearchSummaryReview/round2/luu'
out.mkdir(parents=True, exist_ok=True)
paths = ['Sources/Browser/AppController_BrowserUI.m', 'Sources/Browser/AppController.m',
         'Sources/Browser/AppController_Search.m', 'Sources/Browser/NVBrowserSession.m',
         'build/DerivedData/Build/Products/Development/nvALT.app/Contents/MacOS/nvALT']
def hashes():
    return {p: hashlib.sha256((root / p).read_bytes()).hexdigest() for p in paths}

record = {'head': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip(),
          'productionCheckpoint': '72c668cc5329ce6e48850377b35ec7d6d5b27d5b',
          'os': subprocess.check_output(['sw_vers'], text=True),
          'xcode': subprocess.check_output(['xcodebuild', '-version'], text=True),
          'before': hashes(), 'runs': {}}
record['matchesProductionCheckpoint'] = all(hashlib.sha256(subprocess.check_output(
    ['git', 'show', record['productionCheckpoint'] + ':' + p], cwd=root)).hexdigest() == record['before'][p]
    for p in paths[:-1])
(out / 'identity-before.json').write_text(json.dumps(record, indent=2) + '\n')
command = [sys.executable, str(root / 'Tests/ViewControlsReview/run-probe.py'), '--probe',
           str(here / 'checks.inc'), '--prefix', str(here / 'prefix.h'), '--timeout', '90']
for name in ['production', 'unreclaimed_space_negative_control']:
    env = dict(os.environ, NV_VIEWPORT_RESULT=str(out / 'result.json'))
    if name != 'production': env['NV_VIEWPORT_NEGATIVE'] = '1'
    with (out / (name + '.log')).open('w') as log:
        # The common runner acquires the shared GUI lock before any app launch.
        result = subprocess.run(command, cwd=root, env=env, stdout=log, stderr=subprocess.STDOUT)
    text = (out / (name + '.log')).read_text()
    expected = (result.returncode == 0 and 'SEARCH_SUMMARY_VIEWPORT_PASS' in text) if name == 'production' else (
        result.returncode != 0 and 'FAIL: actual list reclaims all 24 status points except while progress is visible' in text)
    record['runs'][name] = {'returncode': result.returncode, 'expectedOutcome': expected, 'passCount': text.count('PASS:')}
    if name == 'production' and not expected: break
record['after'] = hashes()
record['unchanged'] = record['before'] == record['after']
(out / 'summary.json').write_text(json.dumps(record, indent=2) + '\n')
print(json.dumps(record, indent=2))
assert record['matchesProductionCheckpoint'] and record['unchanged']
assert len(record['runs']) == 2 and all(run['expectedOutcome'] for run in record['runs'].values())
