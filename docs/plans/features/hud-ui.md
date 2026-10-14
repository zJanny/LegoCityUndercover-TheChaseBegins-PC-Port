# HUD and UI layouts

## 1. Scope

The in-game HUD (portrait, hearts, stud counter, studs flying to the counter, the bottom-screen map) and the `.bxaml` UI layouts and storyboards that draw it, plus later menus. Front-end screens are [front-end.md](front-end.md); the stud rule is [studs.md](studs.md). Fonts and text are in [assets-formats.md](assets-formats.md).

## 2. Status

IN_PROGRESS, updated 2026-10-14T19:00:00+02:00. Script text: the tip panel, the level and mission heading (ShowLevelText, ShowIntroText; bottom screen), the dialogue box with its portrait and hexagon frame, the character intro and the information box (top screen) are drawn from the script natives. `chase-game` shows the original's top-screen HUD over a 5:3 3D view and the map on the bottom screen. Hearts, counter, bounce, timing and the flight are recovered; damage, other counters and the real map code are not.

## 3. Verified behaviour

- Portrait (heartsBG_cop plus `sprites/portraits/police`), four hearts (class attribute "Health", default 4), the last heart pulses (`Loop_Heart`), `Gain_Heart` / `Lose_Heart` follow `SetHearts` (0x2316b4): [hud.md](../../re/hud.md). HUD pixels against the oracle: portrait 6.7, hearts 2.5, stud number 5.8 mean abs over the opaque pixels.
- Stud counter: shows the new total at once (no roll-up for pickups), icon bounce for 5 updates, ON 0.5 s / hold 2.0 s / OFF 0.5 s, state sequence equal to a recorded pickup (`hud-real-data`).
- Crossed-out camera icon (`UI_CameraLocked`): bottom left of the top screen at (10, 240 - 10 - height), fade 0.3 s, hold 0.5 s after the last L/R press while the record pose camera is active, fade out 0.3 s; timing read from the original's object (`camera-records`): [camera-records-blending.md §2026-10-14T10:44](../../re/camera-records-blending.md#ts-20261014-104400). Not replicated: the conditions 0x21d744 / 0x236008 (CAMERA-S4).
- Flying studs: flight law and sink unprojection reproduce the recorded positions to 2e-3 (`studs-core`).
- Tip text (bottom screen) and dialogue line (top screen): colours, box, icons, fade of five updates and timing of the tips of mission 1-1 measured on the oracle; box, icons and wrap widths from the draw code (0x15b518, 0x1cc550, 0x23c780, 0x229128), box extents equal the oracle pictures to one pixel ([script-text.md §2026-10-14T15:54](../../re/script-text.md#ts-20261014-155429)); the dialogue fades at 5.0 per second in and 0.25 s out.
- Dialogue portrait and frame: position (200 - half text width - 24 - 7, line box centre - 1) from the draw code, `heartsBG_generic` frame then the portrait, both 64x64; frame extents equal the oracle pixels (CTest `script-text-pages-real-data`): [script-text.md §2026-10-14T16:51](../../re/script-text.md#ts-20261014-165143).
- Tip alpha: written by the update 0x2de284 (0.2 per update up while the tip state is not 0, down in state 0), so the tip stays at 1.0 through its 0.25 s fade-out state; CTest `script-text`: [script-text.md §2026-10-14T16:51](../../re/script-text.md#ts-20261014-165143).
- Tip and dialogue text layout from code (pitch 17 = 23 - 6, line tops, 16 px button icons, squared box alpha): tip text and icon pixels equal the oracle, CTest `script-text-pages-real-data`: [script-text.md §2026-10-14T19:25](../../re/script-text.md#ts-20261014-192500).
- Level heading box (ShowLevelText): key from TablesLevels field +8, box at (160, 30), fade 0.5 s in / 5.0 s hold / 0.5 s out; text pixels and box rows equal the oracle picture: [script-text.md §2026-10-14T16:51](../../re/script-text.md#ts-20261014-165143).
- Heading start: ShowLevelText is the last statement of the `startup` script, so the heading begins with the first tip of 1-1 without any restart; ShowIntroText uses the same heading object with a hint text key; ShowCharacterIntro shows the character's name (gametext, `tablescharacters` column 3) in the dialogue box for 4.0 s: [script-text.md §2026-10-14T18:57](../../re/script-text.md#ts-20261014-185739).
- Waypoint marker on the map: position (with the HUD-S10 scale), disc and ring colour measured on `gameplay-start`; CTest `waypoint-marker-position`: [objective-hud.md §2026-10-14T15:59](../../re/objective-hud.md#ts-20261014-155919).
- `.bxaml` layouts decode; concurrent storyboards and attached layouts are supported by the shared layout code (`LayoutScene::laterStoryboards`, `attachedLayouts`).

## 4. Stand-ins

- HUD-S1 `HudConfig::heartsBackground` and the portrait name are chosen by the caller; the class flag test 0x23a70c is not replicated.
- HUD-S2 The page's `TransitionOn` starts when the HUD is created; the trigger (0x17c0f4) is not traced.
- HUD-S3 The spin clock of the counter icon has the same unrecovered origin as the world studs (STUD-S3).
- HUD-S4 `kStudNumberSpacingFitted` = 0 is fitted to the oracle (5.8 against 38 at -1); the call that sets it is not identified.
- HUD-S5 The bottom-screen map placement (`kMapTexelsPerUnit`, origins, player point) is fitted to four oracle screenshots of lvl_01; `maps/<level>.txt` and the real drawing code are not decoded.
- HUD-S6 `userControl`, `UI_Strike` and `Struggle_Button` of the main page are hidden by hand (the original does not draw them at the gameplay start; why is not traced).
- HUD-S7 The stud icon (HUD and flying) is drawn alpha-tested/blended normally, not with the original's blend state (see STUD-S5).
- HUD-S10 The map marker is placed with 0.92 of the map unit, sized 20 px disc and 11 px badge, and the pulse ring (12 to 90 px radius, 1.2 s) is a guess from stepped screenshots; markers outside the screen are not clamped. needs: HUD-16. evidence: [objective-hud.md §2026-10-14T15:59](../../re/objective-hud.md#ts-20261014-155919).
- HUD-S11 The waypoint model is drawn static: its turning police badge and light-column animation are missing. needs: HUD-17. evidence: [objective-hud.md §2026-10-14T15:59](../../re/objective-hud.md#ts-20261014-155919).
- HUD-S14 The information box (InformationBox) reuses the warning dialog's box with an empty second button (no B icon, B ignored); size and button row for an empty text are not measured. needs: HUD-26. evidence: [script-text.md §2026-10-14T18:57](../../re/script-text.md#ts-20261014-185739).
- HUD-S9 The dialogue background is drawn at full opacity as long as ShowDialogBG is on (no fade). needs: HUD-22. evidence: [script-text.md §2026-10-14T15:25](../../re/script-text.md#ts-20261014-152500).

## 5. Open items

- [ ] HUD-3 — The `.bxaml` track properties and the storyboard property evaluator (not found, time-boxed). evidence: [ui-layouts.md §2026-10-13T09:12](../../re/ui-layouts.md#ts-20261013-0912).
- [ ] HUD-5 — In-game menus and the main menu are not recovered.
- [ ] HUD-6 — Hearts never change: no damage or healing exists yet. needs: enemies (no feature file yet). evidence: [hud.md](../../re/hud.md).
- [ ] HUD-7 — Vehicle heart image (entity type 0x11a / 0x11c), the +1 heart of unlock 17 (0x236e6c).
- [ ] HUD-8 — Boss hearts page, struggle bar, combat stud counter (`CombatStuds`, `_Tally`), the second counter 0x593430 and its floating number.
- [ ] HUD-9 — Script path `AddStuds` (0x1584e0): shows the old total, adds 1.75 s later. needs: SCRIPT-1. blocks: STUD-4.
- [ ] HUD-10 — Who calls 0x17c0f4 (HUD on), `TransitionOff`, and the generic overlay tick that moves bar states 1..3 (measured only through the stepped oracle clock).
- [ ] HUD-11 — Bottom screen: real map drawing code, `maps/<level>.txt`, mission and collectible icons (needs SCRIPT-1 for missions), the `_2` map pieces, no map picture for lvl_00 (the original shows the city map: oracle state `police-station`), the characters button's behaviour. Waypoint markers are drawn (HUD-16 for the rest).
- [ ] HUD-16 — Map marker details: the original's drawing code and icon choice (class parameters), pulse clock and curve, badge blend, clamping at the screen edge, the other icons (sub-objective, enemy, point of interest). blocks: HUD-S10. evidence: [objective-hud.md §2026-10-14T15:59](../../re/objective-hud.md#ts-20261014-155919).
- [ ] HUD-17 — Waypoint model animation (turning badge, light column) and the other `props/map_*` classes. blocks: HUD-S11. evidence: [objective-hud.md §2026-10-14T15:59](../../re/objective-hud.md#ts-20261014-155919).
- [ ] HUD-18 — The top-screen waypoint arrow or indicator (none was seen at the mission start); compare when the player is far from the waypoint. evidence: [objective-hud.md §2026-10-14T15:59](../../re/objective-hud.md#ts-20261014-155919).
- [ ] HUD-12 — chase-game always uses the two-screen layout, also for levels without a player (placeholder camera).
- [ ] HUD-15 — Several text kinds at once (a tip during a dialogue, a dialogue during a DCam cutscene) and the tip on the map's top-screen position are not compared with the oracle. evidence: [script-text.md §2026-10-14T15:25](../../re/script-text.md#ts-20261014-152500).
- [ ] HUD-22 — Remaining text screens: ShowTally, ShowBossHearts, ShowRaceStart, ShowPortraitOverVehicle (their layouts and storyboards) and the fade of the dialogue background (the flag byte 0x51a2bd has no reader found). blocks: SCRIPT-66, HUD-S9. evidence: [script-text.md §2026-10-14T18:57](../../re/script-text.md#ts-20261014-185739).
- [ ] HUD-25 — ShowCharacterIntro also sets the HUD's "Portrait" image to the character (0x1cc8a4); the port only shows the dialogue box with the name (HUD-S1 relates). evidence: [script-text.md §2026-10-14T18:57](../../re/script-text.md#ts-20261014-185739).
- [ ] HUD-27 — Oracle pictures of a one-line dialogue (code says line top 178, the old fit 177) and of a tip or dialogue box at partial alpha (the port squares the box alpha as 0x229128 does, only compared at 240 / 255). evidence: [script-text.md §2026-10-14T19:25](../../re/script-text.md#ts-20261014-192500).
- [ ] HUD-28 — Inline icons for `<start>`, `<select>` (32x16 textures), the 10th icon texture, and the font flag byte bit 3 / rounding byte 0x3766a9 taken as clear / enabled (pictures match). evidence: [script-text.md §2026-10-14T19:25](../../re/script-text.md#ts-20261014-192500).
- [ ] HUD-29 — The round camera-rotate icon drawn below the tip box while the camera tip shows (oracle picture `agentHUD2-out/h2.png`, bottom centre of the top screen) is not drawn; its texture, code and timing are not found. evidence: [script-text.md §2026-10-14T19:25](../../re/script-text.md#ts-20261014-192500).
- [ ] HUD-26 — Compare InformationBox, ShowIntroText and ShowCharacterIntro with the oracle: states `info-barrier` and `mission-1-2-start` exist ([oracle-states.md §2026-10-14T19:17](../../re/oracle-states.md#ts-20261014-191730)); a state at the start of mission 1-4 (ShowCharacterIntro) is still missing. blocks: HUD-S14. evidence: [script-text.md §2026-10-14T18:57](../../re/script-text.md#ts-20261014-185739).

## 6. Findings summary

- `.bxaml` layout format and storyboards: [ui-layouts.md §2026-10-13T11:00](../../re/ui-layouts.md#ts-20261013-1100), runtime layout [ui-layouts.md §2026-10-13T09:12](../../re/ui-layouts.md#ts-20261013-0912).
- HUD page, hearts, stud bar, flight, map screen and the fit: [hud.md](../../re/hud.md).
- Tip text and dialogue objects, measured fade and layout: [script-text.md §2026-10-14T15:25](../../re/script-text.md#ts-20261014-152500); draw code, layout and dialogue fade: [script-text.md §2026-10-14T15:54](../../re/script-text.md#ts-20261014-155429); portrait frame, tip alpha writer, level heading: [script-text.md §2026-10-14T16:51](../../re/script-text.md#ts-20261014-165143).; text layout 0x21bc2c, line pitch, text origin, `_16` icons, box alpha: [script-text.md §2026-10-14T19:25](../../re/script-text.md#ts-20261014-192500).
- Camera lock icon object, states and drawing 0x1b1150: [camera-records-blending.md §2026-10-14T10:44](../../re/camera-records-blending.md#ts-20261014-104400).

## 7. Code and tests

- `src/frontend/hud.*` (`Hud`, `setCameraLockAlpha`), `src/runtime/camera_lock_icon.*`, `src/frontend/minimap.*`, `src/game_hud.hpp` (two-screen presentation), `chase-hudshot`, `LayoutScene` in `src/frontend/layout.*`; `Canvas::clearTransparent`; `src/frontend/script_text_pages.*` (`drawTipPage`, `drawDialogueBox`, `drawHeadingBox`, `levelHeadingKey`, `drawDialogueBackground`), `src/runtime/script_text.*`, `script-text-pages-real-data`.
- CTest: `script-text`, `script-text-real-data`, `tip-text-shot`, `tip-text-panel`, `hud-real-data` (trace dir `CHASE_HUD_TRACE_DIR`), `game-script-hud`, `studs-core`, `frontend-real-data`. Scenarios: `tools/oracle/hud-*.txt`. Pictures (not committed): `artifacts/review/hud/`.
