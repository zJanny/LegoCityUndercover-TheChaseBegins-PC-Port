# Script system: AI movement and navigation

## 1. Scope

How an AI character moves for the script natives: the AIMove path finder and follower on the nav grids (`.bfnnav`), the simple path state of AIFollowSimplePath, the go-to-target state of AISuperJump, the speeds, and the AIMove trigger event they raise. Other AI behaviour (pursuit, stalking, fleeing, weapons) stays in [script-system-natives.md](script-system-natives.md) (SCRIPT-42).

## 2. Status

IN_PROGRESS, updated 2026-10-14T20:01:16+02:00. The path finder 0x204284 with its grid search, exit cell choice and cell search is transcribed (`nav_path.*`, `nav_grid.*`), and so is the in-grid follower with the arrival of a plain AIMove (`nav_follow.*`): `AiCore` runs them for an AIMove when the host gives it the level's nav grids (`AiCore::setNavigation`); result 1 refuses the order. Paths across grids and followers that cannot continue still walk straight (SCRIPT-S31). The level loader does not give the nav grids to `chase-game` yet (SCRIPT-94). State 11 (arrival and heading, `ai_go_to_target.*`) is transcribed but AISuperJump does not use it yet (SCRIPT-96).

## 3. Verified behaviour

- Lookup 0x1c2ccc / 0x20d8a8, the finder, the search steps and the in-grid follower match the disassembly in hand-evaluated tests on made-up grids (`nav-grid`, `nav-path`, `ai-core`); the synthetic cases were not compared with the original. The police entry AIMove on the real grids matches an oracle read of the original's path record (grid 26, cells 39 / 143, phase 2, 37 open cells, the cell chain and costs, the first waypoint): [ai-navigation.md §2026-10-14T20:01](../../re/ai-navigation.md#ts-20261014-200116) (`nav-path-real-data`).
- State 22 (AIFollowSimplePath): arrival within 0.5, index rules and event argument 255 follow the original ([ai-natives.md §2026-10-14T18:42](../../re/ai-natives.md#ts-20261014-184240)).
- Findings of the finder, follower and state 11: [ai-navigation.md §2026-10-14T19:47](../../re/ai-navigation.md#ts-20261014-194710).

## 4. Stand-ins

- SCRIPT-S31 Every AI move runs at a measured stand-in speed (player 0.11 per update walk, 0.23 run). AIMove across grids, or when the follower cannot continue, is a straight walk ending on the target; the event argument is component byte 0 stood in by 0; pursuit, stalk, flee, jump, face state and the AIEnable party loop only record. needs: SCRIPT-42, SCRIPT-79, SCRIPT-92, SCRIPT-93. evidence: [ai-natives.md §2026-10-14T18:42](../../re/ai-natives.md#ts-20261014-184240).
- SCRIPT-S93 The nav section the finder works on is the first one whose lookup finds the start (the original keeps one in global 0x376bec `+4`; its writer is not found). needs: SCRIPT-94. evidence: [ai-navigation.md §2026-10-14T19:47](../../re/ai-navigation.md#ts-20261014-194710).
- SCRIPT-S94 The lookup height tolerance (entity float `+0x70`, stood in as 0.5 doubled) and the gate masks `+0x74` / `+0x78` (stood in as bit 0 only, the original adds the registered gate types of table 0x376f10) are not the original's. needs: SCRIPT-93, SCRIPT-94. evidence: [ai-navigation.md §2026-10-14T19:47](../../re/ai-navigation.md#ts-20261014-194710).
- SCRIPT-S95 The obstacle pass 0x2af1cc (run by the follower while `+0x7c` bit 0 is clear, so for a plain AIMove) and the 200-tick budget of the cell search are skipped; the search always finishes. needs: SCRIPT-95. evidence: [ai-navigation.md §2026-10-14T19:47](../../re/ai-navigation.md#ts-20261014-194710).

## 5. Open items

- [ ] SCRIPT-79 — AI movement: states 22, the finder and the in-grid follower are ported; AIMove across grids (SCRIPT-93), the nav inputs (SCRIPT-94), the obstacle pass (SCRIPT-95), state 11 (SCRIPT-96) and the non-player speeds (SCRIPT-92) are open. needs: SCRIPT-92, SCRIPT-93, SCRIPT-94, SCRIPT-95, SCRIPT-96. blocks: SCRIPT-S31, SCRIPT-75. evidence: [ai-navigation.md §2026-10-14T19:47](../../re/ai-navigation.md#ts-20261014-194710).
- [ ] SCRIPT-92 — Non-player walk and run speed come from locomotion state 72 (0x2414b4 -> animation 0x2eafc8) and the character's class, not from the AI states; the stand-in speed is the player's. blocks: SCRIPT-S31, SCRIPT-79. evidence: [ai-natives.md §2026-10-14T18:42](../../re/ai-natives.md#ts-20261014-184240).
- [ ] SCRIPT-93 — The follower for a path across grids: the transition 0x272598 (cell across the link, phase 3), the phase 3 and 4 branches of 0x1faf04 (0x1fb104, 0x1faf70), the rebuild at 0x27e12c, the codes 5 and above of 0x1fae48 with the gate handlers 0x2684b8 and the gate table 0x376f10 (which abilities open which gate), and 0x1fae48's turn and walk (0x1a6544). About nine of ten paths found between nearby positions of lvl_01 cross a grid. needs: SCRIPT-94. blocks: SCRIPT-S31, SCRIPT-79, SCRIPT-S94. evidence: [ai-navigation.md §2026-10-14T19:47](../../re/ai-navigation.md#ts-20261014-194710).
- [ ] SCRIPT-94 — Give the level's nav grids to `AiCore` (`navWorldsFromLevel`, the `.bfnnav` of each room) in `chase-game`, find which section record the original keeps active, the class parameter `+0x70`, the character's gate mask and component byte 0 (event start argument). needs: SCRIPT-93. blocks: SCRIPT-79, SCRIPT-S93, SCRIPT-S94. evidence: [ai-navigation.md §2026-10-14T19:47](../../re/ai-navigation.md#ts-20261014-194710).
- [ ] SCRIPT-95 — The obstacle pass 0x2af1cc (entities near the path mark cells as kind 2; callbacks, `+0x7f`) and the stopwatch budget of 0x208338 / 0x27ddb4. blocks: SCRIPT-79, SCRIPT-S95. evidence: [ai-navigation.md §2026-10-14T19:47](../../re/ai-navigation.md#ts-20261014-194710).
- [ ] SCRIPT-96 — AISuperJump: wire state 11 (`stepGoToTarget`) into `AiCore`, the flight setup 0x213300 (frames, horizontal speed `+0x308`, climb `+0x30c`, locomotion 72), MinDistance `+0x8c` and TurnSpeed class values, the follow-up lookup 0x1f8d5c shared by the arrival sites 0x2c3c68 and 0x2c630c. needs: SCRIPT-92. blocks: SCRIPT-79. evidence: [ai-navigation.md §2026-10-14T19:47](../../re/ai-navigation.md#ts-20261014-194710).

## 6. Findings summary

- Finder, grid and cell search, exit cell, follower, caller 0x2c5d3c, front end flags, state 11: [ai-navigation.md §2026-10-14T19:47](../../re/ai-navigation.md#ts-20261014-194710).
- State table, AIFollowSimplePath, the AIMove front end 0x22d85c ([corrected](../../re/ai-navigation.md#ts-20261014-194710) in the phase table and the avoid bit): [ai-natives.md §2026-10-14T18:42](../../re/ai-natives.md#ts-20261014-184240).
- Nav grid file format: [levels.md §2026-10-12T23:55](../../re/levels.md#ts-20261012-2355).

## 7. Code and tests

- `src/runtime/nav_grid.*` (lookups, `navWorldsFromLevel`), `nav_path.*` (finder), `nav_follow.*` (follower), `ai_go_to_target.*` (state 11), `ai.*` (`AiCore::setNavigation`, `startNavMove`, `advanceNavMove`), the nav decoder additions in `src/assets/nav.cpp` and `level.cpp` (`NavGrid` extras and flags, `NavRoom` base, end, section).
- CTest: `nav-grid`, `nav-path`, `nav-path-real-data`, `ai-core`, `ai-real-data`, `assets-real-data`.
