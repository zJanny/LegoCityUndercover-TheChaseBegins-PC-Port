# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| QOLCTL | 11 | 12 | 20261101-qolctl-B | 2026-11-01T14:36:56+01:00 |
| QOLCTL-S | 3 | 3 | 20261101-qolctl-B | 2026-11-01T14:36:56+01:00 |
| PLAYER | 26 | 29 | 20261101-mission-D | 2026-11-01T14:38:31+01:00 |
| PLAYER-S | 12 | 14 | 20261101-mission-D | 2026-11-01T14:38:31+01:00 |
