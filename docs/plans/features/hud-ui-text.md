# HUD script text screens

## 1. Scope

The text screens the script natives open over the game: the tip panel, the dialogue box with its portrait and frame, the level and mission heading, the character intro, the information and warning boxes and the remaining text screens (tally, boss hearts, race start). The HUD page and the map are [hud-ui.md](hud-ui.md); the natives are [script-system-hud-natives.md](script-system-hud-natives.md).

## 2. Status

IN_PROGRESS, updated 2026-10-19T23:16:35+02:00. The tip panel, the level and mission heading (ShowLevelText, ShowIntroText; bottom screen), the dialogue box with its portrait and hexagon frame, the character intro and the information box (top screen, box and A icon as the oracle's) are drawn from the script natives.

## 3. Verified behaviour

- Tip text (bottom screen) and dialogue line (top screen): colours, box, icons, fade of five updates and timing of the tips of mission 1-1 measured on the oracle; box, icons and wrap widths from the draw code (0x15b518, 0x1cc550, 0x23c780, 0x229128), box extents equal the oracle pictures to one pixel ([script-text.md §2026-10-14T15:54](../../re/script-text.md#ts-20261014-155429)); the dialogue fades at 5.0 per second in and 0.25 s out.
- Dialogue portrait and frame: position (200 - half text width - 24 - 7, line box centre - 1) from the draw code, `heartsBG_generic` frame then the portrait, both 64x64; frame extents equal the oracle pixels (CTest `script-text-pages-real-data`): [script-text.md §2026-10-14T16:51](../../re/script-text.md#ts-20261014-165143).
- Dialogue background: drawn over the map on the bottom screen by 0x2d9410 with the panel alpha 0x51a2d4 that the tip and BlackoutMap raise, source-alpha blend; CTest `dialog-bg-fade`: [script-text-bottom-screen.md §2026-10-19T23:15:01](../../re/script-text-bottom-screen.md#ts-20261019-231501).
- Tip alpha: the tip is drawn with the panel alpha 0x51a2d4 shared with the dialogue background, written by the update 0x2de284 (0.2 per update up while the tip state is not 0 or BlackoutMap is set, down otherwise), so the tip stays at 1.0 through its 0.25 s fade-out state; CTest `script-text`: [script-text.md §2026-10-14T16:51](../../re/script-text.md#ts-20261014-165143).
- Tip and dialogue text layout from code (pitch 17 = 23 - 6, line tops, 16 px button icons, squared box alpha): tip text and icon pixels equal the oracle, CTest `script-text-pages-real-data`: [script-text.md §2026-10-14T19:25](../../re/script-text.md#ts-20261014-192500).
- Level heading box (ShowLevelText): key from TablesLevels field +8, box at (160, 30), fade 0.5 s in / 5.0 s hold / 0.5 s out; text pixels and box rows equal the oracle picture: [script-text.md §2026-10-14T16:51](../../re/script-text.md#ts-20261014-165143).
- Heading start: ShowLevelText is the last statement of the `startup` script, so the heading begins with the first tip of 1-1 without any restart; ShowIntroText uses the same heading object with a hint text key; ShowCharacterIntro shows the character's name (gametext, `tablescharacters` column 3) in the dialogue box for 4.0 s: [script-text.md §2026-10-14T18:57](../../re/script-text.md#ts-20261014-185739).

## 4. Stand-ins

- HUD-S15 The information box places its first text line at the box top + 3 (measured for three lines only); the warning dialog keeps its earlier fitted button positions. needs: HUD-30. evidence: [script-text.md §2026-10-14T20:46](../../re/script-text.md#ts-20261014-204600).
- HUD-S77 BlackoutMap (`ObjectiveState::mapBlackout`) only raises the panel alpha; its second reader, the direction of the map alpha +0x4a0 at 0x2de3b0, is not ported and the map stays at alpha 1; a released tip stays drawn while BlackoutMap holds the shared panel alpha (the tip draw 0x15b518 was not read for a state test). needs: HUD-83. evidence: [script-text-bottom-screen.md §2026-10-19T23:15:01](../../re/script-text-bottom-screen.md#ts-20261019-231501).

## 5. Open items

- [ ] HUD-15 — Several text kinds at once (a tip during a dialogue, a dialogue during a DCam cutscene) and the tip on the map's top-screen position are not compared with the oracle. evidence: [script-text.md §2026-10-14T15:25](../../re/script-text.md#ts-20261014-152500).
- [ ] HUD-22 — Remaining text screens: ShowTally, ShowBossHearts, ShowRaceStart, ShowPortraitOverVehicle (their layouts and storyboards). blocks: SCRIPT-66. evidence: [script-text.md §2026-10-14T18:57](../../re/script-text.md#ts-20261014-185739).
- [ ] HUD-25 — ShowCharacterIntro also sets the HUD's "Portrait" image to the character (0x1cc8a4); the port only shows the dialogue box with the name (HUD-S1 relates). evidence: [script-text.md §2026-10-14T18:57](../../re/script-text.md#ts-20261014-185739).
- [ ] HUD-27 — Oracle pictures of a one-line dialogue (code says line top 178, the old fit 177) and of a tip or dialogue box at partial alpha (the port squares the box alpha as 0x229128 does, only compared at 240 / 255). evidence: [script-text.md §2026-10-14T19:25](../../re/script-text.md#ts-20261014-192500).
- [ ] HUD-28 — Inline icons for `<start>`, `<select>` (32x16 textures), the 10th icon texture, and the font flag byte bit 3 / rounding byte 0x3766a9 taken as clear / enabled (pictures match). evidence: [script-text.md §2026-10-14T19:25](../../re/script-text.md#ts-20261014-192500).
- [ ] HUD-29 — The round camera-rotate icon drawn below the tip box while the camera tip shows (oracle picture of agent HUD2, 2026-10-14, bottom centre of the top screen) is not drawn; its texture, code and timing are not found. evidence: [script-text.md §2026-10-14T19:25](../../re/script-text.md#ts-20261014-192500).
- [ ] HUD-30 — ShowCharacterIntro against the oracle (states of mission 1-4 from the showroom fight to the roof exist since 2026-11-01; none shows the intro of its caller `1-4_SO_Phase2`, whose trigger is not found yet), an information or warning box with other line counts than three, and the warning dialog's width rule. blocks: HUD-S15. evidence: [script-text.md §2026-10-14T20:46](../../re/script-text.md#ts-20261014-204600).
- [ ] HUD-32 — Does the original's HUD clock (TransitionOn, heart and stud bar animations) keep running while a dialog box is open? The port freezes `hud->update` during a dialog. evidence: [script-text-bottom-screen.md §2026-10-14T21:41](../../re/script-text-bottom-screen.md#ts-20261014-214151).
- [ ] HUD-33 — After the release director camera of mission 1-1 the port shows the driving tip ("Press and hold A…") on the bottom screen, while the original shows the hexagon panel and then the map with no tip at +3, +10 and +30 updates; when the original shows this tip is not measured. needs: MOVIE-24. evidence: [script-text-bottom-screen.md §2026-10-15T17:19](../../re/script-text-bottom-screen.md#ts-20261015-171916)
- [ ] HUD-83 — The map alpha +0x4a0 writer (direction +0x4a4: -1 while a tip is shown, BlackoutMap is set or the counter +0x498 and byte +0x49a are set; 5.0 × dt per update) is not ported; its readers are the map draws of HUD-71. needs: HUD-71. blocks: HUD-S77. uses: BlackoutMap. evidence: [script-text-bottom-screen.md §2026-10-19T23:15:01](../../re/script-text-bottom-screen.md#ts-20261019-231501).

## 6. Findings summary

- Tip text and dialogue objects, measured fade and layout: [script-text.md §2026-10-14T15:25](../../re/script-text.md#ts-20261014-152500); draw code, layout and dialogue fade: [script-text.md §2026-10-14T15:54](../../re/script-text.md#ts-20261014-155429); portrait frame, tip alpha writer, level heading: [script-text.md §2026-10-14T16:51](../../re/script-text.md#ts-20261014-165143); text layout 0x21bc2c, line pitch, text origin, `_16` icons, box alpha: [script-text.md §2026-10-14T19:25](../../re/script-text.md#ts-20261014-192500); dialogue background fade and ShowDialogBG byte: [script-text-bottom-screen.md §2026-10-19T23:15:01](../../re/script-text-bottom-screen.md#ts-20261019-231501).

## 7. Code and tests

- `src/frontend/script_text_pages.*` (`drawTipPage`, `drawDialogueBox`, `drawHeadingBox`, `levelHeadingKey`, `drawDialogueBackground`), `src/runtime/script_text.*`, `src/runtime/dialog_bg_fade.*`.
- CTest: `script-text`, `script-text-real-data`, `script-text-pages-real-data`, `tip-text-shot`, `tip-text-panel`, `game-script-hud`, `dialog-bg-fade`.
