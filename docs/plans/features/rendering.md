# Rendering: materials, lighting, blending

## 1. Scope

How `chase_render` (`src/render/scene.*`, used by `chase-viewer` and `chase-game`) draws models: the texture-environment stages built from each material, blending, alpha test, depth and culling. Fragment lighting, reflections, fog and shadows. Formats and decoders are in [assets-formats.md](assets-formats.md).

## 2. Status

IN_PROGRESS, updated 2026-10-13T22:24:39+02:00. Combiner setups of all 16122 shipped parts are recovered and run in the shader; lighting is a stand-in.

## 3. Verified behaviour

- `materialState` (`src/assets/material.cpp`) replicates 0x12e460 and 0x1292fc for every shipped material (census pinned in `assets-real-data`, named setups pinned there and in `assets-core`).
- Level surfaces match the oracle at gameplay-start (mean colour difference 7 to 11 on road crops, was 64). The player crop differs by 0.8.
- Chase's eyes are no longer holes: the alpha test only exists where the material asks for it (`docs/re/materials.md`).
- Unit order is reversed for reflection materials, so the diffuse texture is Texture1 there.

## 4. Stand-ins

- RENDER-S1 Fragment lighting primary of lit props and characters is the global ambient read once from the original ((138, 161, 182) / 255); level surfaces with baked colours (lighting 1 and 4) use 0. Secondary (specular) is 0. Source: [materials.md §2026-10-13T22:24](../../re/materials.md).
- RENDER-S2 Reflection cube textures are replaced by their average colour.
- RENDER-S3 Blended parts are drawn after opaque parts in instance order (the game sorts by 0x115128).
- RENDER-S4 A missing vertex colour attribute is white; the texture unit to UV set mapping and wrap modes are assumed.
- ASSET-S1 (replaced by the above for materials; the vertex shaders' normal/view outputs stay unused).

## 5. Open items

- [ ] RENDER-1 Fragment lighting: light list per draw (0x12d884, 0x5943b0), light mask, LUTs, specular. Next: break at 0x12d884 in the oracle (`trap`) and read the light objects and material +0x10/+0x14 for a character and a prop; implement in the shader; remove RENDER-S1. evidence: [materials.md](../../re/materials.md)
- [ ] RENDER-2 Reflections (cube sampling, vertex shader program with `Cubemap`). Removes RENDER-S2.
- [ ] RENDER-3 Fog (0x1292fc caller, 0x3760fc), shadows (0x37796c), material colour pulse, draw sorting.
- [ ] RENDER-4 Compare an indoor police-station view with the oracle (checkpoint `police-station` exists since 2026-10-13, New 3DS mode, see oracle.md; lighting 4 lightmap surfaces are unverified against the original). blocks: DOOR-7, CAMERA-6, RENDER-6.
- [ ] RENDER-5 Characters are lighter and more saturated in the original (directional light missing).
- [ ] RENDER-6 — (User screenshot 2026-10-13) A white quad floats above the police-station entrance in `chase-game` (lvl_00); not classified, possibly a blended part drawn without the original's ordering (RENDER-S3). needs: RENDER-4.

## 6. Findings summary

- [materials.md §2026-10-13T22:24](../../re/materials.md); UV scales and vertex shaders: [models.md §2026-10-12T18:05](../../re/models.md).

## 7. Code and tests

- `src/assets/material.cpp`, `src/render/scene.cpp` (fragment shader runs the stages), `src/materials.cpp` (`chase-materials`: census, or stages of the models given).
- CTest `assets-core` (hand-evaluated materials), `assets-real-data` (census and named setups). Game option `--facing DEGREES` places the character for screenshot comparisons.
