# UI menus and screens

## 1. Scope

Every UI screen of the game that is not the HUD, the script text screens or the start screens: the pause screen, the bottom-screen round menu and its pages (options, languages, credits, shop, award viewer, code entry, item grids), the map buttons, the character change screen, photo mode and the screens drawn by code. The census of all 55 layouts and their owners is [ui-screens.md](../../re/ui-screens.md); the HUD is [hud-ui.md](hud-ui.md), the text screens [hud-ui-text.md](hud-ui-text.md), the start screens and slot list [front-end.md](front-end.md). Bottom-screen menus and UI are used by clicking them with the mouse (the touch screen), not with extra keyboard keys; pad buttons the original reads keep their bindings ([input.md](input.md)).

## 2. Status

IN_PROGRESS, updated 2026-10-30T17:26:14+01:00. The pause screen with its Options and Languages pages and the item grid is in [ui-menus-pause.md](ui-menus-pause.md), the award viewer in [ui-menus-award-viewer.md](ui-menus-award-viewer.md). Every other menu is not ported. The map buttons of the bottom screen run in `chase-game`: the characters and scan buttons slide in as in the original, a click acts on the first update of a touch inside the original's click areas, a short click on the characters button is a tap (not ported further) and a hold of more than 0.5 s opens the dark characters panel without its portraits; the fast travel pad opens the fast travel page (exit button, six slots) and its exit button closes it. The other buttons' actions are logged as not ported.

## 3. Verified behaviour

- Map buttons: buttons, click areas, touch phases, characters tap and hold, fast travel page and its exit from the code and the oracle's boxes (`map-buttons`, `map-buttons-real-data`): [ui-screens.md §2026-10-28T22:53](../../re/ui-screens.md#ts-20261028-225333).

## 4. Stand-ins

- UI-S8 The map buttons' update is never blocked (its early returns are not read); the character tests are the port's "player free" flag outside the scanner, characters are always available, the scanner never answers ready (so scan, exit and audio do nothing), the level index is the save's level and the count behind the scan flag is the session's StoryProgress; the close of page 1 by the table entry is not modelled. needs: UI-22. evidence: [ui-screens.md §2026-10-28T22:53](../../re/ui-screens.md#ts-20261028-225333).
- UI-S9 The characters panel draws no portraits and the fast travel page no area name (`textWindow`). needs: UI-20, LEVOBJ-130. evidence: [ui-screens.md §2026-10-28T22:53](../../re/ui-screens.md#ts-20261028-225333).

## 5. Open items

Ranked by how soon the play path needs them.

- [ ] UI-20 — The characters panel: the portraits and their pictures, a portrait press with its close timer, and the character change of a tap on the characters button. blocks: UI-S9. evidence: [ui-screens.md §2026-10-28T22:53](../../re/ui-screens.md#ts-20261028-225333).
- [ ] UI-21 — The actions of the map buttons: scan and exit into and out of the scanner, audio, the red bricks page, the freeplay page, the hint text and the freeplay notice. evidence: [ui-screens.md §2026-10-28T22:53](../../re/ui-screens.md#ts-20261028-225333).
- [ ] UI-22 — The gates and inputs of the map buttons' update in the port: its early returns, the character and scanner tests, the source of the hint flag and of the scan, red bricks and freeplay flags, and the close of page 1 by the table entry. blocks: UI-S8. evidence: [ui-screens.md §2026-10-28T22:53](../../re/ui-screens.md#ts-20261028-225333).
- [ ] UI-10 — Character change screen (ui_characterselect/main, 0x168110 from 0x286304). evidence: [ui-screens.md §2026-10-19T20:33](../../re/ui-screens.md#ts-20261019-203310).
- [ ] UI-11 — Shop (0x1a0bf4: codes with codebutton, gifts, postcards, story clips, red bricks, character classes) and the item grid pages (itemsmall, 0x202d54). evidence: [ui-screens.md §2026-10-19T20:33](../../re/ui-screens.md#ts-20261019-203310).
- [ ] UI-13 — Front-end main menu of a returning player (0x1f2460: Continue, Options, fe_attract). evidence: [ui-screens.md §2026-10-19T20:33](../../re/ui-screens.md#ts-20261019-203310).
- [ ] UI-14 — Round menu modes not used by the pause screen (list mode +0xdd = 0 with Menu_Bar_1..5 and Up / Down, TransitionOn_Menu, blank, codes). evidence: [ui-screens-pause.md §2026-10-19T20:34](../../re/ui-screens-pause.md#ts-20261019-203404).
- [ ] UI-15 — Photo mode (blend/photomode, photo_meter, 0x2ea1c0 from the scanner). evidence: [ui-screens.md §2026-10-19T20:33](../../re/ui-screens.md#ts-20261019-203310).
- [ ] UI-17 — DRC wipe layouts (drc_wipe 0x2dc590, mainwindow 0x194834, EnableMinimapWipe). evidence: [ui-screens.md §2026-10-19T20:33](../../re/ui-screens.md#ts-20261019-203310).
- [ ] UI-18 — Screens drawn by code: help, safe crack, wiretap, rocket bar, crosshair, button prompts, customisation, level select. evidence: [ui-screens.md §2026-10-19T20:33](../../re/ui-screens.md#ts-20261019-203310).
- [ ] UI-19 — Unidentified owners: pages 0x364268, 0x364398, 0x3643c8, 0x364740, 0x364ef8, the callers 0x1e5658, 0x1a31c4, 0x217d0c, and the ten layouts without a loader. blocks: UI-S3. evidence: [ui-screens.md §2026-10-19T20:33](../../re/ui-screens.md#ts-20261019-203310).

## 6. Findings summary

- Census of all layouts and code-drawn screens, method and unknowns: [ui-screens.md §2026-10-19T20:33](../../re/ui-screens.md#ts-20261019-203310).
- Map buttons: object 0x519e24, buttons, touch phases, hit test, update, characters panel, fast travel page, oracle boxes: [ui-screens.md §2026-10-28T22:53](../../re/ui-screens.md#ts-20261028-225333).
- Pause screen, its Options and Languages pages and the item grid: [ui-menus-pause.md](ui-menus-pause.md).

## 7. Code and tests

- `src/runtime/map_buttons.*` (`MapButtons`, `nextTouchPhase`), `src/frontend/map_buttons_view.*` (`mapButtonsSetup`, `mapButtonBoxes`, `mapButtonsScene`), `layoutChildBox` in `src/frontend/layout.*`, glue `src/game_fast_travel.cpp` (`updateMapButtons`); `chase-layoutdump`.
- CTest: `map-buttons`, `map-buttons-real-data`.
