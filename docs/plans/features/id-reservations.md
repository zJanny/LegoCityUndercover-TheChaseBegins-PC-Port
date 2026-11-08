# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| SCRIPT | 443 | 445 | 20261108-traffic-C | 2026-11-08T11:52:27+01:00 |
| SCRIPT-S | 336 | 337 | 20261108-traffic-C | 2026-11-08T11:52:27+01:00 |
