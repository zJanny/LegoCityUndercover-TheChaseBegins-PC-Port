# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| AUDIO | 35 | 36 | 20261021-audio-A | 2026-10-21T22:02:38+02:00 |
| AUDIO-S | 34 | 35 | 20261021-audio-A | 2026-10-21T22:02:38+02:00 |
| AUDIO | 37 | 39 | 20261021-audio-B | 2026-10-21T22:02:38+02:00 |
| AUDIO-S | 36 | 37 | 20261021-audio-B | 2026-10-21T22:02:38+02:00 |
| TRAV | 61 | 62 | 20261021-audio-C | 2026-10-21T22:02:38+02:00 |
| TRAV-S | 36 | 37 | 20261021-audio-C | 2026-10-21T22:02:38+02:00 |
