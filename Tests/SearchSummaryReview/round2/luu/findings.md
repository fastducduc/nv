# Round 2 — actual list viewport and selection

Perspective: Dan Luu's emphasis on measured behavior. This report does not speak for him.

No actionable finding in the changed list layout, scroll position, or result selection.
The first-round disposition required no production correction.
This review uses unchanged production checkpoint `72c668cc5329ce6e48850377b35ec7d6d5b27d5b` for PR #11.
The measured checkout was `612df045585d76d2a6d817d29b591cfe12614759`.

This round extends the extracted first-round fixture with the actual copied Intel app under Rosetta.
The common runner isolates its notes, preferences domain, support files, and temporary files.
It holds `build/pr-review/gui.lock` during each app run.
The probe uses 48 disposable notes whose actual fuzzy search produces 96 result occurrences.
Every note appears in both the title group and the fuzzy group.

From an active desktop session, run:

```sh
python3 Tests/SearchSummaryReview/round2/luu/run.py
```

The production run passed 213 checks and recorded 25 viewport observations.
The environment was macOS 26.5.2 (25F84), with Xcode 26.6 (17F113).
The three histories cover Source at a middle scroll position, Source at the document bottom, and HTML Preview at a middle scroll position.
Each history includes silent pending search, visible progress, and completed search.
Each also includes two width changes during progress and two after completion.
The widths include 480, 600, 780, and 1,000 points.

The probe retains actual completed search results at the service callback boundary.
The search worker, session notifications, delayed progress timer, and result publication use production code.
The gate permits observations before and after the native progress timer fires, without a large or slow note library.
It does not substitute result rows or selection state.

All coordinates in this report use AppKit points.
The pane remained 180 points high, and the table document remained 1,728 points high.

| Transition | List and clip height | Middle viewport origin | Selected fuzzy row |
| --- | --- | --- | --- |
| Completed baseline | 180 | 1,140 | 64 |
| Silent pending | 180 | 1,140 | 64 |
| Visible Searching… | 156 | 1,140 | 64 |
| Completed result | 180 | 1,140 | 64 |

Both Source and HTML Preview retained the same top result occurrence and its 6-point offset through these transitions.
All four width changes retained that origin and selected occurrence in both histories.
The HTML history began and ended with a completed actual HTML render.

The Source bottom history retained selected fuzzy row 95 throughout.
Its baseline origin was 1,548 points (1,728 minus the 180-point viewport).
After progress appeared, the probe scrolled to the new valid bottom origin of 1,572 points.
Completion restored the 180-point viewport and clamped the origin to 1,548 points.
That 24-point adjustment kept the enlarged viewport inside the document.
The selected occurrence and note remained unchanged.

Every history preserved the query, all 96 result occurrences, the body mode, and the selected note.
All 48 fixture notes retained their exact source.
The library retained exactly 48 notes.

The production paths support these results.
`Sources/Browser/AppController_Search.m:59` invokes the affordance update after the progress delay.
Lines 69–77 reset and schedule that delay as search state changes.
`Sources/Browser/AppController_BrowserUI.m:302` shows the status strip only for nonempty status text on a fuzzy query.
Lines 304–306 restore the full list frame after completion.
`Sources/Browser/AppController.m:1817` saves selected occurrence keys before publication.
Lines 1837–1852 restore those keys and the table viewing location after the result rows reload.

The negative control preserves the new hidden status text but reserves 24 empty points only after actual completion.
Its first completed observation had a 156-point list in the 180-point pane.
The production geometry assertion rejected that result after 37 earlier checks passed.
This control detects failure to reclaim the space independently of text visibility.

The initial attempt used an invalid bottom scroll coordinate.
`NSClipView` accepted a direct origin at the document height, outside the normal scroll range.
The corrected probe uses document height minus viewport height for the last valid origin.
The original log remains under `build/SearchSummaryReview/round2/luu/initial-outside-document/`.
This correction concerns the probe, not the application.

This probe measures programmatic resizing and controlled completion histories in one browser window.
It does not measure ordinary search latency, native timer accuracy, frame rate, mouse-drag resizing, or large-library performance.
The common runner substitutes isolated startup paths and suppresses external editor initialization and delayed startup actions.
The probe does not cover older macOS versions, fullscreen transitions, or multiple displays.

All production inputs and the built app executable retained identical SHA-256 hashes before and after the run:

| Input | SHA-256 |
| --- | --- |
| `Sources/Browser/AppController_BrowserUI.m` | `45b01fcb17f0df40c58689424053b9d221917f302571ab21ecb7afb40dc9e104` |
| `Sources/Browser/AppController.m` | `46a9c9f6f0aee838d547dd7b0ef17fa80f97978e1e951ffa0dca91229dac67be` |
| `Sources/Browser/AppController_Search.m` | `c9140868ba8ff918e47f420bf7a33efed22232ec4bdcf77be075fbbad92f6a68` |
| `Sources/Browser/NVBrowserSession.m` | `2ec178fde253541204b26e36c3bfce018aacb17fe0469ceecd17bfa9ce91e170` |
| Built `nvALT` executable | `efc6fce32c2647e78909efe32d699737a58b3987cdc8b0ddb5fde753d67fdb99` |

The runner also confirmed that the four source inputs match the production checkpoint.
Raw observations, logs, and identity records are in `build/SearchSummaryReview/round2/luu/`.
The completed records are `result.json`, `production.log`, `unreclaimed_space_negative_control.log`, and `summary.json`.
No production files, commits, or PR comments changed during this review.
