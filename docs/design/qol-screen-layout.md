# QoL: full-screen view and bottom-screen content

The user's approved design of the first QoL features: the top screen as true full screen, and the content of the bottom screen moved into a minimap, mouse-clicked full-window panels and PC menus. Plan: [qol.md](../plans/qol.md); toggles: [qol-settings.md](qol-settings.md); feature files: [qol-fullscreen.md](../plans/features/qol-fullscreen.md), [qol-minimap.md](../plans/features/qol-minimap.md), [qol-bottom-screen.md](../plans/features/qol-bottom-screen.md).

<a id="ts-20261028-092133"></a>

## 2026-10-28T09:21:33+01:00 — Full screen, HUD and bottom-screen content

Asked of the user by the lead on 2026-10-28 (planner questions 5 to 8); answers quoted as selected. The user's request: "True Full Screen, what is currently the top screen should be entirely Full screen. Everything in the bottom should either move into a minimap (for the map) or Like the Settings menus into a normal Settings Menu Like all PC Games".

- View (question 5a): "Widen view (Rec.)": same vertical field of view, more world visible left and right. The edges are checked so that no pop-in shows (objects, rooms, pedestrians and shadows appear and fade out of the visible range at the 3DS view's edges today).
- HUD (question 5b): "Anchor to corners (Rec.)": hearts, character picture, studs and other top-screen HUD parts move to the corners of the 16:9 area inside the bars ([§2026-10-29T11:42:22](#ts-20261029-114222)).
- Bottom-screen content (question 6): "As recommended": the map is a minimap in a corner; the characters button, character swap and the fast-travel map are full-window panels clicked with the mouse; pause and Options are PC menus; each its own feature file.
- Layout (questions 7 and 8): "Yes to all (Rec.)": the bottom screen is hidden when its features are on; with all QoL off the layout is exactly today's top-over-bottom window. How the bottom screen is switched: [§2026-10-28T09:48:15](#ts-20261028-094815).

<a id="ts-20261028-093102"></a>

## 2026-10-28T09:31:02+01:00 — Minimap, widest aspect and other bottom pages

Asked of the user by the lead on 2026-10-28; the quoted labels are the user's selections.

- Minimap (QOLMAP-2): "Top-right, round, rotates": a round minimap in the top-right corner, about 1/5 of the screen height, rotating with the camera, zoom like the original bottom-screen map.
- Other bottom pages (QOLBOT-6): "Scanner, Build-it, Shop, Award viewer" (all four) get a PC version, a full-window panel clicked with the mouse, when their QoL feature is on.
- Widest aspect (5c): "16:9": the view widens up to 16:9 only; wider screens get black side bars.

<a id="ts-20261028-093731"></a>

## 2026-10-28T09:37:31+01:00 — Narrow windows, HUD corners, view toggle, minimap content, pause menu and panel look

Asked of the user by the lead on 2026-10-28; the quoted labels are the user's selections. Decides QOLFS-4 to QOLFS-6, QOLMAP-3, QOLBOT-12 and QOLBOT-13.

- Narrow windows (QOLFS-4): "5:3 with bars (Rec.)": windows narrower than 5:3 keep the original 5:3 view with bars above and below.
- HUD corners (QOLFS-5): "Standard corners (Rec.)": hearts and character picture top-left, studs and counters top-right, ability bottom-left, at the corners of the 16:9 area inside the bars ([§2026-10-29T11:42:22](#ts-20261029-114222)); scaled to the window height, in windows narrower than 5:3 to the height of the 5:3 view ([§2026-10-28T22:21:19](#ts-20261028-222119)).
- Toggle of the view (QOLFS-6): "One toggle": the widened view and the corner HUD are one QoL toggle. The minimap is not part of it; it follows the master button ([§2026-10-28T09:48:15](#ts-20261028-094815)).
- Minimap content (QOLMAP-3): "Original content, fixed (Rec.)": the same icons and objective markers as the original map, hidden in cutscenes and panels, fixed zoom.
- PC pause menu (QOLBOT-12): "Mirror + PC Settings (Rec.)": Resume, Options (the original page), Credits, Exit and PC Settings.
- Look (QOLBOT-13): "Original art (Rec.)": the original game's art and fonts for all full-window panels, settings included.

<a id="ts-20261028-094815"></a>

## 2026-10-28T09:48:15+01:00 — The master button switches the bottom screen and the minimap

Asked of the user by the lead on 2026-10-28 in three question rounds (09:37, 09:48 with the lead's clarifying question, 09:50); the quoted labels are the user's selections. Decides QOLBOT-11, QOLBOT-15 to QOLBOT-18 and QOLMAP-4.

- Hiding needs the minimap (QOLBOT-11, 09:37): of the selection "Per panel, needs minimap (Rec.)" the minimap part holds: hiding the bottom screen requires the minimap on. There are no per-panel toggles (QOLBOT-16).
- Panel toggles (QOLBOT-16): the user typed their own answer: "No toggle for seperate Screens. Either bottom is on or off".
- Own toggle for hiding (QOLBOT-17): "Only via master": "No separate switch; hiding happens only when the master button is on Improved."
- The lead's clarifying question "So how is the bottom screen switched?": "Master only": "No toggle at all: Improved hides it, Original shows it"; Custom: QOLBOT-18 below.
- Turning one panel off after hiding (QOLBOT-15): "Locked on (Rec.)". Without per-panel toggles this has no effect.
- Minimap toggle (QOLMAP-4, 09:50): "Master only": "Minimap shows only in Improved, together with the hidden bottom screen." This was not the recommended option (the recommendation was a toggle of its own).
- Bottom screen in Custom (QOLBOT-18, 09:50): "Hidden iff minimap on (Rec.)": "Hidden exactly when the minimap is on, so the map is never lost; your mix controls it through the minimap."

Reading of the lead (not a decision of the user): together these mean the minimap is never on in Custom, so the bottom screen is always shown in Custom; the minimap and the hiding of the bottom screen are Improved only. In Custom only the remaining per-feature switches (so far the widened view with the corner HUD) can differ from Original. The master state itself is stored: [qol-settings.md §2026-10-28T09:53:33](qol-settings.md#ts-20261028-095333).

<a id="ts-20261028-222119"></a>

## 2026-10-28T22:21:19+01:00 — Geometry of the widened view and the corner HUD

Implemented by 20261028-qolmenu-B in `src/qol/view_layout.*` from [§2026-10-28T09:21:33](#ts-20261028-092133), [§2026-10-28T09:31:02](#ts-20261028-093102) and [§2026-10-28T09:37:31](#ts-20261028-093731); its own choices (QOLFS-9) were then put to the user.

- The view takes the window aspect, limited to 5:3 at the narrow end and 16:9 at the wide end, centred in the window. Wider windows get a bar left and right, narrower windows a bar above and below; the aspect to render at is the view rectangle's. Sizes round down, and the odd pixel goes to the right or bottom bar.
- The HUD anchors are the view's top-left, top-right and bottom-left corners; the scale is the view height divided by 240, the original top-screen height, so in windows narrower than 5:3 the HUD scales by the 5:3 view's height; an element keeps its original offset from its corner, scaled.
- Window sizes of zero or less, windows whose view would be under one pixel, and windows below 400x240 are errors; for the window this check is only a safeguard (below).

The user's answers on 2026-10-28; the quoted labels are the user's selections:
- QOLFS-9: "Accept; min 400x240": "Keep B's three choices; the smallest window is 400x240, the 3DS top screen size." The three choices are the rounding, the HUD scale by the view height and the scaled corner offset above.
- Below the minimum: "Window can't shrink below": "The window has a minimum size of 400x240, so a smaller size can't happen. The geometry rejecting it stays only as a safeguard."

<a id="ts-20261029-114222"></a>

## 2026-10-29T11:42:22+01:00 — Widened view above the bottom screen; corner HUD at the 16:9 area

Asked of the user by the lead on 2026-10-29; the quoted labels are the user's selections.

- HUD corners (question 4): "16:9 area corners": the corner HUD anchors to the corners of the 16:9 area inside the bars, not to the window corners. When the area is narrower than 16:9 (between 5:3 and 16:9), the area is the whole widened view. `hudAnchor` anchors to the view's corners ([§2026-10-28T22:21:19](#ts-20261028-222119)), and `qol-view-layout` pins the anchors inside the bars of a 21:9 window.
- Where the widened view goes while the bottom screen shows: "Above bottom screen". While the widened view is on, the picture fills only the area above the bottom screen, where the 5:3 top screen is today. The whole window comes through QOLBOT-1, which is built first. The user's answer of 2026-10-29: "QOLBOT-1 first". QOLBOT-1 no longer waits on QOLFS-1 or the minimap; QOLFS-1 waits on QOLBOT-1.
- Tall windows: "Top screen's rows". The area is the full window width at the rows of today's top screen. Height and position stay the same, and only the width grows.
- HUD canvas until the corner HUD is drawn (QOLFS-2): "Today's rectangle". The 400x240 top HUD canvas stays on today's top rectangle, together with the fades, dialogue and movies drawn on it.

Implemented by 20261029-qolview-A:
- The hook `qol::topViewRegion` (`src/qol/view_hook.*`) is called in `LevelSession::draw`. With the toggle off it returns `originalViewRegion` of today's top rectangle. With it on, it applies the 5:3 to 16:9 rule to the area above the bottom screen.
- The glow pass gets the region's rectangle.
- `layoutInArea` applies the rule to an area of any positive size: the area above the bottom screen of a 400x240 window is only 400x120. The 400x240 minimum check stays in `computeViewLayout`, for the window.
- `qol::hudPartRect` (`src/qol/hud_hook.*`, 20261029-qolmenu-D) gives the window rectangle of a 400x240 canvas holding one HUD part, at the part's corner and the scale of the view height; the port cannot yet draw parts separately (QOLFS-12).
- Implemented by 20261029-qolmenu-F (QOLBOT-1): with `master Improved` `game_hud::screensFor` returns (hook `qol::hiddenScreens`) the 400x240 top picture fitted to the window and centred (the user's answer "5:3 centred", 2026-10-29) and an empty bottom rectangle, so the bottom screen is not drawn and the touch tracker takes no click; Original and Custom are unchanged. The user's answer "Hide anyway" (2026-10-29): the pages of the bottom screen have no input until the panels exist (stand-in QOLBOT-S1).
- Implemented by 20261029-qolhook-C (QOLBOT-20): with the bottom screen hidden, `qol::topViewRegion(width, height, top)` applies the same rule to the whole window (`qol::hiddenTopArea`), so tall windows get the 5:3 view centred with bars above and below, and wide ones the 16:9 view with side bars.
- The process-wide store is `qol::activeSettings`, `setActiveSettings` and `enabled`, default Original.

<a id="ts-20261030-140744"></a>

## 2026-10-30T14:07:44+01:00 — Front end screens follow the hiding

Asked of the user by the lead on 2026-10-30 (round 20261030-qolstores); the quoted label is the user's selection. Decides QOLBOT-19.

- Title screen and front-end menus with the bottom screen hidden (QOLBOT-19), "When the Improved option hides the bottom screen during play, what should the title screen and front-end menus do?": "Hide like in play": the front end hides it too and the top screen fills the window. Not chosen: "Keep both screens", "Hide, menus full-window". Open: its menus need clicks on the top-screen layout or a PC panel; how they are reached is to be proposed to the user.
