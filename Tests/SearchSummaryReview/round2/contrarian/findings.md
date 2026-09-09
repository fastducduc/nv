# Round 2 — contrarian review

No introduced actionable defect was found in the tested restart and saved-search boundaries.
This round checks whether persisted browser state can bring back a blank summary strip,
including searches with literal operator characters and only whitespace.

Reviewed production: `72c668cc5329ce6e48850377b35ec7d6d5b27d5b`, against `4624b3d`.
The final run used checkout `eb7026455c726e1697073b8ccb16c9a1aafaaf81`; its production inputs match the reviewed commit.

## Executable evidence

Run `python3 Tests/SearchSummaryReview/round2/contrarian/run.py` from an active desktop session.
Each variant launches the actual Intel app twice with the same disposable library and isolated preferences.
The shared copied-app harness holds `build/pr-review/gui.lock` throughout both launches.

The first process saves three dictionaries returned by the production `browserWindowState` method.
The second reopens the library and passes those dictionaries to the production `restoreBrowserWindowState:` method.
Checks wait for actual search completion, pending selection restoration, and queued divider layout.

| Restored query | Rows | Selection | List height |
| --- | --- | --- | --- |
| `road` in Fuzzy mode | Two occurrences of one note | The saved fuzzy occurrence, with the same note UUID | 125 points |
| `a\|b` in Fuzzy mode | Two occurrences of the literal-operator note | The saved fuzzy occurrence, with the same note UUID | 150 points |
| Five whitespace characters, including tab and newline | All three notes | No selected note | 175 points |

The exact query strings and Fuzzy mode survive the process boundary.
The list fills its parent in each case, status text and both tooltips remain empty, and no Create control appears.
All three header rows remain hidden through both their persisted preferences and actual view visibility.

Before capturing the first state, the fixture imposes the old 24-point gap on the scroll view.
It adds no invented persistence key. The second process restores the correct divider height and full scroll-view height.
This tests reconstruction from real saved state even when the captured child view had a legacy gap.

| Variant | Result |
| --- | --- |
| Production | First process: 28 assertions. Second process: 37 assertions. Total: 65, exit 0. |
| Reintroduce the gap after second-process restoration | First process: 28 assertions. Second process: 11 assertions, then the expected no-gap assertion fails. Total before failure: 39, exit 1. |

## Identity and limits

The runner verifies production inputs against the reviewed commit and records identical before/after hashes.

| Input | SHA-256 |
| --- | --- |
| `AppController_BrowserUI.m` | `45b01fcb17f0df40c58689424053b9d221917f302571ab21ecb7afb40dc9e104` |
| `AppController_MultipleWindows.m` | `9aebd2d5c24e99706a208dd35889c4ad1bcbb65c0e32739728ba7a94ec3b3145` |
| `NVBrowserSession.m` | `2ec178fde253541204b26e36c3bfce018aacb17fe0469ceecd17bfa9ce91e170` |
| Development executable | `efc6fce32c2647e78909efe32d699737a58b3987cdc8b0ddb5fde753d67fdb99` |

Logs, phase counts, and hashes are in `build/SearchSummaryReview/round2/contrarian/`.
The run used macOS 26.5.2 with Xcode 26.6.
The test persists production state dictionaries in its own plist and invokes the real restoration method.
It does not test the application's default startup-state file discovery, abnormal termination, or system session restoration.
Query strings are assigned through the controller, so the whitespace case does not test typing a newline into the search field.

The first fixture required an occurrence key to survive even when no note was selected.
That internal key is not a meaningful selection identity without a saved note UUID.
The corrected assertion checks the actual empty note selection; selected-note cases still require the exact saved occurrence key.
The initial diagnostic log is retained. No production change was needed.
