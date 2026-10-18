# Closed items of player-traversal

Done items of [player-traversal.md](player-traversal.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] TRAV-3 — Rails and climb bars (type 29, corner type 218, CLIMB BAR states 89..92) ported for the route of m12-12; the variants are TRAV-27 to TRAV-33. evidence: [player-traversal.md §2026-10-18T16:05:03](../../re/player-traversal.md#ts-20261018-160503)
- [x] TRAV-4 — Ledge states 23 and 24 are only the `Ledge` bar variant (jump off a bar with the flag, state 24); the rails of mission 1-2 do not have it. evidence: [player-traversal.md §2026-10-18T16:05:03](../../re/player-traversal.md#ts-20261018-160503)
- [x] TRAV-30 — closed 2026-10-18: with the bricks and the rails chained in the player step, the jump off `1-2_Brick_Grab_07` grabs `1-2_ClimbBar_02` 7 updates after the B press in `chase-game`, as in the oracle (test `game-script-brick-rail`). evidence: [player-traversal.md §2026-10-18T15:57:46](../../re/player-traversal.md#ts-20261018-155746).
