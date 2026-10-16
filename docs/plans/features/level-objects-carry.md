# Level objects: carrying

## 1. Scope

Carry objects (`props/prop_carry`, object type 122) and carry targets (`props/prop_carrytargetnomesh`, type 123): the pickup with the action button, the carried state, the drop, the placing into a target and the script objects these fire. Mission 1-1 steps 9 and 10 (`1-1_DonutsDEMO`, `1-1_DonutTarget_DEMO`) are the first user.

## 2. Status

IN_PROGRESS, updated 2026-10-16T14:53:22+02:00. `chase-game` picks up `1-1_DonutsDEMO` with A inside its use zone: the player turns to the box, plays the original's `supercarry_pickup` clip, the box hangs on his right-hand joint (its `carryhandle` node on `weaponRight`) from update 16 of the clip, he walks and stands with the carried clips at the original's carrying step (walk step x 2.5), and drops it with A outside a matching target zone or places it with A inside `1-1_DonutTarget_DEMO`'s zone (`supercarry_putdown`, release at update 8 as in the oracle): `1-1_SO_Donuts_PickedUp`, `1-1_Deliver_Donuts` and `1-1_SO_MissionEnd` start as in the original's order. A released box is put where the hand held it, upright with its heading, on the floor under it (message 0x12 read from the code; the floor height comes from the player's ground probe as a stand-in for the ray). The pickup ends with its clip (oracle slot trace); the ends of the place and drop states and the effects are stand-ins.

## 3. Verified behaviour

- Class values of the donut box and its target (references, `ATTR_SmallObject` 0, `ResetTime` 5 = 150 updates, target `UseBound`, `ATTR_SFX_DROP` 72); CTest `carry-real-data`.
- Use zone of the box: model bounds widened by (1, 1.2, 1), half (1.761, 1.606, 1.383) as read in `m11-8-donuts-in-hand`; axis-aligned, no yaw; the positions where the oracle's A presses worked are inside the zones, `m11-9-delivery-goal` is outside the target's zone; CTest `carry-real-data`.
- Press logic, timers (place: release 8 updates after the press, target effect 1 later; drop release 4), reset after 150 updates; CTest `carry`.
- Mission 1-1 steps 9 and 10 run end to end on the real level data; CTest `carry-real-data`.
- Clips, hand joints, handle offset, event updates (put-down release 8 = oracle, pickup attach 16 derived, pickup end 48 = clip length, slot replaced at update 49 in the oracle) and the carried variants of the role records; CTest `carry-animation-real-data`, `carry`. Walking step with a big object (x 2.5, `0x374368` read in the oracle); CTest `runtime-core`.
- No jump while holding a big object: the B branch of 0x20b984 skips the jump request when the carry test is not 1; a small object still jumps; CTest `carry`. evidence: [carrying.md §2026-10-16T14:53:22](../../re/carrying.md#ts-20261016-145322).
- No sprint while carrying: 0x1abe60 runs the run timer and the sprint only for the run roles 2, 0x6f, 0x83, and the carried run roles are 552 (big) and 151 (small); CTest `runtime-core`, `carry-animation-real-data`. evidence: [carrying.md §2026-10-16T14:13:15](../../re/carrying.md#ts-20261016-141315).

## 4. Stand-ins

- LEVOBJ-S30 The ends of the place and drop states copy the measured place end (update 17; their clips last 26 updates) and the drop release (4) is the one measurement; the pickup ends with its clip (oracle slot trace, 48 updates). needs: LEVOBJ-30, LEVOBJ-74. evidence: [carrying.md §2026-10-16T10:42](../../re/carrying.md#ts-20261016-104210).
- LEVOBJ-S31 The player stands still while a carry state runs (`CarrySet::busy` suppresses the stick); the turn towards the box and the target is done, towards the PlaceObject node is not. evidence: [carrying.md §2026-10-16T09:40](../../re/carrying.md#ts-20261016-094004).
- LEVOBJ-S75 The floor height under a released box is the player's ground probe (`groundHeight`, 3.0 reach) at the hand position; the original casts the ray 0x22b108 (flags 512, mask 3) from 3.0 above to 3.0 below the node. Without a hand pose (no skeleton joint) the old stand-in (target position or player position) is used. needs: LEVOBJ-75. evidence: [carrying.md §2026-10-16T10:18](../../re/carrying.md#ts-20261016-101840).
- LEVOBJ-S72 The track 0x98d31b7e of the carry clips is taken as the attach and release moment of the hook 0x2ce4a4 (inferred from update numbers, not read). needs: LEVOBJ-74. evidence: [carrying.md §2026-10-16T09:40](../../re/carrying.md#ts-20261016-094004).
- LEVOBJ-S73 The carried clip variants assume the player's class bits (ctrl+0x2cd) are clear; the fixed hand matrix for objects without `carryhandle` and the other readers of the carry test are not applied. evidence: [carrying.md §2026-10-16T09:40](../../re/carrying.md#ts-20261016-094004).
- LEVOBJ-S33 Character ability 6 (can lift) is taken as present, ability 7 (strength) as missing, so `ATTR_StrengthOnly` objects cannot be picked up.
- LEVOBJ-S34 The ray test of the pickup message (0x22f134, 3 m) and the interaction-flag gates of the action (ctrl+0x314 bit 6, ctrl+0x316) are not applied; the first matching zone in file order wins.

## 5. Open items

- [ ] LEVOBJ-30 — Measure the pickup in the oracle (update of the attachment, end of state 0xa7, release of a drop from a second point) once the step-9 waypoint can be reached in a scripted walk; replace LEVOBJ-S30. evidence: [carrying.md §2026-10-15T00:17](../../re/carrying.md#ts-20261015-001729).
- [ ] LEVOBJ-31 — Locomotion while carrying: the walk step x 2.5, the missing sprint and the missing jump with a big object are done; port the uses that the carry test gates in 0x20614c (0x2256e4 first for a big object), 0x206510 and 0x19de38, the other jump conditions of 0x20b984 (state 0x101, the obstacle probe 0x203c10), and find which press the registry walk 0x1fee94 answers for carry objects. evidence: [carrying.md §2026-10-16T14:53:22](../../re/carrying.md#ts-20261016-145322).
- [ ] LEVOBJ-32 — Effects of the carry system not played: sounds (`carryit_pickup_big_01`, `carryit_putdown_big_01`, `ATTR_SFX_DROP`, `ATTR_SFXReset`, `ATTR_SFXComplete`), the target's `SpawnDebris`, `DropParticle01/02`, `FadeAfterUse`, the blinking and reset effect of a dropped object, the HUD prompt "PICKUP CARRY IT" / "DROP CARRY IT". evidence: [carrying.md §2026-10-15T00:17](../../re/carrying.md#ts-20261015-001729).
- [ ] LEVOBJ-33 — Other carry classes and parameters: `TargetObject01..04`, `TargetBound01..04`, `TriggerOnRespawn`, `CarryTargetType` 1 and 2, `ATTR_CarryType`, the variants in other levels (`prop_carry_walkie_talkie`, `prop_albatross_*`). evidence: [carrying.md §2026-10-15T00:17](../../re/carrying.md#ts-20261015-001729).
- [ ] LEVOBJ-74 — The end of the place and drop states (place measured at 17 while the clip lasts 26, same test as the pickup, which ends with its clip) and the table from track hashes to event types; the pickup end is done. Measure: place from `m11-9-delivery-goal` and drop from `m11-8-donuts-in-hand`, ctrace 0x22f560 + memtrace of the slot 0x50 bytes, 60 updates at dt 0.033; pickup ctrace frames 46..54. blocks: LEVOBJ-S30, LEVOBJ-S72. evidence: [carrying.md §2026-10-16T10:42](../../re/carrying.md#ts-20261016-104210).
- [ ] LEVOBJ-75 — The ray 0x22b108 of the release (which triangle, flags 512 / mask 3) and the fixed matrix of 0x2ce4a4 for objects without `carryhandle`; message 0x12 itself (node position and heading kept, upright, hit point as height) is ported. blocks: LEVOBJ-S75. evidence: [carrying.md §2026-10-16T10:18](../../re/carrying.md#ts-20261016-101840).
- [ ] LEVOBJ-34 — The carried and dropped state is not saved (save record type 122). needs: SAVE-8. evidence: [carrying.md §2026-10-15T00:17](../../re/carrying.md#ts-20261015-001729).

## 6. Findings summary

- No jump with a big object, the press bits of 0x20b984 (user report 2026-10-16): [carrying.md §2026-10-16T14:53:22](../../re/carrying.md#ts-20261016-145322).
- No sprint while carrying (user report 2026-10-16), other readers of the carry test: [carrying.md §2026-10-16T14:13:15](../../re/carrying.md#ts-20261016-141315).
- Classes, records, zones, pickup, drop, placing, fired events and unknowns: [carrying.md §2026-10-15T00:17](../../re/carrying.md#ts-20261015-001729).
- Pickup end by the slot (oracle): [carrying.md §2026-10-16T10:42](../../re/carrying.md#ts-20261016-104210).
- Release (message 0x12), drop hook, end of the states by the animation slot: [carrying.md §2026-10-16T10:18](../../re/carrying.md#ts-20261016-101840).
- State records, clips, hand attachment, event times, carrying step: [carrying.md §2026-10-16T09:40](../../re/carrying.md#ts-20261016-094004).

## 7. Code and tests

- `src/runtime/carry.*` (zones, press logic, timers, actions), `ScriptRunner::fireObjectTriggered`, `assets::modelBounds`; glue `updateCarry` in `src/game_level_update.cpp`, `carry.load` and the object position source in `src/game_level.cpp`.
- `src/runtime/carry_animation.*` (role records, clips, event updates, hand offset), `src/game_carry.*` (hand joint, object transform), `PlayerRig::carriedRole`.
- CTest: `carry`, `carry-real-data`, `carry-animation-real-data`.
