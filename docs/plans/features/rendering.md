# Rendering: materials, lighting, blending

## 1. Scope

How `chase_render` (`src/render/scene.*`, used by `chase-viewer` and `chase-game`) draws models: the texture-environment stages built from each material, blending, alpha test, depth and culling. Fragment lighting, reflections, fog and shadows. Formats and decoders are in [assets-formats.md](assets-formats.md).

## 2. Status

IN_PROGRESS, updated 2026-10-14T19:00:00+02:00. OpenGL moved out of the game and viewer code into `src/render/` (no behaviour change). Combiner setups of all 16122 shipped parts are recovered and run in the shader; lighting is a stand-in.

## 3. Verified behaviour

- `materialState` (`src/assets/material.cpp`) replicates 0x12e460 and 0x1292fc for every shipped material (census pinned in `assets-real-data`, named setups pinned there and in `assets-core`).
- Level surfaces match the oracle at gameplay-start (mean colour difference 7 to 11 on road crops, was 64). The player crop differs by 0.8.
- Chase's eyes are no longer holes: the alpha test only exists where the material asks for it (`docs/re/materials.md`).
- Unit order is reversed for reflection materials, so the diffuse texture is Texture1 there.

## 4. Stand-ins

- RENDER-S1 Fragment lighting primary of lit props and characters is the global ambient read once from the original ((138, 161, 182) / 255); level surfaces with baked colours (lighting 1 and 4) use 0. Secondary (specular) is 0. Source: [materials.md §2026-10-13T22:24](../../re/materials.md#ts-20261013-222439).
- RENDER-S2 Reflection cube textures are replaced by their average colour.
- RENDER-S3 Blended parts are drawn after opaque parts in instance order (the game sorts by 0x115128). needs: RENDER-35.
- RENDER-S4 A missing vertex colour attribute is white; the texture unit to UV set mapping and wrap modes are assumed.
- RENDER-S5 Fog depth uses projection near 1 and far 260 (`src/runtime/fog.hpp`); the original's source is not found and the matrix address of the first reading was wrong. needs: RENDER-40. evidence: [rendering-fog.md §2026-10-14T18:59](../../re/rendering-fog.md#ts-20261014-185910).
- RENDER-S6 Fog object choice: an object applies without a room or in the current room, accepted when start >= 0 and start < end, last wins; the original also runs a second loop over the loaded rooms (first accepted wins and replaces) and checks an enable byte, neither is replicated. needs: RENDER-39. evidence: [rendering-fog.md §2026-10-14T18:59](../../re/rendering-fog.md#ts-20261014-185910).
- RENDER-S7 The fog table interpolation at fragment level follows Citra/Azahar behaviour, not measured on a console. evidence: [rendering-fog.md §2026-10-14T17:35](../../re/rendering-fog.md#ts-20261014-173500).
- RENDER-S8 Sky models (class `props/prop_skybox`, textures `skybox_*`) are not drawn: the port skips parts whose first texture name contains "sky", so the sky area shows the clear colour (the original's) instead of the model's gradient. needs: RENDER-37. evidence: [rendering-fog.md §2026-10-14T18:59](../../re/rendering-fog.md#ts-20261014-185910).
- ASSET-S1 (replaced by the above for materials; the vertex shaders' normal/view outputs stay unused).

## 5. Open items

- [ ] RENDER-1 Fragment lighting: light list per draw (0x12d884, 0x5943b0), light mask, LUTs, specular. Next: break at 0x12d884 in the oracle (`trap`) and read the light objects and material +0x10/+0x14 for a character and a prop; implement in the shader; remove RENDER-S1. evidence: [materials.md](../../re/materials.md)
- [ ] RENDER-2 Reflections (cube sampling, vertex shader program with `Cubemap`). Removes RENDER-S2.
- [ ] RENDER-30 Fog against the oracle at a long street: numbers recorded for a different camera (3 to 13 per channel, sky model missing); needs an oracle shot with RPC at a known pose (state `street-after-1-1`, player and camera pose written down) and the port at the same pose to compare pixel by pixel. needs: RENDER-37. evidence: [rendering-fog.md §2026-10-14T18:59](../../re/rendering-fog.md#ts-20261014-185910).
- [ ] RENDER-31 Room pop-in: does the fog (full at 260, visible from about 86 in the street) hide room loading? Not analysed; streaming is unchanged. evidence: [rendering-fog.md §2026-10-14T17:35](../../re/rendering-fog.md#ts-20261014-173500).
- [ ] RENDER-33 Shadows (0x37796c). blocks: RENDER-9.
- [ ] RENDER-34 Material colour pulse over time.
- [ ] RENDER-35 Draw sorting (0x115128). blocks: RENDER-S3.
- [ ] RENDER-37 Sky models: placement relative to the camera, draw order and fog handling of `props/prop_skybox` objects and the `SkyBox_Skyline_02` part of lvl_00 room1; `SkyboxOverride` (0x392254) and `SKY%02u` (0x3813b4) unread. blocks: RENDER-S8, RENDER-30. evidence: [rendering-fog.md §2026-10-14T18:59](../../re/rendering-fog.md#ts-20261014-185910).
- [ ] RENDER-38 Clear colour: apply the script native `SetBackgroundColour` (0x291624, 5 call sites in the level scripts) and find the per-frame reader of the colour (not 0x589890). evidence: [rendering-fog.md §2026-10-14T18:59](../../re/rendering-fog.md#ts-20261014-185910).
- [ ] RENDER-39 Fog loop 2 over the loaded rooms (0x21b51c, room object chain `[room+0x60]`) and the object enable byte `[[obj+0x3c]+0xd8]`: replicate once the room load state is known; check on lvl_00, lvl_02 and lvl_05 against the oracle. blocks: RENDER-S6. evidence: [rendering-fog.md §2026-10-14T18:59](../../re/rendering-fog.md#ts-20261014-185910).
- [ ] RENDER-40 Source of projection near 1 and far 260 and the address of the matrix with A = 1.0038609 (0x52a050 holds another matrix in both levels). blocks: RENDER-S5. evidence: [rendering-fog.md §2026-10-14T18:59](../../re/rendering-fog.md#ts-20261014-185910).
- [ ] RENDER-5 Characters are lighter and more saturated in the original (directional light missing).
- [ ] RENDER-9 — The player's shadow (dark patch beside the feet in the oracle picture of the police entrance) is not drawn; part of RENDER-33 (shadows 0x37796c). needs: RENDER-33. The pale disc seen there earlier was an object (LEVOBJ-11, closed). evidence: [objects.md §2026-10-14T15:05](../../re/objects.md#ts-20261014-150541).
- [ ] RENDER-6 — The white quad above the police-station entrance is the `Checker1.tga` part of `room1.bfnmdl` (also room3, room5), seen far away outside; not RENDER-S3. Unknown whether the original draws it. Not fixed (related: WSTREAM-3). evidence: [materials.md §2026-10-14T13:40](../../re/materials.md#ts-20261014-134000).

## 6. Findings summary

- Fog: [rendering-fog.md §2026-10-14T17:35](../../re/rendering-fog.md#ts-20261014-173500), clear colour and selection loops: [rendering-fog.md §2026-10-14T18:59](../../re/rendering-fog.md#ts-20261014-185910).
- [materials.md §2026-10-13T22:24](../../re/materials.md#ts-20261013-222439); UV scales and vertex shaders: [models.md §2026-10-12T18:05](../../re/models.md#ts-20261012-1805).

## 7. Code and tests

- `src/assets/material.cpp`, `src/render/scene.cpp` (`src/render/shaders/scene.frag.glsl` runs the stages; all shaders are `.glsl` files embedded at build time by `tools/embed_shaders.cmake`; OpenGL 4.6 core), CTest `no-inline-shaders`, `src/materials.cpp` (`chase-materials`: census, or stages of the models given).
- All OpenGL calls are in `src/render/*.cpp`. Callers outside `src/render/` see plain-type headers only: `scene.hpp`, `studs.hpp`, `debug_overlay.hpp`, `window.hpp` (viewports, scissor, clears, depth and culling), `hud_overlay.hpp` (two-screen overlay) and `front_presenter.hpp` (front-end screens). Each keeps its GL objects behind a pointer to an implementation class in its `.cpp`. `program.hpp` (shared scene shader) is internal to `src/render/`. CTest `gl-only-in-render` (guard) and `gl-only-in-render-rejects` (fixture `tests/gl_only_in_render/`).
- Fog: `src/runtime/fog.cpp` (table, level choice), shader `applyFog` in `scene.frag.glsl`, `setFog` in `scene.hpp`; CTest `fog-core`, `fog-real-data` (also the clear colour `levelBackgroundColour`, `src/assets/level.cpp`).
- CTest `assets-core` (hand-evaluated materials), `assets-real-data` (census and named setups). Scene: `setInstanceMesh` (one mesh per instance, used by two-leaf doors). Game option `--facing DEGREES` places the character for screenshot comparisons.
