# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| WINTEST | 3 | 5 | 20261112-winhooks-A | 2026-11-12T21:56:17+01:00 |
| WINTEST | 6 | 7 | 20261112-winhooks-B | 2026-11-12T21:56:17+01:00 |
| WINTEST | 8 | 10 | 20261112-winhooks-C | 2026-11-12T21:56:17+01:00 |
| WINTEST | 11 | 12 | 20261112-winhooks-D | 2026-11-12T21:56:17+01:00 |
| WINTEST | 13 | 14 | 20261112-winhooks-E | 2026-11-12T21:56:17+01:00 |
| WINBUILD | 4 | 7 | 20261112-winhooks-E | 2026-11-12T21:56:18+01:00 |
| WINPLAT | 3 | 4 | 20261112-winhooks-E | 2026-11-12T21:56:18+01:00 |
