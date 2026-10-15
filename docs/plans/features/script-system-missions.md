# Script system: missions

## 1. Scope

What a whole mission needs from the script system beyond natives and trigger events that have their own files: the object classes that start or trigger mission scripts, scripts without a known starter, and the mission-level comparison with the original. Mission 1-2 is the first one analysed. General items are in [script-system.md](script-system.md), natives in [script-system-natives.md](script-system-natives.md), the trigger pass in [script-system-triggers.md](script-system-triggers.md).

## 2. Status

IN_PROGRESS, updated 2026-10-15T11:12:11+02:00. Mission 1-2 was compared with the original (gap list in [scripts-missions.md](../../re/scripts-missions.md)): its scripts run in the port (`1-2_MissionStart` reaches its end one update later than the original), 13 of its 65 functions have no handler and 12 of its 22 triggers are fired by object classes the port does not have. Nothing starts a mission by play yet: the waypoint class that starts `1-2_MissionStart` is missing.

## 3. Verified behaviour

- In the original `1-2_MissionStart` is started by the update of the waypoint class (not by a trigger or an event) when the player reaches the waypoint; the natives then run in the order of the script, `DestroyProp` after the 5 s title camera and `PlayCharAnim` after the following `Wait 5`: [scripts-missions.md §2026-10-14T21:39](../../re/scripts-missions.md#ts-20261014-213921).

## 4. Stand-ins

- SCRIPT-S141 Waypoint bound is turned by the yaw of `initialorientation` only (Euler order unknown, lvl_01 waypoints have no pitch or roll); while TakePlayerControl holds, the port zeroes stick and jump, the original keeps the player almost still (drift up to 0.03 in 13 updates, cause of the +0xd8 bit 4 effect not read). evidence: [scripts-missions.md §2026-10-14T22:48](../../re/scripts-missions.md#ts-20261014-224825).
- [x] SCRIPT-S111 Missions were started only by `--call-script NAME` or a script's own `CallScript` / `SpawnScript`; waypoints now start them (waypoints.cpp). needs: SCRIPT-110. evidence: [scripts-missions.md §2026-10-14T21:39](../../re/scripts-missions.md#ts-20261014-213921).

- SCRIPT-S220 The reference-trigger join (reference_trigger.cpp) knows only the `TriggerObject` references of build-its; the original joins over all references registered in its list (0x591d88), so a reference of another class to the same target would be missed. needs: SCRIPT-220. evidence: [buildit.md §2026-10-15T13:07](../../re/buildit.md#ts-20261015-130720).

## 5. Open items

- [ ] SCRIPT-190 — Carry gaps: the instigator argument of 0x244f40 is not modelled; the handler 0x1bc4d4 forwards message 0xff to child entities (class not identified, maybe `scriptobject`); what raises respawn message 0x80000003 (0x1769a0) and the other carry classes of lvl_01 (`TargetObject01..04`, LEVOBJ-33) were not read (PlaceObject is done, the message 8 caller is SCRIPT-221). blocks: SCRIPT-111. evidence: [carrying.md §2026-10-15T13:08](../../re/carrying.md#ts-20261015-130800).
- [ ] SCRIPT-141 — Waypoint variants the port cannot exercise (vehicle waypoints, VehicleString / VehicleType matching, the vehicle second test and the hint are done: [vehicles-waypoints.md](../../re/vehicles-waypoints.md#ts-20261015-113500)): UseMarker waypoints (kind 2, `map_useobjectwaypoint`), `NextWayPoint`, the badge child and the receivers of messages 52 / 53 / 0x2e5. needs: SCRIPT-S29. evidence: [scripts-missions.md §2026-10-14T22:48](../../re/scripts-missions.md#ts-20261014-224825).
- [ ] SCRIPT-142 — Order of the waypoint update against the trigger pass and the runner inside one update is not measured (the port updates waypoints after `checkTriggers`; thread creation to first executor pass matches one update). evidence: [scripts-missions.md §2026-10-14T22:48](../../re/scripts-missions.md#ts-20261014-224825).
- [ ] SCRIPT-111 — Class-fired `ObjectTriggered` events: of the 22 triggers of mission 1-2, 12 belong to `scriptobject` objects and no script calls `TriggerObject` on them; scanner (`Scanned_Pringles`, `Scanner_Enter_Script`, `Scanner_Zoomed_Script`, `Scanner_Locked_Script`), detective spots (`Dtv_DogSpot_01_Script`, `Reveal_Bone_Script`, `SO_Dspot_02_X`), the cage use spot (`Cage_Key_Target_Script`) and the camera off script are fired by classes (`prop_scannerzoneaudio`, `ability_detective*`, `prop_simpleuse`, not traced). needs: SCRIPT-75, SCRIPT-170, SCRIPT-180. blocks: SCRIPT-1. evidence: [scripts-missions.md §2026-10-14T21:39](../../re/scripts-missions.md#ts-20261014-213921).
- [ ] SCRIPT-220 — Reference-trigger step 0x23211c, not ported: the owner event `*0x3775d8` and deactivate mode (rebuild, `*0x3775dc`, ObjectUntriggered), message 10 to the target and message 0xff to the owners, the other classes that call it (about 30 sites). blocks: SCRIPT-S220, SCRIPT-111. evidence: [buildit.md §2026-10-15T13:07](../../re/buildit.md#ts-20261015-130720).
- [ ] SCRIPT-221 — The build-it state change sends message 8 (reset) to carry objects near its final prop (flag `SpawnCheck`), which respawns them and fires `TriggerOnRespawn` (0x176d40); the port does not (sphere radius of 0x237f30 not read). evidence: [carrying.md §2026-10-15T13:08](../../re/carrying.md#ts-20261015-130800).
- [ ] SCRIPT-113 — Two mission 1-2 scripts have no known starter (`2451216995`, `2494968948`); the other two are named by waypoint `TriggerScript` parameters (`1-2_HouseArrival`, `1-2_Spawn_Doorlock`) and start with SCRIPT-110. blocks: SCRIPT-1. evidence: [scripts-missions.md §2026-10-14T21:44](../../re/scripts-missions.md#ts-20261014-214429).
- [ ] SCRIPT-114 — The port runs `1-2_MissionStart` one update later than the original at the end of the title camera (DestroyProp in update 154 after the first call, original 153; PlayCharAnim 306 against 305, 0.033 s updates, harness with the director updated after the runner); the game loop order was not checked. needs: SCRIPT-50. evidence: [scripts-missions.md §2026-10-14T21:39](../../re/scripts-missions.md#ts-20261014-213921).
- [ ] SCRIPT-115 — Only the scripted start of mission 1-2 (330 updates) was traced; the free-play parts (bounds `ShowTheDog`, `Prop_Smash_Hint`, the detective spots, `Waypoint_02_Script`, `MissionEnd` with the dog truck) were not walked in the oracle and need states made by the user (position, mission progress after each part). evidence: [scripts-missions.md §2026-10-14T21:39](../../re/scripts-missions.md#ts-20261014-213921).

- [ ] SCRIPT-200 — The mission 1-1 run (`game-mission-1-1`) replaces walks and the drive to room 33 by dev teleports (`at X Y Z`); real walking along the original's paths, the police car drive and the timing between the stops are not compared with the original. needs: VEH-6. evidence: [scripts-missions.md §2026-10-15T12:41](../../re/scripts-missions.md#ts-20261015-124140).
- [ ] SCRIPT-201 — Mission 1-1 calls natives with no handler (CameraFollow, EnableTrafficAndPedestrians, OverrideMusic, AutomaticMusic) and a PositionObject of a non-player object (`1-1_Police_Car` to `1-1_Dcam_Exclusion_Police_Car_Resnap` at the end of `1-1_Release_Car_Script`) that the port ignores. evidence: [scripts-missions.md §2026-10-15T12:41](../../re/scripts-missions.md#ts-20261015-124140).

## 6. Findings summary

- Waypoint class: states, reach test, timings and oracle numbers: [scripts-missions.md §2026-10-14T22:48](../../re/scripts-missions.md#ts-20261014-224825).
- Mission 1-2 gap analysis: natives without a handler and their items, stand-ins, trigger events, the oracle trace of the start: [scripts-missions.md §2026-10-14T21:39](../../re/scripts-missions.md#ts-20261014-213921).
- Natives that block mission 1-2 and have items: DestroyProp SCRIPT-58, EffectFadeObject / StreamRoomIn SCRIPT-38, SetPartyCharacter SCRIPT-35, DCamCutScene* SCRIPT-29, EnableDistanceFade SCRIPT-57, music natives SCRIPT-33, SetForcedStopOnMyVehicle SCRIPT-36; AI natives and events SCRIPT-42 and SCRIPT-75; the moving dog truck SCRIPT-77.

- First end-to-end run of mission 1-1, teleports, update numbers: [scripts-missions.md §2026-10-15T12:41](../../re/scripts-missions.md#ts-20261015-124140).
- Code-side ObjectTriggered call 0x244f40 (enable, message 0xff, event), its carry callers and the missing build-it caller: [carrying.md §2026-10-15T11:11](../../re/carrying.md#ts-20261015-111115).

## 7. Code and tests

- `src/runtime/waypoints.{hpp,cpp}` (waypoint class), glue `ScriptRunner::loadWaypoints` / `updateWaypoints` and two lines in `game_level_update.cpp`; CTest `waypoints`, `waypoints-real-data`.
- `1-2_MissionStart`, `1-2_MissionStart`, `1-2_Cage_Key_Target_Script` and the level startup run in CTest `natives-animation-real-data`.
- CTests `game-mission-1-1`, `game-mission-1-1-start`; `src/runtime/script_thread_log.*` (log lines), script step `at X Y Z` in `game_options.cpp`.
- `ScriptRunner::triggerObjectEnabling` (0x244f40) and `applyCarryActions`; tests `script-runner` (objectTriggerFromCodeEnablesTheObject) and `carry` (TriggerOnRespawn).
