# QoL: view distance

## 1. Scope

The View distance slider of the Display tab: one factor from x1.0 to x20.0 (0.25 steps to x4.0, whole steps above) that multiplies how far the game draws and loads (far plane, fog, room loading, pedestrian and traffic distances) for every camera. Original always keeps x1.0. Plan: [qol.md](../qol.md); design: [qol-view-distance.md §2026-11-02T18:43:07](../../design/qol-view-distance.md#ts-20261102-184307).

## 2. Status

IN_PROGRESS. The slider, its `view_distance` key and the scaling of the far plane, the fog, the pedestrian distances and room loading are done; traffic is open. The range reaches x20.0 (the user's "x20.0 maximum" and "Coarser above x4", 2026-11-02); the hooks use one rule at every factor (checked at x20.0 in `qol-view-distance`: far plane 5200, cutscene far plane 80000, fog and fade tables). Above x1.0 every room within 260 m times the factor of the camera is loaded besides the original's rooms (the user's "Radial", 2026-11-03); at x20 that is every room of Cherry Tree Hills, measured: draw calls triple, memory and frame time stay level ([view-edges.md §2026-11-03T08:48:54](../../re/view-edges.md#ts-20261103-084854)).

## 3. Verified behaviour

- Original mode first: with the preset Original (the default) every hook returns the original numbers (far plane 260, fog and fog table unchanged, distance fade table unchanged), and the whole test suite passed before and after the change (`qol-view-distance`).
- The row: "View distance" between Resolution and "Reset to defaults", a slider of 28 steps (29 positions) labelled x1.0, x1.25 ... x4.0, x5.0 ... x20.0; Left and Right move one step, a click or drag on the track picks the nearest step; each change is saved at once and does not touch the window (`qol-view-distance-menu`).
- In Original the row shows x1.0, is greyed out, is skipped by Up and Down and ignores clicks; the stored value stays in `config/qol.cfg` and returns in Improved or Custom. "Reset to defaults" of the Display tab sets x1.0 in every preset (`qol-view-distance-menu`).
- At x2.0 the far plane of the follow, cutscene and free cameras doubles, the fog start and end double and the fog table is built from them; the pedestrian fade-in and fade-out distances double, so the spawn window and the node fade reach twice as far (`qol-view-distance`).

- Room loading, original mode first: with the preset Original or both features off the room request is the original's at every update (`qol-room-request-hook`, `qol-room-request-game-original` and the unchanged `game-room-streaming`); above x1.0 rooms whose box comes within 260 m times the factor of the camera are added in every direction, at x2.0 and x20.0 to the metre (`qol-room-request-hook`), and at x20.0 all 43 rooms of Cherry Tree Hills load 9 updates after the start (`qol-room-request-game-x20`).

## 4. Stand-ins

None.

## 5. Open items

- [ ] QOLVD-6 — Traffic distances not scaled (to be scaled with the factor, the user's "Next round", 2026-11-02): the spawn band and the removal radius (`TrafficSystemStep::despawnRadius`, an input since TRAFFIC-13) have no QoL hook yet, so traffic still spawns and leaves at the original distances while the far plane, the fog, the pedestrians and room loading are scaled. evidence: [traffic-ai-driver.md §2026-11-03T08:37:32](../../re/traffic-ai-driver.md#ts-20261103-083732).
- [ ] QOLVD-8 — The rooms the hook adds are not part of the level-start settle (the original's rooms are loaded before the first update); they finish loading 9 updates after the start, so the first frames of a level at a high factor miss them; a settle input for extra rooms would be a port capability. evidence: [view-edges.md §2026-11-03T08:48:54](../../re/view-edges.md#ts-20261103-084854).
- [ ] QOLVD-7 — Depth precision at high factors: the near plane stays, the far plane grows, so the depth buffer resolves coarser far away (follow camera, near 0.5: about 0.01 m at x1.0 and 3.2 m at x20.0 at the far plane; cutscene camera, near 0.1: 3800 m at far 80000); distant coplanar surfaces can flicker. Computed, not seen in a picture. evidence: [view-edges.md §2026-11-02T22:18:57](../../re/view-edges.md#ts-20261102-221857).

## 6. Findings summary

Distances the code uses for drawing and loading, what the slider scales and what it leaves: [view-edges.md §2026-11-02T21:25:47](../../re/view-edges.md#ts-20261102-212547). Range rules of the port: [view-edges.md §2026-10-30T14:03:24](../../re/view-edges.md#ts-20261030-140324). Room request rule and the cost at x20: [view-edges.md §2026-11-03T08:48:54](../../re/view-edges.md#ts-20261103-084854).

## 7. Code and tests

`src/qol/view_distance.hpp`/`.cpp` (steps, labels, strict parse, the effective factor and the hooks `viewDistance`, `withFarPlane`, `withFogDistance`, `withViewDistance`); the row in `src/qol/ui_page_display_view.cpp`; `view_distance` in `src/qol/settings.*`; per-row slider steps (`RowSpec::steps`, `widestValue`). Hook lines: [qol-settings-menu-hooks](qol-settings-menu-hooks.md). `src/qol/room_request_hook.hpp`/`.cpp` (`withViewRooms`, `viewRooms`, `frustumReaches`, `nearestDistance`: the rooms added to the room request, shared with [qol-fullscreen](qol-fullscreen.md) QOLFS-20). Tests: `qol-view-distance` (`tests/qol/view_distance_test.cpp`), `qol-view-distance-menu` (`tests/qol/view_distance_menu_test.cpp`), `qol-room-request-hook` (`tests/qol/room_request_hook_test.cpp`), `qol-room-request-game-original`, `qol-room-request-game-strip`, `qol-room-request-game-x20` (`tests/qol/CMakeLists.txt`, need game data).
