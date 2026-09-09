#!/usr/bin/env python3
"""Persist and restore three browser search states across two real app launches."""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

repo = Path(__file__).resolve().parents[4]
output = repo / 'build/SearchSummaryReview/round2/contrarian'
output.mkdir(parents=True, exist_ok=True)
reviewed = '72c668cc5329ce6e48850377b35ec7d6d5b27d5b'
paths = ['Sources/Browser/AppController_BrowserUI.m', 'Sources/Browser/AppController_MultipleWindows.m',
         'Sources/Browser/NVBrowserSession.m', 'Tests/SearchSummaryReview/round2/contrarian/checks.inc',
         'build/DerivedData/Build/Products/Development/nvALT.app/Contents/MacOS/nvALT']

def hashes():
    return {path: hashlib.sha256((repo / path).read_bytes()).hexdigest() for path in paths}

identity = {'reviewed_production_commit': reviewed,
            'checkout_head': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=repo, text=True).strip(),
            'sha256_before': hashes()}
identity['production_matches_reviewed_commit'] = all(
    (repo / path).read_bytes() == subprocess.check_output(['git', 'show', reviewed + ':' + path], cwd=repo)
    for path in paths if path.startswith('Sources/'))
if not identity['production_matches_reviewed_commit']:
    raise SystemExit('Production inputs no longer match the reviewed commit.')
results = []
for variant in ['production', 'restored-gap-control']:
    env = dict(os.environ)
    env.pop('NV_SUMMARY_RESTORE_NEGATIVE', None)
    if variant != 'production':
        env['NV_SUMMARY_RESTORE_NEGATIVE'] = '1'
    command = [sys.executable, str(repo / 'Tests/ViewControlsReview/run-probe.py'),
               '--probe', str(Path(__file__).with_name('checks.inc')), '--launches', '2', '--timeout', '60']
    result = subprocess.run(command, cwd=repo, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (output / (variant + '.log')).write_text(result.stdout)
    phases = dict(re.findall(r'SUMMARY_RESTORE_PHASE_PASS phase=(\d+) checks=(\d+)', result.stdout))
    expected = result.returncode == 0 and set(phases) == {'1', '2'} if variant == 'production' else (
        result.returncode == 1 and '1' in phases and 'FAIL: relaunch restoration leaves no persistent blank summary strip' in result.stdout)
    results.append({'variant': variant, 'exit_code': result.returncode, 'completed_phases': phases,
                    'checks_passed': result.stdout.count('PASS:'), 'expected': expected})
    print(json.dumps(results[-1]), flush=True)
    if variant == 'production' and not expected:
        print(result.stdout[-13000:])
        break
identity['sha256_after'] = hashes()
identity['inputs_unchanged'] = identity['sha256_before'] == identity['sha256_after']
(output / 'identity.json').write_text(json.dumps(identity, indent=2) + '\n')
(output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
raise SystemExit(0 if len(results) == 2 and all(result['expected'] for result in results) and identity['inputs_unchanged'] else 1)
