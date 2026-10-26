# Studs: super bricks

Subtopic of [studs.md](studs.md).

## 1. Scope

The placed super build bricks (`pickup_sbbrick2x2` / `2x4`, pickuptype 103) as the original handles them: the pickup test, the destroy that adds `SuperBuildValue` to the super brick count (0x593430), the flight to the HUD and the rainbow tint of the idle bricks. The on-screen display is [hud-ui-super-bricks.md](hud-ui-super-bricks.md).

## 2. Status

IN_PROGRESS, split from studs.md 2026-10-25, updated 2026-10-25T11:13:08+01:00 (the rainbow tint phase is carried across level loads and the origins of the phase and the camera shift are read). Placed super build bricks are picked up in `chase-game`: the pickup test, the 1.0 s countdown, the add of `SuperBuildValue` to the super brick count (0x593430), the UI sound and the save request; the roof brick of mission 1-3 is `1-3_Rainbow_02` (200). The HUD display, the flight to 6.0 in front of the camera and the rainbow tint of the idle bricks are ported (STUD-18 closed, 2026-10-19). The counter comes from the save slot (STUD-17 closed).

## 3. Verified behaviour

- The pickup test is a distance below 1.5 between the brick node and the player node (not the stud pickup point); the destroy 31 updates after the pickup adds the amount and plays UI sound 14 (amount 10 or more) or 2. `super-brick` replays the oracle's rows (pickup in frame 6, destroy in frame 37, 96 + 200 = 296); `super-brick-real-data`: lvl_01 has 8 placed bricks (200 / 500 / 1500). evidence: [super-bricks.md §2026-10-19T19:20:44](../../re/super-bricks.md#ts-20261019-192044).

## 4. Stand-ins

- STUD-S8 Super bricks: the idle spin takes the port's sprite clock (STUD-S3), the UI sound index of the destroy is played as a global sound id without a position (the bank `*0x3757a8` is taken as the global table). needs: STUD-5. evidence: [super-bricks.md §2026-10-19T20:35:01+02:00](../../re/super-bricks.md#ts-20261019-203501).
- STUD-S9 Super bricks: the player's entity word +0xc bit 4 (the pickup block) counts as clear, an entity has no `TaskComplete` parameter, and a brick is updated after the player's update of the same frame. needs: STUD-16. evidence: [super-bricks.md §2026-10-19T19:20:44](../../re/super-bricks.md#ts-20261019-192044).
- STUD-S12 The brick flight leaves out the camera shift `p - *0x58ef40 + eye` that the original applies before each step (the writer is read, STUD-28); it only matters while the camera moves. needs: STUD-28. evidence: [super-bricks.md §2026-10-19T20:35:01+02:00](../../re/super-bricks.md#ts-20261019-203501).
- STUD-S13 The flight target is 6.0 straight ahead of the camera because cam +0xcc / +0xd0 were read as 0 (gameplay-start, no brick; the original computes y from them). needs: STUD-24. evidence: [super-bricks.md §2026-10-19T20:35:01+02:00](../../re/super-bricks.md#ts-20261019-203501).

## 5. Open items

- [ ] STUD-16 — Placed super bricks, not ported: the `CollectedTriggerObject` message, `ATTR_SFX` / `ATTR_SFX_APPEAR`, the `TaskComplete` record (0x210260, a collected brick stays gone after a reload, restore 0x225cfc), the `IsSpawnDebris` bricks and the children of the super build-its (`pickup_sbbrick1x1`, `pickup_sbplate1x1`); debris super bricks are VEH-123. blocks: STUD-S9. evidence: [super-bricks.md §2026-10-19T19:20:44](../../re/super-bricks.md#ts-20261019-192044)
- [ ] STUD-28 — Port the camera shift into the brick and debris flights (the port's follow camera against the original's camera is unread). blocks: STUD-S12, STUD-S6. evidence: [super-bricks.md §2026-10-25T11:12:12+01:00](../../re/super-bricks.md#ts-20261025-111212)
- [ ] STUD-24 — cam +0xcc / +0xd0 in a state with a brick (the flight target's y). blocks: STUD-S13. evidence: [super-bricks.md §2026-10-19T20:35:01+02:00](../../re/super-bricks.md#ts-20261019-203501)

## 6. Findings summary

- Placed super build bricks, the count 0x593430 (slot +0x5190), the roof brick of mission 1-3: [super-bricks.md §2026-10-19T19:20:44](../../re/super-bricks.md#ts-20261019-192044); the display, flight and tint: [super-bricks.md §2026-10-19T20:35:01+02:00](../../re/super-bricks.md#ts-20261019-203501); the origins of the tint phase and the camera shift: [super-bricks.md §2026-10-25T11:12:12+01:00](../../re/super-bricks.md#ts-20261025-111212).
- The load of the super brick count from the slot (0x20a4d4): [save-format-state.md §2026-10-19T20:24:11](../../re/save-format-state.md#ts-20261019-202411).

## 7. Code and tests

- `SuperBrickField` (`src/runtime/super_brick.*`, `setTotal`), `SuperBrickObject` (`src/assets/level.cpp`), `SuperBrickTint` (`setPhase`; the phase is carried by `game_save::Progress::superBrickGlobals`) and the flight in `SuperBrickField::update`, the display `src/runtime/super_brick_display.*`, glue `src/game_super_bricks.*`, `ProgressNativeState::requestSave`; the counters from the slot: `chase::save::enterLevel`, `StudField::setCounter`, glue `src/game_save.*` (tests `new-game-slot-real-data`, `game-save-reload`).
- Development option `--super-bricks N` (`src/game_options.cpp`, applied once in `src/game_level.cpp` before `superBricks.setTotal`; the bound is the u32 counter 0x593430 and slot field +0x5190, the HUD number has no digit limit in the port); test `game-options-core`.
- CTest: `super-brick`, `super-brick-real-data`, `game-super-brick-freeplay`, `game-super-brick-roof` (slow), `super-brick-display`.
