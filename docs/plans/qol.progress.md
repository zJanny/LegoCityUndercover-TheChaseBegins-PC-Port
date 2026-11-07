# Progress — Quality-of-life features

## Status

- Overall: IN_PROGRESS
- Current Step: S1
- Last Updated: 2026-10-28T09:53:50+01:00
- Plan: docs/plans/qol.md

## Decisions

### Decision: Settings menu, master toggle and the two rules (2026-10-28T09:21:35+01:00)
Off by default; master toggle with three states in `config/qol.cfg`; menu from the title screen and an always shown "PC Settings" pause entry; tabs Game (mirrored original options), Display, Controls, Quality of life; `qol.cfg` like `controls.cfg`. See [qol-settings.md §2026-10-28T09:32:07](../design/qol-settings.md#ts-20261028-093207).

### Decision: Full screen and bottom-screen content (2026-10-28T09:21:35+01:00)
Widened view with the same vertical field of view, corner HUD, minimap and mouse-clicked panels, bottom screen hidden when its features are on. See [qol-screen-layout.md §2026-10-28T09:21:33](../design/qol-screen-layout.md#ts-20261028-092133).

### Decision: Separate source code (2026-10-28T09:24:08+01:00)
The user's third rule: QoL code is separate from the port's code. See [qol-settings.md §2026-10-28T09:24:08](../design/qol-settings.md#ts-20261028-092408).

### Decision: Reaching the menu, minimap, aspect, other pages (2026-10-28T09:32:53+01:00)
Title plus always shown "PC Settings" pause entry (approved exception); original options mirrored; round top-right rotating minimap; widening up to 16:9; scanner, build-it, shop and award viewer panels; separation rule confirmed. See [qol-settings.md §2026-10-28T09:32:07](../design/qol-settings.md#ts-20261028-093207) and [qol-screen-layout.md §2026-10-28T09:31:02](../design/qol-screen-layout.md#ts-20261028-093102).

### Decision: Master button, files, HUD corners, toggles, panels (2026-10-28T09:38:37+01:00)
Master button cycles all three states; windowed, borderless and F11; bindings in `controls.cfg`, the rest in `qol.cfg`; "PC Settings" last pause entry; 5:3 bars in narrow windows; standard HUD corners; one toggle for view and HUD; original minimap content at fixed zoom; one toggle per panel; original art for all panels. See [qol-settings.md §2026-10-28T09:37:29](../design/qol-settings.md#ts-20261028-093729) and [qol-screen-layout.md §2026-10-28T09:37:31](../design/qol-screen-layout.md#ts-20261028-093731).

### Decision: Custom and hiding the bottom screen (2026-10-28T09:41:06+01:00)
Custom restores the last own mix; hiding the bottom screen turns every panel toggle on (QOLBOT-11 corrected in part). See [qol-settings.md §2026-10-28T09:40:40](../design/qol-settings.md#ts-20261028-094040) and [qol-screen-layout.md §2026-10-28T09:48:15](../design/qol-screen-layout.md#ts-20261028-094815).

### Decision: No per-panel toggles (2026-10-28T09:49:01+01:00)
The master button alone switches the bottom screen and its panels (Improved hides it, Original shows it); QOLBOT-11 and the "hiding forces panels on" decision are superseded. See [qol-screen-layout.md §2026-10-28T09:48:15](../design/qol-screen-layout.md#ts-20261028-094815).

### Decision: Minimap and bottom screen follow the master (2026-10-28T09:50:53+01:00)
The minimap has no toggle of its own; the bottom screen is hidden and the minimap shown exactly when the widened view is on, in every preset (changed by the user on 2026-11-02). See [qol-screen-layout.md §2026-10-28T09:48:15](../design/qol-screen-layout.md#ts-20261028-094815).

### Decision: The master state is stored (2026-10-28T09:53:50+01:00)
Original = all off, Improved = all on, per-feature switches only in Custom. See [qol-settings.md §2026-10-28T09:53:33](../design/qol-settings.md#ts-20261028-095333).

## Blockers

None.

### Decision: Settings menu redesigned as a modern PC menu (2026-10-30T21:56:04+01:00)
The whole PC settings menu is redrawn without game assets as a full-window overlay with top tabs, bundled open font, instant settings with a keep-settings confirm for the window, arrows reaching the tabs, swap or cancel on binding conflicts. Replaces the art-based drawing and the Left/Right-turn-rows keys. See [qol-settings-drawing.md §2026-10-30T21:56:04](../design/qol-settings-drawing.md#ts-20261030-215604).

### Decision: Escape never closes the game; controls overhaul as a QoL feature (2026-11-01T13:50:47+01:00)
Escape always means back, close the menu or pause, and quitting goes through a menu entry; the controls are overhauled to PC conventions on foot and in vehicles as a QoL feature with its own "PC controls" toggle (on in Improved); the user approved the researched design on 2026-11-01. See [qol-controls.md §2026-11-01T13:50:44](../design/qol-controls.md#ts-20261101-135044) and [qol-controls.md §2026-11-01T13:50:47](../design/qol-controls.md#ts-20261101-135047).

## Next Action

The toggle store, the settings menu models and stores, the master button, the widened view with the corner HUD, the round minimap and the front end panel exist. The user redesigned the PC settings menu on 2026-10-30 (full-window overlay, top tabs, bundled open font, no game art). Next: the packages of [qol-settings-ui](features/qol-settings-ui.md) in its suggested order; the slot page bar (QOLSET-53, measure the art first) and front end row and button clicks (QOLBOT-22) stay. Panels of the scanner, build-it, shop, award viewer, characters and fast-travel map (QOLBOT-2..10) wait for the original screens. The user's 2026-11-01 playtest decided Escape and asked for a controls overhaul, both approved: next the items of [qol-controls](features/qol-controls.md), starting with QOLCTL-8 and QOLCTL-5.

## Session Log

One line per session, newest last; `--fix` keeps the newest 20 and moves older ones to `qol.progress-archive.md`.


- 2026-11-01T15:10:18+01:00 — Agent 20261101-qolctl-A (PC controls toggle and on-foot set): the "PC controls" feature (`pc_controls` in `config/qol.cfg`, on in Improved) reads and writes `config/controls.pc.cfg`, created from the built-in on-foot set; Reset restores that set, switching never touches `config/controls.cfg`. See [qol-controls](features/qol-controls.md)
- 2026-11-01T15:22:22+01:00 — Agent 20261101-qolctl-B (Quit game and Escape): the Game tab has a Quit game row with Yes/No (in play it holds only that row), Yes pushes SDL_QUIT; Escape is a fixed second binding of Start in chase-game and never quits. See [qol-controls](features/qol-controls.md)
- 2026-11-01T16:17:35+01:00 — Lead (20261101-qolctl end): merged 20261101-qolctl-A (PC controls toggle and key set), 20261101-qolctl-B (Escape opens the pause menu, Quit game row), 20261101-qolctl-C (dimmed game picture behind the settings menu, old menu model removed) and the help-text recorder through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [qol-controls](features/qol-controls.md), [qol-settings-ui](features/qol-settings-ui.md)
- 2026-11-02T14:56:01+01:00 — Agent 20261102-qolctl2-C (Escape on pause pages): Escape goes back one page (acts as B) on the pause screen's Options and Languages pages through the read-only `LevelSession::pausePage` and `runtime::routeEscape`; main wheel and play keep Start; test escape-pause-pages. See [qol-controls](features/qol-controls.md).
- 2026-11-02T14:58:24+01:00 — Agent 20261102-qolctl2-B (mouse camera): with PC controls on mouse X feeds the follow camera's shoulder turn as an analog R value, the cursor is captured in play, released by the pause screen and the settings menu, and a hidden pointer releases it on the lower screen (the user's answers of 2026-11-02); test qol-pc-mouse-camera. See [qol-controls](features/qol-controls.md).
- 2026-11-02T15:06:12+01:00 — Agent 20261102-qolctl2-D (front end row and button clicks): every front end page with rows or A/B/Y pictures exposes them in bottom-canvas pixels (`FrontEnd::targets()`); with Improved a row click selects and presses A, a picture click is that button, other clicks in the panel do nothing; Original and Custom unchanged; one question for the user about the language list rows is open. See [qol-bottom-screen](features/qol-bottom-screen.md)
- 2026-11-02T15:17:41+01:00 — Agent 20261102-qolctl2-E (view edge research): research of the widened view's edges on the level data (record FOV and rooms only the strip reaches), test qol-view-edges-real-data, two items closed and one question for the user opened. See [qol-fullscreen](features/qol-fullscreen.md).
- 2026-11-02T15:21:07+01:00 — Agent 20261102-qolctl2-F (keep points, window minimum): every frame end of chase-game keeps the frame for the picture behind the menu and a guard test now holds it; the 400x240 window minimum holds from creation, also for an explicit `--window` size, with a test. See [qol-settings-ui](features/qol-settings-ui.md) and [qol-fullscreen](features/qol-fullscreen.md).
- 2026-11-02T15:48:35+01:00 — Agent 20261102-qolctl2-A (vehicle keys and add missing keys): the PC controls switch to a vehicle set (`controls.pc.vehicle.cfg`) while the player sits, an existing `controls.pc.cfg` keeps its bindings and gets only missing actions, and the user's two answers are recorded. See [qol-controls](features/qol-controls.md).
- 2026-11-02T17:11:12+01:00 — Lead (20261102-qolctl2 end): merged 20261102-qolctl2-B (mouse camera), -E (view edge research), -C (Escape on pause pages), -D (front end clicks), -F (keep points, window minimum) and -A (vehicle keys, add missing keys) through tools/merge_agent.py with all tests passing; the user's answers of this session recorded in docs/design; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [qol-controls.md](features/qol-controls.md), [qol-fullscreen.md](features/qol-fullscreen.md), [qol-bottom-screen.md](features/qol-bottom-screen.md)
- 2026-11-02T18:22:59+01:00 — Agent 20261102-qolmenu-A (PC Settings bar selectable): Down from the last slot focuses the slot page bar, Up returns, A opens the menu, the focus stays after closing; new `SlotPageEntry` (`src/qol/slot_page_entry.*`), one hook line, port capability `FrontEnd::setSlotsUnselected`. See [qol-settings-drawing](features/qol-settings-drawing.md).
- 2026-11-02T18:33:37+01:00 — Agent 20261102-qolmenu-B (Controls tab set switch, rebinding help, PC controls off bug): the On foot / Vehicle switch with a hint line, the rewritten rebinding section of docs/controls.md, and the widened view alone hides the bottom screen in every preset (the user rule of 2026-11-02). See [qol-controls](features/qol-controls.md).
- 2026-11-02T18:40:06+01:00 — Agent 20261102-qolmenu-C (movie subtitles over the picture): with Improved a movie in the front end or a level shows no panel and no blue background; the original subtitle bubble is drawn over the lower part of the fitted picture, level movies keep the 5:3 picture with bars and hide the minimap; done, the placement answered "Keep as built". See [qol-bottom-screen](features/qol-bottom-screen.md).
- 2026-11-02T18:45:11+01:00 — Agent 20261102-qolmenu-E (view distance): the View distance slider (Display tab, x1.0 to x4.0 in quarter steps, `view_distance` in `config/qol.cfg`, greyed out at x1.0 in Original) scales the far plane of every camera, the fog and the pedestrian distances; room loading, traffic and the help text are open; the hook list moved to its own feature file and F11 no longer reverts menu changes. See [qol-view-distance](features/qol-view-distance.md).
- 2026-11-02T21:51:25+01:00 — Lead (20261102-qolmenu end): merged 20261102-qolmenu-A (PC Settings bar selectable on the slot page), the audit (README state line, missing QoL tests), 20261102-qolmenu-B (Controls tab on-foot and vehicle set switch, widened-view rule), 20261102-qolmenu-C (movie subtitles over the picture in Improved) and 20261102-qolmenu-E (View distance slider; docs fixes and main merges by stand-ins) through tools/merge_agent.py with all tests passing; worker D was the docs audit brief, run as 20261102-qolmenu-audit; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [qol-controls](features/qol-controls.md), [qol-bottom-screen](features/qol-bottom-screen.md), [qol-view-distance](features/qol-view-distance.md); user docs audit: done
- 2026-11-02T22:08:12+01:00 — Agent 20261102-qolmore-B (front-end pages as panels): a page rule table decides the panel per front-end screen (logos, movies and black moments none, loading with a small spinning icon), click skip open. See [qol-bottom-screen](features/qol-bottom-screen.md).
- 2026-11-02T22:18:58+01:00 — Agent 20261102-qolmore-C (view distance x20): the slider reaches x20.0 (0.25 steps to x4.0, whole steps above, 29 positions); hooks checked at x20; rooms and traffic left for the next round. See [qol-view-distance](features/qol-view-distance.md).
- 2026-11-02T22:24:30+01:00 — Agent 20261102-qolmore-A (mouse sensitivity slider): the Mouse sensitivity slider on the Controls tab replaces the fixed mouse turn speed; the keep-held-keys rebind was not started. See [qol-controls](features/qol-controls.md).
- 2026-11-02T22:47:11+01:00 — Lead (20261102-qolmore end): merged 20261102-qolmore-B (front-end pages in the Improved bottom panel, Loading spinner), 20261102-qolmore-C (View distance maximum x20, coarser steps above x4), 20261102-qolmore-A (Mouse sensitivity slider, reset and help text) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [qol-bottom-screen.md](features/qol-bottom-screen.md), [qol-view-distance.md](features/qol-view-distance.md), [qol-controls.md](features/qol-controls.md)
- 2026-11-03T08:56:42+01:00 — Agent 20261103-large-E (room loading by view): with the widened view on the room request adds the rooms of the 16:9 strip, and above x1.0 every room within 260 m times the View distance factor (the user's "Strip only" and "Radial", 2026-11-03); one hook line, original streaming with both off; at x20 all 43 rooms of Cherry Tree Hills load, draw calls triple, memory and frame time stay level. See [qol-view-distance](features/qol-view-distance.md), [qol-fullscreen](features/qol-fullscreen.md).
