# QoL hook points

## 1. Scope

The list of the hook points: every line of a port file that calls into `src/qol/` (rule file "Separate source code", [qol rules](../../rules/qol.md)), split from [qol-settings-menu](qol-settings-menu.md). The CTest `qol-separation` (`tests/qol_separation.cmake`) reads the list below. Plan: [qol.md](../qol.md).

## 2. Status

IN_PROGRESS. Every branch that adds, moves or removes a hook line updates the list in the same branch.

## 3. Verified behaviour

The CTest `qol-separation` counts the `qol::` lines of each port file against the bullets of that file below.

## 4. Stand-ins

None.

## 5. Open items

None.

## 6. Findings summary

Design of the separation: [qol-settings.md §2026-10-28T09:24:08](../../design/qol-settings.md#ts-20261028-092408).

## 7. Code and tests

Guard: `tests/qol_separation.cmake`, CTest `qol-separation`, fixtures in `tests/qol_separation_fixtures/`.

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
- src/game_level.cpp, LevelSession::draw, qol::withFarPlane hands applyFog the follow planes with the far plane times the view distance factor (the planes unchanged at x1.0), the View distance slider of the Display tab (`view_distance`; always x1.0 in Original)
- src/game_level.cpp, LevelSession::draw, qol::viewDistance multiplies the far plane of the projection and depth range by the factor after the follow and cutscene and free camera choice (all cameras; unchanged at x1.0), the View distance slider of the Display tab (`view_distance`; always x1.0 in Original)
- src/game_level_rooms.cpp, LevelSession::applyFog, qol::withFogDistance multiplies the fog start and end of the level fog by the factor (unchanged at x1.0), the View distance slider of the Display tab (`view_distance`; always x1.0 in Original)
- src/game_pedestrians.cpp, LevelPedestrians::updateSystem, qol::withViewDistance multiplies the fade-in and fade-out distances of the step fade table by the factor so the spawn window scales (the radius tiers stay; unchanged at x1.0), the View distance slider of the Display tab (`view_distance`; always x1.0 in Original)
- src/game_pedestrian_fade.cpp, SlotFades::step, qol::withViewDistance multiplies the fade-in and fade-out distances of the node fade table by the factor (unchanged at x1.0), the View distance slider of the Display tab (`view_distance`; always x1.0 in Original)
- src/game_pedestrian_fade.cpp, SlotFades::drawTest, qol::withViewDistance multiplies the fade-in and fade-out distances of the draw test table by the factor (unchanged at x1.0), the View distance slider of the Display tab (`view_distance`; always x1.0 in Original)

Calls without a `qol::` token that the guard does not count: `src/game.cpp` run, `qolWindow.syncSeatedControls(options.config, input, session.playerSeated(), session.screenOpen())` after `qolWindow.poll` (vehicle keys while seated and no pause or award screen is open, PC controls on; both are read-only `LevelSession` queries); `src/game_pause_menu.cpp` `LevelSession::mouseCameraEvent` and `mouseCameraFrame`, called from `pollEvents` in `src/game.cpp` once per event after the touch tracker and once after the loop, calling `mouseCamera.event` and `mouseCamera.frame` with the open-menu flag (pause requested or pause screen open; the feature check is inside `src/qol/pc_mouse_camera_sdl.cpp`, with the toggle off no SDL call is made); `src/game.cpp` run, `qolWindow.syncControls(options.config, input)` after the window is attached (rebinds the running input to the PC controls file at startup, only with PC controls on); `src/runtime/qol_window.cpp` `QolWindow::menuStores` handing `makeActiveControls` and `readActiveBindings` to the menu ports (off: both read `config/controls.cfg` as before).

Generic read-only interfaces of the port that the front end clicks read, with no QoL logic and no `qol::` token (20261102-qolctl2-D): `src/frontend/page_targets.hpp` (`PageTargets`, the rows and A/B/Y pictures of a bottom page in bottom-canvas pixels, built by `FrontEnd::targets()`) and `FrontInput::selectRow` in `src/frontend/flow.hpp` (a pointer's row pick; the pad never sets it, so with the toggle off nothing changes).
