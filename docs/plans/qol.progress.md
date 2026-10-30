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

- 2026-10-29T16:00:51+01:00 — Agent 20261029-qolmenu2-A (settings page drawing): round form scene, scale rule, hit areas and pointer, wheel and key mapping in `src/qol/menu_page.*` and `menu_hit.*` with tests qol-menu-hit and qol-menu-page-real-data, over new port layout capabilities; the user answered the tap circle, hover, name plate and Game label questions; the list form join and the window hook remain. See [qol-settings-drawing](features/qol-settings-drawing.md).
- 2026-10-29T16:16:01+01:00 — Agent 20261029-qolmenu2-B (list form follow-up): merged main, list builders on the shared hit area type, readable names in the text check; two lines per bar (the user's answer) stays open. See [qol-settings-drawing](features/qol-settings-drawing.md).
- 2026-10-29T16:25:44+01:00 — Lead (20261029-qolmenu2 end): merged 20261029-qolmenu2-A (settings page round form, scale and hit areas), 20261029-qolmenu2-B (Controls list, notice and question forms), 20261029-qolmenu2-C (menu text re-measure, pad buttons) and the recorder (hub map in game-runtime.md) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [qol-settings-drawing.md](features/qol-settings-drawing.md), [qol-settings-menu.md](features/qol-settings-menu.md)
- 2026-10-29T18:06:07+01:00 — Agent 20261029-qolhook-A (list form join): `controlsListForm` joins the Controls list, notice and Reset all question into `ListForm` (new `overlay`), Yes/No from gametext; the Controls tab now draws its rows. See [qol-settings-drawing](features/qol-settings-drawing.md).
- 2026-10-29T18:04:17+01:00 — Agent 20261029-qolhook-C (widened view over the whole window): `qol::topViewRegion` takes the window size and widens over the whole window while the bottom screen is hidden ; test qol-view-hook extended. See [qol-bottom-screen](features/qol-bottom-screen.md), [qol-fullscreen](features/qol-fullscreen.md)
- 2026-10-29T18:25:52+01:00 — Agent 20261029-qolhook-B (settings menu window hook): a text-only "PC Settings" button on the slot list opens the settings menu over the whole window, the front end stops behind it, keys, pad, mouse and wheel drive the menu model (`menu_window`, `menu_window_sdl`, hooks in `game_front::run`, tests qol-menu-window and qol-front-menu-real-data); recorded the user's trigger and button answers; the stores behind the open menu and the pause entry remain. See [qol-settings-drawing](features/qol-settings-drawing.md).
- 2026-10-29T19:23:15+01:00 — Lead (20261029-qolhook end): merged 20261029-qolhook-A (Controls list form and notice/question in the settings page), 20261029-qolhook-C (widened view over the whole window), 20261029-qolhook-B (settings menu opened from the slot page "PC Settings" bar, after two review returns) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [qol-settings-drawing.md](features/qol-settings-drawing.md), [qol-settings-menu.md](features/qol-settings-menu.md), [qol-bottom-screen.md](features/qol-bottom-screen.md)
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
