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

- 2026-10-28T09:21:35+01:00 — Planner: opened the qol plan. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-28T09:32:53+01:00 — Planner: recorded the user's answers on menu access, content, layout, minimap, aspect and separation. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-28T09:38:37+01:00 — Planner: recorded the user's answers on the master button, window modes, files, HUD corners, toggles and panel look. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-28T09:41:06+01:00 — Planner: recorded Custom and hiding decisions, three new open questions. See [qol-bottom-screen](features/qol-bottom-screen.md).
- 2026-10-28T09:49:01+01:00 — Planner: recorded that the master button alone switches the bottom screen; two new open questions. See [qol-bottom-screen](features/qol-bottom-screen.md).
- 2026-10-28T09:50:53+01:00 — Planner: recorded minimap and bottom-screen decisions; one new open question. See [qol-minimap](features/qol-minimap.md).
- 2026-10-28T09:53:50+01:00 — Planner: recorded that the master state is stored; no open user decision left. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-28T21:33:48+01:00 — Agent 20261028-qol-A (toggle store): built library chase_qol with the master state, feature table and strict qol.cfg, test qol-settings. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-28T21:34:16+01:00 — Agent 20261028-qol-B (qol-separation guard): added the qol-separation guard test with accepting and rejecting fixtures. See [qol-settings-menu](features/qol-settings-menu.md).
