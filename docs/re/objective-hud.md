# Objective HUD: waypoints, sub-objective groups and the map marker

Handlers of batch SCRIPT-31 as far as mission 1-1 uses them, and what the original shows for an active waypoint. Evidence: REA (Ghidra 12.1.2 pseudo code of `artifacts/game/game.arm.elf`, sha256 5152ed14...0881), llvm-objdump, level data (`parseLevel`) and RPC screenshots of Azahar (states `gameplay-start`, Old 3DS, with `teleport` as measuring aid, output `artifacts/oracle/agentOH-out/`, never committed).

<a id="ts-20261014-155919"></a>

## 2026-10-14T15:59:19+02:00 — SetActiveWayPoint, SubObjectiveGoToEnable, the sub-objective registry and the waypoint marker on the map

Corrected by [objective-hud.md §2026-10-15T19:56](#ts-20261015-195600): the map marker is the sprite `WayPoint.tga` with two line circles, the unit is 0.94977 pixels per world unit, and the code and clock of the pulse are read.

Corrected by [objective-hud.md §2026-10-15T18:31](#ts-20261015-183130): the badge is not part of the waypoint model; it is a child object of the class file (`dummymeshbouncer`) that the port did not instantiate, so the model was drawn without it rather than "drawn static".

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

<a id="ts-20261015-183130"></a>

## 2026-10-15T18:31:30+02:00 — The waypoint badge is a child object `Badge` of class `dummymeshbouncer`; it turns and hops (llvm-objdump, class files)

[Corrects §2026-10-14T15:59](#ts-20261014-155919): the model `Prop_Waypoint_Small` (`models/prop_waypoint_small/`, one node `Waypoint_Small`, one mesh with the `marker_rings.tga` part, 128 vertices) holds only the light column. The police badge is a second model, `Prop_Waypoint_LCPDBadge` (`models/prop_waypoint_lcpdbadge/`, one node, one part `Prop_Waypoint_LCPDBadge_128.tga`, 107 vertices).

**Why the port drew no badge.** The class files `levels/props/map_waypoint.blvl`, `map_vehiclewaypoint.blvl` and `map_getintovehiclewaypoint.blvl` carry one embedded object (section +0x2c, the same layout as a level's objects): instance `Badge`, class `dummymeshbouncer`, parameters `meshname` = `Prop_Waypoint_LCPDBadge`, `initialposition` (0, 0.3965, 0) in the small class and (0, 2.6375, 0) in the two large ones, and the bouncer parameters below. The port read only the objects of the level file, never the objects of a class file. The setup 0x170fd8 of the waypoint class looks the child up by name (0x2450f0 "Badge", stored at component +0x5c) and hides it with the waypoint (0x2428c0 at its end).

**Class `dummymeshbouncer`** (type 228, `levels/dummymeshbouncer.blvl`; descriptor built at 0x1c8008: create 0x260528, destroy 0x26c858, update 0x260728, handler 0x26c87c = `return 0`). Create reads `RevsPerMin` (hash 7581f2b0) x 0.10471976 into +0x48, `BouncePerMin` (ba47dfdf) x 0.016666668 into +0x40 and `BounceAmount` (40d85717) into +0x44, all defaulting to 0, and keeps the child's placement matrix (0x2364cc from `initialposition` / `initialorientation`) as the record's first 0x40 bytes. Update 0x260728 (dt in s0): clock +0x4c += dt, wraps at 60 s (`if 60 <= clock then clock -= 60`); the matrix is the placement matrix with rows 0 and 2 turned about y by `RevsPerMin * 0.10471976 * clock` radians (0x237bc4: row0' = row0 c - row2 s, row2' = row2 c + row0 s) and the translation raised by `sin(BouncePerMin * 0.016666668 * clock * 2 pi) * BounceAmount` along (0, 1, 0) (the float3 at 0x56d424, measured (0, 1, 0) in [script-natives-flags.md §2026-10-14T19:19](script-natives-flags.md#ts-20261014-191915)); then set on the node (0x24c118). Values: small waypoint `RevsPerMin` 15, `BouncePerMin` 50, `BounceAmount` 0.08; both large classes 15, 20, 0.25. So the badge turns a quarter in 1 s and hops with a period of 1.2 s (small) or 3 s (large).

**The column has no animation data.** The model has one node and the archive holds no `.bfnanm` for it (`models/prop_waypoint_*` has only the models and the chequered flag's `ufo_beam.bfnanm`). The waypoint update 0x17acbc animates it with a node tween (0x229e54 -> 0x21b7cc with flag byte 3: alpha and colour of the node, from the first to the second float over a time): state 0 starts a 0 to 1 fade of the waypoint node over `FadeScaleInTime` (+0x6c: 0.8 s small, 1.0 s large; the parameter name says "scale" but the tween flags are alpha and colour), state 1 fades the Badge 0 to 1 over `BadgeFadeInTime` (+0x68, 0.8 s small), state 2 fades the waypoint 1 to 0 over `FadeScaleOutTime` (+0x70, 0.4 s small). The tween evaluator and the effect of the flag bits on the draw (0x23f708, 0x2408f8, 0x23f540) were not read.

**Port.** `src/runtime/mesh_bouncer.*` (class replica and `propChildren` for the class files), `src/game_prop_parts.*` (scene instances of the children, shown with their waypoint). Tests `prop-parts` (closed form) and `prop-parts-real-data` (class parameters). Picture check against the oracle picture `agentOH-out/tp3a.png`: the same cyan column with the bronze badge inside, and in the port the badge goes from edge-on to three-quarter view over 16 updates; no oracle number of the badge's angle or height exists.

Unknowns:
- The fade tweens of the column and the badge (alpha and colour over 0.8 s, 0.4 s out) are not drawn; the port shows both at full opacity as soon as the waypoint is switched on.
- Where the child's clock starts (record +0x4c is zero from the allocation; the first update adds dt) and whether the bouncer runs while its parent is disabled (the flag byte +0x50 that stops the update has no writer found); the port runs it every update.
- The effect of `UseMarker` waypoints (kind 2, other `props/map_*` classes) on the children.

<a id="ts-20261015-195600"></a>

## 2026-10-15T19:56:00+02:00 — Map screen icons: placement, edge ellipse and arrow, halo, grow, pulse rings (REA 0x2db0dc, 0x2ddd64, oracle)

[Corrects §2026-10-14T15:59](#ts-20261014-155919): the waypoint marker on the bottom screen is not `subobjectivepoint` + `mapiconmission` + `mapiconpulse`. It is the sprite `WayPoint.tga` (`sprites/ui_bottomscreen/maps/waypoint.btga`, 32x32: blue ring with the police badge) plus two circles made of lines; the 0.92 scale was a fit of the wrong constant, the real one is 0.94977.

Also [Corrects hud.md §2026-10-13T21:31](hud.md#ts-20261013-213101): its map screen paragraph names the marker `mapiconmission` and leaves the map transform undecoded.

**Map screen object** (`this` = 0x519e24, its data struct = `*(this + 0x24)`, 0x085df080 in `gameplay-start`). The icon table at 0x3588a0 holds 44 texture names, loaded into struct + 0x14 + 4 * index (setup 0x2dc590): 0 WayPoint, 1 Collectable, 2 UI_MapRedbrick, 3 Standalone, 4 jeopardy, 5 hub, 6 Helicopter, 7 UI_MapSuperbuildit, 8 vehicledrop, 9 skydive, 10 bonusmission, 11 race, 12 PointOfInterest, 13 SubObjectiveEnemy, 14 SubObjectivePoint, 15 SubObjectivePointGreen, 16 WaypointOffscreen, 17 UI_MapRamp, 18 UI_MapPostcard, 19 UI_MapToken, 20 UI_MapVehicleToken, 21 boat, 22..27 UI_Map Parrot, Pig, Chicken, Squirrel, Dog, Cat, 28 Warden, 29 Powernode, 30 WalkieTalkie, 31 Superbrick, 32..37 UI_MapToken Generic, Builder, Miner, Robber, Farmer, Astronaut, 38 TokenCop, 39 TokenFire, 40 WaypointOffscreenBlue, 41 ...Green, 42 ...Red, 43 ...White. `mapiconmission`, `mapiconpulse` are not named anywhere in the code.

**Slots.** 16 slots at struct + 0xe0 + 4 * i (record 0x28 bytes, created by 0x1f865c): +0 x, +4 y (screen), +8 and +0xc = 16.0, +0x10 entity (nonzero for an object-bound slot), +0x14 position pointer, +0x18 alpha, +0x1c icon type byte, +0x1d fade state (1 in, 2 out), +0x20 grow, +0x24 scale. The icon type of an object-bound slot is `*(*(entity + 0x7c) + 0x88)` (0x186d44); slot of `1-1_Waypoint_01` at `gameplay-start`: type 0, alpha 1.0, grow 0.0, scale 0.5, position (179.33, 46.42). A group marker of the sub-objective registry (0x1fcc28) creates a slot of type 14 (go to), 13 (enemy) or 15 (carry, green) from the byte table 0x358950.

**Placement (0x1ee7a8, 0x1ee848, 0x1fc1fc).** World position times the 4x4 map matrix at struct + 0x308 (translation row struct + 0x338..0x344: -1202.8282, -1121.6160, 69.47, w 1337.4733), x and y divided by w, then times struct + 0x354/0x358 (512, 512) and the zoom struct + 0x35c (1.55, read from the oracle at `gameplay-start`). Per unit that is 512 * 1.55 * 1.6007755 / 1337.4733 = 0.94977 pixels in x and in z. The icon is at `(160, 120) + R(camera angle) * ((x - player x), -(y - player y))` with the screen centre 320 / 2 and 240 / 2 (screen sizes 320x240 at 0x529e8c); the player's arrow is drawn centred at the same point. The same matrix gives the map texture's texel: u = 512 + 512 * (1.6007755 x - 1202.8282) / 1337.4733, v = 512 - 512 * (1.6007755 z - 1121.6160) / 1337.4733. With these numbers and the centre (160, 120) the port's map picture equals the oracle's bottom screen at `gameplay-start` and after a teleport to (700, 16.6, 900) (mean absolute difference 0.2 and 0.03 per channel over the area without the player's arrow and the characters button; the marker and its arrow are pixel-identical: 88 pixels of (74, 174, 255), same centroid), which replaces the earlier fitted constants (0.645 texels per pixel, centre y 118; the 0.645 equals 0.61281 / 0.94977). Position check: the slot of `1-1_Waypoint_01` reads (179.33, 46.42); the formula gives (179.4, 46.4).

**Edge ellipse and arrow (0x2db0dc).** Half axes A = 320 * 0.44 - 36 = 104.8 and B = 240 * 0.57 - 36 = 100.8 around (160, 120) (the factors 0.44 and 0.57 are floats at 0x375b34 and 0x375b38, runtime values read from memory, initialiser not found). Outside `dx^2/A^2 + dy^2/B^2 > 1`: the angle is atan2(dy * A / B, dx), the edge point (160 + A cos, 120 + B sin), and the icon is drawn there (rounded with `int(v + 0.5)`). An arrow sprite is drawn first at edge point + 4 * (cos, sin), size 32x32, turned by pi - atan2(cos, sin) (clockwise, the sprite points up), alpha = grow * slot alpha * map alpha * 255: type 0 `WaypointOffscreen` (index 16), 13 index 42 (red), 14 index 40 (blue), 15 index 41 (green), other 43 (white).

**Type 0 (WayPoint) drawing order per slot.** (1) halo: `WayPoint` quad at the rounded unclamped position, half size 16 * (1 - (scale - 0.5)), full alpha; (2) while the ring clock is below 2.0 s two circles at the unclamped position (below); (3) the arrow if outside the ellipse; (4) the icon quad at the (clamped) rounded position, half size 16 * scale, alpha grow * 255. Scale (+0x24): minus 2 per second while the unclamped position is on screen (x > 0, y > 0, x < 320, y < 240), plus 2 otherwise, clamped to 0.5 .. 1; grow (+0x20): plus 2 per second while the shown (clamped, rounded) position is 24 or more pixels from the unclamped slot position, else minus 2, clamped 0 .. 1 (so on screen only the halo at full size is visible; at the edge the small disc and the arrow grow in over 0.5 s). Other types: grow and scale stay 1, a single quad of the icon texture (half size = texture size / 2).

**Pulse rings (0x1ee03c, update 0x2ddd64).** Ring clock `this + 0x450` adds dt every update and returns to 0 once it exceeds 3.0 (floats at 0x375b48; 0x375b44 = 2.0 is the active length, 0x375b4c = 200, 0x375b50 = 20). While below 2.0: t = 1 - clock / 2; two circles around the marker, radii 200 + (2 - 200) * t and 20 + (2 - 20) * t (so both grow from 2 pixels; the big one leaves the screen), 64 line segments of width 2, alpha t * slot alpha * map alpha, each segment faded by 0.05 per pixel within 20 pixels of the screen edges (product of four clamped factors). Oracle (RPC picture series with the game running, marker position (179.4, 46.4)): the ring starts small, the big circle's radius grows 15 to 58 pixels in 14 pictures (about 3.4 per picture, 0.032 s of game time each) with relative opacity 0.80 to 0.53, the small circle ends at radius 13 to 19.5 with opacity 0.35 to 0.03; these match radius 20 - 18 t and opacity t. Colour of the lines: white in the pictures (0x25511c blend state, alpha blending; the colour register of 0x204b68 was not read).

**Slot fades and update (0x2ddd64).** Fade state 1 adds dt to the slot alpha up to 1; state 2 subtracts dt and frees the slot at 0. Zoom: struct + 0x360 is the table value 0x519e18[struct + 0x368] (3.8, 2.5, 1.55; index 2 at `gameplay-start`; times 0.6 when 0x24255c(player) is set), and struct + 0x35c moves to it by `(target - zoom) * dt * 5`; 0.75 is forced while this + 0x49b is set.

**Group markers (0x2dbb04).** For the three registry types (0 go to, 1 enemy, 2 carry) every target of the record is drawn as the 16x16 icon of index 14, 13, 15 (SubObjectivePoint, Enemy, Green) at its mapped position, no clamping, no arrow, alpha `(1 - fade) * 255 * map alpha`; this is separate from the slots.

Unknowns:
- The class parameter that selects the icon type (`*(component + 0x88)`) of the waypoint classes other than `map_waypoint`, and which objects add slots through 0x2258bc (callers 0x1858dc, 0x192548, 0x26d550, 0x2ef144).
- The initialiser of the edge factors 0.44 and 0.57 and of the ring constants 2.0, 3.0, 200, 20 (read from memory).
- The colour of the ring lines (0x204b68 not read) and whether the line width 2 is the GPU line width.
- What sets the zoom level index (struct + 0x368) and the zoom buttons; the port keeps 1.55.
- Arrow and icon positions of types other than 0 were not compared with the oracle.

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
