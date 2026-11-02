# QoL: view distance

## 1. Scope

The View distance slider of the Display tab: one factor from x1.0 to x4.0 in 0.25 steps that multiplies how far the game draws and loads (far plane, fog, room loading, pedestrian and traffic distances) for every camera. Original always keeps x1.0. Plan: [qol.md](../qol.md); design: [qol-view-distance.md §2026-11-02T18:43:07](../../design/qol-view-distance.md#ts-20261102-184307).

## 2. Status

IN_PROGRESS. The slider, its `view_distance` key and the scaling of the far plane, the fog and the pedestrian distances are done; room loading and traffic are open.

## 3. Verified behaviour

- Original mode first: with the preset Original (the default) every hook returns the original numbers (far plane 260, fog and fog table unchanged, distance fade table unchanged), and the whole test suite passed before and after the change (`qol-view-distance`).
- The row: "View distance" between Resolution and "Reset to defaults", a slider of 12 steps labelled x1.0 to x4.0; Left and Right move one step, a click or drag on the track picks the nearest step; each change is saved at once and does not touch the window (`qol-view-distance-menu`).
- In Original the row shows x1.0, is greyed out, is skipped by Up and Down and ignores clicks; the stored value stays in `config/qol.cfg` and returns in Improved or Custom. "Reset to defaults" of the Display tab sets x1.0 in every preset (`qol-view-distance-menu`).
- At x2.0 the far plane of the follow, cutscene and free cameras doubles, the fog start and end double and the fog table is built from them; the pedestrian fade-in and fade-out distances double, so the spawn window and the node fade reach twice as far (`qol-view-distance`).

## 4. Stand-ins

None.

## 5. Open items

- [ ] QOLVD-4 — Question for the user: the help text of the row. Until a wording is approved the row shows no help line. evidence: [qol-view-distance.md §2026-11-02T18:43:07](../../design/qol-view-distance.md#ts-20261102-184307).
- [ ] QOLVD-5 — Question for the user: how room loading reaches further. Rooms follow the room graph, so there is no distance to multiply; a rule needs the user's approval. evidence: [view-edges.md §2026-11-02T21:25:47](../../re/view-edges.md#ts-20261102-212547).
- [ ] QOLVD-6 — Traffic distances not scaled: the removal radius is not an input of the traffic step, so scaling only the spawn band would spawn cars the radius removes. needs: TRAFFIC-13. evidence: [view-edges.md §2026-11-02T21:25:47](../../re/view-edges.md#ts-20261102-212547).

## 6. Findings summary

Distances the code uses for drawing and loading, what the slider scales and what it leaves: [view-edges.md §2026-11-02T21:25:47](../../re/view-edges.md#ts-20261102-212547). Range rules of the port: [view-edges.md §2026-10-30T14:03:24](../../re/view-edges.md#ts-20261030-140324).

## 7. Code and tests

`src/qol/view_distance.hpp`/`.cpp` (steps, labels, strict parse, the effective factor and the hooks `viewDistance`, `withFarPlane`, `withFogDistance`, `withViewDistance`); the row in `src/qol/ui_page_display_view.cpp`; `view_distance` in `src/qol/settings.*`; per-row slider steps (`RowSpec::steps`, `widestValue`). Hook lines: [qol-settings-menu-hooks](qol-settings-menu-hooks.md). Tests: `qol-view-distance` (`tests/qol/view_distance_test.cpp`), `qol-view-distance-menu` (`tests/qol/view_distance_menu_test.cpp`).
