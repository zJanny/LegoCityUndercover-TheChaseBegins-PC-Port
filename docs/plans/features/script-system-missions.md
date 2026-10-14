# Script system: missions

## 1. Scope

What a whole mission needs from the script system beyond natives and trigger events that have their own files: the object classes that start or trigger mission scripts, scripts without a known starter, and the mission-level comparison with the original. Mission 1-2 is the first one analysed. General items are in [script-system.md](script-system.md), natives in [script-system-natives.md](script-system-natives.md), the trigger pass in [script-system-triggers.md](script-system-triggers.md).

## 2. Status

IN_PROGRESS, updated 2026-10-14T21:41:00+02:00. Mission 1-2 was compared with the original (gap list in [scripts-missions.md](../../re/scripts-missions.md)): its scripts run in the port (`1-2_MissionStart` reaches its end one update later than the original), 13 of its 65 functions have no handler and 12 of its 22 triggers are fired by object classes the port does not have. Nothing starts a mission by play yet: the waypoint class that starts `1-2_MissionStart` is missing.

## 3. Verified behaviour

- In the original `1-2_MissionStart` is started by the update of the waypoint class (not by a trigger or an event) when the player reaches the waypoint; the natives then run in the order of the script, `DestroyProp` after the 5 s title camera and `PlayCharAnim` after the following `Wait 5`: [scripts-missions.md §2026-10-14T21:39](../../re/scripts-missions.md#ts-20261014-213921).

## 4. Stand-ins

- SCRIPT-S111 Missions are started by `--call-script NAME` or by a script's own `CallScript` / `SpawnScript`; no waypoint reached by the player starts one. needs: SCRIPT-110. evidence: [scripts-missions.md §2026-10-14T21:39](../../re/scripts-missions.md#ts-20261014-213921).

## 5. Open items

- [ ] SCRIPT-110 — The waypoint class `props/map_waypoint` (update 0x17acbc: states 0 to 3 at component +0x78, messages 52 / 53, the fade states, the thread start 0x17b03c with the script id at component +0x64) is not ported: reaching the active waypoint starts no script (its `TriggerScript` parameter), so no mission starts by play; this is also why the donut shop of mission 1-1 does nothing (user report 2026-10-14, former SCRIPT-140). blocks: SCRIPT-1, SCRIPT-S111. evidence: [scripts-missions.md §2026-10-14T21:39](../../re/scripts-missions.md#ts-20261014-213921).
- [ ] SCRIPT-111 — Class-fired `ObjectTriggered` events: of the 22 triggers of mission 1-2, 12 belong to `scriptobject` objects and no script calls `TriggerObject` on them; scanner (`Scanned_Pringles`, `Scanner_Enter_Script`, `Scanner_Zoomed_Script`, `Scanner_Locked_Script`), detective spots (`Dtv_DogSpot_01_Script`, `Reveal_Bone_Script`, `SO_Dspot_02_X`), the cage use spot (`Cage_Key_Target_Script`) and the camera off script are fired by classes (`prop_scannerzoneaudio`, `ability_detective*`, `prop_simpleuse`, not traced). needs: SCRIPT-75. blocks: SCRIPT-1. evidence: [scripts-missions.md §2026-10-14T21:39](../../re/scripts-missions.md#ts-20261014-213921).
- [ ] SCRIPT-113 — Two mission 1-2 scripts have no known starter (`2451216995`, `2494968948`); the other two are named by waypoint `TriggerScript` parameters (`1-2_HouseArrival`, `1-2_Spawn_Doorlock`) and start with SCRIPT-110. blocks: SCRIPT-1. evidence: [scripts-missions.md §2026-10-14T21:44](../../re/scripts-missions.md#ts-20261014-214429).
- [ ] SCRIPT-114 — The port runs `1-2_MissionStart` one update later than the original at the end of the title camera (DestroyProp in update 154 after the first call, original 153; PlayCharAnim 306 against 305, 0.033 s updates, harness with the director updated after the runner); the game loop order was not checked. needs: SCRIPT-50. evidence: [scripts-missions.md §2026-10-14T21:39](../../re/scripts-missions.md#ts-20261014-213921).
- [ ] SCRIPT-115 — Only the scripted start of mission 1-2 (330 updates) was traced; the free-play parts (bounds `ShowTheDog`, `Prop_Smash_Hint`, the detective spots, `Waypoint_02_Script`, `MissionEnd` with the dog truck) were not walked in the oracle and need states made by the user (position, mission progress after each part). evidence: [scripts-missions.md §2026-10-14T21:39](../../re/scripts-missions.md#ts-20261014-213921).

## 6. Findings summary

- Mission 1-2 gap analysis: natives without a handler and their items, stand-ins, trigger events, the oracle trace of the start: [scripts-missions.md §2026-10-14T21:39](../../re/scripts-missions.md#ts-20261014-213921).
- Natives that block mission 1-2 and have items: DestroyProp SCRIPT-58, EffectFadeObject / StreamRoomIn SCRIPT-38, SetPartyCharacter SCRIPT-35, DCamCutScene* SCRIPT-29, EnableDistanceFade SCRIPT-57, music natives SCRIPT-33, SetForcedStopOnMyVehicle SCRIPT-36; AI natives and events SCRIPT-42 and SCRIPT-75; the moving dog truck SCRIPT-77.

## 7. Code and tests

- No code of its own yet. `1-2_MissionStart`, `1-2_Cage_Key_Target_Script` and the level startup run in CTest `natives-animation-real-data`.
