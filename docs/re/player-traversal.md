# Player traversal: zipline, pole, climb wall, climb bar, ledge

The climbing and sliding moves of the player that mission 1-2 needs on the way to the dog cage: the zipline (`props/parkour_zipline`, object type 101), the pole and drain pipe (`props/prop_poleclimb`, `props/parkour_drainpipe`, type 187), the climbing bricks (`props/parkour_brickgrab_new`, type 20) and free climb wall (`props/parkour_wallclimb`, type 30), the rail and climb bar (`props/parkour_railgrab`, `props/prop_climbbar`, `props/parkour_flagpole`, type 29; corner `props/parkour_cornerrail`, type 218) and the ledge states. Port: `src/runtime/zipline.{hpp,cpp}`, glue `src/game_zipline.cpp`; `src/runtime/climb_bar.{hpp,cpp}`, glue `src/game_climb_bar.cpp`. Gaps: [player-traversal.md](../plans/features/player-traversal.md).

Subtopics: [player-traversal-pole.md](player-traversal-pole.md), [player-traversal-bricks.md](player-traversal-bricks.md), [player-traversal-rails.md](player-traversal-rails.md)

## Current state

- Zipline: type 101 with `GrappleLineType` 1 (0 is the tightrope); create 0x1a999c (`ZipLineSpeed` +0x154, default 7), link 0x19b074 (`TargetTightropeAnchor` +0xf4, node forward turned horizontal towards the target, `WalkBound` spread along the line), post-link 0x1a9bb4 (line list 0x592490), handler 0x1b6664 (message 3). [§2026-10-18T14:52:30](#ts-20261018-145230)
- Entry: the jump update 0x1d5254 runs the grab tests once the jump counter ctrl+0x2cf exceeds 4 (the 7th jump update); 0x2bbbb8 -> 0x2a9d94 tests the line list against the player point; an A press inside the high anchor's `UseBound` (facing it) also enters (not B: [§2026-10-18T16:37:26](#ts-20261018-163726)). Measured in m12-9: grab in the 7th jump update. [§2026-10-18T14:52:30](#ts-20261018-145230)
- States 42 "Zipline Get On" (clip `zipline_grab`, loop sound 107), 43 "Zipline Slide" (`zipline_slide`), 44 "Zipline Jump" (`zipline_dismount` root motion), then state 5; all behaviour in the state objects' virtual functions (table entries 42..44 hold only the object). Slide speed = `ZipLineSpeed` scaled by (30 dt)^2: 6.79 u/s on 1-8_Zip_Line05 at dt 1/30 (rate 0.27367 per second over 25.578). [§2026-10-18T14:52:30](#ts-20261018-145230)
- Other moves surveyed only (classes, descriptors, state numbers): pole states 0x11d..0x121, climb wall states 99..110, climb bar states 89..92, ledge dismount states 23 / 24. [§2026-10-18T14:52:30](#ts-20261018-145230)
- Pole and drain pipe (type 187), ported: [player-traversal-pole.md](player-traversal-pole.md).
- Climbing bricks (type 20), ported, and the climb wall entry: [player-traversal-bricks.md](player-traversal-bricks.md).
- Rails, climb bars and the corner rail (types 29, 218), ported: [player-traversal-rails.md](player-traversal-rails.md).
- The A presses at the zipline anchor and at the pole are not measurable from `m12-9`, `m12-10a` or `m12-10b`; the states they need are listed in [§2026-10-18T20:38:04](#ts-20261018-203804).
- The rope is ported: a straight textured ribbon (`RopeTexture`, `RenderWidth` 0.03) between the anchor nodes, drawn from both anchors (two strips per line), faded out by the camera's distance from the line only with `FadeNearCamera`; the ribbon's facing, render states and the tightrope ropes are open. [§2026-10-19T08:51:31](#ts-20261019-085131)
- The rope's facing and render state: the row at 0x529fec is the camera's view direction (measured, m12-9), the render state is the record of 0x252254 with the mode bytes of 0x297b7c, which the port's reading of the record turns into the states the port already used; the texture sampler is open; the original drew two strips per line. [§2026-10-19T10:00:18](#ts-20261019-100018)
- The rope is fogged with the scene's fog (the state record clears the no-fog bit); the shared particle program has a fog switch that only the rope turns on. [§2026-10-19T10:38:41+02:00](#ts-20261019-103841)
- Every textured zipline line is drawn twice, once from each anchor, because the anchor has no update handler and the gate 0x226644 then treats the target as not live. [§2026-10-19T11:04:28+02:00](#ts-20261019-110428)
- Open: the grab height of attachment set 18 for other characters, the fall-state grab path 0x2010d4, ground contact during the zipline dismount (0x215804), the rope's facing and render states, the tightrope; the other moves in their subtopic files.

<a id="ts-20261018-145230"></a>

## 2026-10-18T14:52:30+02:00 — Climbing classes surveyed; the zipline read and measured (REA, objdump, Azahar GDB traces of m12-9)

Corrected by [player-traversal.md §2026-10-18T16:08:28](player-traversal-pole.md#ts-20261018-160828): the use press goes through 0x2011b0 -> 0x1fef10 with pressed 1 on the A press, not through the B jump gate 0x1fee94 (that one asks with pressed 0).

### Method

REA (Ghidra 12.1.2) pseudocode of 0x1a999c, 0x19b074, 0x1b6664, 0x1a9dc4, 0x1a9bb4, 0x1fee94, 0x269e30, 0x221570, 0x23e130, 0x1ff208, 0x2bba8c, 0x211a44, 0x2288c0, 0x2384b8, 0x1ff030; llvm-objdump (`--triple=armv6k --mattr=+vfp2`) of 0x2a9d94, 0x2bbbb8, 0x1d5254, 0x2c4114, 0x275000, 0x2ecb30..0x2ecce0, 0x2e82b8..0x2e837c, 0x28aca0..0x28acc8, 0x28ab5c, 0x34a87c..0x34aa14 (state objects), 0x1e0a4c, 0x2f0e6c..0x2f0f4c, 0x23df04, 0x2397a0, 0x2536c0, 0x254250, 0x22b62c, 0x221e5c, 0x221920, 0x29f544, 0x23602c, 0x23ca4c, the class descriptors (`mov r0,#type; bl 0x246240`) of types 20, 29, 30, 101, 187, 218 and the state registrations 0x16f910, 0x34a1ec, 0x346ff8, 0x359a50. Class files dumped with `parseLevel` (class parameters and bounds). Oracle: state `m12-9-zipline` (New 3DS, OpenGL, no RPC, port 24710, home `artifacts/oracle/r20-A`), scheduler step fixed with `dt 0.033333335 0.033333335`; runs: `trace 3 zip-press mem` with B held then `trace 200 zip-ride mem`; traps at 0x2bbbb8, 0x2a9d94, 0x2a9f04, 0x2a9f80, 0x275000; a B press after 60 slide updates (`zipb-*`); reads of the state table `*0x3742c8` (0x0835be30), the line list and the zipline object.

### Zipline class (type 101)

- Class `props/parkour_zipline`: `GrappleLineType` 1, meshname `Ability_Parkour_Zip_Line`, defaults `ZipLineSpeed` 7, `FaceTowards` 1, `RopeTexture`, `TargetTightropeAnchor`; bounds `WalkBound` centre (0, -1.5, 0.4) half (0.5, 1.5, 0.8), `UseBound` centre (0.028, 0.1505, 0.2108) half (0.5, 1, 0.5), `HintBound`. `props/parkour_tightrope` is the same type with `GrappleLineType` 0 (player state 41). lvl_01 has 24 zipline anchors; `1-8_Zip_Line08` sets `ZipLineSpeed` 30.
- Descriptor 0x18c054: create 0x1a999c, link 0x19b074, draw 0x1a9dc4 (rope between the anchors with `RopeTexture` and `RenderWidth` 0.03, faded near the camera, through 0x297b7c), handler 0x1b6664, destroy 0x1a9f18, post-link 0x1a9bb4.
- Create: +0x154 `ZipLineSpeed` (7.0 when absent), +0x162 `GrappleLineType`, +0x160 `ATTR_SFX_FOOTSTEP`, +0x158 `RenderWidth`, `WalkBound` copied to +0xf8 (centre) / +0x104 (half), +0x110 = centre z - half z (-0.4); use zone registered by 0x23e130 (class `UseBound`, active from `UseStartActive`, low 3 flag bits = `FaceTowards`).
- Link 0x19b074: +0xf4 = the target object; node forward row = normalized(target - own), then right = up x forward and forward = right x up (up stays (0, 1, 0)), so the forward is horizontal; `WalkBound` centre z = d / 2 and half z = 0.4 + d / 2 with d the horizontal distance (0x22b62c is x/z only). Post-link 0x1a9bb4 adds the object to the line list 0x592490 (count 0x37742c) when its id +0x14 is not below the target's. Oracle: 14 entries; 1-8_Zip_Line05 (object 0x08c02a30, id 0x740) is listed, its target 1-8_Zip_Line04 (0x08c02570) is not.

### Entry

- Jump update 0x1d5254 (state 4): after the move 0x20cd30 and with no pending state change, it runs the grab tests only when the signed byte ctrl+0x2cf > 4. ctrl+0x2cf is set to -1 by the state-4 entry (0x200e6c) and incremented by the third function of state 4 (0x1b8560) in every update, the take-off update included; so the tests start in the 7th update counted from the B press. Tests in order: 0x206668, 0x206510, 0x206318, 0x20629c, 0x206240, 0x21e038, 0x2bbbb8 (lines), 0x273f6c, 0x20614c (not read). 0x2010d4 (callers 0x206914, 0x26ae10, 0x31fac8) also calls 0x2bbbb8 for the player (not read).
- 0x2bbbb8: needs character ability 11 or 12 (0x23a70c), state not 39 / 41, not carrying a big object (0x1ff208); 0x2a9d94 returns a line: ctrl+0x134 = line, state 42 when `GrappleLineType` is not 0, else 41.
- 0x2a9d94: point = node transform of entity+0x60 with y replaced by the node y, moved along the up row of a matrix built from the 3D line direction by h = the largest height of the character's attachment set 18 (0x2bba8c -> 0x2288c0 over ctrl+0xe4, 0x211a44). For each listed line: the point and the target in the line's node frame (0x2536c0), y minus target y times z / target z, inside `WalkBound` (0x2397a0, inclusive). So the point must lie 0 to 3 below the line, within 0.5 to the side and from 0.4 before to 0.4 past the anchors. Oracle (trap 0x2a9f04, m12-9): entity+0x60 = (0, 1.15, 0.08), h = 0 (grab point = base point (870.3043, 67.5988, 630.9428) for player node (870.22626, 67.5988, 630.96051)), the 6th listed line hit.
- Use press: the B edge offers message 3 through the registry walk 0x1fee94 before the jump (as for carry objects, [carrying.md §2026-10-15T11:11](carrying.md#ts-20261015-111115)); 0x221570 for the zipline zone: player position in the anchor's frame (flag bit 5 clear), FaceTowards 1 needs dot(player forward, normalized(anchor - player)) >= 0, then the zone box (0x22f650). Handler 0x1b6664: ability 11 or 12, pressed: ctrl+0x130 = ctrl+0x134 = anchor; type 1 only when the anchor is higher than its target (else 0xff, the jump goes on), state 42. Not measured.

### States 42..44

- The table `*0x3742c8` holds no functions for states 39..44 (read: words 0..4 are 0, +0x14 the state objects 0x592528, 0x592548, 0x592508, 0x592584, 0x5925a0, 0x5925bc). The objects (built at 0x34a87c..0x34a9ec, registered at 0x28ab5c) carry id, animation role (+0x18), blend 0.15 (+0x14) and a vtable: +8 enter, +0xc per-update function (called with dt in s0), +0x10 exit.
- 42 "Zipline Get On" (vtable 0x3626b8, role 22 `zipline_grab`): enter 0x2ecb30: ctrl+0x130 = ctrl+0x134, swapped to the target when that is higher; attach 0x275000; clip; 0x28aca0 plays sound *(0x377470+0x20) = 107 (`abilities/zipline_loop`). Update 0x2c4114; exit 0x2ecc14 stops sound 107 unless the next state is 43. Events: 1 (clip done) -> 43, 6 (B) -> 44.
- 43 "Zipline Slide" (vtable 0x3626d4, role 21 `zipline_slide`, looped): update 0x2c4114, exit stops sound 107. Event 6 -> 44.
- 44 "Zipline Jump" (vtable 0x3622a8, role 23 `zipline_dismount`): enter 0x2e82b8: clip, ctrl+0x2b0 = y, ctrl+0x2b8 = 1, ctrl+0x2b4 = 0; update 0x2e8368 -> 0x215804. Event 1 -> state 5.
- Attach 0x275000: interaction flag ctrl+0x314 bit 6 cleared; start = line node transform of (0, 0, +0x110), end = target node transform of the same point (0.4 past each anchor along its forward); ctrl+0xa = int(atan2(dx, dz) x 10430.378) of end - start; L = |end - start|; ctrl+0x250 = 30 (0x23ca4c) / (L / (dt (0x23602c) x speed)); ctrl+0x24c = |player - start| / L (the closest point 0x29f544 computes is not used).
- Update 0x2c4114 (s0 = dt): t += rate x dt; when t > 1 or t < 0 and the x/z dot of (target - player) with the facing is negative, event 6 is sent and the player does not move; otherwise the player moves 0.2 of the way to anchor + (target - anchor) t (raw anchors, not the extended ends) and ctrl+0x2b8 = height lost; ctrl+0xc bit 0 cleared; 0x1ff030 turns the facing to ctrl+0xa by at most *0x3598c4 = 0x1900 per update; a disabled line (+0x10 bit 0) requests state 5.

### Oracle measurements (m12-9-zipline, dt fixed at 1/30)

- B at the first update: jump updates 1..6, grab in update 7; ctrl+0x250 = 0.273670 and ctrl+0xa = 15683 for the whole ride (L = 7 / 0.27367 = 25.578, the horizontal-forward ends; 3D-forward ends would give 25.598); t = 0.14534 after the first update, + 0.0091223 per update; positions and t of the replica match the recorded rows to 2e-4 and 2e-5 (CTest `zipline-real-data`).
- State 42 for 38 updates (19 frames of `zipline_grab` at 15 per second), 43 until t = 1.04845: event 6 in update 99 after the grab, state 44 in update 100, state 5 in update 130 (30 updates = 15 frames of `zipline_dismount`), soft landing (state 6) 12 updates later at y 58.02. Slide 6.786 u/s (0.22621 per update) on 1-8_Zip_Line05.
- Dismount motion: the root track of `zipline_dismount` along the facing, linear between keys at 15 per second, no move in the entry update; end position differs by up to 5e-4 from the linear values. During state 44 ctrl+0x2b8 = height lost per update; ctrl+0x2b4 equals it while rising and the height lost over the current key interval while descending (0.45688 at the end); state 5 continues with these (gravity 0.05 per update).
- B after 60 slide updates: state 44 in the same update, no slide move, root motion from frame 0.

### Other moves (survey)

- Pole and drain pipe, type 187 (`prop_poleclimb`: `ATTR_CharacterSnapToOffset` 0, `UseBound`, `CollisionBound`; `parkour_drainpipe`: snap offset 0.1, `CanDismount`, 21.8 high bounds; `1-5_Drain_Pipe05` at (940.457, 45.371, 683.463) is the pole of m12-10). Descriptor 0x16f910: create 0x185420, destroy 0x22f0a0, word +0x14 0x191ac0. States (0x24615c: update, enter, third function): 0x11d "POLE MOUNT" (0x28d908, 0x26b700, hook 0x25f754), 0x11e "POLE CLIMB UP" (0x1d579c, 0x282fec, 0x28bcb4), 0x11f "POLE CLIMB DOWN" (0x1d579c, 0x29a274, 0x2a404c), 0x120 "POLE IDLE" (0x283f18, 0x25f7c4), 0x121 "POLE DISMOUNT" (0x2ac32c, 0x28c10c). String "PoleClimb" at 0x185590 (in the create).
- Climbing bricks, type 20 (`parkour_brickgrab_new`: eight object references, `IsTopBrick`, `GrabBound`; the m12-11 bricks are `1-2_Brick_Grab_07` / `08`, with `parkour_brickgrab_hintbound` type 43): descriptor 0x18b06c words 0x1a632c, 0x198ae0, 0x1a649c, 0x1b53a8, 0x1a642c, 0x1a63e4. Free climb wall, type 30 (`parkour_wallclimb`: `ExitType`, `CollisionBound`, `HintBound`): descriptor words 0x1a6900, 0x198f0c, 0x1b5454, 0x1a6b50, 0x1a69c4. Climb wall state objects (init 0x34a1ec): 99 Idle (role 48), 100 Up (49), 101 Down (50), 103 Left (51), 102 Right (52), 104 Up Left (53), 106 Up Right (54), 105 Down Left (55), 107 Down Right (56), 109 Lean Left (57), 108 Lean Right (58), 110 Outro (59). Which class drives them is not read.
- Rail and climb bar, type 29 (`parkour_railgrab`, `prop_climbbar`, `parkour_flagpole` with `AcrobatPole`: `PrimaryNextBar` / `PrimaryPrevBar` / `Secondary*`, `NarrowBar`, `CrumbleDelay`, `CrumbleTrigger`, `TriggerObject`, `FaceJumpDir`, `GrabBound`; m12-12 rails `1-2_ClimbBar_02` / `03` linked by `PrimaryNextBar`, corner `1-2_ClimbBar_04` type 218 with `UseBound`): descriptor 0x17f238 words 0x198b6c, 0x18b108, 0x1a6720, 0x198eb8, 0x1a6744, 0x198e28, 0x198da8; type 218 descriptor 0x17754c. State records at 0x359a40 (0x1c bytes: id, update, enter, -, hook, name): 89 "CLIMB BAR IDLE", 90 "LEFT", 91 "RIGHT" (0x2a1c54, 0x27fccc, hook 0x273e04), 92 "CLIMB BAR HOT" (0x2b7528, 0x2a1c08). String "Ledge" at 0x198d80 is used by the type 29 code.
- Ledge: state objects at 0x346ff8: 23 "Ledge dismount clamber" (role 13), 24 "Ledge distmount jump" (role 14).

Unknowns:
- The attachment set 18 (ctrl+0xe4) of other characters, which makes the grab point higher; empty for Chase in m12-9.
- The entity id +0x14 that orders the line list (level order is not it: 0x740 for object 848); the fall-state path 0x2010d4 and the other jump-update tests.
- 0x215804: ground contact while the dismount runs, the exact rule of ctrl+0x2b4, the key interpolation (5e-4 off at the end).
- The use press on an anchor was not measured; the rope drawing; the code of the pole, brick, wall, bar and ledge states.

<a id="ts-20261018-163726"></a>

## 2026-10-18T16:37:26+02:00 — Zipline use press: A (0x2011b0 -> 0x1fef10 with pressed 1), not B (REA pseudocode; port fixed)

REA (Ghidra 12.1.2) pseudocode read again: 0x2011b0, 0x1fef10, 0x1b6664. Confirms [§2026-10-18T16:08:28](player-traversal-pole.md#ts-20261018-160828), which already corrected the B press of [§2026-10-18T14:52:30](#ts-20261018-145230), and records the port fix.

- 0x1fef10(entity, pressed, out) walks the use registry; for each zone that passes 0x221570 and is not flagged 4 it sends message 3 (4 for entities other than the player) with a record {entity, ability byte of entity+0x7c+0x2cc, pressed}; the first result other than 0 and 0xff is returned. 0x2011b0 (the A press path, `pressed` 1) calls it with 1 for the player; the B path 0x1fee94 uses pressed 0 (a hint query).
- The zipline handler 0x1b6664 (message 3) returns 0xff unless the pressing entity has ability 0xb or 0xc (0x23a87c on the ability byte), and acts only when the record's pressed byte is set: it stores the anchor in ctrl+0x130 / 0x134 and requests state 0x29 (anchor type 0) or, for type 1, state 0x2a only when the anchor is higher than the target (else 0xff). So the press that gets on is A, and B never does.
- Port: `LevelSession::movePlayer` (`src/game_zipline.cpp`) runs `ziplineUseTest` on the A press (`pole.usePressed`, the A edge) on the ground; B gets on only through the jump grab. The pole use test runs first in the chain, so a pole zone overlapping an anchor's zone takes the press (registry order not modelled). CTests `game-script-zipline-use-a` / `-use-b` stand 0.19 m before 1-8_Zip_Line05 (level data, not an oracle position).

Unknowns:
- The press was not measured in the oracle (no state on a zipline start platform); the ability test 0xb / 0xc of the handler is not in the port.
- The order of the use registry between a pole and a zipline zone.

<a id="ts-20261018-203804"></a>

## 2026-10-18T20:38:04+02:00 — The A presses at the zipline anchor and the pole are not measurable from the existing states (round 23 emulator servant, RPC, one try each)

- `m12-9-zipline` (three loads agree): Chase stands on the ground at 870.226 66.001 630.961 (state 1) in front of the blue pole, about 4 m below the anchor of 1-8_Zip_Line05 (868.094 69.992 630.977); the state fits the jump grab only. A held there does nothing (state stays 1, position unchanged over 70 updates at dt 1/30). Teleports onto the platform (867.9 69.95 630.98 and four other x values) gave no grab either, but Chase never moved in y afterwards, so those runs are not evidence about the press (unusable measuring aid; scenario files in the servant's scratch directory, output `artifacts/oracle/r23-servant-out/zipa*`, `zsw*`, not committed).
- `m12-10b-pole-side`: Chase at 939.263 58.301 681.421, facing (-0.455, 0, 0.891), 2.36 m from the axis of 1-5_Drain_Pipe05 with the pipe 57 degrees to his right; A held for updates 1 to 3 (edge in update 2) gives no mount through update 13 (`poleb_A.csv`, `poleb_post.csv`). The zone size behind that outcome was not read.
- `m12-10a-pole-foot` faces 174 degrees away from the pipe; turning to it walks him into it, and the walk-in mounts without A, so an A press there cannot be told apart.
- States that would settle both (the user deferred them, 2026-10-18): Chase on foot within about 0.5 m of a zipline high anchor on its platform, facing it (and a second one about 2 m back); Chase standing 1.0 to 1.5 m from the drain pipe axis of m12-10, facing the pipe, without touching it (and a second one at 0.9 m).

Unknowns:
- Whether the teleported spots on the zipline platform were inside the anchor's `UseBound` and on a floor.

<a id="ts-20261019-085131"></a>

## 2026-10-19T08:51:31+02:00 — The zipline rope: draw 0x1a9dc4 -> 0x297b7c -> strip 0x1f9590 (round 27, REA with the Ghidra provider on `game.arm.elf`)

Corrected by [player-traversal.md §2026-10-19T10:00:18](#ts-20261019-100018): in the oracle view of m12-9 both anchors of the line drew it (two strips per frame with swapped ends), not only the one with the higher id.

Read: 0x1a9dc4 (descriptor draw), 0x297b7c (batch draw), 0x1f9590 (strip), 0x1a999c (create), 0x1a9bb4 (post-link), 0x19b074 (link), and the helpers 0x2536c0, 0x22859c, 0x24d434, 0x23b998, 0x23b960, 0x226644, 0x22e1f4, 0x24a6cc. Ported in `src/runtime/zipline_rope.{hpp,cpp}` (data, fade, strip, draw filter) and `src/render/rope.{hpp,cpp}` (GL), glue in `src/game_zipline.cpp`; CTests `zipline-rope` and `zipline-rope-real-data`.

- Per anchor (create 0x1a999c): `RenderWidth` (+0x158, default literal 0x3cf5c28f = 0.03), `FadeNearCamera` (+0x163 bit 1). `RopeTexture` (+0x15c) is a string parameter loaded by name in the post-link 0x1a9bb4 (0x250244); an empty or missing name sets entity byte +0x13 to 2 and nothing is drawn. The bounds the post-link builds for the rope node are not needed for the picture. All 24 anchors of lvl_01 use the class value `models/textures/prop_generic_ropealphat_a3i5.tga`, width 0.03 (the class has no instance override), `FadeNearCamera` 0.
- Link 0x19b074 also builds a frame at +0x114 (identity, then 0x22859c): row 2 the unit direction from this anchor to its target (0x23b998, fallback (0, 0, 1)), row 0 `(dz, 0, -dx)` when |dy| < 0.98 (compared as the integer 0x3f7ae149) else `(0, -dz, dy)`, normalised, row 1 = row 2 x row 0, translation (+0x144) the anchor node position; alpha byte +0x164 starts at 0xff.
- Draw 0x1a9dc4 per anchor: nothing when there is no texture; nothing when the anchor's own entity id (+0x14) is lower than its target's and the target is "live" (0x226644: flags +0x10 & 0x25 == 0 and (+0x1a != -1 or +0x13 != 0)), so the anchor with the higher id draws the line, as it is the one in the line list; nothing when its own flags +0x10 bit 0 are set. The queue position is the midpoint of the two node positions (0x236080 with the callback 0x297b7c).
- Fade (only with `FadeNearCamera`): the camera node position (0x22e1f4(0), 0x250810, +0x30) expressed in the frame at +0x114 (0x2536c0); d = sqrt(x^2 + y^2) of its first two components, the camera's distance from the infinite line. d >= 5: alpha 255; d <= 2: not drawn (stored 0); between: `(int)((d - 2) * 0.33333334 * 255)`. Without the flag the alpha stays 255.
- Batch 0x297b7c: render state from the alpha: alpha 255 gives mode bytes (1, 0) and sets state flag bit 3 (opaque path), any other alpha gives (4, 5) and clears bit 3 (blend path); the state is applied (0x25511c) whenever the texture changes or the alpha changes between 255 and not 255. The test of a texture alpha channel (0x19613c) is a stub that returns 0. Each strip: `0x1f9590(width, 0.0, 1.0, own node position, target node position, colour = alpha << 24 | 0xffffff)`.
- Strip 0x1f9590: `side = normalise(cross(end - start, normalise(N)))` with N the three floats at 0x529fec; the vertices are relative to the midpoint, the model matrix is the identity with the midpoint as translation (0x253b6c). Triangle strip (primitive mode 6, format 7 at 0x254d90): (start + side w, u 1, v 0), (start - side w, u 0, v 0), (end + side w, u 1, v L), (end - side w, u 0, v L), colour (255, 255, 255, alpha) for each, w = RenderWidth. L = max(|dx|, 2 |dy|, |dz|) of the end minus start (divided by param 3 = 1.0). No sag: the rope is straight between the two nodes. The matrix inverse 0x258858 / 0x253b6c work on a copy of 0x52fce4 and do not change the geometry.
- 0x529fec is row 2 (floats 8..10) of the 4x4 matrix cache written by 0x24a6cc, which the begin and end calls of the primitive (0x254d90, 0x254460) fill from the current matrix. It holds the matrix of the previous draw call, not a constant; the port uses the camera's view direction for it (TRAV-S27).
- Port check: `chase-game` from the m12-9 position (`--start "870.226 66.001 630.961" --facing 102.8`) shows a thin textured rope going from the anchor of 1-8_Zip_Line05 down to 1-8_Zip_Line04 (screenshot not kept, no game pictures in git). The rope draws after the scene and the particles; fully opaque ropes write depth and do not blend, faded ones blend without depth writes (TRAV-S28).

Unknowns:
- The value of the matrix row at 0x529fec when 0x1f9590 runs (TRAV-46); only the sign and the screen-facing of the ribbon depend on it.
- The meaning of the mode bytes (1, 0) / (4, 5) and the state bit 3 in 0x297b7c (blend factors, depth write, alpha test), and the texture wrap and filter (TRAV-47).
- The tightrope anchors (`GrappleLineType` 0) share the descriptor 0x18c054 and so draw the same rope; they are not loaded as anchors yet (TRAV-48).

<a id="ts-20261019-100018"></a>

## 2026-10-19T10:00:18+02:00 — The rope ribbon's facing and render state (round 28: gdb trap T46 on m12-9-zipline, llvm-objdump of 0x297b7c, 0x252254, 0x25511c)

[Corrects player-traversal.md §2026-10-19T08:51:31](#ts-20261019-085131) on the draw gate: the line is drawn twice.

**Facing (TRAV-46).** Trap at 0x1f9590 and 0x1f95bc (state `m12-9-zipline`, `dt 0.033`, 4 updates, port 24784, raw `artifacts/oracle/r28D-out/T46.txt`): per update two calls of 0x1f9590 from 0x297ce4 with start and end swapped: (892.20288, 64.4198, 632.59888) to (868.09351, 69.9924, 630.97711) and the reverse, colour 0xffffffff (alpha 255). At 0x1f95bc r1 = 0x529fec holds (0.96139693, -0.1675961, -0.21823755) in all four updates, a unit vector; its horizontal part (0.9753, -0.2214) equals the camera's (sin, cos of the camera yaw 1.7940) and the next four floats of the matrix cache hold the camera position (860.47, 69.90, 633.17). So row 2 of the cache is the camera's view direction while the rope draws, as the port assumed (`viewDirection`); `side = normalize(cross(end - start, N))` follows. The port's strip with these inputs is pinned in `zipline-rope` (expected vertices computed from the 0x1f95bc formula, not from a measured vertex buffer). Only one view and one camera (4 identical updates) were measured.

**Two draws per line.** The batch 0x297b7c loops over the queued entries (16 bytes each, anchor at +8) and calls 0x1f9590 once per entry with (own node, target node); both anchors of the m12-9 line were queued, so the original draws the line twice (opposite end order, same geometry). The draw gate of 0x1a9dc4 skips the lower-id anchor only when its target is "live" (0x226644), which was not the case here. The port draws one strip per line; with alpha 255 the second strip is invisible; with a faded rope (`FadeNearCamera`, not used in lvl_01) it would blend twice.

**Render state (TRAV-47).** 0x297b7c builds a 0x60-byte material record with 0x252254 (zero, byte 0xa = 9: one texture, byte 0xb |= 0x48, byte 0xc = 0x3e: cull none, colours 0x10 and 0x14 = 0xffffffff, byte 6 = 127, the one texture slot with byte 6 = 5), clears bit 6 of byte 0xb (so fog applies), sets byte 5 = 1 and the blend bytes 8 / 9: (1, 0) with bit 3 set (depth write) for alpha 255, (4, 5) with bit 3 cleared otherwise; 0x25511c applies it through 0x128dc8 / 0x1292fc like a model part. Through the port's `materialState`: opaque = blend one / zero, depth write, depth test LEQUAL, no alpha test, culling off; faded = source alpha / one minus source alpha, no depth write; colours = texture x vertex colour, alpha from the texture times the constant (not from the vertex alpha); fog on. This matches the states the port already used (TRAV-S28), pinned in `zipline-rope`. Note that the opaque path ignores the texture's alpha (A3I5 texture `prop_generic_ropealphat_a3i5`): transparent texels are drawn with their colour.

Unknowns:
- The texture's wrap and filter: the state's texture slot has bytes 4 and 5 zero and 0x25511c takes only the texture handle (+0x14) from it, so the sampler comes from the texture object (the loader 0x250244 was not read).
- Fog on the rope: the record asks for it; the port's rope pass has none.
- Why the line's other anchor was not skipped (the state of 0x226644 for the anchors) and whether other lines draw once.

<a id="ts-20261019-103841"></a>

## 2026-10-19T10:38:41+02:00 — Fog on the zipline rope (round 29, r29-C: port change, no new emulator run)

Fog (TRAV-50). The render state of the rope ([§2026-10-19T10:00:18](#ts-20261019-100018)) clears bit 6 of byte 0xb of the material record built by 0x297b7c, so the material is fogged like a model part whose fog bit is set. The PICA fog registers (table at 0xe8, colour, projection) are global GPU state written by the scene setup, so the rope sees the fog of the room the scene is in: in the port `LevelSession::applyFog` sets it once per frame through `render::setFog`, and `RopeRenderer::draw` now reads it with `render::currentFog()` and uploads the same table, colour and projection near / far to the particle program. The particle program (shared by particles and the rope) got the scene shader's `applyFog` and a `fogOn` switch that is never set for particles, so their output is unchanged; the rope sets it for its draws and clears it afterwards. Fog applies to the colour before blending, as for model parts. Values for lvl_01: range 220 .. 260, colour (183, 199, 221) (oracle 0x376160 / 0x376164 / 0x376168, see [rendering-fog.md](rendering-fog.md)), pinned for the rope in `zipline-rope-real-data` (the rope fog is the scene fog `levelFog` gives for Room7).

Double draw (TRAV-50, second half). Read as in [§2026-10-19T10:00:18](#ts-20261019-100018): the original draws the lvl_01 line twice (swapped ends, same geometry); at alpha 255 with depth write the second strip does not change the image, so the port keeps one strip. A faded rope (not in lvl_01) would blend twice; not ported.

Unknowns:
- No oracle picture of the fogged rope exists, so the fog on the rope is taken from the state record and the shared GPU fog state, not compared with the original's image (needs RPC screenshots at a distance of about 220 .. 260 m from a rope, or a gdb read of the fog registers while 0x297b7c draws).
- Why the line's other anchor passes the gate 0x226644, and whether other lines draw once.

<a id="ts-20261019-110428"></a>

## 2026-10-19T11:04:28+02:00 — Why every zipline line is drawn twice: the gate 0x226644 (round 30, llvm-objdump of 0x1a9dc4, 0x226644, 0x173534, 0x18c054; REA was not used)

Answers the open question of [§2026-10-19T10:00:18](#ts-20261019-100018) and [§2026-10-19T10:38:41+02:00](#ts-20261019-103841) (TRAV-51).

- Gate 0x226644(entity) returns 1 ("live") when the halfword +0x10 has none of the bits 0, 2, 5 (mask 0x25) and (the halfword +0x1a is not 0xffff or the byte +0x13 is not 0).
- Entity +0x1a is the slot in the per-frame update list (0x173534 adds the entity at the end of the list at 0x580564 and stores the index; 0x22d480 removes it and stores 0xffff). 0x173534 adds only entities whose type record (table 0x583124, stride 40, byte +0x12 is the type) has a non-zero update handler at +0xc.
- The registrar of type 101 (0x18c054) stores create 0x1a999c, message 0x1b6664, draw 0x1a9dc4, link 0x19b074, 0x1a9f18 and post-link 0x1a9bb4 in the record's fields +0, +4, +0x10, +0x14, +0x18, +0x1c and zero in +0x8 and +0xc: the zipline anchor has no update handler, so +0x1a stays 0xffff.
- So the target is live only when its byte +0x13 is not 0 (the post-link sets it to 2 for an anchor without rope texture) and its flags +0x10 have none of the bits 0, 2, 5. The draw 0x1a9dc4 skips the anchor with the lower entity id only then. With textures on both anchors (+0x13 = 0), as for all 24 anchors of lvl_01, the gate never skips: every line is drawn twice, once from each anchor (swapped ends, same geometry), as measured on m12-9.
- Port: `ziplineRopeDraws` skips the lower-id anchor only when the target has no rope texture and is enabled; flag bits 2 and 5 of the target are taken as clear (TRAV-S32). A faded rope (`FadeNearCamera`) blends twice.

Unknowns:
- What flag bits 2 and 5 of the halfword +0x10 are and whether any anchor has them set; whether +0x13 of an anchor changes after the post-link.
