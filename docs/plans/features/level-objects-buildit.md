# Level objects: build-its

## 1. Scope

Build-its (`props/prop_buildit`, object type 0x15): the use zone, the build with the action button, the final prop and the event that tells the story script the build is done. Mission 1-1 step 6 (`1-1_BI_DonutVanParts`) is the first user. Super build-its (`props/prop_superbuildit`) and the bricks of other classes are not covered. General object items are in [level-objects.md](level-objects.md).

## 2. Status

IN_PROGRESS, updated 2026-10-14T23:59:00+02:00. `chase-game` builds `1-1_BI_DonutVanParts` when the player stands in its zone and holds A: the state machine, the per-brick timing and the completion run as in the original (85 updates for six bricks, oracle state `m11-4-van-parts`), and the finished build starts `1-1_SO_VanFixed`. The picture is not the original's: the bricks do not fly, nothing wobbles, the player does not play the building animation.

## 3. Verified behaviour

- Class parameters of `1-1_BI_DonutVanParts`: final prop `1-1_Donut_Van_Body`, script object `1-1_SO_VanFixed`, `StudValue` 25, six bricks (nodes with a mesh of `Prop_Donut_Truck_Cabin_BI`); CTest `buildit-real-data`.
- Use zone: the node bounds widened by 1.0 on every half extent; centre (-0.442, 0.997, 0.980), half (4.271, 2.559, 4.007) relative to the object, equal to the record read in the oracle; CTest `buildit-real-data`.
- State machine and timing: brick start one update after the press, nine progress updates of 0.1, 0.106, ... per brick, 12 updates per brick, final prop shown at update 73 and built at update 85 after the first press; CTest `buildit` (rows read in the oracle every update, [buildit.md](../../re/buildit.md#ts-20261014-235539)).
- Completion: parts hidden, final prop shown, `ObjectTriggered` of the script object queued; `1-1_SO_VanFixed` then switches the broken van off and the driveable van on; CTest `buildit-real-data`.

## 4. Stand-ins

- LEVOBJ-S40 The bricks are not drawn separately: the parts model stays whole until the build ends, then the final prop appears at its placement without the rise (sin of the angle, one metre) of the original. needs: LEVOBJ-40.
- LEVOBJ-S41 The action button is read as A (confirm slot); the original's reader of the action flag in the use query was not located.

## 5. Open items

- [ ] LEVOBJ-40 — Brick animation: the scatter and idle wobble of unbuilt bricks (0x206ea0), the flight of a brick while it builds (0x19c9e0, 0x2318c8) and the rise of the final prop in state 10; the scene has no per-node transforms for placed props. evidence: [buildit.md §2026-10-14T23:55](../../re/buildit.md#ts-20261014-235539).
- [ ] LEVOBJ-41 — Effects of the build that are not played: sounds `ATTR_SFXBitDone` and `ATTR_SFXBuilt`, the `StudValue` studs spawned at the end (0x2149ac), the player's building animation (animation 9 asked in the use query) and the camera (`NoCameraFollow`). evidence: [buildit.md §2026-10-14T23:55](../../re/buildit.md#ts-20261014-235539).
- [ ] LEVOBJ-42 — The other 15 build-its of lvl_01 and the parameters this one does not set (`UseBound`, `UseRadius`, `UseExcludeBound`, `DontUseRadius`, `Icon`, `BuildType`, percent triggers, messages 0x14, 0xfe, 0xff) are transcribed from the code only for the default path; none was measured. evidence: [buildit.md §2026-10-14T23:55](../../re/buildit.md#ts-20261014-235539).
- [ ] LEVOBJ-43 — The built state is not saved or restored (save record type 0x15, one byte, [save-format-state.md](../../re/save-format-state.md#ts-20261014-134146)); a loaded game shows the build-it unbuilt. needs: SAVE-8.

## 6. Findings summary

- Class record, messages, use zone and state machine from the code: [buildit.md §2026-10-14T23:55](../../re/buildit.md#ts-20261014-235539).

## 7. Code and tests

- `src/runtime/buildit.*` (state machine), `src/runtime/buildit_set.*` (level objects, zone, actions), `src/runtime/reference_trigger.*` (step 0x23211c), `ScriptRunner::triggerObjectEnabling`, `assets::modelBounds`; glue `updateBuildits` in `src/game_level_update.cpp`.
- CTest: `buildit`, `buildit-real-data`, `reference-trigger`.
