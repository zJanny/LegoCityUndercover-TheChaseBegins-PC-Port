# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| UI | 26 | 28 | 20261028-uiport-C | 2026-10-28T22:23:21+01:00 |
| UI-S | 12 | 13 | 20261028-uiport-C | 2026-10-28T22:23:21+01:00 |
| SCAN | 18 | 20 | 20261028-uiport-D | 2026-10-28T22:23:21+01:00 |
| SCAN-S | 13 | 14 | 20261028-uiport-D | 2026-10-28T22:23:21+01:00 |
| HUD | 86 | 88 | 20261028-uiport-E | 2026-10-28T22:23:21+01:00 |
| HUD-S | 79 | 80 | 20261028-uiport-E | 2026-10-28T22:23:22+01:00 |
