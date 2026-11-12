# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| TRAFFIC | 50 | 51 | 20261112-traffic-A | 2026-11-12T07:58:23+01:00 |
| TRAFFIC-S | 37 | 38 | 20261112-traffic-A | 2026-11-12T07:58:23+01:00 |
| TRAFFIC | 52 | 53 | 20261112-traffic-B | 2026-11-12T07:58:23+01:00 |
| TRAFFIC-S | 39 | 39 | 20261112-traffic-B | 2026-11-12T07:58:23+01:00 |
| TRAFFIC | 54 | 56 | 20261112-traffic-C | 2026-11-12T07:58:23+01:00 |
| TRAFFIC-S | 40 | 41 | 20261112-traffic-C | 2026-11-12T07:58:23+01:00 |
| TRAFFIC | 57 | 58 | 20261112-traffic-D | 2026-11-12T07:58:23+01:00 |
| SCRIPT | 455 | 457 | 20261112-traffic-D | 2026-11-12T07:58:23+01:00 |
| SCRIPT-S | 339 | 340 | 20261112-traffic-D | 2026-11-12T07:58:23+01:00 |
| TRAFFIC-S | 42 | 42 | 20261112-traffic-D | 2026-11-12T07:58:23+01:00 |
| TRAFFIC | 59 | 60 | 20261112-traffic-E | 2026-11-12T07:58:23+01:00 |
| TRAFFIC-S | 43 | 43 | 20261112-traffic-E | 2026-11-12T07:58:23+01:00 |
| PED | 30 | 32 | 20261112-traffic-F | 2026-11-12T07:58:23+01:00 |
| PED-S | 17 | 18 | 20261112-traffic-F | 2026-11-12T07:58:23+01:00 |
