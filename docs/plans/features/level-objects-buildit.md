# Level objects: build-its

## 1. Scope

Build-its (`props/prop_buildit`, object type 0x15): the use zone, the build with the action button, the final prop and the event that tells the story script the build is done. Mission 1-1 step 6 (`1-1_BI_DonutVanParts`) is the first user. Super build-its (`props/prop_superbuildit`) and the bricks of other classes are not covered. General object items are in [level-objects.md](level-objects.md).

## 2. Status

IN_PROGRESS, updated 2026-10-16T09:35:36+02:00. `chase-game` builds `1-1_BI_DonutVanParts` when the player stands in its zone and holds A: the state machine, the per-brick timing and the completion run as in the original (85 updates for six bricks, oracle state `m11-4-van-parts`), and the finished build starts `1-1_SO_VanFixed`. The parts model is drawn node by node: the six bricks lie scattered at the first frame of `prop_donut_truck_cabin_bi_buildit.bfnanm`, each flies to the final prop's place (final prop world matrix times the inverse of the parts', 0x2318c8; oracle trace) while it builds (0x19c9e0) and the final prop rises by sin of the spin (state 10). Unbuilt bricks hop while the player stands in the zone (formula, phase and random draws as measured; no sound yet). The picture is not yet the original's: the player does not play the building animation.

## 3. Verified behaviour

- Class parameters of `1-1_BI_DonutVanParts`: final prop `1-1_Donut_Van_Body`, script object `1-1_SO_VanFixed`, `StudValue` 25, six bricks (nodes with a mesh of `Prop_Donut_Truck_Cabin_BI`); CTest `buildit-real-data`.
- Use zone: the node bounds widened by 1.0 on every half extent; centre (-0.442, 0.997, 0.980), half (4.271, 2.559, 4.007) relative to the object, equal to the record read in the oracle; CTest `buildit-real-data`.
- State machine and timing: brick start one update after the press, nine progress updates of 0.1, 0.106, ... per brick, 12 updates per brick, final prop shown at update 73 and built at update 85 after the first press; CTest `buildit` (rows read in the oracle every update, [buildit.md](../../re/buildit.md#ts-20261014-235539)).
- Completion: parts hidden, final prop shown, `ObjectTriggered` of the script object queued; `1-1_SO_VanFixed` then switches the broken van off and the driveable van on; CTest `buildit-real-data`.

## 4. Stand-ins

- LEVOBJ-S70 The start phase, amplitude and speed of each brick's hop come from a stand-in random source (fixed seed `kBuilditWobbleStandInSeed`); the original draws them from its global generator at link time. evidence: [buildit.md §2026-10-15T19:52](../../re/buildit.md#ts-20261015-195230).
- LEVOBJ-S71 The hop runs while the player stands in the use zone; the original also asks the class (0x244fc8) and a list at `*0x519114` (0x1a0f6c), and has flag bits 2 and 3 in the state-2 check. needs: LEVOBJ-71. evidence: [buildit.md §2026-10-15T19:52](../../re/buildit.md#ts-20261015-195230).
- LEVOBJ-S41 The action button is read as A (confirm slot); the original's reader of the action flag in the use query was not located.

## 5. Open items

- [ ] LEVOBJ-70 — The sound of a hop (sound 5 at the brick, pitch 1.1 - 0.2 r, first three crossings of an update): which sound it is and its volume; `WobbleSound` already carries the pitch. evidence: [buildit.md §2026-10-15T19:52](../../re/buildit.md#ts-20261015-195230).
- [ ] LEVOBJ-71 — The full condition of the hop: what fills the list tested by 0x1a0f6c, the class answer of 0x244fc8, flag bits 2 and 3 of +0x36 in the state-2 check. blocks: LEVOBJ-S71. evidence: [buildit.md §2026-10-15T19:52](../../re/buildit.md#ts-20261015-195230).
- [ ] LEVOBJ-61 — Brick paths of other build-its: `MoveViaPoint` (quadratic curve), `Reverse` (flag bit 4) and animations with more than two frames. evidence: [buildit.md §2026-10-15T18:31](../../re/buildit.md#ts-20261015-183145).
- [ ] LEVOBJ-72 — (User report 2026-10-16) The donut van build looked wrong: the bricks flew to the parts' place and the van snapped in. Fixed from the trace (bricks fly to the final prop); the user is to confirm the look in `chase-game`; the player's building animation (LEVOBJ-41) and the switch time of `1-1_SO_VanFixed` against the last build update are not compared. needs: LEVOBJ-41. evidence: [buildit.md §2026-10-16T09:35](../../re/buildit.md#ts-20261016-093536).
- [ ] LEVOBJ-41 — Effects of the build that are not played: sounds `ATTR_SFXBitDone` and `ATTR_SFXBuilt`, the `StudValue` studs spawned at the end (0x2149ac), the player's building animation (animation 9 asked in the use query) and the camera (`NoCameraFollow`). blocks: LEVOBJ-72. evidence: [buildit.md §2026-10-14T23:55](../../re/buildit.md#ts-20261014-235539).
- [ ] LEVOBJ-42 — The other 15 build-its of lvl_01 and the parameters this one does not set (`UseBound`, `UseRadius`, `UseExcludeBound`, `DontUseRadius`, `Icon`, `BuildType`, percent triggers, messages 0x14, 0xfe, 0xff) are transcribed from the code only for the default path; none was measured. evidence: [buildit.md §2026-10-14T23:55](../../re/buildit.md#ts-20261014-235539).
- [ ] LEVOBJ-43 — The built state is not saved or restored (save record type 0x15, one byte, [save-format-state.md](../../re/save-format-state.md#ts-20261014-134146)); a loaded game shows the build-it unbuilt. needs: SAVE-8.

## 6. Findings summary

- Class record, messages, use zone and state machine from the code: [buildit.md §2026-10-14T23:55](../../re/buildit.md#ts-20261014-235539).
- Brick data, flight and rise from the code: [buildit.md §2026-10-15T18:31](../../re/buildit.md#ts-20261015-183145); built pose and oracle trace of the node matrices: [buildit.md §2026-10-16T09:35](../../re/buildit.md#ts-20261016-093536).
- Hop of the unbuilt bricks (start values, condition, oracle rows): [buildit.md §2026-10-15T19:52](../../re/buildit.md#ts-20261015-195230).

## 7. Code and tests

- `src/runtime/buildit.*` (state machine), `src/runtime/buildit_wobble.*` (hop of the unbuilt bricks), `src/runtime/buildit_set.*` (level objects, zone, actions), `src/runtime/reference_trigger.*` (step 0x23211c), `ScriptRunner::triggerObjectEnabling`, `assets::modelBounds`; glue `updateBuildits` in `src/game_level_update.cpp`.
- Per-node transforms of placed props (the API other features use, e.g. `Attach*`): `Scene::addModelNodes(path, placement)` in `src/render/scene.hpp` returns one scene instance per node with a mesh (-1 for none; GPU data shared by key `path#mesh`) and `Scene::setTransform(instance, matrix)` moves it, each update, to `local * placement` (row-vector `Mat4`, the game's layout). Logic without GL: `src/runtime/prop_nodes.*` (`PropNodes::of(model)` node matrices and `world()`, `nodePoseOf`/`nodeMatrixOf` quaternion conversion, `nodeSlerp`, `nodeFlight`), `src/runtime/buildit_bricks.*` (bricks of a parts model from its `_buildit` animation, `assets::objectAnimationMatrix`), `src/runtime/mesh_bouncer.*` (class `dummymeshbouncer`, `propChildren` for objects embedded in class files); glue `src/game_prop_parts.*` (`PropParts`: build-it bricks, `BuilditBricks::buildAt`, final prop rise, waypoint Badge children), `BuilditState::flight` and `propRise`.
- CTest: `buildit`, `buildit-wobble`, `buildit-real-data`, `reference-trigger`, `prop-parts`, `prop-parts-real-data`.
