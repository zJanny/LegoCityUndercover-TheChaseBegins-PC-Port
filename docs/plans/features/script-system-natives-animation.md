# Script system: animation and attach natives

## 1. Scope

The 13 natives of batch 32 (SCRIPT-32): PlayCharAnim, PlayAnimation, LoadAnimation, UnloadAnimation, StopCharAnim, StopAnim, WaitForCurrentAnim, WaitForCurrentCharAnim, AttachObject, AttachToBone, AttachToBoneRot, AttachToWorldRoot, AttachToRoom. The animation player they drive is in [animation-characters.md](animation-characters.md); the other natives are in [script-system-natives.md](script-system-natives.md).

## 2. Status

IN_PROGRESS, updated 2026-10-19T15:29:35+02:00. For level characters and for skinned placed props (garage shutters, dog cage, dumpster, crane, lawnmowers, gardeners, the dog and the crow) the animation natives play through the original's slot player: PlayCharAnim and PlayAnimation start clips, StopCharAnim / StopAnim stop them, the two waits block until the clip is within one frame of its length. On Player1 the three character natives hand the pose to the script clip through the user-animation state 0x50 and back to locomotion (the clip's root motion moves the player; a state request is applied at the end of the next update, the new state's update starts one update later, as measured); the attach natives only record the new parent.

## 3. Verified behaviour

- `1-1_VanBreakdown` plays `postboxstuck_idle_minifig` in a loop on `1-1_AI_DonutLady`, as the original's slot (mode 1, end 42, rate 15); the van driver is stuck in the mailbox (ANIM-7).
- PlayAnimation skips an animation that is not loaded; the mode is the low byte of argument 3; `AnimationOffset` moves the start.
- AttachObject and the bone natives attach the second object to the first (the lawnmower carries the gardener).
- A placed prop that is an animation target is skinned on its own: `open` of `Prop_Garage_shutter` (60 frames, 15 fps) makes `WaitForCurrentAnim` block for 3.9 s; CTest `prop-animation-real-data`.

- PlayCharAnim on Player1 plays the clip in the player's slots, the previous state's update (locomotion) runs in the update that applies the request, the clip owns the movement and the pose from the next update until the slot kind is 6; locomotion resumes one update after the update that saw kind 6, the enter functions clear the fall offset; StopCharAnim ends it at once; `WaitForCurrentCharAnim` blocks as for level characters (CTest `player-script-animation`, `player-script-animation-real-data`). Order measured with `tools/oracle/state-order-ctrace.txt`: [script-natives-animation.md §2026-10-19T15:29](../../re/script-natives-animation.md#ts-20261019-152935).

## 4. Stand-ins

- SCRIPT-S110 The batch 32 natives on objects the port does not animate only record the request; their waits complete at once; the five Attach natives record the new parent, bones and rotation and do not move anything. Census rows count all 13 as stand-ins. needs: SCRIPT-272, SCRIPT-273, SCRIPT-274. evidence: [script-natives-animation.md §2026-10-15T18:33](../../re/script-natives-animation.md#ts-20261015-183336).
- SCRIPT-S262 The state change of PlayCharAnim on Player1 is applied at the end of the next player update after the previous state's update, like the measured requests of state updates (4 -> 6 -> 1); a request from a script native was not measured, the state itself is modelled by `PlayerScriptAnimation`, not by the player's state machine, locomotion is suppressed by taking the stick (jumps, vertical step and attack code still run), and a clip that is not found does not enter the state. needs: SCRIPT-333. evidence: [script-natives-animation.md §2026-10-19T15:29](../../re/script-natives-animation.md#ts-20261019-152935).
- SCRIPT-S263 `WaitForCurrentCharAnim` on Player1 completes at once whenever the script clip is not the one the player is in or about to enter; after the hand-back the original would test the slot of the locomotion clips. needs: SCRIPT-334. evidence: [script-natives-animation.md §2026-10-19T12:55](../../re/script-natives-animation.md#ts-20261019-125503).
- SCRIPT-S266 All four root axes (x, y, z, yaw) of the extra curve are applied to every script clip on Player1 and the translation goes through the node rotation before the turn, as for props (LEVOBJ-S82); the facing is the exact node heading truncated like 0x22f03c. needs: SCRIPT-324. evidence: [script-natives-animation.md §2026-10-19T13:43](../../re/script-natives-animation.md#ts-20261019-134301).
- SCRIPT-S267 The clip's y is added to the height before the vertical step instead of the mover's rising-y branch, and the wall slide runs for every update with a delta. needs: SCRIPT-324, SCRIPT-325. evidence: [script-natives-animation.md §2026-10-19T13:43](../../re/script-natives-animation.md#ts-20261019-134301).
- SCRIPT-S272 In the update that applies the PlayCharAnim request the old state's update runs as the port's locomotion (the stick is processed) while the script clip is already posed, because the slot holds it from the play on; whether the old state's idle code restarts a locomotion clip in that update is not known. needs: SCRIPT-333. evidence: [script-natives-animation.md §2026-10-19T15:29](../../re/script-natives-animation.md#ts-20261019-152935).

## 5. Open items

- [ ] SCRIPT-272 — The node changes of the Attach natives are not applied. The port moves no parent object, so only the bone attaches on characters (`GrappleGun`, the booms and cameras of 1-3) would show; 0x20f33c builds the child's local matrix from the argument matrix (0x258858, 0x23d39c) and links it under the parent's bone node (0x22690c), not transcribed. Also keep the world pose on re-parenting (0x245008) once a parent moves (platforms, needs a platform feature). blocks: SCRIPT-S110. evidence: [script-natives-animation.md §2026-10-15T20:04](../../re/script-natives-animation.md#ts-20261015-200440).
- [ ] SCRIPT-273 — PlayCharAnim's state request 81 (the cutscene call, state 0x51 has the same functions) and its cape animation (0x1b71e8), PlayAnimation's class message 253, the calls 0x1fb894 / 0x2b9e2c after the start and the writes ctrl+700 / ctrl+0x310 are not ported. blocks: SCRIPT-S110. evidence: [script-natives-animation.md §2026-10-15T18:33](../../re/script-natives-animation.md#ts-20261015-183336).
- [ ] SCRIPT-333 — A request made by a script native (PlayCharAnim) was not traced against the scheduler order: when the script thread runs relative to the player's update, and whether the old state's update (state 1's idle code) restarts a locomotion clip in the applying update; the exit hook and the type 4 / 5 broadcasts of the machine are not read. blocks: SCRIPT-S262, SCRIPT-S272. evidence: [script-natives-animation.md §2026-10-19T15:29](../../re/script-natives-animation.md#ts-20261019-152935).
- [ ] SCRIPT-334 — The wait on the player's locomotion slots after the hand-back: the idle started by the enter 0x1abbb0 (blend 0.1) is in the slots, and the original's wait would test it. blocks: SCRIPT-S263. evidence: [script-natives-animation.md §2026-10-19T15:29](../../re/script-natives-animation.md#ts-20261019-152935).
- [ ] SCRIPT-324 — The axis bits of the animation record of the clips a script plays on the player (which of x, y, z, yaw 0x2327b4 applies), the order of rotation and translation in the turning branch of 0x2c9770 (VFP code 0x258a10 / 0x2501ac / 0x23d39c) and the rising-y branch of the mover (0x203c10, ground-leave, ctrl+0x2b8) are not measured; an oracle run of a clip with a turn (`bluebell_interrogated_intro_minifig`, `collect_token_minifig`) would settle them. blocks: SCRIPT-S266, SCRIPT-S267. evidence: [script-natives-animation.md §2026-10-19T13:43](../../re/script-natives-animation.md#ts-20261019-134301).
- [ ] SCRIPT-325 — The mover's skip of the wall slide while the player stands on the ground with a zero delta (ctrl+0x315 bits 3 and 2, set by the enter 0x25fbec), the fall state started by the ground probe (0x20b48c -> 0x203b34) when a clip leaves the ground, and the heading step of AI-enabled characters during a script clip are not ported. needs: SCRIPT-324. blocks: SCRIPT-S267. evidence: [script-natives-animation.md §2026-10-19T13:43](../../re/script-natives-animation.md#ts-20261019-134301).
- [ ] SCRIPT-274 — LoadAnimation and UnloadAnimation keep a name list only (0x23e6e4 / 0x23e518 load and release are not ported) and the animations a class loads by itself are unknown, so PlayAnimation of `cower_minifig` on `1-4_Booster_Strange` does nothing. blocks: SCRIPT-S110, ANIM-S12 (animation-characters.md). evidence: [script-natives-animation.md §2026-10-15T18:33](../../re/script-natives-animation.md#ts-20261015-183336).

## 6. Findings summary

- Batch 32 handlers: [script-natives-animation.md §2026-10-14T21:36](../../re/script-natives-animation.md#ts-20261014-213649).
- Play rules, waits, attach direction and the van driver: [script-natives-animation.md §2026-10-15T18:33](../../re/script-natives-animation.md#ts-20261015-183336).
- Skinned prop clips, waits and the Attach node changes: [script-natives-animation.md §2026-10-15T20:04](../../re/script-natives-animation.md#ts-20261015-200440).
- The slot player: [npc-animation.md §2026-10-15T18:33](../../re/npc-animation.md#ts-20261015-183306).
- PlayCharAnim / StopCharAnim / wait on Player1, state 0x50: [script-natives-animation.md §2026-10-19T12:55](../../re/script-natives-animation.md#ts-20261019-125503).
- Root motion of a script clip on Player1 (0x215804, 0x20cd30, 0x2c9770): [script-natives-animation.md §2026-10-19T13:43](../../re/script-natives-animation.md#ts-20261019-134301).
- State change order (0x2eaee0, 0x226100), the enter 0x1abbb0 and the 0.1 blend: [script-natives-animation.md §2026-10-19T15:29](../../re/script-natives-animation.md#ts-20261019-152935).
- Census rows: [script-natives-census.md](../../re/script-natives-census.md).

## 7. Code and tests

- `src/runtime/natives_animation.*` (`AnimationNativeState`, `AnimationDriver`, `playCharacterAnimation`, `playObjectAnimation`), `ScriptRunner::setAnimationDriver` / `loadAnimationOffsets`, `game_characters::LevelCharacters` (the driver for level characters), `src/runtime/prop_animation.*` (`PropAnimations`, the driver for skinned placed props), `src/runtime/animation_driver_chain.hpp`, `src/runtime/player_script_animation.*` (`PlayerScriptAnimation`, the driver for Player1; `PlayerRig::poseClip` poses its clip).
- Oracle scenario `tools/oracle/state-order-ctrace.txt`. CTest: `natives-animation`, `natives-animation-real-data`, `prop-animation-real-data`, `animation-slots`, `player-script-animation`, `player-script-animation-real-data`.
