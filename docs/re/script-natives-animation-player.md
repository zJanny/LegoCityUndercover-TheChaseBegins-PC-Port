# Script natives: animation, Player1 script clips (batch SCRIPT-32)

The entries of [script-natives-animation.md](script-natives-animation.md) about PlayCharAnim, StopCharAnim and the character wait on Player1 (user-animation state 0x50, root motion, the order of a state change, the first pose and the hand-back).

## Current state

- PlayCharAnim on Player1 puts the player into the user-animation state 0x50 (update 0x2a47b0 runs no locomotion and requests state 1 when the slot kind is 6; StopCharAnim ends the slot at once); the port poses the script clip through `runtime::PlayerScriptAnimation`. [§2026-10-19T12:55:03](#ts-20261019-125503)
- A state request is applied at the end of the update by 0x226100 (exit, enter), the new state's update starts in the next one; a request pending before the player's update skips the old state's update. PlayCharAnim (in the runner, before the player) enters 0x50 in its own update; the hand-back enter 0x1abbb0 starts a play-once idle at blend 0.1. [§2026-10-19T15:29:35](#ts-20261019-152935), [§2026-10-19T23:24:26](#ts-20261019-232426)
- The script clip's weight is 0.33, 0.66, 0.99, 1.0 from the native's update on (dt 0.033). [§2026-10-19T23:24:26](#ts-20261019-232426)
- The root motion of a script clip on Player1 moves the player: 0x215804 -> mover 0x20cd30 with flag 1 -> 0x2c9770 applies the clip's root delta (x, y, z, yaw) with the wall slide and the ground probe; the heading step runs only for AI-controlled characters; the enter 0x25fbec writes ctrl+0x2b8 = 0 and ctrl+0x315 bit 3; no gravity function runs in state 0x50. [§2026-10-19T13:43:01](#ts-20261019-134301)

<a id="ts-20261019-125503"></a>

## 2026-10-19T12:55:03+02:00 — PlayCharAnim, StopCharAnim and the character wait on Player1: the user-animation state 0x50 hands the pose to the script clip and back (llvm-objdump of `game.arm.elf`, data words from `code.bin`; no oracle run)

Corrected by [§2026-10-19T15:29:35](#ts-20261019-152935): the state functions run from the machine functions 0x2eaee0 and 0x226100, not from 0x2424d0, and a request is applied at the end of the update after the previous state's update, so the new state's update starts one update later.

Extends [§2026-10-15T18:33:36](script-natives-animation.md#ts-20261015-183336) and the state description of [npc-animation.md §2026-10-18T14:28:08+02:00](npc-animation.md#ts-20261018-142808).

**State table.** The words at 0x3577f4..0x357828 (read from `code.bin`) are {id, update, enter, 0, 0, name pointer, 0}: state 0x50 = {update 0x2a47b0, enter 0x25fbec}, 0x51 and 0x52 the same pair. A state request goes 0x2414b4 -> 0x2eafc8, which only stores the state object at [machine+0xc], sets the byte [machine+0x14] = 1 and tail-calls 0x1eb868 (copies the state ids of the stack entries into +0x1c / +0x1e). The enter and update functions run from the state runner 0x2424d0 in the controller update, so a request made by a script native takes effect in a later controller step, not inside the native.

**PlayCharAnim 0x29e5f0, tail after the play.** 0x295a8c (play), 0x241304(entity) (sets ctrl+0x15 = 1), then 0x2414b4(entity, ctrl+0x50, 80, 0); with the caller flag r2 non-zero (the cutscene call) the request is 81. Then 0x1b71e8 (cape lookup) and, when it finds one, 0x23afc0 and 0x1fb894 on the cape. Nothing in this path touches the locomotion state except the request.

**Enter 0x25fbec** (VFP code that llvm-objdump prints as `<unknown>`, decoded by hand): `ctrl+0x2b8 = 0.0`, `ctrl+0x315 |= 8`, return. The function after it (0x25fc08: `ctrl+0x24c = 0.0`, halfword `[r0+0x10] &= ~0x200`) belongs to another state.

**Update 0x2a47b0(entity, ctrl)** (disassembly): kind = 0x22f560(0x233900(entity + 0x40)); when 6, 0x2414b4(entity, ctrl+0x50, 1, 0); then 0x215804(entity, ctrl, flag = ctrl+0x6c is 80 or 82, 0) and `ctrl+10 = ctrl+8`. 0x215804 runs the root-motion mover 0x20cd30 after a heading step (0x23df04 / 0x23deb4) in one case (machine byte +0x14 is 10, and the entity is not `*0x37719c` or ctrl+0xd8 bit 4 is clear). No locomotion function runs in this state, so the stick, jumps and attacks do nothing while the clip plays.

**StopCharAnim 0x1bfb84:** 0x22dacc(object), 0x233900(entity + 0x40), 0x225c5c(slots), returns 1. 0x225c5c on a slot with an animation whose mode is not 6 pauses it (0x24f8fc) and sets mode 6, so the next update of state 0x50 sees kind 6 and requests state 1; locomotion runs again one controller step later. It does not request a state itself. The wait 0x263ae4 reads the same slot, so it ends in the update that sees kind 6 and, for a play-once clip, in the update the clip ends.

**Port.** `runtime::PlayerScriptAnimation` (`src/runtime/player_script_animation.*`, an `AnimationDriver` for `player1`): `play` uses the 0x295a8c slot rules and queues state 0x50; `update(dt)`, called first in the player update, applies the queue and then the 0x2a47b0 rule; while it returns true `PlayerRig::poseClip` poses the clip frame (clamped to the last sample) and the stick is treated as taken like the other control-takers; `stop` is 0x225c5c; `currentNearEnd` is the slot test, or nothing once locomotion has taken over (the wait then completes at once, as before). Tests `player-script-animation` (synthetic idle clip: 19 frames, 15 fps, loop bit; the numbers come from [npc-animation.md §2026-10-15T09:29:00](npc-animation.md#ts-20261015-092900)) and `player-script-animation-real-data` (the same sequence on `idle_minifig.bfnanm`). `chase-game --call-script 1-2_Cage_Key_Target_Script` runs without error; the animation was not compared with the original.

Unknowns:
- Whether the update that applies the request already runs 0x2a47b0 (the port does) and whether the previous state's update runs in the update that switches.
- 0x215804 / 0x20cd30 with the user-animation flag: whether the clip's root motion moves the player (the port applies none), the heading step, and what ctrl+0x315 bit 3 and ctrl+0x2b8 = 0 from the enter do.
- The cape animation 0x1b71e8 (whether the police character has one), 0x1fb894 / 0x2b9e2c after the start, ctrl+700 / ctrl+0x310 and PlayAnimation's message 253 (all SCRIPT-273).
- Whether gravity and the vertical state keep running in state 0x50 (the port keeps its vertical step), the blend of 0.1 between the script clip and the next locomotion clip, and the folder rule 0x22716c for the player (the port uses `models/char_minifig/<class>/`).

<a id="ts-20261019-134301"></a>

## 2026-10-19T13:43:01+02:00 — Root motion of a script clip on Player1: 0x215804, the mover 0x20cd30 with flag 1, 0x2c9770 and the heading step (REA Ghidra pseudocode and llvm-objdump of `game.arm.elf`, literals from `code.bin`; no oracle run)

Extends [§2026-10-19T12:55:03](#ts-20261019-125503).

**0x215804(entity, ctrl, flag, 0)** (state 0x50 passes flag = 1 for the states 80 / 82, from 0x2a47b0): (1) the heading step runs only when the machine byte ctrl+0x14 (the AI component state, [ai-natives.md](ai-natives.md)) is 10 (face target) and, for the player global at 0x3772c0 (the literal at 0x21589c points to it), ctrl+0xd8 bit 4 (AIEnable) is set: ctrl+8 = 0x23df04(6400, ctrl+8, ctrl+0xa), then 0x23deb4 turns the node to ctrl+8 (yaw = s16 * 9.58738e-5). The step limit 6400 is the word at 0x3598c4 (read from `code.bin`). 0x23df04 turns by the wrapped difference clamped to the limit. So for the player outside AI control (the script clip case) **no heading step runs**. (2) ctrl+0xc bit 0 (stick held) is cleared and ctrl+0xa = ctrl+8 for the call of the mover, then both are restored; 0x2a47b0 sets ctrl+0xa = ctrl+8 again afterwards. (3) The mover 0x20cd30(entity, ctrl, flag, 0) runs.

**Mover with flag 1 (0x20cd30).** Bit 0 of the flags clears entity+0xc bit 5; the delta starts as zero and the fall offset ctrl+0x2b8 is NOT subtracted from its y (it is when the flag is 0: locomotion). 0x2c9770 adds the root delta of the playing slot. The stick bit was cleared by 0x215804, so the stick locomotion 0x203d18 never runs. The delta then goes through the object gather, the wall slide 0x21a73c (skipped when ctrl+0x315 bits 3 and 2 are set, the ground bit ctrl+0x314 bit 6 is set and the delta is zero), the node move, and the vertical section: a delta y that is not rising runs the ground probe 0x20b48c with distance |delta| * sqrt(1 - 0.6088^2) - 0.1; a rising y runs the ceiling test 0x203c10, the ground probe, and on lift clears the ground bit and writes ctrl+0x2b8 = -y. At the end ctrl+0x315 |= 0x14. **Gravity: no separate gravity or fall-speed integration runs in state 0x50.** The only fall logic inside the state is the mover's ground probe (0x20b48c -> 0x203b34 starts the fall state when no ground is found); the port's vertical step therefore only approximates it in the air.

**Enter 0x25fbec:** ctrl+0x2b8 = 0 and ctrl+0x315 |= 8 (bit 3, with bit 2 set at the end of every mover run, makes the mover skip the wall slide while the character stands on the ground with a zero delta, see above).

**Root delta 0x2c9770(entity, ctrl, flags, &delta)** (flags bits 0x10 and 0x100 are clear here). Returns 0 when there is no slot or its kind (0x22f560) is 6. 0x2327b4 yields the weighted root delta (x, y, z, yaw) of the slot's clip (extra curve with the axis bits of the animation record, the same sampler 0x270778 as for props, [objects.md](objects.md)). `yawUsed` = the slot's yaw weight is non-zero and the record has the yaw bit. When the clip uses x or yaw (`yawUsed`, record bit 0 or the byte at +0x28): the node's rotation becomes yaw * old rotation, the translation delta goes through the node rotation (written with 0x232788 / 0x24c118), the delta passed on to the mover is the translation, and with `yawUsed` and a non-zero yaw 0x22f03c sets ctrl+8 = ctrl+0xa = trunc(atan2(node row 2 x, z) * 10430.378). Otherwise (z and y only, the locomotion clips) the delta is (0, y, z) turned by the node's rotation, or by ctrl+0xa when it differs from ctrl+8 (here equal). The delta of the first update of a slot is zero (0x2327b4 stores the sample).

**Port.** `PlayerScriptAnimation::rootMotion(facing)` and `applyScriptRootStep` (`src/runtime/player_script_animation.*`) reuse `rootSample` / `rootDelta` / `applyRootDelta` of the props (`src/runtime/prop_root_motion.hpp`): the world step (dx, dy, dz) and, for a clip that turns, the new facing from the exact node heading. The step goes through `playerRootStepUpdate` (wall slide, then the vertical step with the probe; a new `deltaY` argument adds the clip's y to the height first). The glue in `simulate` replaces the locomotion/attack step in an update that has a root step. Test `player-script-animation-real-data`: `combo_1_minifig` (15 frames, root z 1.05684, no x or yaw: archive scan with `rootSample`, manual inspection) moves along z only; `bluebell_interrogated_intro_minifig` (53 frames, yaw +1.57125 rad, z 10.63) ends at facing near 16389.

Unknowns:
- The axis bits of the player clips' animation records (the port applies all four; SCRIPT-S266) and whether the rotation precedes the translation in the rotating branch (0x258a10 / 0x2501ac / 0x23d39c are VFP code that llvm-objdump does not decode; the port takes the translation through the rotation before the turn as the props do).
- The rising-y branch of the mover (0x203c10, ground-leave flags, ctrl+0x2b8), the object gather and 0x21a73c with a clip's x component (SCRIPT-S267).
- Whether AI-enabled characters other than the player (AIEnable, state 10) turn with the heading step during a script clip.

<a id="ts-20261019-152935"></a>

## 2026-10-19T15:29:35+02:00 — Order of a state change inside the player's update, and what state 1 does on the hand-back (llvm-objdump of `game.arm.elf`, REA Ghidra pseudocode of 0x2424d0, ctrace oracle run from `gameplay-start`)

Corrected by [§2026-10-19T23:24:26](#ts-20261019-232426): PlayCharAnim's request is applied in the update of the native and that update runs no state update (0x26a19c skips it while a request is pending); the script runner runs before the player's update.

Extends [§2026-10-19T12:55:03](#ts-20261019-125503) and [§2026-10-19T13:43:01](#ts-20261019-134301). [Corrects §2026-10-19T12:55:03](#ts-20261019-125503): the state functions do not run from 0x2424d0 (it calls a pair of class callbacks), and the update that applies a request does not run the new state's update.

**Two machine functions on `ctrl+0x50` (the state machine).**
- Request 0x2414b4 -> 0x2eafc8 only stores the state object in the pending slot (+0xc), sets +0x14 = 1 and refreshes the ids +0x1c (current) and +0x1e (pending) through 0x1eb868.
- **0x2eaee0(machine, entity, dt)** (events): the top state object r5; when its flag 0x8000 is set and the slot kind (0x22f560) is 6, broadcast type 1 (clip finished) to its children (0x1eb8c4); the timer at +0x18 counts down and gives type 2; when the flag 0x4000 is set, type 3 (update). The type-3 child of the table states is 0x31e6e8, which calls `table[machine+0x1c].word0` (+0 of the 0x18-byte record at `*0x3742c8`), for state 0x50 the update 0x2a47b0. +0x1c is the CURRENT state, so the previous state's update runs while a request is pending.
- **0x226100(machine, entity, dt)** (apply): when the current and the pending object differ it clears the timer, broadcasts type 5 to the current one and calls its exit (vtable +0x10, record +0xc), calls the enter of the pending one (vtable +8, record +4: 0x25fbec for state 0x50, 0x1abbb0 for state 1), broadcasts type 4, copies the pending slot to the current one, then tail-calls vtable +0xc (record +8, none for state 0x50). The same object requested again runs no exit or enter.
- Class update 0x190174 (NPC class): input step (0x29bf30 / 0x277980), 0x2779cc (prelude 0x200234 and 0x2eaee0), 0x226210, the AI component 0x21d974, then 0x1c7520 = dt and 0x226100. The player (`*0x3772b8`) is another class whose update was not read; its state updates are reached through the same dispatcher (lr 0x31e718) and its enter calls come from 0x226100 (lr 0x2261a4).

**Measured (ctrace, `tools/oracle/state-order-ctrace.txt`, `gameplay-start`, dt 0.033, one B jump; raw trace not kept).** For the player entity, in one scheduler update: the state 4 update 0x1d5254 (frame 57) is followed in the SAME update by the enter 0x1abc5c of state 6; the next update (58) runs the state 6 update 0x1d55cc, and so on until frame 84, where the state 6 update is followed in the same update by the enter 0x1abbb0 of state 1; the state 1 update 0x1d5070 runs from frame 85. So a request made by a state's update is applied at the end of the same update (previous state's update, then enter), and the new state's update starts in the next one. This agrees with the fall trace of [player-traversal-rails.md](player-traversal-rails.md) (no 0x25f03c stop in the update that enters state 5); the row notes there are then sampled after the apply.

**State 0x50 on the player.** PlayCharAnim's request is applied at the end of the first player update after it: that update still runs the previous state's update (the locomotion with the stick); the enter 0x25fbec follows (ctrl+0x2b8 = 0, ctrl+0x315 bit 3, [§2026-10-19T12:55:03](#ts-20261019-125503)); the first 0x2a47b0 is the next update (root motion zero in it, the sample is stored). When the slot kind is 6 the 0x2a47b0 of update k requests state 1 and it is applied at the end of k: the enter **0x1abbb0** sets ctrl+0x134 = ctrl+0x130 = 0, picks the idle role with 0x214870, starts it with 0x21941c at blend **0.1** (0 for the states 205 and 206), speed 1.0, and writes ctrl+0x2b4 = ctrl+0x2b8 = 0; the state 1 update with the stick runs in k + 1. StopCharAnim (0x1bfb84 -> 0x225c5c) only pauses the slot (kind 6), so the same path follows in the next update. A repeated PlayCharAnim while in state 0x50 requests the same object: no enter.

**Port.** `PlayerScriptAnimation::update` runs the current state's update (state 0x50: the kind test), then applies the pending request and reports it through `enteredState()`; `applyScriptStateEnter` writes the vertical resets of the two enter functions; `posesScriptClip()` poses the script clip from the update that applies the request on (SCRIPT-S272). Tests `player-script-animation`, `player-script-animation-real-data`.

**The 0.1 blend (ANIM-S13).** Both ends of the script clip start with blend 0.1: 0x295a8c starts the clip with 0.1 ([§2026-10-15T18:33:36](script-natives-animation.md#ts-20261015-183336)) and 0x1abbb0 starts the idle with 0.1. The weights are those of 0x115ecc in [npc-animation.md §2026-10-15T18:33:06](npc-animation.md#ts-20261015-183306) (newest slot min(elapsed / (0.1 * 3000 ticks), 1), older slots the rest): three updates at dt 1/30. The pose side exists (`blendPoses`, 0x252cb0 with several layers); not done: the player rig keeps no outgoing clip and phase to sample as a second layer (ANIM-14).

Unknowns:
- The timing of a request made by a script native relative to the player's update in the scheduler (the port applies it at the end of the next player update) and the old state's update in that update: whether state 1's idle code restarts a locomotion clip that replaces the script clip in the slot (SCRIPT-333).
- The exit hook and the type-5 / type-4 broadcasts of the machine for states 1 and 0x50 (children of the state objects), and the third function (record +8).
- Which function updates the player class (the order input, update, apply was read for the NPC class 0x190174 and measured for the player's state calls).

<a id="ts-20261019-232426"></a>

## 2026-10-19T23:24:26+02:00 — PlayCharAnim on Player1: request timing, first pose and hand-back measured (Azahar oracle the save state at the dog cage trigger of mission 1-2 with RPC, `ctrace` and `trap`; llvm-objdump of `game.arm.elf`)

[Corrects §2026-10-19T15:29:35](#ts-20261019-152935): the update that applies PlayCharAnim's request runs no state update. The order of the runner and the player's update and the skip rule of 0x26a19c are in [scripts-runtime.md §2026-10-19T23:24:26](scripts-runtime.md#ts-20261019-232426).

- **Timing** (dt 0.033 = 99 ticks): PlayCharAnim(Player1, `dog_cage_open_minifig`: 44 frames, 15 fps, loop bit) and the enter 0x25fbec in update 9; 0x2a47b0 in updates 10 to 98; kind 6 and the enter 0x1abbb0 in update 98. 8800 ticks reach frame 44: the state step of update u sees (u - 9) * 99 ticks (update 97: 8712, update 98: 8811), so it runs on the clock the runner saw.
- **First pose** (`tools/oracle/script-anim-blend-trap.txt`, trap at 0x253480 after the weight call 0x115ecc of the pose 0x2533e0, stack word 4 = weight; the player's animation player 0x08568d10): newest slot weight 1.0 in update 8, then 0.33 in update 9 (the script clip, older slot 0.67), 0.66, 0.99 and 1.0 from update 12. The pose of the native's own update shows the clip 99 ticks in, so the clock advances between the state step and the pose; the weights are 0x115ecc with blend 0.1 (300 ticks).
- **State 1 on the hand-back:** 0x1abbb0 starts role 0 through 0x21941c with mode 0 (r2 = r3 = 0 at 0x1abc20), a play-once idle; the IDLE update 0x1d5070 starts the next idle only when the kind is 6. In the update that applies the native's request no old state update runs, so nothing replaces the script clip.
- **Exit and record +8:** the IDLE record at 0x357554 is {update 0x1d5070, enter 0x1abbb0, 0x1b855c, 0x19d48c}; 0x1b855c and 0x19d48c are `bx lr` (word 0xe12fff1e). State 0x50 has neither.
- **Missing clip:** 0x29e5f0 requests 80 (81 with r6 set) after the play 0x295a8c without testing its result (0x29e6e0..0x29e724), so a clip that is not found still enters state 0x50.

**Port.** `PlayerScriptAnimation::update` returns `PlayerStateStep`: `Skipped` while a request is pending (no locomotion and no vertical step in `LevelSession::simulate`), `UserAnimation` for 0x2a47b0, `Locomotion` otherwise; the state step and the root sample use the clock before the update's advance, the pose the advanced clock. The level runner runs before the player. CTests `player-script-animation` (the `cageOrder` replay: enter in the play's update, hand-back after 89 updates), `game-player-script-animation` (`chase-game`: thread started in update 5, state 80 entered in 6, state 1 in 95).

Unknowns:
- The type-5 and type-4 broadcasts of 0x226100 to the children of the state objects of states 1 and 0x50 (SCRIPT-344).
- Which function advances the player's animation clock between the state step and the pose.
- `WaitForCurrentCharAnim` after the hand-back: the play-once idle of 0x1abbb0 is the newest slot; whether the wait reads it depends on the current slot choice 0x252878 (SCRIPT-334).

