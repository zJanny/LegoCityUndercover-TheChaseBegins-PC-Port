# Script system

## 1. Scope

The level scripts (level section +0x18), the script executor 0x234204 and its words, EnableObject / EnableCollision, the `startup` script, the revealer links and the start state of placed objects. At run time the original executes scripts for missions, triggers, door requests and music; the port does not run any script after the level start. Mission flow and enemies are not covered ([README.md](README.md) lists them as not started).

## 2. Status

IN_PROGRESS, updated 2026-10-13T21:29:01+0200. Scripts are decoded and the `startup` script runs once to compute the start state (`levelStartState`). No script runs during play.

## 3. Verified behaviour

- Script data layout and loader 0x1cfc50; function names re-derived by hash (0x208a3c). lvl_01 has 314 scripts.
- Over the 896 lvl_01 objects that would register collision, the state rule gives 819 enabled in both, 48 flag-0 in both, 4 without entity and disabled by the script, 12 flag-0 only through the revealer links, 8 co-located twins; no object that the rule disables while its entity is enabled. `level-start-real-data` pins this against the oracle grid read.

## 4. Stand-ins

- SCRIPT-S1 Waits and every blocking native count as elapsed; SpawnScript runs the child to its end before the parent continues (the original runs it until it yields). Only the `startup` script is run.
- SCRIPT-S2 `IsLevelUnlocked`, `GetGameComplete` and `IsRedBrickExtraActive` return 0 (new game, no save). A loaded save would change the state; the port has no save-aware start yet. needs: SAVE-1.
- SCRIPT-S3 The revealer rule is a data fit, not traced to its code; verified only in lvl_01.

## 5. Open items

- [ ] SCRIPT-1 — No script runs after the level start: no mission events, triggers or bounds, no door requests, no level music start. blocks: DOOR-1 (script-requested doors in lvl_01 stay closed), AUDIO-2 (level music), LEVOBJ-3 (scripted object behaviour), STUD-4 (stud groups). evidence: [scripts.md §2026-10-13T20:42](../../re/scripts.md).
- [ ] SCRIPT-2 — `levelStartState` lists some objects as disabled although the original has them enabled: the hydrant sprays (`Hydrant_Spray_01` appears twice in the startup script), `Pickup_CharacterToken06/07/10`, `FP_Task_COP_CTH_02..05`, `FP_Archway_GrapplePad01`, `1-3_BI_Seesaw`, `1-3_BI_Roundabout` (build-it bits; `MissionHide` disables them). Another script or class handler may re-enable them, or the class ignores message 0xfb; not investigated. None of them collides. evidence: [scripts.md §2026-10-13T20:42](../../re/scripts.md).
- [ ] SCRIPT-3 — The two detective X-spots (`1-2_Dtv_DogPath_XSpot`, `1-3_Dtv_BrickSpot_XSpot_02`) are not explained (no grid entry). evidence: [collision.md §2026-10-13T20:43](../../re/collision.md).
- [ ] SCRIPT-4 — The code that hides the revealer targets; the role of the other type-4 parameters (Destination, ObjectTo, TriggerObject, ...); the revealer rule is a data fit and only lvl_01 was compared with the oracle (lvl_00 not). evidence: [scripts.md §2026-10-13T20:42](../../re/scripts.md).
- [ ] SCRIPT-5 — Variables of type 5 and 7 in conditions are converted to floats; the executor compares floats. evidence: [scripts.md §2026-10-13T20:42](../../re/scripts.md).

## 6. Findings summary

- Script record layout, argument strings, function-name hash lookup: [scripts.md §2026-10-13T20:42](../../re/scripts.md).
- The `startup` script plus build-it / reveal links give the disabled objects (replaces the 85-name list): [scripts.md §2026-10-13T20:42](../../re/scripts.md), [collision.md §2026-10-13T20:43](../../re/collision.md).
- Door open requests arrive as message 0xff from `TriggerObject` / the platform link; the senders in lvl_01 are mission scripts that are not traced: [objects.md §2026-10-13T21:17](../../re/objects.md).

## 7. Code and tests

- `Level::scripts` / `objectReferenceHash` (`src/assets/level.cpp`), `levelStartState` (`src/runtime/level_start.cpp`), `PlacedObjectState` (`src/assets/object_collision.*`).
- CTest: `level-start-real-data`, `collision-real-data`.
