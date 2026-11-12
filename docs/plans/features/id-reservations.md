# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| SCRIPT | 458 | 461 | 20261112-enemy-A | 2026-11-12T10:53:39+01:00 |
| SCRIPT-S | 341 | 343 | 20261112-enemy-A | 2026-11-12T10:53:39+01:00 |
| COMBAT | 15 | 16 | 20261112-enemy-A | 2026-11-12T10:53:39+01:00 |
| COMBAT-S | 10 | 10 | 20261112-enemy-A | 2026-11-12T10:53:39+01:00 |
| SCRIPT | 465 | 467 | 20261112-enemy-E | 2026-11-12T10:53:40+01:00 |
| SCRIPT-S | 346 | 347 | 20261112-enemy-E | 2026-11-12T10:53:40+01:00 |
