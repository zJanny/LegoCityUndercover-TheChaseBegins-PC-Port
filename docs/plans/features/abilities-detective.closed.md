# Closed items of abilities-detective

Done items of [abilities-detective.md](abilities-detective.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] DETECT-2 — closed 2026-10-18: PlayCharAnim now puts a level character into the original's user-animation state, so the finished `dumpster_exit_minifig` is seen by `WaitForCurrentCharAnim` and the Doorlock script ends (update 1064 from the alley); the first point is enabled (test `game-doorlock-detective-1-2`). evidence: [npc-animation.md §2026-10-18T14:28:08](../../re/npc-animation.md#ts-20261018-142808).
- [x] DETECT-7 — closed 2026-10-18: the glass is drawn from the original's data (object, bone, track) and its first update matches the oracle; the look differences are DETECT-9 to DETECT-11. evidence: [abilities-detective.md §2026-10-18T19:30:00](../../re/abilities-detective.md#ts-20261018-193000).
- [x] DETECT-14 — The footprints hover on the dog house stairs (the player ground probe stood up to 0.24 too high; now the ray test, equal to the oracle's hit points) and are placed with the stored path orientation; at the later spot the original draws them unrotated too (identity quaternions, oracle E-2), so the reported wrong direction is not the rotation (DETECT-16). evidence: [abilities-detective.md §2026-10-19T09:11:04](../../re/abilities-detective.md#ts-20261019-091104).
