# QoL true full screen

## 1. Scope

The top screen as the whole window: the 3D view widened up to 16:9 (same vertical field of view, more world left and right; wider windows get black side bars), the HUD anchored to the corners of the 16:9 area, the edges checked for pop-in. While the bottom screen shows, the widened view fills the full window width at the rows of today's top screen ([qol-screen-layout.md §2026-10-29T11:42:22](../../design/qol-screen-layout.md#ts-20261029-114222)). Plan: [qol.md](../qol.md). Toggle: own switch in the settings menu ([qol-settings-menu.md](qol-settings-menu.md)).

## 2. Status

IN_PROGRESS, opened 2026-10-28T09:21:35+01:00. The geometry, the render capability (QOLFS-7) and the hook (QOLFS-8) exist: with the toggle on, the 3D view and the glow pass use the widened region above the bottom screen, or the whole window while the bottom screen is hidden; the HUD canvas stays on today's top rectangle until QOLFS-2. The per-part HUD rectangles exist in `src/qol/hud_hook.*` (the QOLFS-12 port capability exists, the QOLFS-13 hook waits). The config load hands the settings to the active store at startup and after F11 (QOLFS-11).

## 3. Verified behaviour

Window size to game view rectangle, bars and aspect, and the corner anchors of the HUD are pinned by hand-computed values (`qol-view-layout`). The region of the 3D view equals today's top screen with the toggle off and is widened above the bottom screen with it on (`qol-view-hook`). Every behaviour is tested first in original mode (all QoL off, the port's tests unchanged), then with the feature on ([qol rules](../../rules/qol.md)).

## 4. Stand-ins

None.

## 5. Open items

- [ ] QOLFS-1 — The widened full-window view up to 16:9, black side bars beyond, with the same vertical field of view; windows narrower than 5:3 keep the original 5:3 view with bars above and below; one toggle for the view and the corner HUD. Wired above the bottom screen (`qol::topViewRegion`); With the bottom screen hidden (Improved) the view is widened over the whole window (QOLBOT-20). needs: QOLSET-1, QOLFS-7, QOLBOT-1. blocks: QOLFS-2, QOLFS-3, QOLMAP-1, QOLFS-9. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLFS-2 — HUD parts anchored to the corners of the 16:9 area inside the bars: hearts and character picture top-left, studs and counters top-right, ability bottom-left, scaled to the window height; same toggle as QOLFS-1. The anchor math is done (`hudAnchor`, `hudPosition`) and `qol::hudPartRect` places each part's 400x240 canvas at its corner (done, tested); drawing the parts there is left. needs: QOLFS-1, QOLFS-13. evidence: [qol-screen-layout.md §2026-10-29T11:42:22](../../design/qol-screen-layout.md#ts-20261029-114222).
- [ ] QOLFS-3 — Check the widened edges for pop-in (rooms, objects, pedestrians, shadows, fades) at the widest supported aspect, 16:9. needs: QOLFS-1. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLFS-10 — The window has a minimum size of 400x240, set at window creation by the window glue (owned there, not by the view hook). needs: QOLSET-19. evidence: [qol-screen-layout.md §2026-10-28T22:21:19](../../design/qol-screen-layout.md#ts-20261028-222119).
- [ ] QOLFS-13 — The hook line in `LevelSession::draw` that asks `qol::hudPartRect` for each part's rectangle and draws the part there with `Hud::draw(canvas, spin, HudPartSet)` and `HudOverlay::drawTop` (`src/frontend/hud_parts.hpp`): map `qol::HudPart` Hearts to Hearts, Portrait to Portrait, Studs to Studs, Counters to SuperBricks, Ability to CameraLock (the port has no other counters or ability part; HUD-94), clear the canvas between parts, listed in "Hook points" of [qol-settings-menu.md](qol-settings-menu.md) in the same branch. blocks: QOLFS-2. evidence: [qol-screen-layout.md §2026-10-29T11:42:22](../../design/qol-screen-layout.md#ts-20261029-114222).

## 6. Findings summary

Design: [qol-screen-layout.md](../../design/qol-screen-layout.md); geometry rules of this package: [qol-screen-layout.md §2026-10-28T22:21:19](../../design/qol-screen-layout.md#ts-20261028-222119); place of the view, HUD corners and HUD canvas: [§2026-10-29T11:42:22](../../design/qol-screen-layout.md#ts-20261029-114222).

## 7. Code and tests

`src/render/view_region.hpp`, `src/render/view_region.cpp`: `ViewRegion` (pixel rectangle and aspect terms, plain types), `originalViewRegion`, `makeViewRegion` (strict), `viewAspect`, `beginViewRegion` (clears the window black, then the region); test `render-viewport-rect` (`tests/render_viewport_rect_test.cpp`). The glow pass takes the region's rectangle; the HUD overlay keeps today's top rectangle (user answer). `src/qol/view_layout.hpp`, `src/qol/view_layout.cpp`: `computeViewLayout` (window, 400x240 minimum), `layoutInArea` (any positive area), `hudAnchor`, `hudPosition`. `src/qol/view_hook.hpp`, `src/qol/view_hook.cpp` (library `chase_qol_view`): `topViewRegion(width, height, topRect)` (the whole window while the bottom screen is hidden, else the full width at the rows of the top screen), `widenedViewRegion(area)`; test `qol-view-hook` (`tests/qol/view_hook_test.cpp`: off equals `originalViewRegion` over a grid of windows, then on through the menu model). `src/qol/hud_hook.hpp`, `src/qol/hud_hook.cpp` (in `chase_qol_view`): `HudPart`, `hudPartRect` (window rectangle of one part's 400x240 canvas: top screen's rectangle while off, scaled corner placement while on); test `qol-hud-hook` (`tests/qol/hud_hook_test.cpp`: off over a grid of windows, then hand-computed rectangles). `src/qol/active_settings.*`: `activeSettings`, `setActiveSettings`, `enabled`; test `qol-active-settings`. Test: `qol-view-layout` (`tests/qol/view_layout_test.cpp`), pinned to the design entries. The original-mode tests are the unchanged port suite. `src/frontend/hud_parts.*`, `Hud::draw` with a part set and `HudOverlay::drawTop` (`src/render/overlay_quad.*` holds the placement maths): tests `hud-parts`, `hud-parts-real-data`, `render-overlay-quad`; see [hud-ui.md](hud-ui.md).
