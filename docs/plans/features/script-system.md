# Script system

## 1. Scope

The level scripts (level section +0x18), the per-update runner 0x169254, the executor 0x234204 and its words, Wait, CallScript / SpawnScript, the `startup` script, the class `startup` scripts of placed objects, the camera bound and trigger start switches, the `OnSceneEnter<room>` event, the revealer links and the start state of placed objects. Class scripts and events at run time and the `ScriptCheckExtras` request source are not covered. Mission flow and enemies are not covered ([README.md](README.md) lists them as not started). The trigger pass is in [script-system-triggers.md](script-system-triggers.md), the natives in [script-system-natives.md](script-system-natives.md).

## 2. Status

IN_PROGRESS, updated 2026-10-14T16:48:13+02:00. `ScriptRunner` runs threads every update like the original (waits, CallScript, SpawnScript). The director camera runs: StartDCam, StartDCamTrack, WaitForDCam and EndDCam follow the oracle timing of the police entry script. The scene start (`SceneStart`) runs class `startup` scripts, the level `startup` and `OnSceneEnter<room>`, and applies the camera bound and trigger start rules; player bound triggers and `TriggerObject` start scripts. The trigger pass and the natives have their own files (see Scope).

## 3. Verified behaviour

- Script data layout and loader 0x1cfc50; function names re-derived by hash (0x208a3c). lvl_01 has 314 scripts.
- Over the 896 lvl_01 objects that would register collision, the state rule gives 819 enabled in both, 48 flag-0 in both, 4 without entity and disabled by the script, 12 flag-0 only through the revealer links, 8 co-located twins; no object that the rule disables while its entity is enabled. `level-start-real-data` pins this against the oracle grid read.
- The runner is entered once per update per resident scene from the gameplay update (oracle trap, lr 0x2e54b8), and no thread is alive at gameplay-start or police-station; `level-start-real-data` checks that the lvl_01 startup threads all end.
- Executor, Wait, CallScript / SpawnScript and thread creation rules were transcribed from the disassembly; `script-runner` covers them with synthetic scripts.
- After the startup run, 236 of 241 trigger enable bytes and 479 of 482 bound enable bytes equal the oracle read at gameplay-start, and the bound room rule gives exactly the oracle's 257 active bounds for its 12 loaded rooms; `level-start-real-data` pins all three.
- After the scene start, all 241 trigger enable bytes and 482 bound enable bytes of lvl_01 equal the oracle read at gameplay-start; for lvl_00 the 53 trigger bytes and 137 bound bytes equal the oracle read at police-station (`OnSceneEnter1` switches bounds 13 and 14). `level-start-real-data`, `scene-start`. [scene-start.md §2026-10-14T14:27](../../re/scene-start.md#ts-20261014-142739).

## 4. Stand-ins

- SCRIPT-S1 The startup threads are stepped before the first frame with dt 0.034 (the free-running dt read at gameplay-start; the loading phase's own updates are not recovered): `chase-game` until `startup` ends or starts a movie or an in-engine cutscene (the game loop then continues it, as the original runs the thread during the load), `levelStartState` until no thread is left.
- SCRIPT-S2 Closed 2026-10-14: `IsLevelUnlocked`, `GetGameComplete` and `IsRedBrickExtraActive` read the loaded slot ([save-format-state.md §2026-10-14T11:53](../../re/save-format-state.md#ts-20261014-115300)); without `--save` they return the new-game values (level 2 unlocked, rest 0). The mask of `IsRedBrickExtraActive` is a runtime value that is 0 after a load.
- SCRIPT-S3 The revealer rule is a data fit, not traced to its code; verified only in lvl_01.
- SCRIPT-S4 A function without a handler does nothing and continues at once; `chase-game` prints its name the first time (`script function without handler`). The original may block in it (WaitForDCam, DCamCutSceneWait, AIMove, ...). See SCRIPT-7.
- SCRIPT-S5 Closed 2026-10-19: the original faults on a null script in 0x1c3730 and all 894 calls of the main levels resolve; `ScriptRunner` stops with an error. evidence: [scripts.md §2026-10-19T15:12:09](../../re/scripts.md#ts-20261019-151209).
- SCRIPT-S275 The bits 0x14 of ctrl+0x315 cleared by a placement are recorded in `VerticalState::placement` and read by no locomotion code. needs: PLAYER-25. evidence: [script-natives-flags.md §2026-10-19T15:12:32](../../re/script-natives-flags.md#ts-20261019-151232).
- SCRIPT-S6 `chase-game` runs the scripts and the trigger pass after the door update; where 0x2e5158 runs them relative to the player and camera was not compared.
- SCRIPT-S7 For the bound switch +0xb `chase-game` counts the rooms of the room streamer as loaded (street level, since WSTREAM-3). The startup scripts before the first frame and levels without streaming (WSTREAM-S1) still count every room as loaded.
- SCRIPT-S14 Class `startup` scripts run for every placed object of the level whose class has one (the original: entities of loaded rooms) in a private `ScriptRunner` with a private object state; sub-objects of the class are not instantiated and an empty object name is not merged into the owner. needs: LEVOBJ-8, SCRIPT-52.
- SCRIPT-S16 The start order class startups, level `startup`, camera bound switches, `OnSceneEnter<room>` is taken from the call sites of the load code, not traced in one run; the start room is the room of `Player1Start` or of the scene change destination. needs: SCRIPT-53.

## 5. Open items

- [ ] SCRIPT-1 — Scripts run during play and player bound triggers start them, but most natives have no handler and most trigger types start nothing: no mission flow. needs: SCRIPT-7, SCRIPT-75, SCRIPT-76, SCRIPT-110, SCRIPT-111, SCRIPT-113, LEVOBJ-14, LEVOBJ-15. blocks: LEVOBJ-3, STUD-4, HUD-9, TRANS-3, TRANS-5, WSTREAM-6. evidence: [scripts.md §2026-10-14T09:52](../../re/scripts.md#ts-20261014-095252).
- [ ] SCRIPT-2 — `levelStartState` disables some objects the original has enabled (hydrant sprays, tokens, task and archway pads, build-it bits). Class startups do not explain it (their classes have no scripts); cause not found. evidence: [scene-start.md §2026-10-14T14:27](../../re/scene-start.md#ts-20261014-142739), [scripts.md §2026-10-13T20:42](../../re/scripts.md#ts-20261013-204236).
- [ ] SCRIPT-3 — The two detective X-spots (`1-2_Dtv_DogPath_XSpot`, `1-3_Dtv_BrickSpot_XSpot_02`) are not explained (no grid entry). evidence: [collision.md §2026-10-13T20:43](../../re/collision.md#ts-20261013-204340).
- [ ] SCRIPT-4 — The code that hides the revealer targets; the role of the other type-4 parameters (Destination, ObjectTo, TriggerObject, ...); the revealer rule is a data fit and only lvl_01 was compared with the oracle (lvl_00 not). evidence: [scripts.md §2026-10-13T20:42](../../re/scripts.md#ts-20261013-204236).
- [ ] SCRIPT-5 — Variables of type 5 and 7 in conditions are converted to floats; the executor compares floats. evidence: [scripts.md §2026-10-13T20:42](../../re/scripts.md#ts-20261013-204236).
- [ ] SCRIPT-9 — Script argument slots (thread +0x18..) and how a called script reads them; the thread pool size and the reuse of a running thread when the pool is empty (same script or `EffectScript`). evidence: [scripts.md §2026-10-14T09:52](../../re/scripts.md#ts-20261014-095252).
- [ ] SCRIPT-52 — Class owners: an empty object name as the owner itself, the variable `TaskComplete.TASK_COMPLETE` (saved per object?), private bounds and triggers of class instances (EnableBound / EnableTrigger inside classes), the effect of class startups after a save. needs: LEVOBJ-8. blocks: SCRIPT-S14. evidence: [scene-start.md §2026-10-14T14:27](../../re/scene-start.md#ts-20261014-142739).
- [ ] SCRIPT-53 — Order at the initial load: whether 0x2f67fc (class and level startup) precedes 0x2e6264 (camera bound init, `OnSceneEnter`); the second script 0x2f6870 starts by a stored name; which rooms' entities get class startups. blocks: SCRIPT-S14, SCRIPT-S16. evidence: [scene-start.md §2026-10-14T14:27](../../re/scene-start.md#ts-20261014-142739).
- [ ] SCRIPT-54 — `ScriptCheckExtras` has no source: the Extras screen (flag 0x5163e8+0x35 set by 0x290310 when it is left) is not in the port, and its race natives have no handlers. evidence: [scene-start.md §2026-10-14T14:27](../../re/scene-start.md#ts-20261014-142739).
- [ ] SCRIPT-55 — Camera bound system update 0x2ff7c8 (when a switched-off bound comes on again) and `CameraBoundArrive` (0x3345f8) are not read; the bounds stay off. evidence: [scene-start.md §2026-10-14T14:27](../../re/scene-start.md#ts-20261014-142739).
- [ ] SCRIPT-337 — No game test runs PositionPlayer with a seated player (the vehicle arrival scripts such as `CTH-DT_Vehicle_Transition` do) or reads the entity at +0x84 of a non-vehicle object. evidence: [script-natives-flags.md §2026-10-19T15:12:32](../../re/script-natives-flags.md#ts-20261019-151232).

## 6. Findings summary

- Script record layout, argument strings, function-name hash lookup: [scripts.md §2026-10-13T20:42](../../re/scripts.md#ts-20261013-204236).
- The `startup` script plus build-it / reveal links give the disabled objects (replaces the 85-name list): [scripts.md §2026-10-13T20:42](../../re/scripts.md#ts-20261013-204236), [collision.md §2026-10-13T20:43](../../re/collision.md#ts-20261013-204340).
- Runner 0x169254, thread record, executor, Wait, CallScript / SpawnScript, thread starters, trigger queue and check functions, TriggerObject, native table, the police door script: [scripts.md §2026-10-14T09:52](../../re/scripts.md#ts-20261014-095252).
- Class `startup` scripts, camera bound switches, trigger start rule, `OnSceneEnter<room>` and `ScriptCheckExtras`: [scene-start.md §2026-10-14T14:27](../../re/scene-start.md#ts-20261014-142739).
- Door open requests arrive as message 0xff from `TriggerObject` / the platform link: [objects.md §2026-10-13T21:17](../../re/objects.md#ts-20261013-211741).

## 7. Code and tests

- `Level::scripts` / `triggers` / `bounds` / `boundMatrices` / `objectReferenceHash` (`src/assets/level.cpp`), `ScriptRunner` (`src/runtime/script_runner.cpp`), `levelInitialState` / `runLevelStartup` / `levelStartState` (`src/runtime/level_start.cpp`), `PlacedObjectState` (`src/assets/object_collision.*`), the runner hook and the door messages in `src/game_level_update.cpp`, level setup in `src/game_level.cpp`.
- `SceneStart`, `cameraBoundsSwitchedOffAtStart`, `triggersWithoutObject` (`src/runtime/scene_start.*`); `levelStartState` and the session start in `src/game_level.cpp` call it.
- Placement effects (`src/runtime/placement_message.*`, test in `script-runner`), the missing-script stop (`script-runner`, `script-call-targets-real-data`).
