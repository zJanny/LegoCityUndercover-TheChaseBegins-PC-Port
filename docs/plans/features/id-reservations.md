# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| QOLFS | 10 | 12 | 20261029-qolview-A | 2026-10-29T11:33:20+01:00 |
| QOLSET | 30 | 32 | 20261029-qolview-C | 2026-10-29T11:33:20+01:00 |
| QOLSET | 33 | 34 | 20261029-qolview-D | 2026-10-29T11:37:18+01:00 |
| HUD | 87 | 89 | 20261029-hudmap-A | 2026-10-29T11:45:15+01:00 |
| HUD-S | 79 | 80 | 20261029-hudmap-A | 2026-10-29T11:45:15+01:00 |
| HUD | 90 | 92 | 20261029-hudmap-B | 2026-10-29T11:45:15+01:00 |
| HUD-S | 81 | 81 | 20261029-hudmap-B | 2026-10-29T11:45:15+01:00 |
