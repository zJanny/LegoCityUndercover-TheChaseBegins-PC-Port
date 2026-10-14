# Script system

## 1. Scope

The level scripts (level section +0x18), the per-update runner 0x169254, the executor 0x234204 and its words, Wait, CallScript / SpawnScript, the natives with a handler (EnableObject, EnableCollision, TriggerObject, EnableTrigger, EnableBound), the trigger pass 0x1746c0 with the bound checks, the `startup` script, the revealer links and the start state of placed objects. At run time the original also starts scripts from events and class scripts; the port does not start those yet. Mission flow and enemies are not covered ([README.md](README.md) lists them as not started).

## 2. Status

IN_PROGRESS, updated 2026-10-14T13:36:20+02:00. `ScriptRunner` runs threads every update like the original (waits, CallScript, SpawnScript). The `startup` script, player bound triggers and `TriggerObject` start scripts; events, class scripts and other trigger types do not. 20 natives run (`WarningDialogBox`, `PositionObject` for the player and `FadeScreen` among them); the other 224 are sorted into batches SCRIPT-26 to SCRIPT-39.

## 3. Verified behaviour

- Script data layout and loader 0x1cfc50; function names re-derived by hash (0x208a3c). lvl_01 has 314 scripts.
- Over the 896 lvl_01 objects that would register collision, the state rule gives 819 enabled in both, 48 flag-0 in both, 4 without entity and disabled by the script, 12 flag-0 only through the revealer links, 8 co-located twins; no object that the rule disables while its entity is enabled. `level-start-real-data` pins this against the oracle grid read.
- The runner is entered once per update per resident scene from the gameplay update (oracle trap, lr 0x2e54b8), and no thread is alive at gameplay-start or police-station; `level-start-real-data` checks that the lvl_01 startup threads all end.
- Executor, Wait, CallScript / SpawnScript and thread creation rules were transcribed from the disassembly; `script-runner` covers them with synthetic scripts.
- After the startup run, 236 of 241 trigger enable bytes and 479 of 482 bound enable bytes equal the oracle read at gameplay-start, and the bound room rule gives exactly the oracle's 257 active bounds for its 12 loaded rooms; `level-start-real-data` pins all three.

## 4. Stand-ins

- SCRIPT-S1 The startup threads are stepped before the first frame with dt 0.034 (the free-running dt read at gameplay-start; the loading phase's own updates are not recovered): `chase-game` until `startup` ends, `levelStartState` until no thread is left.
- SCRIPT-S2 Closed 2026-10-14: `IsLevelUnlocked`, `GetGameComplete` and `IsRedBrickExtraActive` read the loaded slot ([save-format.md §2026-10-14T11:53](../../re/save-format.md#ts-20261014-115300)); without `--save` they return the new-game values (level 2 unlocked, rest 0). The mask of `IsRedBrickExtraActive` is a runtime value that is 0 after a load.
- SCRIPT-S3 The revealer rule is a data fit, not traced to its code; verified only in lvl_01.
- SCRIPT-S4 A function without a handler does nothing and continues at once; `chase-game` prints its name the first time (`script function without handler`). The original may block in it (WaitForDCam, DCamCutSceneWait, AIMove, ...). See SCRIPT-7.
- SCRIPT-S5 A CallScript / SpawnScript of a script the level does not have continues (the original's result with a null script is unknown).
- SCRIPT-S6 `chase-game` runs the scripts and the trigger pass after the door update; where 0x2e5158 runs them relative to the player and camera was not compared.
- SCRIPT-S7 `chase-game` counts every room as loaded for the bound switch +0xb, because it shows all rooms (WSTREAM-S1, see WSTREAM-3 in [world-streaming.md](world-streaming.md)); the original only counts the loaded rooms.
- SCRIPT-S8 Triggers start enabled (fits 236 of 241 records; the record builder was not read), and the object flag 0x100000 that forces an ObjectTriggered start is taken as clear. The trigger pass does not run while the startup is stepped before the first frame.
- SCRIPT-S9 `WarningDialogBox` is drawn with positions and icon size fitted to one oracle screenshot (`drawWarningDialog`), English table; `chase-game` pauses the whole update while it is open (the original runs no scene thread; the rest of its update is not traced). needs: SCRIPT-13. evidence: [scripts.md §2026-10-14T11:13](../../re/scripts.md#ts-20261014-111312).
- SCRIPT-S10 `FadeScreen` fades linearly over 0.5 s (the duration is the handler's immediate) with a black veil; the tween curve and the end test 0x25207c are not read. needs: SCRIPT-13.

## 5. Open items

- [ ] SCRIPT-1 — Scripts run during play and player bound triggers start them, but most natives have no handler and events, class scripts and most trigger types start nothing: no mission flow. needs: SCRIPT-6, SCRIPT-7, SCRIPT-11. blocks: AUDIO-2, LEVOBJ-3, STUD-4, HUD-9, TRANS-3, TRANS-5, WSTREAM-6. evidence: [scripts.md §2026-10-14T09:52](../../re/scripts.md#ts-20261014-095252).
- [ ] SCRIPT-2 — `levelStartState` lists some objects as disabled although the original has them enabled (hydrant sprays, three character tokens, task and archway pads, two build-it bits); class startup scripts (SCRIPT-6) may re-enable them. None collides. evidence: [scripts.md §2026-10-13T20:42](../../re/scripts.md#ts-20261013-204236).
- [ ] SCRIPT-3 — The two detective X-spots (`1-2_Dtv_DogPath_XSpot`, `1-3_Dtv_BrickSpot_XSpot_02`) are not explained (no grid entry). evidence: [collision.md §2026-10-13T20:43](../../re/collision.md#ts-20261013-204340).
- [ ] SCRIPT-4 — The code that hides the revealer targets; the role of the other type-4 parameters (Destination, ObjectTo, TriggerObject, ...); the revealer rule is a data fit and only lvl_01 was compared with the oracle (lvl_00 not). evidence: [scripts.md §2026-10-13T20:42](../../re/scripts.md#ts-20261013-204236).
- [ ] SCRIPT-5 — Variables of type 5 and 7 in conditions are converted to floats; the executor compares floats. evidence: [scripts.md §2026-10-13T20:42](../../re/scripts.md#ts-20261013-204236).
- [ ] SCRIPT-6 — Other thread starters: the `startup` scripts of placed objects whose class file has scripts (64 class files; owners of scene+0x24 from last to first, order against `Level::objects` unknown), the events `OnSceneEnter%d` / `OnSceneExit` (0x1ecc50) and `ScriptCheckExtras` (0x2e5638). blocks: SCRIPT-1. evidence: [scripts.md §2026-10-14T09:52](../../re/scripts.md#ts-20261014-095252).
- [ ] SCRIPT-7 — Natives: 20 of 245 are implemented, 1 is a stand-in; census, handler addresses, classes A-D and the batches are in the table. needs: SCRIPT-26 to SCRIPT-39. blocks: SCRIPT-1, DOOR-1 (doors.md). evidence: [script-natives.md §2026-10-14T13:33](../../re/script-natives.md#ts-20261014-133339).
- [x] SCRIPT-8 — Closed 2026-10-14: a live thread (`Police_ExtToInt`) was traced from an unmodified state (creation, first pass, blocking in `WarningDialogBox`, the Wait 3 timer); `ScriptRunner` matches, pinned in `script-runner`. evidence: [scripts.md §2026-10-14T11:13](../../re/scripts.md#ts-20261014-111312).
- [ ] SCRIPT-9 — Script argument slots (thread +0x18..) and how a called script reads them; the thread pool size and the reuse of a running thread when the pool is empty (same script or `EffectScript`). evidence: [scripts.md §2026-10-14T09:52](../../re/scripts.md#ts-20261014-095252).
- [ ] SCRIPT-10 — After the startup run the original has triggers 79 to 82 (`1-4_Run_Point_02..05`) and 141 (`DEBUG_Head_Robber_Chase_End`) and the bounds `CameraBound_Police_Helipad`, `CameraBound_VehicleDrop_01/02` switched off; the port leaves them on (class scripts, SCRIPT-6, or the record builder). evidence: [scripts.md §2026-10-14T10:06](../../re/scripts.md#ts-20261014-100640).
- [ ] SCRIPT-11 — Trigger types without a check (object bound events, BoundInside / Outside, AIMove, AISpawned, Character*, PlayerVehicle*, Reset, ...): `chase-game` lists the ones a level uses and they never fire; triggers of class files and the master scene are not modelled. blocks: SCRIPT-1. evidence: [scripts.md §2026-10-14T10:06](../../re/scripts.md#ts-20261014-100640).
- [ ] SCRIPT-12 — `PositionPlayer` (0x1da0dc) not read; `PositionObject` moves only the player (other objects are reported as not applied) and sets position and heading only: the cleared controller fields, the player message 8 and the character-type steps (0x1d9e6c) are not replicated. evidence: [scripts.md §2026-10-14T10:54](../../re/scripts.md#ts-20261014-105432).
- [ ] SCRIPT-13 — Mode 4 (the dialog pushed by `0x25d26c`): its drawing, input, open / close code; why `0x1a142c` refuses it; the FadeScreen tween 0x256a14 and end test 0x25207c. blocks: SCRIPT-S9, SCRIPT-S10. evidence: [scripts.md §2026-10-14T10:54](../../re/scripts.md#ts-20261014-105432).
- [ ] SCRIPT-26 — Batch 1 (A, 22): object flags and script bookkeeping, for example ToggleUseable, EnableDistanceFade, SetIndestructible, EnableTargetable, KillScript, PositionPlayer, PositionBound, RevealCollectable, ResetObject, SkipToFinalState. blocks: SCRIPT-7. evidence: [script-natives.md §2026-10-14T13:33](../../re/script-natives.md#ts-20261014-133339).
- [ ] SCRIPT-27 — Batch 2 (A, 21): progress, save and stud getters / setters, for example UnlockCharacter, UnlockAbility, GiveReward, AddStuds, QuickSave, CountRacesWon, EnableStudsByGroup. blocks: SCRIPT-7. evidence: [script-natives.md §2026-10-14T13:33](../../re/script-natives.md#ts-20261014-133339).
- [ ] SCRIPT-28 — Batch 3 (C and B, 13): DCam core and camera: StartDCam, StartDCamTrack, DCamActive, WaitForDCam (blocks), EndDCam, SnapCamera, ShakeScreen, Set3DFocusObject. needs: SCRIPT-40. blocks: SCRIPT-7. evidence: [script-natives.md §2026-10-14T13:33](../../re/script-natives.md#ts-20261014-133339).
- [ ] SCRIPT-29 — Batch 4 (C and B, 13): DCam cutscenes (DCamCutScene*, PlayCutScene, PlayCredits, ConversationPlayAndWait), BulletTime, fog and background overrides. needs: SCRIPT-40, MOVIE-2. blocks: SCRIPT-7. evidence: [script-natives.md §2026-10-14T13:33](../../re/script-natives.md#ts-20261014-133339).
- [ ] SCRIPT-30 — Batch 5 (C, 16): ShowDialogueText(Timed), ShowDialogBG, ShowTipText*, ShowIntroText, SetMissionTitle / Summary, ShowTally, ShowCharacterIntro. needs: SCRIPT-41. blocks: SCRIPT-7. evidence: [script-natives.md §2026-10-14T13:33](../../re/script-natives.md#ts-20261014-133339).
- [ ] SCRIPT-31 — Batch 6 (C, 15): SetActiveWaypoint, SubObjective* arrows and enables, map dummy objects, PanMapToObject, scanner natives. needs: SCRIPT-41, HUD-11. blocks: SCRIPT-7. evidence: [script-natives.md §2026-10-14T13:33](../../re/script-natives.md#ts-20261014-133339).
- [ ] SCRIPT-32 — Batch 7 (C and B, 13): PlayCharAnim, PlayAnimation, LoadAnimation, WaitForCurrentAnim / CharAnim (block), StopAnim, Attach* natives. needs: ANIM-2. blocks: SCRIPT-7. evidence: [script-natives.md §2026-10-14T13:33](../../re/script-natives.md#ts-20261014-133339).
- [ ] SCRIPT-33 — Batch 8 (C, 13): PlaySound(GO), StopSound, RegisterSound, OverrideMusic, AutomaticMusic, StopMusic, vehicle sound natives. needs: AUDIO-2. blocks: SCRIPT-7. evidence: [script-natives.md §2026-10-14T13:33](../../re/script-natives.md#ts-20261014-133339).
- [ ] SCRIPT-34 — Batch 9 (C and B, 17): AI control: AIEnable, AIActive, AIMove, AIFaceTarget, AIIgnorePlayer, AIPursuit, AIFollowSimplePath and the other AI* natives. needs: SCRIPT-42. blocks: SCRIPT-7. evidence: [script-natives.md §2026-10-14T13:33](../../re/script-natives.md#ts-20261014-133339).
- [ ] SCRIPT-35 — Batch 10 (C, 18): SetPartyCharacter, SuperFreePlayswap, SwapAllegiance, SetWeaponEnabled, CharSet*, mech and pet natives, boss hooks (Rex, Walnuts, 1-8). needs: SCRIPT-42. blocks: SCRIPT-7. evidence: [script-natives.md §2026-10-14T13:33](../../re/script-natives.md#ts-20261014-133339).
- [ ] SCRIPT-36 — Batch 11 (C, 19): Vehicle* natives, StopPlayerVehicle, traffic and pedestrian enables. needs: SCRIPT-43. blocks: SCRIPT-7. evidence: [script-natives.md §2026-10-14T13:33](../../re/script-natives.md#ts-20261014-133339).
- [ ] SCRIPT-37 — Batch 12 (C and B, 14): MovePlatform, SetPlatform*, WaitForPlatform (blocks), Rideable* natives. needs: SCRIPT-44. blocks: SCRIPT-7. evidence: [script-natives.md §2026-10-14T13:33](../../re/script-natives.md#ts-20261014-133339).
- [ ] SCRIPT-38 — Batch 13 (C, 20): SpawnDebris, PlayParticlesGO, Effect* natives, EffectFadeObject, AddToSemiTransList*, StreamRoomIn, RoomStreamingWait. needs: PART-1. blocks: SCRIPT-7. evidence: [script-natives.md §2026-10-14T13:33](../../re/script-natives.md#ts-20261014-133339).
- [ ] SCRIPT-39 — Batch 14 (C, 10): StartMiniGame, HubShop, RedBrickShop, CharacterShop, VehicleShop, HubCustomisation, StudMagnetScene, AwardViewer. needs: SCRIPT-45. blocks: SCRIPT-7. evidence: [script-natives.md §2026-10-14T13:33](../../re/script-natives.md#ts-20261014-133339).
- [ ] SCRIPT-40 — No director camera (DCam): path and track playback, the busy test of WaitForDCam, cutscene load / play / free, snap, shake and focus. blocks: SCRIPT-28, SCRIPT-29. evidence: [script-natives.md §2026-10-14T13:33](../../re/script-natives.md#ts-20261014-133339).
- [ ] SCRIPT-41 — No on-screen text and objective HUD: dialogue lines, tip text, mission title and summary, waypoints and objective arrows. blocks: SCRIPT-30, SCRIPT-31. evidence: [script-natives.md §2026-10-14T13:33](../../re/script-natives.md#ts-20261014-133339).
- [ ] SCRIPT-42 — No AI and character control: AI states, move, face, pursue, weapons, allegiance, party swap. blocks: SCRIPT-34, SCRIPT-35. evidence: [script-natives.md §2026-10-14T13:33](../../re/script-natives.md#ts-20261014-133339).
- [ ] SCRIPT-43 — No vehicles and traffic (see also COLL-2, LEVOBJ-7). blocks: SCRIPT-36. evidence: [script-natives.md §2026-10-14T13:33](../../re/script-natives.md#ts-20261014-133339).
- [ ] SCRIPT-44 — No moving platforms and rideables. blocks: SCRIPT-37. evidence: [script-natives.md §2026-10-14T13:33](../../re/script-natives.md#ts-20261014-133339).
- [ ] SCRIPT-45 — No hub, shop and minigame pages. blocks: SCRIPT-39. evidence: [script-natives.md §2026-10-14T13:33](../../re/script-natives.md#ts-20261014-133339).

## 6. Findings summary

- Script record layout, argument strings, function-name hash lookup: [scripts.md §2026-10-13T20:42](../../re/scripts.md#ts-20261013-204236).
- The `startup` script plus build-it / reveal links give the disabled objects (replaces the 85-name list): [scripts.md §2026-10-13T20:42](../../re/scripts.md#ts-20261013-204236), [collision.md §2026-10-13T20:43](../../re/collision.md#ts-20261013-204340).
- Runner 0x169254, thread record, executor, Wait, CallScript / SpawnScript, thread starters, trigger queue and check functions, TriggerObject, native table, the police door script: [scripts.md §2026-10-14T09:52](../../re/scripts.md#ts-20261014-095252).
- Trigger and bound records, EnableTrigger / EnableBound lookups, the bound test 0x22f650, the room switch +0xb, oracle comparison, why the police entrance did not fire: [scripts.md §2026-10-14T10:06](../../re/scripts.md#ts-20261014-100640).
- WarningDialogBox (text keys, results A 2 / B 1, flow), PositionObject, FadeScreen, the user observation of the loading-screen prompt: [scripts.md §2026-10-14T10:54](../../re/scripts.md#ts-20261014-105432).
- Census of all natives (handlers, classes A-D, batches SCRIPT-26 to SCRIPT-39): [script-natives.md §2026-10-14T13:33](../../re/script-natives.md#ts-20261014-133339).
- Door open requests arrive as message 0xff from `TriggerObject` / the platform link: [objects.md §2026-10-13T21:17](../../re/objects.md#ts-20261013-211741).

## 7. Code and tests

- `Level::scripts` / `triggers` / `bounds` / `boundMatrices` / `objectReferenceHash` (`src/assets/level.cpp`), `ScriptRunner` (`src/runtime/script_runner.cpp`), `levelInitialState` / `runLevelStartup` / `levelStartState` (`src/runtime/level_start.cpp`), `PlacedObjectState` (`src/assets/object_collision.*`), the runner hook and the door messages in `src/game.cpp`.
- `ScriptRunner` dialog / placement / fade API, `objectStart` (`src/assets/level.cpp`), `drawWarningDialog` / `drawBlackVeil` (`src/frontend/pages.cpp`), `--call-script` (development option) in `src/game.cpp`.
- CTest: `script-runner`, `script-dialog-real-data`, `game-script-dialog`, `level-start-real-data`, `collision-real-data`, `progress-tables`.
