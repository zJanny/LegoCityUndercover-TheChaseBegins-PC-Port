# QoL settings menu and toggles

## 1. Scope

The settings menu of the PC port and the toggle store of all QoL features: the master toggle (Original, Custom, Improved), one toggle per feature, window mode and resolution, key bindings, and `config/qol.cfg`. Opened from the title screen and from an always shown "PC Settings" entry of the pause menu, with no extra key; tabs Game, Display, Controls and Quality of life; mouse, keyboard and gamepad. Nothing else of the `qol` plan may start before this exists. Plan: [qol.md](../qol.md).

## 2. Status

IN_PROGRESS, opened 2026-10-28T09:21:35+01:00. The toggle store (QOLSET-1), the drawing-free menu model (tabs, rows, events, master cycling, toggle rows; QOLSET-2 logic part), the Game tab model, its port side `QolGameOptions` (QOLSET-21) and its wiring into the open menu over the live options, the mixer and the save (QOLSET-9, QOLSET-24), the Controls tab model, binding adapter, file writer, `pad` source and the join `QolControlsBindings` (QOLSET-4) and the Display tab store (`window_mode`, `resolution`), row model and its place in the menu model, with the clamp to the largest mode (QOLSET-3, QOLSET-19 model parts) exist in `src/qol/`; `chase-game` applies window mode, size, F11 and the 400x240 minimum at startup (QOLSET-19 glue, questions QOLSET-28 and QOLSET-29 answered); the title-screen hook (a "PC Settings" button on the slot list, QOLSET-41) exists; the pause entry and the other hooks do not; the drawing spec is decided; the page drawing is in progress in [qol-settings-drawing](qol-settings-drawing.md) (QOLSET-33).

## 3. Verified behaviour

None yet. Every behaviour is tested first in original mode (all QoL off, the port's tests unchanged), then with the feature on ([qol rules](../../rules/qol.md)).

## 4. Stand-ins

None.

## 5. Open items

- [ ] QOLSET-2 — Logic part done (`menu_model`, test `qol-menu-model`); the title-screen hook and input wiring are done (QOLSET-41); the drawing remains (the stores behind the open menu are joined, QOLSET-49); the model changes for the user's answers remain too (Left and Right step the volume, Display, master and toggle rows, the mouse wheel moves rows, [qol-settings-drawing.md §2026-10-29T12:11:55](../../design/qol-settings-drawing.md#ts-20261029-121155): Left back and Right forward, stopping at the ends; Invert camera flips; the Language row ignores them, the Controls rows pick the slot). The settings menu with the tabs Game, Display, Controls and Quality of life, drawn with the original game's art and fonts, usable with mouse, keyboard and gamepad, opened from the title screen; the master button cycles Original, Improved, Custom; Custom restores the last own mix kept in `config/qol.cfg` and is skipped while no mix has been set. needs: QOLSET-1, QOLSET-33, QOLSET-38. blocks: QOLSET-3, QOLSET-4, QOLSET-8, QOLSET-9, QOLBOT-5, QOLSET-16, QOLSET-17, QOLSET-19, QOLSET-24. evidence: [qol-settings.md §2026-10-28T09:32:07](../../design/qol-settings.md#ts-20261028-093207).
- [ ] QOLSET-25 — The Language row requests the Languages page (`Result::LanguagesRequested`); the menu has no screen for the Languages page, which the pause screen now shows in the item grid (QOLSET-S6). Show it and return to the menu with the new language. blocks: QOLSET-61, QOLSET-S6. evidence: [ui-screens-pause.md §2026-10-28T22:51:42](../../re/ui-screens-pause.md#ts-20261028-225142).
- [ ] QOLSET-59 — Menu edits do not reach the level run: it reads the options of `--save`, so only the mixer follows. evidence: [qol-settings.md §2026-10-28T09:32:07](../../design/qol-settings.md#ts-20261028-093207).
- [ ] QOLSET-61 — The Game tab shows the slot\'s volumes while the front end mixer runs at `--sound-volume` and `--music-volume` until the first change. needs: QOLSET-25. evidence: [options_page.hpp](../../../src/runtime/options_page.hpp).
- [ ] QOLSET-36 — The Polish and Russian language names are not found in `gametext` (eight of ten are); the Language row is checked with the spec's "Polski" and "Русский" only. evidence: [text-localisation-fonts.md §2026-10-29T13:52:02](../../re/text-localisation-fonts.md#ts-20261029-135202).

## 6. Findings summary

Design: [qol-settings.md](../../design/qol-settings.md); drawing: [qol-settings-drawing.md §2026-10-29T12:11:55](../../design/qol-settings-drawing.md#ts-20261029-121155), over the layout pieces [ui-layouts.md §2026-10-29T11:50:58](../../re/ui-layouts.md#ts-20261029-115058) and the font [text-localisation-fonts.md §2026-10-29T11:50:26](../../re/text-localisation-fonts.md#ts-20261029-115026).

## 7. Code and tests

Display and Controls tab items and code: [qol-settings-tabs](qol-settings-tabs.md).

Library `chase_qol` (`src/qol/`: `features.hpp` feature table, `settings.hpp`/`settings.cpp` master state, switches, window mode and resolution, strict `config/qol.cfg` reader and writer; `display_tab.hpp`/`display_tab.cpp` pure Display row model (window mode row, resolution row cycling a given list, clamp of a stored size to the largest mode); format: `master Original|Improved|Custom`, then `window_mode windowed|fullscreen` (default fullscreen) and `resolution WIDTHxHEIGHT` (minimum 400x240; absent means the desktop size and stays absent until a size is picked), then one `<feature> on|off` line per feature once an own mix exists). Game tab model: `game_tab.hpp`/`game_tab.cpp` (`GameOptions` interface, `GameTab`), test `qol-game-tab` (`tests/qol/game_tab_test.cpp`). Test `qol-settings` (`tests/qol/settings_test.cpp`) covers original mode first, master states, Custom restore, round trip and every rejection. Further tests: one set with the feature off (unchanged behaviour) and one with it on, pinned to the design entry.

Guard: `tests/qol_separation.cmake`, CTest `qol-separation`, with `qol-separation-accepts` and one `qol-separation-rejects-<fixture>` per rule (fixtures in `tests/qol_separation_fixtures/`). Menu model: `menu_model.hpp`/`menu_model.cpp` (pure plain types: tabs, rows generic over `features.hpp`, key and pointer events, `MenuModel::handle`), test `qol-menu-model` (`tests/qol/menu_model_test.cpp`, original mode first). Active store: `active_settings.hpp`/`active_settings.cpp` (`activeSettings`, `setActiveSettings`, `enabled`; default Original), test `qol-active-settings`. Window glue: `window_apply.hpp`/`window_apply.cpp` (`usableModes`, `WindowController`), port side `src/runtime/qol_window.*`, test `qol-window-apply` (`tests/qol/window_apply_test.cpp`). Drawing: [qol-settings-drawing](qol-settings-drawing.md). Text check: `text_check.hpp`/`text_check.cpp` (generic glyph and width check over a `GlyphWidth` callback, adapter for the port's `assets::Font`), `menu_texts.hpp`/`menu_texts.cpp` (every text of the menu model plus the Controls names), tests `qol-text-check` (synthetic font), `qol-menu-texts` and `qol-text-check-real-data` (label `needs-game-data`; SDL key names in `tests/data/qol_sdl_key_names.inc`). Pause entry (QOLSET-8): `pause_entry.hpp`/`pause_entry.cpp` (the always shown "PC Settings" wheel entry and the settings menu opened from it), tests `qol-pause-entry` (`tests/qol/pause_entry_test.cpp`) and `qol-pause-entry-real-data` (`tests/qol/pause_entry_real_test.cpp`).

### Hook points

Port files that call into `src/qol/` (rule file "Separate source code"). One bullet per hook, `- file, function, hook, toggle` (the CTest `qol-separation` counts the bullets per file against the `qol::` lines of that file).
- src/runtime/qol_game_options.hpp, QolGameOptions, implements the plain qol::GameOptions interface over the Options page state and calls no QoL logic, none (the Game tab is not a QoL feature)
- src/runtime/qol_window.cpp, QolWindow constructor, qol::makeWindowController loads config/qol.cfg and plans the window mode and size at startup, none (the Display tab is not a QoL feature)
- src/runtime/qol_window.hpp, QolWindow, holds the plain qol::WindowController that applies F11 and the 400x240 minimum, none (the Display tab is not a QoL feature)
- src/game_level.cpp, LevelSession::draw, qol::topViewRegion returns the region of the 3D view and the glow pass (originalViewRegion while off or for a non-gameplay camera), qol::Feature::WidenedView
- src/game_level.cpp, LevelSession::draw, qol::drawHudParts draws each HUD part at its corner rectangle instead of the whole HUD on the top canvas (returns false and draws nothing while off), qol::Feature::WidenedView
- src/game_level.cpp, LevelSession::draw, qol::drawRoundMinimap draws the round minimap from the filled bottom canvas into the top-right corner (draws nothing unless the master is Improved or while a panel shows), qol::Master::Improved (the master button; there is no toggle of its own)
- src/game_hud.hpp, screensFor, qol::hiddenScreens returns the fitted top rectangle and an empty bottom rectangle while the bottom screen is hidden (this also ends its touch as the touch tracker reads the same rectangle), qol::Master::Improved (the master button; there is no toggle of its own)
- src/runtime/qol_controls_bindings.hpp, QolControlsBindings, aliases the plain qol::ControlsTab types for the join of the Controls tab with the controls file, none (the Controls tab is not a QoL feature)
- src/runtime/qol_controls_bindings.hpp, QolControlsBindings, implements the plain qol::ControlsPort interface that the menu model reads, none (the Controls tab is not a QoL feature)
- src/runtime/qol_controls_bindings.cpp, QolControlsBindings, calls the qol:: file reader and writer with the SDL key resolver and namer, none (the Controls tab is not a QoL feature)
- src/game_front.hpp, game_front::run, qol::FrontMenu holds the settings menu of the front end (built when its button is first clicked), none (the settings menu is the toggle mechanism)
- src/game_front.hpp, game_front::run, qol::frontMenuConsumes takes the left click on the slot page's "PC Settings" button and while the menu is open every key or button press and every motion or wheel step (releases and quitting pass on), none (the settings menu is the toggle mechanism)
- src/game_front.hpp, game_front::run, qol::frontMenuPresented draws the open menu over the whole window and the loop skips the front end's update and drawing (the front end stops), none (the settings menu is the toggle mechanism)
- src/game_front.hpp, game_front::run, qol::drawFrontMenuButton draws the "PC Settings" bar (the third list bar at half size) under slot 2 of the slot page, none (the settings menu is the toggle mechanism)
- src/game_front.hpp, game_front::run, qol::presentFrontPanels in the loop draws the darkened top picture with the 320x240 bottom page as a centred panel while the bottom screen is hidden (returns false and draws nothing while off so the presenter shows the two screens as before), qol::Master::Improved (the master button; there is no toggle of its own)
- src/game_front.hpp, game_front::run, qol::presentFrontPanels in the script path does the same for the scripted picture and screenshot, qol::Master::Improved (the master button; there is no toggle of its own)
- src/game_front.hpp, game_front::Options, qol::StoresPorts carries the ports of the stores behind the open menu (settings file and window and controls callbacks) from the window glue to the menu, none (the settings menu is the toggle mechanism)
- src/runtime/qol_window.hpp, QolWindow::menuStores, returns the plain qol::StoresPorts of the window glue, none (the settings menu is the toggle mechanism)
- src/runtime/qol_window.cpp, QolWindow::menuStores, names the qol:: types of the ports it fills (display modes and window controller adopt and the controls join), none (the settings menu is the toggle mechanism)
- src/runtime/qol_game_wiring.hpp, qolGamePortFactory, returns the factory of the plain qol::GamePort that the menu stores call, none (the Game tab is not a QoL feature)
- src/runtime/qol_game_wiring.cpp, QolGamePort, implements the plain qol::GamePort interface over the Options page state and calls the mixer callback and the save writer, none (the Game tab is not a QoL feature)
- src/runtime/qol_game_wiring.cpp, qolGamePortFactory, names the qol::GamePort type of the factory it returns, none (the Game tab is not a QoL feature)
- src/runtime/qol_game_wiring.cpp, qolGamePortFactory lambda, names the qol::GamePort type it makes, none (the Game tab is not a QoL feature)
- src/game_level.hpp, game_level::Shared, qol::StoresPorts carries the ports of the stores behind the open menu from the window glue to the level session, none (the settings menu is the toggle mechanism)
- src/game_level.hpp, LevelSession, qol::PauseSettings holds the settings menu opened from the pause entry, none (the settings menu is the toggle mechanism)
- src/game_pause_menu.cpp, LevelSession::holdForPause, qol::pauseEntryHolds opens the menu when the extra entry was chosen and holds the pause screen and the game while it is open, none (the settings menu is the toggle mechanism)
- src/game_pause_menu.cpp, LevelSession::holdForPause, qol::withPauseEntry appends the always shown "PC Settings" entry to the setup of the pause menu (the last entry; port side `PauseMenuSetup::extraEntries`), none (approved exception of docs/rules/qol.md)
- src/game_pause_menu.cpp, LevelSession::drawPause, qol::pauseEntryPresented draws the open menu over the whole window instead of the pause screen, none (the settings menu is the toggle mechanism)
- src/game_pause_menu.cpp, LevelSession::pauseSettingsConsume, qol::pauseEntryConsumes takes every key and button press and every mouse and wheel event while the menu is open, none (the settings menu is the toggle mechanism)
