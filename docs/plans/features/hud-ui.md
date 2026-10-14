# HUD and UI layouts

## 1. Scope

The in-game HUD (portrait, hearts, stud counter, studs flying to the counter, the bottom-screen map) and the `.bxaml` UI layouts and storyboards that draw it, plus later menus. Front-end screens are [front-end.md](front-end.md); the stud rule is [studs.md](studs.md). Fonts and text are in [assets-formats.md](assets-formats.md).

## 2. Status

IN_PROGRESS, updated 2026-10-14T10:47:00+0200. `chase-game` shows the original's top-screen HUD over a 5:3 3D view and the map on the bottom screen. Hearts, counter, bounce, timing and the flight are recovered; damage, other counters and the real map code are not.

## 3. Verified behaviour

- Portrait (heartsBG_cop plus `sprites/portraits/police`), four hearts (class attribute "Health", default 4), the last heart pulses (`Loop_Heart`), `Gain_Heart` / `Lose_Heart` follow `SetHearts` (0x2316b4): [hud.md](../../re/hud.md). HUD pixels against the oracle: portrait 6.7, hearts 2.5, stud number 5.8 mean abs over the opaque pixels.
- Stud counter: shows the new total at once (no roll-up for pickups), icon bounce for 5 updates, ON 0.5 s / hold 2.0 s / OFF 0.5 s, state sequence equal to a recorded pickup (`hud-real-data`).
- Crossed-out camera icon (`UI_CameraLocked`): bottom left of the top screen at (10, 240 - 10 - height), fade 0.3 s, hold 0.5 s after the last L/R press while the record pose camera is active, fade out 0.3 s; timing read from the original's object (`camera-records`): [camera.md §2026-10-14T10:44](../../re/camera.md#ts-20261014-104400). Not replicated: the conditions 0x21d744 / 0x236008 (CAMERA-S4).
- Flying studs: flight law and sink unprojection reproduce the recorded positions to 2e-3 (`studs-core`).
- `.bxaml` layouts decode; concurrent storyboards and attached layouts are supported by the shared layout code (`LayoutScene::laterStoryboards`, `attachedLayouts`).

## 4. Stand-ins

- HUD-S1 `HudConfig::heartsBackground` and the portrait name are chosen by the caller; the class flag test 0x23a70c is not replicated.
- HUD-S2 The page's `TransitionOn` starts when the HUD is created; the trigger (0x17c0f4) is not traced.
- HUD-S3 The spin clock of the counter icon has the same unrecovered origin as the world studs (STUD-S3).
- HUD-S4 `kStudNumberSpacingFitted` = 0 is fitted to the oracle (5.8 against 38 at -1); the call that sets it is not identified.
- HUD-S5 The bottom-screen map placement (`kMapTexelsPerUnit`, origins, player point) is fitted to four oracle screenshots of lvl_01; `maps/<level>.txt` and the real drawing code are not decoded.
- HUD-S6 `userControl`, `UI_Strike` and `Struggle_Button` of the main page are hidden by hand (the original does not draw them at the gameplay start; why is not traced).
- HUD-S7 The stud icon (HUD and flying) is drawn alpha-tested/blended normally, not with the original's blend state (see STUD-S5).

## 5. Open items

- [x] HUD-1 — Draw the HUD (hearts, stud counter) from the `.bxaml` layouts. Done 2026-10-13. blocks: STUD-2 (done).
- [x] HUD-2 — Studs flying to the counter. Done 2026-10-13 (STUD-S4 removed).
- [ ] HUD-3 — The `.bxaml` track properties and the storyboard property evaluator (not found, time-boxed). evidence: [ui-layouts.md §2026-10-13T09:12](../../re/ui-layouts.md#ts-20261013-0912).
- [x] HUD-4 — The stud counter's display and per-level stud total: display recovered ([hud.md](../../re/hud.md)); the per-level total (0x593420..) is STUD-6.
- [ ] HUD-5 — In-game menus and the main menu are not recovered.
- [ ] HUD-6 — Hearts never change: no damage or healing exists yet. needs: enemies (no feature file yet). evidence: [hud.md](../../re/hud.md).
- [ ] HUD-7 — Vehicle heart image (entity type 0x11a / 0x11c), the +1 heart of unlock 17 (0x236e6c).
- [ ] HUD-8 — Boss hearts page, struggle bar, combat stud counter (`CombatStuds`, `_Tally`), the second counter 0x593430 and its floating number.
- [ ] HUD-9 — Script path `AddStuds` (0x1584e0): shows the old total, adds 1.75 s later. needs: SCRIPT-1. blocks: STUD-4.
- [ ] HUD-10 — Who calls 0x17c0f4 (HUD on), `TransitionOff`, and the generic overlay tick that moves bar states 1..3 (measured only through the stepped oracle clock).
- [ ] HUD-11 — Bottom screen: real map drawing code, `maps/<level>.txt`, mission and collectible icons (needs SCRIPT-1 for missions), the `_2` map pieces, no map picture for lvl_00 (the original shows the city map around the police station with the player marker inside the station: oracle state `police-station`, 2026-10-14), the characters button's behaviour. blocks: SCRIPT-31.
- [ ] HUD-12 — chase-game always uses the two-screen layout, also for levels without a player (placeholder camera).

## 6. Findings summary

- `.bxaml` layout format and storyboards: [ui-layouts.md §2026-10-13T11:00](../../re/ui-layouts.md#ts-20261013-1100), runtime layout [ui-layouts.md §2026-10-13T09:12](../../re/ui-layouts.md#ts-20261013-0912).
- HUD page, hearts, stud bar, flight, map screen and the fit: [hud.md](../../re/hud.md).
- Camera lock icon object, states and drawing 0x1b1150: [camera.md §2026-10-14T10:44](../../re/camera.md#ts-20261014-104400).

## 7. Code and tests

- `src/frontend/hud.*` (`Hud`, `setCameraLockAlpha`), `src/runtime/camera_lock_icon.*`, `src/frontend/minimap.*`, `src/game_hud.hpp` (two-screen presentation), `chase-hudshot`, `LayoutScene` in `src/frontend/layout.*`; `Canvas::clearTransparent`.
- CTest: `hud-real-data` (trace dir `CHASE_HUD_TRACE_DIR`), `game-script-hud`, `studs-core`, `frontend-real-data`. Scenarios: `tools/oracle/hud-*.txt`. Pictures (not committed): `artifacts/review/hud/`.
