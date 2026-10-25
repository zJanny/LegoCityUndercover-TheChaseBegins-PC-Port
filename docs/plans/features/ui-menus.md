# UI menus and screens

## 1. Scope

Every UI screen of the game that is not the HUD, the script text screens or the start screens: the pause screen, the bottom-screen round menu and its pages (options, languages, credits, shop, award viewer, code entry, item grids), the map buttons, the character change screen, photo mode and the screens drawn by code. The census of all 55 layouts and their owners is [ui-screens.md](../../re/ui-screens.md); the HUD is [hud-ui.md](hud-ui.md), the text screens [hud-ui-text.md](hud-ui-text.md), the start screens and slot list [front-end.md](front-end.md). Bottom-screen menus and UI are used by clicking them with the mouse (the touch screen), not with extra keyboard keys; pad buttons the original reads keep their bindings ([input.md](input.md)).

## 2. Status

IN_PROGRESS, updated 2026-10-19T20:35:16+02:00. Start during gameplay opens the pause screen of the original in `chase-game`: PauseTop on the top screen and the round menu (Resume, Options, Credits, Exit) with the Back and Select shortcuts on the bottom screen, drawn from their layouts with the original's storyboards. The D-pad (or the circle pad through 0x2289b8), taps and swipes with the mouse on the bottom screen turn and select; Resume, B and Start close it and the game goes on. Options, Credits and Exit do nothing yet; the top screen's counts are not drawn. Every other menu is not ported.

## 3. Verified behaviour

- Opening: Start in a free gameplay update pauses, the next update opens the screen and the game stops updating; closing waits for TransitionOff_Round (0.5 s), then the game goes on (`game-pause-menu`, `pause-menu`): [ui-screens.md §2026-10-19T20:34](../../re/ui-screens.md#ts-20261019-203404).
- Page 0 entries, texts, pictures, sounds (0x14 select, 0x11 back and close, 0x12 turn) and the order of page manager, round menu and Start test from the code; slot u shows item (selected + u - move - 2) mod 4 (`pause-menu`).
- Touch: taps within 32 pixels of the three button centres and swipes (more than 30 pixels over 4 updates, 15 updates apart) from 0x1f08d4; the centres are computed from the layout and equal the oracle's reads (54, 119), (159, 119), (264, 119) (`pause-menu-real-data`). A left click that starts on the bottom screen is a touch, not A.
- Picture of the open menu against the oracle (r38-B-1): 7.0 top and 4.3 bottom mean difference, 0.9 on the bottom without the text rows; six sample pixels pinned in `pause-menu-real-data`.

## 4. Stand-ins

- UI-S1 Storyboard playing: a stopped storyboard keeps its last pose, later ones act on top, frames 0xffff, 0xffff place a storyboard at its end (one oracle picture supports the first rule); the storyboard player is part of the property evaluator of HUD-3. evidence: [ui-screens.md §2026-10-19T20:34](../../re/ui-screens.md#ts-20261019-203404).
- UI-S2 The Back and Select shortcuts play their TransitionOn / TransitionOff from 0 when switched; 0x21ef20 is not read. needs: UI-7. evidence: [ui-screens.md §2026-10-19T20:34](../../re/ui-screens.md#ts-20261019-203404).
- UI-S3 The round layout's own `backButton` and `selectButton` references are hidden by hand (the oracle picture shows only the shortcut bar's buttons). needs: UI-19. evidence: [ui-screens.md §2026-10-19T20:34](../../re/ui-screens.md#ts-20261019-203404).
- UI-S4 Options, Credits and Exit play the select sound and do nothing (logged as not ported). needs: UI-3, UI-4, UI-9. evidence: [ui-screens.md §2026-10-19T20:34](../../re/ui-screens.md#ts-20261019-203404).
- UI-S5 The top screen draws no collectible counts or ability count (the ticker is drawn, SCRIPT-S282); the ability icon is fixed to case 0 and the portrait border to the cop's. needs: UI-5. evidence: [ui-screens.md §2026-10-19T20:34](../../re/ui-screens.md#ts-20261019-203404).
- UI-S6 The page manager is busy only while the screen closes (its fade and dialog tests are not modelled). needs: UI-2. evidence: [ui-screens.md §2026-10-19T20:34](../../re/ui-screens.md#ts-20261019-203404).
- UI-S7 The Start gates (fade test, 0x374730, 0x3741a4, game state 3, gm+0x234) are "no cutscene and no other hold"; the black interval of the mode push and pop, the paused sounds and animation slots are not reproduced. needs: UI-2. evidence: [ui-screens.md §2026-10-19T20:34](../../re/ui-screens.md#ts-20261019-203404).

## 5. Open items

Ranked by how soon the play path needs them.

- [ ] UI-1 — Map buttons of the bottom screen (mapbuttons: hint, freeplay, red bricks, characters, audio, exit, scan, fast travel; their click areas and pages); the fast travel map of the helipad is reached from here; its exit button `area_exit` is only a click area in the port (LEVOBJ-S97). blocks: LEVOBJ-120, LEVOBJ-S97. evidence: [ui-screens.md §2026-10-19T20:33](../../re/ui-screens.md#ts-20261019-203310).
- [ ] UI-2 — The pause mode push and pop: fades 0x25d26c(…, 0, 0.5) and 0x22b6d4(0.5, 0.5), the black interval measured on the oracle (about 2 s), the busy tests of 0x2b4c6c, the audio and animation pause of 0x2e6a60 and the Start gates. blocks: UI-S6, UI-S7. evidence: [ui-screens.md §2026-10-19T20:34](../../re/ui-screens.md#ts-20261019-203404).
- [ ] UI-3 — Pause Exit: the quit question (0x20aa94, Yes / No, 0x20eec4(1, 0)), answer Yes sets result 5 and the gameplay update calls 0x1ecdf4(0, 0.5, 4, 2) (where it leads is not read). blocks: UI-S4. evidence: [ui-screens.md §2026-10-19T20:34](../../re/ui-screens.md#ts-20261019-203404).
- [ ] UI-4 — Pause Options page (vtable 0x364888: SFX, Music, Invert camera, Language), the Languages page (0x364a68) and page 0x3645fc (red brick extras); the option values are save fields (SAVE-4). blocks: UI-S4. evidence: [ui-screens.md §2026-10-19T20:33](../../re/ui-screens.md#ts-20261019-203310).
- [ ] UI-5 — Pause top content: collected / total of the six items (0x239184, 0x20a630, 0x20a7fc), the ability item and portrait of the class (0x2adadc, tables 0x51b24c, 0x519454, 0x519474), L / R cycling of the class. blocks: UI-S5. evidence: [ui-screens.md §2026-10-19T20:34](../../re/ui-screens.md#ts-20261019-203404).
- [ ] UI-6 — The stud and super brick counters of the pause top (draw 0x300204 pass 6). evidence: [ui-screens.md §2026-10-19T20:34](../../re/ui-screens.md#ts-20261019-203404).
- [ ] UI-7 — Click areas of the Back / Select shortcuts (0x21efd0 callbacks set round menu +0x10c / +0x10d) and their switch 0x21ef20. blocks: UI-S2. evidence: [ui-screens.md §2026-10-19T20:34](../../re/ui-screens.md#ts-20261019-203404).
- [ ] UI-8 — Pause sounds not played: 0x3d of the round opening (units of 0x213140), 0x235684(…, 0x14) at the pause, the Left / Right volume restore 0x1dca5c. evidence: [ui-screens.md §2026-10-19T20:34](../../re/ui-screens.md#ts-20261019-203404).
- [ ] UI-9 — Credits (0x213184(0x51456c, 1), credits_*.loc). blocks: UI-S4. evidence: [ui-screens.md §2026-10-19T20:34](../../re/ui-screens.md#ts-20261019-203404).
- [ ] UI-10 — Character change screen (ui_characterselect/main, 0x168110 from 0x286304). evidence: [ui-screens.md §2026-10-19T20:33](../../re/ui-screens.md#ts-20261019-203310).
- [ ] UI-11 — Shop (0x1a0bf4: codes with codebutton, gifts, postcards, story clips, red bricks, character classes) and the item grid pages (itemsmall, 0x202d54). evidence: [ui-screens.md §2026-10-19T20:33](../../re/ui-screens.md#ts-20261019-203310).
- [ ] UI-12 — Award viewer (0x1d2db4) and the unloaded awardviewertop/menutop. evidence: [ui-screens.md §2026-10-19T20:33](../../re/ui-screens.md#ts-20261019-203310).
- [ ] UI-13 — Front-end main menu of a returning player (0x1f2460: Continue, Options, fe_attract). evidence: [ui-screens.md §2026-10-19T20:33](../../re/ui-screens.md#ts-20261019-203310).
- [ ] UI-14 — Round menu modes not used by the pause screen (list mode +0xdd = 0 with Menu_Bar_1..5 and Up / Down, TransitionOn_Menu, blank, codes). evidence: [ui-screens.md §2026-10-19T20:34](../../re/ui-screens.md#ts-20261019-203404).
- [ ] UI-15 — Photo mode (blend/photomode, photo_meter, 0x2ea1c0 from the scanner). evidence: [ui-screens.md §2026-10-19T20:33](../../re/ui-screens.md#ts-20261019-203310).
- [ ] UI-17 — DRC wipe layouts (drc_wipe 0x2dc590, mainwindow 0x194834, EnableMinimapWipe). evidence: [ui-screens.md §2026-10-19T20:33](../../re/ui-screens.md#ts-20261019-203310).
- [ ] UI-18 — Screens drawn by code: help, safe crack, wiretap, rocket bar, crosshair, button prompts, customisation, level select. evidence: [ui-screens.md §2026-10-19T20:33](../../re/ui-screens.md#ts-20261019-203310).
- [ ] UI-19 — Unidentified owners: pages 0x364268, 0x364398, 0x3643c8, 0x364740, 0x364ef8, the callers 0x1e5658, 0x1a31c4, 0x217d0c, and the ten layouts without a loader. blocks: UI-S3. evidence: [ui-screens.md §2026-10-19T20:33](../../re/ui-screens.md#ts-20261019-203310).
- [ ] UI-16 — Layout texts of the pause screen sit 2 to 5 pixels off the oracle's (font size field, see HUD-75). evidence: [ui-screens.md §2026-10-19T20:34](../../re/ui-screens.md#ts-20261019-203404).

## 6. Findings summary

- Census of all layouts and code-drawn screens, method and unknowns: [ui-screens.md §2026-10-19T20:33](../../re/ui-screens.md#ts-20261019-203310).
- Pause screen: opening chain, mode 0x527358, page manager, page 0, round menu 0x376014, touch, close, oracle pictures: [ui-screens.md §2026-10-19T20:34](../../re/ui-screens.md#ts-20261019-203404).

## 7. Code and tests

- `src/frontend/round_menu.*` (`RoundMenu`), `src/frontend/pause_menu.*` (`PauseMenu`), `drawBackSelectShortcuts` in `src/frontend/pages.*`, `stickToDpad` in `src/runtime/pad.*`, glue `src/game_pause_menu.cpp`, touch glue `src/game_touch.hpp`; `chase-layoutdump`; `chase-hudshot` commands `pause`, `press`, `touch`.
- CTest: `pause-menu`, `pause-menu-real-data`, `game-pause-menu` (slow), `layoutdump-option-unknown`, `hudshot-option-press`, `hudshot-option-touch`.
