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
- Hiding the bottom screen requires the minimap on (corrected in part by [§2026-10-28T09:48:15](#ts-20261028-094815): no per-panel toggles); the PC pause menu holds Resume, Options (original page), Credits, Exit and PC Settings; all panels use the original game's art and fonts: [§2026-10-28T09:37:31](#ts-20261028-093731).
- There are no per-panel toggles and no own toggle for the bottom screen: the master button hides it (Improved) or shows it (Original); the panels show when it is hidden: [§2026-10-28T09:48:15](#ts-20261028-094815).
- The minimap has no toggle of its own and shows only in Improved; the bottom screen is hidden exactly when the minimap is on: [§2026-10-28T09:50:34](#ts-20261028-095034).
- The master state is stored in `config/qol.cfg`; Improved turns on the minimap and the hidden bottom screen, the per-feature switches apply only in Custom: [qol-settings.md §2026-10-28T09:53:33](qol-settings.md#ts-20261028-095333).
- Geometry of the widened view and the corner HUD as pure functions, with the points the spec leaves open: [§2026-10-28T22:21:19+01:00](#ts-20261028-222119).
- The user accepted the geometry choices and set the smallest window to 400x240: [§2026-10-28T22:25:37+01:00](#ts-20261028-222537).
- The window has a minimum size of 400x240; the geometry's rejection of smaller sizes is only a safeguard: [§2026-10-28T22:39:39+01:00](#ts-20261028-223939).

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
Corrected by [qol-screen-layout.md §2026-10-28T09:48:15](#ts-20261028-094815): there are no per-panel toggles and the bottom screen is switched by the master button only.
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
Corrected by [qol-screen-layout.md §2026-10-28T09:48:15](#ts-20261028-094815): there are no per-panel toggles and the bottom screen is switched by the master button only.

Asked of the user by the lead on 2026-10-28; the quoted label is the user's selection. Decides QOLBOT-14. [Corrects §2026-10-28T09:37:31 in part](#ts-20261028-093731): QOLBOT-11 said "one toggle per panel; hiding the bottom screen requires the minimap on".

- Hidden page with its panel toggle off (QOLBOT-14): "Hiding forces panels on" (not the recommended option, which was to show the bottom screen for that page only): hiding the bottom screen turns every panel toggle on too. The option text the user saw said the per-panel choice becomes meaningless.
- What stands: one toggle per panel, and hiding requires the minimap on (QOLBOT-11). What changes: with the bottom screen hidden, every panel is on.

Open (not decided by the user): after the bottom screen is hidden, what turning one panel off does (QOLBOT-15); what a panel toggle does while the bottom screen is visible (QOLBOT-16); whether "hide the bottom screen" is its own toggle (QOLBOT-17).

<a id="ts-20261028-094815"></a>

## 2026-10-28T09:48:15+01:00 — No per-panel toggles: the master button switches the bottom screen

Asked of the user by the lead on 2026-10-28; the quoted labels are the user's selections. Decides QOLBOT-15 to QOLBOT-17. [Corrects §2026-10-28T09:37:31](#ts-20261028-093731) (QOLBOT-11, one toggle per panel) and [Corrects §2026-10-28T09:40:42](#ts-20261028-094042) (hiding turns every panel toggle on).

- Own toggle for hiding (QOLBOT-17): "Only via master": "No separate switch; hiding happens only when the master button is on Improved."
- Panel toggle while the bottom screen is shown (QOLBOT-16): the user typed their own answer: "No toggle for seperate Screens. Either bottom is on or off".
- Turning one panel off after hiding (QOLBOT-15): "Locked on (Rec.)". Without per-panel toggles this has no effect.
- The lead's clarifying question, because the answers above were ambiguous together: "So how is the bottom screen switched?" The user chose "Master only": "No toggle at all: Improved hides it, Original shows it, and in Custom it stays as it was in the last mix's source (needs recording)."

What stands: hiding the bottom screen requires the minimap on (QOLBOT-11); the panels use the original art and fonts; the pause menu content. What no longer holds: one toggle per panel; hiding turns every panel toggle on.

Open (not decided by the user): what the bottom screen and the panels do in Custom, since the option text "stays as it was in the last mix's source" is unclear (QOLBOT-18); whether the minimap stays its own toggle or follows the master button only (QOLMAP-4).

<a id="ts-20261028-095034"></a>

## 2026-10-28T09:50:34+01:00 — The minimap follows the master button; the bottom screen follows the minimap

Asked of the user by the lead on 2026-10-28; the quoted labels are the user's selections. Decides QOLMAP-4 and QOLBOT-18, the open points of [§2026-10-28T09:48:15](#ts-20261028-094815).

- Minimap toggle (QOLMAP-4): "Master only": "Minimap shows only in Improved, together with the hidden bottom screen." This was not the recommended option (the recommendation was a toggle of its own).
- Bottom screen in Custom (QOLBOT-18): "Hidden iff minimap on (Rec.)": "Hidden exactly when the minimap is on, so the map is never lost; your mix controls it through the minimap."

Reading of the lead (not a decision of the user): together these mean the minimap is never on in Custom, so the bottom screen is always shown in Custom; the minimap and the hiding of the bottom screen are Improved only. In Custom only the remaining per-feature switches (so far the widened view with the corner HUD) can differ from Original.

Open (not decided by the user): how the master state is stored and derived now that most features follow the master only (QOLSET-15).

<a id="ts-20261028-222119"></a>

## 2026-10-28T22:21:19+01:00 — Geometry of the widened view and the corner HUD

Implemented by 20261028-qolmenu-B in `src/qol/view_layout.*` from [§2026-10-28T09:21:33](#ts-20261028-092133), [§2026-10-28T09:31:02](#ts-20261028-093102) and [§2026-10-28T09:37:31](#ts-20261028-093731); no new user decision.

- The view takes the window aspect, limited to 5:3 at the narrow end and 16:9 at the wide end, centred in the window. Wider windows get a bar left and right, narrower windows a bar above and below; the aspect to render at is the view rectangle's.
- The HUD anchors are the view's top-left, top-right and bottom-left corners; the scale is the view height divided by 240, the original top-screen height; an element keeps its original offset from its corner, scaled.
- Window sizes of zero or less, and windows whose view would be under one pixel, are errors.

Unknown, for the user (QOLFS-9): the rounding of view and bar sizes (implemented: floor, the odd pixel to the right or bottom bar); the smallest window size; whether the HUD scale in a window narrower than 5:3 follows the window height (as the spec words it) or the 5:3 view height (implemented, so the HUD stays inside the view); whether HUD elements keep their original offset from the corner.

<a id="ts-20261028-222537"></a>

## 2026-10-28T22:25:37+01:00 — Geometry choices accepted, smallest window 400x240
Asked of the user by the lead on 2026-10-28; the quoted label is the user's selection. Decides QOLFS-9, the open points of [§2026-10-28T22:21:19](#ts-20261028-222119).

- "Accept; min 400x240": "Keep B's three choices; the smallest window is 400x240, the 3DS top screen size." The choices: sizes round down with the odd pixel in the right or bottom bar; in windows narrower than 5:3 the HUD scales by the view height; HUD elements keep their scaled corner offset.

Reading of the agent (not a decision of the user): a window below 400x240 in either dimension is rejected with an error like any unusable size.

Open (not decided by the user): what the game does for a window below the minimum (reject, or grow the window).

<a id="ts-20261028-223939"></a>

## 2026-10-28T22:39:39+01:00 — The window cannot shrink below 400x240
Asked of the user by the lead on 2026-10-28; the quoted label is the user's selection. Resolves the open point of the entry on the 400x240 minimum (what happens when the window is made smaller).

- "Window can't shrink below": "The window has a minimum size of 400x240, so a smaller size can't happen. The geometry rejecting it stays only as a safeguard."
