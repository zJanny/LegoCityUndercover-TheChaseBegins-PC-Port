# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| WINTEST | 14 | 16 | 20261113-winopen-A | 2026-11-13T00:30:23+01:00 |
| WINPLAT | 5 | 6 | 20261113-winopen-B | 2026-11-13T00:30:23+01:00 |
| WINPLAT | 7 | 8 | 20261113-winopen-C | 2026-11-13T00:30:23+01:00 |
| WINBUILD | 7 | 8 | 20261113-winopen-G | 2026-11-13T00:30:24+01:00 |
| WINTEST | 23 | 23 | 20261113-winopen-G | 2026-11-13T00:30:24+01:00 |
