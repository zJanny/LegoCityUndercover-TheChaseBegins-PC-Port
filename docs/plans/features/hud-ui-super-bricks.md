# HUD: the super brick count display

## 1. Scope

The display the HUD shows when a super brick is picked up: the fader 0x5198b0, the number, the spinning brick icon and the HUD fields at 0x51920c + 0x2a0..0x2ac (the tail of the HUD draw callback 0x193604). Split from [hud-ui.md](hud-ui.md); the pickup, the flight and the count are [studs.md](studs.md).

## 2. Status

IN_PROGRESS, updated 2026-10-19T20:35:01+02:00. `chase-game` shows the display at a pickup as the original does: fade-in 0.25 s, the number (yellow, 32 above its place before the slide, bouncing for 5 updates when the destroy changes it), the spinning brick icon, 80 updates in total, then a 0.1 s fade-out. The icon is a fitted CPU rasterisation of the original's model (stand-in); the number's font, the HUD camera of the icon and the build-menu placeholders are open.

## 3. Verified behaviour

- The display lasts 80 updates from the pickup (oracle: about 160 vblanks); the number changes from 96 to 296 at the destroy 31 updates after the pickup; the digits' top row is y 16 and all 72 oracle yellow pixels are covered; the icon's silhouette matches the oracle's with IoU 0.897 and mean abs 12.4 over 15 pictures. Tests `super-brick-display`, `super-brick-display-real-data`. evidence: [super-bricks.md §2026-10-19T20:35:01+02:00](../../re/super-bricks.md#ts-20261019-203501)
- The fader is the state machine shared with the camera lock icon (`runtime::CameraLockIcon` with separate fade times and a hold of -1). evidence: [super-bricks.md §2026-10-19T20:35:01+02:00](../../re/super-bricks.md#ts-20261019-203501)

## 4. Stand-ins

- HUD-S74 The icon is rasterised on the CPU with a fitted scale (47 px per unit), offset (0, -7), one light factor and the texture without its second slot, not through the HUD camera, lights and material of 0x27b3e4. needs: HUD-79. evidence: [super-bricks.md §2026-10-19T20:35:01+02:00](../../re/super-bricks.md#ts-20261019-203501).
- HUD-S75 The number uses the front-end font with a one pixel black shadow and no bounce scale; the font 0x3757a0 and the calls 0x235cb0 / 0x235c9c are unidentified. needs: HUD-80. evidence: [super-bricks.md §2026-10-19T20:35:01+02:00](../../re/super-bricks.md#ts-20261019-203501).
- HUD-S76 The display counts updates from 1000, its spin starts at 0 with the HUD, its shown count is the super brick field's total (the loaded count is STUD-17), and a hide during the fade-in continues the fade value. evidence: [super-bricks.md §2026-10-19T20:35:01+02:00](../../re/super-bricks.md#ts-20261019-203501).

## 5. Open items

- [ ] HUD-79 — The icon through the HUD camera, lights and material of 0x27b3e4 (scale, light, texture slots) instead of the fit. blocks: HUD-S74. evidence: [super-bricks.md §2026-10-19T20:35:01+02:00](../../re/super-bricks.md#ts-20261019-203501)
- [ ] HUD-80 — The font 0x3757a0, the effect of 0x235cb0 / 0x235c9c and the bounce scale of the number. blocks: HUD-S75. evidence: [super-bricks.md §2026-10-19T20:35:01+02:00](../../re/super-bricks.md#ts-20261019-203501)
- [ ] HUD-81 — The flag 0x37588c (the callback calls 0x2144f8 every update while it is set) and the two placeholder sprites drawn at the faders 0x519830 / 0x519870; its writer. needs: HUD-74. evidence: [super-bricks.md §2026-10-19T20:35:01+02:00](../../re/super-bricks.md#ts-20261019-203501)
- [ ] HUD-82 — The origins of the spin angle 0x375890 and of the update count against the original's, and the shown count after a load (+0x2a8). needs: STUD-17. blocks: HUD-S76. evidence: [super-bricks.md §2026-10-19T20:35:01+02:00](../../re/super-bricks.md#ts-20261019-203501)

## 6. Findings summary

- The display, fader, icon, flight and tint: [super-bricks.md §2026-10-19T20:35:01+02:00](../../re/super-bricks.md#ts-20261019-203501).

## 7. Code and tests

- `SuperBrickDisplay` (`src/runtime/super_brick_display.*`), `SuperBrickDisplayDraw` (`src/frontend/super_brick_display.*`), `Hud::superBrickPickedUp` / `superBrickBanked`, the shared fader `runtime::CameraLockIcon`, glue `src/game_super_bricks.cpp`, `chase-hudshot` commands `brickcount`, `brick`, `brickbank`.
- CTest: `super-brick-display`, `super-brick-display-real-data`, `hudshot-option-brickcount`, `hudshot-option-brick`.
