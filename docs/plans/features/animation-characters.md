# Animation and character rigs

## 1. Scope

Skeletons (`.bfnskl`), animations (`.bfnanm`), curves, pose composition, skinning, bone palettes, frame interpolation, playback rate and the extra root-motion curve. Who plays which animation is in [level-objects.md](level-objects.md) (NPCs) and [player-movement.md](player-movement.md) (player). Particles are [particles.md](particles.md).

## 2. Status

IN_PROGRESS, updated 2026-10-15T09:35:00+0200. Player and level characters are posed and skinned natively; level characters play the idle state's animations (idle / idle2, once each at the stored rate); other NPC states and blending are open.

## 3. Verified behaviour

- Pose math replicates game routines 0x24c734, 0x248870, 0x24c5a0, 0x252cb0; bone palettes, frame interpolation and the frame count (u16 +4 & 0x3fff) recovered; one curve stores frames + 1.
- Playback rate comes from the joint table word and the instance; the player's rate and walk/run speeds are derived from code and data (no fitted constants).
- `chase-viewer` plays each animation at its stored rate (`animationFramesPerSecond`); `--fps` replaces it.
- Head attachment through `jointAttachment` (pinned test); joint orientation via the skeleton record's +0x88 quaternion.

## 4. Stand-ins

- ANIM-S2 The pose of a finished clip is clamped to `frames - 1` (the original's out-of-range sample rule was not read). evidence: [npc-animation.md §2026-10-15T09:29](../../re/npc-animation.md#ts-20261015-092900).

## 5. Open items

- [ ] ANIM-1 — Cutscene actors play on the camera's clock; whether an actor's own stored rate applies is not recovered, and whether any shipped animation stores rate 0 is not checked. evidence: [skeleton-animation.md §2026-10-14T13:39](../../re/skeleton-animation.md#ts-20261014-133953).
- [ ] ANIM-5 — The general animation slot player (4 slots, modes 1 / 6 loop, 2, 3, 5, start offsets, finished flag, `WaitForCurrent` lengths, `PlayAnimation` offsets) is only implemented for the idle state. blocks: SCRIPT-112, SCRIPT-32. evidence: [npc-animation.md §2026-10-15T09:29](../../re/npc-animation.md#ts-20261015-092900).
- [ ] ANIM-3 — Blending between different animations is not implemented. evidence: [skeleton-animation.md §2026-10-12T22:10](../../re/skeleton-animation.md#ts-20261012-2210).
- [ ] ANIM-4 — The shared character rig code for animals and custom characters is missing (rigs of `char_pig`, `char_cat`, `char_dog`, `char_shark`, `char_Crow`). see LEVOBJ-4.
- [ ] ANIM-7 — (User report 2026-10-15) At the broken-down donut van the original shows the van driver (`1-1_AI_DonutLady`) stuck inside a mailbox; the port shows the driver standing in front of it. Likely the PlayCharAnim of `1-1_VanBreakdown` / `1-1_Found_Van`, which the port does not play. needs: SCRIPT-112. evidence: [script-natives-animation.md §2026-10-15T17:50](../../re/script-natives-animation.md#ts-20261015-175022)

## 6. Findings summary

- Containers, keyframes, pose composition, interpolation, bone palettes: [skeleton-animation.md §2026-10-12T18:40](../../re/skeleton-animation.md#ts-20261012-1840), [skeleton-animation.md §2026-10-12T20:15](../../re/skeleton-animation.md#ts-20261012-2015), [skeleton-animation.md §2026-10-12T21:30](../../re/skeleton-animation.md#ts-20261012-2130).
- Playback rate and the extra (root motion) curve: [skeleton-animation.md §2026-10-13T17:02](../../re/skeleton-animation.md#ts-20261013-170203).
- NPC idle state, slots, RNG: [npc-animation.md §2026-10-15T09:29](../../re/npc-animation.md#ts-20261015-092900).
- Head attachment: [player-entity-input.md §2026-10-13T12:56](../../re/player-entity-input.md#ts-20261013-125622).

## 7. Code and tests

- `src/assets/anim.cpp`, `src/render/character.*`, `src/runtime/npc_idle.*`, `src/runtime/game_random.*`, `src/game_characters.*`, C ABI `chase_assets_c` (`tools/chase_native.py`).
- CTest: `npc-idle`, `npc-idle-real-data`, `assets-core`, `assets-real-data`, `viewer-real-smoke`.
