# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| QOLSET | 49 | 52 | 20261029-qolhook-B | 2026-10-29T17:57:30+01:00 |
| QOLSET-S | 4 | 4 | 20261029-qolhook-B | 2026-10-29T17:57:30+01:00 |
| QOLBOT | 21 | 22 | 20261029-qolhook-C | 2026-10-29T17:57:30+01:00 |
| QOLFS | 14 | 15 | 20261029-qolhook-C | 2026-10-29T17:57:30+01:00 |
