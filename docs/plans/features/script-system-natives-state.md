# Script system: natives, flags and progress

## 1. Scope

The flag, placement, kind-gated and progress natives of batches 26, 27 and 31 whose effects are recorded by instance name and not read yet. The rest of the natives: [script-system-natives.md](script-system-natives.md).

## 2. Status

IN_PROGRESS. The batch 26 flag natives, PositionObject / PositionPlayer (message 8 shows the player's node) and the batch 27 progress, save and stud natives run as stand-ins that record their effect; the readers are the open items below.

## 3. Verified behaviour

- The handlers are read and transcribed as far as the stand-ins below state. Handlers: [script-natives-flags.md](../../re/script-natives-flags.md), [script-natives-progress.md](../../re/script-natives-progress.md).

## 4. Stand-ins

- SCRIPT-S238 `EnableShadow` (kinds 27, 149, 121, 159, 242) and `EnableSwitch` (message 8, use flag, switch state 3 for kinds 94, 139, 100, 58) record their state by instance name; nothing reads it. needs: SCRIPT-297. uses: EnableShadow, EnableSwitch. evidence: [object-kinds.md §2026-10-19T07:55:07](../../re/object-kinds.md#ts-20261019-075507).
- SCRIPT-S18 The six flag natives and ResetObject / SkipToFinalState store their value or message by instance name without the original's checks (null object, player alias 0x22dacc, instance exists) and count as stand-ins. Nothing reads them yet. needs: SCRIPT-56. evidence: [script-natives-flags.md §2026-10-14T14:16](../../re/script-natives-flags.md#ts-20261014-141621).
- SCRIPT-S20 Nine batch 27 natives (UnlockAbility, LockAbility, UnlockScanner, SetParachuteDeployHeight, SetRaceInProgress, DisableSuperBuiltItSave, EnableStudsByGroup, GiveReward, AddStuds) only record; nothing reads it yet. EnableSuperBricks is read by the debris super bricks (vehicles-damage, VEH-123). needs: SCRIPT-64. evidence: [script-natives-progress.md §2026-10-14T14:41](../../re/script-natives-progress.md#ts-20261014-144155).
- SCRIPT-S21 UnlockCharacter sets the group byte and the unlock bit (and the "new" bit) of ids 9, 16, 19, 29, 39, 49, 59, 69 only; the second bit, the recursion into the character table, the unlock popup and the entity message are missing. needs: SCRIPT-61. evidence: [script-natives-progress.md §2026-10-14T14:41](../../re/script-natives-progress.md#ts-20261014-144155).
- SCRIPT-S97 The kind list of 0x236b6c (count byte 0x3743ec, kinds 0x515a94) is the one entry read in the oracle (159); who fills it is not known. Only decides 0x21aae0 for non-player kinds. needs: SCRIPT-59. evidence: [script-natives-flags.md §2026-10-14T19:19](../../re/script-natives-flags.md#ts-20261014-191915).
- SCRIPT-S274 Message 8 of PositionObject / PositionPlayer to the player shows the player's scene instance (`takeNodeShow`), except during a cutscene, where the port hides the player by its own cast copy; the request of state 1 is recorded in `PlacementEffects` and read by nothing; the entity enable is a no-op for the player. needs: SCRIPT-335. evidence: [script-natives-flags.md §2026-10-19T23:06:37](../../re/script-natives-flags.md#ts-20261019-230637).
- SCRIPT-S276 PositionPlayer of a seated player moves the vehicle with `placeSeatedVehicle`: the target y without the vehicle's entity +0x70, no vehicle state 5, no driver re-placement. needs: SCRIPT-336. evidence: [script-natives-flags.md §2026-10-19T15:12:32](../../re/script-natives-flags.md#ts-20261019-151232).
- SCRIPT-S22 GetLanguage uses language 1 (English) unless the host sets another (no language option wired); GetPlayerNumber always writes 1.0 (the two globals it compares are unknown). needs: SCRIPT-65. evidence: [script-natives-progress.md §2026-10-14T14:41](../../re/script-natives-progress.md#ts-20261014-144155).

## 5. Open items

- [ ] SCRIPT-56 — The update flag is read by doors now (update loop 0x1a4098, `ObjectNativeState::entityUpdateSkipped`); the other readers of the flag and of the flags/messages (8, 54) the batch 26 natives record are not ported: prompt, targeting, shove, destruction, the 11 mask-0x4 gather sites. blocks: SCRIPT-S18. evidence: [script-natives-flags.md §2026-10-14T18:39](../../re/script-natives-flags.md#ts-20261014-183900).
- [ ] SCRIPT-57 — Kind-gated natives: the kind of every class is read (`objectKind`); ToggleUseable, EnableSwitch (use-zone gate), EnableShadow and enableobjectfade run as stand-ins; left: DistanceFadeMid and the node kind gates (SCRIPT-282). needs: SCRIPT-282. blocks: SCRIPT-S238. evidence: [object-kinds.md §2026-10-19T07:55:07](../../re/object-kinds.md#ts-20261019-075507).
- [ ] SCRIPT-59 — PositionPlayer ground probe: the gather 0x237f30 finds no characters in the port (mask 4 entries missing), so the ray 0x21ab20 (length 5.0 along (0, 1, 0), read at 0x56d424) never runs; the ray origin, descriptor 0x362298 are not transcribed. blocks: SCRIPT-S97. evidence: [script-natives-flags.md §2026-10-14T19:19](../../re/script-natives-flags.md#ts-20261014-191915).
- [ ] SCRIPT-335 — Consumer of message 8 to the player: the state machine request of state 1 (hook `[0x377194 + 4]`, pending byte ctrl+0x64); the node show is done. blocks: SCRIPT-S274. evidence: [script-natives-flags.md §2026-10-19T15:12:32](../../re/script-natives-flags.md#ts-20261019-151232).
- [ ] SCRIPT-336 — Vehicle branch of PositionPlayer: entity +0x70 of a vehicle, the 0x29f014 test, state 5 with 0x22b224 / 0x204e98, the driver at +0x84. blocks: SCRIPT-S276. evidence: [script-natives-flags.md §2026-10-19T15:12:32](../../re/script-natives-flags.md#ts-20261019-151232).
- [ ] SCRIPT-60 — ResetCarriedThing needs the carry system; VehicleSetUnshovable blocks while the vehicle state is 6 or 7 and needs vehicles. needs: SCRIPT-43. evidence: [script-natives-flags.md §2026-10-14T14:16](../../re/script-natives-flags.md#ts-20261014-141621).
- [ ] SCRIPT-61 — UnlockCharacter: character table 0x372e10 (stride 0x28, fields +4, +0x22, +0x23, run-time filled) for the second bit and the recursive unlock, the unlock popup 0x205c18 and the counter at 0x518024+0x30. blocks: SCRIPT-S21. evidence: [script-natives-progress.md §2026-10-14T14:41](../../re/script-natives-progress.md#ts-20261014-144155).
- [ ] SCRIPT-62 — GiveReward: the reward switch 0x210480 (ids 3..16) and the level collected counter 0x1d9720 for rewards 1 and 2 (capped by level table byte +0x40) are not transcribed. blocks: SCRIPT-S20. evidence: [script-natives-progress.md §2026-10-14T14:41](../../re/script-natives-progress.md#ts-20261014-144155).
- [ ] SCRIPT-64 — No reader for ability bits, scanner unlock, parachute height, race-in-progress, super build-it save and stud group requests. needs: STUD-9. blocks: SCRIPT-S20. evidence: [script-natives-progress.md §2026-10-14T14:41](../../re/script-natives-progress.md#ts-20261014-144155).
- [ ] SCRIPT-65 — The language option byte (0x376adc) is not wired into `ProgressNativeState::setLanguage`, and the globals 0x3772c0 / 0x3772b8 of GetPlayerNumber are unidentified. needs: SAVE-12. blocks: SCRIPT-S22. evidence: [script-natives-progress.md §2026-10-14T14:41](../../re/script-natives-progress.md#ts-20261014-144155).

## 6. Findings summary

- Batch 26 handlers (flag writes, class messages 8 and 54, PositionPlayer): [script-natives-flags.md §2026-10-14T14:16](../../re/script-natives-flags.md#ts-20261014-141621).
- Batch 27 handlers (progress, saves, studs): [script-natives-progress.md §2026-10-14T14:41](../../re/script-natives-progress.md#ts-20261014-144155).
- Message 8 to the player, vehicle branch of PositionPlayer: [script-natives-flags.md §2026-10-19T15:12:32](../../re/script-natives-flags.md#ts-20261019-151232).

## 7. Code and tests

- `src/runtime/script_placement.*`, `placement_message.*`, `natives_objects.*`, `natives_progress.*`. CTest: `script-runner`, `natives-objects`, `natives-objects-real-data`, `natives-progress`, `natives-progress-real-data`, `progress-tables`.
