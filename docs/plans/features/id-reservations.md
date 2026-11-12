# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| TRAFFIC | 50 | 51 | 20261112-traffic-A | 2026-11-12T07:58:23+01:00 |
| TRAFFIC-S | 37 | 38 | 20261112-traffic-A | 2026-11-12T07:58:23+01:00 |
