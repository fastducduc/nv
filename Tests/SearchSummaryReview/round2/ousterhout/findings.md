# Round 2: design, ownership, and layout review

Perspective: John Ousterhout's emphasis on simple interfaces and explicit ownership. This review does not represent him.

Production checkpoint: `72c668cc5329ce6e48850377b35ec7d6d5b27d5b`, compared with `4624b3d`. The run used head `612df045585d76d2a6d817d29b591cfe12614759`, which contains the same production source.

Round 1 requested no production correction. This review uses the same production checkpoint.

## Findings

No introduced actionable defect found in the executed lifecycle and layout cases.

The existing status label and action button remain in their original list container. Completed and empty queries leave no status text, tooltip, or reserved row. Native list collapse and restoration reuse these controls without changes to browser ownership.

The relevant changed code is `Sources/Browser/AppController_BrowserUI.m:299` through `:309`. The lifecycle cases also exercise the existing `updateNotesListVisibility` method at `:161` and controller teardown at `Sources/Browser/AppController.m:1931`.

## Executable evidence

Run this command from the repository root in an active desktop session:

```sh
python3 Tests/SearchSummaryReview/round2/ousterhout/run.py
```

The runner uses the actual Intel Development app through the shared copied-app harness. Each process uses two disposable notes and a unique defaults domain. Native runs share `build/pr-review/gui.lock`.

| Case | Result |
| --- | --- |
| Production: two peer lifecycles and four collapse/show pairs | 178 assertions passed, exit 0 |
| Hidden label with a restored 24-point gap | 8 assertions passed, then the geometry assertion failed, exit 1 |

One browser uses a completed fuzzy query while its peer uses an empty query. The second lifecycle reverses those roles. Both browsers select the same note through the shared library.

The probe uses the native notes-list command while the peer's Search field editor owns focus. After collapse and restoration, assertions cover both browsers' queries, selection, window frames, status controls, and available list space. The probe does not call `updateSearchAffordance` to repair layout after those transitions.

Each peer close removes one layout manager from the shared text storage. Deallocation witnesses observed two releases each for the peer controller, status label, and action button. Those witnesses retain names only. The surviving browser retains its selected note and full list area.

The negative control leaves the status label hidden and changes only the scroll-view height after completed fuzzy searches. The label assertion passes, then the geometry assertion detects the blank row.

## Source record and limits

Four production source hashes, the application binary hash, and the application plist hash match before and after both runs. `build/SearchSummaryReview/round2/ousterhout/results.json` contains those hashes, probe hashes, commands, assertion counts, and exit codes. Detailed logs remain beside that file.

The application binary SHA-256 is `efc6fce32c2647e78909efe32d699737a58b3987cdc8b0ddb5fde753d67fdb99`. The changed source SHA-256 is `45b01fcb17f0df40c58689424053b9d221917f302571ab21ecb7afb40dc9e104`.

The probe ran as x86_64 on macOS 26.5.2 (25F84), with Xcode 26.6 (17F113). It covers ordinary Source windows, native split-view visibility, completed searches, and secondary-window closure. It does not cover live IME composition, preview navigation, restored application sessions, or older macOS releases.

This review made no production edits. No production change requested by this review.
