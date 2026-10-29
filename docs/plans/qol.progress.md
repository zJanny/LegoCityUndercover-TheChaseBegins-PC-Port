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
Custom restores the last own mix; hiding the bottom screen turns every panel toggle on (QOLBOT-11 corrected in part). See [qol-settings.md §2026-10-28T09:40:40](../design/qol-settings.md#ts-20261028-094040) and [qol-screen-layout.md §2026-10-28T09:40:42](../design/qol-screen-layout.md#ts-20261028-094042).

### Decision: No per-panel toggles (2026-10-28T09:49:01+01:00)
The master button alone switches the bottom screen and its panels (Improved hides it, Original shows it); QOLBOT-11 and the "hiding forces panels on" decision are superseded. See [qol-screen-layout.md §2026-10-28T09:48:15](../design/qol-screen-layout.md#ts-20261028-094815).

### Decision: Minimap and bottom screen follow the master (2026-10-28T09:50:53+01:00)
The minimap has no toggle of its own and shows only in Improved; the bottom screen is hidden exactly when the minimap is on, so Custom always shows it. See [qol-screen-layout.md §2026-10-28T09:50:34](../design/qol-screen-layout.md#ts-20261028-095034).

### Decision: The master state is stored (2026-10-28T09:53:50+01:00)
Original = all off, Improved = all on, per-feature switches only in Custom. See [qol-settings.md §2026-10-28T09:53:33](../design/qol-settings.md#ts-20261028-095333).

## Blockers

None.

## Next Action

S1: the toggle store and the settings menu ([qol-settings-menu](features/qol-settings-menu.md)); nothing else of the plan starts before it. Propose the `qol-separation` guard (QOLSET-7) in the first round. Open questions for the user are marked as such in the feature files and must be answered before the work on them starts.

## Session Log

One line per session, newest last; `--fix` keeps the newest 20 and moves older ones to `qol.progress-archive.md`.

- 2026-10-28T09:53:50+01:00 — Planner: recorded that the master state is stored; no open user decision left. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-28T21:33:48+01:00 — Agent 20261028-qol-A (toggle store): built library chase_qol with the master state, feature table and strict qol.cfg, test qol-settings. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-28T21:34:16+01:00 — Agent 20261028-qol-B (qol-separation guard): added the qol-separation guard test with accepting and rejecting fixtures. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-28T21:43:23+01:00 — Lead (recorded): the user approved the `config/qol.cfg` key names ("Keep these names"): `master` with Original, Improved, Custom; snake_case feature keys with on or off. See [qol-settings §2026-10-28T21:43:23](../design/qol-settings.md#ts-20261028-214323).
- 2026-10-28T21:55:52+01:00 — Lead (20261028-qol end): merged 20261028-qol-A (toggle store, library chase_qol and config/qol.cfg), 20261028-qol-B (qol-separation guard) and the recorder branch (approved qol.cfg key names) through tools/merge_agent.py with all tests passing; C (settings menu model) dropped by the user until A is merged; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [qol-settings-menu](features/qol-settings-menu.md)
- 2026-10-28T22:20:41+01:00 — Lead (recorded): the user chose to store window mode and resolution in `config/qol.cfg` as `window_mode` and `resolution`; value formats open. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-28T22:21:14+01:00 — Agent 20261028-qolmenu-B (view geometry): added pure window-to-view and corner-HUD geometry in src/qol with test qol-view-layout, enforcing the 400x240 minimum window the user chose; wiring left. See [qol-fullscreen](features/qol-fullscreen.md).
- 2026-10-28T22:38:57+01:00 — Agent 20261028-qolmenu-A (settings menu model): added the drawing-free menu model and test qol-menu-model, then applied the user's answers (feature rows, wrap); drawing and hooks remain. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-28T23:13:22+01:00 — Agent 20261028-qolmenu-A (settings menu model): tabs of the menu model now wrap like the rows, per the user's answer. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-28T23:19:32+01:00 — Lead (20261028-qolmenu end): merged 20261028-qolmenu-A (settings menu model with the user's Custom switch, row and tab wrap answers), 20261028-qolmenu-B (widened view and HUD corner geometry with the 400x240 minimum) and the recorder branch (qol.cfg window_mode and resolution keys) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [qol-settings-menu.md](features/qol-settings-menu.md), [qol-fullscreen.md](features/qol-fullscreen.md); user docs audit: done
- 2026-10-29T09:14:22+01:00 — Agent 20261029-qolset-B (render view region): added the plain-type view region (rectangle and aspect, black clear outside) to chase_render with test render-viewport-rect; the default path is unchanged. See [qol-fullscreen](features/qol-fullscreen.md).
- 2026-10-29T09:07:03+01:00 — Agent 20261029-qolset-C (Game tab model): added the GameOptions interface and GameTab model with test qol-game-tab. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-29T09:14:39+01:00 — Agent 20261029-qolset-A (display tab store): added `window_mode` and `resolution` to the strict qol.cfg store and a pure Display row model with test qol-display-tab; defaults (fullscreen, desktop size), old files and the SDL mode list per the user, wiring and F11 remain. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-29T09:32:19+01:00 — Lead (20261029-qolset end): merged 20261029-qolset-A (Display tab store and rows), 20261029-qolset-B (view region capability), 20261029-qolset-C (Game tab model) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [qol-settings-menu.md](features/qol-settings-menu.md), [qol-fullscreen.md](features/qol-fullscreen.md)
- 2026-10-29T10:34:11+01:00 — Agent 20261029-qolwire-B (game options): added QolGameOptions over the Options page state, language row now requests the Languages page, test qol-game-options. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-29T10:34:38+01:00 — Agent 20261029-qolwire-A (display tab): added the Display tab to the menu model over display_tab (setDisplayModes, effectiveResolution, clamp to the largest mode, no resolution line while the desktop size is in use) and recorded the user's four answers; open for the next planner: a spec round for the menu drawing (which original art and fonts to reuse). See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-29T10:29:11+01:00 — Agent 20261029-qolwire-C (controls tab): added the pure Controls tab model (waiting for key, assign, cancel, conflict report, reset) with test qol-controls-tab; open flow questions recorded; wiring and file writer remain. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-29T11:11:07+01:00 — Lead (20261029-qolwire end): merged 20261029-qolwire-A (Display tab in the menu model, user answers recorded), 20261029-qolwire-B (Game tab port side), 20261029-qolwire-C (Controls tab model) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [qol-settings-menu](features/qol-settings-menu.md)
- 2026-10-29T11:45:52+01:00 — Agent 20261029-qolview-B (window mode and F11): startup clamp, effective size, borderless fullscreen or windowed, `Fullscreen` F11 binding and the 400x240 minimum in chase-game, test qol-window-apply; the user answered the open questions. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-29T11:47:10+01:00 — Agent 20261029-qolview-C (controls tab): rebind flow in the Controls tab model (refuse, confirmed Reset all, Escape, mouse, two slots with modes, clearing), adapter over readBindings and atomic strict writer with round-trip tests; recorded the art and Language decisions; pad format, comments and defaults open. See [qol-settings-menu](features/qol-settings-menu.md).
