# Level transitions

## 1. Scope

Leaving a street for a building and travelling between world levels: `SceneChange` script native, `scenechange` objects, the loading screen, the destination placement. Room loading is in [world-streaming.md](world-streaming.md). The police station (lvl_00) and the city (lvl_01) are separate levels in the port.

## 2. Status

IN_PROGRESS, updated 2026-10-14T10:07:15+0200. The chain from the script native to the scene change execution is mapped; event bindings (`Level::triggers`) are decoded; one switch (street to lvl_00) was reproduced in the oracle with a poked enable byte. Nothing runs in `chase-game`.

## 3. Verified behaviour

- `SceneChange <bound>` (handler 0x1b262c) finds a `scenechange` object (type 78) by bound hash and sends it message 0xff; execution 0x303480 unloads and reloads the world when the destination world level is not loaded (progress 25/50/100) and only moves the player when it is. evidence: [streaming.md §2026-10-13T22:27](../../re/streaming.md#ts-20261013-222700).
- 24 `SceneChange` calls exist in shipped level scripts (city to Auburn, Downtown, Albatross, police entry and helipad entry, bluebell and fort meadows exits). Entering the police station from the street runs script `Police_ExtToInt` (dialog, camera, doors, `SceneChange`).
- The master level lists 35 `scenechange` objects with qualified bounds (`WorldLevel_01_CherryTreeHills.Police_ExtToInt`).
- The `Police_Station_Transition_Lower` bound was disabled at gameplay-start (enable byte +0xa of its 0x28-byte record; `1-1_MissionHide` clears it, `1-1_MissionEnd` and `1-1_MissionSkip` set it). With it enabled and the player inside, script `Police_ExtToInt` opens `WarningDialogBox` 34 updates later; A answers 2; the switch then runs `0x303480` -> `0x2e3cc4` -> `0x2e3a9c` -> `0x223878` -> `0x21ac70` in one update (poked states, measuring aid). evidence: [streaming.md §2026-10-14T10:07](../../re/streaming.md#ts-20261014-100715).
- Event bindings (bound entered and others to scripts) are decoded and pinned (`streaming-real-data`).

## 4. Stand-ins

- TRANS-S1 The port treats lvl_00 and lvl_01 as separate levels chosen on the command line; there is no switch at run time.

## 5. Open items

- [x] TRANS-1 — closed 2026-10-14: cause of the silent bound and the trap chain recorded (see Verified behaviour). Remaining question moved to TRANS-7.
- [ ] TRANS-7 — Who requests the mode switch to `0x303480`: in one run the switch ran without the `SceneChange` native (0x1b262c) being hit, in another the native ran 141 updates after the dialog and no switch followed within 30 updates. Candidate: the scenechange update 0x1c47e8 (bound enabled and player inside, manager slot +0x28/+0x2c at 0x3038a0/0x3037e8). Also needs a reproducible state reached without a poke (play mission 1-1 to `1-1_MissionEnd`). needs: none. blocks: TRANS-2.
- [ ] TRANS-2 — Destination semantics: the scenechange object's `initialposition`, `SceneChangeTo`, hash 0xccaaccdd and the scene index byte (+0xd of the destination entity); how the bound argument is qualified with the world level name; whether world level offsets of the master are applied. needs: TRANS-7.
- [ ] TRANS-3 — Master level in `chase-game`: load `lvl_the_city`, world levels as scenes, switch with fade and the generic loading screen. needs: SCRIPT-1, WSTREAM-3, FE-3.
- [x] TRANS-4 — Closed 2026-10-14: trigger dispatch runs in `chase-game` (`ScriptRunner::checkTriggers`, 0x1746c0): BoundEntered, Player1BoundEntered, PlayerOnFootBoundEntered and BoundLeft with the bounds of section +0x24, ObjectTriggered / ObjectUntriggered from `TriggerObject`; the other trigger types are SCRIPT-11 ([script-system.md](script-system.md)). evidence: [scripts.md §2026-10-14T10:06](../../re/scripts.md#ts-20261014-100640).
- [ ] TRANS-5 — Natives needed by the transition scripts (`WarningDialogBox` behaviour: [streaming.md §2026-10-14T10:07](../../re/streaming.md#ts-20261014-100715)): `WarningDialogBox`, `PositionPlayer`, `PositionObject`, `SnapCamera`, `FadeScreen`, `AIMove`, `StartDCam`. needs: SCRIPT-1.
- [ ] TRANS-6 — Building entries inside a level (lift in lvl_00 between room1/3/5, `Lobby2Basement_SceneChange` objects enabled by `EnableBound`). needs: TRANS-7.

## 6. Findings summary

- [streaming.md §2026-10-13T22:27](../../re/streaming.md#ts-20261013-222700) (level switch chain, event bindings, unknowns).
- [streaming.md §2026-10-14T10:07](../../re/streaming.md#ts-20261014-100715) (bound enable byte, WarningDialogBox, observed switch chain, order unknown).

## 7. Code and tests

- `Level::triggers`, `LevelTrigger` (`src/assets/level.cpp`). CTest `streaming-real-data`.
