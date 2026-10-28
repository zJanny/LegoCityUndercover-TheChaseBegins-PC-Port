# QoL: full-screen view and bottom-screen content

The user's approved design of the first QoL features: the top screen as true full screen, and the content of the bottom screen moved into a minimap, mouse-clicked full-window panels and PC menus. Plan: [qol.md](../plans/qol.md); toggles: [qol-settings.md](qol-settings.md); feature files: [qol-fullscreen.md](../plans/features/qol-fullscreen.md), [qol-minimap.md](../plans/features/qol-minimap.md), [qol-bottom-screen.md](../plans/features/qol-bottom-screen.md).

## Current state

- True full screen: the view that is the top screen today fills the whole window, widened (same vertical field of view, more world left and right), HUD anchored to the corners, edges checked so that no pop-in shows: [§2026-10-28T09:21:33+01:00](#ts-20261028-092133).
- The bottom screen is hidden when its features are on; with all QoL off the layout is exactly today's top over bottom: [§2026-10-28T09:21:33+01:00](#ts-20261028-092133).
- Bottom-screen content: the map becomes a corner minimap; the characters button, character swap and fast-travel map become full-window panels clicked with the mouse; pause and Options become PC menus: [§2026-10-28T09:21:33+01:00](#ts-20261028-092133).
- Minimap: round, top-right corner, about one fifth of the screen height, rotates with the camera, zoom like the original bottom-screen map: [§2026-10-28T09:31:02](#ts-20261028-093102).
- The view widens up to 16:9 only; wider windows get black side bars: [§2026-10-28T09:31:02](#ts-20261028-093102).
- Scanner, Build-it, Shop and Award viewer get a PC version (full-window panel clicked with the mouse) when their QoL feature is on: [§2026-10-28T09:31:02](#ts-20261028-093102).
- Windows narrower than 5:3 keep the original 5:3 view with bars above and below; hearts and character picture top-left, studs and counters top-right, ability bottom-left, scaled to the window height; the widened view and the corner HUD are one toggle, the minimap its own: [§2026-10-28T09:37:31](#ts-20261028-093731).
- The minimap shows the same icons and objective markers as the original map at a fixed zoom, hidden in cutscenes and panels: [§2026-10-28T09:37:31](#ts-20261028-093731).
- One toggle per panel; hiding the bottom screen requires the minimap on and turns every panel toggle on too (corrected in part by [§2026-10-28T09:40:42](#ts-20261028-094042)); the PC pause menu holds Resume, Options (original page), Credits, Exit and PC Settings; all panels use the original game's art and fonts: [§2026-10-28T09:37:31](#ts-20261028-093731).
- Hiding the bottom screen turns every panel toggle on too: [§2026-10-28T09:40:42](#ts-20261028-094042).
- Open: what turning one panel off does after hiding (QOLBOT-15), what a panel toggle does while the bottom screen is visible (QOLBOT-16), and whether hiding is its own toggle (QOLBOT-17).

<a id="ts-20261028-092133"></a>

## 2026-10-28T09:21:33+01:00 — Full screen, HUD and bottom-screen content

Asked of the user by the lead on 2026-10-28 (planner questions 5 to 8); answers quoted as selected. The user's request: "True Full Screen, what is currently the top screen should be entirely Full screen. Everything in the bottom should either move into a minimap (for the map) or Like the Settings menus into a normal Settings Menu Like all PC Games".

- View (question 5a): "Widen view (Rec.)": same vertical field of view, more world visible left and right. The edges are checked so that no pop-in shows (objects, rooms, pedestrians and shadows appear and fade out of the visible range at the 3DS view's edges today).
- HUD (question 5b): "Anchor to corners (Rec.)": hearts, character picture, studs and other top-screen HUD parts move to the corners of the window.
- Bottom-screen content (question 6): "As recommended": the map is a minimap in a corner; the characters button, character swap and the fast-travel map are full-window panels clicked with the mouse; pause and Options are PC menus; each its own feature file.
- Layout (questions 7 and 8): "Yes to all (Rec.)": the bottom screen is hidden when its features are on; with all QoL off the layout is exactly today's top-over-bottom window.

Open (not decided by the user): minimap position, size, shape (square or round), zoom and whether it turns with the camera like the original map; the largest aspect ratio the widening supports; the look of the panels; which other bottom-screen pages exist and need a PC form (scanner page SCAN-11, build-it pages, shop, award viewer).

<a id="ts-20261028-093102"></a>

## 2026-10-28T09:31:02+01:00 — Minimap, widest aspect and other bottom pages

Asked of the user by the lead on 2026-10-28; the quoted labels are the user's selections. Decides the open points of [§2026-10-28T09:21:33](#ts-20261028-092133).

- Minimap (QOLMAP-2): "Top-right, round, rotates": a round minimap in the top-right corner, about 1/5 of the screen height, rotating with the camera, zoom like the original bottom-screen map.
- Other bottom pages (QOLBOT-6): "Scanner, Build-it, Shop, Award viewer" (all four) get a PC version, a full-window panel clicked with the mouse, when their QoL feature is on.
- Widest aspect (5c): "16:9": the view widens up to 16:9 only; wider screens get black side bars.

Open (not decided by the user): see the list in the report of the planner, recorded in the feature files as open items.

<a id="ts-20261028-093731"></a>

## 2026-10-28T09:37:31+01:00 — Narrow windows, HUD corners, toggles, minimap content, pause menu and panel look
Corrected by [qol-screen-layout.md §2026-10-28T09:40:42](#ts-20261028-094042): the toggle decision for the bottom-screen panels (QOLBOT-11) is partly superseded; hiding the bottom screen turns every panel toggle on too.

Asked of the user by the lead on 2026-10-28; the quoted labels are the user's selections. Decides QOLFS-4 to QOLFS-6, QOLMAP-3 and QOLBOT-11 to QOLBOT-13.

- Narrow windows (QOLFS-4): "5:3 with bars (Rec.)": windows narrower than 5:3 keep the original 5:3 view with bars above and below.
- HUD corners (QOLFS-5): "Standard corners (Rec.)": hearts and character picture top-left, studs and counters top-right, ability bottom-left; scaled to the window height.
- Toggles of the view (QOLFS-6): "One toggle": the widened view and the corner HUD are one QoL toggle; the minimap stays separate.
- Minimap content (QOLMAP-3): "Original content, fixed (Rec.)": the same icons and objective markers as the original map, hidden in cutscenes and panels, fixed zoom.
- Bottom-screen toggles (QOLBOT-11): "Per panel, needs minimap (Rec.)": one toggle per panel; hiding the bottom screen requires the minimap on.
- PC pause menu (QOLBOT-12): "Mirror + PC Settings (Rec.)": Resume, Options (the original page), Credits, Exit and PC Settings.
- Look (QOLBOT-13): "Original art (Rec.)": the original game's art and fonts for all full-window panels, settings included.

Open (not decided by the user): what a bottom-screen page does when the bottom screen is hidden but that page's panel toggle is off (QOLBOT-14).

<a id="ts-20261028-094042"></a>

## 2026-10-28T09:40:42+01:00 — Hiding the bottom screen forces the panels on

Asked of the user by the lead on 2026-10-28; the quoted label is the user's selection. Decides QOLBOT-14. [Corrects §2026-10-28T09:37:31 in part](#ts-20261028-093731): QOLBOT-11 said "one toggle per panel; hiding the bottom screen requires the minimap on".

- Hidden page with its panel toggle off (QOLBOT-14): "Hiding forces panels on" (not the recommended option, which was to show the bottom screen for that page only): hiding the bottom screen turns every panel toggle on too. The option text the user saw said the per-panel choice becomes meaningless.
- What stands: one toggle per panel, and hiding requires the minimap on (QOLBOT-11). What changes: with the bottom screen hidden, every panel is on.

Open (not decided by the user): after the bottom screen is hidden, what turning one panel off does (QOLBOT-15); what a panel toggle does while the bottom screen is visible (QOLBOT-16); whether "hide the bottom screen" is its own toggle (QOLBOT-17).
