# Vehicles: which props a vehicle contact destroys

What a moving vehicle does to the placed props of a level: the grid test for box and mesh entries, the reaction of each prop class to the vehicle's message 0, the bricks it releases and the timing against the port. The first findings (the pass 0x16ce9c -> 0x275c00, the wobbly handler) are in [vehicles-physics-props.md](vehicles-physics-props.md#ts-20261016-111522). Plan: [features/vehicles.md](../plans/features/vehicles.md) (VEH-103 closed here, VEH-129..132 open).

## Current state

- The grid test (mode 1 of 0x237f30) uses the shape at entry +0x18 for both entry kinds: the entity box of a box entry, the root node of the collision item of a mesh entry; the filter 0x2c02c8 always uses the entity box. 393 of the 674 wobbly props register a mesh, all 62 lamp posts among them. [§2026-10-18T19:56:24](#ts-20261018-195624)
- Per class, the vehicle's message 0 (damage 100, kind 11) destroys leprop_wobblydestructible (health), prop_trafficlight (health), prop_respawnabledestructible and leprop_simpledestructible (any damage); doors and leprop_simpleindestructible ignore it; the 57 classes of ObjectType 67 use the hit-sequence machine (not ported). [§2026-10-18T19:56:24](#ts-20261018-195624)
- The prop that `veh-smash-prop` destroys at update 18 is a traffic light, not a lamp post; a smashed prop releases 9 bricks (six plus three super bricks) in the oracle, 6 in the port. [§2026-10-18T19:56:24](#ts-20261018-195624)
- The port destroys the traffic light in frame 19 and the tree in frame 50 as the oracle does, and a lamp post one update early (frame 44 against 45). [§2026-10-18T19:56:24](#ts-20261018-195624)
- Open: that lamp update (VEH-130), the type 67 hit sequences (VEH-131), indestructible props in the car's way (VEH-132), the prop's TriggerObject on a car smash (VEH-129).

<a id="ts-20261018-195624"></a>

## 2026-10-18T19:56:24+02:00 — Which props a vehicle contact destroys: mesh-kind entries, trafficlight / respawnable / simple classes, and the other classes' reaction (llvm-objdump and REA of artifacts/game/game.arm.elf; own Azahar GDB runs without RPC, port 24732, fixed dt 0.033, New 3DS, OpenGL)

[Corrects debris.md §2026-10-18T14:46:04](debris.md#ts-20261018-144604): the object destroyed at update 18 of `veh-smash-prop` is not a lamp post. [Extends vehicles-physics-props.md §2026-10-16T11:15:22](vehicles-physics-props.md#ts-20261016-111522) (Unknowns: classes other than the wobbly one, the grid shape of a mesh-kind prop, indestructible props).

Evidence: llvm-objdump of 0x237f30 (the gather, mode 1 at 0x238168 and 0x2381c8), 0x2c02c8, 0x230088, 0x22ec3c, 0x275c00..0x275eb8, 0x2a3390, 0x1bc38c, 0x1f8948, 0x2b3fcc, 0x1749b8, 0x15e408, the registrations (a `mov r0, #type` and `bl 0x246240`, then the pool `{ctor, ?, ?, ?, message handler, ...}`); the class files of artifacts/fib (ObjectType, WobbleHealth, StudValue) through a scratch dump of `parseLevel`; the level data of lvl_01 (objects per class). Oracle runs from `veh-smash-prop` (own emulator, `ctrace` of 0x23cd80, 0x2a3390, 0x1bc38c, 0x21e2e4, 0x275c00; `memtrace` of the van's node matrix at node+0x5c, node bounds node+0xa0, entity box E+0x60, rigid velocity 0x08e18ffc, the props' entity words) and, for the street lamp, with the circle pad at -156 for the first 20 updates (the pad steers slowly; the unsteered run hits the traffic light at frame 19 and the tree at frame 50 every time). Raw files stay in artifacts/oracle/r23B-out (not committed).

**The grid test is the same for both kinds.** The branch of 0x237f30 for the descriptor mode 1 (0x238168 for kind 1 entries, 0x2381c8 for mode 2) takes the owner node matrix (0x250810), multiplies the entry's optional matrix (+0x14, null for entities), and runs 0x22ec3c on the shape at entry +0x18, whatever the entry kind: the entity box (+0x60 / +0x6c) of a box entry, the root node of the collision item (centre +0, half +0xc of the 0x1c-byte node) of a mesh entry. The filter 0x2c02c8 afterwards always uses the target's entity box +0x60 / +0x6c, also for a mesh entry. The port's mesh-kind props therefore use the item's root node in the first test and the automatic entity box in the second. Of the 674 wobbly props of lvl_01 393 register a mesh, among them all 62 `Lampost_*`; that is why cars drove through lamp posts in `chase-game`.

**What the message does per class** (the vehicle's message 0: msg+4 the vehicle, msg+0xc damage 100, msg+0x14 = 11, the rest 0; 0x275e74). The 5th word of the registration pool is the message handler.

| ObjectType | classes in lvl_01 | handler | message 0 from a vehicle |
|---|---|---|---|
| 119 (0x77) | leprop_wobblydestructible, 674 | 0x2a3390 | health (record +0x10, `WobbleHealth`, class 3, instances 2..6) minus 100; at 0 or below 0x23cd80 (no `WobbleFallApartAnim` in lvl_01) |
| 204 (0xcc) | prop_trafficlight, 38 | 0x1bc38c | the same with the health at record +0x110 (`WobbleHealth`, default 2, class 3) |
| 214 (0xd6) | prop_respawnabledestructible, 15 | 0x2b3fcc | message 1 runs 0x239898 first, everything else goes to the base handler 0x1f8948 |
| 85 (0x55) | leprop_simpledestructible, 0 | 0x1f8948 | message 0 with a damage that is not 0 calls 0x23cd80 at once |
| 86 (0x56) | leprop_simpleindestructible, 69 | none (the pool holds no handler) | ignored |
| 42 (0x2a) | prop_door, 5 | 0x1749b8 | handles only messages 8, 41, 54, 252, 254 and 255: ignored |
| 67 (0x43) | prop_prop 16, leprop_prop 4 (57 classes in all) | 0x15e408 | the hit-sequence machine 0x2398d8 / 0x2b7894 (up to four slots at record +0x10 * n, clips and stud spawns): not read in full, not ported |

Destroy (0x23cd80 -> message 1 -> 0x1f8948): `Respawn` -> 0x239898, 0x2aa5c8 (explosion, debris), `ATTR_SFX_DESTROYED`, `TriggerObject` -> 0x244f40 (also for a hit by the player: the path is the same, the class handler only decides health). So a car destroying a prop fires its `TriggerObject` script in the original; the port does not (VEH-129).

**Do indestructible props stop the car?** By code no: a vehicle contact only sends the message; a class without a handler (86) or a handler without message 0 (doors) ignores it, and what stops a car is the static world and the Bullet boxes (`collision_vehicle`, mask 17), not the placed props of mask 6. Not measured in the oracle (no state with a leprop_simpleindestructible in the car's way: VEH-132).

**The object of `veh-smash-prop` is a traffic light.** The entity 0x08de6690 destroyed in frame 19 (health 3 -> -97, handler 0x1bc38c, lr 0x1bc470) is `Trafficlight31` (props/prop_trafficlight) at (862.71, 31.15, 920.91), the start of the burst of [debris.md §2026-10-18T14:46:04](debris.md#ts-20261018-144604) (x 862.7043, z 920.9139). The second object (frame 50, 0x2a3390) is `Generic_Tree_13` (878.32, 32.65, 911.48, wobbly, box entry). A wobbly lamp post (`Lampost_05`, 875.60, 31.16, 924.47, mesh entry, entity 0x08d89760) is destroyed in frame 45 of the steered run.

**Bricks released** (`ctrace 0x21e2e4`): Trafficlight31 mode 3, count 9, colour 0xffc0c0c0 (r4 of the call); Generic_Tree_13 mode 3, count 9, colour 0xff03742e; both are the six `DebrisAmount` bricks plus three super bricks of [debris.md §2026-10-18T14:40:56](debris.md#ts-20261018-144056). The vehicle's own contact (0x22a034 -> 0x229fb8) bursts 6. No call with the modes 0..2 (dropped studs) happened in 160 updates, and 0x23a6b0, 0x2149ac and 0x22ca88 were not entered: [studs.md §2026-10-18T19:57:26](studs.md#ts-20261018-195726). The port bursts `DebrisAmount` (6) bricks and no super bricks (VEH-S103).

**Timing pins (port against oracle).** The memtrace frame of a row is its row index + 3 and equals the frame of the ctrace stops. With the van's node matrix of each frame, its node bounds (centre (-0.0014, 0.0381, -0.0762), half (1.1846, 1.2574, 2.5146)) and entity box (centre (-0.0020, -0.0202, -0.0392), half (0.9336, 1.1986, 2.4915)): the port's smasher destroys Trafficlight31 in frame 19 and Generic_Tree_13 in frame 50, the frames of the oracle's 0x23cd80 (`vehicle-prop-smash-real-data`). For `Lampost_05` the port destroys it in frame 44, one update before the oracle's frame 45 (the gap on the lamp's x axis is +0.32 in frame 43 and -0.39 in frame 44, so it is not a rounding difference; the lamp's root-node and entity boxes have the same x extent, so the mesh shape does not explain it). The entity word +0x10 of the traffic light and the tree shows bit 0 two rows after those frames in the unsteered memtrace; the lamp's shows it in frame 45.

**Port.** `VehiclePropSmasher` (src/runtime/vehicle_prop_smash.*) now loads the classes 119, 204, 214 and 85 (727 props in lvl_01: 674 + 38 + 15), uses the root node of a mesh entry for the first test, and keeps the rest unchanged; the destroyed prop is disabled, hidden and bursts as before. Tests `vehicle-prop-smash` and `vehicle-prop-smash-real-data`.

Unknowns:
- Why the lamp is destroyed one update later than the port's first overlap (VEH-130): the shape, the vehicle's world entity box in a tilted pose (the lamp rows pitch 0.2) or an order inside the update were not isolated.
- The hit-sequence machine of type 67 for a vehicle message (VEH-131), whether any of the 57 classes of that type drops studs or bricks when a car touches it.
- The grid shape read from a running game for a mesh entry (the root node is taken from the table in [collision.md](collision.md) and the code above, not read in memory).
- Whether an indestructible prop is in the way of a car in a real level (VEH-132).
