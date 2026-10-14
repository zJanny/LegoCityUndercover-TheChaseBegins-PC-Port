# Rendering: materials, lighting, blending

## 1. Scope

How `chase_render` (`src/render/scene.*`, used by `chase-viewer` and `chase-game`) draws models: the texture-environment stages built from each material, blending, alpha test, depth and culling. Fragment lighting, reflections, fog and shadows. Formats and decoders are in [assets-formats.md](assets-formats.md).

## 2. Status

IN_PROGRESS, updated 2026-10-14T09:50:11+02:00. Combiner setups of all 16122 shipped parts are recovered and run in the shader; lighting is a stand-in.

## 3. Verified behaviour

- `materialState` (`src/assets/material.cpp`) replicates 0x12e460 and 0x1292fc for every shipped material (census pinned in `assets-real-data`, named setups pinned there and in `assets-core`).
- Level surfaces match the oracle at gameplay-start (mean colour difference 7 to 11 on road crops, was 64). The player crop differs by 0.8.
- Chase's eyes are no longer holes: the alpha test only exists where the material asks for it (`docs/re/materials.md`).
- Unit order is reversed for reflection materials, so the diffuse texture is Texture1 there.

## 4. Stand-ins

- RENDER-S1 Fragment lighting primary of lit props and characters is the global ambient read once from the original ((138, 161, 182) / 255); level surfaces with baked colours (lighting 1 and 4) use 0. Secondary (specular) is 0. Source: [materials.md §2026-10-13T22:24](../../re/materials.md#ts-20261013-222439).
- RENDER-S2 Reflection cube textures are replaced by their average colour.
- RENDER-S3 Blended parts are drawn after opaque parts in instance order (the game sorts by 0x115128).
- RENDER-S4 A missing vertex colour attribute is white; the texture unit to UV set mapping and wrap modes are assumed.
- ASSET-S1 (replaced by the above for materials; the vertex shaders' normal/view outputs stay unused).

## 5. Open items

- [ ] RENDER-1 Fragment lighting: light list per draw (0x12d884, 0x5943b0), light mask, LUTs, specular. Next: break at 0x12d884 in the oracle (`trap`) and read the light objects and material +0x10/+0x14 for a character and a prop; implement in the shader; remove RENDER-S1. evidence: [materials.md](../../re/materials.md)
- [ ] RENDER-2 Reflections (cube sampling, vertex shader program with `Cubemap`). Removes RENDER-S2.
- [ ] RENDER-3 Fog (0x1292fc caller, 0x3760fc), shadows (0x37796c), material colour pulse, draw sorting.
- [x] RENDER-4 Closed 2026-10-14: the lobby compared with the oracle (lightmap surfaces match within 1 colour level on the stair wall, floor about 8 percent lighter; props darker = RENDER-1/5; camera differs, see CAMERA-6). evidence: [materials.md §2026-10-14T09:50](../../re/materials.md#ts-20261014-095011).
- [ ] RENDER-5 Characters are lighter and more saturated in the original (directional light missing).
- [ ] RENDER-9 — The player's shadow (dark patch beside the feet in the oracle picture of the police entrance) is not drawn; part of RENDER-3 (shadows 0x37796c). The pale disc seen there earlier was an object (LEVOBJ-11, closed). evidence: [objects.md §2026-10-14T15:05](../../re/objects.md#ts-20261014-150541).
- [ ] RENDER-6 — The white quad above the police-station entrance is the `Checker1.tga` part of `room1.bfnmdl` (also room3, room5), seen far away outside; not RENDER-S3. Unknown whether the original draws it. Not fixed (related: WSTREAM-3). evidence: [materials.md §2026-10-14T13:40](../../re/materials.md#ts-20261014-134000).

## 6. Findings summary

- [materials.md §2026-10-13T22:24](../../re/materials.md#ts-20261013-222439); UV scales and vertex shaders: [models.md §2026-10-12T18:05](../../re/models.md#ts-20261012-1805).

## 7. Code and tests

- `src/assets/material.cpp`, `src/render/scene.cpp` (`src/render/shaders/scene.frag.glsl` runs the stages; all shaders are `.glsl` files embedded at build time by `tools/embed_shaders.cmake`; OpenGL 4.6 core), CTest `no-inline-shaders`, `src/materials.cpp` (`chase-materials`: census, or stages of the models given).
- CTest `assets-core` (hand-evaluated materials), `assets-real-data` (census and named setups). Scene: `setInstanceMesh` (one mesh per instance, used by two-leaf doors). Game option `--facing DEGREES` places the character for screenshot comparisons.
