# Level objects: carrying

## 1. Scope

Carry objects (`props/prop_carry`, object type 122) and carry targets (`props/prop_carrytargetnomesh`, type 123): the pickup with the action button, the carried state, the drop, the placing into a target and the script objects these fire. Mission 1-1 steps 9 and 10 (`1-1_DonutsDEMO`, `1-1_DonutTarget_DEMO`) are the first user.

## 2. Status

IN_PROGRESS, updated 2026-10-16T09:41:31+02:00. `chase-game` picks up `1-1_DonutsDEMO` with A inside its use zone: the player turns to the box, plays the original's `supercarry_pickup` clip, the box hangs on his right-hand joint (its `carryhandle` node on `weaponRight`) from update 16 of the clip, he walks and stands with the carried clips at the original's carrying step (walk step x 2.5), and drops it with A outside a matching target zone or places it with A inside `1-1_DonutTarget_DEMO`'s zone (`supercarry_putdown`, release at update 8 as in the oracle): `1-1_SO_Donuts_PickedUp`, `1-1_Deliver_Donuts` and `1-1_SO_MissionEnd` start as in the original's order. The ends of the states, the fall of a released box and the effects are stand-ins.

## 3. Verified behaviour

- Class values of the donut box and its target (references, `ATTR_SmallObject` 0, `ResetTime` 5 = 150 updates, target `UseBound`, `ATTR_SFX_DROP` 72); CTest `carry-real-data`.
- Use zone of the box: model bounds widened by (1, 1.2, 1), half (1.761, 1.606, 1.383) as read in `m11-8-donuts-in-hand`; axis-aligned, no yaw; the positions where the oracle's A presses worked are inside the zones, `m11-9-delivery-goal` is outside the target's zone; CTest `carry-real-data`.
- Press logic, timers (place: release 8 updates after the press, target effect 1 later; drop release 4), reset after 150 updates; CTest `carry`.
- Mission 1-1 steps 9 and 10 run end to end on the real level data; CTest `carry-real-data`.
- Clips, hand joints, handle offset, event updates (put-down release 8 = oracle, pickup attach 16 derived) and the carried variants of the role records; CTest `carry-animation-real-data`, `carry`. Walking step with a big object (x 2.5, `0x374368` read in the oracle); CTest `runtime-core`.

## 4. Stand-ins

- LEVOBJ-S30 The ends of the three states copy the measured place end (update 17; the pickup ends 9 updates after its attach) and the drop release (4) is the one measurement; the original ends the states on the clip (kind test of 0x2b3494). needs: LEVOBJ-30, LEVOBJ-74. evidence: [carrying.md §2026-10-16T09:40](../../re/carrying.md#ts-20261016-094004).
- LEVOBJ-S31 The player stands still while a carry state runs (`CarrySet::busy` suppresses the stick); the turn towards the box and the target is done, towards the PlaceObject node is not. evidence: [carrying.md §2026-10-16T09:40](../../re/carrying.md#ts-20261016-094004).
- LEVOBJ-S32 A released box is put at the target position (place) or at the player (drop); the original lets it fall to the ground from the hand (message 0x12, ray 0x22b108). needs: LEVOBJ-75. evidence: [carrying.md §2026-10-15T00:17](../../re/carrying.md#ts-20261015-001729).
- LEVOBJ-S72 The track 0x98d31b7e of the carry clips is taken as the attach and release moment of the hook 0x2ce4a4 (inferred from update numbers, not read). needs: LEVOBJ-74. evidence: [carrying.md §2026-10-16T09:40](../../re/carrying.md#ts-20261016-094004).
- LEVOBJ-S73 The carried clip variants assume the player's class bits (ctrl+0x2cd) are clear; the fixed hand matrix for objects without `carryhandle` and the other readers of the carry test are not applied. evidence: [carrying.md §2026-10-16T09:40](../../re/carrying.md#ts-20261016-094004).
- LEVOBJ-S33 Character ability 6 (can lift) is taken as present, ability 7 (strength) as missing, so `ATTR_StrengthOnly` objects cannot be picked up.
- LEVOBJ-S34 The ray test of the pickup message (0x22f134, 3 m) and the interaction-flag gates of the action (ctrl+0x314 bit 6, ctrl+0x316) are not applied; the first matching zone in file order wins.

## 5. Open items

- [ ] LEVOBJ-30 — Measure the pickup in the oracle (update of the attachment, end of state 0xa7, release of a drop from a second point) once the step-9 waypoint can be reached in a scripted walk; replace LEVOBJ-S30. evidence: [carrying.md §2026-10-15T00:17](../../re/carrying.md#ts-20261015-001729).
- [ ] LEVOBJ-31 — Locomotion while carrying: the walk step x 2.5 for a big object is done; the other readers of the carry test 0x225754 (0x203e14 is done; 0x2061f8, 0x206540, 0x20bb7c, 0x219644) and what the player cannot do while carrying are open. evidence: [carrying.md §2026-10-16T09:40](../../re/carrying.md#ts-20261016-094004).
- [ ] LEVOBJ-32 — Effects of the carry system not played: sounds (`carryit_pickup_big_01`, `carryit_putdown_big_01`, `ATTR_SFX_DROP`, `ATTR_SFXReset`, `ATTR_SFXComplete`), the target's `SpawnDebris`, `DropParticle01/02`, `FadeAfterUse`, the blinking and reset effect of a dropped object, the HUD prompt "PICKUP CARRY IT" / "DROP CARRY IT". evidence: [carrying.md §2026-10-15T00:17](../../re/carrying.md#ts-20261015-001729).
- [ ] LEVOBJ-33 — Other carry classes and parameters: `TargetObject01..04`, `TargetBound01..04`, `TriggerOnRespawn`, `CarryTargetType` 1 and 2, `ATTR_CarryType`, the variants in other levels (`prop_carry_walkie_talkie`, `prop_albatross_*`). evidence: [carrying.md §2026-10-15T00:17](../../re/carrying.md#ts-20261015-001729).
- [ ] LEVOBJ-74 — Read what ends states 0xa7, 0xa8 and 0xd6 (kind test of 0x2b3494; the place state ended at update 17 while its clip lasts 26 updates), the table from track hashes to the hook's event types, and the drop hook's event; replace LEVOBJ-S30 and S72. blocks: LEVOBJ-S30, LEVOBJ-S72. evidence: [carrying.md §2026-10-16T09:40](../../re/carrying.md#ts-20261016-094004).
- [ ] LEVOBJ-75 — The release of a carried object: message 0x12 (ray down, node matrix), the matrix it leaves the hand with and where it lands; the fixed matrix of 0x2ce4a4 for objects without `carryhandle`. blocks: LEVOBJ-S32. evidence: [carrying.md §2026-10-16T09:40](../../re/carrying.md#ts-20261016-094004).
- [ ] LEVOBJ-34 — The carried and dropped state is not saved (save record type 122). needs: SAVE-8. evidence: [carrying.md §2026-10-15T00:17](../../re/carrying.md#ts-20261015-001729).

## 6. Findings summary

- Classes, records, zones, pickup, drop, placing, fired events and unknowns: [carrying.md §2026-10-15T00:17](../../re/carrying.md#ts-20261015-001729).
- State records, clips, hand attachment, event times, carrying step: [carrying.md §2026-10-16T09:40](../../re/carrying.md#ts-20261016-094004).

## 7. Code and tests

- `src/runtime/carry.*` (zones, press logic, timers, actions), `ScriptRunner::fireObjectTriggered`, `assets::modelBounds`; glue `updateCarry` in `src/game_level_update.cpp`, `carry.load` and the object position source in `src/game_level.cpp`.
- `src/runtime/carry_animation.*` (role records, clips, event updates, hand offset), `src/game_carry.*` (hand joint, object transform), `PlayerRig::carriedRole`.
- CTest: `carry`, `carry-real-data`, `carry-animation-real-data`.
