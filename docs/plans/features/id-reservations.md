# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| TRAFFIC | 23 | 24 | 20261107-traffic-A | 2026-11-07T11:57:43+01:00 |
| TRAFFIC-S | 17 | 18 | 20261107-traffic-A | 2026-11-07T11:57:43+01:00 |
| TRAFFIC | 25 | 26 | 20261107-traffic-B | 2026-11-07T11:57:43+01:00 |
| TRAFFIC-S | 19 | 20 | 20261107-traffic-B | 2026-11-07T11:57:43+01:00 |
| TRAFFIC | 27 | 28 | 20261107-traffic-C | 2026-11-07T11:57:43+01:00 |
| TRAFFIC-S | 21 | 22 | 20261107-traffic-C | 2026-11-07T11:57:43+01:00 |
| TRAFFIC | 29 | 30 | 20261107-traffic-D | 2026-11-07T11:57:43+01:00 |
| TRAFFIC-S | 23 | 24 | 20261107-traffic-D | 2026-11-07T11:57:43+01:00 |
| SCRIPT | 432 | 434 | 20261107-traffic-E | 2026-11-07T11:57:43+01:00 |
| SCRIPT-S | 329 | 330 | 20261107-traffic-E | 2026-11-07T11:57:43+01:00 |
| SCRIPT | 435 | 436 | 20261107-traffic-F | 2026-11-07T11:57:43+01:00 |
| SCRIPT-S | 331 | 331 | 20261107-traffic-F | 2026-11-07T11:57:43+01:00 |
| SCRIPT | 437 | 439 | 20261107-traffic-G | 2026-11-07T11:57:43+01:00 |
| SCRIPT-S | 332 | 333 | 20261107-traffic-G | 2026-11-07T11:57:43+01:00 |
