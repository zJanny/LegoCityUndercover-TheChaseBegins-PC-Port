# Animation and character rigs

## 1. Scope

Skeletons (`.bfnskl`), animations (`.bfnanm`), curves, pose composition, skinning, bone palettes, frame interpolation, playback rate and the extra root-motion curve. Who plays which animation is in [level-objects.md](level-objects.md) (NPCs) and [player-movement.md](player-movement.md) (player). Particles are [particles.md](particles.md).

## 2. Status

IN_PROGRESS, updated 2026-10-13T21:29:01+0200. Player and level characters are posed and skinned natively; animation playback for NPCs is not implemented.

## 3. Verified behaviour

- Pose math replicates game routines 0x24c734, 0x248870, 0x24c5a0, 0x252cb0; bone palettes, frame interpolation and the frame count (u16 +4 & 0x3fff) recovered; one curve stores frames + 1.
- Playback rate comes from the joint table word and the instance; the player's rate and walk/run speeds are derived from code and data (no fitted constants).
- Head attachment through `jointAttachment` (pinned test); joint orientation via the skeleton record's +0x88 quaternion.

## 4. Stand-ins

- ANIM-S1 `chase-viewer` plays animations at an assumed 30 fps (`--fps`) although each animation stores its rate (docs/model-viewer.md).

## 5. Open items

- [ ] ANIM-1 — The viewer's assumed 30 fps and the real animation speed of other animations. evidence: [skeleton-animation.md §2026-10-12T22:10](../../re/skeleton-animation.md), [skeleton-animation.md §2026-10-13T17:02](../../re/skeleton-animation.md).
- [ ] ANIM-2 — Playback of NPC animations is not implemented. blocks: LEVOBJ-2. evidence: [objects.md §2026-10-13T16:34](../../re/objects.md).
- [ ] ANIM-3 — Blending between different animations is not implemented. evidence: [skeleton-animation.md §2026-10-12T22:10](../../re/skeleton-animation.md).
- [ ] ANIM-4 — The shared character rig code for animals and custom characters is missing (rigs of `char_pig`, `char_cat`, `char_dog`, `char_shark`, `char_Crow`). see LEVOBJ-4.

## 6. Findings summary

- Containers, keyframes, pose composition, interpolation, bone palettes: [skeleton-animation.md §2026-10-12T18:40](../../re/skeleton-animation.md), [skeleton-animation.md §2026-10-12T20:15](../../re/skeleton-animation.md), [skeleton-animation.md §2026-10-12T21:30](../../re/skeleton-animation.md).
- Playback rate and the extra (root motion) curve: [skeleton-animation.md §2026-10-13T17:02](../../re/skeleton-animation.md).
- Head attachment: [player.md §2026-10-13T12:56](../../re/player.md).

## 7. Code and tests

- `src/assets/anim.cpp`, `src/render/character.*`, C ABI `chase_assets_c` (`tools/chase_native.py`).
- CTest: `assets-core`, `assets-real-data`, `viewer-real-smoke`.
