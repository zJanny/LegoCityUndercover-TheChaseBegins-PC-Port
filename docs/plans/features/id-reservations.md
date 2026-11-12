# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| TRAFFIC | 64 | 66 | 20261112-trafficfeed-C | 2026-11-12T09:46:57+01:00 |
| TRAFFIC-S | 45 | 46 | 20261112-trafficfeed-C | 2026-11-12T09:46:57+01:00 |
| DISG | 1 | 8 | 20261112-trafficfeed-D | 2026-11-12T09:46:57+01:00 |
| DISG-S | 1 | 2 | 20261112-trafficfeed-D | 2026-11-12T09:46:57+01:00 |
