# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| QOLSET | 59 | 61 | 20261030-qolpanels-A | 2026-10-30T15:35:37+01:00 |
| QOLMAP | 5 | 7 | 20261030-qolpanels-B | 2026-10-30T15:35:37+01:00 |
| QOLSET | 62 | 64 | 20261030-qolpanels-C | 2026-10-30T15:35:37+01:00 |
| HUD | 97 | 98 | 20261030-qolpanels-D | 2026-10-30T15:35:37+01:00 |
| QOLSET-S | 6 | 6 | 20261030-qolpanels-A | 2026-10-30T15:35:37+01:00 |
| QOLMAP-S | 1 | 1 | 20261030-qolpanels-B | 2026-10-30T15:35:37+01:00 |
| RENDER | 80 | 81 | 20261030-qolpanels-E | 2026-10-30T15:35:47+01:00 |
