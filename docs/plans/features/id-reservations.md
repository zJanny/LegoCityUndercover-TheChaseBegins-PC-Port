# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| QOLSET | 99 | 100 | 20261102-qolmenu-A | 2026-11-02T18:12:49+01:00 |
| QOLBOT | 26 | 27 | 20261102-qolmenu-A | 2026-11-02T18:12:49+01:00 |
| QOLCTL | 16 | 19 | 20261102-qolmenu-B | 2026-11-02T18:12:49+01:00 |
| QOLBOT | 28 | 30 | 20261102-qolmenu-C | 2026-11-02T18:12:49+01:00 |
| QOLVD | 1 | 6 | 20261102-qolmenu-E | 2026-11-02T18:21:42+01:00 |
