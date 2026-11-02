# QoL settings menu and toggles

## 1. Scope

The settings menu of the PC port and the toggle store of all QoL features: the master toggle (Original, Custom, Improved), one toggle per feature, window mode and resolution, key bindings, and `config/qol.cfg`. Opened from the title screen and from an always shown "PC Settings" entry of the pause menu, with no extra key; tabs Game, Display, Controls and Quality of life; mouse, keyboard and gamepad. Nothing else of the `qol` plan may start before this exists. Plan: [qol.md](../qol.md).

## 2. Status

IN_PROGRESS, opened 2026-10-28T09:21:35+01:00. The toggle store (QOLSET-1), the drawing-free menu model, the Game tab model, its port side `QolGameOptions` (QOLSET-21) and its wiring (QOLSET-9, QOLSET-24), the Controls tab model and its join `QolControlsBindings` (QOLSET-4) and the Display tab store (`window_mode`, `resolution`), row model and its place in the menu model, with the clamp to the largest mode (QOLSET-3, QOLSET-19 model parts) exist in `src/qol/`; `chase-game` applies window mode, size, F11 and the 400x240 minimum at startup (QOLSET-19 glue); the title-screen hook (a "PC Settings" button on the slot list, QOLSET-41) exists; the pause entry exists (QOLSET-8); the menu is drawn and wired as the modern full-window menu of [qol-settings-ui](qol-settings-ui.md) (QOLSET-79).

## 3. Verified behaviour

None yet. Every behaviour is tested first in original mode (all QoL off, the port's tests unchanged), then with the feature on ([qol rules](../../rules/qol.md)).

## 4. Stand-ins

None.

## 5. Open items

- [ ] QOLSET-2 — The title-screen hook and input wiring (QOLSET-41), the stores behind the open menu (QOLSET-49) and the new drawing with its controller (QOLSET-79 in [qol-settings-ui](qol-settings-ui.md)) are done; what remains is a playtest by the user against this item. The settings menu with the tabs Game, Display, Controls and Quality of life, drawn crisp without game assets as the full-window overlay of [qol-settings-ui](qol-settings-ui.md), usable with mouse, keyboard and gamepad; the master button cycles Original, Improved, Custom; Custom restores the last own mix kept in `config/qol.cfg` and is skipped while no mix has been set. needs: QOLSET-1, QOLSET-79, QOLSET-38. blocks: QOLSET-3, QOLSET-4, QOLSET-8, QOLSET-9, QOLBOT-5, QOLSET-16, QOLSET-17, QOLSET-19, QOLSET-24, QOLCTL-8. evidence: [qol-settings.md §2026-10-28T09:32:07](../../design/qol-settings.md#ts-20261028-093207).
- [ ] QOLSET-25 — The Language row requests the Languages page (`Result::LanguagesRequested`); the menu has no screen for the Languages page, which the pause screen now shows in the item grid (QOLSET-S6). Show it and return to the menu with the new language. blocks: QOLSET-61, QOLSET-S6. evidence: [ui-screens-pause.md §2026-10-28T22:51:42](../../re/ui-screens-pause.md#ts-20261028-225142).
- [ ] QOLSET-59 — Menu edits do not reach the level run: it reads the options of `--save`, so only the mixer follows. evidence: [qol-settings.md §2026-10-28T09:32:07](../../design/qol-settings.md#ts-20261028-093207).
- [ ] QOLSET-61 — The Game tab shows the slot\'s volumes while the front end mixer runs at `--sound-volume` and `--music-volume` until the first change. needs: QOLSET-25. evidence: [options_page.hpp](../../../src/runtime/options_page.hpp).
- [ ] QOLSET-36 — The Polish and Russian language names are not found in `gametext` (eight of ten are); the Language row is checked with the spec's "Polski" and "Русский" only. evidence: [text-localisation-fonts.md §2026-10-29T13:52:02](../../re/text-localisation-fonts.md#ts-20261029-135202).

## 6. Findings summary

Design: [qol-settings.md](../../design/qol-settings.md); drawing: [qol-settings-drawing.md §2026-10-30T21:56:04](../../design/qol-settings-drawing.md#ts-20261030-215604), over the layout pieces [ui-layouts.md §2026-10-29T11:50:58](../../re/ui-layouts.md#ts-20261029-115058) and the font [text-localisation-fonts.md §2026-10-29T11:50:26](../../re/text-localisation-fonts.md#ts-20261029-115026).

## 7. Code and tests

Display and Controls tab items and code: [qol-settings-tabs](qol-settings-tabs.md).

Library `chase_qol` (`src/qol/`: `features.hpp` feature table, `settings.hpp`/`settings.cpp` master state, switches, window mode and resolution, strict `config/qol.cfg` reader and writer; `display_tab.hpp`/`display_tab.cpp` pure Display row model (window mode and resolution rows, clamp to the largest mode); format: `master Original|Improved|Custom`, then `window_mode windowed|fullscreen` (default fullscreen) and `resolution WIDTHxHEIGHT` (minimum 400x240; absent means the desktop size and stays absent until a size is picked), then `view_distance <factor>` while the View distance slider is above x1.0 ([qol-view-distance](qol-view-distance.md)), then one `<feature> on|off` line per feature once an own mix exists). Game tab model: `game_tab.hpp`/`game_tab.cpp` (`GameOptions` interface, `GameTab`), test `qol-game-tab` (`tests/qol/game_tab_test.cpp`). Test `qol-settings` (`tests/qol/settings_test.cpp`) covers original mode first, master states, Custom restore, round trip and rejections.

Guard: `tests/qol_separation.cmake`, CTest `qol-separation`, with `qol-separation-accepts` and one `qol-separation-rejects-<fixture>` per rule (fixtures in `tests/qol_separation_fixtures/`). Active store: `active_settings.hpp`/`active_settings.cpp` (`activeSettings`, `setActiveSettings`, `enabled`; default Original), test `qol-active-settings`. Window glue: `window_apply.hpp`/`window_apply.cpp` (`usableModes`, `WindowController`), port side `src/runtime/qol_window.*`, test `qol-window-apply` (`tests/qol/window_apply_test.cpp`). Drawing: [qol-settings-drawing](qol-settings-drawing.md). Pause entry (QOLSET-8): `pause_entry.hpp`/`pause_entry.cpp` (the always shown "PC Settings" wheel entry and the settings menu opened from it), tests `qol-pause-entry` (`tests/qol/pause_entry_test.cpp`) and `qol-pause-entry-real-data` (`tests/qol/pause_entry_real_test.cpp`).

Hook points of the port files that call into `src/qol/`: [qol-settings-menu-hooks](qol-settings-menu-hooks.md).
