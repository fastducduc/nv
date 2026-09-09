# Round two: browser ownership under interleaved search completion

No introduced defect found in this bounded review of PR #11. The review uses a Kyle Kingsbury-inspired state-consistency perspective; it does not claim his identity or endorsement.

## Scope and source identity

Read `AGENTS.md`, the search and window ownership section of `architecture.md`, and the production diff from `4624b3d` to `72c668cc5329ce6e48850377b35ec7d6d5b27d5b`. The changed `updateSearchAffordance` method is in `Sources/Browser/AppController_BrowserUI.m:291`. It removes the completed summary and gives the list its full area when no transient status is visible.

The execution started and ended at evidence HEAD `402ecbfd4cf83e5f5bbd1370e1060c7a167f5d4b`. All seven reviewed production inputs matched production commit `72c668cc5329ce6e48850377b35ec7d6d5b27d5b`, byte for byte, before and after the runs. `results.json` records each SHA-256, the exact commands, and fixture hashes.

- Browser UI source SHA-256 before and after: `45b01fcb17f0df40c58689424053b9d221917f302571ab21ecb7afb40dc9e104`.
- Actual Intel app executable SHA-256 before and after: `efc6fce32c2647e78909efe32d699737a58b3987cdc8b0ddb5fde753d67fdb99`.
- Host: macOS 26.5.2 (25F84), Xcode 26.6 (17F113), Intel app under Rosetta.

This review writes only its own evidence directory. It makes no production edits, commits, or PR comments.

## Independent model and actual components

`checks.inc` supplies explicit expected states for each browser: committed and visible query, mode, selected document, result currency, status text, and Retry visibility. Expected states come from the event history, not from copying actual UI properties. A nonempty transient status reserves 24 points; every completed state reserves zero. Every checkpoint also compares both tooltips and the complete list frame against those expectations.

The probe loads into a disposable copy of the actual app. Both windows, browser controllers, sessions, shared service, native fuzzy computation, delayed-progress timer, native Retry button action, and window close path are production components. The fixture uses three disposable notes and exactly six native search requests. The common runner serializes desktop access with `build/pr-review/gui.lock`.

The delivery gate holds real native results at the service completion boundary. Both browser sessions can therefore remain pending while the probe chooses publication order. The worker itself remains serial. The gate records each submitting session pointer and checks that every captured callback has the intended owner. Synthetic errors and callback replays use the original production completion block.

## New histories

| History | Controlled events | Required owner-local outcome |
| --- | --- | --- |
| A | Both browsers enter delayed progress. Browser A receives an error. Browser B then succeeds. | A keeps its error, Retry action, query, selected note, and 24-point status area. B clears its status and returns the full list area. |
| B | A retries through its native button. B submits a new query. Old success/error callbacks replay. B completes first, followed by A. | Stale callbacks change neither owner. B's completion leaves A searching. A's completion clears only its own status. Both accepted Reveal selections survive. A late progress callback cannot restore the completed gap. |
| C | Both browsers enter delayed progress again. B closes. A completes. B's retained success/error callbacks replay after close. | Only B leaves the window registry. A keeps its pending status, then completes with the expected query and selection. Closed-owner callbacks do not reopen a window, show Retry, or restore a gap in A. |

These cases extend round one's single-browser histories with overlapping owners, reverse publication order, and close isolation.

## Execution and negative control

Run from the repository root:

```sh
python3 Tests/SearchSummaryReview/round2/kingsbury/run.py
git diff --check
```

The wrapper ran the shared copied-app command twice. Exact arguments and environment overrides are in `results.json`.

| Run | Result |
| --- | --- |
| Production history | Exit 0; **179 checks passed** across the three histories. |
| `NV_SUMMARY_WRONG_OWNER=1` control | Exit 1 after **60 passed checks**, at `owner-local status visibility follows the independent model`. Expected rejection. |
| Wrapper | Exit 0; both outcomes expected; source and executable hashes unchanged. |
| Diff whitespace check | Exit 0. |

The negative control wraps only the copied app's `updateSearchAffordance` method at runtime. When B completes, it deliberately clears A's error label, tooltips, and reserved area. It leaves A's session in the failed state. The model rejects this wrong-owner UI update at the first visibility assertion. Thus the assertions can detect owner isolation failures rather than merely successful completion.

Logs are generated at `build/SearchSummaryReview/round2/kingsbury/history.log` and `wrong-owner.log`. Neither compilation nor the normal history emitted a warning, error, or exception.

## Limits

This is three deterministic histories, not an exhaustive schedule search. Publication ordering is controlled after native computation; it does not measure worker timing or prove cancellation during a native call. Errors are synthetic. The probe retains the closed peer long enough to replay its callbacks, so it checks session invalidation and survivor isolation, not deallocation behavior. It does not exercise composition, shared-body Undo, or the previously documented baseline creation/selection issue. Normal peer lifecycle and list collapse are covered by a separate round-two review.

No actionable finding or severity is assigned from these histories.
