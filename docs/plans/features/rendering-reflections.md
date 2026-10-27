# Rendering: reflections

Subtopic of [rendering.md](rendering.md).

## 1. Scope

Reflection cube maps on shiny materials (texture bytes 5, 6 and 7 of the material record): the reflection flag, the reflection vector of the vertex programs, the cube texture upload and its lookup in the scene shader (`src/render/scene.cpp`, `scene.vert.glsl`, `scene.frag.glsl`).

## 2. Status

IN_PROGRESS, split from rendering.md 2026-10-27, updated 2026-10-27T18:48:32+01:00. Reflection parts sample their cube map with the original's reflection vector (world-space reflection of the eye ray at the object-space normal) instead of its average colour (RENDER-2 closed, stand-in RENDER-S2 removed); vehicle windows show the sky and cloud cube map. The look is not yet compared with the oracle: the first oracle picture shows a van only from behind, where no part reflects (RENDER-75).

## 3. Verified behaviour

- The reflection flag, the reverse slot order, the cube on unit 0, uv0 for the diffuse unit with the cube, the face order of the stored faces and the switched-off cube units outside reflection materials are pinned by `reflection-core`; the census (354 reflection parts, all with a cube map on unit 0, three cube maps of 128 x 128 faces, no stage reads a switched-off unit) by `reflection-real-data`. evidence: [rendering-reflections.md §2026-10-27T18:36:08](../../re/rendering-reflections.md#ts-20261027-183608).

## 4. Stand-ins

- RENDER-S69 Which direction each stored cube face stands for and the coordinates within a face follow the GL cube map table as Citra and Azahar emulate the PICA; the cube unit filters linearly with mipmaps and clamps to the edge (the filter bits of the slot are not decoded). needs: RENDER-75. evidence: [rendering-reflections.md §2026-10-27T18:36:08](../../re/rendering-reflections.md#ts-20261027-183608).

## 5. Open items

- [ ] RENDER-75 Compare the reflections with the oracle: the van picture after mission 1-1 shows the van only from behind, where no part reflects; still needed is a reflecting window or prop seen from the front (a user save state with a police car parked in the open) against `chase-game` at the same pose; also check whether `TexTrans` can keep an earlier draw's value when no matrix is dirty. blocks: RENDER-S69. evidence: [rendering-reflections.md §2026-10-27T18:48:32](../../re/rendering-reflections.md#ts-20261027-184832).

## 6. Findings summary

- Reflection flag, vertex program cube subroutine, `TexTrans`, binding and census: [rendering-reflections.md §2026-10-27T18:36:08](../../re/rendering-reflections.md#ts-20261027-183608). Oracle van picture: [rendering-reflections.md §2026-10-27T18:48:32](../../re/rendering-reflections.md#ts-20261027-184832).

## 7. Code and tests

- `src/runtime/reflection.cpp` (`unitSampling`, `unitUvSet`, `cubeFace`), `src/render/reflection.cpp` (cube upload and binding), the hook in `drawPart` and `texture` of `src/render/scene.cpp`, `cubeOn` and the vector in `scene.vert.glsl`, unit mode 2 in `scene.frag.glsl`. CTest `reflection-core`, `reflection-real-data`.
