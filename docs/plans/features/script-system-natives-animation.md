# Script system: animation and attach natives

## 1. Scope

The 13 natives of batch 32 (SCRIPT-32): PlayCharAnim, PlayAnimation, LoadAnimation, UnloadAnimation, StopCharAnim, StopAnim, WaitForCurrentAnim, WaitForCurrentCharAnim, AttachObject, AttachToBone, AttachToBoneRot, AttachToWorldRoot, AttachToRoom. The animation player they drive is in [animation-characters.md](animation-characters.md); the other natives are in [script-system-natives.md](script-system-natives.md).

## 2. Status

IN_PROGRESS, updated 2026-10-19T12:55:03+02:00. For level characters and for skinned placed props (garage shutters, dog cage, dumpster, crane, lawnmowers, gardeners, the dog and the crow) the animation natives play through the original's slot player: PlayCharAnim and PlayAnimation start clips, StopCharAnim / StopAnim stop them, the two waits block until the clip is within one frame of its length. On Player1 the three character natives hand the pose to the script clip through the user-animation state 0x50 and back to locomotion (no root motion yet); the attach natives only record the new parent.

## 3. Verified behaviour

- `1-1_VanBreakdown` plays `postboxstuck_idle_minifig` in a loop on `1-1_AI_DonutLady`, as the original's slot (mode 1, end 42, rate 15); the van driver is stuck in the mailbox (ANIM-7).
- PlayAnimation skips an animation that is not loaded; the mode is the low byte of argument 3; `AnimationOffset` moves the start.
- AttachObject and the bone natives attach the second object to the first (the lawnmower carries the gardener).
- A placed prop that is an animation target is skinned on its own: `open` of `Prop_Garage_shutter` (60 frames, 15 fps) makes `WaitForCurrentAnim` block for 3.9 s; CTest `prop-animation-real-data`.

- PlayCharAnim on Player1 plays the clip in the player's slots, the player stands still (stick taken) and the clip owns the pose until the slot kind is 6; locomotion resumes one update later; StopCharAnim ends it at once; `WaitForCurrentCharAnim` blocks as for level characters (CTest `player-script-animation`, `player-script-animation-real-data`).

## 4. Stand-ins

- SCRIPT-S110 The batch 32 natives on objects the port does not animate only record the request; their waits complete at once; the five Attach natives record the new parent, bones and rotation and do not move anything. Census rows count all 13 as stand-ins. needs: SCRIPT-272, SCRIPT-273, SCRIPT-274. evidence: [script-natives-animation.md §2026-10-15T18:33](../../re/script-natives-animation.md#ts-20261015-183336).
- SCRIPT-S262 The state change of PlayCharAnim on Player1 is applied at the start of the next player update and that update already runs the state's update 0x2a47b0; the state itself is modelled by `PlayerScriptAnimation`, not by the player's state machine, locomotion is suppressed by taking the stick (jumps, vertical step and attack code still run), and a clip that is not found does not enter the state. needs: SCRIPT-320. evidence: [script-natives-animation.md §2026-10-19T12:55](../../re/script-natives-animation.md#ts-20261019-125503).
- SCRIPT-S263 `WaitForCurrentCharAnim` on Player1 completes at once whenever the script clip is not the one the player is in or about to enter; after the hand-back the original would test the slot of the locomotion clips. needs: SCRIPT-320. evidence: [script-natives-animation.md §2026-10-19T12:55](../../re/script-natives-animation.md#ts-20261019-125503).

## 5. Open items

- [ ] SCRIPT-272 — The node changes of the Attach natives are not applied. The port moves no parent object, so only the bone attaches on characters (`GrappleGun`, the booms and cameras of 1-3) would show; 0x20f33c builds the child's local matrix from the argument matrix (0x258858, 0x23d39c) and links it under the parent's bone node (0x22690c), not transcribed. Also keep the world pose on re-parenting (0x245008) once a parent moves (platforms, needs a platform feature). blocks: SCRIPT-S110. evidence: [script-natives-animation.md §2026-10-15T20:04](../../re/script-natives-animation.md#ts-20261015-200440).
- [ ] SCRIPT-273 — PlayCharAnim's state request 81 (the cutscene call, state 0x51 has the same functions) and its cape animation (0x1b71e8), PlayAnimation's class message 253, the calls 0x1fb894 / 0x2b9e2c after the start and the writes ctrl+700 / ctrl+0x310 are not ported. blocks: SCRIPT-S110. evidence: [script-natives-animation.md §2026-10-15T18:33](../../re/script-natives-animation.md#ts-20261015-183336).
- [ ] SCRIPT-319 — The user-animation state's root motion (0x215804 -> 0x20cd30 with the flag for states 80 / 82, heading step 0x23df04) and the enter 0x25fbec (ctrl+0x2b8 = 0, ctrl+0x315 bit 3) are not transcribed, so a script clip on Player1 never moves the player. blocks: SCRIPT-S110. evidence: [script-natives-animation.md §2026-10-19T12:55](../../re/script-natives-animation.md#ts-20261019-125503).
- [ ] SCRIPT-320 — The order of the state switch inside the controller step (does the update that applies a request already run 0x2a47b0, does the previous state's update run, does gravity run in state 0x50) and the wait on the player's locomotion slots after the hand-back are not measured. blocks: SCRIPT-S262, SCRIPT-S263. evidence: [script-natives-animation.md §2026-10-19T12:55](../../re/script-natives-animation.md#ts-20261019-125503).
- [ ] SCRIPT-274 — LoadAnimation and UnloadAnimation keep a name list only (0x23e6e4 / 0x23e518 load and release are not ported) and the animations a class loads by itself are unknown, so PlayAnimation of `cower_minifig` on `1-4_Booster_Strange` does nothing. blocks: SCRIPT-S110, ANIM-S12 (animation-characters.md). evidence: [script-natives-animation.md §2026-10-15T18:33](../../re/script-natives-animation.md#ts-20261015-183336).

## 6. Findings summary

- Batch 32 handlers: [script-natives-animation.md §2026-10-14T21:36](../../re/script-natives-animation.md#ts-20261014-213649).
- Play rules, waits, attach direction and the van driver: [script-natives-animation.md §2026-10-15T18:33](../../re/script-natives-animation.md#ts-20261015-183336).
- Skinned prop clips, waits and the Attach node changes: [script-natives-animation.md §2026-10-15T20:04](../../re/script-natives-animation.md#ts-20261015-200440).
- The slot player: [npc-animation.md §2026-10-15T18:33](../../re/npc-animation.md#ts-20261015-183306).
- PlayCharAnim / StopCharAnim / wait on Player1, state 0x50: [script-natives-animation.md §2026-10-19T12:55](../../re/script-natives-animation.md#ts-20261019-125503).
- Census rows: [script-natives-census.md](../../re/script-natives-census.md).

## 7. Code and tests

- `src/runtime/natives_animation.*` (`AnimationNativeState`, `AnimationDriver`, `playCharacterAnimation`, `playObjectAnimation`), `ScriptRunner::setAnimationDriver` / `loadAnimationOffsets`, `game_characters::LevelCharacters` (the driver for level characters), `src/runtime/prop_animation.*` (`PropAnimations`, the driver for skinned placed props), `src/runtime/animation_driver_chain.hpp`, `src/runtime/player_script_animation.*` (`PlayerScriptAnimation`, the driver for Player1; `PlayerRig::poseClip` poses its clip).
- CTest: `natives-animation`, `natives-animation-real-data`, `prop-animation-real-data`, `animation-slots`, `player-script-animation`, `player-script-animation-real-data`.
