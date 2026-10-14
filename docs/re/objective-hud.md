# Objective HUD: waypoints, sub-objective groups and the map marker

Handlers of batch SCRIPT-31 as far as mission 1-1 uses them, and what the original shows for an active waypoint. Evidence: REA (Ghidra 12.1.2 pseudo code of `artifacts/game/game.arm.elf`, sha256 5152ed14...0881), llvm-objdump, level data (`parseLevel`) and RPC screenshots of Azahar (states `gameplay-start`, Old 3DS, with `teleport` as measuring aid, output `artifacts/oracle/agentOH-out/`, never committed).

<a id="ts-20261014-155919"></a>

## 2026-10-14T15:59:19+02:00 — SetActiveWayPoint, SubObjectiveGoToEnable, the sub-objective registry and the waypoint marker on the map

**What mission 1-1 calls.** In lvl_01 (`1-1_*` scripts): `SetActiveWayPoint(object, 0|1)` and `SubObjectiveGoToEnable(waypoint, target, 0|1)`; `EnableMinimapWipe` appears in the `1-2` scripts. `1-1_MissionStart` (hash 1340660679) switches `1-1_Waypoint_01` on (the donut shop, world 817.35, 23.93, 1071.20); `1-1_SO_Donuts_PickedUp`-type scripts move the waypoint to `1-1_Waypoint_02` (police car), `_03` (donut van), `1-1_Van_Waypoint_Enter`, `_05`, `_Donuts`. The waypoint objects are placed objects of the classes `props/map_waypoint` (mesh `Prop_Waypoint_Small`), `props/map_getintovehiclewaypoint` and `props/map_vehiclewaypoint` (mesh `Prop_Waypoint_Large`); the classes have no script.

**Handlers (REA and objdump).**

| Native | Handler | Behaviour |
|---|---|---|
| SetActiveWayPoint | 0x27bde8 | `0x26e168(entity of arg 0, enable = arg 1 != 0)`. 0x26e168 returns without effect when `enable == 0` and bit 4 of the byte at `component + 0x8e` (component = entity + 0x7c) is clear; otherwise it calls `0x242cf0(entity)` when enabling and always `0x229c98(entity, enable, 0)`. 0x229c98 sets bit 0 of `component + 0x8e` to `enable`, on enable stores a scene node link (0x1a2114) and shows the node, on disable hides it (0x2428c0 and message 0x229e54). So an active waypoint is the placed object being shown; the mesh is drawn only then. |
| SubObjectiveGoToEnable | 0x2adaac | `0x2a62b8(entity of arg 0, entity of arg 1, enable = arg 2 != 0)`. Enable: 0x1fce14(table 0x51a3a8, key, target, type 0). Disable: find the record (0x1fcc68 with type 0), remove the target by moving the last entry into its place. |
| SubObjectiveEnemyEnable | 0x2b4c3c | `mov r0, #1; bx lr`: no effect. |
| SubObjectiveArrowGoToEnable | 0x2c57b0 | `0x2c289c(entity, arg 1 != 0)` (not read). |
| SubObjectiveArrowEnemyEnable | 0x2c87f0 | `0x230dbc(entity, arg 1 != 0)` (not read). |
| EnableMinimapWipe | 0x27bbe8 | `0x187f28(arg != 0)` (not read). |
| EnableScanner / OverrideScannable / PanMapToObject | 0x1cd9dc / 0x27bce4 / 0x1d9e04 | call 0x246128 / 0x241624 + 0x236968 / 0x19465c (object and three floats); not read further. |

**Registry (0x51a3a8).** Records of 0xc4 bytes from `table + 0x498`, count at `table + 0x1d18`. Per record (offsets from the record start): +0 up to 32 target entity pointers, +0x80 one byte per target (set to `~(target + 0x10) & 1` when added), +0xa0 target count, +0xa4 centroid (a 0x14-byte vector object, cleared and refilled by 0x2be934), +0xb0 effect handle (-1 none), +0xb4 fade timer, +0xb8 byte "a target is on screen" (0x29de40 of the node), +0xbc key entity (the waypoint), +0xc0 type byte (0 go to, 1 enemy, 2 carry). `0x1fcc68(table, key, type, target)` finds the record by key and type, or, with key 0, by type and target membership. `0x1fce14` appends a target, creating the record when none exists (a target already in the list is not added twice).

**Per-update 0x2be934 (read, not transcribed).** For every record, newest first: each target whose enabled bit is clear and (target flag +0x10 bit 0 or +0xc bit 4) is removed again; the centroid of the target nodes (+0x30 of the node transform 0x250810) is averaged, with the key entity counted for the player; a record without targets is deleted; the marker effect (id from the byte table `0x0f0d0e` indexed by type: go to 0x0e, enemy 0x0d, carry 0x0f; created by 0x1fcc28) is created at the centroid and faded in by `timer / DAT_00375d38`, or faded out when the record is hidden. The textures `SubObjectivePoint.tga`, `SubObjectivePointGreen.tga`, `SubObjectiveEnemy.tga`, `SubObjectiveArrowGoto.tga`, `SubObjectiveArrowEnemy.tga`, `SubObjectiveArrowCarry.tga` (strings 0x366bd8..0x366e4f) are the effect textures. Mission 1-1 starts no sub-objective in its first steps, so none was compared.

**Measured in the oracle (state `gameplay-start`, Old 3DS, RPC screenshots 400x480).**
- Bottom screen map marker of `1-1_Waypoint_01` (player 886.76, 1111.17, camera yaw -2.3501): a blue disc with black outline (`sprites/ui_bottomscreen/maps/subobjectivepoint.btga`, 16x16) about 20 px across, a light badge inside (`sprites/ui_bottomscreen/mapiconmission.btga`, 16x16) about 11 px, and a white pulse ring (`sprites/ui_bottomscreen/mapiconpulse.btga`, 16x16). Blue ring pixels (74, 174, 255): 74, centroid (218.5, 285.7) in the window picture, that is canvas (178.5, 45.7) relative to the player arrow at (160, 118).
- Placement: with the map's own unit (one world unit = one pixel, the rotation of `drawMinimap`) the marker lies 0.91 to 0.96 of the predicted distance from the player (gs0 0.93, tp1 0.89, tp2b 0.96 over 16 to 80 units); the port uses 0.92 (stand-in HUD-S10).
- Pulse ring: grows from about 12 px radius to about 90 px and fades; stepped screenshots (6 updates apart, with the free-running time of each screenshot in between) show radii 12, 15, 17, 19 then 28 to 55 then 78 to 89; a gap without ring follows each pulse; the cycle repeats about every 36 updates. The exact clock and curve are not recovered.
- In the world a waypoint is the model `Prop_Waypoint_Small`: a translucent cyan light column with a turning police badge inside (screenshots `tp3a`, `tp3b` with the player teleported to 822.0, 24.0, 1078.0). The port draws the column; the badge and its turning are not drawn (the mesh is drawn static).
- Waypoints are hidden before a script switches them on (docs/re/objects.md).

**Port.** `src/runtime/objectives.{hpp,cpp}` (state, registry, natives), dispatch `ScriptRunner::objectiveNative`, markers in `src/frontend/minimap.cpp`, waypoint meshes in `src/game_level.cpp` (`DrawableObjects::waypoints`). Tests: `objectives`, `objectives-real-data`, `waypoint-marker-shot` with `waypoint-marker-position` (`tools/check_waypoint_shot.py`).

Unknowns:
- Meaning of bit 4 of `component + 0x8e` (a disable of SetActiveWayPoint is skipped when it is clear): the port always disables.
- Which code draws the map markers and which parameter selects the icon (the class values at hashes 0x6fef256, 0x51a935e, 0x59521dda, 0xc93e4625 of the waypoint classes are untested); markers outside the map screen (clamp or not) and the other icons (`subobjectiveenemy.btga`, `pointofinterest.btga`, mission icons).
- Handlers 0x2c289c, 0x230dbc, 0x187f28, 0x246128, 0x241624, 0x236968, 0x19465c and the update 0x2be934 in detail; the effect ids 0x0d, 0x0e, 0x0f and what a go-to group looks like in the world; the arrow of SubObjectiveArrow* (not used by mission 1-1).
- The pulse clock, the badge colours (the sprite drawn by the port looks darker than the oracle's badge, so the original probably uses another blend), the badge's turning in the world.

<a id="ts-20261014-212357"></a>

## 2026-10-14T21:23:57+02:00 — Mission 1-1 flow in the level data and what stops it in the port (donut shop waypoint)

Source: `Level::scripts`, `Level::triggers` and the object parameters of `lvl_01_cherrytreehills_main.blvl` (`tools/convert_levels.py --param-names` and a scratch dump through `chase_assets` / `scriptFunctionSupport`, not kept). No oracle run.

**User report 2026-10-14.** Walking into the donut shop marker at the start of mission 1-1 does nothing in `chase-game`.

**Why.** The marker is `1-1_Waypoint_01` (class `props/map_waypoint`, 817.35, 23.93, 1071.20, room8). It has no trigger record: its instance parameter `TriggerScript` = `1-1_VanBreakdown` names the level script that the original starts when the player reaches the waypoint. The port shows the waypoint (SetActiveWayPoint) but has no reach test and never reads `TriggerScript`. All 23 waypoint objects of lvl_01 (`props/map_waypoint`, `props/map_vehiclewaypoint`, `props/map_getintovehiclewaypoint`) work this way.

**Mission 1-1 order (scripts and what starts them).**

| Step | Started by | Script | Needs in the port |
|---|---|---|---|
| 1 | `startup` (`1-<StoryProgress>_MissionStart`) | `1-1_MissionStart`: intro movie `01_GameIntro` once, in-engine cutscene `1_1_a`, waypoint 01 on | PlayCutscene, DCamCutScene* (SCRIPT-29, MOVIE-2), SetPartyCharacter, VehicleSetLocked / ForcedStop (vehicles) |
| 2 | waypoint 01 reached | `1-1_VanBreakdown`: cutscene `1_1_b`, dialogue, police car and waypoint 02 on, broken van scene in room33 | waypoint reach and `TriggerScript`; PlayCharAnim; CameraFollow |
| 3 | waypoint 02 reached (police car) | `1-1_Get_Car`: unlocks `1-1_Police_Car` | vehicles: enter and drive |
| 4 | `PlayerVehicleEntered` of `1-1_Police_Car` (trigger 2) | `1-1_Release_Car_Script`: director cameras, waypoint 03 on | vehicle trigger events (SCRIPT-76) |
| 5 | waypoint 03 reached (room33) | `1-1_Found_Van`: dialogue with `1-1_AI_DonutLady`, build-it `1-1_BI_DonutVanParts` on | AIFaceTarget, EnableTrafficAndPedestrians, PlayCharAnim |
| 6 | build-it finished (inferred from the names; `ObjectTriggered` of the script object `1-1_SO_VanFixed`, trigger 7) | `1-1_SO_VanFixed`: driveable donut van, `1-1_Van_Waypoint_Enter` on | build-its (`props/prop_buildit`, parameters `BuilditFinalProp`, `TriggerObject`) |
| 7 | `1-1_Van_Waypoint_Enter` reached (`props/map_getintovehiclewaypoint`, `TriggerScript`) | `1-1_Van_Waypoint_Enter_Script`: waypoint 05 on | vehicles; VehicleAllowJumpOut |
| 8 | waypoint 05 reached in the van (`props/map_vehiclewaypoint`) | `1-1_VanEscorted`: cutscene `1_1_d`, player out of the van, donut box waypoint on | VehicleJumpOut, VehicleSetForcedStop |
| 9 | `1-1_Waypoint_Donuts` reached | `1-1_Carry_Object_Tutorial`: carry box `1-1_DonutsDEMO` on, `1-1_Deliver_Donuts` trigger on | carrying (`props/prop_carry`, parameter `TriggerPickup`) |
| 10 | box carried into its target (inferred from the names; `ObjectBoundEntered` of `1-1_DonutsDEMO`, trigger 1; target `1-1_DonutTarget_DEMO`, class `props/prop_carrytargetnomesh`, parameter `TriggerObject`) | `1-1_Deliver_Donuts`, then `1-1_SO_MissionEnd` (`ObjectTriggered`, trigger 6): cutscene `1_1_e` | carry target; moving object bounds (SCRIPT-77) |
| 11 | `SpawnScript` | `1-1_MissionEnd`: StoryProgress 2, reward, director cameras, police station doors on, mission 1-2 waypoint | GiveReward, SnapCamera (stand-ins) |

Side scripts: `1-1_Get_Back_To_Donuts` (BoundLeft, warns when the player walks away), `1-1_SO_Donuts_PickedUp` (`ObjectTriggered`, tip texts), `1-1_Buildit_Tutorial` (empty).

**Script functions of mission 1-1 without any handler:** DCamCutSceneLoad / Play / Wait / Free, PlayCutscene, SetPartyCharacter, VehicleSetLocked, VehicleSetForcedStop, SetForcedStopOnMyVehicle, VehicleSetUnshovable, VehicleAllowJumpOut, VehicleJumpOut, EnableTrafficAndPedestrians, PlayCharAnim, CameraFollow, OverrideMusic, AutomaticMusic. The other calls run, many of them as stand-ins (tip texts, dialogue, waypoints, AIEnable, AIFaceTarget, GiveReward, SnapCamera, QuickSave).

Unknowns:
- The waypoint reach test: code (class behaviour of `props/map_waypoint` and the two vehicle classes), radius or bound, on foot or in a vehicle, and when the script starts relative to the trigger pass.
- How a script object (class `scriptobject`) is fired by a build-it, a carry pickup or a carry target: the object reference parameters (`TriggerObject`, `TriggerPickup`, `BuilditFinalProp`, unnamed hash 0xc23d689b) print as unset (`None`) in the converter, so the link may be made elsewhere.
- What the original shows at step 1 on a new game in the port's order (intro movie, then `1_1_a`).
