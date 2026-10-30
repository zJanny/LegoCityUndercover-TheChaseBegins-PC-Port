# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| QOLSET | 65 | 66 | 20261030-qolmenu-A | 2026-10-30T20:53:39+01:00 |
| QOLSET | 67 | 68 | 20261030-qolmenu-B | 2026-10-30T20:53:40+01:00 |
| QOLFS | 20 | 21 | 20261030-qolmenu-C | 2026-10-30T20:53:40+01:00 |
