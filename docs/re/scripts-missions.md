# Mission scripts: gap analysis of what a mission needs

What the scripts of one mission call, which natives and trigger events the port lacks, and what the original does at the start of the mission. Script engine: [scripts.md](scripts.md); natives: [script-natives.md](script-natives.md); trigger events: [script-triggers.md](script-triggers.md). Items: [script-system-missions.md](../plans/features/script-system-missions.md).

<a id="ts-20261014-213921"></a>

## 2026-10-14T21:39:21+02:00 — Mission 1-2 (the dog Pringles): natives and trigger events of the whole mission, order and call counts in the original during its first 330 updates (C++ level loader dump, `chase-game`, Azahar GDB trace of state `mission-1-2-start`, no RPC)

### Method

- **Port side.** `chase-game --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --call-script 1-2_MissionStart --script "0 0 600"` prints the first call of every function without a handler. The first twelve names (`addtosemitransnearcameralist`, `enabledistancefade`, `attachobject`, `attachtobonerot`, `dcamcutsceneload`, `playcutscene`, `vehiclesetlocked`, `vehiclesetforcedstop`, `dcamcutsceneplay`, `setpartycharacter`, `dcamcutscenewait`, `dcamcutscenefree`) belong to the level startup, which runs before `1-2_MissionStart`; the mission's own first unhandled calls follow in this order: `destroyprop`, `effectfadeobject`, `playcharanim`. The runner reports only the first use of each function and only the scripts that run, so the rest of the mission was read statically: a throwaway dump of `Level::scripts`, `triggers`, `bounds` and `objects` (`parseLevel`, not committed) gave the 28 scripts of the mission and the natives each calls, matched with `scriptFunctionSupport` through `tools/chase_native.py` (`level_census`). The closure follows `CallScript` / `SpawnScript` from the 24 scripts named `1-2_*` plus four scripts without a known name (below); `1-2_MissionEnd` spawns `1-3_MissionStart`, which belongs to the next mission and is not followed.
- **Oracle side.** State `mission-1-2-start` (the user's, New 3DS; player at 884.172, 16.583, 1106.309, forward -0.977 0 -0.213, camera yaw -1.7863), Old/New mode from the state, `--renderer opengl`, no RPC. Scenario: `loadstate`, `frames 2`, `dt 0.033 0.033`, `stick 0 120`, then `ctrace 0x2342b4,0x22ce9c,0x23cee8` over 30 + 30 updates with the stick up and 120 + 150 updates with the stick released (the new lean command `ctrace` of `tools/azahar_oracle.py`: one `g` packet per stop, no stack and no pointed memory; about 1 s per update with a stop per update). 0x2342b4 is the executor's call `blx r2` of the handler (r2 = handler address, r7 = thread; [scripts.md §2026-10-14T09:52](scripts.md#ts-20261014-095252)), 0x22ce9c creates a thread, 0x23cee8 is the event firer ([script-triggers.md §2026-10-14T15:53](script-triggers.md#ts-20261014-155345)). Handler addresses were mapped to names with the census.

### The mission start in the original

- The user's state works as described: the player walks 23 updates with the stick up (a first try that released the stick after 14 updates started nothing; at that point the player stood at 881.948, 16.583, 1105.822) and the mission starts. Position after 32 updates: 881.285, 16.593, 1105.860.
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
- The same mechanism starts mission 1-1's steps: `1-1_Waypoint_01` (the donut shop) names `1-1_VanBreakdown` ([objective-hud.md §2026-10-14T21:23](objective-hud.md#ts-20261014-212357)).

Unknowns:
- Starters of 2451216995 and 2494968948 (no `TriggerScript` value hashes to them).
- Whether component +0x64 is filled from `TriggerScript`.
