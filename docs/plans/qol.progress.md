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

## Next Action

S1: the toggle store and the settings menu ([qol-settings-menu](features/qol-settings-menu.md)); nothing else of the plan starts before it. Propose the `qol-separation` guard (QOLSET-7) in the first round. Open questions for the user are marked as such in the feature files and must be answered before the work on them starts.

## Session Log

One line per session, newest last; `--fix` keeps the newest 20 and moves older ones to `qol.progress-archive.md`.

- 2026-10-29T11:52:58+01:00 — Agent 20261029-qolview-D (menu drawing spec): wrote the drawing proposal on the user's "Options-page art" decision (parts and art reused, scaling, font and glyphs, navigation onto the model's events, eleven questions for the user) and two data findings (round menu layout pieces, front-end font); drawing and text check opened as items; awaits the user's answers. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-29T12:12:54+01:00 — Agent 20261029-qolview-D (menu drawing answers): recorded the user's answers to the drawing questions and the Reset all source; the drawing spec is decided and the drawing waits for the port's layout text size fix. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-29T12:25:27+01:00 — Agent 20261029-qolview-B2 (config to active store): the window glue hands the loaded qol.cfg and each F11 write to the active QoL store, so the widened view switches on from the file; test in qol-window-apply. See [qol-fullscreen](features/qol-fullscreen.md).
- 2026-10-29T12:33:28+01:00 — Lead (20261029-qolview end): merged 20261029-qolview-A (widened view hook and settings store), -B (window mode, size and F11 at startup), -B2 follow-up (qol.cfg loaded into the active store), -C (controls file writer and rebind flow), -D (settings menu drawing spec, decided by the user) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. Next by user request: design docs keep only the current version. See docs/plans/features/qol-fullscreen.md, docs/plans/features/qol-settings-menu.md
- 2026-10-29T12:56:11+01:00 — Agent 20261029-designdocs (design docs current only): by the user's request, docs/design now holds only the current version of each decision (rule, checker, merge tools, skills); the three qol design files are rewritten to their current decisions with every decision kept and the links into them fixed. See [qol-settings-menu](features/qol-settings-menu.md) and [docs/design](../design/README.md).
- 2026-10-29T13:49:49+01:00 — Agent 20261029-qolmenu-D (corner HUD placement): added `qol::hudPartRect` and test qol-hud-hook (off, then on); drawing waits for a port capability. See [qol-fullscreen](features/qol-fullscreen.md).
- 2026-10-29T13:52:18+01:00 — Agent 20261029-qolmenu-B (menu text check): added the glyph and box checker and the menu text list with tests; every text has glyphs, two do not fit the name plate and two language names are open. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-29T13:55:59+01:00 — Agent 20261029-qolmenu-C (Controls tab join, pad source): `pad` source in the controls format and writer, SDL key resolver and namer, `QolControlsBindings` writes the file after each change; the tab rows, drawing and pad events remain. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-29T13:58:14+01:00 — Agent 20261029-qolmenu-A (layout text size): read the layout text font path in the original; the size rule is not found yet and the item stays open. See [hud-ui](features/hud-ui.md).
- 2026-10-29T15:00:22+01:00 — Agent 20261029-qolmenu-G (HUD part draw): `Hud::draw` takes a part set and `HudOverlay::drawTop` draws a top canvas on a window rectangle; the hook waits. See [qol-fullscreen](features/qol-fullscreen.md).
- 2026-10-29T14:59:51+01:00 — Agent 20261029-qolmenu-H (Controls rows): the menu model has the Controls tab rows, slot texts, Waiting slot, refuse message and Reset all question over a ControlsPort interface; the user answered the slot and wording questions. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-29T15:07:14+01:00 — Agent 20261029-qolmenu-E (layout text size): found the original's layout text draw: fontSize changes no glyph size, text is drawn at scale 1 and spacing 0; the port's default layout text spacing follows and the font size item is closed. See [hud-ui](features/hud-ui.md).
- 2026-10-29T15:11:24+01:00 — Agent 20261029-qolmenu-F (hide the bottom screen): master Improved hides the bottom screen through `qol::hiddenScreens` (hook in `screensFor`, touch follows), top picture fitted 5:3 and centred, test qol-bottom-hook; panels and widening over the whole window remain. See [qol-bottom-screen](features/qol-bottom-screen.md).
- 2026-10-29T15:30:12+01:00 — Lead (20261029-qolmenu end): merged 20261029-qolmenu-B (menu text check), -D (HUD corner placement, partial), -C (Controls file join and pad source), -A (layout text findings), -G (HUD part draw), -H (Controls tab rows), -E (layout text spacing, Opus replacement) and -F (hiding the bottom screen) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [qol-settings-menu.md](features/qol-settings-menu.md), [qol-fullscreen.md](features/qol-fullscreen.md), [qol-bottom-screen.md](features/qol-bottom-screen.md), [hud-ui.md](features/hud-ui.md)
- 2026-10-29T15:53:52+01:00 — Agent 20261029-qolmenu2-C (menu text re-measure, pad events): re-measured the settings menu texts at layout spacing 0 (none exceeds the name plate or list bar), Invert camera words now On/Off, game-controller buttons feed InputState as pad presses in chase-game, chase-viewer and chase-input-lab; Window mode labels left for the user. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-29T16:00:51+01:00 — Agent 20261029-qolmenu2-A (settings page drawing): round form scene, scale rule, hit areas and pointer, wheel and key mapping in `src/qol/menu_page.*` and `menu_hit.*` with tests qol-menu-hit and qol-menu-page-real-data, over new port layout capabilities; the user answered the tap circle, hover, name plate and Game label questions; the list form join and the window hook remain. See [qol-settings-drawing](features/qol-settings-drawing.md).
- 2026-10-29T16:16:01+01:00 — Agent 20261029-qolmenu2-B (list form follow-up): merged main, list builders on the shared hit area type, readable names in the text check; two lines per bar (the user's answer) stays open. See [qol-settings-drawing](features/qol-settings-drawing.md).
- 2026-10-29T16:25:44+01:00 — Lead (20261029-qolmenu2 end): merged 20261029-qolmenu2-A (settings page round form, scale and hit areas), 20261029-qolmenu2-B (Controls list, notice and question forms), 20261029-qolmenu2-C (menu text re-measure, pad buttons) and the recorder (hub map in game-runtime.md) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [qol-settings-drawing.md](features/qol-settings-drawing.md), [qol-settings-menu.md](features/qol-settings-menu.md)
- 2026-10-29T18:06:07+01:00 — Agent 20261029-qolhook-A (list form join): `controlsListForm` joins the Controls list, notice and Reset all question into `ListForm` (new `overlay`), Yes/No from gametext; the Controls tab now draws its rows. See [qol-settings-drawing](features/qol-settings-drawing.md).
- 2026-10-29T18:04:17+01:00 — Agent 20261029-qolhook-C (widened view over the whole window): `qol::topViewRegion` takes the window size and widens over the whole window while the bottom screen is hidden ; test qol-view-hook extended. See [qol-bottom-screen](features/qol-bottom-screen.md), [qol-fullscreen](features/qol-fullscreen.md)
