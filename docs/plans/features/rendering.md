# Rendering: materials, lighting, blending

## 1. Scope

How `chase_render` (`src/render/scene.*`, used by `chase-viewer` and `chase-game`) draws models: the texture-environment stages built from each material, blending, alpha test, depth and culling. Fragment lighting, reflections, fog and shadows. Formats and decoders are in [assets-formats.md](assets-formats.md).

## 2. Status

IN_PROGRESS, updated 2026-10-18T09:41:47+02:00. Particle quads are drawn after the scene (blend, depth state of the original; PART-1). Sky models are drawn (RENDER-37). Combiner setups of all 16122 shipped parts are recovered and run in the shader. Fragment lighting follows the original: level light objects, the per-draw registers of 0x12d884 and the lookup tables of the lighting init (RENDER-1, RENDER-5).

## 3. Verified behaviour

- `materialState` (`src/assets/material.cpp`) replicates 0x12e460 and 0x1292fc for every shipped material (census pinned in `assets-real-data`, named setups pinned there and in `assets-core`).
- Level surfaces match the oracle at gameplay-start (mean colour difference 7 to 11 on road crops, was 64). The player crop differs by 0.8.
- Chase's eyes are no longer holes: the alpha test only exists where the material asks for it (`docs/re/materials.md`).
- Unit order is reversed for reflection materials, so the diffuse texture is Texture1 there.
- Fragment lighting: the registers the port computes for lvl_01 equal the oracle's command list (0x1c0, 0x140, 0x142, 0x149; CTest `lighting-core`, `lighting-real-data`). Chase's shirt at gameplay-start: oracle (80.9, 112.7, 157.9), port (76.5, 108.1, 153.8), was (62.6, 94.2, 141.5). evidence: [rendering-lighting.md §2026-10-15T17:47](../../re/rendering-lighting.md#ts-20261015-174725).

## 4. Stand-ins

- RENDER-S60 The lights that apply are those without a room or in the current room (the fog rule); the original's registration 0x2523cc was not read. needs: RENDER-60. evidence: [rendering-lighting.md §2026-10-15T17:47](../../re/rendering-lighting.md#ts-20261015-174725).
- RENDER-S61 Normals are interpolated (the PICA interpolates quaternions), light vectors are not rounded to float16, LUT scale 0x1d2 is assumed 0 and the LUT entries decode as value / 4096 plus difference / 2048 (PICA documentation, not measured). evidence: [rendering-lighting.md §2026-10-15T17:47](../../re/rendering-lighting.md#ts-20261015-174725).
- RENDER-S62 The specular exponent of LUT D0 is 20, the value of the variable 0x37798c read in the oracle at gameplay-start; its writer is not read. needs: RENDER-63. evidence: [rendering-lighting.md §2026-10-15T17:47](../../re/rendering-lighting.md#ts-20261015-174725)
- RENDER-S2 Reflection cube textures are replaced by their average colour.
- RENDER-S3 Blended parts are drawn after opaque parts in instance order (the game sorts by 0x115128). needs: RENDER-35.
- RENDER-S4 A missing vertex colour attribute is white; the texture unit to UV set mapping and wrap modes are assumed.
- RENDER-S6 Fog object choice: an object applies without a room or in the current room, accepted when start >= 0 and start < end, last wins; the original also runs a second loop over the loaded rooms (first accepted wins and replaces) and checks an enable byte, neither is replicated. needs: RENDER-39. evidence: [rendering-fog.md §2026-10-14T18:59](../../re/rendering-fog.md#ts-20261014-185910).
- RENDER-S7 The fog table interpolation at fragment level follows Citra/Azahar behaviour, not measured on a console. evidence: [rendering-fog.md §2026-10-14T17:35](../../re/rendering-fog.md#ts-20261014-173500).
- RENDER-S50 The sky draws without depth test and depth write, standing in for the original's depth scale (0.99951 on the projection depth entries, values at draw time not captured). needs: RENDER-50. evidence: [rendering-sky.md §2026-10-15T15:39](../../re/rendering-sky.md#ts-20261015-153917).
- RENDER-S51 A level with two sky objects (lvl_06) uses the first; the original's choice is not read. needs: RENDER-50. evidence: [rendering-sky.md §2026-10-15T15:39](../../re/rendering-sky.md#ts-20261015-153917).
- RENDER-S63 The particle pass uses texture x vertex colour with linear filtering and clamp to edge: the combiner registers of the draw 0x2722cc were not read, only its state bytes (texture alpha x vertex alpha, vertex colour). needs: RENDER-64. evidence: [particles.md §2026-10-18T09:40](../../re/particles.md#ts-20261018-094004).
- RENDER-S64 Emitters are drawn in creation order after the scene and the studs, and the view depth of a particle (near fade, far-to-near sort) is the distance along the camera's forward axis; the original's draw item order and model-view product were not read. needs: RENDER-65. evidence: [particles.md §2026-10-18T09:40](../../re/particles.md#ts-20261018-094004).
- ASSET-S1 (replaced by the above for materials; normals and the view vector now feed the fragment lighting, see RENDER-S61).

## 5. Open items

- [ ] RENDER-2 Reflections (cube sampling, vertex shader program with `Cubemap`). Removes RENDER-S2.
- [ ] RENDER-30 Fog fade at pixel level: the oracle and the port at the same pose (`street-after-1-1`) agree within about 12 per channel at depth 197 to 360, but windows of identical content were rare; needs one material seen over depth 86 to 260 in both pictures. evidence: [rendering-fog.md §2026-10-14T21:42](../../re/rendering-fog.md#ts-20261014-214220).
- [ ] RENDER-42 The port's near ground (fog 0, depth 17 to 33) is lighter than the oracle by about (+11, +10, +13) at the `street-after-1-1` pose; lighting or texture filtering of level surfaces. RENDER-1 is done; re-measure. evidence: [rendering-fog.md §2026-10-14T21:42](../../re/rendering-fog.md#ts-20261014-214220).
- [ ] RENDER-33 Shadows (0x37796c). blocks: RENDER-9.
- [ ] RENDER-64 Compare the particle draw with the original: an oracle picture (`--rpc` screenshot, OpenGL renderer) of the siren-light emitters of state `in-vehicle` (record 242) against `chase-game --particle` at the same pose: blend, colour, alpha and size. blocks: RENDER-S63. evidence: [particles.md §2026-10-18T09:40](../../re/particles.md#ts-20261018-094004).
- [ ] RENDER-66 The distance fade of mesh nodes: every mesh node fades out beyond a distance that depends on its radius (tiers below 3, 6 and 12 m radius, fade-out at 38, 76 and 114 m of surface distance, fade-in again below 36, 72 and 108; the alpha byte steps by 16 per update and the draw takes another pass below 255) unless `EnableDistanceFade(object, 0)` switched it off; the port draws every mesh opaque. Needs the inputs of 0x10eb70 checked in the oracle (eye position, step order, the two flag bytes) and the translucent pass. needs: RENDER-35. blocks: SCRIPT-S233, SCRIPT-282. evidence: [script-natives-world.md §2026-10-18T11:01:40](../../re/script-natives-world.md#ts-20261018-110140).
- [ ] RENDER-67 The semi-transparent near-camera lists of the rooms (entries of 0x1f40d8 filled by `AddToSemiTransNearCameraList`, flag bit 4): the draw side that reads them, which models become see-through near the camera (the waypoint columns and badges in lvl_01). needs: RENDER-35. blocks: SCRIPT-S233, SCRIPT-283. evidence: [script-natives-world.md §2026-10-18T11:01:40](../../re/script-natives-world.md#ts-20261018-110140).
- [ ] RENDER-65 Where the particle pass sits among the other blended draws: the draw item order of 0x2722cc's callers, the sort 0x115128 and the semi-transparent lists (script natives `AddToSemiTransList*`). needs: RENDER-35. blocks: RENDER-S64. evidence: [particles.md §2026-10-18T09:40](../../re/particles.md#ts-20261018-094004).
- [ ] RENDER-34 Material colour pulse over time.
- [ ] RENDER-35 Draw sorting (0x115128). blocks: RENDER-S3, RENDER-65.
- [ ] RENDER-50 Sky details: the projection depth scale of the pass (0x1811f4, values at draw time), the gate word `[[0x3772d0] + 0x8a]`, the choice among several sky objects, `SkyboxOverride` (string 0x192254, parameter of another class, constructor 0x192008) and the dome's lower rim at a high pose (rooftop (472.6, 104, 874.6) in lvl_01): compare with the original. blocks: RENDER-S50, RENDER-S51. evidence: [rendering-sky.md §2026-10-15T15:39](../../re/rendering-sky.md#ts-20261015-153917).
- [ ] RENDER-38 Clear colour: apply the script native `SetBackgroundColour` (0x291624, 5 call sites in the level scripts) and find the per-frame reader of the colour (not 0x589890). evidence: [rendering-fog.md §2026-10-14T18:59](../../re/rendering-fog.md#ts-20261014-185910). The native writes r, g, b (float to int) and 255 to offsets 4 to 7 of the singleton from 0x23878c (level setup 0x1bd054 writes the same bytes); implement it with the natives batch (SCRIPT-26..39). Reader still open: [rendering-sky.md §2026-10-15T15:39](../../re/rendering-sky.md#ts-20261015-153917).
- [ ] RENDER-39 Fog loop 2 over the loaded rooms (0x21b51c, room object chain `[room+0x60]`) and the object enable byte `[[obj+0x3c]+0xd8]`: replicate once the room load state is known; check on lvl_00, lvl_02 and lvl_05 against the oracle. blocks: RENDER-S6. evidence: [rendering-fog.md §2026-10-14T18:59](../../re/rendering-fog.md#ts-20261014-185910).
- [ ] RENDER-60 Light registration by room: the loop 0x2523cc over the registry 0x52fa44 and who fills it; check on lvl_00 rooms 3 and 5. blocks: RENDER-S60. evidence: [rendering-lighting.md §2026-10-15T17:47](../../re/rendering-lighting.md#ts-20261015-174725)
- [ ] RENDER-61 `LightMask` (0x23a9f4) with the objects' `lightexcludemask`, the flicker object of `LightUpdateType`, spot cones of type 3. evidence: [rendering-lighting.md §2026-10-15T17:47](../../re/rendering-lighting.md#ts-20261015-174725)
- [ ] RENDER-62 Point lights (type 2) against the oracle: lvl_00 `Point`/`Point01` (Intensity 2, MaxRange 1) on baked surfaces. evidence: [rendering-lighting.md §2026-10-15T17:47](../../re/rendering-lighting.md#ts-20261015-174725)
- [ ] RENDER-63 The writer of 0x37798c (the specular exponent of LUT D0) and whether it changes per level. blocks: RENDER-S62. evidence: [rendering-lighting.md §2026-10-15T17:47](../../re/rendering-lighting.md#ts-20261015-174725)
- [ ] RENDER-9 — The player's shadow (dark patch beside the feet in the oracle picture of the police entrance) is not drawn; part of RENDER-33 (shadows 0x37796c). needs: RENDER-33. The pale disc seen there earlier was an object (LEVOBJ-11, closed). evidence: [objects.md §2026-10-14T15:05](../../re/objects.md#ts-20261014-150541).
- [ ] RENDER-68 The glow post-process 0x2bf830 (pass 5 of 0x2e4858): screen copied to a 256 x 128 target, three blurs with the table modes 0xc / 0xd, added back with mode 0x12; off in normal lvl_01 play (0x516410 = 0), on in the scanner view with (0.6, 1.4). Needs the blur tables and the downsample material read in the oracle, then a `.glsl` pass in `src/render/`. evidence: [abilities-scanner.md §2026-10-19T14:06:22](../../re/abilities-scanner.md#ts-20261019-140622)
- [ ] RENDER-6 — The white quad above the police-station entrance is the `Checker1.tga` part of `room1.bfnmdl` (also room3, room5), seen far away outside; not RENDER-S3. Unknown whether the original draws it. Not fixed (related: WSTREAM-3). evidence: [materials.md §2026-10-14T13:40](../../re/materials.md#ts-20261014-134000).

## 6. Findings summary

- Fog: [rendering-fog.md §2026-10-14T17:35](../../re/rendering-fog.md#ts-20261014-173500), clear colour and selection loops: [rendering-fog.md §2026-10-14T18:59](../../re/rendering-fog.md#ts-20261014-185910).
- Sky: [rendering-sky.md §2026-10-15T15:39](../../re/rendering-sky.md#ts-20261015-153917).
- Particle draw state and quads: [particles.md §2026-10-18T09:40](../../re/particles.md#ts-20261018-094004).
- Lighting: [rendering-lighting.md §2026-10-15T17:47](../../re/rendering-lighting.md#ts-20261015-174725).
- Material override 0x530834 (scanner wash) and the glow post-process 0x2bf830: [abilities-scanner.md §2026-10-19T14:06:22](../../re/abilities-scanner.md#ts-20261019-140622).
- [materials.md §2026-10-13T22:24](../../re/materials.md#ts-20261013-222439); UV scales and vertex shaders: [models.md §2026-10-12T18:05](../../re/models.md#ts-20261012-1805).

## 7. Code and tests

- `src/assets/material.cpp`, `src/render/scene.cpp` (`src/render/shaders/scene.frag.glsl` runs the stages; all shaders are `.glsl` files embedded at build time by `tools/embed_shaders.cmake`; OpenGL 4.6 core), CTest `no-inline-shaders`, `src/render/particles.*` with `particle.{vert,frag}.glsl` (particle quads), `src/materials.cpp` (`chase-materials`: census, or stages of the models given).
- All OpenGL calls are in `src/render/*.cpp`. Callers outside `src/render/` see plain-type headers only: `scene.hpp`, `studs.hpp`, `debug_overlay.hpp`, `window.hpp` (viewports, scissor, clears, depth and culling), `hud_overlay.hpp` (two-screen overlay) and `front_presenter.hpp` (front-end screens). Each keeps its GL objects behind a pointer to an implementation class in its `.cpp`. `program.hpp` (shared scene shader) is internal to `src/render/`. CTest `gl-only-in-render` (guard) and `gl-only-in-render-rejects` (fixture `tests/gl_only_in_render/`).
- Sky: `src/runtime/sky.cpp` (lift, translation), `Scene::drawSky` in `src/render/scene.cpp`, wiring in `LevelSession::draw` (`src/game_level.cpp`); CTest `sky-core`, `sky-real-data`.
- Lighting: `src/runtime/lighting.cpp` (light objects to light list, per-draw registers, LUTs), `assets::lightObjects` (`src/assets/level.cpp`), `fragmentLighting` in `scene.frag.glsl`, `Scene::setLighting`; CTest `lighting-core`, `lighting-real-data`.
- Fog: `src/runtime/fog.cpp` (table, level choice), shader `applyFog` in `scene.frag.glsl`, `setFog` in `scene.hpp`; CTest `fog-core`, `fog-real-data` (also the clear colour `levelBackgroundColour`, `src/assets/level.cpp`).
- CTest `assets-core` (hand-evaluated materials), `assets-real-data` (census and named setups). Scene: `setInstanceMesh` (one mesh per instance, used by two-leaf doors). Game option `--facing DEGREES` places the character for screenshot comparisons.
