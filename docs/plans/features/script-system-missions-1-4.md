# Script system: missions, mission 1-4 (the car showroom)

## 1. Scope

Mission 1-4 ("Gone in 60 Minutes", Combat Mechanics) in `chase-game`: `1-4_MissionStart` (spawned by the end of mission 1-3), the waypoint `1-4_Waypoint_01`, `1-4_ShowroomArrival` and the rest of the 29 scripts named `1-4_*` (the fight chain of AI spawners, the roof, the chase, `1-4_MissionEnd`). This file is the gap list of the mission and ranks the gaps for the next rounds. The player's attack on characters and the characters' hit reaction are other features (PLAYER-26 to PLAYER-29, COMBAT-1 to COMBAT-6).

## 2. Status

IN_PROGRESS, updated 2026-11-01T15:05:57+01:00. From `1-4_MissionStart` the port runs the first part: the waypoint starts `1-4_ShowroomArrival`, which plays cutscene `1_4_a`, puts the player at `1-4_P1_GoTo`, shows the red-icon hint and 5 s later the counter hint, and ends. No native of the first part lacks a handler. After that nothing of the fight exists: the AI spawners are not ported, so the 22 characters of the mission stand visible from level load, and no goon attacks, is defeated or becomes arrestable. Of all 29 scripts four natives lack a handler (BulletTime, SpawnDebris2Colour, VehicleSetAI, VehicleSetChased). Ranking of the gaps for the next round: the start of the fight needs SCRIPT-410 and SCRIPT-411 (with the player's attack and the hit reaction of other features); the roof needs SCRIPT-413 and SCRIPT-414; the chase needs SCRIPT-415. The analysis is in [scripts-missions-1-4.md §2026-11-01T15:04:22](../../re/scripts-missions-1-4.md#ts-20261101-150422).

## 3. Verified behaviour

- The first part of the mission runs without a native lacking a handler: `1-4_MissionStart` ends at update 30, entering the waypoint from 13 units away starts `1-4_ShowroomArrival` 14 updates later, the cutscene `1_4_a` plays and the script ends with the player at `1-4_P1_GoTo` (577.484, 27.5656, 967.533, level data). The update numbers are the port's, not measured in the original. CTest `game-showroom-1-4` (slow). evidence: [scripts-missions-1-4.md §2026-11-01T15:04:22](../../re/scripts-missions-1-4.md#ts-20261101-150422).
- The gap list is pinned to the level data: 29 scripts named `1-4_*`, the four natives without handler, 26 triggers by event, 8 AI spawners, 15 goons, 22 characters and 150 objects named `1-4_*`; CTest `mission-1-4-census-real-data` fails when a gap is closed so that this file is updated. evidence: same entry.

## 4. Stand-ins

- SCRIPT-S320 The characters of mission 1-4 (15 goons, two Booster Strange, the chaser, the end robber, the salesman, two cops) stand in place from level load, as the level data places them; the original holds them until their AI spawner is triggered. evidence: [scripts-missions-1-4.md §2026-11-01T15:04:22](../../re/scripts-missions-1-4.md#ts-20261101-150422).

## 5. Open items

- [ ] SCRIPT-410 — The AI spawner class (kind 14, `props/prop_aispawner`): characters held from level load (StartSpawned 0), released when the spawner is triggered (TriggerObject, message 255), and when all are done the spawner fires its `TriggerObject` reference (the chain of the mission is in the linked entry). The spawn conditions, what makes a slot done and the counters are not read. Blocks the start of the fight (rank 1). needs: SCRIPT-359. blocks: SCRIPT-411, SCRIPT-414, SCRIPT-415, SCRIPT-417. evidence: [scripts-missions-1-4.md §2026-11-01T15:04:22](../../re/scripts-missions-1-4.md#ts-20261101-150422).
- [ ] SCRIPT-411 — The trigger events CharacterArrestable (goons 03 and 06, the hint "Press A to arrest a criminal.") and CharacterArrested (`Booster_Strange_Chaser`, `Booster_Strange_02`); the fire sites are located in the executable, not read. Part of SCRIPT-75. Rank 1 for the arrest hint, rank 3 for the chase end. needs: SCRIPT-410. blocks: SCRIPT-415. evidence: [scripts-missions-1-4.md §2026-11-01T15:04:22](../../re/scripts-missions-1-4.md#ts-20261101-150422).
- [ ] SCRIPT-412 — The goons' conduct in the fight: the GoTo targets of the spawner (`1-4_AI_Goon_NN_GoTo`), the range bound `1-4_AIRange_01` and the goon's own bound parameter, CharacterInvulnerable (flag recorded only), the sub-objective arrows, the 600 and 700 score popups the user saw; the attack, defeat and arrest are PLAYER-26 to PLAYER-29 and COMBAT-1 to COMBAT-6. Rank 1. evidence: [scripts-missions-1-4.md §2026-11-01T15:04:22](../../re/scripts-missions-1-4.md#ts-20261101-150422).
- [ ] SCRIPT-413 — BulletTime (`1-4_SO_Phase2` calls it with 0.01 / 0.1 / 0 and 1 / 0.01 / 0): no handler; the time scale record the handler fills and its readers were not read. Rank 2 (the roof scene). uses: BulletTime. evidence: [scripts-missions-1-4.md §2026-11-01T15:04:22](../../re/scripts-missions-1-4.md#ts-20261101-150422).
- [ ] SCRIPT-414 — The roof scene objects: the ladder kick (`LadderKick` / `LadderLand` particle objects, `1-4_Showroom_Ladder` and its platform), Booster Strange's `AISuperJump` flights (height is a stand-in, SCRIPT-103), the reader of `FallArrestBound01..07` and `FallArrestBoundSystem`, ShowCharacterIntro against the oracle (HUD-30), `1-4_SO_LadderFallen`, the music scripts and `sfx_rooftime`. Rank 2. needs: SCRIPT-410. evidence: [scripts-missions-1-4.md §2026-11-01T15:04:22](../../re/scripts-missions-1-4.md#ts-20261101-150422).
- [ ] SCRIPT-415 — The chase: VehicleSetAI, VehicleSetChased (`1-4_Vehicle_Path`) and SpawnDebris2Colour have no handler; VehicleChaseEnded (`1-4_Stolen_Vehicle` -> `1-4_Car_Crash`, SCRIPT-76) never fires; characters jumping in vehicles (VEH-22); cutscenes `1_4_c` and `1_4_e`; `1-4_BoosterChaser_Spawner`. Rank 3. needs: SCRIPT-410, SCRIPT-411. uses: VehicleSetAI, VehicleSetChased, SpawnDebris2Colour. evidence: [scripts-missions-1-4.md §2026-11-01T15:04:22](../../re/scripts-missions-1-4.md#ts-20261101-150422).
- [ ] SCRIPT-416 — `1-4_ShowroomArrival` against the oracle (cutscene length, the player's placement, the tip times). Measure the waypoint start, the cutscene length, the placement and the two tip times. evidence: [scripts-missions-1-4.md §2026-11-01T15:04:22](../../re/scripts-missions-1-4.md#ts-20261101-150422).
- [ ] SCRIPT-417 — Who triggers `1-4_Spawner_01` (goons 01, 02) and `1-4_Pursuit_Booster_Spawner` (`Booster_Strange_02`): no script of lvl_01 does; the four AIMove triggers and the bound trigger on `1-4_AI_HeadRobber` can never fire (no such object). needs: SCRIPT-410. evidence: [scripts-missions-1-4.md §2026-11-01T15:04:22](../../re/scripts-missions-1-4.md#ts-20261101-150422).

## 6. Findings summary

- Scripts, natives without handler, trigger events, the spawner chain, texts and the class code read in outline: [scripts-missions-1-4.md §2026-11-01T15:04:22](../../re/scripts-missions-1-4.md#ts-20261101-150422).
- The mission 1-2 gap list this one follows: [scripts-missions.md §2026-10-14T21:39:21](../../re/scripts-missions.md#ts-20261014-213921).

## 7. Code and tests

- No new runtime code. CTests `game-showroom-1-4` (slow; the walk into the waypoint to the end of the arrival script) and `mission-1-4-census-real-data` (`tests/mission14_census_real_test.cpp`), both `needs-game-data`.
