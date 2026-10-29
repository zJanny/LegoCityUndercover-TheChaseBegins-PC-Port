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

- 2026-10-29T09:32:19+01:00 — Lead (20261029-qolset end): merged 20261029-qolset-A (Display tab store and rows), 20261029-qolset-B (view region capability), 20261029-qolset-C (Game tab model) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [qol-settings-menu.md](features/qol-settings-menu.md), [qol-fullscreen.md](features/qol-fullscreen.md)
- 2026-10-29T10:34:11+01:00 — Agent 20261029-qolwire-B (game options): added QolGameOptions over the Options page state, language row now requests the Languages page, test qol-game-options. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-29T10:34:38+01:00 — Agent 20261029-qolwire-A (display tab): added the Display tab to the menu model over display_tab (setDisplayModes, effectiveResolution, clamp to the largest mode, no resolution line while the desktop size is in use) and recorded the user's four answers; open for the next planner: a spec round for the menu drawing (which original art and fonts to reuse). See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-29T10:29:11+01:00 — Agent 20261029-qolwire-C (controls tab): added the pure Controls tab model (waiting for key, assign, cancel, conflict report, reset) with test qol-controls-tab; open flow questions recorded; wiring and file writer remain. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-29T11:11:07+01:00 — Lead (20261029-qolwire end): merged 20261029-qolwire-A (Display tab in the menu model, user answers recorded), 20261029-qolwire-B (Game tab port side), 20261029-qolwire-C (Controls tab model) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [qol-settings-menu](features/qol-settings-menu.md)
- 2026-10-29T11:45:52+01:00 — Agent 20261029-qolview-B (window mode and F11): startup clamp, effective size, borderless fullscreen or windowed, `Fullscreen` F11 binding and the 400x240 minimum in chase-game, test qol-window-apply; the user answered the open questions. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-29T11:47:10+01:00 — Agent 20261029-qolview-C (controls tab): rebind flow in the Controls tab model (refuse, confirmed Reset all, Escape, mouse, two slots with modes, clearing), adapter over readBindings and atomic strict writer with round-trip tests; recorded the art and Language decisions; pad format, comments and defaults open. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-29T11:48:33+01:00 — Agent 20261029-qolview-A (widened view hook): added the active QoL store and the view hook in LevelSession::draw; with the toggle on the 3D view and glow pass use the widened region above the bottom screen, recorded the user's four answers; the config load and the corner HUD remain. See [qol-fullscreen](features/qol-fullscreen.md).
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
