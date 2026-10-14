# Level transitions

## 1. Scope

Leaving a street for a building and travelling between world levels: `SceneChange` script native, `scenechange` objects, the loading screen, the destination placement. Room loading is in [world-streaming.md](world-streaming.md). The police station (lvl_00) and the city (lvl_01) are separate levels in the port.

## 2. Status

IN_PROGRESS, updated 2026-10-13T22:30:00+0200. The chain from the script native to the scene change execution is mapped; event bindings (`Level::triggers`) are decoded. Nothing runs in `chase-game`.

## 3. Verified behaviour

- `SceneChange <bound>` (handler 0x1b262c) finds a `scenechange` object (type 78) by bound hash and sends it message 0xff; execution 0x303480 unloads and reloads the world when the destination world level is not loaded (progress 25/50/100) and only moves the player when it is. evidence: [streaming.md §2026-10-13T22:27](../../re/streaming.md#ts-20261013-222700).
- 24 `SceneChange` calls exist in shipped level scripts (city to Auburn, Downtown, Albatross, police entry and helipad entry, bluebell and fort meadows exits). Entering the police station from the street runs script `Police_ExtToInt` (dialog, camera, doors, `SceneChange`).
- The master level lists 35 `scenechange` objects with qualified bounds (`WorldLevel_01_CherryTreeHills.Police_ExtToInt`).
- Event bindings (bound entered and others to scripts) are decoded and pinned (`streaming-real-data`).

## 4. Stand-ins

- TRANS-S1 The port treats lvl_00 and lvl_01 as separate levels chosen on the command line; there is no switch at run time.

## 5. Open items

- [ ] TRANS-1 — Reproduce one switch in the oracle: find why the `Police_Station_Transition_Lower` bound did not fire after `teleport` (bound enabled by `EnableBound`? mission state? trigger enabled by the `startup` script), then trap 0x303480, 0x2e3cc4, 0x2e3a9c and record the order and the room/scene states before and after. One session, oracle without RPC works. needs: none.
- [ ] TRANS-2 — Destination semantics: the scenechange object's `initialposition`, `SceneChangeTo`, hash 0xccaaccdd and the scene index byte (+0xd of the destination entity); how the bound argument is qualified with the world level name; whether world level offsets of the master are applied. needs: TRANS-1.
- [ ] TRANS-3 — Master level in `chase-game`: load `lvl_the_city`, world levels as scenes, switch with fade and the generic loading screen. needs: SCRIPT-1, WSTREAM-3, FE-3.
- [ ] TRANS-4 — Trigger dispatch: `Level::triggers` to script runs on BoundEntered / PlayerOnFootBoundEntered / ObjectTriggered with the bounds from section +0x24. needs: SCRIPT-1.
- [ ] TRANS-5 — Natives needed by the transition scripts: `WarningDialogBox`, `PositionPlayer`, `PositionObject`, `SnapCamera`, `FadeScreen`, `AIMove`, `StartDCam`. needs: SCRIPT-1.
- [ ] TRANS-6 — Building entries inside a level (lift in lvl_00 between room1/3/5, `Lobby2Basement_SceneChange` objects enabled by `EnableBound`). needs: TRANS-1.

## 6. Findings summary

- [streaming.md §2026-10-13T22:27](../../re/streaming.md#ts-20261013-222700) (level switch chain, event bindings, unknowns).

## 7. Code and tests

- `Level::triggers`, `LevelTrigger` (`src/assets/level.cpp`). CTest `streaming-real-data`.
