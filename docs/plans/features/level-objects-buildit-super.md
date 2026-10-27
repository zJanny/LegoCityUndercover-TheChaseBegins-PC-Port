# Level objects: super build-its

## 1. Scope

Super build-its (`props/prop_superbuildit`, kind 98, with their holders of kind 253) from the helipad of mission 1-3 on: the prompt, the confirmation, the movie, the build, the object messages, the save record and the restore. Build-its of kind 21 are in [level-objects-buildit.md](level-objects-buildit.md), the fast travel pad in [level-objects-fast-travel.md](level-objects-fast-travel.md).

## 2. Status

IN_PROGRESS, updated 2026-10-27T23:40:31+01:00. On `1-3_Helipad` `chase-game` shows the original's prompt page ("Helipad", 300, "Not enough Super Bricks!" or A "Build"), A with enough bricks opens "Are you sure?", OK plays `superbuildit_helipad`, and the update after it builds: the bricks are deducted, the helipad prop and the fast travel pad appear and `1-3_Helipad_Built` starts; the count is the super brick count of the pickups ([studs-super-bricks.md](studs-super-bricks.md)), with a save request after the build. The class answers the script messages 8 (`ResetObject`) and 0xfe (`TriggerObject` with 0) as the handler 0x1bbec4 does, `DisableSuperBuiltItSave` sets the save-off bit, the level state writes the one-byte record of every plate (built bit unless save is off) and a loaded state with the bit set builds the plate again (hides it, shows the prop, activates the build-complete script). The built sound `ATTR_SFXBuilt` plays at the build.

## 3. Verified behaviour

- Super build-it `1-3_Helipad.SBPlate`: value 300 from the holder (the plate's own 100), type 1, zone centre 0 0.1055 0, half 6.596 0.3055 6.596, the positions of the save state on the helipad that needs 300 super bricks and the save state on the helipad with enough super bricks inside, the save state on the fast travel pad of the built helipad outside; build: 302 -> 2 bricks, states 2/4 during the dialog, 2/3 during the movie, 3/3 after (RPC reads; CTests `super-buildit`, `super-buildit-real-data`, `game-super-buildit-prompt`, `game-super-buildit-build`): [super-buildit.md §2026-10-19T19:23:11](../../re/super-buildit.md#ts-20261019-192311).
- The messages, the save byte and the restore are transcribed from the handler 0x1bbec4, the callbacks 0x192608 / 0x192548 and 0x26d530; CTest `superbuildit-messages` (no oracle comparison): [super-buildit.md §2026-10-27T23:40:31+01:00](../../re/super-buildit.md#ts-20261027-234031).

## 4. Stand-ins

- LEVOBJ-S94 The super build-it prompt gate is the port's "player free" flag, closed from the OK answer on by the zone switch `zoneActive` (the OK answer switches it off, `ResetObject` switches it on again); the original's globals, player states (0x24255c) and the characters that block the zone are not tested. needs: LEVOBJ-117. evidence: [super-buildit.md §2026-10-19T19:23:11](../../re/super-buildit.md#ts-20261019-192311).
- LEVOBJ-S95 Closed 2026-10-19: the build reads and lowers the super brick count of the pickups (`LevelSuperBricks::spend`). evidence: [super-buildit.md §2026-10-19T19:23:11](../../re/super-buildit.md#ts-20261019-192311).
- LEVOBJ-S100 The level state writes the record of a plate after the records kept from the slot's saved state (the plate's own kept record is replaced in place); the original writes the records in the scene's object order. evidence: [super-buildit.md §2026-10-27T23:40:31+01:00](../../re/super-buildit.md#ts-20261027-234031).
- LEVOBJ-S101 Message 6 and the sound list message 0xfc are never sent (their sender is not found), so the story fix of the helipad (plate value 0, `1-3_Rainbow_02` completed) does not run and no sound list is handed out. needs: LEVOBJ-129. evidence: [super-buildit.md §2026-10-27T23:40:31+01:00](../../re/super-buildit.md#ts-20261027-234031).

## 5. Open items

- [ ] LEVOBJ-117 — The rest of the super build-it class: the reference step at the reset and the build, the map icon, the progress counter and the level bit of the build, `NewPlayerLocation`, `PlayerFaceObject`, `TempVehicle`, the player state after OK and the characters that block the zone, and the other super build-its (none compared with the oracle). blocks: LEVOBJ-S94. evidence: [super-buildit.md §2026-10-19T19:23:11](../../re/super-buildit.md#ts-20261019-192311).
- [ ] LEVOBJ-129 — Find who sends message 6 and message 0xfc to the objects (not a plain object message send; the scene start, the state restore and the sound loader are candidates) and wire `superBuilditMessage` and the completion of `1-3_Rainbow_02` (`SuperBrickField`, value 0 and `TaskComplete`). blocks: LEVOBJ-S101. evidence: [super-buildit.md §2026-10-27T23:40:31+01:00](../../re/super-buildit.md#ts-20261027-234031).

## 6. Findings summary

- Super build-its (helipad of mission 1-3): class, prompt page, use query, state change, oracle reads: [super-buildit.md §2026-10-19T19:23:11](../../re/super-buildit.md#ts-20261019-192311).
- Messages, save callback, restore callback and the native: [super-buildit.md §2026-10-27T23:40:31+01:00](../../re/super-buildit.md#ts-20261027-234031).

## 7. Code and tests

- Super build-its: `src/runtime/super_buildit.*` (`SuperBuilditSet`), `src/frontend/super_build_prompt.*`, glue `src/game_super_buildit.*`; CTest `super-buildit`, `super-buildit-real-data`, `game-super-buildit-prompt`, `game-super-buildit-build` (slow).
- `src/runtime/superbuildit_messages.*` (messages 8, 0xfe, 6, 0xfc, save byte, records, restore), `LevelStateInput::liveRecords` in `src/save/level_state.*`; CTest `superbuildit-messages`.
