# Build-its: bricks (props/prop_buildit)

The bricks of a build-it: scatter, flight, built pose, the rise of the final prop and the hop of unbuilt bricks. Split from [buildit.md](buildit.md). For developers; no game assets.

## Current state

- Bricks: 228-byte records, six flown bricks of the donut van; brick k flies in updates 3+12k..11+12k, flight is slerp plus linear position plus a hop of 0.2 x 4t(1-t) x (|dx|+|dz|) [§2026-10-16T09:35](#ts-20261016-093536).
- Built pose of a brick is end key x F x inverse(P) (final prop's world matrix times the inverse of the parts'), not the identity; this replaces the pose in [§2026-10-15T18:31](#ts-20261015-183145) [§2026-10-16T09:35](#ts-20261016-093536).
- Unbuilt bricks wobble (start values 0x18275c, condition `0x210f30`, sound for the first three crossings per update, not four) [§2026-10-15T19:52](#ts-20261015-195230).
- Rise of the final prop in state 10: y = base y + sin(spin), spin grows by 12 per update (24 from 90 degrees); state 11 writes the base y back [§2026-10-15T18:31](#ts-20261015-183145).

<a id="ts-20261015-183145"></a>

## 2026-10-15T18:31:45+02:00 — Brick animation of the build-it: scatter, flight and rise of the final prop (llvm-objdump, archive data)

Corrected by [buildit.md §2026-10-16T09:35:36](#ts-20261016-093536): the built pose of a brick is not the animation's last frame (identity) but that frame times the final prop's world matrix times the inverse of the parts' own; the brick data, flight, hop and the rise are confirmed by an oracle trace.

Corrected by [buildit.md §2026-10-15T19:52](#ts-20261015-195230): the wobble sound is made for the first three crossings (not four); start values and condition of the wobble are recovered.

Source: llvm-objdump of `artifacts/game/game.arm.elf`, the extracted RomFS; no oracle run. Completes the unknowns of [§2026-10-14T23:55](buildit.md#ts-20261014-235539).

**Data.** `models/prop_donut_truck_cabin_bi/prop_donut_truck_cabin_bi_buildit.bfnanm` (2 frames, 30 fps, six object tracks `Bit1`..`Bit6`) next to the parts model with six nodes `Bit1`..`Bit6`, one mesh each (the mesh vertices sit at their built positions). The path is built by 0x18ddc0 from the strings "models/%s/", "_buildit" and ".fnanm" and the object's mesh name; the instance's mesh is `Prop_Donut_Truck_Cabin_BI/..._atlas_lvl_01_cherrytreehills_main`, so the `%s` must be its folder (inferred from the shipped file name). Each object track holds two blocks; the first decodes with the curve decoder 0x248910 (`decodeChannels`, `localMatrix`) to a matrix per frame: frame 1 is the identity for all six (built), frame 0 is the scattered pose, for example Bit1 a quarter turn about z at (-0.5106, 1.045, -1.54) and Bit6 a turn about x at (-1.3425, 3.169, -5.134). The model's own node matrices (block +0x20) equal frame 0 within the animation's quantisation of 0.0025.

**Brick setup (0x18ddc0 -> 0x21e96c, then 0x2318c8).** The brick array has 228 bytes per brick. 0x21e96c copies the node's current matrix into record +0xa0 (end) and +0x60 (start), then, for a node whose name an animation object carries (0x25ad90), samples the track (0x24a0c8) into +0x60 (key 0) and +0xa0 (key 1). 0x2318c8 turns both matrices into a quaternion (0x248618, Shepperd's method with the table {1, 2, 0} at 0x358f04) and a position: start at +0x40 and +0x28, end at +0x50 and +0x34, and sets the rate +0x40 of the record to 0.1. Bricks and nodes pair by index (node k is brick k, or the reverse for flag bit 4).

**Flight, 0x19c9e0(entity, brick).** Called by the check 0x1d428c in state 8 while the current brick's progress t (+0x24) is below 1.0, before the progress grows by the rate: rotation = slerp(start, end, t) (0x21a600: shortest arc, linear above a dot product of 0.99999), position = start + (end - start) t (0x221e5c), then y += 0.2 x 4t(1 - t) x (|dx| + |dz|) of start and end (a hop); with a `MoveViaPoint` object (flag bit 5) the position is a quadratic curve through the point instead (not transcribed). The result goes to the node through 0x22b5a0 (matrix of the node with that index). The change to state 9 (0x1aae6c) sets the node to the end pose (key 1, here the identity) and the progress to 1.0. The change to state 11 puts all nodes back to the start pose and the progress to 0.

**Rise of the final prop.** State 10 check 0x1d4320: y of the final prop's node = base y (record +0x4c, stored by the change to state 10) + sin(spin degrees), with the spin before it grows by 12 (24 from 90 degrees); the change to state 11 writes the base y back.

**Wobble of the unbuilt bricks, 0x206ea0(entity, 0).** Called every update in state 2 (flag bits 2 and 3 clear) and in state 8. When a condition holds (r5: 0x210f30 on the global at 0x3772b8, or 0x1a0f6c(entity)) or the byte +0x39 is non-zero (it counts down by 1 per update), every brick that is not active (+0xe2 = 0) and not at progress 1.0 hops: y offset = sin(phase degrees) x amplitude on the node (matrix y = base y +0x2c + offset), phase (+8) += speed (+4) per update; when the phase passes 180 it falls by 180, amplitude (+0) becomes 0.2 + 0.3 r and speed 15 + 10 r (r random in [0, 1) from 0x2345dc), and for the first four bricks a sound-like effect call (0x23fa54, 0x22f770 with 1.1 - 0.2 r) is made. Otherwise (condition false, counter 0) a brick that wobbled (+0x36 bit 0) is put back to its base matrix. The constants 0.2, 0.3, 10, 15, 180 and 1.1 are the literals at 0x207270..0x207284.

**Port.** `src/runtime/prop_nodes.*` (quaternion, slerp, flight), `src/runtime/buildit_bricks.*` (bricks of a parts model and its animation), `BuilditState::flight` and `propRise`, `src/game_prop_parts.*` (node instances of the parts model, the final prop's rise), `Scene::addModelNodes`. Tests `prop-parts`, `prop-parts-real-data`, `buildit`. Picture check only by eye against the code (no oracle picture of the scattered bricks exists): the six bricks lie scattered around the build-it, the first flies in over about 12 updates, the van rises at the end.

Unknowns:
- The condition r5 of the wobble (what 0x210f30 on 0x3772b8 and 0x1a0f6c test) and what sets the counter +0x39; the port does not wobble the bricks (LEVOBJ-60).
- The brick order against the node order for other build-its (`Reverse`, flag bit 4) and the `MoveViaPoint` curve; the end key of an animation with more than two frames (the port takes the last frame).
- Whether the node matrix per update equals the port's: a `ctrace` of 0x22b5a0 over a build in the oracle was not run.

<a id="ts-20261015-195230"></a>

## 2026-10-15T19:52:30+02:00 — Hop of the unbuilt bricks: start values, condition and measured rows (llvm-objdump, oracle)

[Corrects §2026-10-15T18:31](#ts-20261015-183145) on the wobble: the sound is made for the first three crossings of an update, not four, and the start values and the condition are now known.

Source: llvm-objdump of `artifacts/game/game.arm.elf`; Azahar oracle, GDB only (no RPC), state `m11-4-van-parts` (player in the use zone, brick array 0x08e114e0 of that state): `memtrace 40 brick 0x08e114e0:0x558 0x08a75580:0xa0` idle; the same with `poke` of 12 zero bytes into each brick (+0..+0xb) and with `teleport 900 52.4 650` away from the zone and back. The user state was not modified.

**Brick record.** +0 amplitude, +4 speed (degrees per update), +8 phase (degrees), +0xc offset (the last sin(phase) x amplitude), +0x24 progress, +0x28 start position, +0xe2 active byte; 228 bytes per brick, zero-filled at allocation (0x25c030 with the clear flag, 0x24d234).

**Start values, 0x18275c(entity, brick)**, called for the bricks in order 0..n-1 from 0x18ddc0 (link of the build-it): r = 0x2345dc (low 16 bits of 0x25b2b8 times 2^-16, one draw each): phase = 180 r, then amplitude = 0.2 + 0.3 r, then speed = 15 + 10 r. It also zeroes progress, active byte and offset and puts the identity into +0x60 and +0xa0. Poking zeros into the three fields in the oracle leaves them zero for 40 updates: the update alone never starts a hop, only these start values do.

**Condition (r5 of 0x206ea0).** `0x210f30(*0x3772b8, entity)` is true when the player can use the build-it: it walks the entity's use-zone list (entity +0x20), tests the player against the zone shape of the entry (0x221570: shape in the low 3 bits of entry+4, 3 is the box) and then asks the class (0x244fc8, message 3 or 4 by whether the asker is the player) whether the use is possible. `0x1a0f6c(entity)` is true when the entity is in the list at `*0x519114 + 0xa8` (8-byte entries, count in the flag bits 3..6 of +0xc0); what fills that list was not read. The counter +0x39 is never written by the build-it code (stores to +0x38/+0x39 exist only for other classes) and reads 0 in the oracle (`06 ff 01 00 00 00 00 01` at +0x34..+0x3b).

**Measured (oracle).** In the zone every update: offset = sin(phase) x amplitude with the phase before the step, then phase += speed; for example brick 0 at the first row amplitude 0.361096203, speed 20.749054, phase 67.8738403 gives phases 88.6228943, 109.371948, 130.121002, 150.870056 and offsets 0.334503919, 0.360991895, 0.340652794, 0.276124865 at the next four rows. Brick 3 crosses 180 from phase 162.102509 (offset 0.291660875 with the old amplitude 0.43334505, speed 24.4050598) to phase 6.50756836, amplitude 0.482522607 (r = 61718 / 65536 gives exactly this float) and speed 15.171814 (r = 1126 / 65536). Away from the zone (teleport 40 m): all fields stay frozen, the offset reads 0.0; back in the zone the hop continues from the same phase.

**Sound of a crossing.** The crossing counter is incremented before the test `< 4`, so the first three crossings of an update call 0x23fa54 (sound 5, position = the brick's node translation, brick index) and 0x22f770 (sound 5, entity halfword +0x14, value 1.1 - 0.2 r with a draw before the amplitude and speed draws). Which sound id 5 is (a global sound table, not an `ATTR_` parameter) and its volume were not read.

**Port.** `src/runtime/buildit_wobble.*` (`BuilditWobble`), `BuilditSet::updateWobble` (runs in the update when state equals target and is 2 or 8; condition = the player in the use zone), `PropParts::apply` adds the offset to the brick's node y. Test `buildit-wobble` pins the rows above.

Unknowns:
- The seed of the original random source at link time: the start values of a real run differ from the port's (stand-in source with a fixed seed).
- What fills the list of 0x1a0f6c and whether the class answer of 0x244fc8 adds conditions beyond the zone (the port tests the zone only).
- The flag bit 2 of +0x36 (set by message 0x14) and bit 3 (sets target 7) in the state-2 check are not modelled.
- The sound 5 and its volume.

<a id="ts-20261016-093536"></a>

## 2026-10-16T09:35:36+02:00 — Oracle trace of the brick node matrices: the bricks fly to the final prop (Azahar GDB + RPC, llvm-objdump)

[Corrects §2026-10-15T18:31](#ts-20261015-183145) on the built pose: it is not the identity of the animation's last frame.

Source: Azahar oracle, New 3DS, OpenGL, state `m11-4-van-parts` (user save, read-only), `hold A` and `trap 0x22b5a0 110` (registers, 32 stack words and 16 words at r2 per stop; the call at 0x19cb58 passes r2 = sp+8, the node matrix, row-vector 3x4 with the translation at +0x30); llvm-objdump of 0x21e96c, 0x2318c8, 0x258bc4; free-run RPC screenshots of the same build (about 25 per second). The trace has 222 stops: 156 from the hop (0x20706c), 54 from the flight (0x19cb5c, 9 per brick), 6 from the change to state 9 (0x1aaf54) and 6 from the change to state 11 (0x1ab13c).

**Order and timing (update after the first press).** Brick k flies in updates 3+12k .. 11+12k (9 stops), its end pose is set in update 12+12k (0x1aaf54); updates 3..72 for six bricks. At update 85 (state 11) the six bricks are put back to their start pose (0x1ab13c) in one update. Bricks and nodes pair by index: brick 0 starts at Bit1's frame-0 pose (-0.5106, 1.045, -1.54), brick 5 at Bit6's (-1.3425, 3.1688, -5.1344), the others as the animation's frame 0.

**Built pose.** Every brick ends at the same pose, position (-5.1974, -0.0594, -5.1371), first rotation row (0.564, 0.000, -0.826), not at the identity. 0x2318c8 builds a matrix M = F * inverse(P) (0x258bc4: A times the rigid inverse of B), with F the world matrix of the object at record +0x1c (0x2429d0, here `1-1_Donut_Van_Body`) and P the build-it's own (0x250810), multiplies the end key (+0xa0) of every brick by M and turns the result into quaternion and position; the start keys are unchanged. For the donut van, F * inverse(P) from the level placements (parts 874.6367 52.0105 696.9986, final 869.4393 51.9511 691.8615, yaw 55.6467) is exactly the traced pose, so the flown bricks end where the final prop stands. The port took the identity and flew the bricks to the parts' own place; the final prop then appeared elsewhere when the parts were hidden, which was the snap the user saw.

**Flight.** The port's formula (slerp, linear position, hop 0.2 x 4t(1 - t) x (|dx| + |dz|)) reproduces the traced rows: brick 0 at progress 0.1 (update 4) at (-0.9793, 1.5310, -1.8997) with first row (0.140, -0.987, -0.080), brick 5 at (-1.7280, 3.1237, -5.1346) with (0.993, 0.096, -0.065).

**Pictures.** Oracle free-run shots (build from the state, 7 s of A, then 3 s): the scattered bricks fly toward the van body standing behind the parts, the van rises and the broken van is gone at the end; the port after the fix shows bricks flying to the same van and no snap. The port run used the mission route of CTest `game-mission-1-1` (the camera and dialog boxes differ), so the pictures were compared by eye only; the numbers above are the evidence.

**Port.** `BuilditBricks::buildAt`, `nodeRelativeTo` (0x258bc4), wired in `PropParts::loadBuildits`; test `prop-parts-real-data` pins the built pose and two flight rows from the trace.

Unknowns:
- Which field of the class record +0x1c is (the port uses the final prop's placement; the trace agrees for this build-it); a build-it without a final prop was not read.
- What the player does visually during the build (building animation, LEVOBJ-41); the oracle shots show it, the port does not.
- The order and update of the switch of `1-1_SO_VanFixed` from the broken van to the driveable van against the build's last update was not measured per update.

