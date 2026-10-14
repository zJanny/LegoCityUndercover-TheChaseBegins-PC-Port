# Animation and character rigs

## 1. Scope

Skeletons (`.bfnskl`), animations (`.bfnanm`), curves, pose composition, skinning, bone palettes, frame interpolation, playback rate and the extra root-motion curve. Who plays which animation is in [level-objects.md](level-objects.md) (NPCs) and [player-movement.md](player-movement.md) (player). Particles are [particles.md](particles.md).

## 2. Status

IN_PROGRESS, updated 2026-10-13T21:46:54+0200. Player and level characters are posed and skinned natively; animation playback for NPCs is not implemented.

## 3. Verified behaviour

- Pose math replicates game routines 0x24c734, 0x248870, 0x24c5a0, 0x252cb0; bone palettes, frame interpolation and the frame count (u16 +4 & 0x3fff) recovered; one curve stores frames + 1.
- Playback rate comes from the joint table word and the instance; the player's rate and walk/run speeds are derived from code and data (no fitted constants).
- `chase-viewer` plays each animation at its stored rate (`animationFramesPerSecond`); `--fps` replaces it.
- Head attachment through `jointAttachment` (pinned test); joint orientation via the skeleton record's +0x88 quaternion.

## 4. Stand-ins

- None.

## 5. Open items

- [ ] ANIM-1 — Cutscene actors play on the camera's clock; whether an actor's own stored rate applies is not recovered, and whether any shipped animation stores rate 0 is not checked. evidence: [skeleton-animation.md §2026-10-14T13:39](../../re/skeleton-animation.md#ts-20261014-133953).
- [ ] ANIM-2 — Playback of NPC animations is not implemented. blocks: LEVOBJ-2, SCRIPT-32. evidence: [objects.md §2026-10-13T16:34](../../re/objects.md#ts-20261013-163430).
- [ ] ANIM-3 — Blending between different animations is not implemented. evidence: [skeleton-animation.md §2026-10-12T22:10](../../re/skeleton-animation.md#ts-20261012-2210).
- [ ] ANIM-4 — The shared character rig code for animals and custom characters is missing (rigs of `char_pig`, `char_cat`, `char_dog`, `char_shark`, `char_Crow`). see LEVOBJ-4.

## 6. Findings summary

- Containers, keyframes, pose composition, interpolation, bone palettes: [skeleton-animation.md §2026-10-12T18:40](../../re/skeleton-animation.md#ts-20261012-1840), [skeleton-animation.md §2026-10-12T20:15](../../re/skeleton-animation.md#ts-20261012-2015), [skeleton-animation.md §2026-10-12T21:30](../../re/skeleton-animation.md#ts-20261012-2130).
- Playback rate and the extra (root motion) curve: [skeleton-animation.md §2026-10-13T17:02](../../re/skeleton-animation.md#ts-20261013-170203).
- Head attachment: [player-entity-input.md §2026-10-13T12:56](../../re/player-entity-input.md#ts-20261013-125622).

## 7. Code and tests

- `src/assets/anim.cpp`, `src/render/character.*`, C ABI `chase_assets_c` (`tools/chase_native.py`).
- CTest: `assets-core`, `assets-real-data`, `viewer-real-smoke`.
