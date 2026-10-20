# Vehicles: props a car smashes

## 1. Scope

What a moving vehicle does to the placed props of a level: the smash pass 0x16ce9c -> 0x275c00 (grid query, filter, message 0), the reaction of each prop class (wobbly, traffic light, respawnable, simple destructible, ObjectType 67), the destroy effects and the timing against the oracle. Split from [vehicles.md](vehicles.md) (records, driving, camera, traffic). The car's own damage is in [vehicles-damage.md](vehicles-damage.md); the bricks and the destroyed prop's effects are shared with [vehicles-damage.md](vehicles-damage.md) (debris).

## 2. Status

IN_PROGRESS, updated 2026-10-20T09:32+02:00. A moving vehicle smashes wobbly props (box and mesh entries, so the lamp posts too), traffic lights, respawnable and simple destructible props, and the ObjectType 67 props whose `Destructable` mode a car can break (modes 1 and 4 without `AbilityDestructable`: five props of lvl_01, among them `1-2_RoofProp_03` and `1-3_Lawnmower_Final`); a destroyed prop starts its explosion emitter and bursts its bricks. Kind 67 props now register collision by their own rule (also for the player). No oracle run of a car smashing a kind 67 prop yet (VEH-131).

## 3. Verified behaviour

- The smash pass and per-class reactions (`src/runtime/vehicle_prop_smash.*`, tests `vehicle-prop-smash`, `vehicle-prop-smash-real-data`): the traffic light in frame 19 and the tree in frame 50 as the oracle's 0x23cd80, the lamp post one update early (VEH-130); evidence: [vehicles-prop-smash.md §2026-10-18T19:56:24](../../re/vehicles-prop-smash.md#ts-20261018-195624).
- ObjectType 67 (`src/runtime/prop_hit_sequence.*`, tests `prop-hit-sequence`, `prop-hit-sequence-real-data`): the gates of 0x15e408 and the modes of 0x2398d8 case 0 from the code; the five mode 1 records of lvl_01 (+0xcc = 1, +0xd4 = 1) and the 22 kind 67 grid entries (entry kind and entity box within 1e-3) agree with the oracle census of veh-smash-prop; evidence: [vehicles-prop-smash.md §2026-10-20T09:30:16](../../re/vehicles-prop-smash.md#ts-20261020-093016).

## 4. Stand-ins

- VEH-S105 A wobbly prop with a fall-apart clip is destroyed at once instead of playing it; a destroyed prop is disabled, never deleted. needs: VEH-127. evidence: [vehicles-prop-smash.md §2026-10-18T19:56:24](../../re/vehicles-prop-smash.md#ts-20261018-195624)
- VEH-S102 A prop destroyed by a car has no screen shake (0x22c500), flash light (0x239698), sound (`ATTR_SFX_DESTROYED`) or `Respawn` (0x239898). needs: VEH-126. evidence: [debris.md §2026-10-18T14:40:56](../../re/debris.md#ts-20261018-144056)
- VEH-S106 A scripted DestroyProp of a vehicle sets the destroyed state that every `isDisabled` consumer treats as disabled. needs: VEH-133.
- VEH-S89 The vehicle's node bounds for the smash query are the model's mesh box union (`modelBounds`), checked against the oracle for the trooper only. needs: VEH-130. evidence: [vehicles-physics-props.md §2026-10-16T11:15:22](../../re/vehicles-physics-props.md#ts-20261016-111522)
- VEH-S107 The kind 67 hit rule takes the culled test 0x23de70 as false (whatever `DamageWhenCulled`) and the record byte +0xde bit 0 as clear, and raises no event 0x23cee8. needs: VEH-136. evidence: [vehicles-prop-smash.md §2026-10-20T09:30:16](../../re/vehicles-prop-smash.md#ts-20261020-093016)
- VEH-S108 The smash test takes each prop at its placement matrix: a prop that a script or cutscene moved (`1-2_Dog`) is tested where it was placed. evidence: [vehicles-prop-smash.md §2026-10-20T09:30:16](../../re/vehicles-prop-smash.md#ts-20261020-093016)

## 5. Open items

- [ ] VEH-131 — Check a car smashing a kind 67 prop in the oracle (bricks, clip, disable): no state shows one (in veh-smash-prop the four mode 1 props near streets are disabled); needs a user state with a car before `1-3_Lawnmower_Final` (979.41, 16.79, 1040.27, lvl_01) while mission 1-3 has it enabled, driving towards it. evidence: [vehicles-prop-smash.md §2026-10-20T09:30:16](../../re/vehicles-prop-smash.md#ts-20261020-093016)
- [ ] VEH-134 — The hit machine's mode 3 (lights, area damage) and mode 11 (the hit sequence 0x239c40 / 0x2b7894 with its clips, sounds and stud spawns, the slot loop 0x15e780), and the ability test 0x23a70c -> 0x219a8c of a character sender: read, not ported; a car never reaches them, the miner's rock smash does. evidence: [vehicles-prop-smash.md §2026-10-20T09:30:16](../../re/vehicles-prop-smash.md#ts-20261020-093016)
- [ ] VEH-135 — The physics-body branch of the smash pass (0x275e20: 0x23d610, message 18 through 0x22e884) and the body of kind 67 props with `NoPhysics` 0 (0x23e090: the roadblocks, the rock smashes): not ported, a car neither pushes nor messages them. evidence: [vehicles-prop-smash.md §2026-10-20T09:30:16](../../re/vehicles-prop-smash.md#ts-20261020-093016)
- [ ] VEH-136 — Read the node flags of the culled test 0x23de70, the writers of record +0xde bit 0 and the consumers of the event 0x23cee8 of message 0. blocks: VEH-S107. evidence: [vehicles-prop-smash.md §2026-10-20T09:30:16](../../re/vehicles-prop-smash.md#ts-20261020-093016)
- [ ] VEH-130 — The port destroys `Lampost_05` (mesh entry) in frame 44, the oracle in frame 45 (`veh-smash-prop`, steered); the traffic light and the tree match to the frame. Isolate the cause: the vehicle's world entity box in a tilted pose, the order inside the update or the grid shape. blocks: VEH-S89. evidence: [vehicles-prop-smash.md §2026-10-18T19:56:24](../../re/vehicles-prop-smash.md#ts-20261018-195624)
- [ ] VEH-133 — A vehicle that DestroyProp destroyed counts as disabled in the port (no boarding, no dynamic body); the destroyed-bit tests of the vehicle update 0x16ce9c (0x16cf28, 0x16d04c) and the jump-out check 0x322aa4 were not compared. blocks: VEH-S106. evidence: [script-natives-world.md §2026-10-19T08:53:30](../../re/script-natives-world.md#ts-20261019-085330)
- [ ] VEH-132 — Measure in the oracle that a leprop_simpleindestructible or a door in the car's way does not stop it (by code the message is ignored and only static boxes block); needs a user state with the van before such a prop. evidence: [vehicles-prop-smash.md §2026-10-18T19:56:24](../../re/vehicles-prop-smash.md#ts-20261018-195624)
- [ ] VEH-126 — The rest of a prop's destroy message: screen shake 0x22c500 (`DestroyScreenShakeTime`, `DestroyScreenShakeFactor`), flash light 0x239698 (`FlashColour`), sound `ATTR_SFX_DESTROYED`, `Respawn` 0x239898 and its room list, the event 0x23cee8 (`*0x3775e0`) of message 0 and the check 0x1c54ac for a non-player sender. Size M. blocks: VEH-S102. evidence: [debris.md §2026-10-18T14:40:56](../../re/debris.md#ts-20261018-144056)
- [ ] VEH-127 — The wobble of a prop whose health stays above 0 (0x16b958: spring table 0x593440, 0x2f5af8; 0x2bc370; `ATTR_SFX_HIT`), `WobbleDebrisAmount`, and the fall-apart clip at record +0xc (0x23afc0) used instead of the destroy. Size M. blocks: VEH-S105. evidence: [debris.md §2026-10-18T14:40:56](../../re/debris.md#ts-20261018-144056)
- [ ] VEH-106 — The kinematic branch of the smash pass (0x16ce9c with rigid flag bit 1: 0x24ef70 positive and the record state in 2, 3, 5, 8, 9): traffic vehicles smash props too. Size S. needs: VEH-12. evidence: [vehicles-physics-props.md §2026-10-16T11:15:22](../../re/vehicles-physics-props.md#ts-20261016-111522)

## 6. Findings summary

Which props a car destroys, per class, the grid test and the timing pins: [vehicles-prop-smash.md §2026-10-18T19:56:24](../../re/vehicles-prop-smash.md#ts-20261018-195624). ObjectType 67: its create, collision rule, message 0 gates, the hit machine's modes and the hit sequence, the oracle census: [vehicles-prop-smash.md §2026-10-20T09:30:16](../../re/vehicles-prop-smash.md#ts-20261020-093016). The smash pass itself: [vehicles-physics-props.md §2026-10-16T11:15:22](../../re/vehicles-physics-props.md#ts-20261016-111522).

## 7. Code and tests

`src/runtime/vehicle_prop_smash.{hpp,cpp}` (`VehiclePropSmasher`; glue `LevelSession::smashProps` in `src/game_vehicle.cpp`), `src/runtime/prop_hit_sequence.{hpp,cpp}` (kind 67 message 0), kind 67 collision in `src/assets/object_collision.cpp`; CTest `vehicle-prop-smash`, `vehicle-prop-smash-real-data`, `prop-hit-sequence`, `prop-hit-sequence-real-data`, `collision-real-data`.
