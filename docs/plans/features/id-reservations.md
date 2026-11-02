# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| SCRIPT | 418 | 420 | 20261102-mission14-A | 2026-11-02T14:46:05+01:00 |
| SCRIPT-S | 321 | 322 | 20261102-mission14-A | 2026-11-02T14:46:05+01:00 |
| COMBAT | 7 | 9 | 20261102-mission14-B | 2026-11-02T14:46:05+01:00 |
| COMBAT-S | 4 | 5 | 20261102-mission14-B | 2026-11-02T14:46:05+01:00 |
| SCRIPT | 421 | 422 | 20261102-mission14-C | 2026-11-02T14:46:05+01:00 |
| SCRIPT-S | 323 | 323 | 20261102-mission14-C | 2026-11-02T14:46:05+01:00 |
| PLAYER | 30 | 32 | 20261102-mission14-D | 2026-11-02T14:46:05+01:00 |
| PLAYER-S | 15 | 16 | 20261102-mission14-D | 2026-11-02T14:46:05+01:00 |
| QOLCTL | 13 | 14 | 20261102-qolctl2-A | 2026-11-02T14:47:45+01:00 |
| QOLCTL-S | 4 | 4 | 20261102-qolctl2-A | 2026-11-02T14:47:45+01:00 |
| QOLCTL | 17 | 17 | 20261102-qolctl2-C | 2026-11-02T14:47:45+01:00 |
| QOLBOT | 24 | 26 | 20261102-qolctl2-D | 2026-11-02T14:47:45+01:00 |
| QOLBOT-S | 2 | 2 | 20261102-qolctl2-D | 2026-11-02T14:47:45+01:00 |
| QOLFS | 23 | 23 | 20261102-qolctl2-F | 2026-11-02T14:47:45+01:00 |
| QOLSET | 99 | 100 | 20261102-qolctl2-F | 2026-11-02T14:47:45+01:00 |
