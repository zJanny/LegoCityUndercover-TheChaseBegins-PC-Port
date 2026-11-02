# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| COMBAT | 10 | 12 | 20261102-mission14b-B | 2026-11-02T21:59:12+01:00 |
| COMBAT-S | 6 | 7 | 20261102-mission14b-B | 2026-11-02T21:59:12+01:00 |
| SCRIPT | 425 | 426 | 20261102-mission14b-C | 2026-11-02T21:59:12+01:00 |
| SCRIPT-S | 325 | 325 | 20261102-mission14b-C | 2026-11-02T21:59:12+01:00 |
