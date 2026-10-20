# Objective HUD: the waypoint model and mission 1-1 flow

Subtopic of [objective-hud.md](objective-hud.md): the waypoint column and its badge, the node tweens that fade them and how mission 1-1 reaches its steps through waypoints.

## Current state

- A waypoint is the column model `Prop_Waypoint_Small` plus a child `Badge` (`dummymeshbouncer`, turns and hops); the class files carry the child. [§2026-10-15T18:31](#ts-20261015-183130)
- Column and badge fade in and out through node tweens (registry 0x21b7cc, flags 3: alpha and colour). [§2026-10-15T19:58](#ts-20261015-195841)
- The waypoint is reached through its class update and `TriggerScript`, which starts the mission script; mission 1-1 has 11 steps. [§2026-10-14T21:23](#ts-20261014-212357), [scripts-missions.md §2026-10-14T22:48](scripts-missions.md#ts-20261014-224825)

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

<a id="ts-20261015-183130"></a>

## 2026-10-15T18:31:30+02:00 — The waypoint badge is a child object `Badge` of class `dummymeshbouncer`; it turns and hops (llvm-objdump, class files)

[Corrects §2026-10-14T15:59](objective-hud.md#ts-20261014-155919): the model `Prop_Waypoint_Small` (`models/prop_waypoint_small/`, one node `Waypoint_Small`, one mesh with the `marker_rings.tga` part, 128 vertices) holds only the light column. The police badge is a second model, `Prop_Waypoint_LCPDBadge` (`models/prop_waypoint_lcpdbadge/`, one node, one part `Prop_Waypoint_LCPDBadge_128.tga`, 107 vertices).

**Why the port drew no badge.** The class files `levels/props/map_waypoint.blvl`, `map_vehiclewaypoint.blvl` and `map_getintovehiclewaypoint.blvl` carry one embedded object (section +0x2c, the same layout as a level's objects): instance `Badge`, class `dummymeshbouncer`, parameters `meshname` = `Prop_Waypoint_LCPDBadge`, `initialposition` (0, 0.3965, 0) in the small class and (0, 2.6375, 0) in the two large ones, and the bouncer parameters below. The port read only the objects of the level file, never the objects of a class file. The setup 0x170fd8 of the waypoint class looks the child up by name (0x2450f0 "Badge", stored at component +0x5c) and hides it with the waypoint (0x2428c0 at its end).

**Class `dummymeshbouncer`** (type 228, `levels/dummymeshbouncer.blvl`; descriptor built at 0x1c8008: create 0x260528, destroy 0x26c858, update 0x260728, handler 0x26c87c = `return 0`). Create reads `RevsPerMin` (hash 7581f2b0) x 0.10471976 into +0x48, `BouncePerMin` (ba47dfdf) x 0.016666668 into +0x40 and `BounceAmount` (40d85717) into +0x44, all defaulting to 0, and keeps the child's placement matrix (0x2364cc from `initialposition` / `initialorientation`) as the record's first 0x40 bytes. Update 0x260728 (dt in s0): clock +0x4c += dt, wraps at 60 s (`if 60 <= clock then clock -= 60`); the matrix is the placement matrix with rows 0 and 2 turned about y by `RevsPerMin * 0.10471976 * clock` radians (0x237bc4: row0' = row0 c - row2 s, row2' = row2 c + row0 s) and the translation raised by `sin(BouncePerMin * 0.016666668 * clock * 2 pi) * BounceAmount` along (0, 1, 0) (the float3 at 0x56d424, measured (0, 1, 0) in [script-natives-flags.md §2026-10-14T19:19](script-natives-flags.md#ts-20261014-191915)); then set on the node (0x24c118). Values: small waypoint `RevsPerMin` 15, `BouncePerMin` 50, `BounceAmount` 0.08; both large classes 15, 20, 0.25. So the badge turns a quarter in 1 s and hops with a period of 1.2 s (small) or 3 s (large).

**The column has no animation data.** The model has one node and the archive holds no `.bfnanm` for it (`models/prop_waypoint_*` has only the models and the chequered flag's `ufo_beam.bfnanm`). The waypoint update 0x17acbc animates it with a node tween (0x229e54 -> 0x21b7cc with flag byte 3: alpha and colour of the node, from the first to the second float over a time): state 0 starts a 0 to 1 fade of the waypoint node over `FadeScaleInTime` (+0x6c: 0.8 s small, 1.0 s large; the parameter name says "scale" but the tween flags are alpha and colour), state 1 fades the Badge 0 to 1 over `BadgeFadeInTime` (+0x68, 0.8 s small), state 2 fades the waypoint 1 to 0 over `FadeScaleOutTime` (+0x70, 0.4 s small). The tween evaluator and the effect of the flag bits on the draw (0x23f708, 0x2408f8, 0x23f540) were not read.

**Port.** `src/runtime/mesh_bouncer.*` (class replica and `propChildren` for the class files), `src/game_prop_parts.*` (scene instances of the children, shown with their waypoint). Tests `prop-parts` (closed form) and `prop-parts-real-data` (class parameters). Picture check against the oracle picture `agentOH-out/tp3a.png`: the same cyan column with the bronze badge inside, and in the port the badge goes from edge-on to three-quarter view over 16 updates; no oracle number of the badge's angle or height exists.

Unknowns:
- The fade tweens of the column and the badge (alpha and colour over 0.8 s, 0.4 s out) are not drawn; the port shows both at full opacity as soon as the waypoint is switched on.
- Where the child's clock starts (record +0x4c is zero from the allocation; the first update adds dt) and whether the bouncer runs while its parent is disabled (the flag byte +0x50 that stops the update has no writer found); the port runs it every update.
- The effect of `UseMarker` waypoints (kind 2, other `props/map_*` classes) on the children.

<a id="ts-20261015-195841"></a>

## 2026-10-15T19:58:41+02:00 — Fade of the waypoint column and badge: the node tween (llvm-objdump)

Source: llvm-objdump of `artifacts/game/game.arm.elf`; no oracle run. Completes the fade unknown of [§2026-10-15T18:31](#ts-20261015-183130).

**Waypoint update 0x17acbc, timer +0x74 and state +0x78.** The timer is reduced by dt and clamped to 0; only the update in which it reaches 0 runs the state. State 0: the waypoint node gets a tween from 0 to 1 over `FadeScaleInTime` (+0x6c) through 0x229e54 (flags 3), the Badge child (+0x5c) a tween from 0 to 0, timer = +0x6c, state 1, sound 0x35. State 1 (only when the badge exists and bit 1 of +0x8f is set): the Badge gets a tween from 0 to 1 over `BadgeFadeInTime` (+0x68); the state stays 1. State 2: the waypoint node gets a tween from 1 to 0 over `FadeScaleOutTime` (+0x70), timer = +0x70, state 3 (the Badge is not touched). So the column is transparent at the start of its fade-in, the badge starts fading in when the column is opaque, and the column fades out while the badge keeps full opacity until the waypoint switches off.

**Tween registry 0x21b7cc(manager, node, flags, from, to, duration, linked object).** A table of up to 20 entries of 28 bytes at manager +0x338 (count +0x334; an entry for the same node is reused, a full table drops the oldest): node, start time (0x23f74c), duration, from, to, linked object, flag byte. On registration the `from` value is applied at once. Evaluation 0x1593c8 (entry index, dt; dt of -1.0 means the time since the start): value = from + (to - from) elapsed / duration, 1.0 from 1.0 up, 0 at or below 0; a duration of 0 is skipped. Applying a value: flag bit 0 sets the node alpha to int(value x 255) (0x23f708), bit 1 sets the node colour to value x the float3 at 0x56d40c = (1, 1, 1) (0x2408f8, 0x23f6d0), bit 2 a third node property (0x23f540); at value 0 the node is hidden (0x23f734) and the linked object disabled with message 251 (0), shown again with message 251 (1) otherwise. The flags used by the waypoint are 3: alpha and colour.

**Port.** `src/runtime/node_tween.hpp` (the evaluator), `Waypoint::columnTween` / `badgeTween` in `src/runtime/waypoints.*` (start points as above, `BadgeFadeInTime` read from the class), `Scene::setInstanceFade` (uniform `instanceFade` multiplies the colour and alpha of the whole instance in `scene.frag.glsl`), glue in `src/game_level.cpp` and `PropParts::apply`. Test `waypoints` (`fades`).

Unknowns:
- How the node alpha and colour enter the material of the draw (0x23f708, 0x23f6d0 store them in the node; the draw path that reads them was not read): the port multiplies colour and alpha of the finished fragment by the value (HUD-S70).
- Bit 1 of +0x8f (condition for the badge fade-in; a distance test of 0x17ad20) is assumed set; the tween clock (time since registration) is advanced by dt per update from the next update on.
- The tweens of the other `props/map_*` classes (`UseMarker`) and the sound 0x35 at the start of state 0.

