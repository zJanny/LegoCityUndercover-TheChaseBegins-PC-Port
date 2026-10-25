# Level objects: carrying

## 1. Scope

Carry objects (`props/prop_carry`, object type 122) and carry targets (`props/prop_carrytargetnomesh`, type 123): the pickup with the action button, the carried state, the drop, the placing into a target and the script objects these fire. Mission 1-1 steps 9 and 10 (`1-1_DonutsDEMO`, `1-1_DonutTarget_DEMO`) are the first user.

## 2. Status

IN_PROGRESS, updated 2026-10-25T14:38:56+01:00. Carry objects and targets are found by ObjectType, so the key of mission 1-3 and its keyhole take part; free carry objects and the target of the carried object show the green carry arrow ([script-system-missions-1-3.md](script-system-missions-1-3.md)). `chase-game` picks up `1-1_DonutsDEMO` with A inside its use zone: the player turns to the box, plays the original's `supercarry_pickup` clip, the box hangs on his right-hand joint (its `carryhandle` node on `weaponRight`) from update 16 of the clip, he walks and stands with the carried clips at the original's carrying step (walk step x 2.5), and drops it with A outside a matching target zone or places it with A inside `1-1_DonutTarget_DEMO`'s zone (`supercarry_putdown`, release at update 8 as in the oracle): `1-1_SO_Donuts_PickedUp`, `1-1_Deliver_Donuts` and `1-1_SO_MissionEnd` start as in the original's order. A released box is put where the hand held it, upright with its heading, on the floor under it (message 0x12 read from the code; the floor height comes from the player's ground probe as a stand-in for the ray). Every carry state ends with its clip and the place and drop release at the clip's event track, as measured in the oracle; the effects are missing.

## 3. Verified behaviour

- Class values of the donut box and its target (references, `ATTR_SmallObject` 0, `ResetTime` 5 = 150 updates, target `UseBound`, `ATTR_SFX_DROP` 72); CTest `carry-real-data`.
- Use zone of the box: model bounds widened by (1, 1.2, 1), half (1.761, 1.606, 1.383) as read in the save state carrying the donut box in mission 1-1; axis-aligned, no yaw; the positions where the oracle's A presses worked are inside the zones, the save state in front of the donut delivery target of mission 1-1 is outside the target's zone; CTest `carry-real-data`.
- Press logic, timers (place and drop: release 8 updates after the press, target effect 1 later, state end 26 = the put-down clip), reset after 150 updates; CTest `carry`, `carry-animation-real-data`. evidence: [carrying-animation.md §2026-10-18T10:02:21](../../re/carrying-animation.md#ts-20261018-100221).
- Mission 1-1 steps 9 and 10 run end to end on the real level data; CTest `carry-real-data`.
- Clips, hand joints, handle offset, event updates (the track 0x98d31b7e sends the state event: put-down release 8 = oracle for place and drop, pickup attach 16 derived; every state ends with its clip: pickup 48, place and drop 26, oracle at dt 0.033: 49 and 27) and the carried variants of the role records; CTest `carry-animation-real-data`, `carry`. Walking step with a big object (x 2.5, `0x374368` read in the oracle); CTest `runtime-core`.
- No jump while holding a big object: the B branch of 0x20b984 skips the jump request when the carry test is not 1; a small object still jumps; CTest `carry`. evidence: [carrying.md §2026-10-16T14:53:22](../../re/carrying.md#ts-20261016-145322).
- Carry objects and targets are loaded by ObjectType (the keyhole class `props/prop_keyhole` is a target), the zone comes from the mesh the model loader resolves, the target reads `PlacedObject`; the key of mission 1-3 has the oracle's zone; CTest `carry-real-data`. evidence: [carrying.md §2026-10-25T14:37:21](../../re/carrying.md#ts-20261025-143721).
- A carry target hides its `PlacedObject` while the level loads and enables it on use (post-link 0x18e0f8, state change 0x25e354): the build-it behind `1-3_BasePlate`; CTest `level-start-real-data`, `game-parts-crate-1-3`. evidence: [carrying.md §2026-10-25T16:20:07](../../re/carrying.md#ts-20261025-162007).
- Carry arrows and the carry group (0x16d420, 0x19cf5c): on the key before the pickup, on the keyhole after it, as in the oracle; CTest `carry-arrows`, `game-key-pickup-1-3`. evidence: [carrying.md §2026-10-25T14:37:21](../../re/carrying.md#ts-20261025-143721).
- No sprint while carrying: 0x1abe60 runs the run timer and the sprint only for the run roles 2, 0x6f, 0x83, and the carried run roles are 552 (big) and 151 (small); CTest `runtime-core`, `carry-animation-real-data`. evidence: [carrying.md §2026-10-16T14:13:15](../../re/carrying.md#ts-20261016-141315).

## 4. Stand-ins

- LEVOBJ-S31 The player stands still while a carry state runs (`CarrySet::busy` suppresses the stick); the turn towards the box and the target is done, towards the PlaceObject node is not. evidence: [carrying-animation.md §2026-10-16T09:40](../../re/carrying-animation.md#ts-20261016-094004).
- LEVOBJ-S75 The floor height under a released box is the player's ground probe (`groundHeight`, 3.0 reach) at the hand position; the original casts the ray 0x22b108 (flags 512, mask 3) from 3.0 above to 3.0 below the node. Without a hand pose (no skeleton joint) the old stand-in (target position or player position) is used. needs: LEVOBJ-75. evidence: [carrying-animation.md §2026-10-16T10:18](../../re/carrying-animation.md#ts-20261016-101840).
- LEVOBJ-S73 The carried clip variants assume the player's class bits (ctrl+0x2cd) are clear; the fixed hand matrix for objects without `carryhandle` and the other readers of the carry test are not applied. evidence: [carrying-animation.md §2026-10-16T09:40](../../re/carrying-animation.md#ts-20261016-094004).
- LEVOBJ-S33 Character ability 6 (can lift) is taken as present, ability 7 (strength) as missing, so `ATTR_StrengthOnly` objects cannot be picked up.
- LEVOBJ-S99 Carry arrows (`carry_arrows.cpp`): only enabled carry objects and targets ask for their carry arrow and group (whether a disabled entity gets its class update is not read), and the use hint's arrow hide is asked whenever the A press would use the object or target (the gate of the hint's caller is not read). needs: LEVOBJ-127. evidence: [carrying.md §2026-10-25T14:37:21](../../re/carrying.md#ts-20261025-143721).
- LEVOBJ-S34 The ray test of the pickup message (0x22f134, 3 m) and the interaction-flag gates of the action (ctrl+0x314 bit 6, ctrl+0x316) are not applied; the first matching zone in file order wins.

## 5. Open items

- [ ] LEVOBJ-30 — Measure the update of the attachment in the pickup in free run (the rise of the track 0x98d31b7e at update 16 for a big and 14 for a small object is derived from the clip; a stepped trace of the key of mission 1-3 saw message 0x11 at update 8, [carrying.md §2026-10-25T14:37:21](../../re/carrying.md#ts-20261025-143721)); the end of state 0xa7 and the drop release are measured. evidence: [carrying.md §2026-10-15T00:17](../../re/carrying.md#ts-20261015-001729).
- [ ] LEVOBJ-31 — Locomotion while carrying: the walk step x 2.5, the missing sprint and the missing jump with a big object are done; port the uses that the carry test gates in 0x20614c (0x2256e4 first for a big object), 0x206510 and 0x19de38, the other jump conditions of 0x20b984 (state 0x101, the obstacle probe 0x203c10), and find which press the registry walk 0x1fee94 answers for carry objects. evidence: [carrying.md §2026-10-16T14:53:22](../../re/carrying.md#ts-20261016-145322).
- [ ] LEVOBJ-32 — Effects of the carry system not played: sounds (`carryit_pickup_big_01`, `carryit_putdown_big_01`, `ATTR_SFX_DROP`, `ATTR_SFXReset`, `ATTR_SFXComplete`), the target's `SpawnDebris`, `DropParticle01/02`, `FadeAfterUse`, the blinking and reset effect of a dropped object, the HUD prompt "PICKUP CARRY IT" / "DROP CARRY IT". evidence: [carrying.md §2026-10-15T00:17](../../re/carrying.md#ts-20261015-001729).
- [ ] LEVOBJ-33 — Other carry classes and parameters: `TargetObject01..04`, `TargetBound01..04`, `TriggerOnRespawn`, `CarryTargetType` 1 and 2, `ATTR_CarryType`, the variants in other levels (`prop_carry_walkie_talkie`, `prop_albatross_*`). evidence: [carrying.md §2026-10-15T00:17](../../re/carrying.md#ts-20261015-001729).
- [ ] LEVOBJ-75 — The ray 0x22b108 of the release (which triangle, flags 512 / mask 3) and the fixed matrix of 0x2ce4a4 for objects without `carryhandle`; message 0x12 itself (node position and heading kept, upright, hit point as height) is ported. blocks: LEVOBJ-S75. evidence: [carrying-animation.md §2026-10-16T10:18](../../re/carrying-animation.md#ts-20261016-101840).
- [ ] LEVOBJ-127 — Whether a disabled entity gets its class update (0x2428c0 calls 0x22d5f0 on entity +0x40, not followed), what gates the caller of the use hint 0x1cba8c, and the other callers of the carry arrow and group (0x1ab414, 0x231c78 / 0x231c84, 0x277558, 0x284200, 0x28eec4). blocks: LEVOBJ-S99. evidence: [carrying.md §2026-10-25T14:37:21](../../re/carrying.md#ts-20261025-143721).
- [ ] LEVOBJ-128 — The A press (`CarrySet::useAt`) does not check the object's UseStartActive flag while the carry arrows do; whether the zone walk 0x1fef10 skips zones whose active bit (record +4 bit 4) is clear is not read. evidence: [carrying.md §2026-10-25T14:37:21](../../re/carrying.md#ts-20261025-143721).
- [ ] LEVOBJ-34 — The carried and dropped state is not saved (save record type 122). needs: SAVE-8. evidence: [carrying.md §2026-10-15T00:17](../../re/carrying.md#ts-20261015-001729).

## 6. Findings summary

- The key of mission 1-3, the keyhole, the carry arrows and the held-object matrix (oracle C-1): [carrying.md §2026-10-25T14:37:21](../../re/carrying.md#ts-20261025-143721).
- No jump with a big object, the press bits of 0x20b984 (user report 2026-10-16): [carrying.md §2026-10-16T14:53:22](../../re/carrying.md#ts-20261016-145322).
- No sprint while carrying (user report 2026-10-16), other readers of the carry test: [carrying.md §2026-10-16T14:13:15](../../re/carrying.md#ts-20261016-141315).
- Classes, records, zones, pickup, drop, placing, fired events and unknowns: [carrying.md §2026-10-15T00:17](../../re/carrying.md#ts-20261015-001729).
- Place and drop end with the put-down clip, release at the event track, track to state event chain (oracle): [carrying-animation.md §2026-10-18T10:02:21](../../re/carrying-animation.md#ts-20261018-100221).
- Pickup end by the slot (oracle): [carrying-animation.md §2026-10-16T10:42](../../re/carrying-animation.md#ts-20261016-104210).
- Release (message 0x12), drop hook, end of the states by the animation slot: [carrying-animation.md §2026-10-16T10:18](../../re/carrying-animation.md#ts-20261016-101840).
- State records, clips, hand attachment, event times, carrying step: [carrying-animation.md §2026-10-16T09:40](../../re/carrying-animation.md#ts-20261016-094004).

## 7. Code and tests

- `src/runtime/carry.*` (zones, press logic, timers, actions), `ScriptRunner::fireObjectTriggered`, `assets::modelBounds`; glue `updateCarry` in `src/game_level_update.cpp`, `carry.load` and the object position source in `src/game_level.cpp`.
- `src/runtime/carry_animation.*` (role records, clips, event updates, hand offset), `src/game_carry.*` (hand joint, object transform), `PlayerRig::carriedRole`.
- `src/runtime/carry_arrows.*` (carry arrow list 2 and the carry group), glue `src/game_key_pickup.*`.
- CTest: `carry`, `carry-arrows`, `carry-real-data`, `carry-animation-real-data`, `game-key-pickup-1-3`.
