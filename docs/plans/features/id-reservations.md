# ID reservations

Ranges of item IDs held by parallel agents, named like `20261021-audio-A`. The planner reserves them with `tools/check_doc_links.py --reserve PREFIX COUNT WHO`; `tools/merge_agent.py finish --release WHO` drops them in the merge commit. `--next` skips reserved ranges.

| Prefix | First | Last | Holder | Date |
|---|---|---|---|---|
| TOOLS | 21 | 23 | 20261025-recorded-A | 2026-10-25T14:08:22+01:00 |
| TOOLS-S | 3 | 4 | 20261025-recorded-A | 2026-10-25T14:08:22+01:00 |
| PED | 27 | 29 | 20261025-recorded-B | 2026-10-25T14:08:22+01:00 |
| PED-S | 15 | 16 | 20261025-recorded-B | 2026-10-25T14:08:22+01:00 |
| SCRIPT | 362 | 365 | 20261025-recorded-C | 2026-10-25T14:08:22+01:00 |
| SCRIPT-S | 291 | 292 | 20261025-recorded-C | 2026-10-25T14:08:22+01:00 |
| LEVOBJ | 127 | 128 | 20261025-recorded-C | 2026-10-25T14:08:22+01:00 |
| LEVOBJ-S | 99 | 99 | 20261025-recorded-C | 2026-10-25T14:08:23+01:00 |
