# Front end (boot to the first level)

## 1. Scope

The screens from power-on to the first level: boot sequencer 0x302080, loading page, logos, TT Games movie, language list, StreetPass and HOME pages, autosave notice, slot list, saving page, loading screen, intro movie bubble and title card, prompt state machine 0x313368, save indicator. Not covered: the main menu and in-game menus (not recovered), movie decoding ([movies-cutscenes.md](movies-cutscenes.md)), save content ([saves-options.md](saves-options.md)).

## 2. Status

IN_PROGRESS, updated 2026-10-13T21:29:01+0200. Squash-merged to main; `chase-game` starts in the front-end mode by default (`--gameplay`, `--level`, `--script`, `--start` start in the level).

## 3. Verified behaviour

- Screen order and triggers, prompt state machine, save indicator 0x1a3d0c / 0x24004c, boot pages, intro bubble and title card, compared with oracle screenshots (mean absolute difference): language 0.22, StreetPass 0.38, HOME 0.20, autosave 0.38, slot list 1.44, boot page 1.7, logos 0.56.
- Tests: `frontend-flow`, `frontend-real-data`, `front-game-smoke`, `front-game-handoff`, `frontshot-real-smoke` (ctest 35/35 at the merge).

## 4. Stand-ins (all labelled in the code)

- FE-S31 The percent of a used slot sits 37 px left of its layout element (`kSlotPercentShiftX`). evidence: [front-end.md §2026-10-14T20:46](../../re/front-end.md#ts-20261014-204620).
- FE-S32 The Delete label is tinted (255, 200, 100) by hand and the extra button sits 6 px right of the oracle's. evidence: [front-end.md §2026-10-14T20:46](../../re/front-end.md#ts-20261014-204620).
- FE-S33 Continue goes through the loading screen with the stand-in duration and no black interval. needs: FE-3. evidence: [front-end.md §2026-10-14T20:46](../../re/front-end.md#ts-20261014-204620).
- FE-S1 Durations: boot pages 3 s, saving page 2 s, loading screen 4 s, title card 6 s or A; emptied slot page interval 0.5 s; save indicator off time 0.3 s; language fade-in 0.25 s.
- FE-S2 `StreetPassEnvironment` takes the oracle's values.
- FE-S3 Without `--front-save` every slot is an empty New Game slot; Continue starts the level of slot byte +6 (SAVE-S4).
- FE-S4 The new game starts in Cherry Tree Hills at the oracle's player position (886.764, 16.589, 1111.166).
- FE-S5 The loading screen shows a flat badge instead of the 3D scene and a fixed tip.
- FE-S6 The title card's hexagon art is a stand-in (not found in one texture); the in-engine cutscene on the top screen under the title card is black.
- FE-S40 The bubble box height follows the line count by the 0x229128 rule and its centre (26.5 + 11.5 per line) is fitted to the one- and two-line boxes; three or more lines are extrapolated. evidence: [front-end.md §2026-10-14T21:36](../../re/front-end.md#ts-20261014-213617).
- FE-S41 The subtitle wrap width is 7/8 of the screen width (280 on the bottom screen; `chase-fmv` uses it on the movie width) instead of the unexplained original value in 272..283. needs: FE-40. evidence: [front-end.md §2026-10-14T21:36](../../re/front-end.md#ts-20261014-213617).

## 5. Open items

- [ ] FE-1 — Existing slots with time, percent and location, Continue, delete: `chase_save` integration is open; the returning-player flow (flag 0: whether rows 4 and 5 still show, which slot rows show) is unknown. needs: SAVE-2, FE-30 (FE-11 and SAVE-9 are closed: Continue = level of slot byte +6 at Player1Start). evidence: [front-end.md §2026-10-13T20:46](../../re/front-end.md#ts-20261013-204607).
- [ ] FE-30 — The delete confirmation page (Y on a used slot: text, B Cancel / Confirm A, result, save write) and a test pinning the used-row texts with the real save; save state at the delete confirmation of a used slot. blocks: FE-1. evidence: [front-end.md §2026-10-14T20:46](../../re/front-end.md#ts-20261014-204620).
- [ ] FE-2 — Logo bottom variants and table suffixes for non-English languages. evidence: [front-end.md §2026-10-13T20:46](../../re/front-end.md#ts-20261013-204607).
- [ ] FE-3 — The loading screen's 3D badge scene (`prop_special_loadingring`, model, lights, camera, spin and star motion) and its tip choice; the async load that row 0 waits for. The loading screen also waits for A in the real game (user observation). blocks: TRANS-3, TRANS-8, TRANS-14, FE-S33. evidence: [front-end.md §2026-10-13T20:46](../../re/front-end.md#ts-20261013-204607).
- [ ] FE-4 — The title card's hexagon art and the in-engine cutscene on the top screen; when the card ends (the oracle route pressed A). needs: MOVIE-2. evidence: [front-end.md §2026-10-13T20:46](../../re/front-end.md#ts-20261013-204607).
- [ ] FE-5 — Page-to-page transitions (the StreetPass pages build a new page after 0x240030 reports the old one done; timing unknown). evidence: [front-end.md §2026-10-13T20:46](../../re/front-end.md#ts-20261013-204607).
- [ ] FE-6 — The title layers of the slot screen differ from the oracle by 2.4 mean; whether they move or blend as measured. evidence: [front-end.md §2026-10-13T20:46](../../re/front-end.md#ts-20261013-204607).
- [ ] FE-7 — Which level, position and heading a new game starts with beyond the oracle's player position; the level and heading a new game starts with (Cherry Tree Hills at the oracle's player position). evidence: [front-end.md §2026-10-13T20:46](../../re/front-end.md#ts-20261013-204607).
- [ ] FE-8 — The activation call 0x1e59f8 and the system reads behind 0x329338 / 0x3293b8 / 0x20aa64. evidence: [front-end.md §2026-10-13T20:46](../../re/front-end.md#ts-20261013-204607).
- [ ] FE-9 — The intro movie is stereoscopic: the port shows the first picture of each pair; which eye the second picture is only matters for a stereoscopic display. see MOVIE-1.
- [ ] FE-40 — The effective subtitle wrap width: the clip code gives 288 on the bottom screen but the oracle breaks at 272..283 (port glyph widths); trap 0x23c4a4 during the intro movie, then replace FE-S41. blocks: FE-S41. evidence: [front-end.md §2026-10-14T21:36](../../re/front-end.md#ts-20261014-213617).

## 6. Findings summary

- Screen flow and the boot sequencer: [front-end.md §2026-10-13T17:54](../../re/front-end.md#ts-20261013-175408).
- Prompt state machine, save indicator, boot pages, intro screens: [front-end.md §2026-10-13T20:46](../../re/front-end.md#ts-20261013-204607).
- Intro movie is stereoscopic, 47 of 49 moflex movies alternate eye frames: [front-end.md §2026-10-13T20:58](../../re/front-end.md#ts-20261013-205838), [fmv.md §2026-10-13T20:58](../../re/fmv.md#ts-20261013-205838).

## 7. Code and tests

- `src/frontend/` (`chase_frontend`: Flow state machine, CPU-drawn screens), `src/movie/` (`chase_movie`), `chase-frontshot`, `src/game_front.hpp`.
- Scenario `tools/oracle/front-screens.txt`. Review pictures (native, oracle, compare) were local and are not kept.
