# UI menus: pause screen

## 1. Scope

The pause screen of a level: PauseTop on the top screen, the round menu with page 0 (Resume, Options, Credits, Exit), the Options page, the Languages page in the item grid, the Back and Select shortcuts, the page manager and the pause mode's push and pop. The other UI screens, the map buttons and the census are in [ui-menus.md](ui-menus.md); the award viewer is [ui-menus-award-viewer.md](ui-menus-award-viewer.md). Bottom-screen menus are used by clicking them with the mouse (the touch screen); pad buttons the original reads keep their bindings ([input.md](input.md)).

## 2. Status

IN_PROGRESS, updated 2026-10-30T17:26:14+01:00. Start during gameplay opens the pause screen of the original in `chase-game`: PauseTop on the top screen and the round menu (Resume, Options, Credits, Exit) with the Back and Select shortcuts on the bottom screen, drawn from their layouts with the original's storyboards. The D-pad (or the circle pad through 0x2289b8), taps and swipes with the mouse on the bottom screen turn and select; Resume, B and Start close it and the game goes on. Options opens the Options page: Up / Down change the sound effects and music volume, Invert camera toggles, B saves a change and goes back; the Language row opens the nine languages in the item grid (flag tiles, no row selected at first, a tap selects a row, a tap on it again or A stores it in the options and saves); Credits and Exit do nothing yet; the top screen's counts are not drawn.

## 3. Verified behaviour

- Opening: Start in a free gameplay update pauses, the next update opens the screen and the game stops updating; closing waits for TransitionOff_Round (0.5 s), then the game goes on (`game-pause-menu`, `pause-menu`): [ui-screens-pause.md §2026-10-19T20:34](../../re/ui-screens-pause.md#ts-20261019-203404).
- Page 0 entries, texts, pictures, sounds (0x14 select, 0x11 back and close, 0x12 turn) and the order of page manager, round menu and Start test from the code; slot u shows item (selected + u - move - 2) mod 4 (`pause-menu`).
- Touch: taps within 32 pixels of the three button centres and swipes (more than 30 pixels over 4 updates, 15 updates apart) from 0x1f08d4; the centres are computed from the layout and equal the oracle's reads (54, 119), (159, 119), (264, 119) (`pause-menu-real-data`). A left click that starts on the bottom screen is a touch, not A.
- Options page: rows, texts, Up / Down steps (the music step previewed through the sound effects byte), A, B, the Left / Right reload of the sound effects byte and the Select shortcut per row from the code, checked against the oracle (20261028-uiport-B-1) (`options-page`, `pause-menu`, `game-pause-menu`): [ui-screens-pause.md §2026-10-28T22:51:42](../../re/ui-screens-pause.md#ts-20261028-225142).
- Item grid 0x37600c: tiles, rows, open, close, A, B, page buttons, touches, page change and texts from the code (`item-grid`, `item-grid-real-data`, `pause-menu`); not compared with the oracle: [ui-screens-pause.md §2026-10-30T17:26:14](../../re/ui-screens-pause.md#ts-20261030-172614).
- Picture of the open menu against the oracle (r38-B-1): 7.0 top and 4.3 bottom mean difference, 0.9 on the bottom without the text rows; six sample pixels pinned in `pause-menu-real-data`.

## 4. Stand-ins

- UI-S1 Storyboard playing: a stopped storyboard keeps its last pose, later ones act on top, frames 0xffff, 0xffff place a storyboard at its end (one oracle picture supports the first rule); the storyboard player is part of the property evaluator of HUD-3. evidence: [ui-screens-pause.md §2026-10-19T20:34](../../re/ui-screens-pause.md#ts-20261019-203404).
- UI-S2 The Back and Select shortcuts play their TransitionOn / TransitionOff from 0 when switched; 0x21ef20 is not read. needs: UI-7. evidence: [ui-screens-pause.md §2026-10-19T20:34](../../re/ui-screens-pause.md#ts-20261019-203404).
- UI-S3 The round layout's own `backButton` and `selectButton` references are hidden by hand (the oracle picture shows only the shortcut bar's buttons). needs: UI-19. evidence: [ui-screens-pause.md §2026-10-19T20:34](../../re/ui-screens-pause.md#ts-20261019-203404).
- UI-S4 Credits and Exit play the select sound and do nothing (logged as not ported). needs: UI-3, UI-9. evidence: [ui-screens-pause.md §2026-10-19T20:34](../../re/ui-screens-pause.md#ts-20261019-203404).
- UI-S5 The top screen draws no collectible counts or ability count (the ticker is drawn, SCRIPT-S282); the ability icon is fixed to case 0 and the portrait border to the cop's. needs: UI-5. evidence: [ui-screens-pause.md §2026-10-19T20:34](../../re/ui-screens-pause.md#ts-20261019-203404).
- UI-S6 The page manager is busy only while the screen closes or the item grid opens or closes (its fade and other dialog tests are not modelled). needs: UI-2. evidence: [ui-screens-pause.md §2026-10-19T20:34](../../re/ui-screens-pause.md#ts-20261019-203404).
- UI-S10 The options save on B stores the file at once with the live options and goes back to page 0 in the same update; the file check, the "Checking save data" alert, the question and the I/O time are not modelled; a new game sets options +2 to its slot, with the slot volumes from `--sound-volume` / `--music-volume` (their defaults 10 / 8 are the original default slot's volumes), and its first save is not ported. needs: UI-25. evidence: [ui-screens-pause.md §2026-10-28T22:51:42](../../re/ui-screens-pause.md#ts-20261028-225142).
- UI-S15 Item grid: the click centre of a tile and an arrow (node +0xf0) is taken as the element's centre, A with no row selected does nothing, the grey tint of a disabled row is not drawn. needs: UI-31. evidence: [ui-screens-pause.md §2026-10-30T17:26:14](../../re/ui-screens-pause.md#ts-20261030-172614).
- UI-S7 The Start gates (fade test, 0x374730, 0x3741a4, game state 3, gm+0x234) are "no cutscene and no other hold"; the black interval of the mode push and pop, the paused sounds and animation slots are not reproduced. needs: UI-2. evidence: [ui-screens-pause.md §2026-10-19T20:34](../../re/ui-screens-pause.md#ts-20261019-203404).

## 5. Open items

Ranked by how soon the play path needs them.

- [ ] UI-2 — The pause mode push and pop: fades 0x25d26c(…, 0, 0.5) and 0x22b6d4(0.5, 0.5), the black interval measured on the oracle (about 2 s), the busy tests of 0x2b4c6c, the audio and animation pause of 0x2e6a60 and the Start gates. blocks: UI-S6, UI-S7. evidence: [ui-screens-pause.md §2026-10-19T20:34](../../re/ui-screens-pause.md#ts-20261019-203404).
- [ ] UI-3 — Pause Exit: the quit question (0x20aa94, Yes / No, 0x20eec4(1, 0)), answer Yes sets result 5 and the gameplay update calls 0x1ecdf4(0, 0.5, 4, 2) (where it leads is not read). blocks: UI-S4. evidence: [ui-screens-pause.md §2026-10-19T20:34](../../re/ui-screens-pause.md#ts-20261019-203404).
- [ ] UI-31 — Compare the item grid with the oracle: the Languages page's picture, the open, page move and close timing (the background fade of the close), the click areas of the tiles and arrows (node +0xf0) and A with no row selected. blocks: UI-S15. evidence: [ui-screens-pause.md §2026-10-30T17:26:14](../../re/ui-screens-pause.md#ts-20261030-172614).
- [ ] UI-30 — The language change loads the other language: the text, hint and system files and the cutscene subtitle and movie suffix follow options +1 (the resource reload on a language change); the port reads only the UK files. evidence: [ui-screens-pause.md §2026-10-30T14:16:01](../../re/ui-screens-pause.md#ts-20261030-141601).
- [ ] UI-24 — The red brick extras page of the pause object: what opens it and its rows. evidence: [ui-screens-pause.md §2026-10-19T20:34](../../re/ui-screens-pause.md#ts-20261019-203404).
- [ ] UI-25 — The options save task: the file check, the question and its answers, the child task of state 5 and the way back to page 0. blocks: UI-S10. evidence: [ui-screens-pause.md §2026-10-28T22:51:42](../../re/ui-screens-pause.md#ts-20261028-225142).
- [ ] UI-5 — Pause top content: collected / total of the six items (0x239184, 0x20a630, 0x20a7fc), the ability item and portrait of the class (0x2adadc, tables 0x51b24c, 0x519454, 0x519474), L / R cycling of the class. blocks: UI-S5. evidence: [ui-screens-pause.md §2026-10-19T20:34](../../re/ui-screens-pause.md#ts-20261019-203404).
- [ ] UI-6 — The stud and super brick counters of the pause top (draw 0x300204 pass 6). evidence: [ui-screens-pause.md §2026-10-19T20:34](../../re/ui-screens-pause.md#ts-20261019-203404).
- [ ] UI-7 — Click areas of the Back / Select shortcuts (0x21efd0 callbacks set round menu +0x10c / +0x10d, the item grid's callbacks set its +1 / +2) and their switch 0x21ef20. blocks: UI-S2. evidence: [ui-screens-pause.md §2026-10-19T20:34](../../re/ui-screens-pause.md#ts-20261019-203404).
- [ ] UI-8 — Pause sounds not played: 0x3d of the round opening (units of 0x213140), 0x235684(…, 0x14) at the pause. evidence: [ui-screens-pause.md §2026-10-19T20:34](../../re/ui-screens-pause.md#ts-20261019-203404).
- [ ] UI-9 — Credits (0x213184(0x51456c, 1), credits_*.loc). blocks: UI-S4. evidence: [ui-screens-pause.md §2026-10-19T20:34](../../re/ui-screens-pause.md#ts-20261019-203404).
- [ ] UI-16 — Layout texts of the pause screen sit 2 to 5 pixels off the oracle's (HUD-75 is closed: layout text now has spacing 0; re-compare with the oracle). evidence: [ui-screens-pause.md §2026-10-19T20:34](../../re/ui-screens-pause.md#ts-20261019-203404).

## 6. Findings summary

- Pause screen: opening chain, mode 0x527358, page manager, page 0, round menu 0x376014, touch, close, oracle pictures: [ui-screens-pause.md §2026-10-19T20:34](../../re/ui-screens-pause.md#ts-20261019-203404).
- Languages page: the nine rows in the order of table 0x3759b0 with their name keys and flag pictures, Back at the start, Select while a row is selected, A (select sound, list closes, both shortcuts off), the change of options +1 and the save request once the list has closed, the save callback back to the Options page on the Language row, B (back sound) and the leave request, from the code (`languages-page`, `pause-menu`, `options-page`); not compared with the oracle: [ui-screens-pause.md §2026-10-30T14:16:01](../../re/ui-screens-pause.md#ts-20261030-141601).
- Languages page: vtable 0x364a68, list 0x3759b0, the grid 0x37600c, the language change and its save: [ui-screens-pause.md §2026-10-30T14:16:01](../../re/ui-screens-pause.md#ts-20261030-141601).
- Item grid 0x37600c: create, tiles and old tiles, rows, open, texts, update, page change, close, layout values: [ui-screens-pause.md §2026-10-30T17:26:14](../../re/ui-screens-pause.md#ts-20261030-172614).
- Options page: vtable 0x364888, rows, steps, A, B, the options save task, oracle: [ui-screens-pause.md §2026-10-28T22:51:42](../../re/ui-screens-pause.md#ts-20261028-225142).

## 7. Code and tests

- `src/frontend/round_menu.*` (`RoundMenu`), `src/frontend/pause_menu.*` (`PauseMenu`), `drawBackSelectShortcuts` in `src/frontend/pages.*`, `stickToDpad` in `src/runtime/pad.*`, `OptionsPage` in `src/runtime/options_page.*`, `LanguagesPage` in `src/runtime/languages_page.*`, `ItemGrid` in `src/runtime/item_grid.*` with its picture in `src/frontend/item_grid_view.*`, glue `src/game_pause_menu.cpp`, touch glue `src/game_touch.hpp`; `chase-layoutdump`; `chase-hudshot` commands `pause`, `press` (also UP, DOWN), `touch`.
- CTest: `pause-menu`, `options-page`, `languages-page`, `item-grid`, `item-grid-real-data`, `pause-menu-real-data`, `game-pause-menu` (slow), `game-options-reload`, `layoutdump-option-unknown`, `hudshot-option-press`, `hudshot-option-touch`.
