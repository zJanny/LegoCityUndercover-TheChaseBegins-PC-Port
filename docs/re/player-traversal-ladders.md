# Player traversal: ladders

Ladders (object kind 56: `props/prop_ladderlarge`, `props/prop_laddermedium`, `props/prop_crane_ladderlarge` and the other classes listed in [object-kinds.md §2026-10-18T21:36:13](object-kinds.md#ts-20261018-213613)) and the player states that climb them, measured in the mission 1-3 user state `m13-2-ladder` in front of the crane ladder `1-4_Ladder01`.

## Current state

- Kind 56 descriptor (registered at 0x16a10c): create 0x17f3c0 (`ATTR_LadderType`, `ATTR_CharacterSnapToOffset` default 0.3, `CanDismount` default 1, `StartFurled`, `invisible`), post-link 0x17f540 (use zone 0x23e130 from `UseBound`), message handler 0x18b458 (messages 3 / 4: ability 5, refuses a furled ladder and a player above top - clamber rise, then Ladder Mount). [§2026-10-20T00:29:37](#ts-20261020-002937)
- States: 33 Ladder Mount (4 updates of 0.25 to the snap point, facing kept, then the ladder heading + 0x8000), 36 Ladder Idle, 37 Ladder Idle (Blocked), 34 Ladder Climb Up and 35 Ladder Climb Down (0.05 x 30 x dt per update, the ladder clips have no root track), the top test `top <= height + 1.8325` gives the clamber state 23 (clip `ladder_dismount`, root motion, state 1 at the clip end); A (event 7) and the bottom of the climb down (event 10) fall with the facing turned by -0x3000, B (event 6) jumps. Replayed against the oracle in CTest `ladder-real-data`. [§2026-10-20T00:29:37](#ts-20261020-002937)
- The player mounts by the A press (0x2011b0 -> 0x1fef10) or by walking into a ladder (0x206668 in the walk, fall and jump updates); the port has the A press and the walk-in after a ground step. [§2026-10-20T00:29:37](#ts-20261020-002937)
- Open: the entry from a jump or a fall, furled ladders, the push-out after the A drop and the wall contact of the clamber, the message 0x39 to the player, sounds and camera.

<a id="ts-20261020-002937"></a>

## 2026-10-20T00:29:37+02:00 — Ladders: class handlers, the ladder states and the clamber (REA pseudocode, llvm-objdump, Azahar GDB traces of m13-2-ladder)

Answers [playtests.md §2026-10-19T20:23:38](playtests.md#ts-20261019-202338).

Evidence: REA (Ghidra 12.1.2) pseudocode of 0x17f3c0, 0x17f540, 0x17f6ac, 0x18b458 (ev_545ebe1d…), 0x325258, 0x325380, 0x3232d8 (ev_f75b0236…), 0x1e43c0, 0x32c97c, 0x32ca18, 0x32f7a0, 0x32f83c (ev_f66cbee5…), 0x206668, 0x2010d4 (ev_c94424a9…), 0x210c9c, 0x239678, 0x23deb4, 0x1ff208, 0x201c70, 0x21e854 (ev_9f97665d…), 0x302dcc, 0x21e1d4, 0x221a54 (ev_d1fb6edd…), 0x211a44, 0x2147e4 (ev_b5c80378…), 0x29f97c, 0x23e130, 0x221570, 0x23e578 (ev_2deb1ccb…), 0x23d4c0, 0x22f134 (ev_f2fce98c…), 0x32cbb0, 0x32cc18, 0x32ec70 (ev_1a2b5a95…). llvm-objdump (`--triple=armv6k --mattr=+vfp2`) of the descriptor registration 0x16a0c0, the state object initialisers 0x345c60 (ladder) and 0x346ff8 (ledge), the state registration 0x2805dc..0x280758, the event initialisers 0x34ef10 and 0x34f898, the updates 0x323378, 0x302e94 and the asm of 0x18b458 and 0x32f83c. Class files and clips through the C++ loaders. Oracle: New 3DS user state `m13-2-ladder`, Azahar OpenGL, xvfb, no RPC, port 40401, own home, scheduler step `dt 0.033333335 0.033333335`: runs `lad-up` (A, then stick (0, 120) for 200 updates), `down` (A, up 10, down 20), `adrop` (A, up 6, A) and `bjump` (A, idle, B); the four loads agreed in the mount rows.

### The ladder object

- Descriptor of kind 56 (0x16a0c0): create 0x17f3c0, post-link 0x17f540 (+0x1c), destroy 0x17f6ac (+0x18), message handler 0x18b458 (+0x14).
- Create 0x17f3c0: 0xa0-byte object, +0x9c bit 4 = `ATTR_LadderType`, +0x94 = `ATTR_CharacterSnapToOffset` (default 0x3e99999a = 0.3), +0x9c bit 1 set when `CanDismount` (default 1) is 0, `invisible` hides the node, +0x9c bit 2 = `StartFurled`.
- Post-link 0x17f540: the use zone at +0x88 through 0x23e130 (`UseBound`, or the model bounds grown by the vector at 0x56d40c when there is none; `UseStartActive` default 1, `FaceTowards` default 0); a furled ladder gets +0x9c bit 3 and plays `UnfurlAnimation` on message 0xff.
- Handler 0x18b458: messages 3 / 4 need ability 5 (0x23a70c / 0x23a87c) else 0xff; +0x9c bit 3 (furled) answers 0; with pressed 0 it answers 1. With pressed 1: the clamber rise r = root y of clip +0x1c of the mount state (13, `ladder_dismount`) at its last frame (0x221a54 -> 0x211a44 -> 0x29f97c); answer 0xff when the player node y is above ladder y + (+0x70 + +0x64) - r; else ctrl+0x268 = ladder position + snap offset x ladder forward, ctrl+0x26c = player y, ctrl+0xa = atan2(forward) x 10430.378 + 0x8000, ctrl+0x25c = player position, ctrl+0x134 = ladder, state 33.
- lvl_01 holds 10 placed kind 56 objects; both crane ladders nest `ladder_part_02` (kind 56 as well). The crane ladder class `props/prop_crane_ladderlarge`: snap 0.45, `UseBound` centre (0, 5.99124, 0.54) half (0.8, 6.19124, 0.8), `CollisionBound` centre (0, 5.98798, 0) half (0.8, 6.18749, 0.252): the parent's bounds cover both parts (top 12.17547 over 18.5162, the roof at 30.69).

### How the player gets on

- A press: 0x2011b0 -> registry walk 0x1fef10 with pressed 1 -> the zone test 0x221570 (disabled flag, UseStartActive, the FaceTowards modes 1..5 for the player, the position in the ladder's frame inside the box) -> the handler. Oracle (lad-up): Chase at (894.07965, 23.62261, 1134.04700) facing 9922 is in the zone (local x -0.334, z 1.220); the A press in row 3 already moves the first mount step.
- Walk-in 0x206668 (called from 0x2010d4 in the walk and fall chains and first in the jump-update test list 0x1d5254): for the player only, 0x1ff208 must pass (not carrying, or carrying an `ATTR_SmallObject` while 0x377162 is set), the object the move last touched (DAT_00377134) must be kind 56; when the object 0x239678 names (index at entity+0x80) is kind 56 it is used instead; 0x210c9c then runs the zone test, asks with pressed 0 and sends pressed 1; on success ctrl+8 = ctrl+0xa and the node is turned (0x23deb4). Not measured.

### The states

- State objects (initialiser 0x345c60, blend 0.15, registered at 0x2805dc..0x280758 with their events): 33 "Ladder Mount" (vtable 0x365070, clip 10 `ladder_idle`, +0x1c clip 13), 36 "Ladder Idle" and 37 "Ladder Idle (Blocked)" (vtable 0x364f2c, clip 10, looped), 34 "Ladder Climb Up" (0x365320, clip 11 `ladder_up`), 35 "Ladder Climb Down" (0x3655a8, clip 12 `ladder_down`). Events: all have "Hide All Weapons" and "Unshoveable"; 36, 37, 34 and 35 add "Ladder Jump Off Event" (type 7, state 5) and "Default Jump Event" (type 6, state 4); 34 adds "Ladder clamber up" (type 10, state 23), 35 adds a second "Ladder Jump Off Event" (type 10, state 5). The jump-off handler 0x32cbb0 requests the state, ctrl+0xa += 0x8000 (the ladder heading, ctrl+0xa = ctrl+8 on the ladder), ctrl+8 -= 0x3000, sends message 0x39 with ctrl+0x130 to the player and clears ctrl+0x130.
- Mount enter 0x325258: ctrl+0x130 = ctrl+0x134, 0x210c9c(player, ladder, 0), clip 10, ctrl+0x24c = 0, ctrl+0x250 = the clamber rise (player +0x70 + +0x64 = 2.0 without the clip), ctrl+0x25c = position. Update 0x325380: ctrl+0xa = heading to the ladder (0x201c70, the facing does not turn), ctrl+0x24c += dt x 30 x 0.25, position = lerp(ctrl+0x25c, ctrl+0x268, ctrl+0x24c); at 1: state 36 (37 when the pending state is 37), ctrl+0x24c = player y - ladder y, the node = the ladder node turned by pi at the snap point, y = ctrl+0x26c.
- 0x1e43c0 (idle update and the step of the climbs, s1 = 0 / +1 / -1): ctrl+0xa = ctrl+8; with the stick held the quadrant ((ctrl+6 - camera yaw x 10430.378) + 0x2000) >> 14 asks for 34 (0, or 37 when blocked) or 35 (2); the step is the root motion of the update (0x2327b4) or 0.05 (*0x377274) x sign x 30 x dt, times ctrl+0x29c + 1; without the stick it asks for 36 (37) and returns 0. The ladder clips have no root track, so the step is 0.05 at dt 1/30.
- Climb up 0x32ca18: restarts the clip when its slot reports 6 (0x22f560), height += step; when ladder +0x70 + +0x64 <= height + ctrl+0x250: with `CanDismount` message 0x39 to the player, ctrl+0x130 = 0, event 10 (clamber); without it the step is undone and a positive step asks for 37; otherwise node y += step.
- Climb down 0x32f83c: the step with sign -1, then the ray 0x22f134 (from the player box bottom, node + (0, 1.15 - 0.85, 0.08) x dt x 30, down to node y - 0.1 - |step|); a hit or height + step < -(player +0x70 = 0.85) sends event 10 (fall, facing -0x3000), else node y += step.
- Clamber, state 23 "Ledge dismount clamber" (vtable 0x3638a8, clip 13): enter 0x302dcc sets the node up row to (0, 1, 0) and plays the clip once; update 0x302e94 = 0x215804 (the move with ground contact); "Default Anim Done Event" (type 1) gives state 1.

### Oracle measurements (m13-2-ladder, dt 1/30)

- Mount: rows 3..6 move 0.25 of the way each to (894.38904, 23.62261, 1134.82788) = ladder + 0.45 x forward, facing 9922 in rows 3..5 and 8192 (225 + 180 degrees) in row 6, ctrl+0x24c = 5.10641, state 36 from row 7. ctrl+0x250 = 1.8325 (0x3fea8f5c) in every row, before the mount as well; ctrl+0x29c = 0; entity+0x70 = 0.85.
- Climb: the stick in row 16 asks for 34 without a move, rows 17.. rise 0.05 per update (y 23.67261, ..., 28.82253 in row 120). A sideways or reversed stick is read in the update but the current direction still moves once (down run row 17).
- Top: in row 121 height 10.35643 + 1.8325 >= 12.17547: no move, ctrl+0x130 = 0, state 23 from row 122. Rows 122..126 do not move; from row 127 the node moves by the root motion of `ladder_dismount` (15 frames at 15 per second) between frames 0.5 (n - 2) and 0.5 (n - 1) of the n-th update of state 23, turned by the facing (x/z to 2e-4 in rows 127..130); in row 131 the forward motion stops (the player is below the roof edge), so x/z trail the root motion by one update from there; y follows the root motion up to 31.0219 and snaps to the roof 30.71885 in row 138 (0.092 above it while descending). State 1 in row 152 at (894.80237, 30.71885, 1135.29028), 31 updates after the top.
- Down run: rows 18..24 descend 0.05; in row 25 the ray reaches 23.6226 and meets the platform (23.624): no move, facing 8192 - 0x3000 = -4096, ctrl+0xa -24576, state 5, landing in row 27.
- A in row 13 of adrop while climbing: state 5 in that update, no move, facing -4096 (ctrl+0xa -24576 as in the down run follows from the jump-off handler; it was not read in this trace); row 14 is pushed 0.22 m off the ladder ((-0.155, +0.155)) and falls 0.05. B in row 10 of bjump while idle: state 4, no move, facing 8192; row 11 rises 0.27306 (the standing jump).

### Port

`src/runtime/ladder.{hpp,cpp}` (loader, zone, handler gate, walk-in and use tests, mount, idle, climbs, clamber, exits), `src/game_ladder.cpp` (`LevelSession::movePlayerWithLadder` around the pole step), ladder clips loaded in `src/game_player.cpp`; the pole's box touch is shared as `playerBoxTouches`. CTests `ladder` (made-up ladder), `ladder-real-data` (lvl_01 ladders and the four runs), `game-script-ladder` (A, climb, clamber onto the roof) and `game-script-ladder-walk-in`.

Unknowns:
- Whether the walk-in mounts in the touch update as the A press does (the port runs the first mount step at once in both).
- The writer of *0x377274 (0.05, the climb step) and the source of entity+0x70 (0.85) of the player; both were read in m13-2-ladder.
- How the clip slot raises the Default Anim Done Event; the port ends the clamber when the clip time reaches its length - 1e-3.
- The meaning of entity+0x80 (0x239678) for a nested ladder part; the ability table behind ability 5; DAT_00377162 in the carry gate 0x1ff208; what the player does with message 0x39.
- The push-out of row 14 after the A drop and the wall contact of 0x215804 in the clamber; the ray 0x22f134 reaches 0.1 past the step (the pole port's TRAV-S8 uses the step only).
- Whether ctrl+0x250 = 1.8325 of the pole (TRAV-S5) comes from the same clip: it held 1.8325 before the ladder mount too.
- Furled ladders (`StartFurled`, messages 8 and 0xff, `UnfurlAnimation`), the sounds of the climb (`sounds/movement/ladder_*`) and the camera on the ladder.
