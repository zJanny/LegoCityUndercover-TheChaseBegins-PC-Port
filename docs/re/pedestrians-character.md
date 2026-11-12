# Pedestrians: the pedestrian character (kind 197)

The `chars/pedestrian` character that the pedestrian manager spawns ([pedestrians.md](pedestrians.md)): its create 0x1913f8, the state step 0x1d7e88, the walk 0x2b9138 with the avoidance 0x2edf08, the steady updates of states 3, 4, 5 and 7 and the walk inputs. Items: [pedestrians-traffic-pedestrians.md](../plans/features/pedestrians-traffic-pedestrians.md).

## Current state

- The pedestrian (kind 197): its update runs the walk 0x2b9138 in state 2 and then the state step 0x1d7e88 (entries of states 1 and 3 .. 7, the hold counter +0x10). The walk turns toward the target at dt x 5, moves along row 2 at speed x dt, advances the path parameter by 0.3 near the target, turns back by 0.75, and sidesteps by 1.3 for the avoidance 0x2edf08 (player or pedestrian within 2.0 and 1.0 to the side). The walk clip starts in mode 1. [§2026-10-20T12:20:49](#ts-20261020-122049)
- The steady updates (0x277c24): states 3, 4 and 7 move the node by the clip's root translation through its rows (0x277afc), then the wall test 0x2059ec and the ground probe 0x2058cc (0.25 up and down). State 5 runs the contact 0x1fd8c8 with wall test and probe, and moves the node by the push once more on a touch. The draw test 0x2930e0 of the hold uses the fade-in distance of the node's tier from the fade point 0x52a1b4; state 6 refreshes its timer while the pedestrian is inside the player vehicle's box in x and z (0x278b04). Type 1 paths are linear (0x1b33fc), and no path gets a node at run time. [§2026-10-20T13:09:29](#ts-20261020-130929)
- Port: `src/runtime/pedestrian_walk.*` (test `pedestrian-walk`). [§2026-10-20T12:20:49](#ts-20261020-122049), [§2026-10-20T13:09:29](#ts-20261020-130929)
- The message slot 0x19efbc is the shared character animation sound handler: it requests no state and never gets 0x80000003 (type byte +0x20 is 0); the dodge and splat requests are the vehicle test's. The entity pass runs the pedestrians after every kind below 197 at priority 128. Port: `src/runtime/pedestrian_messages.*`. [§2026-11-12T08:05:25](#ts-20261112-080525)
- Open: the writer of the fade point 0x52a1b4, and the sound tracks of the pedestrian's clips. [§2026-10-20T13:09:29](#ts-20261020-130929), [§2026-11-12T08:05:25](#ts-20261112-080525)

<a id="ts-20261020-122049"></a>

## 2026-10-20T12:20:49+02:00 — The pedestrian character (kind 197): create, state step, walk and avoidance (llvm-objdump only, no REA session, no emulator)
Corrected by [§2026-10-20T13:09:29](#ts-20261020-130929): the result of 0x24ef70 in the state step is the distance the draw test 0x2930e0 reads, not unused.
Corrected by [§2026-11-12T08:05:25](#ts-20261112-080525): the message slot 0x19efbc is read in full; it requests no dodge or splat state.

[Corrects pedestrians.md §2026-10-20T11:17:42](pedestrians.md#ts-20261020-111742) in two points. 0x1918e0 is not the pedestrian's message handler: it reads component +0x176 / +0x178, and the pedestrian's component has 100 bytes. The registration of kind 197 at 0x1786dc puts 0x19efbc in slot 0x14, the message slot. And the walk clip is started in mode 1 (loop). Mode 6 is what the stop 0x172754 of the manager's removal sets, which fits the mode 6 of the 14 lvl_01 pedestrians in [npc-animation.md](npc-animation.md) (not checked in that state).

Method: `llvm-objdump -d --triple=armv6k-none-eabi --mattr=+vfp2` on `artifacts/game/game.arm.elf`. Port: `src/runtime/pedestrian_walk.*` (`PedestrianWalk`), test `pedestrian-walk`.

**Create 0x1913f8.** It copies the 0x88-byte entity and allocates a 100-byte component (+0x7c). +0xc = the model 0x2442b4. Entity +0x5c = the length of entity +0x6c (0x2536a8; 0x242df4 fills +0x60 / +0x6c, not read). The clips (0x223a68) come from `models/char_minifig/<AnimationClass>/`; the `dodge_*01` clips come from `CLASS_Generic/` when the class has no `.bfnanm` of that name (0x21d758):
- +0x14 `walk_minifig`
- +0x18 `dodge_right01_minifig`, +0x1c `dodge_left01_minifig`, +0x20 `dodge_right02_minifig`, +0x24 `dodge_left02_minifig`
- +0x28 `bump01_minifig`, +0x2c `bump02_minifig`
- +0x30 `splat_minifig`, +0x34 `splat_up_minifig`

The four dodge clips and `splat_up` get bits 0xf of their word +8, the root motion bits of [objects.md](objects.md). Byte +0x61 = 4 when the parameter `IsMale` (default 1) is set, else 8. The state hwords +2 and +4 are 0. Post-link 0x191810 scales the model by 2.0 when 0x2369a8(6) is set, else by 1.0.

**Update, slot 0xc 0x19188c.** 0x2424d0 calls 0x277c24 only while state (+2) == requested (+4). Then the state step 0x1d7e88 runs. 0x277c24 switches on the state:
- 2: the walk 0x2b9138.
- 3, 4, 7: 0x277afc, the root motion of the current clip, then the wall test 0x2059ec and the ground probe 0x2058cc.
- 5: 0x1fd8c8 with the vector at 0x56d400, then the node position + that vector on a touch.
- 6: nothing.

**State step 0x1d7e88.** When state != requested, the entry of the requested state runs and the state becomes the requested one. Clip starts are 0x23afc0(entity, clip, mode, first 0, end 0xffff, speed 1.0, blend 0.0). "Root motion" below means 0x239568 on the new slot.
- 1: the target +0x38 = the path sample at +0x4c + 0.1 (forward +0x5c) or - 0.1, plus the lateral offset. The parameter itself stays. +0x10 = random % 50 + 10. Byte +0x62 = 1. Walk clip in mode 1.
- 3 / 4: +0x5e = 1 / 0, or 3 / 2 when byte +0x5f is 1. That dodge clip (+0x18 + 4 x +0x5e) in mode 0 with root motion.
- 5: +0x60 = random % 2. That bump clip in mode 0. The node rotation becomes the heading to the player: uxth(int(atan2(dx, dz) x 10430.378)), then 0x23deb4 (identity rows turned about y by heading x 9.58738e-5).
- 6: the splat clip in mode 1, +0x58 = 3.0.
- 7: `splat_up` in mode 0 with root motion.

Then, by state:
- 1 requests 2.
- 3, 4, 5 and 7 request 1 once their clip's kind 0x245f84 is 6 (finished).
- 6: when 0x278b04(player vehicle, entity) is true, +0x58 = 3.0. While +0x58 > 0 it drops by dt (0x23602c), and at 0 or below state 7 is requested.

Last comes the hold counter +0x10, which the manager's removal reads. When 0x2930e0(node) is true, 0x23de70 is false and state or requested is not 0, +0x10 = max(+0x10, 10). Otherwise it counts down to 0. Byte +0x5d counts down to 0.

**Walk 0x2b9138 (state 2).**
1. Turn: M = 0x22859c(from the node position to the target): row 2 = unit(target - position); row 0 = (z, 0, -x) of row 2, or (0, -z, y) when |row 2 y| > 0.98, normalised; row 1 = row 2 x row 0. The node's row 2 y and row 0 y become M's. Then rows 0 and 2 move toward M's by t = dt x 5.0 (0x221e5c, a + (b - a) t).
2. velocity = row 2 x (speed +0x54 x dt).
3. Ground probe flag. Byte 0x375d5c (cleared by the manager update) is clear: the flag is set, and so is the byte. Otherwise, with +0x5d != 0: the first angle of 0x247660 (atan2(-r2y, r2z), or its pair with +pi when that has the smaller length) is reduced to |a - pi| when |a| > pi / 2; the flag is set above 0.34906587 (20 degrees).
4. 0x1fd8c8(entity, position, velocity, 0, flag). When the player has a vehicle and the flag is clear, it returns 0 and does nothing else. Otherwise the pedestrian touches the player when the xz distance squared is below 1.5 (0x20af40). With the player's state hword +0x6c 2 or 3, the velocity's x and z then get + unit(position - player) x 0.1. With the flag set, the ground probe 0x2058cc runs: a ray from the position up and down by the floats at 0x374494 / 0x374498 (0 in the image, written at run time) sets the node y and the room.
5. A touch with player state 2 or 3 requests state 5. The ray 0x21ab20 from the position to (x, -1.0, z) then sets the node y on a hit.
6. Blocked: the first non-zero 0x1ffcd4(entity, position, velocity, dt) over the list at 0x5863b4 + 0x305c (count +0x3058). Objects whose +0x84 has component bit 1 of +0xcc are skipped.
7. position += velocity. The rows are rebuilt with up (0, 1, 0) (0x56d424): row 0 = unit(up x row 2), row 2 = unit(row 0 x up), row 1 = up.
8. Result r (blocked result first). When r is 0 and +0x5d is 0, r = the avoidance 0x2edf08.
   - r = 4: the direction flips. The parameter moves by +0.75 (new direction forward) or -0.75, and the target is sampled there.
   - r = 0 or 3 (and r = 0 while +0x5d != 0): only when the 3D distance from the position to the target is below entity +0x5c. Then r = 0 moves the parameter by +0.3 / -0.3 and samples the target there; r = 3 sidesteps with side -1.
   - r = 1 or 2: sidestep with side +1 for 1, else -1.
   - A sidestep sets target = position + row 2 x 1.3 + row 0 x (1.3 x side), and +0x5d = 30.
   - Wrap of the parameter: below 0 it becomes count + p; at count (hword path +0xa) or above it becomes p - count, for open paths as well.
9. With requested 3 or 4 (set during the update by a source not read), the node rows become 0x22859c(position, player position).

The path sample with offset: the type callback slot 0 of table 0x357e24 (20 bytes per type: type 0 0x1c17c0, type 1 0x1b33fc) is called as (path +8, target, tangent, 1, p). The target then gets x + (-tangent z) x lateral +0x48 and z + tangent x x lateral. 0x1c17c0 is the spline of `sampleDirectorPath` with the unit tangent of `detectivePathTangent`. With its fourth argument set and path +0x18 non-zero, it transforms both by the path's node (0x254250).

**Avoidance 0x2edf08(manager, entity).** The player first, then every live slot of the manager except the pedestrian itself, in slot order. d = other - position. When the xz length (0x20e968) is below 2.0 and the side s = d . row 0 in xz (0x221920) has |s| < 1.0, the result is 2 for s > 0, else 1. Otherwise the next one is tested, and 0 when none matches. There is no test of in front or behind.

**Message slot 0x19efbc**, read only at its start:
- 0x80000003: +0x10 = 0; in state 1 or with requested 1, state = requested = 0.
- 50: a distance test against the record of the table at 0x19f3f8 indexed by +0x61, then 0x23ae84.
- 0x80000001 and 252: not read.

Port: `PedestrianWalk` (`update`, `walk`, `avoidance`) on a `PedestrianBody` (the component fields, the node rows and position). The caller supplies dt, the path (`PedestrianPath`; `pedestrianLevelPath` for type 0), entity +0x5c, the player's position, vehicle and state, the other live pedestrians, byte 0x375d5c, 0x2930e0 / 0x23de70, 0x278b04, the blocked result 0x1ffcd4, the probe 0x2058cc, the ray 0x21ab20 and the clip finished test. Animation starts are returned as requests. The steady updates of states 3, 4, 5 and 7 are not run. Not wired into `chase-game`.

Unknowns:
- 0x1ffcd4 (the blocked results 1 .. 4, with a random % 2 at 0x1fff24), the floats 0x374494 / 0x374498 of the probe, the vector 0x56d400, 0x278b04 and 0x2930e0 (a node draw test with the distance fade tiers).
- 0x242df4 (the bounds behind entity +0x5c), whether any pedestrian path has a node at path +0x18, and type 1 0x1b33fc.
- The message slot 0x19efbc past its start: who requests states 3, 4 and 6 (vehicle hits, dodges). The root motion step 0x277afc of states 3, 4 and 7.
- 0x24ef70(position, 0x52a1b4) in the state step, whose result is not used.

<a id="ts-20261020-130929"></a>

## 2026-10-20T13:09:29+02:00 — The pedestrian's steady updates of states 3, 4, 5 and 7 and the walk inputs (llvm-objdump only, no REA session, no emulator)

[Corrects §2026-10-20T12:20:49](#ts-20261020-122049): the result of 0x24ef70(position, 0x52a1b4) in the state step is used. It is the squared distance that the draw test 0x2930e0 gets in s0.

Method: `llvm-objdump -d --triple=armv6k-none-eabi --mattr=+vfp2` on `artifacts/game/game.arm.elf`, data words of the ELF image, scans of `bl` targets and literal pools. Port: `src/runtime/pedestrian_walk.*`, test `pedestrian-walk`.

**Steady callback 0x277c24.** Jump table on state - 2 (0x277c4c): 2 the walk 0x2b9138; 3, 4, 7 0x277afc(entity, a zero vector); 5 0x277c94; 6 and other states nothing.

**Root motion 0x277afc (states 3, 4, 7).**
1. slot = 0x233900(entity +0x40), the current slot. 0x2327b4(entity +0x44, out) gives the root delta (translation, yaw at +0xc); a 0 return ends the update: no wall test, no probe.
2. N = a copy of the node's world matrix (0x2429d0).
3. With bit 0 of the slot clip's word +8, or the slot's hword +4 bit 0x1000 with byte +0x28 set: R = identity turned by the yaw (0x24d434, 0x237bc4) with the delta translation (0x2554c0); R = R x N (0x258a10). R's rotation goes only into the copy (0x232788), which is never stored. move = -N.t + R.t, i.e. ((t.x N0 + t.y N1) + t.z N2) + N3 - N3 per column.
4. Otherwise the translation is turned by the heading atan2(N2.x, N2.z) (0x24765c, 0x17e3c0) and its x negated.
5. vector += move, then the wall test 0x2059ec(entity, vector) and the probe 0x2058cc(entity, 0).

The create sets bits 0xf on the four dodge clips and splat_up ([§2026-10-20T12:20:49](#ts-20261020-122049)), the clips the entries of 3, 4 and 7 start. So step 3 always runs and the node is not turned (inference: the current slot is the clip the entry started with blend 0.0).

**Contact 0x1fd8c8(entity, position, velocity, wall, probe).** 0 at once when the player has a vehicle (0x24255c) and `probe` is 0. Touch: xz distance squared to the player (0x20af40) below 1.5. On a touch, player state 2 or 3 adds unit(position - player) x 0.1 to the velocity's x and z, then `wall` runs 0x2059ec(entity, velocity). `probe` runs 0x2058cc whether touching or not (its second argument is not read). Returns the touch. The walk passes `wall` 0 (0x2b924c).

**State 5, 0x277c94.** v = the vector 0x56d400 (BSS, (0, 0, 0) in the oracle, [debris.md](debris.md)). On a touch from 0x1fd8c8(entity, node position, v, 1, 1) the node position += v once more (0x23d39c) after the wall test moved it, so a push moves the node twice in x and z, and by the slide's y once, after the probe. The node is stored (0x24c118).

**Wall test 0x2059ec(entity, move).** 0x23d4c0 gives the entities within entity +0x5c x 1.5 (0x205b54); one is kept when its hword +0x10 has bit 9 or its +0x78 is set, and (both +0x5c) x 1.5 exceeds the distance 0x2507dc to its box centre (+0x60 through its node, 0x250da8). 0x21a73c(entity, (move x, 0, move z) (0x249030), kept list, count, 0, 0x80000, 0, 0) slides it. Node x and z += slid x and z; move = (slid x, move y + slid y, slid z).

**Ground probe 0x2058cc.** Ray 0x21ab20 from (x, y + *0x374494, z) to (x, y - *0x374498, z), filter record 0x362298, mask 16. A hit sets node y to the hit's y; with a surface, 0x221064 / 0x23e110 set the room. The static initialiser 0x347904 stores 0.25, 0.25 and 5.0 at 0x374494, 0x374498 and 0x37449c. The only other literal reader is the blocked test 0x1ffcd4; no other writer was found.

**Entity box 0x242df4(entity, +0x60, +0x6c), create 0x191464.** With node word bit 9 clear the node bounds are built first (0x242eb0, `assets::modelBounds`). min = node +0xa0 - +0xac, max = +0xa0 + +0xac; +0x60 = (min + max) x 0.5, +0x6c = (max - min) x 0.5. The create stores the length of +0x6c (0x2536a8) in +0x5c.

**Draw test 0x2930e0(node, s0), state step 0x1d8210.** s0 = 0x24ef70(node position, 0x52a1b4), the squared distance: the call the earlier entry called unused feeds it. Result 1 unless the node type (word bits 0 .. 4) is *0x37674c with word bit 9 and +0xde bit 4 set; then the tier of 0x28781c ([pedestrians.md §2026-10-20T12:27:05](pedestrians.md#ts-20261020-122705)), a larger radius gives 1. Result 0 when (fade-in word 0x529e30 +0x3ac + 4 tier) squared <= s0. 0x52a1b4 = 0x529e30 +0x384 is the distance fade's point: 0x1077dc copies it to +0x390 and the model fade 0x10ec20 reads it. Its writer was not found.

**Vehicle test 0x278b04(vehicle, entity), state 6.** 0 without a vehicle. d = 0x2536c0(entity position, vehicle node matrix) = (p - t) dotted with rows 0 .. 2. 1 when vehicle +0x6c > |d.x| and |d.z| < vehicle +0x74 (entity box half extents).

**Type 1 path 0x1b33fc(record +8, position, tangent, transform, p).** Open (byte +1 clear): p clamped to 0 .. count - 1. i = int(floor p) (0x252c14), j = i + 1, f = p - i; closed: i and j wrap by the signed remainder 0x24e500. d = (point j - point i) x ((i + f) - i); tangent = unit(d) (0x23b998, (0, 0, 1) at a whole parameter); position = d + point i. With the transform argument and the node +0x10 set, 0x254250 / 0x249ee8 apply the node. Open at p = count - 1 reads the point after the block, times 0.

**Path node +0x18.** The setter 0x1c1b48 is called only from 0x2a8388 (via 0x2ca9cc) and 0x2bac30, the handlers of `EffectAttachParticlesToPath` and `EffectLinkPathToObject` (native table 0x359250). No level or class script calls either ([script-natives.md](script-natives.md), class D). So no path gets a node at run time; this answers PREGEN-5.

Port: `PedestrianWalk::rootMotion` (step 3 only), `bumped`, `contact` (shared with the walk), `wallTest`; `pedestrianGroundProbe`, `pedestrianEntityBox`, `pedestrianNodeDrawn`, `pedestrianDistanceSquared`, `pedestrianUnderVehicle`, `pedestrianLinearPathSample`; `pedestrianLevelPath` picks the sampler by type. The caller gives `rootTranslation` (0x2327b4), `wallSlide` (0x21a73c with its near list), `drawTest` and `playerVehicleBox`; the last two replace `visible`, `hidden` and `splatRefresh`.

Unknowns:
- The writer and run-time value of the fade point 0x52a1b4.
- The level file word at path record +0x18 was not read; it is taken as 0.
- Step 4 of 0x277afc is not ported: no clip of states 3, 4 and 7 lacks bit 0.

<a id="ts-20261112-080525"></a>

## 2026-11-12T08:05:25+01:00 — The pedestrian's message slot 0x19efbc is the shared character handler, and the entity pass order of the pedestrians (llvm-objdump only, no REA session, no emulator)

[Corrects §2026-10-20T12:20:49](#ts-20261020-122049): the slot 0x19efbc was read only at its start there; it is read in full now. Method: `llvm-objdump -d --triple=armv6k-none-eabi --mattr=+vfp2` on `artifacts/game/game.arm.elf`, worker 20261112-traffic-F, 2026-11-12. Functions read: 0x19efbc, the kind 197 descriptor builder 0x17868c .. 0x1786dc, the registration copy 0x246240, the update list removal 0x22d480, and a scan of the whole image for the literal 0x80000003. Port: `src/runtime/pedestrian_messages.*` (test `pedestrian-messages`), `UpdateSequence::atObjectKind` in `src/runtime/update_order.*` (test `update-order`).

**The handler requests no state.** 0x19efbc is the animation sound handler of the characters (the same address serves the player: [audio.md §2026-10-13T17:54:24](audio.md#ts-20261013-175424), [player-traversal-ladders.md §2026-10-21T22:44:32](player-traversal-ladders.md#ts-20261021-224432)); the pedestrian's component only supplies the voice row, byte +0x61 (4 or 8), of the character sound table behind 0x375f7c (68 bytes per row, 34 u16 slots). Nothing in it writes +0x5f or requests the states 3, 4 or 6. The dodge (3, 4, with +0x5f) and splat (6) requests are made by the vehicle test 0x1ffcd4 ([pedestrians-inputs.md §2026-10-20T13:20:02](pedestrians-inputs.md#ts-20261020-132002); `pedestrian_blocked`). The messages:
- 0x80000003: +0x10 = 0; when state (+2) or requested (+4) is 1, both become 0. Returns 1.
- 50: id = slot 5 of the voice row. The squared distance (0x24ef70) between the entity node translation (0x250810(+0x3c) +0x30) and the listener matrix translation (0x227f40() = 0x52ac10, +0x30) must be below the square of byte +0xb of the 16-byte row `id` of the table at *0x375f40 (the hearing distance, `SoundEffect::paramB`); then 0x23ae84(id, entity) plays it with the owner u16 +0x14. Returns 1.
- 252: 34 calls of the function in the message data, argument {function, argument}, with the ids of the slots 0 .. 33 of the voice row in order (a sound preload). Returns 0.
- 0x80000001 (the animation event, data kind at +0): kind 0xcacfb995 (footsteps): for every room record (284 bytes each, count hword at data + 0x474) and each entry whose track hash is 0x68cc6917 with value x weight minus previous value x previous weight above 0.5, the entity sends itself message 50. Kind 0x3a01a706 (character sound): entries with the track 0x13e3511c play (rise above 0.5: the draw 0x2345dc against the chance, then 0x23ae84, then the volume 0x219178 or 0x2191c0) or stop (fall below -0.5, 0x219244 then 0x23ae50 with fade -1.0) the slot's sound; entries of the hash 0x9223bd02 do the same with the category byte +0x21 of the character table row `*0x372e10 + 40 x voice` instead of the voice row and with the chance and volume taken from the entry. Returns 1 for both kinds, else 0. The port has these branches as `ClipSounds` / `FootstepDetector` for the player; the pedestrians do not use them yet (PED-30).
- Every other message returns 0.

**0x80000003 never reaches a pedestrian.** The descriptor of kind 197 is built on the stack at 0x17868c with 0 at +0x20; 0x246240 copies that word into the type record, and 0x22d480 sends 0x80000003 only for a type whose byte +0x20 is not 0. 0x22d50c is the only code in the image that loads the literal 0x80000003 as a message. So the reset in the handler is not run on a removal from the update list; a respawn sets state 0 and requested 1 itself (`PedestrianManager`).

**Order in the entity pass.** The pass sorts its list by the priority byte (128 for every object, written by the loader at 0x1a4214) and then by the class type, both ascending ([traffic-junction.md §2026-11-07T12:09:40](traffic-junction.md#ts-20261107-120940), [pedestrians-inputs.md §2026-10-20T14:10:27](pedestrians-inputs.md#ts-20261020-141027)). The registration `mov r0, #197` at 0x1786d8 makes the pedestrians run after every kind below 197 (the traffic cars 183, the junctions 192, doors, switches, pushables) and before the characters (priority 254). The port's `UpdateSequence` now sorts the object phase by a kind number (stable); the pedestrians have 197 and the other object steps keep their call order before them (PED-S17).

Unknowns:
- Whether the walk, dodge, bump and splat clips of the pedestrian classes carry the sound tracks that feed the 0x80000001 branches, and who sends 252 to them (PED-30).
- The kinds of the other object steps in the port (doors, switches, spawners, pushables, traffic cars, junctions) relative to each other: the original orders them by kind, the port by call order (PED-S17).
