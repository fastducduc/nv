#!/usr/bin/env python3
"""Run completed-search layout and ownership histories in an isolated Intel app."""
import hashlib
import json
import os
from pathlib import Path
import plistlib
import subprocess
import sys

repo = Path(__file__).resolve().parents[4]
here = Path(__file__).resolve().parent
out = repo / "build/SearchSummaryReview/round2/ousterhout"
out.mkdir(parents=True, exist_ok=True)
app = repo / "build/DerivedData/Build/Products/Development/nvALT.app"
info_path = app / "Contents/Info.plist"
info = plistlib.loads(info_path.read_bytes())
binary = app / "Contents/MacOS" / info["CFBundleExecutable"]
sources = ["Sources/Browser/AppController_BrowserUI.m", "Sources/Browser/AppController.m",
           "Sources/Browser/AppController.h", "Sources/Application/NVApplicationController.m"]
def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()
def inputs():
    return {"sources": {p: digest(repo / p) for p in sources}, "app_binary": digest(binary), "app_info": digest(info_path)}
record = {"head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=repo, text=True).strip(),
          "before": inputs(), "binary": str(binary.relative_to(repo)), "runs": []}
command = [sys.executable, str(repo / "Tests/ViewControlsReview/run-probe.py"), "--probe", str(here / "checks.inc"),
           "--prefix", str(here / "instrumentation.h"), "--app", str(app), "--timeout", "90"]
for name, extra in [("production", {}), ("reserved-gap-control", {"NV_SUMMARY_GAP_CONTROL": "1"})]:
    result = subprocess.run(command, cwd=repo, env=dict(os.environ, **extra), text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (out / (name + ".log")).write_text(result.stdout)
    expected = (result.returncode == 0 and "SUMMARY_OUSTERHOUT_ROUND2_PASS" in result.stdout) if name == "production" else (
        result.returncode != 0 and "FAIL: visible notes list has no reserved summary gap" in result.stdout)
    entry = {"name": name, "exit_code": result.returncode, "pass_count": result.stdout.count("PASS:"),
             "expected_outcome": expected, "command": command}
    record["runs"].append(entry)
    print(json.dumps(entry), flush=True)
record["after"] = inputs()
record["inputs_unchanged"] = record["before"] == record["after"]
record["probe_hashes"] = {p.name: digest(p) for p in (here / "checks.inc", here / "instrumentation.h", here / "run.py")}
(out / "results.json").write_text(json.dumps(record, indent=2) + "\n")
if not record["inputs_unchanged"] or not all(x["expected_outcome"] for x in record["runs"]):
    raise SystemExit(1)
