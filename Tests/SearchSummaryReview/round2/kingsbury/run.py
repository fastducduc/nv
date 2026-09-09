#!/usr/bin/env python3
"""Run two-browser copied-app histories and a wrong-owner clearing control."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

repo=Path(__file__).resolve().parents[4]
here=Path(__file__).resolve().parent
out=repo/'build/SearchSummaryReview/round2/kingsbury'
out.mkdir(parents=True,exist_ok=True)
paths=['Sources/Browser/AppController.h','Sources/Browser/AppController.m','Sources/Browser/AppController_BrowserUI.m',
       'Sources/Browser/AppController_Search.m','Sources/Browser/NVBrowserSession.m','Sources/Search/NVSearchService.m',
       'Sources/Application/NVApplicationController.m']
digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
head_before=subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip()
before={p:digest(repo/p) for p in paths}
baseline='72c668cc5329ce6e48850377b35ec7d6d5b27d5b'
matches=all(hashlib.sha256(subprocess.check_output(['git','show',baseline+':'+p],cwd=repo)).hexdigest()==h for p,h in before.items())
binary=repo/'build/DerivedData/Build/Products/Development/nvALT.app/Contents/MacOS/nvALT'
binary_before=digest(binary)
command=[sys.executable,str(repo/'Tests/ViewControlsReview/run-probe.py'),'--probe',str(here/'checks.inc'),
         '--prefix',str(here/'prefix.h'),'--timeout','90']
records=[]
for name,extra in [('history',{}),('wrong-owner',{'NV_SUMMARY_WRONG_OWNER':'1'})]:
    r=subprocess.run(command,env=dict(os.environ,**extra),text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    (out/(name+'.log')).write_text(r.stdout)
    ok=(r.returncode==0 and 'SEARCH_SUMMARY_KINGSBURY_R2_PASS' in r.stdout) if name=='history' else (
        r.returncode!=0 and 'FAIL: owner-local status visibility follows the independent model' in r.stdout)
    rec={'name':name,'command':command,'environment_overrides':extra,'exit_code':r.returncode,
         'expected_outcome':ok,'pass_count':r.stdout.count('PASS:')}
    records.append(rec); print(json.dumps(rec),flush=True)
after={p:digest(repo/p) for p in paths}
summary={'head_before':head_before,'head_after':subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip(),
    'baseline':baseline,'inputs_match_baseline':matches,'before_sha256':before,'after_sha256':after,
    'production_inputs_stable':before==after,'binary_before_sha256':binary_before,'binary_after_sha256':digest(binary),
    'fixture_sha256':{p.name:digest(p) for p in [here/'checks.inc',here/'prefix.h',Path(__file__)]},'runs':records}
(here/'results.json').write_text(json.dumps(summary,indent=2)+'\n')
if not all(r['expected_outcome'] for r in records) or before!=after or binary_before!=digest(binary) or not matches: raise SystemExit(1)
