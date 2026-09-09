# Search summary removal

Completed searches no longer show a result count or the “title matches first” message above the notes list.
The notes list reclaims the 24-point strip, and the search tooltip clears after completion.
Delayed progress and search errors retain their status text. Retry Search and zero-result note creation remain available.

The production change is `72c668c`, based on merged upstream commit `4624b3d`.
It changes only `updateSearchAffordance` in `AppController_BrowserUI.m`.

## Validation

The Development Intel build passed on macOS 26.5.2 (25F84), with Xcode 26.6 (17F113).
The [copied-app probe](app/checks.inc) passed 26 checks, including the optional window render.
It uses disposable notes and the shared desktop-test lock.

```sh
python3 Tests/SearchSummaryReview/app/run.py
```

The probe covers four result rows across three notes, literal-title priority, Exact search, zero-result creation, and search clearing.
The [window render](../../docs/screenshots/search-without-summary.png) shows the actual app with an active query and no summary strip.

[CI passed on merged master](https://github.com/dangduc/nv/actions/runs/34393965516), commit `b6f372d`.

The required multiple-window suite still fails after shared-body Undo: index 13 is outside string length 12.
The previous PR reproduced this exception on the unchanged baseline. This change does not alter that code.
Aggregate regressions pass their native search checks, then stop at the copied fuzzy app's active-window assertion.
The complete desktop suites are therefore not green.

## Reviews

Two rounds use the requested Ousterhout, Luu, Torvalds, Kingsbury, and contrarian perspectives.
Each reviewer supplies executable evidence and a deliberate failure control.
[PR #11](https://github.com/dangduc/nv/pull/11) contains the change and review discussion.

| Round | Perspective | Evidence | PR comment |
| --- | --- | --- | --- |
| 1 | Ousterhout | [157 checks](round1/ousterhout/findings.md) | [Comment](https://github.com/dangduc/nv/pull/11#issuecomment-5607237481) |
| 1 | Luu | [40 geometry cases](round1/luu/findings.md) | [Comment](https://github.com/dangduc/nv/pull/11#issuecomment-5607273531) |
| 1 | Torvalds | [91 checks and availability compile](round1/torvalds/findings.md) | [Comment](https://github.com/dangduc/nv/pull/11#issuecomment-5607243747) |
| 1 | Kingsbury | [101 actual-app checks](round1/kingsbury/findings.md) | [Comment](https://github.com/dangduc/nv/pull/11#issuecomment-5607283179) |
| 1 | Contrarian | [39 actual-app checks](round1/contrarian/findings.md) | [Comment](https://github.com/dangduc/nv/pull/11#issuecomment-5607348612) |
| 2 | Ousterhout | [178 actual-app checks](round2/ousterhout/findings.md) | [Comment](https://github.com/dangduc/nv/pull/11#issuecomment-5607393787) |
| 2 | Luu | [213 actual-app checks](round2/luu/findings.md) | [Comment](https://github.com/dangduc/nv/pull/11#issuecomment-5607403954) |
| 2 | Torvalds | [46 actual-app checks](round2/torvalds/findings.md) | [Comment](https://github.com/dangduc/nv/pull/11#issuecomment-5607496930) |
| 2 | Kingsbury | [179 actual-app checks](round2/kingsbury/findings.md) | [Comment](https://github.com/dangduc/nv/pull/11#issuecomment-5607500779) |
| 2 | Contrarian | [65 checks across two launches](round2/contrarian/findings.md) | [Comment](https://github.com/dangduc/nv/pull/11#issuecomment-5607503740) |

The [round-one response](round1/response.md) records all five dispositions and the corrected fixture assumptions.
The [round-two response](round2/response.md) records the additional checks and delegated runner correction.
All ten reviews are complete. No introduced production correction was requested.

The [P3 runner comment](https://github.com/dangduc/nv/pull/11#issuecomment-5607417150) was delegated to the reviewer and addressed in `dc0c0db`.
The runner no longer requires the original local executable hash. Source checkpoints and before/after executable hashes remain.
Its 46 checks and expected negative control passed again.

The maintainer merged PR #11 while round two was running.
The merged application code matches the reviewed production checkpoint.
Later commits on the fork contain review evidence only; the PR comments link to those commits.
