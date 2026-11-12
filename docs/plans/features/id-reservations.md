# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| TRAFFIC | 42 | 44 | 20261111-chase-B | 2026-11-11T23:47:58+01:00 |
| SCRIPT | 446 | 447 | 20261111-chase-B | 2026-11-11T23:47:58+01:00 |
| TRAFFIC-S | 31 | 32 | 20261111-chase-B | 2026-11-11T23:47:58+01:00 |
