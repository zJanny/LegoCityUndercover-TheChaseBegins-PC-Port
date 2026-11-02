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

- Minimap (QOLMAP-2): "Top-right, round, rotates": a round minimap in the top-right corner, about 1/5 of the screen height, rotating with the camera, zoom like the original bottom-screen map. Placement (answered 2026-10-30, "Below the counters (Recommended)"): still top-right, moved down so it starts under the studs and counters, with a small margin (not chosen: "Bottom-right corner", "Agent proposes mock-up", "Keep top-right, add margin"); at 16:9 the flush top-right circle overlapped the corner HUD. Speech (answered 2026-10-30T17:33:45+01:00, "Yes, hide it too"): any dialogue on screen, including a character's speech box, hides the minimap.
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
- Minimap and bottom screen (QOLMAP-4, QOLBOT-18), changed by the user on 2026-11-02 after a playtest bug (with the widened view on, switching PC controls off from Improved brought the bottom screen back). The user: "Custom should be a collection of changed settings that differ from the preset. And widened screen is one setting, if that widened screen setting is turned on, everything that needs to change for the widened screen to work needs to also be on. So if if it is toggled, the bottom screen disappears everything is moved to the top screen. Like the minimap we already have". So whenever the Widened view setting is on, in any preset including Custom, the bottom screen is hidden and its content (the minimap) is moved to the top screen; with it off the bottom screen is shown and there is no minimap. Improved has the widened view on, so it hides the bottom screen as before. The master state itself is stored: [qol-settings.md §2026-10-28T09:53:33](qol-settings.md#ts-20261028-095333).

<a id="ts-20261028-222119"></a>

## 2026-10-28T22:21:19+01:00 — Geometry of the widened view and the corner HUD

Implemented by 20261028-qolmenu-B in `src/qol/view_layout.*` from [§2026-10-28T09:21:33](#ts-20261028-092133), [§2026-10-28T09:31:02](#ts-20261028-093102) and [§2026-10-28T09:37:31](#ts-20261028-093731); its own choices (QOLFS-9) were then put to the user.

- The view takes the window aspect, limited to 5:3 at the narrow end and 16:9 at the wide end, centred in the window. Wider windows get a bar left and right, narrower windows a bar above and below; the aspect to render at is the view rectangle's. Sizes round down, and the odd pixel goes to the right or bottom bar.
- The HUD anchors are the view's top-left, top-right and bottom-left corners; the scale is the view height divided by 240, the original top-screen height, so in windows narrower than 5:3 the HUD scales by the 5:3 view's height; an element keeps its original offset from its corner, scaled.
- Window sizes of zero or less, windows whose view would be under one pixel, and windows below 400x240 are errors; for the window this check is only a safeguard (below).

The user's answers on 2026-10-28; the quoted labels are the user's selections:
- QOLFS-9: "Accept; min 400x240": "Keep B's three choices; the smallest window is 400x240, the 3DS top screen size." The three choices are the rounding, the HUD scale by the view height and the scaled corner offset above.
- Below the minimum: "Window can't shrink below": "The window has a minimum size of 400x240, so a smaller size can't happen. The geometry rejecting it stays only as a safeguard."
- A `--window` size below 400x240 (QOLFS-23, asked 2026-11-02): "Error below 400x240": the start stops with an error naming the option. The option itself still parses widths from 200; the check is in the QoL window code.

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
- Implemented by 20261030-qolstores-A (QOLFS-13, QOLFS-2): `qol::drawHudParts` (`src/qol/hud_parts_hook.*`) draws the five parts, each on a cleared 400x240 canvas, at its `hudPartRect` (Counters as the super brick display, Ability as the camera lock icon); the head icon and the scanner overlay stay on the top rectangle.
- Which cameras widen (the user's answer of 2026-10-30, round 20261030-qolstores, to "The gameplay camera widens to 16:9. Should the cutscene, director and scanner cameras widen too, or stay at the 3DS 5:3 shape?"): "Only gameplay widens": the cutscene, director and scanner cameras keep the 5:3 shape, with bars at the sides.
- Corner HUD while the 5:3 picture is shown (the user's answer of 2026-10-30T20:13:28+01:00, round 20261030-qolpanels2, to "During director and scanner views the corner HUD parts stay at the corners of the wide window, outside the 5:3 picture. Should they move to the picture's corners then?"): "Move to picture corners": while the 5:3 picture is shown, the corner HUD sits on the picture's corners, as in the original. Implemented by 20261030-qolmenu-C (QOLFS-19): `qol::hudPartRect` and `hudPartDraws` take the camera kind of `topViewRegion` and give the picture rectangle for a non-gameplay camera.
- Implemented by 20261029-qolmenu-F (QOLBOT-1): with `master Improved` `game_hud::screensFor` returns (hook `qol::hiddenScreens`) the 400x240 top picture fitted to the window and centred (the user's answer "5:3 centred", 2026-10-29) and an empty bottom rectangle, so the bottom screen is not drawn and the touch tracker takes no click; Original and Custom are unchanged. The user's answer "Hide anyway" (2026-10-29): the pages of the bottom screen have no input until the panels exist (stand-in QOLBOT-S1).
- Implemented by 20261029-qolhook-C (QOLBOT-20): with the bottom screen hidden, `qol::topViewRegion(width, height, top)` applies the same rule to the whole window (`qol::hiddenTopArea`), so tall windows get the 5:3 view centred with bars above and below, and wide ones the 16:9 view with side bars.
- The process-wide store is `qol::activeSettings`, `setActiveSettings` and `enabled`, default Original.

<a id="ts-20261030-140744"></a>

## 2026-10-30T14:07:44+01:00 — Front end screens follow the hiding

Asked of the user by the lead on 2026-10-30 (round 20261030-qolstores); the quoted label is the user's selection. Decides QOLBOT-19.

- Title screen and front-end menus with the bottom screen hidden (QOLBOT-19), "When the Improved option hides the bottom screen during play, what should the title screen and front-end menus do?": "Hide like in play": the front end hides it too and the top screen fills the window. Not chosen: "Keep both screens", "Hide, menus full-window".
- How they are reached (answered 2026-10-30 after the user chose "Agent proposes first" and approved the proposal): with Improved the top screen fills the window; every front-end page with bottom-screen content (language list, prompts, autosave notice, save slot page, delete confirm, mission card, boot and saving notices) shows its 320x240 page as a panel centred over the top picture, clicked with the mouse (the window point is mapped back to bottom-canvas pixels). A click on a list row selects it and presses A; a click on a button picture (A, B, Y) is that button; a click elsewhere does nothing. Keys keep working (the flow still gets `FrontInput`). The panel appears and disappears with its page, no manual switching; logos and movies show no panel; subtitles become a caption at the movie's bottom edge. Original mode is unchanged.
  - "Approve this general rule?": "Approve centred panel (Recommended)" (not chosen: "Small corner overlay").
  - "Panel size and top picture?": "Larger, art dimmed (Recommended)": the panel is scaled up as far as it fits keeping 4:3 and the top picture is darkened behind it (not chosen: "Larger, art sharp", "Native 4:3 size, art sharp").
  - "What does a click on a list row do?": "Select and confirm (Recommended)" (not chosen: "Select, then confirm").
  - Language list row width (QOLBOT-24, answered 2026-11-02), the list has no row pictures: "Full-width band": a click anywhere in the full-width 24-pixel band around a text line selects that row and presses A (not chosen: text width only).
  - "Where does the PC Settings bar go in Improved?": "Same place as QoL off (Recommended)": under the slots inside the panel, one layout for all modes ([qol-settings.md §2026-10-29T19:34:22](qol-settings.md#ts-20261029-193422)).
  - "Should a mouse click anywhere skip the fusion movie and the mission card?": "Yes, click skips (Recommended)": a left click works like the skip button, only where a button already skips (not on a first run, where the original does not allow it) (not chosen: "No, keys only").
  - "Boot ('Preparing') and 'Saving Game' notices with Improved?": "Alert panel (Recommended)": the notice page is shown as the centred panel like other front-end pages (not chosen: "Spinner only").
  - "Intro movie subtitles at the movie's bottom edge?": "Original bubble, moved (Recommended)": the game's own subtitle bubble art drawn over the bottom edge of the movie (not chosen: "Plain caption strip").
  - Amounts (QOLBOT-23, answered 2026-10-30T17:12:58+01:00), "Is that right?": "Confirm as built": the top picture at half brightness, fitted 5:3 in the window like in play; the 4:3 bottom panel fitted to the whole window, centred over the picture (in a 4:3 window it covers the picture).

<a id="ts-20261102-154207"></a>

## 2026-11-02T15:42:07+01:00 — Rooms that only the widened view reaches are loaded too

Asked of the user by the lead on 2026-11-02 (round 20261102-qolctl2); the quoted label is the user's selection. Decides QOLFS-20.

- "Should the widened view also load the rooms only the 16:9 strip reaches, which would change the game's room streaming, only with the widened view on?": "Yes, load them". With the widened view on, the game also requests the rooms that the 16:9 frustum reaches (they are outside the current room's neighbours and preloads, mostly far in the fog); it is a QoL option behind the widened view's toggle (the one toggle of the view and the corner HUD). With the toggle off the room streaming stays the original's. Findings: [view-edges.md §2026-11-02T15:16:57](../re/view-edges.md#ts-20261102-151657).
- Open: how many rooms and from which depth (the scan found them mostly beyond 208 m); to be proposed with the implementation.
