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
The minimap has no toggle of its own and shows only in Improved; the bottom screen is hidden exactly when the minimap is on, so Custom always shows it. See [qol-screen-layout.md §2026-10-28T09:48:15](../design/qol-screen-layout.md#ts-20261028-094815).

### Decision: The master state is stored (2026-10-28T09:53:50+01:00)
Original = all off, Improved = all on, per-feature switches only in Custom. See [qol-settings.md §2026-10-28T09:53:33](../design/qol-settings.md#ts-20261028-095333).

## Blockers

None.

### Decision: Settings menu redesigned as a modern PC menu (2026-10-30T21:56:04+01:00)
The whole PC settings menu is redrawn without game assets as a full-window overlay with top tabs, bundled open font, instant settings with a keep-settings confirm for the window, arrows reaching the tabs, swap or cancel on binding conflicts. Replaces the art-based drawing and the Left/Right-turn-rows keys. See [qol-settings-drawing.md §2026-10-30T21:56:04](../design/qol-settings-drawing.md#ts-20261030-215604).

## Next Action

The toggle store, the settings menu models and stores, the master button, the widened view with the corner HUD, the round minimap and the front end panel exist. The user redesigned the PC settings menu on 2026-10-30 (full-window overlay, top tabs, bundled open font, no game art). Next: the packages of [qol-settings-ui](features/qol-settings-ui.md) in its suggested order; the slot page bar (QOLSET-53, measure the art first) and front end row and button clicks (QOLBOT-22) stay. Panels of the scanner, build-it, shop, award viewer, characters and fast-travel map (QOLBOT-2..10) wait for the original screens.

## Session Log

One line per session, newest last; `--fix` keeps the newest 20 and moves older ones to `qol.progress-archive.md`.

- 2026-10-30T17:03:35+01:00 — Lead (recorded the user's rule that arrow keys keep to their axis in the settings menu): design entry added; the user answered the key layout (Left/Right turn rows, Up/Down change values). See [qol-settings-drawing](features/qol-settings-drawing.md)
- 2026-10-30T17:07:04+01:00 — Agent 20261030-qolpanels2-A (front end panel): with Improved the front end shows the darkened top picture with the bottom page as a centred 4:3 panel (`src/qol/front_panel*`, `FrontPresenter::presentBoxes`), the slot page "PC Settings" bar click maps through the panel; row and button clicks stay open; split `qol-settings-tabs` off the settings menu file. See [qol-bottom-screen](features/qol-bottom-screen.md).
- 2026-10-30T17:10:03+01:00 — Agent 20261030-qolpanels2-B (minimap placement and panels): the round minimap now sits under the studs and counters (counters edge measured on real data, 4 unit margin) and is hidden in dialog boxes, the scene change page and the fast-travel map besides the scanner; the pause menu already replaces the picture. See [qol-minimap](features/qol-minimap.md).
- 2026-10-30T17:12:07+01:00 — Agent 20261030-qolpanels2-C (camera kinds, volume clicks): only the gameplay camera widens, a click on the left or right half of a volume row's name plate steps the volume; see [qol-fullscreen.md](features/qol-fullscreen.md).
- 2026-10-30T20:31:45+01:00 — Lead (20261030-qolpanels2 end): merged the recorder (arrow-key rule for menus), 20261030-qolpanels2-A (front-end panel in Improved; row and button clicks still open), -D (pause screen item grid), -B (minimap under the counters, hidden in panels and speech boxes), -C (gameplay-only widening and volume value clicks; HUD corners in 5:3 views decided, not built) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [qol-bottom-screen.md](features/qol-bottom-screen.md), [qol-minimap.md](features/qol-minimap.md), [qol-fullscreen.md](features/qol-fullscreen.md), [qol-settings-drawing.md](features/qol-settings-drawing.md), [ui-menus-pause.md](features/ui-menus-pause.md)
- 2026-10-30T21:01:03+01:00 — Agent 20261030-qolmenu-A (arrow keys by axis): Left/Right and the wheel turn rows, Up/Down change the value (Up raises or goes forward, stops at the ends) on every tab, Controls Up/Down pick the slot; Q8 answer recorded; slot direction left as a question. See [qol-settings-drawing](features/qol-settings-drawing.md)
- 2026-10-30T20:56:12+01:00 — Agent 20261030-qolmenu-C (corner HUD at picture corners): while the cutscene, director or scanner camera shows the 5:3 picture the corner HUD parts are drawn at the picture's corners, using the same camera kind as the view region (`qol::cameraWidens`). See [qol-fullscreen](features/qol-fullscreen.md).
- 2026-10-30T21:03:03+01:00 — Agent 20261030-qolmenu-B (two-line Controls bars): the Controls list shows four bars per page with the action on top and the two slots in a smaller line underneath (`LayoutScene::scales`), one hit area per slot, and the page cache key includes element scales (tests qol-menu-list-page, qol-menu-redraw). See [qol-settings-drawing](features/qol-settings-drawing.md).
- 2026-10-30T21:34:05+01:00 — Lead (20261030-qolmenu end): merged 20261030-qolmenu-C (corner HUD at picture corners), 20261030-qolmenu-B (two-line controls bars), 20261030-qolmenu-A (arrow keys keep to their axis) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [qol-settings-drawing.md](features/qol-settings-drawing.md), [qol-fullscreen.md](features/qol-fullscreen.md)
- 2026-10-30T21:57:13+01:00 — Lead (recorded the user's redesign of the PC settings menu): design entry and open points in [qol-settings-drawing.md](../design/qol-settings-drawing.md), new items in [qol-settings-ui](features/qol-settings-ui.md), old art-based drawing items voided.
- 2026-10-30T22:02:09+01:00 — Lead (recorded the user's answers to the redesign's open points): all nine decided. See [qol-settings-ui](features/qol-settings-ui.md).
- 2026-10-30T22:13:05+01:00 — Agent 20261030-qolui-B (UI primitives draw list): `chase_render` has a plain-type `UiDrawList` (fills, rounded corners, borders, lines, dim, clip stack, recorded text), pure batching into scissor runs and a GL executor `UiDraw` with `.glsl` shaders; test render-ui-batch; the executor still needs a run in a window and text drawing. See [qol-settings-ui](features/qol-settings-ui.md).
- 2026-10-30T22:16:03+01:00 — Agent 20261030-qolui-C (widget and layout model): the layout model `src/qol/ui_layout.*` with test `qol-ui-layout` is in; proportions approved by the user. See [qol-settings-ui](features/qol-settings-ui.md).
- 2026-10-30T22:27:25+01:00 — Agent 20261030-qolui-A (bundled font and UI text): Noto Sans, its OFL text and stb_truetype are bundled with pinned hashes; `chase_ui_font` measures, kerns and lays out text without OpenGL and `UiText` in `chase_render` draws it from a glyph atlas; tests ui-text-core and qol-bundled-font. See [qol-settings-ui](features/qol-settings-ui.md).
- 2026-10-30T22:46:01+01:00 — Lead (20261030-qolui end): merged 20261030-qolui-A (bundled font and text rendering), 20261030-qolui-B (settings menu draw list) and 20261030-qolui-C (widget and layout model, proportions approved by the user) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [qol-settings-ui](features/qol-settings-ui.md)
- 2026-10-30T23:04:40+01:00 — Agent 20261030-qolui2-B (Run UI draw in a GL window and draw text): UiDraw runs in a hidden GL 4.6 window with a readback test of shapes, clips and dim; UiBatch texts are now drawn through UiText between the shape runs in draw-list order; test render-ui-draw-window; the window and text items of the feature are closed. See [qol-settings-ui](features/qol-settings-ui.md).
- 2026-10-30T23:01:21+01:00 — Agent 20261030-qolui2-A (Navigation and focus model): pure model `src/qol/ui_nav.*` over the layout model with one shared focus, tab bar reach and wrap, Esc and right-click layering, slider drag, wheel and pad buttons as plain-type actions, not wired; test qol-ui-nav; See [qol-settings-ui](features/qol-settings-ui.md).
- 2026-10-30T23:23:04+01:00 — Lead (20261030-qolui2 end): merged 20261030-qolui2-B (UiDraw window test and draw-list text) and 20261030-qolui2-A (UiNav navigation and focus model, with the user's answers in the design file) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [qol-settings-ui.md](features/qol-settings-ui.md)
- 2026-11-01T12:34:28+01:00 — Agent 20261101-qolui3-A (Controls page, prompt input): `UiNav` takes raw prompt input and Delete, rows get cells with hit areas, `ControlsPage` over `ControlsPort` (not wired). See [qol-settings-ui](features/qol-settings-ui.md).
- 2026-11-01T12:25:07+01:00 — Agent 20261101-qolui3-C (Quality of life page and menu painter): `QolPage` (`src/qol/ui_page_qol.*`) and `paintMenu` with the generic dialog (`src/qol/ui_paint.*`) are in, not wired; tests `qol-ui-page-qol` and `qol-ui-paint` at 720p, 1080p and 4K. See [qol-settings-ui](features/qol-settings-ui.md).
