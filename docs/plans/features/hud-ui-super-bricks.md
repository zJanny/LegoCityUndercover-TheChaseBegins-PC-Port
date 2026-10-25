# HUD: the super brick count display

## 1. Scope

The display the HUD shows when a super brick is picked up: the fader 0x5198b0, the number, the spinning brick icon and the HUD fields at 0x51920c + 0x2a0..0x2ac (the tail of the HUD draw callback 0x193604). Split from [hud-ui.md](hud-ui.md); the pickup, the flight and the count are [studs.md](studs.md).

## 2. Status

IN_PROGRESS, updated 2026-10-25T11:13:08+01:00. `chase-game` shows the display at a pickup of a placed or a debris super brick as the original does: fade-in 0.25 s, the number (yellow, 32 above its place before the slide, bouncing for 5 updates when the destroy changes it), the spinning brick icon, 80 updates in total, then a 0.1 s fade-out. The shown count is the HUD's own word (+0x2a8, 0x5194b4): set at the level load, by the destroy and lowered by a finished helipad build; the display is shown in every update while a super build-it owns the prompt page. The icon is a fitted CPU rasterisation of the original's model (stand-in); the number's font, the HUD camera of the icon and the build-menu placeholders are open.

## 3. Verified behaviour

- The display lasts 80 updates from the pickup (oracle: about 160 vblanks); the number changes from 96 to 296 at the destroy 31 updates after the pickup; the digits' top row is y 16 and all 72 oracle yellow pixels are covered; the icon's silhouette matches the oracle's with IoU 0.897 and mean abs 12.4 over 15 pictures. Tests `super-brick-display`, `super-brick-display-real-data`. evidence: [super-bricks.md §2026-10-19T20:35:01+02:00](../../re/super-bricks.md#ts-20261019-203501)
- The debris super bricks show the display at their collect (0x2c4f78 -> 0x2144f8) and change the number when their value is added 1.0 s later (0x29c090 -> 0x262778); the HUD starts the number at the count (0x17bda4); the pending add +0x2ac has no writer. Test `debris-pickup`. evidence: [super-bricks.md §2026-10-20T22:04:29+02:00](../../re/super-bricks.md#ts-20261020-220429)
- The helipad build lowers the shown count by the plate's value together with the counter (302 -> 2 in the oracle) and shows nothing; the display is kept shown by the prompt (0x37588c owner) and ends 80 updates after it. Tests `super-brick-display` (`spend`, `heldByPrompt`), `game-super-buildit-prompt`, `game-super-buildit-build`. evidence: [super-bricks.md §2026-10-25T11:12:12+01:00](../../re/super-bricks.md#ts-20261025-111212)
- The spin angle and the tint phase start at 0 and are never reset by a level load; the run carries them (`SuperBrickGlobals`, test `super-brick-display` `globalsCarry`). evidence: [super-bricks.md §2026-10-25T11:12:12+01:00](../../re/super-bricks.md#ts-20261025-111212)
- The fader is the state machine shared with the camera lock icon (`runtime::CameraLockIcon` with separate fade times and a hold of -1). evidence: [super-bricks.md §2026-10-19T20:35:01+02:00](../../re/super-bricks.md#ts-20261019-203501)

## 4. Stand-ins

- HUD-S74 The icon is rasterised on the CPU with a fitted scale (47 px per unit), offset (0, -7), one light factor and the texture without its second slot, not through the HUD camera, lights and material of 0x27b3e4. needs: HUD-79. evidence: [super-bricks.md §2026-10-19T20:35:01+02:00](../../re/super-bricks.md#ts-20261019-203501).
- HUD-S75 The number uses the front-end font with a one pixel black shadow and no bounce scale; the font 0x3757a0 and the calls 0x235cb0 / 0x235c9c are unidentified. needs: HUD-80. evidence: [super-bricks.md §2026-10-19T20:35:01+02:00](../../re/super-bricks.md#ts-20261019-203501).
- HUD-S76 The display counts updates from 1000 with the HUD update (the original's count 0x241098 is clock-derived), and a hide during the fade-in continues the fade value. needs: HUD-84. evidence: [super-bricks.md §2026-10-25T11:12:12+01:00](../../re/super-bricks.md#ts-20261025-111212).
- HUD-S78 The display's update count and spin stop while the game is held for the super build-it dialog and movie (no HUD update runs); the original's count is clock-derived and may run on. needs: HUD-84. evidence: [super-bricks.md §2026-10-25T11:12:12+01:00](../../re/super-bricks.md#ts-20261025-111212).

## 5. Open items

- [ ] HUD-79 — The icon through the HUD camera, lights and material of 0x27b3e4 (scale, light, texture slots) instead of the fit. blocks: HUD-S74. evidence: [super-bricks.md §2026-10-19T20:35:01+02:00](../../re/super-bricks.md#ts-20261019-203501)
- [ ] HUD-80 — The font 0x3757a0, the effect of 0x235cb0 / 0x235c9c and the bounce scale of the number. blocks: HUD-S75. evidence: [super-bricks.md §2026-10-19T20:35:01+02:00](../../re/super-bricks.md#ts-20261019-203501)
- [ ] HUD-84 — The origin of the clock-derived update count 0x241098 and whether it runs on while the game is paused or held. blocks: HUD-S76, HUD-S78. evidence: [super-bricks.md §2026-10-25T11:12:12+01:00](../../re/super-bricks.md#ts-20261025-111212)
- [ ] HUD-85 — The two placeholders of the super build-it prompt are not drawn (sprite, model icon or both; an oracle picture must tell). needs: HUD-74. evidence: [super-bricks.md §2026-10-25T11:12:12+01:00](../../re/super-bricks.md#ts-20261025-111212)

## 6. Findings summary

- The display, fader, icon, flight and tint: [super-bricks.md §2026-10-19T20:35:01+02:00](../../re/super-bricks.md#ts-20261019-203501).
- The debris pickup, its HUD calls and the pending add: [super-bricks.md §2026-10-20T22:04:29+02:00](../../re/super-bricks.md#ts-20261020-220429).
- The shown count after a build, the prompt flag, the placeholders and the origins of the spin and update count: [super-bricks.md §2026-10-25T11:12:12+01:00](../../re/super-bricks.md#ts-20261025-111212).

## 7. Code and tests

- `SuperBrickDisplay` (`src/runtime/super_brick_display.*`), `SuperBrickDisplayDraw` (`src/frontend/super_brick_display.*`), `Hud::superBrickPickedUp` / `superBrickBanked` / `superBrickSpent`, `SuperBrickGlobals` (kept in `game_save::Progress`), the shared fader `runtime::CameraLockIcon`, glue `src/game_super_bricks.cpp` and `src/game_debris_pickup.cpp`, `runtime::applyHudPendingAdd` (`src/runtime/hud_pending_add.*`, not called), `chase-hudshot` commands `brickcount`, `brick`, `brickbank`.
- CTest: `super-brick-display`, `super-brick-display-real-data`, `debris-pickup`, `game-super-buildit-prompt`, `game-super-buildit-build`, `hudshot-option-brickcount`, `hudshot-option-brick`.
