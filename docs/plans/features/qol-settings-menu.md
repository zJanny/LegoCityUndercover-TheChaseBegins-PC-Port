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

Library `chase_qol` (`src/qol/`: `features.hpp` feature table, `settings.hpp`/`settings.cpp` master state, switches, window mode and resolution, strict `config/qol.cfg` reader and writer; `display_tab.hpp`/`display_tab.cpp` pure Display row model (window mode and resolution rows, clamp to the largest mode); format: `master Original|Improved|Custom`, then `window_mode windowed|fullscreen` (default fullscreen) and `resolution WIDTHxHEIGHT` (minimum 400x240; absent means the desktop size and stays absent until a size is picked), then one `<feature> on|off` line per feature once an own mix exists). Game tab model: `game_tab.hpp`/`game_tab.cpp` (`GameOptions` interface, `GameTab`), test `qol-game-tab` (`tests/qol/game_tab_test.cpp`). Test `qol-settings` (`tests/qol/settings_test.cpp`) covers original mode first, master states, Custom restore, round trip and rejections.

Guard: `tests/qol_separation.cmake`, CTest `qol-separation`, with `qol-separation-accepts` and one `qol-separation-rejects-<fixture>` per rule (fixtures in `tests/qol_separation_fixtures/`). Active store: `active_settings.hpp`/`active_settings.cpp` (`activeSettings`, `setActiveSettings`, `enabled`; default Original), test `qol-active-settings`. Window glue: `window_apply.hpp`/`window_apply.cpp` (`usableModes`, `WindowController`), port side `src/runtime/qol_window.*`, test `qol-window-apply` (`tests/qol/window_apply_test.cpp`). Drawing: [qol-settings-drawing](qol-settings-drawing.md). Pause entry (QOLSET-8): `pause_entry.hpp`/`pause_entry.cpp` (the always shown "PC Settings" wheel entry and the settings menu opened from it), tests `qol-pause-entry` (`tests/qol/pause_entry_test.cpp`) and `qol-pause-entry-real-data` (`tests/qol/pause_entry_real_test.cpp`).

### Hook points

Port files that call into `src/qol/` (rule file "Separate source code"). One bullet per hook, `- file, function, hook, toggle` (the CTest `qol-separation` counts the bullets per file against the `qol::` lines of that file).
- src/runtime/qol_game_options.hpp, QolGameOptions, implements the plain qol::GameOptions interface over the Options page state and calls no QoL logic, none (the Game tab is not a QoL feature)
- src/runtime/qol_window.cpp, QolWindow constructor, qol::makeWindowController loads config/qol.cfg and plans the window mode and size at startup, none (the Display tab is not a QoL feature)
- src/runtime/qol_window.hpp, QolWindow, holds the plain qol::WindowController that applies F11 and the 400x240 minimum, none (the Display tab is not a QoL feature)
- src/game_level.cpp, LevelSession::draw, qol::topViewRegion returns the region of the 3D view and the glow pass (originalViewRegion while off or for a non-gameplay camera), qol::Feature::WidenedView
- src/game_level.cpp, LevelSession::draw, qol::cameraKindFor names the camera kind (gameplay or a 5:3 camera) for topViewRegion and drawHudParts (no effect by itself), qol::Feature::WidenedView
- src/game_level.cpp, LevelSession::draw, qol::drawHudParts draws each HUD part at its corner rectangle (at the 5:3 picture's corners for a non-gameplay camera) instead of the whole HUD on the top canvas (returns false and draws nothing while off), qol::Feature::WidenedView
- src/game_level.cpp, LevelSession::draw, qol::drawRoundMinimap draws the round minimap from the filled bottom canvas into the top-right corner (draws nothing unless the master is Improved or while a panel shows), qol::Master::Improved (the master button; there is no toggle of its own)
- src/game_hud.hpp, screensFor, qol::hiddenScreens returns the fitted top rectangle and an empty bottom rectangle while the bottom screen is hidden (this also ends its touch as the touch tracker reads the same rectangle), qol::Master::Improved (the master button; there is no toggle of its own)
- src/runtime/qol_pc_controls.hpp, readActiveBindings and makeActiveControls, aliases the plain qol::ControlsPort that makeActiveControls returns for the Controls tab, qol::Feature::PcControls (the choice of file is made in src/qol/pc_controls.cpp)
- src/runtime/qol_pc_controls.cpp, readActiveBindings and makeActiveControls, names the qol functions that pick the controls file and wrap the Controls tab port (qol::activeControlsForSet and qol::ReloadingControls), qol::Feature::PcControls
- src/runtime/qol_window.hpp, QolWindow, holds the plain qol::SeatedControls, qol::Feature::PcControls
- src/runtime/qol_controls_bindings.hpp, QolControlsBindings, aliases the plain qol::ControlsTab types for the join of the Controls tab with the controls file, none (the Controls tab is not a QoL feature)
- src/runtime/qol_controls_bindings.hpp, QolControlsBindings, implements the plain qol::ControlsPort interface that the menu model reads, none (the Controls tab is not a QoL feature)
- src/runtime/qol_controls_bindings.cpp, QolControlsBindings, calls the qol:: file reader and writer with the SDL key resolver and namer, none (the Controls tab is not a QoL feature)
- src/game_front.hpp, game_front::run, qol::FrontMenu holds the settings menu of the front end (built when its button is first clicked), none (the settings menu is the toggle mechanism)
- src/game_front.hpp, game_front::run, qol::frontMenuConsumes takes the left click on the slot page's "PC Settings" button and while the menu is open every key or button press and every motion or wheel step (releases and quitting pass on), none (the settings menu is the toggle mechanism)
- src/game_front.hpp, game_front::run, qol::frontMenuPresented draws the open menu over the dimmed last frame and the loop skips the front end's update and drawing (the front end stops; the call returns false once after the menu opens so that one normal frame is kept), none (the settings menu is the toggle mechanism)
- src/game_front.hpp, game_front::run, qol::steerFrontMenuEntry moves the cursor onto the "PC Settings" bar and back and takes its A press and withholds them from the front end, none (the settings menu is the toggle mechanism)
- src/game_front.hpp, game_front::run, qol::drawFrontMenuButton draws the "PC Settings" bar (the third list bar at half size) under slot 2 of the slot page, none (the settings menu is the toggle mechanism)
- src/game_front.hpp, game_front::run, qol::presentFrontPanels in the loop shows the bottom page as a panel over the darkened picture; a movie gets its subtitle bubble and no panel (QOLBOT-28; false while off), qol::Master::Improved
- src/game_front.hpp, game_front::run, qol::presentFrontPanels in the script path does the same for the scripted screenshot, qol::Master::Improved
- src/game_level.cpp, LevelSession::draw, qol::drawMovieSubtitles draws a movie's subtitle bubble over the picture (QOLBOT-29; nothing while off), qol::Master::Improved
- src/game_front.hpp, game_front::run, qol::FrontClicks holds the queue of the pending panel click, qol::Master::Improved (the master button; there is no toggle of its own)
- src/game_front.hpp, game_front::run, qol::frontClicksConsume takes a left click on a page with rows or buttons while the front end panel shows and queues the row pick and button press it means (other events and pages and Original or Custom pass on), qol::Master::Improved (the master button; there is no toggle of its own)
- src/game_front.hpp, game_front::run, qol::applyFrontClicks adds the queued row pick and button press to the pad input of one update, qol::Master::Improved (the master button; there is no toggle of its own)
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
- src/game_pause_menu.cpp, LevelSession::drawPause, qol::pauseEntryPresented draws the open menu over the dimmed last pause frame instead of the pause screen (returns false once after the menu opens so that one pause frame is kept), none (the settings menu is the toggle mechanism)
- src/game_pause_menu.cpp, LevelSession::pauseSettingsConsume, qol::pauseEntryConsumes takes every key and button press and every mouse and wheel event while the menu is open, none (the settings menu is the toggle mechanism)
- src/game_level.hpp, LevelSession, qol::MouseCameraSdl holds the mouse camera state (the captured cursor and the accumulated mouse X), qol::Feature::PcControls
- src/game_level_update.cpp, LevelSession::updateFollowCamera, qol::enabled and mouseCamera.take add the accumulated mouse X as an analog R value to the existing shoulder turn (0 while off), qol::Feature::PcControls
- src/game_pause_menu.cpp, mouseScreenOf, qol::MouseScreen carries the window size and the lower-screen rectangle in window pixels (plain ints from screensFor) to the mouse camera, qol::Feature::PcControls

Calls without a `qol::` token that the guard does not count: `src/game.cpp` run, `qolWindow.syncSeatedControls(options.config, input, session.playerSeated(), session.screenOpen())` after `qolWindow.poll` (vehicle keys while seated and no pause or award screen is open, PC controls on; both are read-only `LevelSession` queries); `src/game_pause_menu.cpp` `LevelSession::mouseCameraEvent` and `mouseCameraFrame`, called from `pollEvents` in `src/game.cpp` once per event after the touch tracker and once after the loop, calling `mouseCamera.event` and `mouseCamera.frame` with the open-menu flag (pause requested or pause screen open; the feature check is inside `src/qol/pc_mouse_camera_sdl.cpp`, with the toggle off no SDL call is made); `src/game.cpp` run, `qolWindow.syncControls(options.config, input)` after the window is attached (rebinds the running input to the PC controls file at startup, only with PC controls on); `src/runtime/qol_window.cpp` `QolWindow::menuStores` handing `makeActiveControls` and `readActiveBindings` to the menu ports (off: both read `config/controls.cfg` as before).

Generic read-only interfaces of the port that the front end clicks read, with no QoL logic and no `qol::` token (20261102-qolctl2-D): `src/frontend/page_targets.hpp` (`PageTargets`, the rows and A/B/Y pictures of a bottom page in bottom-canvas pixels, built by `FrontEnd::targets()`) and `FrontInput::selectRow` in `src/frontend/flow.hpp` (a pointer's row pick; the pad never sets it, so with the toggle off nothing changes).
