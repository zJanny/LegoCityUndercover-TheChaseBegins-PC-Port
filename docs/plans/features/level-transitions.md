# Level transitions

## 1. Scope

Leaving a street for a building and travelling between world levels: `SceneChange` script native, `scenechange` objects, the loading screen, the destination placement. Room loading is in [world-streaming.md](world-streaming.md). The police station (lvl_00) and the city (lvl_01) are separate levels in the port.

## 2. Status

IN_PROGRESS, updated 2026-10-14T11:13:12+02:00. The chain from the script native to the scene change execution is mapped and its request order is proven from an unmodified oracle state (trigger, dialog, `SceneChange`, request, loading page, second A, switch); event bindings (`Level::triggers`) are decoded. Nothing runs in `chase-game`. The chain from the script native to the scene change execution is mapped; event bindings (`Level::triggers`) are decoded; one switch (street to lvl_00) was reproduced in the oracle with a poked enable byte. Nothing runs in `chase-game`.

## 3. Verified behaviour

- `SceneChange <bound>` (handler 0x1b262c) finds a `scenechange` object (type 78) by bound hash and sends it message 0xff; execution 0x303480 unloads and reloads the world when the destination world level is not loaded (progress 25/50/100) and only moves the player when it is. evidence: [streaming.md §2026-10-13T22:27](../../re/streaming.md#ts-20261013-222700).
- 24 `SceneChange` calls exist in shipped level scripts (city to Auburn, Downtown, Albatross, police entry and helipad entry, bluebell and fort meadows exits). Entering the police station from the street runs script `Police_ExtToInt` (dialog, camera, doors, `SceneChange`).
- The master level lists 35 `scenechange` objects with qualified bounds (`WorldLevel_01_CherryTreeHills.Police_ExtToInt`).
- The `Police_Station_Transition_Lower` bound was disabled at gameplay-start (enable byte +0xa of its 0x28-byte record; `1-1_MissionHide` clears it, `1-1_MissionEnd` and `1-1_MissionSkip` set it). With it enabled and the player inside, script `Police_ExtToInt` opens `WarningDialogBox` 34 updates later; A answers 2; the switch then runs `0x303480` -> `0x2e3cc4` -> `0x2e3a9c` -> `0x223878` -> `0x21ac70` in one update (poked states, measuring aid). evidence: [streaming.md §2026-10-14T10:07](../../re/streaming.md#ts-20261014-100715).
- From the unmodified state `street-at-entrance` (after mission 1-1) walking into `Police_Station_Transition_Lower` starts `Police_ExtToInt` by itself (PlayerOnFootBoundEntered trigger). Order, stable in three full runs: trigger fires and the thread is queued (update 16); `WarningDialogBox` blocks (17) and the scene threads stop while the dialog is open; after the answer the script waits 3 s, then `SceneChange` (59 updates after the answer) sends message 0xff to the scenechange entity, whose handler calls manager slot +0x2c (0x3037e8): it stores the destination and opens the loading page; a second A on the loading page starts the switch `0x303480` 4 updates later. The dialog answer and the scenechange update 0x1c47e8 do not request the switch. evidence: [streaming.md §2026-10-14T11:13](../../re/streaming.md#ts-20261014-111312).
- Event bindings (bound entered and others to scripts) are decoded and pinned (`streaming-real-data`).

## 4. Stand-ins

- TRANS-S1 The port treats lvl_00 and lvl_01 as separate levels chosen on the command line; there is no switch at run time.

## 5. Open items

- [x] TRANS-1 — closed 2026-10-14: cause of the silent bound and the trap chain recorded (see Verified behaviour). Remaining question moved to TRANS-7.
- [x] TRANS-7 — Closed 2026-10-14: the `SceneChange` native requests the switch through the scenechange entity (slot +0x2c 0x3037e8); `0x303480` runs after the loading-page A press. Order stable in three full runs from an unmodified state. evidence: [streaming.md §2026-10-14T11:13](../../re/streaming.md#ts-20261014-111312). blocks: none.
- [ ] TRANS-2 — Destination semantics: the scenechange object's `initialposition`, `SceneChangeTo`, hash 0xccaaccdd and the scene index byte (+0xd of the destination entity); how the bound argument is qualified with the world level name; whether world level offsets of the master are applied (the request stores the destination `[[entity+0x7c]+0x14]` at manager +0x28; what it is was not read).
- [ ] TRANS-3 — Master level in `chase-game`: load `lvl_the_city`, world levels as scenes, switch with fade and the generic loading screen. needs: SCRIPT-1, WSTREAM-3, FE-3.
- [x] TRANS-4 — Closed 2026-10-14: trigger dispatch runs in `chase-game` (`ScriptRunner::checkTriggers`, 0x1746c0): BoundEntered, Player1BoundEntered, PlayerOnFootBoundEntered and BoundLeft with the bounds of section +0x24, ObjectTriggered / ObjectUntriggered from `TriggerObject`; the other trigger types are SCRIPT-11 ([script-system.md](script-system.md)). evidence: [scripts.md §2026-10-14T10:06](../../re/scripts.md#ts-20261014-100640).
- [ ] TRANS-5 — Natives needed by the transition scripts: `WarningDialogBox`, `PositionObject` (player) and `FadeScreen` (stand-in) run in `chase-game` (SCRIPT-7); still without handler: `PositionPlayer`, `SnapCamera`, `AIMove`, `StartDCam`, `WaitForDCam`, `AIEnable`, `DCamActive`, `CharSetAllowVehicleEntry`, `RelockChasePlainClothes`, `SceneChange`. evidence: [scripts.md §2026-10-14T10:54](../../re/scripts.md#ts-20261014-105432). needs: SCRIPT-1.
- [ ] TRANS-8 — The level switch waits for a second A on the loading screen (a blank screen with a "press A" prompt bottom right; leaving the station shows only that prompt). User observation in the real game, confirmed in the oracle: the loading page (scheduler object vtable 0x00361fb4) waits and 0x303480 follows 4 updates after the A press; the page code that tests A is not read (UI widget code around 0x2a85a0 / 0x2a8658 / 0x2c34b4). evidence: [streaming.md §2026-10-14T11:13](../../re/streaming.md#ts-20261014-111312). needs: FE-3, TRANS-3. evidence: [scripts.md §2026-10-14T10:54](../../re/scripts.md#ts-20261014-105432).
- [ ] TRANS-9 — Whether the scenechange update 0x1c47e8 and manager slot +0x28 (0x3038a0) are ever used (a walk-in without a script); neither fired while the player stood in the bound or in the full `Police_ExtToInt` sequence. Read the 0x1c47e8 conditions and the entity fields it tests. evidence: [streaming.md §2026-10-14T11:13](../../re/streaming.md#ts-20261014-111312).
- [ ] TRANS-6 — Building entries inside a level (lift in lvl_00 between room1/3/5, `Lobby2Basement_SceneChange` objects enabled by `EnableBound`).

## 6. Findings summary

- [streaming.md §2026-10-13T22:27](../../re/streaming.md#ts-20261013-222700) (level switch chain, event bindings, unknowns).
- [scripts.md §2026-10-14T10:54](../../re/scripts.md#ts-20261014-105432) (WarningDialogBox results and look, the poke region belongs to `Police_Station_Transition_Lower`, the user's loading-screen observation).
- [streaming.md §2026-10-14T11:13](../../re/streaming.md#ts-20261014-111312) (trigger, dialog, SceneChange, request order, loading page, room states).
- [streaming.md §2026-10-14T10:07](../../re/streaming.md#ts-20261014-100715) (bound enable byte, WarningDialogBox, observed switch chain, order unknown).

## 7. Code and tests

- `Level::triggers`, `LevelTrigger` (`src/assets/level.cpp`). CTest `streaming-real-data`.
