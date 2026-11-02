# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| COMBAT | 7 | 9 | 20261102-mission14-B | 2026-11-02T14:46:05+01:00 |
| COMBAT-S | 4 | 5 | 20261102-mission14-B | 2026-11-02T14:46:05+01:00 |
| QOLCTL | 13 | 14 | 20261102-qolctl2-A | 2026-11-02T14:47:45+01:00 |
| QOLCTL-S | 4 | 4 | 20261102-qolctl2-A | 2026-11-02T14:47:45+01:00 |
