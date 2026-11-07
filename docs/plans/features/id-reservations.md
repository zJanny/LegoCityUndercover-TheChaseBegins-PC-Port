# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| TRAFFIC | 34 | 36 | 20261107-traffic2-B | 2026-11-07T18:05:00+01:00 |
| TRAFFIC-S | 28 | 30 | 20261107-traffic2-B | 2026-11-07T18:05:00+01:00 |
| SCRIPT | 441 | 443 | 20261107-traffic2-C | 2026-11-07T18:05:00+01:00 |
| SCRIPT-S | 333 | 335 | 20261107-traffic2-C | 2026-11-07T18:05:00+01:00 |
