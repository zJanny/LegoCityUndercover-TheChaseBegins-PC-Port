# Models, vertex layout and materials

Reverse-engineering findings, moved verbatim from the progress log. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis.

## 2026-10-12T16:10+02:00 — .bfnmdl model format decoded (REA, Ghidra 12.1.2 provider)

- Opened game.arm.elf with provider_id=ghidra. Ghidra defines the procedures Hopper missed, and its pseudo-code evidence IDs are:

  | Function | Evidence ID | Recovered role |
  |---|---|---|
  | 0x10ccf4 | ev_4bdf7c11…c1e51c7 | model deserializer |
  | 0x114860 | ev_a1448f66…3d91e | material |
  | 0x11528c | ev_0346c245…f0f0d | vertex buffer |
  | 0x114640 | ev_080a4fc6…28d97 | model+0x1c section |
  | 0x11fd20 | ev_44944596…b7c3fc3b | sub-section |
  | 0x12095c | ev_91ccfb89…5c3d363 | vertex flags |
  | 0x1205bc | ev_259de55d…53f2 | buffer relocation callback |
  | 0x113600 | ev_3b555d22…adfc | render-state dirty flags (not attribute setup) |

- A size-0 block is a NULL pointer, because 0x253800 returns 0 for it.
- Model record (−15, 0x24):
  - u16 flags, u16 nodes, u16 meshes, u8 textures, then pointers at +8, +0xc, +0x10, +0x14, +0x1c and +0x20.
  - Node (0x18): name pointer, float3, optional pointer at +0x14.
  - Mesh (0x24): u8 part count at +2, parts pointer at +4, float3 at +8.
  - Part (0x24): material at +0x18, extra at +0x1c, vertex buffer at +0x20.
- Material record (−15, 0x60): texture count = (b10&0x3f)>>3 and UV sets = b10&7. The texture slots are −16 data (n×8 records, then a name per slot).
- Vertex buffer header (−14, 0x78): u16 vertex count at +2, u32 index count at +4, u16 stride at +0x1a. Optional blocks follow when +0x24/+0x48 are set, and morph data when +0x44 is set. The index and vertex data are separate −14 groups, read straight into GPU memory by 0x24d1c4.
- Vertex mask 0x12095c is replicated using the rodata tables at 0x3798c4/d1/e9/f8. The attribute order was recovered from vertex data:
  - position f32×3
  - uv0 s16×2 (bit 4), uv1 (bit 8)
  - normal s8×3+pad (0x40); decoded vectors have length ≈127
  - colour RGBA8 (0x80)
  - 8 bytes (bit 2, set when m0=1), probably skin weights/indices; unconfirmed
  - Bit 1 (m0=2) does not occur.
- `python3 tools/convert_models.py --census artifacts/fib/files`:
  - 3113/3113 models parse with every pool block consumed.
  - All 16122 parts have an index count divisible by 3 (triangle lists).
  - All 16122 computed strides equal the stored stride.
- Visual checks with tools/preview_model.py (software renderer):
  - csdata trolley: a textured hand truck with wheels and cargo.
  - char_bear: bind pose, recognisable.
  - lvl_00_policestation/room1 with the SkyBox texture excluded: cliff, grass and road pieces line up, so level vertices are stored in world space.
- Not recovered: the meaning of node/mesh float3 fields, the model+0x1c section's contents, skin field semantics, and material render states.
- Added tools/convert_models.py, tools/preview_model.py and tests/test_convert_models.py (3 tests); suite PASS 36/36.

## 2026-10-12T16:45+02:00 — S4a part 2: model geometry in C++

- Added src/assets/model.cpp. parseModel replays 0x10ccf4/0x114860/0x11528c through PoolCursor. vertexFlags is the 0x12095c replica, built on a byte-exact copy of the rodata tables at 0x3798c4–0x379907. Vertices decode to position/uv0/uv1/normal/colour; skin bytes are skipped. Indices are bounds-checked.
- tools/convert_models.py --census now reports totals: 16122 parts, 7211595 vertices, 4603391 triangles.
- assets-core adds a synthetic model (mirroring the Python fixture), an unconsumed-block rejection and the stride table. assets-real-data now parses all models and requires the Python totals.
- `cmake --build build` — 0 warnings. `ctest --test-dir build --output-on-failure` — PASS 4/4 (assets-real-data 10.2 s): 3113 models / 16122 parts / 7211595 vertices / 4603391 triangles, all equal to the Python reference.
- Remaining in S4a: CWAV decode in C++ (needed for S4d, not for the S4b viewer).

## 2026-10-12T18:05+02:00 — Vertex shaders, UV scales and lightmaps

- The material post-load calls 0x115ec4 (ev_e7652328…b4c5) and 0x115718 (ev_5cfc022b…7939) are stubs that return 1. 0x115128 (ev_adf59b90…39aa) builds a draw-sort key from m1/m2/m3 and the textures. The material shading selection is not in the loader.
- Added tools/dump_dvlb.py, which reads the public SHBIN layout. code.bin contains one DVLB at file offset 0x2779a0 (data segment) with 11 vertex programs, 342 code words and 83 operand descriptors.
  - The symbol tables name the inputs: vPos, vUV0, vUV1, vColour, vNormal, vWeights and vIndex. This confirms the 8-byte vertex attribute (flag 2) is skin weights and indices.
  - They also name the uniforms: ViewComposite, ModelView, MaterialColour, UVScales, UVOffset, TexTrans, Bones (0x22–0x6f), Skinned, Shadows, Cubemap, SimpleTex, VColourTint, ObjspaceCamPos and OutlineScale.
  - Program 1 outputs normal_quat and view, i.e. it uses PICA fragment lighting.
  - The shader names are game-derived output and are not committed beyond this log.
- UV decoding fix: the vertex-buffer header holds the UV scales as f32 at +0x50 (uv0.x, uv0.y, uv1.x, uv1.y), and the shaders multiply the raw s16 values by them.
  - room60/room3 values range from 1.42e-5 to 1.94e-4. The earlier fixed 1/32767 matched only untiled parts; tiled walls/floors repeat 2–6×.
  - The Python and C++ decoders now use the stored scales; fixtures updated.
- In room3, the parts with uv1 carry a second texture "LIGHTMAP_lvl_00_PoliceStation_Basement000.tga" and black vertex colour (000000ff). Lit props carry "cubemap_abstract_c.tga" as their second texture.
- The viewer modulates by the lightmap on uv1 instead of vertex colour. This is an approximation, because the TEV combiner configuration is not recovered.
  - room3 now shows tiled floor and wall textures with lightmap shading.
  - The overall level stays dark, so a lightmap scale (e.g. ×2) is likely but unverified.
- `ctest` PASS 5/5 (real-data totals unchanged), Python PASS 39/39.

