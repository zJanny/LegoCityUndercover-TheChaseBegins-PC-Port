# Animation and character rigs

## 1. Scope

Skeletons (`.bfnskl`), animations (`.bfnanm`), curves, pose composition, skinning, bone palettes, frame interpolation, playback rate and the extra root-motion curve. Who plays which animation is in [level-objects.md](level-objects.md) (NPCs) and [player-movement.md](player-movement.md) (player). Particles are [particles.md](particles.md).

## 2. Status

IN_PROGRESS, updated 2026-10-19T23:24:26+02:00. Player and level characters are posed and skinned natively; each level character has the original's four-slot animation player (modes, offsets, stop, finished test); its idle state plays idle / idle2 through it and the script natives PlayCharAnim, PlayAnimation, StopCharAnim, StopAnim and the WaitForCurrent natives drive it. Other NPC states and blending are open.

## 3. Verified behaviour

- Pose math replicates game routines 0x24c734, 0x248870, 0x24c5a0, 0x252cb0; bone palettes, frame interpolation and the frame count (u16 +4 & 0x3fff) recovered; one curve stores frames + 1.
- Playback rate comes from the joint table word and the instance; the player's rate and walk/run speeds are derived from code and data (no fitted constants).
- `chase-viewer` plays each animation at its stored rate (`animationFramesPerSecond`); `--fps` replaces it.
- Head attachment through `jointAttachment` (pinned test); joint orientation via the skeleton record's +0x88 quaternion.
- Slot player 0x245b40 / 0x24cbd4 / 0x22f560 / 0x225c5c / 0x22c2fc: length and loop bit of the clip, modes 0, 1, 2, 3, 5, 6, the extra wrap frame of looping clips without the loop bit, offsets, stop; the recorded slot of the van driver (mode 1, end 42, rate 15) is pinned (`animation-slots`).

## 4. Stand-ins

- ANIM-S2 The pose of a finished clip is clamped to `frames - 1` (the original's out-of-range sample rule was not read). evidence: [npc-animation.md §2026-10-15T09:29](../../re/npc-animation.md#ts-20261015-092900).
- ANIM-S10 One animation clock for all level characters, advanced by the update's dt (100 ticks per 1/30 s update) and running while a character is switched off or its room is not shown; the original pauses and resumes slots (0x24f8fc). needs: ANIM-10. evidence: [npc-animation.md §2026-10-15T18:33](../../re/npc-animation.md#ts-20261015-183306).
- ANIM-S11 Script animations of level characters are looked up in `models/char_minifig/<class>/`; the original picks `char_minifig`, `char_minifig_small` or `char_minifig_tall` from the character table (0x2270f8) or another folder when 0x22716c is false. needs: ANIM-11. evidence: [script-natives-animation.md §2026-10-15T18:33](../../re/script-natives-animation.md#ts-20261015-183336).
- ANIM-S12 The "already loaded" test of PlayCharAnim (0x21e8b4) knows a character's idle clips and its current script clip only. needs: SCRIPT-274 (script-system-natives-animation.md). evidence: [script-natives-animation.md §2026-10-15T18:33](../../re/script-natives-animation.md#ts-20261015-183336).
- ANIM-S13 The player's script clip (PlayCharAnim on Player1) is shown at full weight from the native's update instead of the measured 0.1 blend, and the player's own locomotion clips are not played through the slot player. needs: ANIM-13, ANIM-14, ANIM-3. evidence: [script-natives-animation.md §2026-10-19T23:24](../../re/script-natives-animation.md#ts-20261019-232426).

## 5. Open items

- [ ] ANIM-1 — Cutscene actors play on the camera's clock; whether an actor's own stored rate applies is not recovered, and whether any shipped animation stores rate 0 is not checked. evidence: [skeleton-animation.md §2026-10-14T13:39](../../re/skeleton-animation.md#ts-20261014-133953).
- [ ] ANIM-3 — Blending between different animations is not implemented. blocks: ANIM-14, ANIM-S13. evidence: [skeleton-animation.md §2026-10-12T22:10](../../re/skeleton-animation.md#ts-20261012-2210).
- [ ] ANIM-4 — The shared character rig code for animals and custom characters is missing (rigs of `char_pig`, `char_cat`, `char_dog`, `char_shark`, `char_Crow`). see LEVOBJ-4.
- [ ] ANIM-10 — Slot bookkeeping of the player: who pauses and resumes slots (0x24f8fc), the current slot choice 0x252878, the release 0x24cf70, the blend start 0x1ce608 and the zero blend of a first slot. blocks: ANIM-S10. evidence: [npc-animation.md §2026-10-15T18:33](../../re/npc-animation.md#ts-20261015-183306).
- [ ] ANIM-11 — Model folder of script animations: 0x22716c and 0x2270f8 for level characters (small and tall figures, the entity +0x40 folder). blocks: ANIM-S11. evidence: [script-natives-animation.md §2026-10-15T18:33](../../re/script-natives-animation.md#ts-20261015-183336).
- [ ] ANIM-13 — The player's locomotion clips (idle, walk, run, jumps) are chosen by the port's locomotion code, not started in the player's slots with the original's blend as the user-animation state's hand-back needs. blocks: ANIM-S13, SCRIPT-S262, SCRIPT-334 (script-system-natives-animation.md). evidence: [script-natives-animation.md §2026-10-19T12:55](../../re/script-natives-animation.md#ts-20261019-125503).
- [ ] ANIM-14 — The 0.1 crossfade of the player's script clip: the weights are measured (0.33, 0.66, 0.99, 1.0 from the native's update at dt 0.033), the pose blend exists (`blendPoses`), but the player rig keeps no outgoing clip and phase to sample as a second layer, and the script clip pauses at its last sample while the idle blends in. needs: ANIM-3. blocks: ANIM-S13. evidence: [script-natives-animation.md §2026-10-19T23:24](../../re/script-natives-animation.md#ts-20261019-232426).

## 6. Findings summary

- Containers, keyframes, pose composition, interpolation, bone palettes: [skeleton-animation.md §2026-10-12T18:40](../../re/skeleton-animation.md#ts-20261012-1840), [skeleton-animation.md §2026-10-12T20:15](../../re/skeleton-animation.md#ts-20261012-2015), [skeleton-animation.md §2026-10-12T21:30](../../re/skeleton-animation.md#ts-20261012-2130).
- Playback rate and the extra (root motion) curve: [skeleton-animation.md §2026-10-13T17:02](../../re/skeleton-animation.md#ts-20261013-170203).
- NPC idle state, slots, RNG: [npc-animation.md §2026-10-15T09:29](../../re/npc-animation.md#ts-20261015-092900).
- The slot player (type callbacks, start, position, kind, stop, offset, the van driver's slot): [npc-animation.md §2026-10-15T18:33](../../re/npc-animation.md#ts-20261015-183306).
- Script plays and the van driver in the mailbox: [script-natives-animation.md §2026-10-15T18:33](../../re/script-natives-animation.md#ts-20261015-183336).
- The player's script clips and the user-animation state 0x50: [script-natives-animation.md §2026-10-19T12:55](../../re/script-natives-animation.md#ts-20261019-125503); the measured blend weights of the script clip: [script-natives-animation.md §2026-10-19T23:24](../../re/script-natives-animation.md#ts-20261019-232426).
- Head attachment: [player-entity-input.md §2026-10-13T12:56](../../re/player-entity-input.md#ts-20261013-125622).

## 7. Code and tests

- `src/assets/anim.cpp`, `src/render/character.*`, `src/runtime/animation_slots.*`, `src/runtime/npc_idle.*`, `src/runtime/game_random.*`, `src/game_characters.*`, `src/runtime/player_script_animation.*`, C ABI `chase_assets_c` (`tools/chase_native.py`).
- CTest: `animation-slots`, `npc-idle`, `npc-idle-real-data`, `natives-animation-real-data`, `assets-core`, `assets-real-data`, `viewer-real-smoke`.
