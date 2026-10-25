# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| SCRIPT | 365 | 367 | 20261025-garage-A | 2026-10-25T16:08:02+01:00 |
| SCRIPT-S | 292 | 293 | 20261025-garage-A | 2026-10-25T16:08:02+01:00 |
| SCRIPT | 368 | 370 | 20261025-garage-B | 2026-10-25T16:08:02+01:00 |
| SCRIPT-S | 294 | 295 | 20261025-garage-B | 2026-10-25T16:08:02+01:00 |
| SCRIPT | 371 | 373 | 20261025-garage-C | 2026-10-25T16:08:02+01:00 |
| SCRIPT-S | 296 | 297 | 20261025-garage-C | 2026-10-25T16:08:02+01:00 |
