# QoL true full screen

## 1. Scope

The top screen as the whole window: the 3D view widened up to 16:9 (same vertical field of view, more world left and right; wider windows get black side bars), the HUD anchored to the corners of the 16:9 area, the edges checked for pop-in. While the bottom screen shows, the widened view fills the full window width at the rows of today's top screen ([qol-screen-layout.md §2026-10-29T11:42:22](../../design/qol-screen-layout.md#ts-20261029-114222)). Plan: [qol.md](../qol.md). Toggle: own switch in the settings menu ([qol-settings-menu.md](qol-settings-menu.md)).

## 2. Status

IN_PROGRESS, opened 2026-10-28T09:21:35+01:00. The geometry, the render capability (QOLFS-7) and the hook (QOLFS-8) exist: with the toggle on, the 3D view and the glow pass use the widened region above the bottom screen, or the whole window while the bottom screen is hidden; with the toggle on each HUD part is drawn at its corner rectangle, at the picture's corners while a 5:3 camera shows (QOLFS-19; `src/qol/hud_hook.*`, `src/qol/hud_parts_hook.*`, QOLFS-2, QOLFS-13); the head icon and the scanner overlay stay on today's top rectangle. Only the gameplay camera widens; the cutscene, director and scanner cameras keep the 5:3 region (QOLFS-18). The config load hands the settings to the active store at startup and after F11 (QOLFS-11). The window has its 400x240 minimum from creation (QOLFS-10).

## 3. Verified behaviour

Window size to game view rectangle, bars and aspect, and the corner anchors of the HUD are pinned by hand-computed values (`qol-view-layout`). The region of the 3D view equals today's top screen with the toggle off and is widened above the bottom screen with it on (`qol-view-hook`). Every behaviour is tested first in original mode (all QoL off, the port's tests unchanged), then with the feature on ([qol rules](../../rules/qol.md)).

## 4. Stand-ins

None.

## 5. Open items

- [ ] QOLFS-1 — The widened full-window view up to 16:9, black side bars beyond, with the same vertical field of view; windows narrower than 5:3 keep the original 5:3 view with bars above and below; one toggle for the view and the corner HUD. Wired above the bottom screen (`qol::topViewRegion`); With the bottom screen hidden (Improved) the view is widened over the whole window (QOLBOT-20). needs: QOLSET-1, QOLFS-7, QOLBOT-1. blocks: QOLFS-2, QOLFS-3, QOLMAP-1, QOLFS-9, QOLFS-16, QOLFS-17, QOLFS-18, QOLFS-20. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLFS-20 — With the widened view on, the room request also asks for the rooms the 16:9 frustum reaches, beyond the current room's neighbours and preloads (original streaming with the toggle off); needs a hook point on the room request and a plain-type interface from `src/qol/`. needs: QOLFS-1. evidence: [qol-screen-layout.md §2026-11-02T15:42:07](../../design/qol-screen-layout.md#ts-20261102-154207).

## 6. Findings summary

Pop-in at the widened edges: [view-edges.md §2026-10-30T14:03:24](../../re/view-edges.md#ts-20261030-140324) (range decisions are radial or depth based; the pedestrian spawn cone covers 16:9 at the follow camera's FOV); largest record FOV and rooms only the strip reaches: [view-edges.md §2026-11-02T15:16:57](../../re/view-edges.md#ts-20261102-151657).

Design: [qol-screen-layout.md](../../design/qol-screen-layout.md); geometry rules of this package: [qol-screen-layout.md §2026-10-28T22:21:19](../../design/qol-screen-layout.md#ts-20261028-222119); place of the view, HUD corners and HUD canvas: [§2026-10-29T11:42:22](../../design/qol-screen-layout.md#ts-20261029-114222).

## 7. Code and tests

`src/render/view_region.hpp`, `src/render/view_region.cpp`: `ViewRegion` (pixel rectangle and aspect terms, plain types), `originalViewRegion`, `makeViewRegion` (strict), `viewAspect`, `beginViewRegion` (clears the window black, then the region); test `render-viewport-rect` (`tests/render_viewport_rect_test.cpp`). The glow pass takes the region's rectangle; the HUD overlay keeps today's top rectangle (user answer). `src/qol/view_layout.hpp`, `src/qol/view_layout.cpp`: `computeViewLayout` (window, 400x240 minimum), `layoutInArea` (any positive area), `hudAnchor`, `hudPosition`. `src/qol/view_hook.hpp`, `src/qol/view_hook.cpp` (library `chase_qol_view`): `topViewRegion(width, height, topRect)` (the whole window while the bottom screen is hidden, else the full width at the rows of the top screen), `widenedViewRegion(area)`; test `qol-view-hook` (`tests/qol/view_hook_test.cpp`: off equals `originalViewRegion` over a grid of windows, then on through the menu model). `src/qol/hud_hook.hpp`, `src/qol/hud_hook.cpp` (in `chase_qol_view`): `HudPart`, `hudPartRect` (window rectangle of one part's 400x240 canvas: top screen's rectangle while off, scaled corner placement while on); test `qol-hud-hook` (`tests/qol/hud_hook_test.cpp`: off over a grid of windows, then hand-computed rectangles). `src/qol/hud_parts_hook.*` (library `chase_qol_hud`): `hudPartDraws` (empty while off, else the five parts with their rectangles), `drawHudPartsWith` (clears the canvas before each part), `drawHudParts` (draws through `HudOverlay::drawTop`); test `qol-hud-parts-hook` (`tests/qol/hud_parts_hook_test.cpp`: off, then the mapping, rectangles and clearing). `src/qol/active_settings.*`: `activeSettings`, `setActiveSettings`, `enabled`; test `qol-active-settings`. Test: `qol-view-layout` (`tests/qol/view_layout_test.cpp`), pinned to the design entries. The original-mode tests are the unchanged port suite. `src/frontend/hud_parts.*`, `Hud::draw` with a part set and `HudOverlay::drawTop` (`src/render/overlay_quad.*` holds the placement maths): tests `hud-parts`, `hud-parts-real-data`, `render-overlay-quad`; see [hud-ui.md](hud-ui.md). Research test `qol-view-edges-real-data` (`tests/qol/view_edges_real_test.cpp`, needs game data, slow): record FOV maximum 43 over all levels and the per-level counts and depths of strip-only unloaded rooms, pinned with provenance.
