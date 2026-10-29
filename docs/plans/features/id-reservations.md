# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| QOLSET | 22 | 23 | 20261029-qolwire-A | 2026-10-29T10:26:07+01:00 |
| QOLSET | 26 | 27 | 20261029-qolwire-C | 2026-10-29T10:26:07+01:00 |
| QOLSET-S | 1 | 1 | 20261029-qolwire-A | 2026-10-29T10:26:07+01:00 |
