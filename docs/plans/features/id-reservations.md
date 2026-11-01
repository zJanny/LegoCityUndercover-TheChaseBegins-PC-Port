# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| QOLCTL | 9 | 10 | 20261101-qolctl-A | 2026-11-01T14:36:56+01:00 |
| QOLCTL-S | 2 | 2 | 20261101-qolctl-A | 2026-11-01T14:36:56+01:00 |
| QOLCTL | 11 | 12 | 20261101-qolctl-B | 2026-11-01T14:36:56+01:00 |
| QOLCTL-S | 3 | 3 | 20261101-qolctl-B | 2026-11-01T14:36:56+01:00 |
| QOLSET | 98 | 99 | 20261101-qolctl-C | 2026-11-01T14:36:56+01:00 |
| QOLSET-S | 8 | 8 | 20261101-qolctl-C | 2026-11-01T14:36:56+01:00 |
