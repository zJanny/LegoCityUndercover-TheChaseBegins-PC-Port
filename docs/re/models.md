# Models, vertex layout and materials

Reverse-engineering findings, moved verbatim from the progress log. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis.

## Current state

- `.bfnmdl` records: model (0x24), node (0x18), mesh (0x24), part (0x24), material (0x60), vertex buffer header (0x78); deserializer 0x10ccf4, material 0x114860, vertex buffer 0x11528c. [§2026-10-12T16:10](#ts-20261012-1610)
- The model+0x1c section is collision geometry (see collision.md), not render data. [§2026-10-13T14:46:37](#ts-20261013-144637)
- Vertex layout from mask 0x12095c: position f32x3, uv0/uv1 s16x2, normal s8x3, colour RGBA8, 8 bytes skin weights and indices (names vWeights/vIndex confirmed in the vertex shaders). [§2026-10-12T18:05](#ts-20261012-1805)
- UVs are raw s16 times the f32 scales at vertex-buffer header +0x50; level vertices are in world space. [§2026-10-12T18:05](#ts-20261012-1805)
- `parseModel` (`src/assets/model.cpp`) parses all 3113 models (16122 parts, 7211595 vertices, 4603391 triangles); the Python model tools are gone. [§2026-10-13T15:08:53](#ts-20261013-150853)
- Combiner setups are recovered in [materials.md](materials.md); the lightmap modulation in the viewer is an approximation.
- The model draw 0x205160 draws each node's mesh with the node's matrix (palette node+0x110, else model+0x20, when w is not 0) before the node world, skinned vertices after their skinning; `assets::nodeDrawMatrix`. [§2026-10-19T19:24:18](#ts-20261019-192418)
- Open: meaning of node and mesh float3 fields, skin field semantics, material render states, the node override list 0x5300f4.

<a id="ts-20261012-1610"></a>

## 2026-10-12T16:10+02:00 — .bfnmdl model format decoded (REA, Ghidra 12.1.2 provider)

Corrected by [models.md §2026-10-13T14:46](#ts-20261013-144637): the model+0x1c section is collision geometry

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

<a id="ts-20261012-1645"></a>

## 2026-10-12T16:45+02:00 — S4a part 2: model geometry in C++

- Added src/assets/model.cpp. parseModel replays 0x10ccf4/0x114860/0x11528c through PoolCursor. vertexFlags is the 0x12095c replica, built on a byte-exact copy of the rodata tables at 0x3798c4–0x379907. Vertices decode to position/uv0/uv1/normal/colour; skin bytes are skipped. Indices are bounds-checked.
- tools/convert_models.py --census now reports totals: 16122 parts, 7211595 vertices, 4603391 triangles.
- assets-core adds a synthetic model (mirroring the Python fixture), an unconsumed-block rejection and the stride table. assets-real-data now parses all models and requires the Python totals.
- `cmake --build build` — 0 warnings. `ctest --test-dir build --output-on-failure` — PASS 4/4 (assets-real-data 10.2 s): 3113 models / 16122 parts / 7211595 vertices / 4603391 triangles, all equal to the Python reference.
- Remaining in S4a: CWAV decode in C++ (needed for S4d, not for the S4b viewer).

<a id="ts-20261012-1805"></a>

## 2026-10-12T18:05+02:00 — Vertex shaders, UV scales and lightmaps

The combiner setups that this entry lists as unrecovered were recovered later: [materials.md](materials.md).

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

<a id="ts-20261013-150853"></a>

## 2026-10-13T15:08:53+02:00 — Python model tools removed

- tools/convert_models.py (`--census`, `--vertices`, `bone_batches`, `skin_joints`) and tools/preview_model.py were removed. The C++ parser (`parseModel`) and `chase-viewer --screenshot` replace them.
- Reference values from the earlier entries stay valid and are pinned in tests/assets_real_test.cpp: the model census totals, the bone palette totals and the skinned position sums. Provenance: `python3 tools/convert_models.py --census artifacts/fib/files` and preview_model at 9d04c91. To reproduce, check out that commit (for example `git worktree add /tmp/old 9d04c91`) and run the command there.

<a id="ts-20261013-144637"></a>

## 2026-10-13T14:46:37+02:00 — Correction: the model+0x1c section is collision geometry

- The "model+0x1c section" that earlier entries ([Corrects models.md §2026-10-12T16:10](#ts-20261012-1610) and the loader notes of 0x114640/0x11fd20) skipped as unrecovered holds the collision mesh: items with a name, a bounding-box tree, 0x20-byte triangle records and vertices. Format, queries and tests: [collision.md](collision.md). `parseModel` now keeps it in `Model::collision`.

<a id="ts-20261019-192418"></a>

## 2026-10-19T19:24:18+02:00 — The model draw applies every node's matrix (model+0x20) to its mesh, after skinning (REA with Ghidra 12.1.2, llvm-objdump, a PICA200 disassembly of vertex program 0, census over all models with the C++ parser)

Answers the open order in [player-traversal.md §2026-10-19T18:16:50](player-traversal.md#ts-20261019-181650). [Corrects cutscene-actors-props.md §2026-10-19T18:30:11](cutscene-actors-props.md#ts-20261019-183011): the original skins first and applies the node matrix after. [Corrects player-entity-input.md §2026-10-13T12:56:22](player-entity-input.md#ts-20261013-125622): the head's 1.364 offset is its own node matrix.

**Method.** REA (Ghidra 12.1.2) pseudocode of 0x2442b4, 0x2123cc, 0x22356c, 0x242eb0, 0x250830, 0x22cbbc, 0x205160, 0x10e028, 0x10bbe8, 0x188778, 0x188aac, 0x2273f8, 0x1dcaa8, 0x2390fc, 0x250ed0, 0x253a00, 0x253b6c, 0x10da18, 0x253e70, 0x108238, 0x2121e8, 0x24f444, 0x2023bc, 0x242814, 0x22690c; llvm-objdump (`--triple=armv6k --mattr=+vfp2`) of the multiplies 0x2588f4, 0x258a10 and the copy 0x258858 (vector mode, length 4); a scratch PICA200 decoder over the DVLB at code.bin 0x2779a0 (program 0, code 0x00..0x4a). The model node record +0x14 blocks were dumped with a temporary print in `parseModel` (not committed).

**Scene node of a model.** 0x22356c (called by 0x2123cc for `models/<meshname>/<leaf>.fnmdl`, from the create's 0x2442b4) allocates a 0x11c-byte node: +0xc0 the model resource (and +0xc4 a `_lod1` resource), +0xc8 the controller (0x2123c4), +0x110 the palette (null until set), +0x114 a per-node hide bit set. 0x250830(resource) returns the parsed model (resource +0x14 when its state byte +8 is 2).

**Draw 0x205160** (one scene node): picks the LOD (0x10da18), gets a skin palette from the controller when node +0xc8 is set (0x253e70 through the type table 0x52ce94: joint count `0x5300a4`, palette `0x5300f0`, 0x40 bytes per joint), then walks the model's nodes (record 0x18 bytes: +0x10 mesh index, +0x12 parent index, +0x14 joint name) and for every node with a mesh that the hide bits do not hide calls **0x10e028(model, scene node, node index, node world)**:
- an override list (`0x5300f4` records of 0x4c bytes, matched by node name, byte +0x48 hides the node) gives the matrix first (filled elsewhere, not traced);
- else the palette entry `node+0x110 + i*0x40`, or `model+0x20 + i*0x40` when the palette is null, when its w (+0x3c) is not 0;
- when the node record +0x14 is set and the controller has a skeleton, the skin matrix of that joint (0x24f444 matches the name against the skeleton's joint records) follows;
- the result is matrix · [joint skin ·] node world (0x2588f4 out = a · b for row vectors; 0x258a10 out = out · b), else the node world itself.
The parts of the mesh are then queued (0x10bbe8 / 0x188aac) with that matrix as the model matrix (0x253b6c) and, for a skinned node, the palette pointer and joint count (0x2390fc). The bounds routine 0x242eb0 ([collision.md §2026-10-13T20:53:44](collision.md#ts-20261013-205344)) uses the same palette rule.

**Palette 0x22cbbc(node, i, matrix, recurse, ...)** writes entry i; on the first write it allocates the palette and copies the whole model+0x20 block (identity entries when the model has none); with recurse set, every child node (record +0x12 == i) gets its model matrix · the new matrix. So doors, animated nodes and cutscene object tracks replace an entry; the draw takes the stored matrix per node with no parent chain.

**Skinned vertices: skin first, node matrix after.** 0x188778 sets the record's model matrix (0x253b6c) and palette (0x2390fc); 0x2273f8 uploads the bones of each batch raw (0x1dcaa8: `Bones` registers from palette[batch joint]); 0x250ed0 composes the model matrix (with the node's scale and offset) into the matrix the shader gets. Vertex program 0: `r0 = vPos`; when bool `Skinned` (b0) is set, the subroutine at 0x10 computes Σ weight · (r0 · Bones[index]); then `o0 = r0 · ViewComposite` (c0..c3). So a skinned vertex is drawn at (Σ w · v · skin) · node matrix · node world.

**Data (census over 3113 models).** 926 models carry the model+0x20 block, 614 of them a non-identity matrix on a node with a mesh (379 with more than one node); no mesh is shared by two nodes and no mesh is without a node. Examples: every minifig head (`head`, translation 0, 1.364, 0.0381; 117 of 117 heads used by level characters, `Char_Head_Chase` too), the trooper's `BrakeLight` (z -0.708: its mesh at z -2.115 moves to the rear bumper, -2.82), wheels (`veh_wheel_17x6_a` x -0.1906 centres the hub), `Prop_Plant_04` (y -0.0378), `Prop_Construction_Elevator_Runners` (a 90 degree turn), the zipline pole (y -27.432), `veh_van_dulmen` (z -0.1905), the dog footprint card and the two skinned models (`policebarker`, `prop_furnacevent`). 155 nodes name a joint in record +0x14 (`head` in the csdata minifigs, `JNT_10`, `Master_Jnt`); no runtime head model does.

**Port.** `assets::nodeDrawMatrix` (src/assets/model.cpp) is the rule; `Scene::addModel` draws every node's mesh with it before the instance transform, skinned parts after their CPU skinning, bounds included; `Scene::setNodeMatrix` replaces one entry (doors). Removed special cases: the zipline pole matrix, the skinned fit `skinnedPartInModelSpace`, the detective card's node matrix (which the port had applied after the card's turn). Instances with `onlyMesh` (prop nodes, cutscene object tracks) keep a caller-written node matrix, as 0x22cbbc does. Heads and hats hang on their joint's skin matrix and get their own node matrix from the scene. CTest `node-draw-matrix-real-data`, `zipline-pole`, `zipline-pole-real-data`.

Unknowns:
- Who fills the override list `0x5300f4` and the second list `0x5300fc` / `0x530104`; the LOD switch (`_lod1`, distance node +0xb8) is not ported.
- The joint-bound nodes (record +0x14) are drawn without their joint's skin matrix in the port (MOVIE-40).
- The consumer of the attach record 0x242814 stores at node+0x10 for a `HeadModel` / `HatModel` node (LEVOBJ-S92); 0x2121e8 also lowers the attached node by `0x376ab8` for `char_minifig_small`.
