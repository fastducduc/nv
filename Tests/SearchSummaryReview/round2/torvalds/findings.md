# Round 2: native action compatibility in the minimum-size window

No introduced actionable defect emerged from this focused review. Severity: none.

This review applies a correctness and compatibility perspective inspired by Linus Torvalds.
It does not represent his identity, review, or endorsement.

Reviewed change: [PR 11](https://github.com/dangduc/nv/pull/11), production checkpoint `72c668cc5329ce6e48850377b35ec7d6d5b27d5b`, against base `4624b3d`.
Both runs started at head `5f7d5afce8dd0bb217018160a6e0d0269b29e1ad`, which adds review evidence without changing production.
Relevant production code is `Sources/Browser/AppController_BrowserUI.m:291` and `Sources/Browser/AppController_Search.m:53`.

## New executable evidence

Run from the repository root with authorized AppKit desktop access:

```sh
python3 Tests/SearchSummaryReview/round2/torvalds/run.py
```

The actual copied Intel app passed **46 assertions**, including three common fixture checks.
The negative variant failed at its expected assertion after **15 passes**.
The tests used macOS 26.5.2 (25F84), Xcode 26.6 (17F113), and the shared `build/pr-review/gui.lock`.
The common runner creates disposable notes and an isolated preferences domain for each app copy.

This probe extends round one's extracted-method and SDK checks into the actual nib-backed controls and browser session.
It uses one supported minimum-size window, avoiding another size or lifecycle matrix.

| Native geometry | Points |
| --- | --- |
| Window content | 480 × 320 |
| List container | 480 × 107 |
| Retry/Create frame | x=12, y=30.5, width=456, height=32 |
| Status frame | x=8, y=85, width=464, height=18 |
| List height during error | 83 |
| List height after completion | 107 |

The probe holds genuine native search completions at the service callback boundary.
It supplies one long German, Japanese, and French error description through the existing completion callback.
The production session accepts the error and updates the real controls.
The positive run does not replace the status method, action handlers, or control hierarchy.

The complete error remains available through the status value and both native tooltips.
The measured text width exceeds the label width, and the label retains its native tail-truncation mode.
The status frame stays inside the list container and does not overlap Retry.
The error reserves exactly 24 points of list height.

Retry remains visible, enabled, and targeted at the browser's existing handler.
Native hit testing at its center and two inset edge points reaches the button.
Its actual `performClick:` action starts exactly one new request, clears the error, and preserves the query and note count.

A successful zero-result completion clears the status and tooltips and restores all 107 points of list height.
The original button changes to Create without replacement.
All three hit points still reach Create despite the expanded scroll view beneath it.

A matching Exact query hides the action.
Returning to an unmatched Fuzzy query restores Create with the same native target, selector, and control objects.
The fixture note's source characters and the library's single-note count remain unchanged.

## Negative control

The negative variant moves the existing scroll view above Retry inside the disposable app copy.
It leaves the button's visibility, target, selector, and frame unchanged.
Those checks pass, but the center hit test fails at `Retry: native hit point 0 reaches the action`.

The process exits with status 1, as required by the runner.
This control demonstrates that the probe detects an obstructed action despite valid control properties.
The runner returns success only when both production and the expected negative failure are verified.

## Source and binary identity

The runner rejects production sources that differ from the checkpoint and rejects an unexpected Development executable.
Source, probe, harness, and executable hashes match before and after both runs.

| Input | SHA-256 |
| --- | --- |
| `AppController_BrowserUI.m` | `45b01fcb17f0df40c58689424053b9d221917f302571ab21ecb7afb40dc9e104` |
| `AppController_Search.m` | `c9140868ba8ff918e47f420bf7a33efed22232ec4bdcf77be075fbbad92f6a68` |
| `NVBrowserSession.m` | `2ec178fde253541204b26e36c3bfce018aacb17fe0469ceecd17bfa9ce91e170` |
| Development executable | `efc6fce32c2647e78909efe32d699737a58b3987cdc8b0ddb5fde753d67fdb99` |
| New `checks.inc` | `905bbb78390f23607f909505e73ffecce4b4b904ffc9a52df6a45f666d6ef29b` |

Logs and `results.json` reside in `build/SearchSummaryReview/round2/torvalds/`.
The JSON includes full runner commands, return codes, assertion counts, and before/after input hashes.
It reports no changed inputs.

## Limits

The multilingual error is synthetic; this probe does not validate translation resources or natural service failure causes.
Hit testing and `performClick:` exercise AppKit routing without hardware mouse events.
The checks inspect native frames and truncation settings; they do not measure rendered text readability or run VoiceOver.
Create dispatch and note creation were covered in round one; this round checks its actual layout, identity, and routing properties.

This review does not repeat older-macOS runtime, delayed-timer, stale-callback, viewport, peer-lifecycle, or shared-body Undo tests.
It adds only its new probe, prefix, runner, and report. It makes no production edits, commits, or review comments.
