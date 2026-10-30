# QoL true full screen

## 1. Scope

The top screen as the whole window: the 3D view widened up to 16:9 (same vertical field of view, more world left and right; wider windows get black side bars), the HUD anchored to the corners of the 16:9 area, the edges checked for pop-in. While the bottom screen shows, the widened view fills the full window width at the rows of today's top screen ([qol-screen-layout.md §2026-10-29T11:42:22](../../design/qol-screen-layout.md#ts-20261029-114222)). Plan: [qol.md](../qol.md). Toggle: own switch in the settings menu ([qol-settings-menu.md](qol-settings-menu.md)).

## 2. Status

IN_PROGRESS, opened 2026-10-28T09:21:35+01:00. The geometry, the render capability (QOLFS-7) and the hook (QOLFS-8) exist: with the toggle on, the 3D view and the glow pass use the widened region above the bottom screen, or the whole window while the bottom screen is hidden; with the toggle on each HUD part is drawn at its corner rectangle (`src/qol/hud_hook.*`, `src/qol/hud_parts_hook.*`, QOLFS-2, QOLFS-13); the head icon and the scanner overlay stay on today's top rectangle. The config load hands the settings to the active store at startup and after F11 (QOLFS-11).

## 3. Verified behaviour

Window size to game view rectangle, bars and aspect, and the corner anchors of the HUD are pinned by hand-computed values (`qol-view-layout`). The region of the 3D view equals today's top screen with the toggle off and is widened above the bottom screen with it on (`qol-view-hook`). Every behaviour is tested first in original mode (all QoL off, the port's tests unchanged), then with the feature on ([qol rules](../../rules/qol.md)).

## 4. Stand-ins

None.

## 5. Open items

- [ ] QOLFS-1 — The widened full-window view up to 16:9, black side bars beyond, with the same vertical field of view; windows narrower than 5:3 keep the original 5:3 view with bars above and below; one toggle for the view and the corner HUD. Wired above the bottom screen (`qol::topViewRegion`); With the bottom screen hidden (Improved) the view is widened over the whole window (QOLBOT-20). needs: QOLSET-1, QOLFS-7, QOLBOT-1. blocks: QOLFS-2, QOLFS-3, QOLMAP-1, QOLFS-9, QOLFS-16, QOLFS-17, QOLFS-18. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLFS-16 — Scan the camera record FOVs of all levels for the largest pose FOV: the pedestrian spawn cone (36.9 degrees half angle) holds the 16:9 half diagonal only below pose FOV 52.2 degrees; a larger one lets pedestrians fade in beside the cone. needs: QOLFS-1. evidence: [view-edges.md §2026-10-30T14:03:24](../../re/view-edges.md#ts-20261030-140324).
- [ ] QOLFS-17 — Find rooms that only the 16:9 strip reaches and that are neither loaded nor preloaded when looking straight ahead (level data: room boxes against the 16:9 frustum). needs: QOLFS-1. evidence: [view-edges.md §2026-10-30T14:03:24](../../re/view-edges.md#ts-20261030-140324).
- [ ] QOLFS-18 — Only the gameplay camera widens; the cutscene, director and scanner cameras keep the 5:3 shape with bars at the sides (the user's answer of 2026-10-30, recorded in [qol-screen-layout.md §2026-10-29T11:42:22](../../design/qol-screen-layout.md#ts-20261029-114222)). Open: the view hook widens whatever camera is active; it has to ask which camera is active and use the 5:3 region for the other three. needs: QOLFS-1. evidence: [view-edges.md §2026-10-30T14:03:24](../../re/view-edges.md#ts-20261030-140324).
- [ ] QOLFS-10 — The window has a minimum size of 400x240, set at window creation by the window glue (owned there, not by the view hook). needs: QOLSET-19. evidence: [qol-screen-layout.md §2026-10-28T22:21:19](../../design/qol-screen-layout.md#ts-20261028-222119).

## 6. Findings summary

Pop-in at the widened edges: [view-edges.md §2026-10-30T14:03:24](../../re/view-edges.md#ts-20261030-140324) (range decisions are radial or depth based; the pedestrian spawn cone covers 16:9 at the follow camera's FOV).

Design: [qol-screen-layout.md](../../design/qol-screen-layout.md); geometry rules of this package: [qol-screen-layout.md §2026-10-28T22:21:19](../../design/qol-screen-layout.md#ts-20261028-222119); place of the view, HUD corners and HUD canvas: [§2026-10-29T11:42:22](../../design/qol-screen-layout.md#ts-20261029-114222).

## 7. Code and tests

`src/render/view_region.hpp`, `src/render/view_region.cpp`: `ViewRegion` (pixel rectangle and aspect terms, plain types), `originalViewRegion`, `makeViewRegion` (strict), `viewAspect`, `beginViewRegion` (clears the window black, then the region); test `render-viewport-rect` (`tests/render_viewport_rect_test.cpp`). The glow pass takes the region's rectangle; the HUD overlay keeps today's top rectangle (user answer). `src/qol/view_layout.hpp`, `src/qol/view_layout.cpp`: `computeViewLayout` (window, 400x240 minimum), `layoutInArea` (any positive area), `hudAnchor`, `hudPosition`. `src/qol/view_hook.hpp`, `src/qol/view_hook.cpp` (library `chase_qol_view`): `topViewRegion(width, height, topRect)` (the whole window while the bottom screen is hidden, else the full width at the rows of the top screen), `widenedViewRegion(area)`; test `qol-view-hook` (`tests/qol/view_hook_test.cpp`: off equals `originalViewRegion` over a grid of windows, then on through the menu model). `src/qol/hud_hook.hpp`, `src/qol/hud_hook.cpp` (in `chase_qol_view`): `HudPart`, `hudPartRect` (window rectangle of one part's 400x240 canvas: top screen's rectangle while off, scaled corner placement while on); test `qol-hud-hook` (`tests/qol/hud_hook_test.cpp`: off over a grid of windows, then hand-computed rectangles). `src/qol/hud_parts_hook.*` (library `chase_qol_hud`): `hudPartDraws` (empty while off, else the five parts with their rectangles), `drawHudPartsWith` (clears the canvas before each part), `drawHudParts` (draws through `HudOverlay::drawTop`); test `qol-hud-parts-hook` (`tests/qol/hud_parts_hook_test.cpp`: off, then the mapping, rectangles and clearing). `src/qol/active_settings.*`: `activeSettings`, `setActiveSettings`, `enabled`; test `qol-active-settings`. Test: `qol-view-layout` (`tests/qol/view_layout_test.cpp`), pinned to the design entries. The original-mode tests are the unchanged port suite. `src/frontend/hud_parts.*`, `Hud::draw` with a part set and `HudOverlay::drawTop` (`src/render/overlay_quad.*` holds the placement maths): tests `hud-parts`, `hud-parts-real-data`, `render-overlay-quad`; see [hud-ui.md](hud-ui.md).
