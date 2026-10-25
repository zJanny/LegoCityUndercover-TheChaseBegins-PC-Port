# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| LEVOBJ | 123 | 125 | 20261025-mission13-A | 2026-10-25T10:55:30+01:00 |
| LEVOBJ-S | 97 | 98 | 20261025-mission13-A | 2026-10-25T10:55:30+01:00 |
| SCRIPT | 358 | 360 | 20261025-mission13-B | 2026-10-25T10:55:30+01:00 |
| SCRIPT-S | 289 | 290 | 20261025-mission13-B | 2026-10-25T10:55:30+01:00 |
| HUD | 84 | 86 | 20261025-mission13-C | 2026-10-25T10:55:30+01:00 |
| HUD-S | 78 | 78 | 20261025-mission13-C | 2026-10-25T10:55:30+01:00 |
| STUD | 28 | 28 | 20261025-mission13-C | 2026-10-25T10:55:30+01:00 |
| TOOLS | 18 | 19 | 20261025-mission13-D | 2026-10-25T11:15:38+01:00 |
| TOOLS-S | 3 | 3 | 20261025-mission13-D | 2026-10-25T11:15:38+01:00 |
