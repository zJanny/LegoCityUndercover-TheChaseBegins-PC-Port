# Script system: missions, mission 1-4 (the car showroom)

## 1. Scope

Mission 1-4 ("Gone in 60 Minutes", Combat Mechanics) in `chase-game`: `1-4_MissionStart` (spawned by the end of mission 1-3), the waypoint `1-4_Waypoint_01`, `1-4_ShowroomArrival` and the rest of the 29 scripts named `1-4_*` (the fight chain of AI spawners, the roof, the chase, `1-4_MissionEnd`). This file is the gap list of the mission and ranks the gaps for the next rounds; the showroom fight (AI spawners, StartState, the goons' conduct) is in [script-system-missions-1-4-fight](script-system-missions-1-4-fight.md), the roof scene and the natives of the roof and the chase in [script-system-missions-1-4-roof](script-system-missions-1-4-roof.md). The player's attack on characters and the characters' hit reaction are other features (PLAYER-26 to PLAYER-29, COMBAT-1 to COMBAT-6).

## 2. Status

IN_PROGRESS, updated 2026-11-03T08:54:21+01:00. From `1-4_MissionStart` the port runs the first part: the waypoint starts `1-4_ShowroomArrival`, which plays cutscene `1_4_a`, puts the player at `1-4_P1_GoTo`, shows the red-icon hint and 5 s later the counter hint, and ends. The fight that follows (the goons held and released by their AI spawners, thrown, arrested, their idle decision) is in [script-system-missions-1-4-fight](script-system-missions-1-4-fight.md), the roof scene in [script-system-missions-1-4-roof](script-system-missions-1-4-roof.md). No native of the 29 scripts lacks a handler. Ranking of the gaps for the next round: the fight needs the fight state of COMBAT-9 and the grab and counter of COMBAT-10; the roof needs SCRIPT-414, SCRIPT-425 and SCRIPT-426; the chase needs SCRIPT-415 and SCRIPT-422. The analysis is in [scripts-missions-1-4.md §2026-11-01T15:04:22](../../re/scripts-missions-1-4.md#ts-20261101-150422), the four natives in [scripts-missions-1-4.md §2026-11-02T15:04:48](../../re/scripts-missions-1-4.md#ts-20261102-150448) and the roof scene in [scripts-missions-1-4.md §2026-11-02T22:40:54](../../re/scripts-missions-1-4.md#ts-20261102-224054).

## 3. Verified behaviour

- The first part of the mission runs without a native lacking a handler: `1-4_MissionStart` ends at update 30, entering the waypoint from 13 units away starts `1-4_ShowroomArrival` 14 updates later, the cutscene `1_4_a` plays and the script ends with the player at `1-4_P1_GoTo` (577.484, 27.5656, 967.533, level data). The update numbers are the port's, not measured in the original. CTest `game-showroom-1-4` (slow). evidence: [scripts-missions-1-4.md §2026-11-01T15:04:22](../../re/scripts-missions-1-4.md#ts-20261101-150422).
- The gap list is pinned to the level data: 29 scripts named `1-4_*`, that no native lacks a handler, 26 triggers by event, 8 AI spawners, 15 goons, 22 characters and 150 objects named `1-4_*`; CTest `script-census-real-data` fails when a gap is closed so that this file is updated. The four natives pinned in it as lacking a handler (BulletTime, SpawnDebris2Colour, VehicleSetAI, VehicleSetChased) are stand-ins now. evidence: same entry.

## 4. Stand-ins

- None in this file; the fight's are in [script-system-missions-1-4-fight](script-system-missions-1-4-fight.md), the roof's in [script-system-missions-1-4-roof](script-system-missions-1-4-roof.md).

## 5. Open items

- [ ] SCRIPT-415 — The chase: VehicleChaseEnded (`1-4_Stolen_Vehicle` -> `1-4_Car_Crash`, SCRIPT-76) never fires; characters jumping in vehicles (VEH-22); cutscenes `1_4_c` and `1_4_e`; `1-4_BoosterChaser_Spawner`; what arrests `1-4_Booster_Strange_Chaser`. Rank 3. needs: SCRIPT-410, SCRIPT-411, SCRIPT-422, COMBAT-10. evidence: [scripts-missions-1-4.md §2026-11-01T15:04:22](../../re/scripts-missions-1-4.md#ts-20261101-150422).
- [ ] SCRIPT-416 — `1-4_ShowroomArrival` against the oracle: measured; the placement at `1-4_P1_GoTo` and the tip waits match, but the port's cutscene `1_4_a` runs about 0.75 s longer than the original's 17.28 to 17.32 s from the first cut to the placement, and the waypoint start and control change were not compared update by update. evidence: [scripts-missions-1-4.md §2026-11-02T15:54:42](../../re/scripts-missions-1-4.md#ts-20261102-155442).
- [ ] SCRIPT-422 — The stolen car drives: step the traffic AI driver of a chased car (state 5 along `1-4_Vehicle_Path` at speed 29), the route distance, the vehicle's reaction to the requested state 2, the mass scale and the kinematic switch. Rank 3. needs: VEH-12. blocks: SCRIPT-415, SCRIPT-S323. uses: VehicleSetAI, VehicleSetChased. evidence: [scripts-missions-1-4.md §2026-11-02T15:04:48](../../re/scripts-missions-1-4.md#ts-20261102-150448).

## 6. Findings summary

- The arrival scene in the oracle (waypoint start, cutscene, placement, tips, the goons' first attacks): [scripts-missions-1-4.md §2026-11-02T15:54:42](../../re/scripts-missions-1-4.md#ts-20261102-155442).
- Scripts, natives without handler, trigger events, the spawner chain, texts and the class code read in outline: [scripts-missions-1-4.md §2026-11-01T15:04:22](../../re/scripts-missions-1-4.md#ts-20261101-150422).
- The mission 1-2 gap list this one follows: [scripts-missions.md §2026-10-14T21:39:21](../../re/scripts-missions.md#ts-20261014-213921).

## 7. Code and tests

- CTests `game-showroom-1-4` (slow; the walk into the waypoint to the end of the arrival script) and `script-census-real-data` (`tests/script_census_real_test.cpp`), both `needs-game-data`.
