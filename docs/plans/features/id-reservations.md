# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| SCRIPT | 377 | 379 | 20261026-crane-A | 2026-10-26T22:22:26+01:00 |
| SCRIPT-S | 301 | 302 | 20261026-crane-A | 2026-10-26T22:22:26+01:00 |
