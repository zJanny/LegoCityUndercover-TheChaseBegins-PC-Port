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

The toggle store, the settings menu (S1), the master button, the widened view with the corner HUD, the round minimap and the front end panel exist. Round 20261030-qolmenu finishes decided menu and view items: arrow keys by axis (QOLSET-64), two-line Controls bars (QOLSET-43), corner HUD at the picture corners in 5:3 views (QOLFS-19). Next, in this order: the slot page "PC Settings" bar (QOLSET-53, measure the art first), front end row and button clicks (QOLBOT-22, needs row geometry from the port), redraw speed (QOLSET-58) and the menu storyboards (QOLSET-52). Panels of the scanner, build-it, shop, award viewer, characters and fast-travel map (QOLBOT-2..10) wait for the original screens. Open questions for the user are marked as such in the feature files and must be answered before the work on them starts. The port keeps priority over every QoL item.

## Session Log

One line per session, newest last; `--fix` keeps the newest 20 and moves older ones to `qol.progress-archive.md`.

- 2026-10-29T19:31:32+01:00 — Lead (recorded playtest of the PC settings menu from the slot page): the user found it laggy, not dynamic, the button small and its use unclear, low res; verified against code and a timing. See [qol-settings-drawing](features/qol-settings-drawing.md)
- 2026-10-29T19:34:43+01:00 — Lead (recorded the user's decisions after the settings menu playtest): full-size labelled bar with a hint, crisp native-resolution text, original UI animations; design entry added and the half-size bar place replaced. See [qol-settings-drawing](features/qol-settings-drawing.md)
- 2026-10-30T14:14:22+01:00 — Agent 20261030-qolstores-C (menu redraw cost): the open settings menu keeps its last six rendered pages (`MenuRedraw`, equal to the full redraw) and uploads the window only when the page changed; a page not seen before still costs the full draw. See [qol-settings-drawing](features/qol-settings-drawing.md).
- 2026-10-30T14:14:35+01:00 — Agent 20261030-qolstores-B (stores behind the open menu): the menu opened from the slot page now reads and writes the active store, writes `config/qol.cfg` after each change, sends Display changes to the window, joins the Controls tab with `config/controls.cfg` and the running input, and gets the display modes and desktop size (`qol::MenuStores`, test qol-menu-stores); the Game tab stays on its copy; the user answered the rebind and --window questions the same day. See [qol-settings-drawing](features/qol-settings-drawing.md)
- 2026-10-30T14:05:55+01:00 — Agent 20261030-qolstores-A (corner HUD hook): `qol::drawHudParts` and one hook line in `LevelSession::draw` draw each HUD part at its corner rectangle with the widened view on; the head icon and scanner overlay stay on the top canvas. See [qol-fullscreen](features/qol-fullscreen.md).
- 2026-10-30T14:07:44+01:00 — Lead (recorded the user's answers of 20261030-qolstores): front end hides the bottom screen like play, mock-up first for the slot page bar, text and frames sharp, wait for the smaller text for two-line Controls bars. See [qol-bottom-screen](features/qol-bottom-screen.md), [qol-settings-drawing](features/qol-settings-drawing.md)
- 2026-10-30T14:03:31+01:00 — Agent 20261030-qolstores-D (pop-in at the widened edges): code reading of every cull, range, fade and shadow decision against the 16:9 edge; all are radial or depth based except the pedestrian spawn cone, which covers 16:9 at the follow camera's FOV; the pop-in check is closed and three follow-up items are open. See [qol-fullscreen](features/qol-fullscreen.md).
- 2026-10-30T15:26:12+01:00 — Lead (20261030-qolstores end): merged 20261030-qolstores-D (culling at the widened view edges), 20261030-qolstores-E (linux-port: pause Languages page), 20261030-qolstores-A (corner-anchored HUD parts), 20261030-qolstores-B (settings menu stores applied and saved), 20261030-qolstores-F (linux-port: mission 1-3 fence vault, checked against oracle run 20261030-qolstores-F-1), 20261030-qolstores-C (menu redraw cache), plus the recorder's record of the user's design answers, through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [qol-fullscreen.md](features/qol-fullscreen.md), [qol-settings-drawing.md](features/qol-settings-drawing.md), [qol-settings-menu.md](features/qol-settings-menu.md), [ui-menus.md](features/ui-menus.md), [player-traversal.md](features/player-traversal.md)
- 2026-10-30T15:42:40+01:00 — Agent 20261030-qolpanels-A (Game tab on the live options): the menu opened from the slot page owns the Game tab over a live `OptionsPage`, options block and volume bytes (`qol::GamePort`, `src/runtime/qol_game_wiring.*`): Left, Right and Enter edit the values, gains go to the mixer at once, the options are stored on leaving; the Language row requests the Languages page, which the menu cannot show yet; tests qol-menu-game and qol-game-wiring. See [qol-settings-menu](features/qol-settings-menu.md).
- 2026-10-30T15:44:15+01:00 — Agent 20261030-qolpanels-C (pause entry): the round pause menu has the always shown "PC Settings" entry as its last wheel item (generic `extraEntries` in the port, `src/qol/pause_entry.*`); it opens the settings menu over the same stores, the game waits and Escape returns; tests qol-pause-entry and qol-pause-entry-real-data. See [qol-settings-menu](features/qol-settings-menu.md)
- 2026-10-30T15:39:59+01:00 — Agent 20261030-qolpanels-B (round minimap): the Improved master draws a round minimap in the top-right corner from the original map canvas (rotation, icons and objective markers unchanged), hidden in cutscenes and the scanner; test qol-minimap; margin, overlap with the studs counter and other panels remain. See [qol-minimap](features/qol-minimap.md).
- 2026-10-30T16:35:51+01:00 — Lead (20261030-qolpanels end): merged 20261030-qolpanels-A (Game tab on the live options), -B (round minimap), -C (PC Settings pause entry), -D (smaller layout text, linux-port), -E (canvas clipping, linux-port) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [qol-settings-menu.md](features/qol-settings-menu.md), [qol-minimap.md](features/qol-minimap.md), [hud-ui.md](features/hud-ui.md), [rendering.md](features/rendering.md); user docs audit: declined
- 2026-10-30T16:42:34+01:00 — Lead (recorded the user's decisions of 2026-10-30): answers on the PC Settings bar, the front end panel, the minimap place and volume clicks are in docs/design; items updated in [qol-bottom-screen](features/qol-bottom-screen.md).
- 2026-10-30T17:03:35+01:00 — Lead (recorded the user's rule that arrow keys keep to their axis in the settings menu): design entry added; the user answered the key layout (Left/Right turn rows, Up/Down change values). See [qol-settings-drawing](features/qol-settings-drawing.md)
- 2026-10-30T17:07:04+01:00 — Agent 20261030-qolpanels2-A (front end panel): with Improved the front end shows the darkened top picture with the bottom page as a centred 4:3 panel (`src/qol/front_panel*`, `FrontPresenter::presentBoxes`), the slot page "PC Settings" bar click maps through the panel; row and button clicks stay open; split `qol-settings-tabs` off the settings menu file. See [qol-bottom-screen](features/qol-bottom-screen.md).
- 2026-10-30T17:10:03+01:00 — Agent 20261030-qolpanels2-B (minimap placement and panels): the round minimap now sits under the studs and counters (counters edge measured on real data, 4 unit margin) and is hidden in dialog boxes, the scene change page and the fast-travel map besides the scanner; the pause menu already replaces the picture. See [qol-minimap](features/qol-minimap.md).
- 2026-10-30T17:12:07+01:00 — Agent 20261030-qolpanels2-C (camera kinds, volume clicks): only the gameplay camera widens, a click on the left or right half of a volume row's name plate steps the volume; see [qol-fullscreen.md](features/qol-fullscreen.md).
- 2026-10-30T20:31:45+01:00 — Lead (20261030-qolpanels2 end): merged the recorder (arrow-key rule for menus), 20261030-qolpanels2-A (front-end panel in Improved; row and button clicks still open), -D (pause screen item grid), -B (minimap under the counters, hidden in panels and speech boxes), -C (gameplay-only widening and volume value clicks; HUD corners in 5:3 views decided, not built) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [qol-bottom-screen.md](features/qol-bottom-screen.md), [qol-minimap.md](features/qol-minimap.md), [qol-fullscreen.md](features/qol-fullscreen.md), [qol-settings-drawing.md](features/qol-settings-drawing.md), [ui-menus-pause.md](features/ui-menus-pause.md)
- 2026-10-30T20:56:12+01:00 — Agent 20261030-qolmenu-C (corner HUD at picture corners): while the cutscene, director or scanner camera shows the 5:3 picture the corner HUD parts are drawn at the picture's corners, using the same camera kind as the view region (`qol::cameraWidens`). See [qol-fullscreen](features/qol-fullscreen.md).
- 2026-10-30T21:03:03+01:00 — Agent 20261030-qolmenu-B (two-line Controls bars): the Controls list shows four bars per page with the action on top and the two slots in a smaller line underneath (`LayoutScene::scales`), one hit area per slot, and the page cache key includes element scales (tests qol-menu-list-page, qol-menu-redraw). See [qol-settings-drawing](features/qol-settings-drawing.md).
