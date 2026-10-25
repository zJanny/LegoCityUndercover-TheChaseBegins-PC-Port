# Mission scripts: gap analysis of what a mission needs

What the scripts of one mission call, which natives and trigger events the port lacks, and what the original does at the start of the mission. Script engine: [scripts.md](scripts.md); natives: [script-natives.md](script-natives.md); trigger events: [script-triggers.md](script-triggers.md). Items: [script-system-missions.md](../plans/features/script-system-missions.md).

## Current state

- Mission scripts are started by waypoint objects, not by the trigger pass: the class update 0x17acbc starts the thread of the `TriggerScript` parameter (component +0x64) at 0x17b03c once the fade-out state 3 is reached; the first script of a mission is the one named by `<mission>_Waypoint_01`. [§2026-10-14T22:48:25](#ts-20261014-224825), [§2026-10-14T21:44:29](#ts-20261014-214429)
- Waypoint timing measured: a hit at horizontal distance below the bound radius 1 sets state 2; the thread starts exactly 14 updates later (FadeScaleOutTime 0.4 needs 13 more); the player is frozen during that time when `TakePlayerControl` is set. A disable of an AutoTrigger waypoint is ignored. [§2026-10-14T22:48:25](#ts-20261014-224825)
- Mission 1-2 in the original: `1-2_MissionStart` runs its natives in a fixed order (title camera 153 updates, then Wait until update 331); the port matches at both points (the earlier one-update lag came from the scratch harness, [§2026-10-20T09:22](scripts-runtime.md#ts-20261020-092238)). [§2026-10-14T21:39:21](#ts-20261014-213921)
- The 13 natives of mission 1-2 without a handler and the 12 class-driven ObjectTriggered events are listed in the entry; `HouseArrival` (3388757305) and `Spawn_Doorlock` (3035472981) are the `TriggerScript` values of two waypoints. [§2026-10-14T21:44:29](#ts-20261014-214429)
- lvl_01 startup runs the scripts for the current `StoryProgress` (1 starts `1-1_MissionStart`, 2 starts `1-2_MissionAvailable`); the new-game start is `movie 01_GameIntro`, cutscene `1_1_a`, then `1-1_MissionStart`. [§2026-10-14T23:52:14](#ts-20261014-235214), [§2026-10-15T12:41:40](#ts-20261015-124140)
- Mission 1-1 runs end to end in `chase-game` in the original's step order (11 steps, StoryProgress 2 after 5831 updates), with teleports standing in for walks; `1-1_Deliver_Donuts` only fires after `1-1_Carry_Object_Tutorial` has ended. [§2026-10-15T12:41:40](#ts-20261015-124140) The user played it end to end in real play on 2026-10-18 [§2026-10-18T10:37:46](#ts-20261018-103746).
- The two unnamed scripts have no starter in this build: every thread start resolves a name string (24 call sites read) and no name hashes to them. The dumpster peek 2451216995 is a copy of what `1-2_HouseArrival` does inline; 2494968948 is a single no-op `PrintDebugString`. [§2026-10-18T11:14:04](#ts-20261018-111404), [§2026-10-18T19:58:54](#ts-20261018-195854)
- User playtest of mission 1-2 in `chase-game` (2026-10-18): past the Doorlock scene and the first detective point; next blocker is the smashable table `DonutTable_14` that gives the bricks of the trampoline build-it, which needs an attack on foot; cars drive through street lamps without breaking them (VEH-103). [§2026-10-18T15:27:22](#ts-20261018-152722)
- Mission 1-2 from the dog cage on: the release script blacks the screen out and the cutscene mode of `1_2_c` fades it in again; `chase-game` runs on through `1-2_MissionEnd` (StoryProgress 3) to `1-3_MissionStart`. [use-spot.md §2026-10-19T09:12:53](use-spot.md#ts-20261019-091253)
- Open: an unnamed code path firing `1-2_Door_Lock_Hint_Script`'s ObjectTriggered (not excluded), the vehicle bound test 0x17b21c, who fires class-driven ObjectTriggered events, missing natives in mission 1-1 (CameraFollow, EnableTrafficAndPedestrians, music), original update numbers between steps.


<a id="ts-20261014-213921"></a>

## 2026-10-14T21:39:21+02:00 — Mission 1-2 (the dog Pringles): natives and trigger events of the whole mission, order and call counts in the original during its first 330 updates (C++ level loader dump, `chase-game`, Azahar GDB trace of start-trigger save state, no RPC)
Corrected by [scripts-runtime.md §2026-10-20T09:22:38](scripts-runtime.md#ts-20261020-092238): the one-update lag came from the scratch harness, which updated the director after the runner; the port already matched at 153 and 305.

### Method

- **Port side.** `chase-game --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --call-script 1-2_MissionStart --script "0 0 600"` prints the first call of every function without a handler. The first twelve names (`addtosemitransnearcameralist`, `enabledistancefade`, `attachobject`, `attachtobonerot`, `dcamcutsceneload`, `playcutscene`, `vehiclesetlocked`, `vehiclesetforcedstop`, `dcamcutsceneplay`, `setpartycharacter`, `dcamcutscenewait`, `dcamcutscenefree`) belong to the level startup, which runs before `1-2_MissionStart`; the mission's own first unhandled calls follow in this order: `destroyprop`, `effectfadeobject`, `playcharanim`. The runner reports only the first use of each function and only the scripts that run, so the rest of the mission was read statically: a throwaway dump of `Level::scripts`, `triggers`, `bounds` and `objects` (`parseLevel`, not committed) gave the 28 scripts of the mission and the natives each calls, matched with `scriptFunctionSupport` through `tools/chase_native.py` (`level_census`). The closure follows `CallScript` / `SpawnScript` from the 24 scripts named `1-2_*` plus four scripts without a known name (below); `1-2_MissionEnd` spawns `1-3_MissionStart`, which belongs to the next mission and is not followed.
- **Oracle side.** Save state in front of the mission 1-2 start trigger (New 3DS; player at 884.172, 16.583, 1106.309, forward -0.977 0 -0.213, camera yaw -1.7863), Old/New mode from the state, `--renderer opengl`, no RPC. Scenario: `loadstate`, `frames 2`, `dt 0.033 0.033`, `stick 0 120`, then `ctrace 0x2342b4,0x22ce9c,0x23cee8` over 30 + 30 updates with the stick up and 120 + 150 updates with the stick released (the new lean command `ctrace` of `tools/azahar_oracle.py`: one `g` packet per stop, no stack and no pointed memory; about 1 s per update with a stop per update). 0x2342b4 is the executor's call `blx r2` of the handler (r2 = handler address, r7 = thread; [scripts.md §2026-10-14T09:52](scripts.md#ts-20261014-095252)), 0x22ce9c creates a thread, 0x23cee8 is the event firer ([script-triggers.md §2026-10-14T15:53](script-triggers.md#ts-20261014-155345)). Handler addresses were mapped to names with the census.

### The mission start in the original

- The save state works as described: the player walks 23 updates with the stick up (a first try that released the stick after 14 updates started nothing; at that point the player stood at 881.948, 16.583, 1105.822) and the mission starts. Position after 32 updates: 881.285, 16.593, 1105.860.
- **The mission script is not started by the trigger pass or by an event.** In update 25 0x22ce9c ran once, called from 0x17b040: that address is the `bl 0x22ce9c` at 0x17b03c inside the update of the waypoint object class `props/map_waypoint` (0x17acbc, entered per update for each waypoint with a state byte at component +0x78, states 0 to 3; the script id is the word at component +0x64, started when state 3 is reached and bit 0x40 of the byte at +0x8e is clear; messages 52, 53 and the fade states 0 to 2 precede it). No stop at 0x23cee8 (the event firer) happened in the 330 traced updates. The thread ran from update 26, the first update after its creation (like a queued trigger thread).
- Order of the natives of `1-2_MissionStart`, with the updates in which they were called (the update counter of the trace: 26 is the first update with a call): `=` (StoryProgress), `=` (MissionInProgress), SetMissionTitle, SetMissionSummary, EnableObject, SetActiveWayPoint, EnableObject, `=`, EnableObject x4, SetActiveWayPoint (all in update 26), StartDCam, PositionObject, ShowIntroText, WaitForDCam (update 26, called again in every update until update 179, 154 stops, the last one lets the script go on: the title camera lasts 5 s of 0.033 s updates), then in update 179: EnableObject, EnableBound x2, KillObject (the handler 0x1a2fe4 is shared with DestroyProp: the script's `DestroyProp 1-4_Police_Car`), EnableObject, EffectFadeObject, ShowDialogueTextTimed, Wait (blocked until update 331, 153 stops, the last one lets the script go on), then in update 331: ShowDialogueTextTimed, PlayCharAnim; the script ends. No other native was called in these updates (the player stood still after update 32).
- The port (`1-2_MissionStart` run with 0.033 s updates, the director updated after the runner; scratch test, not committed) reaches DestroyProp 154 updates and PlayCharAnim 306 updates after the first call, the original 153 and 305: one update later at both points. The port's end of the title camera (WaitForDCam) is one update late or the harness order differs; not traced further.

### Natives of the mission (28 scripts, 65 distinct functions, 13 without a handler)

Without a handler (call sites in the mission scripts, item):

| Native | Calls | First use in the story | Item |
|---|---|---|---|
| DestroyProp (KillObject) | 1 | MissionStart (confirmed in the original, update 179) | SCRIPT-58 |
| EffectFadeObject | 1 | MissionStart (confirmed, update 179) | SCRIPT-38 |
| SetPartyCharacter | 2 | `Bone_Trigger`, the intro script `3388757305` | SCRIPT-35 |
| DCamCutSceneLoad, DCamCutScenePlay, DCamCutSceneWait, DCamCutSceneFree | 2 each | the intro script (`1_2_a`) and `Cage_Key_Target_Script` (`1_2_c`) | SCRIPT-29 |
| StreamRoomIn | 1 | `Cage_Key_Target_Script` | SCRIPT-38 |
| EnableDistanceFade | 2 | `SO_Dspot_02_X`, `Waypoint_02_Script` | SCRIPT-57 |
| StopMusic, AutomaticMusic, OverrideMusic | 1 each | `SO_Dspot_02_X`, the Doorlock entrance script `3035472981` | SCRIPT-33 |
| SetForcedStopOnMyVehicle | 1 | the intro script | SCRIPT-36 |

With a handler that only records (stand-ins, call sites): PlayCharAnim 9, LoadAnimation 5, PlayAnimation 5, UnloadAnimation 2, StopCharAnim 4, StopAnim 1, WaitForCurrentCharAnim 1 (batch 32, now recorded: [script-natives-animation.md §2026-10-14T21:36](script-natives-animation.md#ts-20261014-213649)); SetActiveWayPoint 20, SubObjectiveGoToEnable 6, SubObjectiveArrowGoToEnable 1, OverrideScannable 2, EnableScanner 1, RemoveScannerMapMarker 1, DisableIcon 1 (batch 31, SCRIPT-71 to SCRIPT-74); ShowDialogueTextTimed 14, ShowDialogueText 8, ShowDialogBG 8, ShowTipTextTimed 10, ShowTipTextOnce 1, ShowIntroText 1, SetMissionTitle 2, SetMissionSummary 2 (batch 30, SCRIPT-66 to SCRIPT-70); AIEnable 8, AIFaceTarget 6, AIMove 3, AIResetState 2, AINoState 2, AIActive 1 (batch 34, SCRIPT-42); StartDCamTrack 3, SnapCamera 2, DCamActive 2 (SCRIPT-46); FadeScreen 5 (SCRIPT-13); ToggleUseable 1, ResetObject 2 (SCRIPT-S17, SCRIPT-S18); GiveReward 1, UnlockScanner 1, DisableSuperBuiltItSave 1, GetLanguage 1 (SCRIPT-S20, SCRIPT-S22).

Run: EnableObject 85, Wait 44, PrintDebugString 33, EnableBound 22, SetActiveWayPoint (stand-in) 20, StartDCam 13, PositionObject 12, `=` 9, EnableCollision 4, TriggerObject 6, EnableTrigger 6, SpawnScript 2, KillScript 1, IsRedBrickExtraActive 2, and the words `?`, `!?`, `??`.

### Trigger events of the mission

The mission has 22 triggers (lvl_01 triggers 9 to 30):
- BoundEntered, 7 (`Bone_Trigger`, `DogReached`, `Door_Lock_Hint`, `GrabBrickFound`, `Pringles_Cage_Locked`, `Prop_Smash_Hint`, `ShowTheDog`): the port checks them.
- ObjectBoundEntered, 3 (`1-2_Doorlock_Holmes` twice, `1-2_Dogtruck` for `1-2_MissionEnd`): checked with the placement position; both objects move or are vehicles (SCRIPT-77, SCRIPT-42, SCRIPT-43; `vehicles/vehicle_police_barker` is not created, LEVOBJ-7).
- ObjectTriggered, 12, all with objects of class `scriptobject` (`MissionStart`, `Waypoint_02_Script`, `Cage_Key_Target_Script`, `Door_Lock_Hint_Script`, `Dtv_DogSpot_01_Script`, `Reveal_Bone_Script`, `Scanned_Pringles`, `Scanner_Enter_Script`, `Scanner_Locked_Script`, `Scanner_Zoomed_Script`, `SO_Dspot_02_X`, `SO_IntroCamOff`): the port fires such a trigger only for a script's `TriggerObject` call. No script of the mission calls `TriggerObject` on them; the original's class code fires them (by the pattern of the names: the waypoint objects `1-2_Waypoint_01` / `_02` start `MissionStart` / `Waypoint_02_Script`, the scanner objects `prop_scannerzoneaudio`, the detective spot classes `ability_detective*` and the use spot `prop_simpleuse` the others; none of this was traced except the first). The port never reports them as unhandled (the report lists event names, not firing objects).
- The `unhandled events` report of `chase-game` lists `characterarrestable` and `characterarrested` for the level; neither belongs to this mission.

### Scripts without a name in the port

Four scripts of the mission have a name hash that no string of the level gives: `3388757305` (the intro: cutscene `1_2_a`, `1-2_Chase_Position_Start`, dumpster camera), `2451216995` (the dumpster peek), `3035472981` (the Doorlock Holmes entrance and conversation), `2494968948` (the hint text "Blue and white objects can be used to access new areas."). Nothing in the level's triggers, bounds or scripts starts them; the class files or object parameters that name them were not searched.

Unknowns:
- The state machine of the waypoint class 0x17acbc (which message or bound puts it from state 0 to state 3, what the fade states do, why the byte +0x8e bit 0x40 matters); only the thread start was observed.
- Who fires the class-driven ObjectTriggered events and who starts the four unnamed scripts.
- The rest of the first minute after update 331: the player stood still after the scripted part; the free-play triggers (`ShowTheDog` at 715.8, 66.0, 715.3 and the others) were not walked.
- Why the port is one update behind at the end of the title camera.

<a id="ts-20261014-214429"></a>

## 2026-10-14T21:44:29+02:00 — Waypoint TriggerScript names two of the unnamed mission 1-2 scripts

Source: instance parameters of `lvl_01_cherrytreehills_main.blvl` (`tools/convert_levels.py --param-names`) and `nameHash` of each `TriggerScript` value (scratch program, not kept). Lead check during the merge.

- Every waypoint object (`props/map_waypoint`, `map_vehiclewaypoint`, `map_getintovehiclewaypoint`) has a string parameter `TriggerScript` naming a level script; lvl_01 has 22 distinct values. The script id at component +0x64 that 0x17b03c starts (see above) is presumably this parameter's hash; not traced.
- `nameHash("1-2_HouseArrival")` = 3388757305 (the unnamed intro with cutscene `1_2_a`), parameter of `1-2_Waypoint_02` (room40).
- `nameHash("1-2_Spawn_Doorlock")` = 3035472981 (the Doorlock Holmes entrance), parameter of `1-2_Waypoint_03` (room40).
- `1-2_Waypoint_01` names `1-2_MissionStart`; `1-2_Waypoint_04` names `1-2_Waypoint_02_Script`.
- The same mechanism starts mission 1-1's steps: `1-1_Waypoint_01` (the donut shop) names `1-1_VanBreakdown` ([objective-hud-waypoints.md §2026-10-14T21:23](objective-hud-waypoints.md#ts-20261014-212357)).

Unknowns:
- Starters of 2451216995 and 2494968948 (no `TriggerScript` value hashes to them).
- Whether component +0x64 is filled from `TriggerScript`.

<a id="ts-20261014-224825"></a>

## 2026-10-14T22:48:25+02:00 — The waypoint class: init, update 0x17acbc, reach test, timings (llvm-objdump of game.arm.elf; Azahar GDB, state `gameplay-start`, no RPC)

[Corrects objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919): a disable of SetActiveWayPoint is ignored when bit 4 of +0x8e is clear (the earlier unknown "port always disables").

Resolves the unknowns of [§2026-10-14T21:39](#ts-20261014-213921) (state machine) and [§2026-10-14T21:44](#ts-20261014-214429) (+0x64 from `TriggerScript`). One C++ class serves `props/map_waypoint`, `map_vehiclewaypoint` and `map_getintovehiclewaypoint` (vtable 0x165fc4: init 0x17aa34, setup 0x170fd8, update 0x17acbc); the three differ by class data only.

**Init 0x17aa34 (component 0x90 bytes at entity+0x7c).** +0x88 `WaypointType` (unused by the update), +0x60 the bound named `TriggerBound` in the *class file* (0x23e798: record stride 40 in the class's bound table; props/map_waypoint: shape 4 cylinder, centre (0, 0.7603, 0), radius 1, half height 0.902, active byte 1; vehicle classes radius 4, half height 3.4, centre y 3.4), +0x68 `BadgeFadeInTime`, +0x6c `FadeScaleInTime` (0.8 small, 1.0 large), +0x70 `FadeScaleOutTime` (0.4 small, 1.0 large), bit 3 of +0x8e = `TakePlayerControl` (class default 1, instances may set 0), bit 4 of +0x8e = (`AutoTrigger` == 0) (class default 1), +0x84 = -1, +0x64 = 0x2388e4(level root's script table, string of `TriggerScript`) when the string is not empty (the thread's script record; confirms the parameter), +0x8d a byte from an int parameter (0x2451d0, name not resolved). Hashes: TriggerScript 0xd3575d66, FadeScaleInTime 0xd7954f4e, FadeScaleOutTime 0x190be976, TakePlayerControl 0x59521dda, AutoTrigger 0xc93e4625, VehicleString 0x7629ded9. The entity starts hidden (0x2428c0).
**Setup 0x170fd8 (kind at +0x8c).** `NextWayPoint` +0x4c, `Badge` child +0x5c; an instance with a `VehicleString` parameter gets +0x8c = 1 (+0x50 the named vehicle or +0x54 `VehicleType`); `GetIntoVehicle` int sets +0x8f bit 0; an instance with a `UseMarker` object gets +0x58 and +0x8c = 2. Plain `map_waypoint` instances: kind 0.
**Enable 0x229c98 (SetActiveWayPoint through 0x26e168).** 0x26e168 runs only when enable != 0 or bit 4 of +0x8e is set, so a disable is ignored for AutoTrigger waypoints (corrects the open question in [objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919); the port now ignores it too). Enable: bit 0 and bit 1 of +0x8e set, bit 2 cleared, state +0x78 = 0, timer +0x74 = 2^-23, entity shown.
**Update 0x17acbc.** Runs only when bits 0 and 1 of +0x8e are set. (1) Timer: when +0x74 > 0 it is reduced by dt, clamped to 0, and only the update in which it reaches exactly 0 runs the state handler. State 0: fade-in message, state 1, timer = FadeScaleInTime, message 53. State 1: nothing relevant. State 2: fade-out message, state 3, timer = FadeScaleOutTime, with TakePlayerControl bit 4 of the player controller byte +0xd8 is set (+0x241304), message 52 (0x2e5 when 0x270484() != 0). State 3 (bit 6 of +0x8e clear): `0x22ce9c(0x23c188(scene) = level root, +0x64)` at 0x17b03c, the player flag is cleared again, and when +0x58 == 0 the waypoint leaves the active list (0x229aa4) and bit 0 of +0x8e is cleared: it switches itself off. (2) Reach test (every update, after the timer step, when +0x60 is set, bit 4 clear, bit 2 clear): r6 = the player's vehicle (0x22ec10 / 0x24255c, controller type 282 to 284 or 411, +0x130), else 0; kind 0 is eligible always, kind 1 with no named vehicle needs r6 != 0 and `VehicleType` equal; point = the player node translation (+0x30 of 0x250810) in the waypoint's frame (0x2536c0, inverse rotation), tested with 0x22f650(point, bound, 0): active byte +0xa set, shape 4: horizontal length < +0x1c and |y| < +0x20. Hit with bit 6 clear: kind 2 moves state 1 to 2; kind 0 with r6 != 0 shows a hint and does not fire (unless the vehicle's class hash is `Vehicle_RC`); otherwise bit 2 of +0x8e is set (fired), state = 2, timer = 0.001, +0x7c = the player, +0x80 = the vehicle. For a vehicle the position of the vehicle node is tested as a second try (0x17b21c, not transcribed). Bit 6 of +0x8e is never set in this class (setter not found).
**Oracle (GDB ctrace 0x17b3a4 = hit, 0x17b03c = thread start; dt 0.033 fixed; runs d1 to d3, not kept).** State `gameplay-start`, teleport to 819.0, 25.0, 1072.0, stick (0, 120): hit in update 36 (player node 818.2785, 24.505, 1071.4512: horizontal distance 0.962 from the waypoint, the update before 1.18 was outside), thread start in update 50 (r1 = the script record, r7 = the waypoint entity), exactly 14 updates later (state 2 in update 37, FadeScaleOutTime 0.4 needs 13 more). Trace d2 (same input): the player stops moving from update 38 (controller flag byte 0, state 1) until update 50, drifting 0.02 in 13 updates, flag back at 51; in update 52 the script moves the player (817.879, 1071.297), in 55 to 817.025, 1073.166. The first executor pass of the new thread is the next update. A teleport to 819.0, 25.0, 1072.0 alone (distance 1.83) hit nothing in 40 updates (d1), as radius 1 predicts. A walk from the start with a fixed stick circles because the camera follows the heading; the full walk was not repeated.
**Port check.** `chase-game --level lvl_01 --call-script 1-1_MissionStart --start "819.0 25.0 1072.0" --facing 225 --script "0 120 80"` prints `waypoint started script 1-1_VanBreakdown at update 18`; player heights of the first six updates equal the oracle rows (24.9505, 24.8515, 24.703, 24.505, 24.2575, 23.9252). `1-1_VanBreakdown` then runs through `CameraFollow` (no handler) to the end of what the port has; the trigger `playervehicleentered` of step 4 has no check (vehicles, SCRIPT-S29).

Unknowns:
- The second (vehicle) bound test 0x17b21c, VehicleType matching, the hint text for a foot waypoint hit from a vehicle, kind 2 (UseMarker) hits, who receives messages 52 / 53, the effect of +0xd8 bit 4 on the player, and the update order against the trigger pass inside one update.

<a id="ts-20261014-235214"></a>

## 2026-10-14T23:52:14+02:00 — Which mission script the lvl_01 startup runs per StoryProgress (C++ script parser, `chase-game` runs; no oracle)

- The startup script of lvl_01 (decoded with the C++ parser, 339 calls) ends the `MissionHide` calls with `? StoryProgress == 1` then `CallScript 1-1_MissionStart`; `StoryProgress == 2` calls `1-1_MissionSkip` and `1-2_MissionAvailable`; 3 to 9 call the `Skip` scripts of the finished chapters and the `MissionStart` of the next one. `StoryProgress` is the level default 1.0 on a new game, so the new-game start runs `1-1_MissionStart`: `SetMissionTitle/Summary`, `DCamCutSceneLoad 1_1_a`, `? 1-1_IntroPlayed == 0` then `PlayCutscene 01_GameIntro` and `1-1_IntroPlayed = 1`, `QuickSave`, `DCamCutScenePlay 1_1_a 0 1 1 1`, `SetPartyCharacter Player1 9 1`, `EnableObject 1-1_Waypoint_01 1`, `SetActiveWayPoint 1-1_Waypoint_01 1`, `DCamCutSceneWait`, `DCamCutSceneFree`, mail box collision off, `Donut_Box_Empty` on, `PositionObject Player1 1-1_Chase_Intro_Shot_Dummy`, `Wait 1`, `SpawnScript 1-1_OnScreenText_Script`.
- In the current port the waypoint is on after the startup run: `chase-game --level lvl_01 --start "819.0 25.0 1072.0" --facing 225 --script "0 120 80"` without `--call-script` prints `waypoint started script 1-1_VanBreakdown at update 18`, as with `--call-script`. `level-start-real-data` pins both cases: StoryProgress 1 leaves `1-1_Waypoint_01` active and `1-2_Waypoint_01` off, StoryProgress 2 the reverse (`1-2_MissionAvailable`: `StoryProgress = 2`, `MissionInProgress = 0`, `SetActiveWayPoint 1-2_Waypoint_01 1`). The save path (`--save`) sets StoryProgress before the startup run (`game_save::apply`).

<a id="ts-20261015-124140"></a>

## 2026-10-15T12:41:40+02:00 — First end-to-end run of mission 1-1 in `chase-game` (CTests `game-mission-1-1`, `game-mission-1-1-start`; no oracle, no REA)

Method: `chase-game --script` with the new dev step `at X Y Z [FACING]` (teleport before the next update) and the new log lines `script NAME started/ended at update N`, `cutscene NAME` / `movie NAME started/ended at update N`, `StoryProgress N` at the end. The run does not use `--call-script`; every step is started by the level itself.

**Result.** All 11 steps run in the original's order up to `1-1_MissionEnd` (StoryProgress 2, 5831 updates with `--no-start-cutscenes`). Update numbers (0.033 s steps, port values, not oracle values): `1-1_VanBreakdown` 47, `1_1_b` 48 to 490, `1-1_Get_Car` 824, police car entered 893, `1-1_Release_Car_Script` ends 1201, `1-1_Found_Van` 1328, build-it done and `1-1_SO_VanFixed` 1608, van entered 2187, `1-1_Van_Waypoint_Enter_Script` 2197, `1-1_VanEscorted` 2746, `1_1_d` 2747 to 2978, `1-1_Carry_Object_Tutorial` 3673 (ends 4100), pickup `1-1_SO_Donuts_PickedUp` 4127, `1-1_Deliver_Donuts` 4320, `1-1_SO_MissionEnd` 4339, `1_1_e` 4380 to 4669, `1-1_MissionEnd` 4669 to 5326.

**Teleports (dev steps standing in for walks, listed in the CTest):** onto the shop waypoint (819, 25, 1072), onto `1-1_Waypoint_02` (888.6, 16.58, 1115.9), out of the police car straight onto `1-1_Waypoint_03` (877.3, 52.6, 702.0; the 400 unit drive to room 33 is not driven), next to the build-it (874.0, 52.0, 695.5), next to the donut van (866.0, 52.0, 691.0), onto `1-1_Waypoint_Donuts` (817.9, 23.94, 1071.9) and into the target zone with the box (881.897, 16.597, 1120.189, the oracle's placing position, [carrying.md](carrying.md)). Walked or driven for real: the last steps onto waypoint 01, the 60 updates in the police car, the drive from the van to `1-1_Waypoint_05` (as `game-van-escort`), pickup and placing of the box.

**Timing lesson.** `1-1_Deliver_Donuts` (generic trigger pass) only fires after `1-1_Carry_Object_Tutorial` has ended (it enables the trigger at its end); placing the box earlier skips it. The original's order was not measured for this early placing.

**New-game start order (MOVIE-23).** Without `--no-start-cutscenes` the start runs `movie 01_GameIntro` (update 0 to 2066), `cutscene 1_1_a` (2066 to 2506), `1-1_MissionStart` ends 2537, then later `1_1_b`, `1_1_d`, `1_1_e`: the order the script gives. In the original save state at the end of the cutscene 1_1_b the sequence was saved near its end: the viewer (`chase-viewer --cutscene csdata/models/1_1_b --frame 330`) shows the police garage with the car at frame 330, so "Go to your patrol car" is the closing line of `1_1_b`, not a different cutscene.

**Harbour view (COLL-22).** `chase-game --start "886.681 16.590 1112.930" --call-script 1-1_VanBreakdown --frames 25` without `--no-start-cutscenes` shows the intro movie `01_GameIntro` (statue and harbour on the top screen, hexagon background below, subtitle "The city where I became a cop..." at update 330), played by the level start, not the game camera. The earlier comparison runs of COLL-21 lacked `--no-start-cutscenes`; the MOVIE-21 runs had it.

**Gaps seen.** Natives without handler in the run: CameraFollow (`1-1_VanBreakdown` end), EnableTrafficAndPedestrians (`1-1_Found_Van`), OverrideMusic (`1-1_Carry_Object_Tutorial`), AutomaticMusic (`1-1_SO_MissionEnd`), SetPartyCharacter and the two startup natives. `1-1_Release_Car_Script` calls PositionObject for `1-1_Police_Car` to `1-1_Dcam_Exclusion_Police_Car_Resnap`, which the port does not apply (only the player moves). The ordering against the original of everything between the waypoints (walking times) is not measured.

Unknowns:
- Whether the original's update numbers between the steps agree with the port's; only single steps were measured so far.
- The 442 update length of `1_1_b` in the port against the original.

<a id="ts-20261018-103746"></a>

## 2026-10-18T10:37:46+02:00 — Mission 1-1 played end to end in chase-game

- The user played mission 1-1 through in `chase-game` on main at f639069 or later (car break and particles merged) and reported that it can be played end to end. No difference to the original was reported.

Unknowns:
- Timings between the stops and details of the look were not compared in real play; the user's report covers playability only.

<a id="ts-20261018-111404"></a>

## 2026-10-18T11:14:04+02:00 — The two unnamed mission 1-2 scripts: contents, negative name searches and a start trace (C++ level loader scans; Azahar GDB, alley-trigger save state and the first-trigger save state, no RPC)

Partial result for SCRIPT-113: the names are still unknown.

- **2451216995 (0x921a9a63), 12 calls:** `TriggerObject(1-2_Doorlock_Spawner, 1)`, `Wait(0.1)`, `StartDCamTrack(Dcam_Dumpster, 1-2_Dumpster_Open, 0, 5)`, `PlayCharAnim(1-2_Doorlock_Holmes, "dumpster_peak_minifig", ...)`, `LoadAnimation(1-2_Dumpster_Open, "dumpster_peak", ...)`, `EnableObject(1-2_Dumpster_Closed, 0)`, `EnableObject(1-2_Dumpster_Open, 1)`, `PlayAnimation(1-2_Dumpster_Open, "dumpster_peak", ...)`, `WaitForDCam`, `SnapCamera`, `Wait(1.5)`, `EnableObject(1-2_Doorlock_Holmes, 0)`: Doorlock Holmes peeks out of the dumpster; `1-2_Spawn_Doorlock` (0xb4eda455) later loads `dumpster_exit`.
- **2494968948 (0x94b63474), 1 call:** `PrintDebugString("HINT: Blue and white objects can be used to access new areas.", 5)`.
- **No text names them.** No NUL-delimited printable run (also UTF-16LE) of `lvl_01_cherrytreehills_main.blvl` or of any of the 4959 non-media files of the archive hashes to either value (whole runs and substrings of up to 80 characters starting in the first 8 characters of a run); neither value is a literal in `game.arm.elf`; no instance, class, bound, trigger or script-argument name of the level combined with about 60 suffixes (`_Count`, `_ON`, `_OFF`, `_Tally`, `_Script`, ...) and 8 prefixes hashes to them.
- **Where the hashes occur in the level file:** both as script records (section +0x18, stride 0x1c); 0x94b63474 also in the level's default parameter table (section +0x10, 52 entries {name hash, type, value}, the level variables), as do the hashes of three other scripts (0xf1781a95, 0x64e1bad5, 0xa3ed4e71).
- **35 of the 314 scripts of lvl_01 are named by no trigger, CallScript / SpawnScript / KillScript and no string parameter.** Seven of them are named after a variable, an object or a mission (`1-3_BuildingSite_Supercounter`, `GameComplete`, `Race01Progress05Player`, `music_timer`, `1-5_WireTap_02`, `1-4_AI_Goon_10_Run_Off_Script`, `1-3_MissionEnd`), `startup` is started by name from code, 27 remain unnamed (including both). So scripts are started by names that are built or taken at run time. The `=` handler 0x17efc0 only stores the value (no script start).
- **The code that starts scripts:** thread creation 0x22ce9c is called from 0x174744, 0x17b03c (waypoint class), 0x1c3744 (CallScript), 0x1ecc9c and 0x1ecd00 (`startup` / `OnSceneEnter%d` by name, 0x1ecc50), 0x2e567c, 0x2f688c and 0x2fe44c; the lookup by name 0x2388e4 from 0x17abb4 (the waypoint's `TriggerScript` parameter), 0x197c98, 0x1a56f4, 0x1ecc84, 0x1eccec, 0x211fe4 (name from a parameter token, 0x21b3fc), 0x2238f8, 0x2e5658, 0x2f6870 and 0x2fe440. Format strings in the executable that make script names: `OnSceneEnter%d` (0x97768), `%i-%u_MissionStart` and `%i-%u_MissionEnd` (0xa1940, 0xa1928), `%s_Count`, `%s_ON`, `%s_OFF`, `%s_Tally` (0x5c4a4 to 0x5c4f4).
- **Oracle trace (ctrace of 0x22ce9c, `dt 0.033`, stick forward):** from the alley-trigger save state one thread is created in update 28 with lr 0x17b040 and the script record's first word 0xb4eda455 (= `nameHash("1-2_Spawn_Doorlock")` = 3035472981): the waypoint class starts `Spawn_Doorlock`, as known. From the first-trigger save state one thread is created in update 28 with lr 0x17b040 (record 0x08618d7c, hash not read) and none in the next 172 updates. Neither shows the dumpster script or the hint.

Unknowns:
- Who starts 0x921a9a63 and 0x94b63474. Candidates left: a name made from a variable or object name by code (0x211fe4, 0x1a56f4, 0x197c98 not read), or a start in a step of the mission the traces did not reach (the dumpster peek comes before `Spawn_Doorlock`; the hint with the bound `1-2_Prop_Smash_Hint`). Next: ctrace of 0x22ce9c and 0x2388e4 from a state before the alley with the player walking into the dumpster area.
- The meaning of scripts that share their hash with a level variable.

<a id="ts-20261018-152722"></a>

## 2026-10-18T15:27:22+02:00 — User playtest of mission 1-2 in `chase-game`: detective point, the table to smash, cars and street lamps

Source: the user's own play of `chase-game` from their save (`--save ... --slot 0`, street level) after the round 20 merges, and the level data of lvl_01 (`tools/convert_levels.py --param-names`).

- After the Doorlock scene the first detective point at 723.58 29.77 630.54 works with A (Space): the user sees the footprints on the ground. The user reports that the detective look is "a bit off" and that Chase's magnifying glass is not drawn.
- `1-2_Doorlock_Holmes` stays in the bin (the `PositionObject` gap of SCRIPT-287).
- Next step of the mission in the original: hit the table at 718.06 32.68 642.69 (facing -80.2 degrees); when it is destroyed its bricks build a trampoline. Level data around that spot (room40): `DonutTable_14` (class `leprops/leprop_wobblydestructible`, 715.63 32.53 643.67, with `DonutTable_14_Script`), the build-it `Trampoline_03_Buildits` (`props/prop_buildit`, 718.07 32.62 643.32), the bouncer `Trampoline_03` (`leprops/leprop_bouncer`, 720.20 32.58 637.47) and `1-2_Temp_Trampoline` (`props/prop_prop`). The port has no attack on foot, and the class `leprop_bouncer` is not ported.
- Cars drive through street lamps in `chase-game` without breaking them; other props do break. lvl_01 has 63 lamp objects of class `leprops/leprop_wobblydestructible`, the class of the table too. The port smashes only the wobbly props with a box shape, not the mesh kind (VEH-103); which kind these lamps are was not checked.

Unknowns:
- Whether a hit of the attack, a vehicle or something else destroys the table in the original, and how the build-it is enabled from its bricks (`DonutTable_14_Script` not read).
- Whether the street lamps are wobbly props of the mesh kind (VEH-103).


<a id="ts-20261018-195854"></a>

## 2026-10-18T19:58:54+02:00 — The two unnamed mission 1-2 scripts have no starter in this build; Door_Lock_Hint_Script's object trigger (REA and objdump on game.arm.elf, full substring scan, Azahar GDB traps without RPC)

Resolves the unknown of [§2026-10-18T11:14:04](#ts-20261018-111404) for SCRIPT-113 and narrows SCRIPT-285.

**What the two scripts are.**
- `1-2_HouseArrival` (3388757305, the `TriggerScript` of `1-2_Waypoint_02`) carries the dumpster peek inline: after `PlayCharAnim(1-2_AI_Dogman, crying_idle_minifig)` and `Wait 1` it calls `TriggerObject(1-2_Doorlock_Spawner, 1)`, `Wait 0.1`, `StartDCamTrack(Dcam_Dumpster, 1-2_Dumpster_Open, 0, 5)`, then a dialogue line, the same `PlayCharAnim` / `LoadAnimation` / `EnableObject` x2 / `PlayAnimation` of the dumpster, `wait 1`, the enable of `1-2_Waypoint_03`, `WaitForDCam`, `SnapCamera`, `Wait 0.6` and `EnableObject(1-2_Doorlock_Holmes, 0)`. All ten non-wait calls of 0x921a9a63 appear there in the same order with the same arguments; only the last wait differs (0.6 against 1.5). So the peek runs as part of `HouseArrival`; 0x921a9a63 is a copy of it.
- 0x94b63474 is one `PrintDebugString("HINT: Blue and white objects can be used to access new areas.", 5)`; the handler 0x26fde8 is `mov r0,#1; bx lr` ([script-natives-census.md](script-natives-census.md)), so the script has no effect even when run. (Not the smash hint: that is the named `1-2_Prop_Smash_Hint`, BoundEntered of the bound of that name.)

**Every thread start needs a name string (static).**
- Threads come from the free list 0x589734. Its only users (REA xrefs) are the pool setup 0x15bdfc, the scene start 0x197c3c (creation inlined, name `startup`), the return to the list in 0x1a5338 (end of all threads of a scene) and 0x234358 (end of a thread), and the two creators 0x22ce9c(owner, record) and 0x2238d0(owner, name). 0x2238d0 is a second start-by-name helper that the earlier list missed: it looks the name up with 0x2388e4 and creates the thread inline.
- 0x2388e4(owner, name) hashes the part of the name after its last `.` (0x23895c; the whole name when there is no dot or the dot is at index 0) with the lowercased CRC-32 (0x24da28, `assets::nameHash`) and compares it with the first word of the owner's script records (+0x24 → +0x18, stride 0x1c).
- The 24 direct call sites (objdump, ARM) and their name sources. 0x22ce9c: 0x174744 trigger queue (record from the trigger's script name, resolved by name in the trigger setup 0x1a553c at 0x1a56f4); 0x17b03c waypoint (record from the `TriggerScript` string, 0x17abb4); 0x1c3744 CallScript / SpawnScript (record from the argument token, looked up at run time by the argument evaluator 0x211e04 at 0x211fe4, whose only caller is the executor 0x234204); 0x1ecc9c / 0x1ecd00 (`startup`, `OnSceneEnter%d`, 0x1ecc84 / 0x1eccec); 0x2e567c (`ScriptCheckExtras`); 0x2f688c (the name in 0x516422, written only by the native `PlayScriptOnStartup` 0x2915d4 from its string argument); 0x2fe44c (`RaceEventToggled`). 0x2238d0: 0x189b80 / 0x189c10 (`EffectScript`, `EffectStoppedScript`), 0x1ef224 (`3-5_AfterMinigame`), 0x26fd04 and 0x27bca0 (the string arguments of the natives `IfPartyCharacter` and `IfPlayerCharacter`), 0x2c1824 (`registersounds`), 0x2d7dbc (`CraneDropOffControl%02d`, slot 1 to 3).
- So every script start resolves a name that comes from level script text (trigger script names, a waypoint parameter, script-call arguments), from a literal of the executable or from a format string of the executable.

**No such name hashes to either script.**
- Every substring of up to 160 characters of every printable run (ASCII and UTF-16LE) of all 12836 extracted archive files, `code.bin`, `exheader.bin` and the ExeFS (1.19e9 substrings, scratch C++ scanner): the only hits of 0x921a9a63 and 0x94b63474 are one each inside texture pixel data (`lightmap_apollo_drheinriks_lab000.btga`) and model index data (`char_head_dogwalker.bfnmdl`), as expected from random CRC collisions at that count (about 0.3 per value), not names. This covers the suffix after a dot as well.
- Run-time names: `OnSceneEnter0..999`, `CraneDropOffControl00..999` and `%i-%u_MissionStart` / `_MissionEnd` for missions -5..29 and 0..49 do not match. The other format strings that make names (`%s_Count`, `%s_ON`, `%s_OFF`, `%s_Tally` and similar) are not on any start path above.
- Level data (`assets::parseLevel`): no trigger script name, string parameter, object reference or script-call argument of lvl_01 hashes to either value (CTest `script-starters-real-data`).

Conclusion: in this build nothing can start 0x921a9a63 or 0x94b63474. They are leftovers: the peek was folded into `HouseArrival`, the hint is a debug print.

**Supporting traps (own emulator, port 24735, own home, OpenGL, no RPC; new command `freetrap`, [oracle-measuring.md §2026-10-18T19:58:54](oracle-measuring.md#ts-20261018-195854)).** Breakpoints at 0x22ce9c, 0x2238d0 and 0x2388e4, and from the second-waypoint save state on also 0x244f40 and 0x21a560; the record word at [r1] names the script:
- the first-trigger save state: 40 stepped updates with the stick up start `HouseArrival` (0xc9fc5539) in update 28 from the waypoint (lr 0x17b040), then `freetrap 150`: no stop, and the player ends at 745.731 29.236 631.241 facing -x (the `PositionObject` of the script's end), so the peek ran inside `HouseArrival` with no other start.
- the alley-trigger save state: 20 stepped updates with the stick up, then `freetrap 120`: `Spawn_Doorlock` (0xb4eda455) by the waypoint at 0.63 s and `1-2_Doorlock_Bound_Entered_01` (0x149fd6c7, lr 0x174748) at 39 s, nothing else.
- the second-waypoint save state: `freetrap 20`, A held for 4 stepped updates, 2 more, `freetrap 60`: no stop at all (no thread start, no 0x244f40 and no queued trigger).
- the dog cage save state: A for 3 updates fires 0x244f40 from the use spot (lr 0x192358) and the queue 0x21a560, and starts `Cage_Key_Target_Script` (0x652f4480, lr 0x174748); in the next 60 s CallScript starts `1-2_MissionEnd`, `1-3_MissionStart` and `1-3_Animations`, each by its argument text through 0x211fe4 → 0x2388e4 and 0x1c3748 → 0x22ce9c.
- the table save state: `freetrap 60` with no input: no stop.

**`1-2_Door_Lock_Hint_Script` (SCRIPT-285).** The name occurs in the whole archive only in lvl_01: the scriptobject's instance name, the object of its ObjectTriggered trigger and the script name of that trigger and of the BoundEntered trigger of the bound `1-2_Door_Lock_Hint`. No object reference points at the scriptobject (resolved scan of all type-4 parameters of lvl_01); the detective spot next to it, `1-2_Dtv_DogSpot_02` (720.391 37.040 654.534), has no `TriggerObject`, while `1-2_Dtv_DogSpot_01` has one (`1-2_Dtv_DogSpot_01_Script`). No script calls `TriggerObject` on it. The trap from the second-waypoint save state (standing in the bound, A pressed) saw no 0x244f40 and no queued trigger. The hint therefore runs from its bound, as the port does.

Unknowns:
- ObjectTriggered of `1-2_Door_Lock_Hint_Script` from code that finds an entity without a name (by position or class) is not excluded: the 28 call sites of 0x244f40 and the 53 of the event firer 0x23cee8 were not read one by one.
- Whether a thread can switch to another script record after its creation (thread +0x10 written elsewhere) was not checked.
