# QoL settings menu and toggles

## 1. Scope

The settings menu of the PC port and the toggle store of all QoL features: the master toggle (Original, Custom, Improved), one toggle per feature, window mode and resolution, key bindings, and `config/qol.cfg`. Opened from the title screen and from an always shown "PC Settings" entry of the pause menu, with no extra key; tabs Game, Display, Controls and Quality of life; mouse, keyboard and gamepad. Nothing else of the `qol` plan may start before this exists. Plan: [qol.md](../qol.md).

## 2. Status

IN_PROGRESS, opened 2026-10-28T09:21:35+01:00. The toggle store (QOLSET-1), the drawing-free menu model (tabs, rows, events, master cycling, toggle rows; QOLSET-2 logic part), the Game tab model (QOLSET-9 model part) and the Display tab store (`window_mode`, `resolution`) and row model (QOLSET-3 part) exist in `src/qol/`; the drawing, the title-screen hook, the pause entry and the other hooks do not.

## 3. Verified behaviour

None yet. Every behaviour is tested first in original mode (all QoL off, the port's tests unchanged), then with the feature on ([qol rules](../../rules/qol.md)).

## 4. Stand-ins

None.

## 5. Open items

- [ ] QOLSET-2 — Logic part done (`menu_model`, test `qol-menu-model`); drawing, title-screen hook and input wiring remain. The settings menu with the tabs Game, Display, Controls and Quality of life, drawn with the original game's art and fonts, usable with mouse, keyboard and gamepad, opened from the title screen; the master button cycles Original, Improved, Custom; Custom restores the last own mix kept in `config/qol.cfg` and is skipped while no mix has been set. needs: QOLSET-1. blocks: QOLSET-3, QOLSET-4, QOLSET-8, QOLSET-9, QOLBOT-5, QOLSET-16, QOLSET-17, QOLSET-19. evidence: [qol-settings.md §2026-10-28T09:32:07](../../design/qol-settings.md#ts-20261028-093207).
- [ ] QOLSET-3 — Storage and row model done (`window_mode`, `resolution` in `settings`, `display_tab`, test `qol-display-tab`); menu wiring, window changes and F11 remain (QOLSET-19). Windowed and borderless full screen in the Display tab, plus resolution, and an F11 shortcut that switches the window mode (the key is a binding in `config/controls.cfg`, format in `docs/controls.md`); window mode and resolution are stored in `config/qol.cfg` as `window_mode` (`windowed` or `fullscreen`) and `resolution` (`WIDTHxHEIGHT`, for example `1280x720`), both written in every mode (values: [qol-settings.md §2026-10-28T22:38:57](../../design/qol-settings.md#ts-20261028-223857)). needs: QOLSET-2. blocks: QOLSET-18. evidence: [qol-settings.md §2026-10-28T22:20:41](../../design/qol-settings.md#ts-20261028-222041).
- [ ] QOLSET-19 — Wire the Display rows (`display_tab`) into the Display tab of the menu model, pass the SDL display modes as the resolution list, resolve the desktop size into the store before saving (`Settings::resolveDesktop`), apply window mode and resolution to the window, and add the F11 binding; question for the user: the file spelling while the desktop size is unresolved and an upper bound for a stored resolution. needs: QOLSET-2, QOLSET-18. evidence: [qol-settings.md §2026-10-28T22:20:41](../../design/qol-settings.md#ts-20261028-222041).
- [ ] QOLSET-4 — Key bindings in the Controls tab, stored in `config/controls.cfg` (format: `docs/controls.md`); the Game, Display and Controls tabs work with every QoL feature off, their values other than the bindings are stored in `config/qol.cfg` (window mode and resolution as `window_mode` and `resolution`, value formats still open). needs: QOLSET-2. evidence: [qol-settings.md §2026-10-28T09:32:07](../../design/qol-settings.md#ts-20261028-093207).
- [ ] QOLSET-8 — The always shown "PC Settings" entry is the last entry of the round pause menu, chosen by mouse click, D-pad or keyboard like the other entries; it opens the PC settings menu only and changes no game behaviour (the one approved exception, shown even with every QoL feature off; the original Options page stays unchanged; the entry is a hook point). needs: QOLSET-2, QOLSET-7. evidence: [qol-settings.md §2026-10-28T09:32:07](../../design/qol-settings.md#ts-20261028-093207).
- [ ] QOLSET-9 — Model done (`game_tab`, test `qol-game-tab`: four rows over the plain `GameOptions` interface, volumes 0 to 10 in steps of 1, invert camera a toggle, languages 1 to 9, values read live so the tab and the original Options page stay in sync); the port side that implements the interface is QOLSET-21, the Game tab rows in the menu model and the drawing remain. needs: QOLSET-2, QOLSET-21. evidence: [qol-settings.md §2026-10-28T09:32:07](../../design/qol-settings.md#ts-20261028-093207).
- [ ] QOLSET-21 — Hook: a port-side class implements `qol::GameOptions` over the live volume bytes and the options block of the save (`src/runtime/options_page.hpp`: volumes 0 to 10, invert camera bit, language 1 to 9), so the Game tab edits the same values and the same save as the original Options page; open: how the tab picks a language (the original opens a Languages page, UI-23). blocks: QOLSET-9. evidence: [qol-settings.md §2026-10-28T09:32:07](../../design/qol-settings.md#ts-20261028-093207).

## 6. Findings summary

Design: [qol-settings.md](../../design/qol-settings.md).

## 7. Code and tests

Library `chase_qol` (`src/qol/`: `features.hpp` feature table, `settings.hpp`/`settings.cpp` master state, switches, window mode and resolution, strict `config/qol.cfg` reader and writer; `display_tab.hpp`/`display_tab.cpp` pure Display row model (window mode row, resolution row cycling a given list); format: `master Original|Improved|Custom`, then `window_mode windowed|fullscreen` (default fullscreen) and `resolution WIDTHxHEIGHT` (minimum 400x240; absent means the desktop size, written once resolved), then one `<feature> on|off` line per feature once an own mix exists). Game tab model: `game_tab.hpp`/`game_tab.cpp` (`GameOptions` interface, `GameTab`), test `qol-game-tab` (`tests/qol/game_tab_test.cpp`). Test `qol-settings` (`tests/qol/settings_test.cpp`) covers original mode first, master states, Custom restore, round trip and every rejection. Test `qol-display-tab` (`tests/qol/display_tab_test.cpp`) covers the rows, toggling, list cycling and rejections. Further tests: one set with the feature off (unchanged behaviour) and one with it on, pinned to the design entry.

Guard: `tests/qol_separation.cmake`, CTest `qol-separation`, with `qol-separation-accepts` and one `qol-separation-rejects-<fixture>` per rule (fixtures in `tests/qol_separation_fixtures/`). Menu model: `menu_model.hpp`/`menu_model.cpp` (pure plain types: tabs, rows generic over `features.hpp`, key and pointer events, `MenuModel::handle`), test `qol-menu-model` (`tests/qol/menu_model_test.cpp`, original mode first). Drawing: not written yet.

### Hook points

Port files that call into `src/qol/` (rule file "Separate source code"): none yet. One bullet per hook, `- file, function, hook, toggle` (the CTest `qol-separation` counts the bullets per file against the `qol::` lines of that file).
