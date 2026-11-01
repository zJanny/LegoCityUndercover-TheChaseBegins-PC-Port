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

### Decision: Escape never closes the game; controls overhaul as a QoL feature (2026-11-01T13:50:47+01:00)
Escape always means back, close the menu or pause, and quitting goes through a menu entry; the controls are overhauled to PC conventions on foot and in vehicles as a QoL feature with its own "PC controls" toggle (on in Improved); the user approved the researched design on 2026-11-01. See [qol-controls.md §2026-11-01T13:50:44](../design/qol-controls.md#ts-20261101-135044) and [qol-controls.md §2026-11-01T13:50:47](../design/qol-controls.md#ts-20261101-135047).

## Next Action

The toggle store, the settings menu models and stores, the master button, the widened view with the corner HUD, the round minimap and the front end panel exist. The user redesigned the PC settings menu on 2026-10-30 (full-window overlay, top tabs, bundled open font, no game art). Next: the packages of [qol-settings-ui](features/qol-settings-ui.md) in its suggested order; the slot page bar (QOLSET-53, measure the art first) and front end row and button clicks (QOLBOT-22) stay. Panels of the scanner, build-it, shop, award viewer, characters and fast-travel map (QOLBOT-2..10) wait for the original screens. The user's 2026-11-01 playtest decided Escape and asked for a controls overhaul, both approved: next the items of [qol-controls](features/qol-controls.md), starting with QOLCTL-8 and QOLCTL-5.

## Session Log

One line per session, newest last; `--fix` keeps the newest 20 and moves older ones to `qol.progress-archive.md`.

- 2026-10-30T22:02:09+01:00 — Lead (recorded the user's answers to the redesign's open points): all nine decided. See [qol-settings-ui](features/qol-settings-ui.md).
- 2026-10-30T22:13:05+01:00 — Agent 20261030-qolui-B (UI primitives draw list): `chase_render` has a plain-type `UiDrawList` (fills, rounded corners, borders, lines, dim, clip stack, recorded text), pure batching into scissor runs and a GL executor `UiDraw` with `.glsl` shaders; test render-ui-batch; the executor still needs a run in a window and text drawing. See [qol-settings-ui](features/qol-settings-ui.md).
- 2026-10-30T22:16:03+01:00 — Agent 20261030-qolui-C (widget and layout model): the layout model `src/qol/ui_layout.*` with test `qol-ui-layout` is in; proportions approved by the user. See [qol-settings-ui](features/qol-settings-ui.md).
- 2026-10-30T22:27:25+01:00 — Agent 20261030-qolui-A (bundled font and UI text): Noto Sans, its OFL text and stb_truetype are bundled with pinned hashes; `chase_ui_font` measures, kerns and lays out text without OpenGL and `UiText` in `chase_render` draws it from a glyph atlas; tests ui-text-core and qol-bundled-font. See [qol-settings-ui](features/qol-settings-ui.md).
- 2026-10-30T22:46:01+01:00 — Lead (20261030-qolui end): merged 20261030-qolui-A (bundled font and text rendering), 20261030-qolui-B (settings menu draw list) and 20261030-qolui-C (widget and layout model, proportions approved by the user) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [qol-settings-ui](features/qol-settings-ui.md)
- 2026-10-30T23:04:40+01:00 — Agent 20261030-qolui2-B (Run UI draw in a GL window and draw text): UiDraw runs in a hidden GL 4.6 window with a readback test of shapes, clips and dim; UiBatch texts are now drawn through UiText between the shape runs in draw-list order; test render-ui-draw-window; the window and text items of the feature are closed. See [qol-settings-ui](features/qol-settings-ui.md).
- 2026-10-30T23:01:21+01:00 — Agent 20261030-qolui2-A (Navigation and focus model): pure model `src/qol/ui_nav.*` over the layout model with one shared focus, tab bar reach and wrap, Esc and right-click layering, slider drag, wheel and pad buttons as plain-type actions, not wired; test qol-ui-nav; See [qol-settings-ui](features/qol-settings-ui.md).
- 2026-10-30T23:23:04+01:00 — Lead (20261030-qolui2 end): merged 20261030-qolui2-B (UiDraw window test and draw-list text) and 20261030-qolui2-A (UiNav navigation and focus model, with the user's answers in the design file) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [qol-settings-ui.md](features/qol-settings-ui.md)
- 2026-11-01T12:18:40+01:00 — Agent 20261101-qolui3-B (Game and Display pages): page models `GamePage` and `DisplayPage` over the existing stores (volume sliders 0 to 100, language selector, resolution dropdown greyed in full screen, resets with confirm, keep-settings outcome), not wired; tests qol-ui-page-game and qol-ui-page-display. See [qol-settings-ui](features/qol-settings-ui.md).
- 2026-11-01T12:18:48+01:00 — Agent 20261101-qolui3-D (swap dialog and keep-settings models): ControlsTab keeps a pending swap with confirmSwap and cancelConflict, SwapDialog with default focus Cancel, KeepSettings with a 15-second countdown, revert of window and stored values and KeepDialog with default focus Keep; not wired, adapter forwarding still open; tests qol-controls-swap and qol-keep-settings. See [qol-settings-ui](features/qol-settings-ui.md)
- 2026-11-01T12:34:28+01:00 — Agent 20261101-qolui3-A (Controls page, prompt input): `UiNav` takes raw prompt input and Delete, rows get cells with hit areas, `ControlsPage` over `ControlsPort` (not wired). See [qol-settings-ui](features/qol-settings-ui.md).
- 2026-11-01T12:25:07+01:00 — Agent 20261101-qolui3-C (Quality of life page and menu painter): `QolPage` (`src/qol/ui_page_qol.*`) and `paintMenu` with the generic dialog (`src/qol/ui_paint.*`) are in, not wired; tests `qol-ui-page-qol` and `qol-ui-paint` at 720p, 1080p and 4K. See [qol-settings-ui](features/qol-settings-ui.md).
- 2026-11-01T13:20:26+01:00 — Agent 20261101-qolui3-F (settings menu wiring): the new menu controller and presenter replace the art-based drawing at the slot page bar and the pause entry; the old drawing and its tests are removed; three follow-ups are open. See [qol-settings-ui](features/qol-settings-ui.md).
- 2026-11-01T13:44:24+01:00 — Lead (20261101-qolui3 end): merged 20261101-qolui3-A (Controls page, prompt keys), 20261101-qolui3-B (Game and Display pages), 20261101-qolui3-C (Quality of life page, menu painter), 20261101-qolui3-D (swap and keep-settings dialogs), 20261101-qolui3-F (new menu wired in, old drawing removed) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [qol-settings-ui.md](features/qol-settings-ui.md)
- 2026-11-01T13:51:33+01:00 — Lead (recorded the user's playtest of the new settings menu): menu confirmed good; Escape never closes the game and the PC controls overhaul recorded, the user then approved the researched controls design and the Escape rules. See [qol-controls](features/qol-controls.md).
- 2026-11-01T14:36:47+01:00 — Lead (recorded the user's approval of the draft help texts of the Game, Display and Controls rows): approved as shown and recorded in the design entry. See [qol-settings-ui](features/qol-settings-ui.md).
- 2026-11-01T15:00:27+01:00 — Agent 20261101-qolctl-C (settings menu clean-up and picture behind it): removed the old art-based menu model and its tests, and the menu is now drawn over the dimmed last frame kept by chase_render. See [qol-settings-ui](features/qol-settings-ui.md).
- 2026-11-01T15:10:18+01:00 — Agent 20261101-qolctl-A (PC controls toggle and on-foot set): the "PC controls" feature (`pc_controls` in `config/qol.cfg`, on in Improved) reads and writes `config/controls.pc.cfg`, created from the built-in on-foot set; Reset restores that set, switching never touches `config/controls.cfg`. See [qol-controls](features/qol-controls.md)
- 2026-11-01T15:22:22+01:00 — Agent 20261101-qolctl-B (Quit game and Escape): the Game tab has a Quit game row with Yes/No (in play it holds only that row), Yes pushes SDL_QUIT; Escape is a fixed second binding of Start in chase-game and never quits. See [qol-controls](features/qol-controls.md)
- 2026-11-01T16:17:35+01:00 — Lead (20261101-qolctl end): merged 20261101-qolctl-A (PC controls toggle and key set), 20261101-qolctl-B (Escape opens the pause menu, Quit game row), 20261101-qolctl-C (dimmed game picture behind the settings menu, old menu model removed) and the help-text recorder through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [qol-controls](features/qol-controls.md), [qol-settings-ui](features/qol-settings-ui.md)
