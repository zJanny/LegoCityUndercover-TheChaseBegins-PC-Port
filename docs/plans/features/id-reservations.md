# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| TRAFFIC | 14 | 16 | 20261102-traffic-A | 2026-11-02T23:31:41+01:00 |
| TRAFFIC-S | 9 | 10 | 20261102-traffic-A | 2026-11-02T23:31:41+01:00 |
| TRAFFIC | 17 | 19 | 20261102-traffic-B | 2026-11-02T23:31:41+01:00 |
| TRAFFIC-S | 11 | 12 | 20261102-traffic-B | 2026-11-02T23:31:41+01:00 |
