# Script system: missions, mission 1-4 (the car showroom)

## 1. Scope

Mission 1-4 ("Gone in 60 Minutes", Combat Mechanics) in `chase-game`: `1-4_MissionStart` (spawned by the end of mission 1-3), the waypoint `1-4_Waypoint_01`, `1-4_ShowroomArrival` and the rest of the 29 scripts named `1-4_*` (the fight chain of AI spawners, the roof, the chase, `1-4_MissionEnd`). This file is the gap list of the mission and ranks the gaps for the next rounds; the showroom fight (AI spawners, StartState) is in [script-system-missions-1-4-fight](script-system-missions-1-4-fight.md), the goons' conduct in [script-system-missions-1-4-conduct](script-system-missions-1-4-conduct.md), the roof scene and the natives of the roof and the chase in [script-system-missions-1-4-roof](script-system-missions-1-4-roof.md), the chase in [script-system-missions-1-4-chase](script-system-missions-1-4-chase.md). The player's attack on characters and the characters' hit reaction are other features (PLAYER-26 to PLAYER-29, COMBAT-1 to COMBAT-6).

## 2. Status

IN_PROGRESS, updated 2026-11-12T11:30:29+01:00. From `1-4_MissionStart` the port runs the first part: the waypoint starts `1-4_ShowroomArrival`, which plays cutscene `1_4_a`, puts the player at `1-4_P1_GoTo`, shows the red-icon hint and 5 s later the counter hint, and ends. The cutscene's length and the time before its first cut match the oracle's free run. The chase is in [script-system-missions-1-4-chase](script-system-missions-1-4-chase.md). The fight that follows (the goons held and released by their AI spawners, thrown, arrested, their idle decision) is in [script-system-missions-1-4-fight](script-system-missions-1-4-fight.md), the roof scene in [script-system-missions-1-4-roof](script-system-missions-1-4-roof.md). No native of the 29 scripts lacks a handler. Ranking of the gaps for the next round: the fight needs the strike of COMBAT-15 and the grab and counter of COMBAT-10; the roof needs SCRIPT-414, SCRIPT-425 and SCRIPT-426; the chase needs SCRIPT-415, SCRIPT-443, SCRIPT-445, SCRIPT-457, SCRIPT-447 and SCRIPT-462. The analysis is in [scripts-missions-1-4.md §2026-11-01T15:04:22](../../re/scripts-missions-1-4.md#ts-20261101-150422), the four natives in [scripts-missions-1-4.md §2026-11-02T15:04:48](../../re/scripts-missions-1-4.md#ts-20261102-150448) and the roof scene in [scripts-missions-1-4.md §2026-11-02T22:40:54](../../re/scripts-missions-1-4.md#ts-20261102-224054).

## 3. Verified behaviour

- The first part of the mission runs without a native lacking a handler: `1-4_MissionStart` ends at update 30, entering the waypoint from 13 units away starts `1-4_ShowroomArrival` 14 updates later, the cutscene `1_4_a` plays and the script ends with the player at `1-4_P1_GoTo` (577.484, 27.5656, 967.533, level data). The update numbers are the port's, not measured in the original. CTest `game-showroom-1-4` (slow). evidence: [scripts-missions-1-4.md §2026-11-01T15:04:22](../../re/scripts-missions-1-4.md#ts-20261101-150422).
- The cutscene `1_4_a` plays 507 updates of 0.033 s from its first cut, as in the stepped oracle run; with the fade of flag a, the end fade and the exit wait it lasts 545 updates from DCamCutScenePlay, and from the first cut to the placement it is within two updates of the free oracle run. CTest `natives-cutscene-real-data`. evidence: [scripts-missions-1-4-spawners.md §2026-11-07T12:08:43](../../re/scripts-missions-1-4-spawners.md#ts-20261107-120843).
- The gap list is pinned to the level data: 29 scripts named `1-4_*`, that no native lacks a handler, 26 triggers by event, 8 AI spawners, 15 goons, 22 characters and 150 objects named `1-4_*`; CTest `script-census-real-data` fails when a gap is closed so that this file is updated. The four natives pinned in it as lacking a handler (BulletTime, SpawnDebris2Colour, VehicleSetAI, VehicleSetChased) are stand-ins now. evidence: same entry.



## 4. Stand-ins

- SCRIPT-S337 The 0.19 s between DCamCutSceneLoad and DCamCutScenePlay and the 0.1 s of the mode switch after the fade (placed as a block of DCamCutScenePlay, which does not block in the original) are measured on `1_4_a` only and applied to every cutscene; the first cut after the mode enter is not modelled. evidence: [scripts-missions-1-4-spawners.md §2026-11-08T12:09:48](../../re/scripts-missions-1-4-spawners.md#ts-20261108-120948)
- The chase's are in [script-system-missions-1-4-chase](script-system-missions-1-4-chase.md); the fight's are in [script-system-missions-1-4-fight](script-system-missions-1-4-fight.md), the roof's in [script-system-missions-1-4-roof](script-system-missions-1-4-roof.md).

## 5. Open items

- None in this file; the chase's are in [script-system-missions-1-4-chase](script-system-missions-1-4-chase.md), the fight's, the goons' conduct and the roof's in their files.

## 6. Findings summary

- The arrival scene in the oracle (waypoint start, cutscene, placement, tips, the goons' first attacks): [scripts-missions-1-4.md §2026-11-02T15:54:42](../../re/scripts-missions-1-4.md#ts-20261102-155442).
- The time before the first cut in a free run (Load 0.03 s, Play 0.23 s, mode enter 0.84 to 0.87 s): [scripts-missions-1-4-spawners.md §2026-11-08T12:09:48](../../re/scripts-missions-1-4-spawners.md#ts-20261108-120948).
- The cutscene `1_4_a` in updates against the stepped and the free oracle run: [scripts-missions-1-4-spawners.md §2026-11-07T12:08:43](../../re/scripts-missions-1-4-spawners.md#ts-20261107-120843).
- Scripts, natives without handler, trigger events, the spawner chain, texts and the class code read in outline: [scripts-missions-1-4.md §2026-11-01T15:04:22](../../re/scripts-missions-1-4.md#ts-20261101-150422).
- The mission 1-2 gap list this one follows: [scripts-missions.md §2026-10-14T21:39:21](../../re/scripts-missions.md#ts-20261014-213921).

## 7. Code and tests

- CTests `game-chase-1-4` (slow; the hatch script to the stolen car driving off), `game-showroom-1-4` (slow; the walk into the waypoint to the end of the arrival script), `script-census-real-data` (`tests/script_census_real_test.cpp`) and `natives-cutscene-real-data` (`tests/natives_cutscene_real_test.cpp`, the update counts of `1_4_a`), all `needs-game-data`.
