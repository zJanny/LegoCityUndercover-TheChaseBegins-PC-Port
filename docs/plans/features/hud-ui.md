# HUD and UI layouts

## 1. Scope

The in-game HUD (portrait, hearts, stud counter, studs flying to the counter, the bottom-screen map) and the `.bxaml` UI layouts and storyboards that draw it, plus later menus. The script text screens (tips, dialogue, headings, boxes) are [hud-ui-text.md](hud-ui-text.md). Front-end screens are [front-end.md](front-end.md); the stud rule is [studs.md](studs.md); the super brick display is [hud-ui-super-bricks.md](hud-ui-super-bricks.md). Fonts and text are in [assets-formats.md](assets-formats.md).

## 2. Status

IN_PROGRESS, updated 2026-10-19T15:05:12+02:00. The map also shows the sub-objective group icons and their fading group slot with its edge arrow. Script text screens: [hud-ui-text.md](hud-ui-text.md). `chase-game` shows the original's top-screen HUD over a 5:3 3D view and the map on the bottom screen. Hearts, counter, bounce, timing and the flight are recovered; damage, other counters and the real map code are not.

## 3. Verified behaviour

- Portrait (heartsBG_cop plus `sprites/portraits/police`), four hearts (class attribute "Health", default 4), the last heart pulses (`Loop_Heart`), `Gain_Heart` / `Lose_Heart` follow `SetHearts` (0x2316b4): [hud.md](../../re/hud.md). HUD pixels against the oracle: portrait 6.7, hearts 2.5, stud number 5.8 mean abs over the opaque pixels.
- Stud counter: shows the new total at once (no roll-up for pickups), icon bounce for 5 updates, ON 0.5 s / hold 2.0 s / OFF 0.5 s, state sequence equal to a recorded pickup (`hud-real-data`).
- Crossed-out camera icon (`UI_CameraLocked`): bottom left of the top screen at (10, 240 - 10 - height), fade 0.3 s, hold 0.5 s after the last L/R press while the record pose camera is active, fade out 0.3 s; timing read from the original's object (`camera-records`): [camera-records-blending.md §2026-10-14T10:44](../../re/camera-records-blending.md#ts-20261014-104400). Not replicated: the conditions 0x21d744 / 0x236008 (CAMERA-S4).
- Flying studs: flight law and sink unprojection reproduce the recorded positions to 2e-3 (`studs-core`).
- Map screen picture and waypoint marker from the original's code and matrix: placement 0.94977 pixels per unit around (160, 120), edge ellipse with arrow, halo, grow and the two pulse rings (clock 3 s, active 2 s); the bottom screen equals the oracle's (mean difference 0.03 per channel) and the marker and its arrow are pixel-identical at the edge; CTests `map-marker`, `hud-real-data`, `waypoint-marker-position`: [objective-hud-map.md §2026-10-15T19:56](../../re/objective-hud-map.md#ts-20261015-195600).
- `.bxaml` layouts decode; concurrent storyboards and attached layouts are supported by the shared layout code (`LayoutScene::laterStoryboards`, `attachedLayouts`).

## 4. Stand-ins

- HUD-S1 `HudConfig::heartsBackground` and the portrait name are chosen by the caller; the class flag test 0x23a70c is not replicated.
- HUD-S2 The page's `TransitionOn` starts when the HUD is created; the trigger (0x17c0f4) is not traced.
- HUD-S3 The spin clock of the counter icon has the same unrecovered origin as the world studs (STUD-S3).
- HUD-S4 `kStudNumberSpacingFitted` = 0 is fitted to the oracle (5.8 against 38 at -1); the call that sets it is not identified.
- HUD-S6 `userControl`, `UI_Strike` and `Struggle_Button` of the main page are hidden by hand (the original does not draw them at the gameplay start; why is not traced).
- HUD-S7 The stud icon (HUD and flying) is drawn alpha-tested/blended normally, not with the original's blend state (see STUD-S5).
- HUD-S60 The ring lines are white (colour register of the line draw not read), the zoom is fixed at 1.55 and the map matrix, edge factors 0.44 and 0.57 and ring constants are values read from memory (lvl_01 only; `maps/<level>.txt` is not read). needs: HUD-60, HUD-61. evidence: [objective-hud-map.md §2026-10-15T19:56](../../re/objective-hud-map.md#ts-20261015-195600).
- HUD-S71 Group slots are drawn after all waypoint markers instead of in the original's shared slot order (16 slots, 0x2db0dc), and the map alpha (+0x4a0) is 1. needs: HUD-71. evidence: [objective-hud.md §2026-10-19T15:00](../../re/objective-hud.md#ts-20261019-150002).
- HUD-S70 The waypoint column and badge fade as the original's tweens (linear, class times), but the node alpha and colour are applied as a multiplier of the finished fragment's colour and alpha (`Scene::setInstanceFade`); the original's draw path was not read. needs: HUD-70. evidence: [objective-hud.md §2026-10-15T19:58](../../re/objective-hud.md#ts-20261015-195841).
- HUD-S73 Super build-it prompt: name and cost white (oracle), frontendfont. needs: HUD-75. evidence: [super-buildit.md §2026-10-19T19:23:11](../../re/super-buildit.md#ts-20261019-192311).

## 5. Open items

- [ ] HUD-3 — The `.bxaml` track properties and the storyboard property evaluator (not found, time-boxed). evidence: [ui-layouts.md §2026-10-13T09:12](../../re/ui-layouts.md#ts-20261013-0912).
- [ ] HUD-6 — Hearts never change: no damage or healing exists yet. needs: enemies (no feature file yet). evidence: [hud.md](../../re/hud.md).
- [ ] HUD-7 — Vehicle heart image (entity type 0x11a / 0x11c), the +1 heart of unlock 17 (0x236e6c).
- [ ] HUD-8 — Boss hearts page, struggle bar, combat stud counter (`CombatStuds`, `_Tally`), the second counter 0x593430 and its floating number.
- [ ] HUD-9 — Script path `AddStuds` (0x1584e0): shows the old total, adds 1.75 s later. needs: SCRIPT-1. blocks: STUD-4.
- [ ] HUD-10 — Who calls 0x17c0f4 (HUD on), `TransitionOff`, and the generic overlay tick that moves bar states 1..3 (measured only through the stepped oracle clock).
- [ ] HUD-11 — Bottom screen: `maps/<level>.txt` for the other levels (HUD-S60), mission and collectible icons (needs SCRIPT-1 for missions), the `_2` map pieces, no map picture for lvl_00 (the original shows the city map: oracle state `police-station`), the characters button's behaviour.
- [ ] HUD-60 — Map icons of the other types: enemy, point of interest and collectibles (code read, nothing drawn; needs the objects that add slots, callers of 0x2258bc). The group markers (0x2dbb04, slots of types 14/13/15 with their arrows) are drawn since 2026-10-19 ([objective-hud.md §2026-10-19T15:00](../../re/objective-hud.md#ts-20261019-150002)). blocks: HUD-S60. evidence: [objective-hud-map.md §2026-10-15T19:56](../../re/objective-hud-map.md#ts-20261015-195600).
- [ ] HUD-61 — Map zoom levels (3.8, 2.5, 1.55 by struct + 0x368, 0.6 factor, the zoom buttons) and what changes the level. blocks: HUD-S60. evidence: [objective-hud-map.md §2026-10-15T19:56](../../re/objective-hud-map.md#ts-20261015-195600).
- [ ] HUD-73 — The pending add of the HUD update 0x2a5f80 (HUD +0x2ac, applied 35 ticks after the stamp) and its writers (probably the debris super bricks). needs: VEH-123. evidence: [super-bricks.md §2026-10-19T19:20:44](../../re/super-bricks.md#ts-20261019-192044)
- [ ] HUD-71 — The shared 16 map slots of waypoints and groups (allocation 0x1f865c, order, a full table) and the map alpha +0x4a0 are not modelled. blocks: HUD-S71. evidence: [objective-hud.md §2026-10-19T15:00](../../re/objective-hud.md#ts-20261019-150002)
- [ ] HUD-70 — How the node alpha and colour of a tween enter the draw (0x23f708, 0x23f6d0 and their readers), the `UseMarker` classes (other `props/map_*`) and the sound 0x35 of state 0. blocks: HUD-S70, SCRIPT-S232. evidence: [objective-hud.md §2026-10-15T19:58](../../re/objective-hud.md#ts-20261015-195841).
- [ ] HUD-18 — The top-screen waypoint arrow or indicator (none was seen at the mission start); compare when the player is far from the waypoint. evidence: [objective-hud.md §2026-10-14T15:59](../../re/objective-hud.md#ts-20261014-155919).
- [ ] HUD-12 — chase-game always uses the two-screen layout, also for levels without a player (placeholder camera).
- [ ] HUD-74 — The brick models of the super build-it prompt are not drawn. evidence: [super-buildit.md §2026-10-19T19:23:11](../../re/super-buildit.md#ts-20261019-192311).
- [ ] HUD-75 — Layout text ignores `fontSize` (table 0x52ebf8). blocks: HUD-S73. evidence: [super-buildit.md §2026-10-19T19:23:11](../../re/super-buildit.md#ts-20261019-192311).

## 6. Findings summary

- `.bxaml` layout format and storyboards: [ui-layouts.md §2026-10-13T11:00](../../re/ui-layouts.md#ts-20261013-1100), runtime layout [ui-layouts.md §2026-10-13T09:12](../../re/ui-layouts.md#ts-20261013-0912).
- HUD page, hearts, stud bar, flight, map screen and the fit: [hud.md](../../re/hud.md).
- Super brick display: [hud-ui-super-bricks.md](hud-ui-super-bricks.md).
- Sub-objective group icons and slots on the map: [objective-hud.md §2026-10-19T15:00](../../re/objective-hud.md#ts-20261019-150002).
- Camera lock icon object, states and drawing 0x1b1150: [camera-records-blending.md §2026-10-14T10:44](../../re/camera-records-blending.md#ts-20261014-104400).

## 7. Code and tests

- `src/frontend/super_build_prompt.*`, `src/frontend/hud.*` (`Hud`, `setCameraLockAlpha`), `src/runtime/camera_lock_icon.*`, `src/frontend/minimap.*`, `src/game_hud.hpp` (two-screen presentation), `chase-hudshot`, `LayoutScene` in `src/frontend/layout.*`; `Canvas::clearTransparent`.
- CTest: `hud-real-data` (trace dir `CHASE_HUD_TRACE_DIR`), `studs-core`, `frontend-real-data`. Scenarios: `tools/oracle/hud-*.txt`. Pictures (not committed): `artifacts/review/hud/`.
