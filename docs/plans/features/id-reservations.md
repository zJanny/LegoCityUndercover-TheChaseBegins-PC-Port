# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| HUD | 91 | 92 | 20261029-qolmenu-A | 2026-10-29T13:43:42+01:00 |
| QOLSET | 35 | 36 | 20261029-qolmenu-B | 2026-10-29T13:43:42+01:00 |
| QOLSET | 37 | 39 | 20261029-qolmenu-C | 2026-10-29T13:43:43+01:00 |
| QOLFS | 12 | 13 | 20261029-qolmenu-D | 2026-10-29T13:43:43+01:00 |
