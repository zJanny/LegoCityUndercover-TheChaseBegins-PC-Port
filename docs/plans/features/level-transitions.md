# Level transitions

## 1. Scope

Leaving a street for a building and travelling between world levels: `SceneChange` script native, `scenechange` objects, the loading page, the destination placement. Room loading is in [world-streaming.md](world-streaming.md). The police station (lvl_00) and the city (lvl_01) are separate level files in the port.

## 2. Status

IN_PROGRESS, updated 2026-10-14T12:05:00+02:00. `chase-game` switches between the police station and the street at run time in both directions: the lobby exit (walk-in bound of a master `scenechange` object) and the entry (script `Police_ExtToInt`, `SceneChange` native) show the loading page, wait for A and place the player at the destination point of the original (positions and headings from oracle traces). The master level is not loaded as a scene (TRANS-3), the loading page is a stand-in, and the lifts inside the station and the other world levels are not done.

## 3. Verified behaviour

- `SceneChange <bound>` (handler 0x1b262c) finds a `scenechange` object (type 78) by bound hash and sends it message 0xff; manager slot +0x2c (0x3037e8) stores the request (`[[entity+0x7c]+0x14]` at +0x28) and pushes the loading page (mode 6); after the A press the execution 0x303480 unloads and reloads the world when the destination world level is not loaded and only moves the player when it is. evidence: [streaming.md §2026-10-13T22:27](../../re/streaming.md#ts-20261013-222700), [§2026-10-14T11:13](../../re/streaming.md#ts-20261014-111312).
- Entry order in the original (unmodified state): trigger `PlayerOnFootBoundEntered` on `Police_Station_Transition_Lower`, `WarningDialogBox`, 3 s wait, `SceneChange`, loading page, second A, switch 4 updates later; the dialog answer does not request the switch. The master level lists 35 `scenechange` objects with qualified bounds (`WorldLevel_01_CherryTreeHills.Police_ExtToInt`).
- Exit in the original: no script. The master object `SceneChange_LobbyToCherryTree` fires from its own update 0x1c47e8 when the player is inside its enabled bound `Lobby_to_CherryTree` of lvl_00 (update 13 of the traced walk, z 20.156 inside, 19.673 outside), through manager slot +0x2c when the destination world level is not loaded (slot +0x28, 0x3038a0, without loading page, when it is). evidence: [streaming.md §2026-10-14T12:02](../../re/streaming.md#ts-20261014-120258).
- Destination: the `SceneChangeTo` parameter (hash d2fb9421) is a 20-byte reference {world level hash, point hash, pointer, flags, scene index byte +0xd}; the player lands exactly on the `point` object, heading = its yaw, no world level offset: lobby to street (902.00848, 16.57730, 1111.12634) yaw 249.25; street to lobby (-1.11520, 2.09010, 15.70230) yaw 152.32, then it falls to y 1.43053. The second destination parameter (hash 0xccaaccdd) is not read. `scene-change-real-data`, `game-scene-change-exit`, `game-scene-change-entry`.
- Loading page (oracle screenshot): key art on the top screen, black bottom screen with a blue line, badge and `Continue (A)` bottom right; the same in both directions. The A wait is in the generic menu widget (addresses in the streaming entry).
- Event bindings (bound entered and others to scripts) are decoded and pinned (`streaming-real-data`).

## 4. Stand-ins

- TRANS-S1 The port runs one level file at a time and rebuilds the level at each switch (`chase-game`), not the master level `lvl_the_city` with resident world levels; the state of the left level is lost, and `--call-script` runs again in the new level when it has the script (stands in for the saved game, SAVE-1). needs: TRANS-3.
- TRANS-S2 `SceneChange <bound>` is qualified with the calling level's world level instance name (data fit, every shipped call matches; 0x26344c not read). needs: TRANS-10.
- TRANS-S3 The loading page is drawn from one oracle screenshot (`drawSceneChangeBottom`); the top screen shows the generic1 background instead of the key art, the badge has no ring, the text key `Continue` (systemtext 0xe3f70996) is inferred from the text. needs: TRANS-8.
- TRANS-S4 The page accepts A from its first update, and nothing runs between the A press and the switch, which comes 4 updates after the press (trap measurement of the execution 0x303480). needs: TRANS-8.
- TRANS-S5 The follow camera is reset behind the landing heading after a switch; the original's pose was not compared. needs: TRANS-12.

## 5. Open items

- [x] TRANS-1 — closed 2026-10-14: cause of the silent bound and the trap chain recorded. Remaining question moved to TRANS-7.
- [x] TRANS-7 — closed 2026-10-14: the `SceneChange` native requests the switch through the scenechange entity (slot +0x2c); `0x303480` runs after the loading-page A press. evidence: [streaming.md §2026-10-14T11:13](../../re/streaming.md#ts-20261014-111312).
- [x] TRANS-4 — closed 2026-10-14: trigger dispatch runs in `chase-game` (`ScriptRunner::checkTriggers`, 0x1746c0). evidence: [scripts.md §2026-10-14T10:06](../../re/scripts.md#ts-20261014-100640).
- [x] TRANS-2 — closed 2026-10-14: destination semantics and both landing positions pinned (see Verified behaviour). evidence: [streaming.md §2026-10-14T12:02](../../re/streaming.md#ts-20261014-120258). The qualification rule moved to TRANS-10.
- [x] TRANS-9 — closed 2026-10-14: the scenechange update 0x1c47e8 is the walk-in path (enabled bound plus player inside, then slot +0x28 or +0x2c); the lobby exit uses it. evidence: [streaming.md §2026-10-14T12:02](../../re/streaming.md#ts-20261014-120258).
- [ ] TRANS-3 — Master level in `chase-game`: load `lvl_the_city`, world levels as scenes (resident set, unload and reload as 0x303480), switch with fade and the generic loading screen. needs: SCRIPT-1, WSTREAM-3, FE-3.
- [ ] TRANS-5 — Natives needed by the transition scripts: `WarningDialogBox`, `PositionObject` (player), `SceneChange` and `FadeScreen` (stand-in) run in `chase-game` (SCRIPT-7); still without handler: `PositionPlayer`, `SnapCamera`, `AIMove`, `StartDCam`, `WaitForDCam`, `AIEnable`, `DCamActive`, `CharSetAllowVehicleEntry`, `RelockChasePlainClothes`. evidence: [scripts.md §2026-10-14T10:54](../../re/scripts.md#ts-20261014-105432). needs: SCRIPT-1.
- [ ] TRANS-8 — Loading page code: which input read gives the A edge, whether A counts before the load finishes, what the badge and line do while loading, the text key and the source of the top key art. The page object is vtable 0x361fb4 (enter 0x2e1324), the A wait is in the generic menu widget (0x2a8540 state table). evidence: [streaming.md §2026-10-14T12:02](../../re/streaming.md#ts-20261014-120258). blocks: TRANS-S3, TRANS-S4. needs: FE-3.
- [ ] TRANS-6 — Building entries inside a level (lift in lvl_00 between room1/3/5, `Lobby2Basement_SceneChange` objects with unqualified references in the same level, enabled by `EnableBound`); these resolve in the loaded level, so they take slot +0x28 (TRANS-11).
- [ ] TRANS-10 — How 0x26344c qualifies the script argument with the world level name, and what the second destination parameter (hash 0xccaaccdd) is for. blocks: TRANS-S2.
- [ ] TRANS-11 — What manager slot +0x28 (0x3038a0, taken when the destination is already loaded) does after storing the request and who runs 0x303480 for it. blocks: TRANS-6.
- [ ] TRANS-12 — The drift of the player after landing in the lobby (x -1.1152 to -1.1395 and z 15.7023 to 15.7487 within 12 updates after the fall, player state 4) and the camera pose after a switch. blocks: TRANS-S5.

## 6. Findings summary

- [streaming.md §2026-10-14T12:02](../../re/streaming.md#ts-20261014-120258): lobby exit as walk-in, scenechange update 0x1c47e8, destination reference record, landing positions, loading page (enter, widget, screenshot measurements).
- [streaming.md §2026-10-13T22:27](../../re/streaming.md#ts-20261013-222700) (level switch chain, event bindings), [§2026-10-14T11:13](../../re/streaming.md#ts-20261014-111312) (entry order, loading page, room states), [§2026-10-14T10:07](../../re/streaming.md#ts-20261014-100715) (bound enable byte, poked switch).
- [scripts.md §2026-10-14T10:54](../../re/scripts.md#ts-20261014-105432) (WarningDialogBox results and look, user observation of the loading screen).

## 7. Code and tests

- `Level::triggers`, `LevelTrigger` (`src/assets/level.cpp`); `src/runtime/scene_change.{hpp,cpp}` (master `scenechange` objects, destination points, `SceneChangeFlow`); `ScriptRunner::takeSceneChanges` / `playerInNamedBound`; `drawSceneChangeBottom` (`src/frontend/pages.cpp`); the per-update scene change flow in `src/game_level_update.cpp`, the switch loop in `src/game.cpp`, `advanceToDestination` in `src/game_level.cpp`.
- CTest `streaming-real-data`, `scene-change-real-data`, `game-scene-change-exit`, `game-scene-change-entry`, `script-runner` (`sceneChangeIsRecorded`).
