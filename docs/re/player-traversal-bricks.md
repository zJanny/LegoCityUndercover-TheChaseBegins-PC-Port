# Player traversal: climbing bricks

The climbing bricks (object type 20, player states 141..152): the grab test of the jump update, hanging, reaching, the jumps between bricks and off the last brick, measured in the mission 1-2 state `m12-11`; the entry of the free climb wall (type 30). Parent topic: [player-traversal.md](player-traversal.md).

## Current state

- Climbing bricks (type 20): the m12-11 climb is the brick states 141..152, not the climb wall. Entry by the grab test 0x273f6c of the jump update (7th update, box test 0x202504 against `GrabBound`), snap 2 below the brick turned by pi, B jumps to the adjacent brick along the stick (149..151, clip root scaled to the target) or starts state 4, A drops (state 5), stick reaches (146..148); measured in m12-11 and ported (`src/runtime/brick_grab.*`). The climb wall (type 30) is entered by its handler 0x1b5454 into state 99; its states are not read. [§2026-10-18T15:57:46](#ts-20261018-155746)
- Open: the jumps to the side and state 152, the sender of the brick use message, the climb wall states 99..110.

<a id="ts-20261018-155746"></a>

## 2026-10-18T15:57:46+02:00 — Climbing bricks: states 141..152 read, ported and measured in m12-11; the climb wall's entry found (REA, objdump, Azahar GDB traces)

### Method

REA (Ghidra 12.1.2) pseudocode of 0x1a632c, 0x198ae0, 0x32c4b8, 0x32c5b8, 0x2a0e08, 0x2abcb8, 0x2bd028, 0x202504, 0x23d39c, 0x237eb4, 0x1ff198, 0x1ceebc, 0x1fa7e0, 0x2147e4, 0x1ffa30, 0x211a00, 0x229a40, 0x257f14, 0x21bbb8; llvm-objdump of the descriptors 0x18b01c (type 20) and 0x18c054 (type 101, for the word order), the handlers 0x1b53a8 / 0x1b5454, the brick update 0x1a649c and list code 0x1a63e4 / 0x1a642c, the state init 0x34999c..0x349d24, 0x1e07b8 / 0x1e0840, the event handlers 0x31a408, 0x31a49c, 0x31c430, 0x31c488, 0x3233e8, the jump enter / update 0x32c6fc / 0x32c8ec, the grab test 0x273f6c / 0x273fe4, 0x2383ec, 0x227b5c; data words of the vtables 0x3652cc, 0x3652e8, 0x365304, 0x3647bc, 0x3647d0 and the state record of 152 at 0x3579ec. Level and clip data dumped with `parseLevel` and `rootMotion`. Oracle: user state `m12-11-brick-climb` (New 3DS, OpenGL, no RPC, port 24721, own home), `dt 0.033333335 0.033333335`, `trace N NAME mem` (state ctrl+0x6c, ctrl+0x130 / 0x134, clip ctrl+0x2bc, node); runs `idle` (no input), `up` (stick (0, 120) from update 4, B in update 16), `drop` (A in update 3, B in update 49, `read` of both bricks and the list), `top` (stick (0, 120) from update 1, B in updates 2 and 37).

### What m12-11 is

- The user state hangs on `1-2_Brick_Grab_08` (object 0x08a92010) in state 145 "Brickgrab idle" with clip 90; the climb wall states 99..110 are not involved. 08 links `AdjacentBrick01` = `1-2_Brick_Grab_07` (0x08a91d00, 2.145 higher); 07 has no adjacent brick and `IsTopBrick` 0. From 07, B starts the jump state 4 and the jump grabs the rail `0x08a933c0` in state 88 in its 7th update (run `top`, update 44), the start of [TRAV-3](../plans/features/player-traversal.md).
- The free climb wall (type 30) is entered by its handler 0x1b5454 (messages 3 / 4, after 0x274188 on the wall): ctrl+0x134 = wall, state 99. lvl_01 has two (`FP_Free_Climb_02` / `03`, x 545.8), not on the mission 1-2 route.

### Brick class (type 20)

- Descriptor 0x18b01c (word order as for type 101: +0 create, +4 link, +0xc update, +0x14 handler, +0x18 destroy, +0x1c post-link): create 0x1a632c (+0x94 = the class bound `GrabBound` (centre (-0.0115, 0, 0.3857), half (0.607, 0.2, 0.3)), +0xc2 bit 0 = `IsTopBrick`, use zone 0x23e130 without a bound name), link 0x198ae0 (+0x98..+0xb4 = `AdjacentBrick01`..`08`, +0xbc = `TriggerObject`), update 0x1a649c, handler 0x1b53a8, destroy 0x1a642c and post-link 0x1a63e4 (the brick list 0x37718c: count, array; 35 entries in m12-11).
- Update 0x1a649c: with a `TriggerObject`, it is switched by 0x244f40 / 0x244f88 while the brick is held / free; while +0xb8 (the holding player) is set, +0xb8 is cleared when the holder's ctrl+0x130 is another object, and +0xc0 = 15; otherwise +0xc0 counts down to 0.
- Handler 0x1b53a8 (messages 3 / 4): refused while +0xb8 or +0xc0 is set; pressed: ctrl+0x134 = brick, state 145. Who sends it is not known.

### Entry: the grab test of the jump update

- 0x273f6c is the 8th test of the jump update 0x1d5254 (after the line test 0x2bbbb8), so it runs from the 7th jump update as the zipline's. For the player entity (type 27, ctrl+0x314 bit 6 clear) it walks the brick list; 0x273fe4 needs +0xb8 = 0, +0xc0 = 0, brick +0x8c bit 4, brick +0x10 bit 0 clear, entity+0xc bit 4 clear, not carrying (0x1ff208: ctrl+0x13c = 0) and 0x227b5c (type 20 case: a character test through 0x219b80 / 0x239678, not read).
- Facing: dot(brick forward row, player forward row) > 0.75 fails. Box test 0x202504 (separating axes, 15 axes): player box with half extents entity+0x6c with y x 0.25 at the node position plus (entity+0x60.x, entity+0x60.y x 1.75, entity+0x60.z) added unrotated, in the player's node frame (0x237eb4); the `GrabBound` box turned into that frame (0x254250, 0x1ff198 -> 0x1ceebc: r[i][j] = brick row i . player row j). Read in m12-11: entity+0x60 = (0, 1.15, 0.08), entity+0x6c = (0.6, 0.85, 0.52). On success: ctrl+0x134 = brick, state 141.

### States and events

- Objects (vtable +8 enter, +0xc update, +0x10 exit; blend +0x14 = 0.15; clip +0x18): 141 "Brickgrab grab" (95 `brickgrab_grab`), 142 "grab up" (98), 143 "grab left" (96), 144 "grab right" (97): vtable 0x3652cc, enter 0x32c4b8, empty update; 145 "idle" (90), 146 "reach up" (101), 147 "reach left" (99), 148 "reach right" (100): vtable 0x3652e8, enter 0x32c5b8 (clip only when it differs from ctrl+0x2bc, looped), update 0x32c6bc; 149 "jump up" (94), 150 "jump left" (92), 151 "jump right" (93): vtable 0x365304, enter 0x32c6fc, update 0x32c8ec, exit 0x32c8dc. 152 "BRICK GRAB JUMP UP" is a table record (0x3579ec): enter 0x2abcb8 (clip 91 `brickgrab_jump`, blend 0.1), update 0x2bd028.
- Enters 0x32c4b8 / 0x32c5b8: ctrl+0x130 = ctrl+0x134, brick +0xb8 = player, player node = brick node with y - 2 turned by pi about y (0x237bc4), ctrl+0xa = ctrl+8 = its heading (0x1f8384).
- Events (0x2a0e08; id at +4): 141..144: 1 "Brick Grabbed" -> 145, 6 "Brick Grab Jump" (0x31a49c), 7 "Brick Grab Use" -> state 5, 3 "Brick Grab Check" (0x31c430), 5 "Brick Grab Released" (0x3233e8); 145..148: 9 "Brick Grab Reach" (0x31c488) instead of 1; 149..151: 1 "Brick Grab Event" (0x31a408: 149 -> 142, 150 -> 143, 151 -> 144) and 5. Measured: 6 = B, 7 = A, 9 = stick held, 1 = clip done; 0x2383ec flags an event 3 (state +6 bit 14) and an event 1 (bit 15).
- 0x31c430 (check): state 5 unless ctrl+0x130 is a type 20 object with +0x10 bit 0 clear and +0x8c bit 4 set. 0x3233e8 (released): ctrl+0x2b8 = 0, brick +0xb8 = 0. 0x32c6bc: without the stick ctrl+0x134 = ctrl+0x130 and state 145 unless already requested (ctrl+0x6e).
- 0x31c488 (reach): stick direction (sin a, 0, cos a), a = ctrl+6 x 2pi / 65536: . brick right > 0.5 -> 147, < -0.5 -> 148, else . brick forward < -0.5 -> 146, else 145; a change sets ctrl+0x134 = ctrl+0x130.
- 0x31a49c (jump): `IsTopBrick` -> ctrl+0x308 = 1, ctrl+0x30c = 2, state 152. Else the direction: the stick's (as above), or without the stick yaw = acos(normalized camera row 2 . normalized(-brick forward)) + camera yaw *0x376894; when its dot with the brick forward is below -0.5, its y becomes the brick's up row y and it is normalized. Over the 8 adjacent bricks (type 20, +0x10 bit 0 clear, +0xb8 = 0): the largest positive dot of the normalized way; none -> state 4; else ctrl+0x134 = it and way . target up > 0.707 -> 149, way . target right > 0.707 -> 150, else 151.
- 0x32c6fc (jump enter): ctrl+0x2b4 = 0; the way between the two bricks (both y - 2) in the current brick's frame (0x229a40) divided per axis by the extent max - min of the clip's root track (0x2147e4 -> 0x292ef8, axes without the animation flag get extent 0) is the root-motion scale (0x211a00 -> anim+0x18), x negated for 151, z always negated; ctrl+0xa = heading of the target turned by pi; entity+0x10 bit 9 cleared (set again by the exit). Update 0x32c8ec: ctrl+8 = the current brick's hanging heading during the move 0x1fa7e0 (0x20cd30 with the stick bit cleared), then restored; 0x1f3f1c.
- 152 update 0x2bd028: the move 0x1fa7e0 with the root track; at the clip end ctrl+0x2b8 = the last update's height change, ctrl+0xa = ctrl+8, state 5 and the ground probe 0x20b48c (3.0).

### Oracle measurements (m12-11-brick-climb, dt 1/30)

- `up`: state 146 in the first stick update (ctrl+6 = 0x57ff, ctrl+0xc bit 0); B in update 16: state 149 with ctrl+0x134 = 07 in that update, no move in updates 16..18, node y 65.8505, 66.0035, 66.1571, ... 67.6898 in updates 19..31 (0.0700 x 2.1847 per half frame of `brickgrab_up`, root rising 0.14 per frame from frame 1), state 142 in update 32 at y 67.8427 (07 - 2), 145 in update 46 (14 updates = 7 frames of `brickgrab_grabup`), 146 in update 47.
- `drop`: A in update 3 -> state 5 in that update, fall 0.05, 0.1, ... per update, soft landing (6) at y 64.4281 in update 10, idle in update 36; B in update 49 -> state 4, state 141 on 08 in update 55 (7th jump update) at (908.9421, 65.6976, 673.1696), ctrl+8 = 22209 (0x56c1); 141 for 22 updates (11 frames of `brickgrab_grab`, its root track not applied), 145 in update 77. Brick reads: 08 +0xb8 = player, +0xc0 = 15, +0xc2 = 0, +0x8c = 0x18; 07 +0xc2 = 0.
- `top`: B on 07 in update 37 -> state 4 (no adjacent brick), rise 0.2655, 0.2656, 0.2699, 0.2700, 0.2536, 0.2536 per update with the stick held; state 88 on the rail in update 44.

### Port

`src/runtime/brick_grab.{hpp,cpp}` (bricks, grab test, states 141..152), glue `src/game_brick_grab.cpp` (LevelSession::moveClimbingPlayer around movePlayer); CTests `brick-grab`, `brick-grab-real-data` (lvl_01 bricks, the `up` and `drop` rows), `game-script-brick-grab`. chase-game reproduces the `top` rise to within the 0.033 s script step.

Unknowns:
- Who sends messages 3 / 4 to a brick (the use zone has no bound); the meaning of +0x8c bit 4, entity+0x10 bit 9, ctrl+0x308 / ctrl+0x30c; the 0x227b5c character test for type 20.
- The order of events inside one update (one change per update was seen: 145 entered by event 1 reached 146 only in the next update) and when event 5 is sent.
- The axes of 0x20cd30 for the scaled x / z root motion (jumps 150 / 151 not measured; the m12-11 jump uses only y), its collision during 149..152, and state 152 (no top brick on the route).
- The camera row the no-stick jump direction reads; the climb wall states 99..110.
