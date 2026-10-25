# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| TRAFFIC | 4 | 6 | 20261025-traffic-A | 2026-10-25T12:03:08+01:00 |
| TRAFFIC-S | 3 | 4 | 20261025-traffic-A | 2026-10-25T12:03:09+01:00 |
| TRAFFIC | 7 | 9 | 20261025-traffic-B | 2026-10-25T12:03:09+01:00 |
| TRAFFIC-S | 5 | 6 | 20261025-traffic-B | 2026-10-25T12:03:09+01:00 |
| TRAFFIC | 10 | 12 | 20261025-traffic-C | 2026-10-25T12:03:09+01:00 |
| TRAFFIC-S | 7 | 8 | 20261025-traffic-C | 2026-10-25T12:03:09+01:00 |
| PREGEN | 6 | 7 | 20261025-traffic-C | 2026-10-25T12:03:09+01:00 |
| PREGEN-S | 4 | 4 | 20261025-traffic-C | 2026-10-25T12:03:09+01:00 |
| TOOLS | 20 | 22 | 20261025-traffic-D | 2026-10-25T12:20:48+01:00 |
