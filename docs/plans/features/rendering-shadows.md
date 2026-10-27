# Rendering: shadows

## 1. Scope

The shadows of characters, props and vehicles: the shadow light, the shadow camera, the caster pass and how receivers apply the shadow texture. Split from [rendering](rendering.md).

## 2. Status

IN_PROGRESS, updated 2026-10-27T19:02:08+01:00. The shadow map of the original runs in `chase-game`: the level's shadow light, the focus ahead of the camera, the orthographic shadow camera, the `Shadow_Caster` / `Shadow_Receiver` flags of the player, the level characters and the placed objects, and the two darkening stages (RENDER-33, RENDER-9 closed). Edges are hard, street people and traffic cast none yet.

## 3. Verified behaviour

- Outside the police station after mission 1-1 the port's focus, the shadow camera's axes, y and z, the projection and the shadow constant 0x999999 equal the oracle's; the player's shadow has the oracle's shape and darkness (0.37 to 0.39 of the lit ground in the oracle, 0.40 in the port). CTest `shadow-core`, `shadow-real-data`. evidence: [rendering-shadows.md §2026-10-27T19:02:08](../../re/rendering-shadows.md#ts-20261027-190208).

## 4. Stand-ins

- RENDER-S67 The PICA shadow framebuffer and shadow texture lookup are a 512 x 512 depth texture with a linear depth comparison (hard edges), casters are drawn without culling, and the two receiving stages run as a second multiplying draw of the receiving parts after the scene (so they also scale fog and blended draws over them). needs: RENDER-72. evidence: [rendering-shadows.md §2026-10-27T19:02:08](../../re/rendering-shadows.md#ts-20261027-190208).
- RENDER-S68 Every room model receives (the +0xc byte of the room records is not read), every material is taken to use a program with the `Shadows` uniform (the program table is not read), and the focus ray uses the two camera values read in the oracle instead of their writer. needs: RENDER-73, RENDER-74. evidence: [rendering-shadows.md §2026-10-27T19:02:08](../../re/rendering-shadows.md#ts-20261027-190208).

## 5. Open items

- [ ] RENDER-72 The PICA shadow pass: what the casters write (depth, penumbra intensity), the texture register 0x8b (bias, perspective), the soft edges, and the cull flip of the caster pass; compare the edge with the oracle. blocks: RENDER-S67. evidence: [rendering-shadows.md §2026-10-27T19:02:08](../../re/rendering-shadows.md#ts-20261027-190208).
- [ ] RENDER-73 The other casters and receivers: street people, traffic, nested props, hand items and debris are not flagged; the flag byte of the room records and the program table decide which room draws receive. blocks: RENDER-S68. evidence: [rendering-shadows.md §2026-10-27T19:02:08](../../re/rendering-shadows.md#ts-20261027-190208).
- [ ] RENDER-74 The writer of the two camera values of the focus ray and the camera entity the shadow camera step measures: the oracle's shadow camera is 2.08 m further along x than the port's at the police station pose. blocks: RENDER-S68. evidence: [rendering-shadows.md §2026-10-27T19:02:08](../../re/rendering-shadows.md#ts-20261027-190208).

## 6. Findings summary

- The shadow map (light, focus, camera, flags, stages) and the oracle comparison: [rendering-shadows.md §2026-10-27T19:02:08](../../re/rendering-shadows.md#ts-20261027-190208).
- EnableShadow and the kind gate (bit 2 of comp+0x316, the character ground probe 0x203b34): [object-kinds.md §2026-10-19T07:55:07](../../re/object-kinds.md#ts-20261019-075507).

## 7. Code and tests

- `src/runtime/shadow.*` (light, constant, focus, camera, projection, flags), `src/render/shadow.*` with `shadow.{vert,frag}.glsl` (caster pass and receiving draw), `Scene::setInstanceShadow` / `Scene::shadowParts`, glue `src/game_shadows.*` (`LevelSession`); `assets::LightObject` reads `CastShadows`, `ShadowOpacity`, `ShadowColour`, `ShadowRadius`. CTest `shadow-core`, `shadow-real-data`.
