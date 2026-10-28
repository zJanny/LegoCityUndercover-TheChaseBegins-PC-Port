# QoL true full screen

## 1. Scope

The top screen as the whole window: the 3D view widened up to 16:9 (same vertical field of view, more world left and right; wider windows get black side bars), the HUD anchored to the corners, the edges checked for pop-in. Plan: [qol.md](../qol.md). Toggle: own switch in the settings menu ([qol-settings-menu.md](qol-settings-menu.md)).

## 2. Status

IN_PROGRESS, opened 2026-10-28T09:21:35+01:00. The pure geometry exists (QOLFS-1, QOLFS-2 stay open until it is wired); no render capability and no hook yet.

## 3. Verified behaviour

Window size to game view rectangle, bars and aspect, and the corner anchors of the HUD are pinned by hand-computed values (`qol-view-layout`). Every behaviour is tested first in original mode (all QoL off, the port's tests unchanged), then with the feature on ([qol rules](../../rules/qol.md)).

## 4. Stand-ins

None.

## 5. Open items

- [ ] QOLFS-1 — The widened full-window view up to 16:9, black side bars beyond, with the same vertical field of view; windows narrower than 5:3 keep the original 5:3 view with bars above and below; one toggle for the view and the corner HUD. The geometry is done (`computeViewLayout`); wiring is left. needs: QOLSET-1, QOLFS-7, QOLFS-8. blocks: QOLFS-2, QOLFS-3, QOLMAP-1, QOLBOT-1, QOLFS-9. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLFS-2 — HUD parts anchored to the window corners: hearts and character picture top-left, studs and counters top-right, ability bottom-left, scaled to the window height; same toggle as QOLFS-1. The anchor math is done (`hudAnchor`, `hudPosition`); drawing the HUD at the anchors is left. needs: QOLFS-1. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLFS-3 — Check the widened edges for pop-in (rooms, objects, pedestrians, shadows, fades) at the widest supported aspect, 16:9. needs: QOLFS-1. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLFS-7 — Port item: a generic capability in `chase_render` to draw the top screen into a given viewport rectangle at a given aspect and to clear the bars black, with no change at the defaults; QoL cannot widen the view without it. blocks: QOLFS-8, QOLFS-1. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLFS-8 — The hook: one call from the port's window and viewport code into `src/qol/view_layout.hpp`, behind `qol::Feature::WidenedView`, listed in "Hook points" of [qol-settings-menu.md](qol-settings-menu.md); the window gets a minimum size of 400x240 ([qol-screen-layout.md §2026-10-28T22:39:39+01:00](../../design/qol-screen-layout.md#ts-20261028-223939)). needs: QOLFS-7. blocks: QOLFS-1. evidence: [qol-screen-layout.md §2026-10-28T09:37:31](../../design/qol-screen-layout.md#ts-20261028-093731).

## 6. Findings summary

Design: [qol-screen-layout.md](../../design/qol-screen-layout.md); geometry rules of this package: [qol-screen-layout.md §2026-10-28T22:21:19](../../design/qol-screen-layout.md#ts-20261028-222119).

## 7. Code and tests

`src/qol/view_layout.hpp`, `src/qol/view_layout.cpp`: `computeViewLayout`, `hudAnchor`, `hudPosition`. Test: `qol-view-layout` (`tests/qol/view_layout_test.cpp`), pinned to the design entries. The original-mode tests are the unchanged port suite.
