# AI states: AISuperJump, face target, run away, stalk, pursuit

Update functions and set-up functions behind the AI natives that are not movement along a path ([ai-natives.md](ai-natives.md), [ai-navigation.md](ai-navigation.md)). Evidence: llvm-objdump (`--triple=armv6k --mattr=+vfp2`) of `artifacts/game/game.arm.elf`; every address is for the Europe v1.0 build ([game-version.md](game-version.md)).

## Current state

- State table built at 0x2a3c24 (global 0x3771f0): record N at +16 N, word 0 is the update function; state 10 is 0x2c5bc4, 11 0x2c3bd8, 25 0x28b7a0, 27 0x2c1c90, 35 and 36 0x2ab6cc. [§2026-10-14T21:21:58](#ts-20261014-212158)
- AISuperJump set-up 0x213300 computes heading, distance, frames and height (ctrl+0x308 / +0x30c) and requests state 11 after locomotion state 72; ported as `planSuperJump`. [§2026-10-14T21:21:58](#ts-20261014-212158)
- State 11 ends when `ATTR_RunSpeed` (default 0.23; ctrl+0x8c, not "MinDistance") exceeds the horizontal distance to the target. [§2026-10-14T21:21:58](#ts-20261014-212158)
- State 10 (face target) pins the node to its home position and turns toward the target each tick; it never ends by itself; ported as `stepFaceTarget`. [§2026-10-14T21:21:58](#ts-20261014-212158)
- AIRunAway requests state 25 (0x228c70), AIStalkPlayer state 27 (0x282b88), AIPursuit state 35 or 36 (0x1d4688). [§2026-10-14T21:21:58](#ts-20261014-212158)
- The flight: state 72 starts state 73 (update 0x28e4cc) through the jump enter 0x200cc4, which takes the climb ctrl+0x30c as the jump height of the jump-run clip; 0x203d18 moves the character ctrl+0x308 x 30 x dt per update along its facing; after 28 counts of ctrl+0x2cf the jump becomes a fall, a landing ends it. The horizontal step is ported. [§2026-10-25T16:37:12](#ts-20261025-163712)
- Oracle: the counter ctrl+0x2cf counts one per update, the facing ctrl+0xa equals the heading from the first move update, the horizontal step matches 0x203d18 to 2e-3; the height follows the jump-run clip with a slow first frame and a fall after the clip. [§2026-10-25T16:46:40](#ts-20261025-164640)
- Stand-in SCRIPT-S103: the port's height is a parabola and a step longer than the distance left is cut at the target, which ends the jump. [§2026-10-25T16:37:12](#ts-20261025-163712)
- Open: the height and the landing of the flight, the update functions of states 25, 27, 35, 36 and 2, the follow-up lookup 0x1f8d5c, and non-player `ATTR_RunSpeed` values.

<a id="ts-20261014-212158"></a>

## 2026-10-14T21:21:58+02:00 — AISuperJump set-up 0x213300, state 10 (0x2c5bc4), state 11 arrival value, state numbers of stalk, run away and pursuit

Corrected by [ai-states.md §2026-10-25T16:37:12](#ts-20261025-163712): ctrl+0x308 is read by the horizontal step 0x203d18 and ctrl+0x30c by the jump enter 0x200cc4; state 72 only starts state 73.

[Corrects ai-navigation.md §2026-10-14T19:47](ai-navigation.md#ts-20261014-194710): ctrl+`0x8c`, which state 11 calls "MinDistance" (default -1.0), is `ATTR_RunSpeed`.

**State table.** Record N of the table built at 0x2a3c24 (global 0x3771f0) is at +16 N; word 0 is the update function. Read from the literals 0x2a3ea4..0x2a3f64: state 2 0x2c4708, 4 and 5 0x2c5d3c, 9 0x2c3a50, **10 0x2c5bc4**, 11 0x2c3bd8, 12 0x2c3d1c, 22 0x2cbe14, **25 0x28b7a0**, **27 0x2c1c90**, 35 and 36 both 0x2ab6cc.

**AISuperJump set-up 0x213300(entity, target, frames, height)** (AISuperJump passes frames = 28 and its height argument). v = target - node position (0x257f14, node = 0x250810(entity+0x3c), position at +0x30); the target is stored at ctrl+0xb8 (0x2554c0); heading ctrl+6 = int(atan2(vx, vz) * 10430.378) and bit 0 of ctrl+0xc are set. distance = sqrt(vx^2 + vz^2) (0x20e968; 0x25b578 is vsqrt). When vy > 0: frames = int((pi - asin(vy / (vy + height))) * 8.9126768) (0x1ff204 jumps to 0x2406c4, a fdlibm asinf: pio2_hi 0x3fc90000, pio2_lo 0x39fdaa22 and a sqrt; literals 0x40490fdb = pi and 0x410e9a53). ctrl+0x308 = distance / float(uint frames), ctrl+0x30c = max(vy, 0) + height. Then 0x2414b4(entity, ctrl+0x50, 72, 0) starts locomotion state 72 and 0x218b38(entity, ctrl, 11) requests state 11. Ported as `planSuperJump` (`src/runtime/ai_super_jump.*`); the formula is hand-evaluated for two cases in `ai-core` (the asin is the C library's, the original's polynomial may differ in the last bit, the result is truncated to an integer).

**State 11 arrival value.** ctrl+0x8c is written by the character set-up 0x236d74 at 0x236ed4 from `0x241280(entity, "ATTR_RunSpeed", 0, *0x37450c)`, default 0.23 ([player-locomotion.md](player-locomotion.md): the player's value read in the oracle). The -1.0 comes from constructors that run before (0x2309c8..0x2309e8 write one literal into +0x84..+0xa0). So state 11 ends when 0.23 (or the class value) is greater than the horizontal distance to the target: a jump lands within 0.23 of its target. Class overrides of `ATTR_RunSpeed` were not read for non-player characters.

**State 10, face target (0x2c5bc4).** Requested by 0x211110(entity, target) (stores 10 at ctrl+0x15, the target at +0xc4, bit 3 of +0xdb when the target is the player entity *0x3772b8, home position +0xb8 = node position). Each tick: node x and z are set to ctrl+0xb8 / ctrl+0xc0 (the home position) and the node is applied (0x24c118); when the target exists and bit 3 of +0xdb is set, +0xc4 is reset to the player entity; when a target with a node exists, heading ctrl+6 and ctrl+10 = int(atan2(dx, dz) * 10430.378) of target - node (0x23df44). Ported as `stepFaceTarget` (`src/runtime/ai_face_target.*`) and run by `AiCore::update`. The state never ends by itself.

**Set-up of the other natives.** AIRunAway 0x228c70(entity, -1.0, target 0x3772b8): requests state 25, sub-structure (ctrl+0xe0) +0xac = target, +0xa8 = -1.0. AIStalkPlayer 0x282b88(entity, distance, target 0x3772b8): requests 27, +0xc4 = target, bit 5 of +0xd9 = bit 4 of +0xdb (bits 3 and 5 cleared first), sub-structure +0xa4 = distance, then 0x1fc080(entity, target). AIPursuit 0x1d4688(entity, first node, takedown): clears bit 4 of byte +0xde of the entity's node flag word and of ctrl+0x16c, requests 35 (takedown non-zero) or 36, bit 5 of +0xd9 = bit 4 of +0xdb. State 2 (0x2c4708, requested by 0x21102c) tests bit 3 of a class table word, calls 0x206ad0, and goes idle (0x2246c0) when the target +0xc4 is invalid (0x245b00) or its ctrl+0x6c fails the tests (324 / 0x2107e0), else continues in 0x2bb558. `AiCore` records the requested state numbers 25 and 27 of the natives.

**Follow-up lookup 0x1f8d5c** (target to attack after a move, shared by the arrival sites 0x2c3c68 and 0x2c630c) is a target search over the player and entity lists (about 0x200 bytes; uses +0xd9 bit 0 and +0xc4); not transcribed. `AiCore` stands in "no target": idle.

Unknowns:
- The flight itself: locomotion state 72 (0x2414b4 -> animation 0x2eafc8) moves the character; no reader of ctrl+0x308 / +0x30c was identified (the candidates 0x2a4af4 and 0x33b01c read a different object). The unit of "frames" (30 Hz updates assumed) and the arc are stood in.
- Bit 6 of ctrl+0x314 (a character below the target counts as arrived) and who sets it; the value of component byte 0 (event argument).
- Update functions of states 25 (0x28b7a0), 27 (0x2c1c90), 35 / 36 (0x2ab6cc) and 2 (0x2bb558 and callees): they use the path follower 0x1faf04 / 0x1fae48 / 0x1faa6c with target and attack code; read only as far as listed above.
- Class values of `ATTR_RunSpeed` for characters other than the player.

<a id="ts-20261025-163712"></a>

## 2026-10-25T16:37:12+01:00 — The AISuperJump flight: locomotion states 72 and 73, the horizontal step 0x203d18, the end of the jump

[Corrects §2026-10-14T21:21:58](#ts-20261014-212158): the reader of ctrl+0x308 is the horizontal step 0x203d18 and the reader of ctrl+0x30c is the jump enter 0x200cc4; state 72 is only the start of the jump.

Method: llvm-objdump (`--triple=armv6k --mattr=+vfp2`) of `artifacts/game/game.arm.elf` and the words of `code.bin` (20261025-garage-C): the state description table at 0x357668..0x3579b0 (7 words per state: hook, id, update, enter, third function, exit, name), 0x26bca0, 0x28e4cc, 0x1b8560, 0x200cc4..0x200f4c, 0x203d18..0x2040a0, 0x20cd30 (calls only), the literals 0x204014..0x204020 and 0x26bce4.

**States.** 72 "SUPERJUMP START" (0x357864): no update, enter 0x26bca0. 73 "SUPERJUMP" (0x357880): update 0x28e4cc, enter 0x26bca0, third function 0x1b8560. 74 / 75 are "JUMPTOSAFETY START" / "JUMPTOSAFETY" with the same layout (enter 0x28d420, update 0x2acc4c). The enter 0x26bca0 requests state 73 (0x2414b4(entity, ctrl+0x50, 73, 0)), sets ctrl+0x2b8 = *0x3771b0, clears bit 6 of ctrl+0x314 (the landed bit that state 11 and 0x28e4cc test) and tail-calls 0x200cc4, the jump enter that the player's jump (state 4, 0x44) uses too.

**Jump enter 0x200cc4 for state 73.** It clears ctrl+0x130 / +0x134; for a pending state (ctrl+0x6e) of 73 or 75 it copies ctrl+0x30c (the climb of 0x213300) into ctrl+0x88, the jump height the player's jump takes from *0x3742a0 (2.1). It plays the role 4 animation (`jump_run`) when bit 0 of ctrl+0xc is set (AISuperJump sets it), else role 3 (`jump`), stores the node height in ctrl+0x2b0, sets the counter byte ctrl+0x2cf to 255 (-1), scales the clip so that the extent of its root height curve is ctrl+0x88 (0x200e70..0x200ecc, the rule of [player-locomotion.md](player-locomotion.md)), plays a sound of the character's sound row and sets ctrl+0x2b8 = *0x3771b0, ctrl+0x2b4 = 0.

**Horizontal step 0x203d18, current state 73 (branch 0x20402c).** Only with bit 0 of ctrl+0xc (move wanted): the facing ctrl+10 times 2 pi / 65536 (literal 0x38c90fdb) gives sin and cos (0x249038); x += (ctrl+0x308 x 30) x (dt x sin), z += (ctrl+0x308 x 30) x (dt x cos), dt from 0x23602c and 30 the literal 0x41f00000. So "frames" are updates at 30 per second, scaled by the real dt, and the step is not clamped at the target. State 75 (0x204074) adds ctrl+0x308 per update without dt. The step runs inside the character move 0x20cd30 with the collision of the character.

**Update 0x28e4cc of state 73.** 0x20cd30(entity, ctrl, 0, 0) moves the character; then, when no state change is pending (ctrl+0x6c == ctrl+0x6e): when the signed counter ctrl+0x2cf is 28 or more the jump becomes a fall (state 5) with ctrl+0x2b8 = -ctrl+0x208; else when bit 6 of ctrl+0x314 (landed) is set it plays the landing sound of the character's sound row and requests state 6 (landing). The counter is incremented only by 0x1b8560, the third function of the state records 0x44, 73 and 75; the state machine calls that slot once per update (oracle, [§2026-10-25T16:46:40](#ts-20261025-164640)).

**Port.** `stepSuperJumpHorizontal` (`src/runtime/ai_super_jump.*`) is the step of 0x203d18 and `AiCore::advanceSuperJump` uses it with the heading of state 11 as the facing (in the oracle the facing equalled the heading from the first move update). Stand-in SCRIPT-S103: the height follows a parabola, and a step longer than the distance left is cut at the target so that state 11 arrives in the next update (the original ends the flight by its landing); the jump-run clip of the minifig classes (`class_generic` and `class_police` `jump_run_minifig`, 12 frames at 15 per second, 0.733 s) is the curve the original scales to the climb. Test `ai-core` pins the step (hand evaluation).

Unknowns:
- The value of ctrl+0x208 at the end of the jump (the call rate of 0x1b8560 and the facing are in [§2026-10-25T16:46:40](#ts-20261025-164640)).
- Which animation class an AI actor plays (the girder guy is `chars/char_ai_actor`).

<a id="ts-20261025-164640"></a>

## 2026-10-25T16:46:40+01:00 — The girder guy's AISuperJump in the oracle

Method: servant request RPC 20261025-garage-C-1 (agent 20261025-garage-servant, 2026-10-25; New 3DS, OpenGL, RPC; recorded from a save state on the generator trigger spot of mission 1-3), stepped with `dt 0.0333333` (the game's dt 0.033), A pressed at u0; the guy `1-3_AI_Girder_Guy` (chars/char_ai_actor) traced per update with code traps on 0x1b2344 and 0x1b8560.

**Before the jump.** u0..u192 the guy stands on the girder (component C: C+0x15 = 1, C+0x6c = C+0x6e = 80, C+0x88 = 2.1, C+0x314 = 64) and goes down with the platform. AISuperJump at u193 (the set-up 0x213300 follows in the same update): position (947.881, 17.617, 1142.456), C+0x15 = 11, C+0x6c = C+0x6e = 73, C+0x88 = 2.0, C+0x2b8 = 0.8, C+0x2cf = -1, C+0x308 = 0.2415, C+0x30c = 2.0, C+0x314 = 0; C+6 and C+0xc are still 0 after that update and the node has not moved.

**Flight u194..u208.** C+6 = C+0xa = 44393, C+0xc = 1; x and z move 0.2146 and 0.105 per update (0.2415 x 30 x 0.033 along the heading 44393: the step of 0x203d18; positions in the CTest `ai-core`). y: 17.702, 17.872, 18.133, 18.397, 18.630, 18.862, 19.030, 19.194, 19.260, 19.318, 19.335, 19.347, 19.342, 19.334, 19.272, with C+0x2b4 the negative height change of the update. The counter C+0x2cf reads -1, 0, 1 .. 14 over u193..u208 and 15 at u209: 0x1b8560 runs once per update (trap: 23 calls, u194..u216, one per update).

**The script stops the jump.** The script's wait(0.5) after AISuperJump ends at u209 and AIFaceTarget(1-3_AI_Girder_Guy, Player1) requests state 10: C+0x15 = 10, C+0xc = 0, x and z stay at (944.661, 1140.876), 3.6 of the 6.76 to `1-3_girder_Jump_2`. The height goes on: 19.202, 19.020, 18.817, 18.499, 18.156, 17.795, 17.430, 17.016 (u209..u216, C+0x2b4 growing to 0.4145), landing at u217 at y 16.8266 with C+0x6c = C+0x6e = 6, C+0x314 = 64, C+0x2cf = 22; it stays there (u247 and u520 the same). The landing comes before the 28 counts, so the fall branch of 0x28e4cc was not reached here.

**Comparison with the clip.** The jump-run clip of the minifig classes (12 frames at 15 per second) scaled to the climb 2.0 rises 0.253, 0.257, 0.242, 0.166, 0.066, 0.012 per half frame; the trace rises 0.085, 0.170, then pairs close to those values one frame later, and its apex is 1.73 above the start instead of 1.99: the first frame is slower (a clip blend is likely; not read).

**Port.** The trace replay in `ai-core` checks the plan, the heading and the horizontal positions of u194..u208 (to 2e-3) and the hold of state 10; a 40-unit jump ends on the target with the AIMove event (the cut step of SCRIPT-S103). The height of the port stays the parabola of SCRIPT-S103; chase-game does not move level characters by their AI positions yet, so the flight is not shown.

Unknowns:
- The height model (the clip blend at the start, the move 0x20cd30 with the character's collision that lands the guy at y 16.83), and C+0x208, the fall speed after 28 counts.
