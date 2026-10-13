# Skeletons, animation and skinning

Reverse-engineering findings, moved verbatim from the progress log. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis.

## 2026-10-12T18:40+02:00 — Skeleton and animation containers (REA, Ghidra)

- Loaders:
  - 0x108f30 rewrites the path to .bfnanm and calls 0x10edec (ev_1b7f5557…8fc1).
  - The skeleton loader 0x10f26c (ev_fbc95e75…a557) opens .bfnskl itself.
  - The track reader is 0x253764 (ev_8cc8ebc4…997f).
  - Neither pushes a pool, and both formats use only the −14 pool, so blocks are handed out in order.
- Skeleton: the header's u8 at +1 is the joint count, followed by blocks for +8, +0xc and +4. Joint records are 0x98 bytes, each with a name block.
- Animation (0x10edec):
  - Optional sections: +8 (count from bits), +0xc (u8 +1 entries × 3 blocks), +0x10 (u8 +2 × 2), +0x14 (u8 +3 × 3), +0x18 tracks (u16 +6).
  - +0x1c sub-animations (u32 +0x20), each made of a fixup list, a count, tracks, a data block and fixup targets.
  - A track (0x18) consists of a first block (u32 name hash), its own pointer-array block, u16(+6) entry blocks, then +0x10 (key times, f32 0..1) and +0x14 (key values).
- Example (csdata donut1_0000): bone-like entries "Donut" and "carryhandle", each with a 0xdf0 data block and a 0x198 per-frame block. One sub-animation has 8 tracks with incrementing hashes and 10 keys of 20 bytes each.
- Added tools/convert_anims.py and tests/test_convert_anims.py (2 tests). `--census`: 2330/2330 files consume every block (376 skeletons / 5280 joints; 1954 animations / 3804 sub-animations). Python PASS 41/41.
- Not recovered: the per-bone keyframe encoding (0xdf0/0x198 blocks), the track value layout, and playback/evaluation code.
- Joint record (0x98), recovered from data:
  - +0 name pointer
  - +4 s32 parent index (−1 for the root)
  - +8 world bind matrix and +0x48 parent-relative matrix, both row-major 4×4 with the translation in the last row
  - +0x88 four floats, unknown (often 0,1,0,0,0,1)
- check_hierarchy() verifies, across all 376 skeletons / 5280 joints, that parents precede children and that world == local·parent.world (row vectors). The maximum relative error is 4.65e-5.
- Skin attribute (vertex flag 2, 8 bytes): shipped data reads as u8 weights then u8 indices (e.g. ff000000 00000000). This matches the shader inputs vWeights/vIndex and the 26-entry "Bones" uniform (regs 0x22–0x6f, 3 regs per bone). The order and the bone-palette mapping are not yet confirmed in code.
- Python PASS 42/42.

## 2026-10-12T19:05+02:00 — Animation keyframe decoding (REA, Ghidra)

- Found by searching for the `lsl #19; lsr #19` extraction of the per-joint table count (anim+8 word: (w & 0x1fff) + bit 28).
- Call chain:
  - 0x1205f8 (ev_34ecb50a…32df) is a pose cache that allocates 0x40 bytes per joint and calls the sampler.
  - 0x12a2b4 (ev_210a33ed…3420) loops over the joints, calling 0x12d4b0 (ev_c0f59246…1227) per joint. 0x12d4b0 clamps the frame (or wraps to 0) and calls the curve decoder 0x248910 (ev_6bc461f5…852c).
  - It also uses the joint record's +0x88 float4 as a default, together with optional per-joint s16/256 corrections.
- Curve format (0x248910), read with the LSB-first bit reader 0x247a48 (ev_29ed4ed7…29b6):
  - 8-bit header. 0xfa means raw f32 4×4 matrices per frame at blob+1+frame·0x40. Otherwise shift = header>>4 (bit 3 adds 2 more base bits to translation), and header bits 0–3 enable four 3-bit channel-mask groups.
  - 12 channels. Absent channels default to 1.0 for 3–5 and 0 otherwise.
  - Each present channel: a signed base (12 bits, 16 for channels 9–11; plus shift), a delta width (4/5 bits; plus shift), then one fixed-width unsigned delta per frame. value = (base + delta[frame]) · scale / 2^shift, and scale channels are clamped to ≥ 0.0001.
- Channel table: 0x376494 (data) holds the bit widths. Its scales are written by the static initializer at 0x347c5c (decoded with `llvm-objdump --mattr=+vfp2`): 0.02 for channels 0–2 (rotation, radians), 0.01 for 3–5 (scale), 6–8 (unknown) and 9–11 (translation).
- 0x248248 (ev_676da22a…9b72) converts channels 0–2 into a quaternion from half-angle sin/cos (0x249038 calls 0x247ac8 = sin by its polynomial, ev_4caeb96f…49a, then 0x247c90 = cos). The formula is replicated in euler_to_quaternion().
- Verification: char_bear idle frame 0 decodes per-joint translations equal to the skeleton's local bind translations (LeftForeArm, Head, Knees, Feet and Hands exact; Pelvis 1.344 vs 1.363, i.e. animated). Rotations are small radian values.
- Added decode_channels() and euler_to_quaternion() to tools/convert_anims.py, plus a synthetic bit-stream test. Python PASS 43/43.
- 19:15 visual test with tools/preview_model.py (`--skeleton --anim --frame [--transpose-rotation]`) on char_bear walk, frames 0/12/24:
  - Skin attribute: u8 weights (summing to 255), then u8 indices. An index is joint − 1 (the root "Character" is skipped); this is a hypothesis.
  - Local pose = S·R·T in row-vector order, with the skin matrix = inverse(bind world)·animated world.
  - With `--transpose-rotation`, the result is a natural quadruped walk cycle. Without it the bear rears up unnaturally, so the transposed quaternion matrix is the likely convention. This is a visual inference only, not traced in code.
- Not recovered: how the pose is composed into joint matrices (order of q/scale/translation and the role of channels 6–8 and joint +0x88), root motion (the extra 24th blob), and the 0xfa raw-matrix decomposition 0x1324f8.
- Named in REA with a _candidate suffix (ev_d6ee55638e4b4ce712665d235d1d66a9d8c1add97c17e5f210b671225ddb1b28): srvGetServiceHandle_candidate, strlen_candidate, dspInit_candidate, fsUserInit_candidate, ndmInit_candidate. Hopper does not define procedures at 0x109fc0, 0x10f8e4 or 0x11006c.

## 2026-10-12T20:15+02:00 — Pose composition recovered (REA, Ghidra)

- Call chain: 0x10f800 (ev_4dd35b1b…6e5) clears a blend state, then 0x2533e0 (ev_ef46d59d…730) adds weighted pose layers through 0x114b58 (ev_732ce83c…7ff, which calls the pose cache 0x1205f8). 0x2530f0 (ev_83cb1c23…509) then calls 0x252cb0 (ev_31aa99b7…9b9), which builds the joint matrices.
- Pose entry (0x40 bytes, written by 0x12d4b0): floats 0–3 quaternion, 4–6 channels 3–5, 7–9 channels 6–8, 10–12 channels 9–11, word 13 the channel mask.
- 0x252cb0, per joint:
  - With more than one layer, it blends quaternions with a sign flip on a negative dot product (0x24c924), normalizes them (0x24c898), and weight-sums the other groups.
  - It then calls 0x24c734(out, quat|0, scale|0, shear|0, translation|0). Each pointer is NULL when that mask group (bits 0–2 / 3–5 / 6–8 / 9–11) is empty, as confirmed by the call-site disassembly at 0x252f34–0x252fb4.
- 0x24c734 (ev_94f85e30…81f):
  - It starts from the rotation 0x248870 (ev_8c943203…64e), or identity.
  - If a shear is present: row2 += row1·h2, row1 += row0·h1, row0 += row2·h0.
  - It scales each row by 0x248850 (row i × s_i).
  - The translation goes into row 3; when it is absent, a vector from 0x56d400 (BSS, runtime value unknown, assumed zero) is used.
  - So channels 6–8 are a shear.
- 0x248870 is the rotation matrix of quaternion (x,y,z,w): rows [1−2(y²+z²), 2(xy−wz), 2(xz+wy)], [2(xy+wz), 1−2(x²+z²), 2(yz−wx)], [2(xz−wy), 2(yz+wx), 1−2(x²+y²)]. This is the transpose of the former preview default, which confirms in code the convention that the 19:15 visual test favoured.
- Hierarchy: world = local · parent.world (0x258a10 multiplies in place, A = A·B, ev_538f88ae…5f0). The skin matrix is rigid-inverse(bind world at joint+8) · world (0x24c5a0, ev_ba75c37d…587, which uses the transposed 3×3).
- After posing, 0x2530f0 applies optional per-joint s16/256 scales (skipped when all three are 0x100) through 0x24c2c4. This step is not replicated.
- tools/convert_anims.py: quaternion_matrix() and local_matrix(); the preview uses them, and `--transpose-rotation` is removed. tests: test_local_matrix. Python PASS 44/44.
- The char_bear walk frames 0/12 render as a natural walk.

## 2026-10-12T20:45+02:00 — Native skeletons, animation curves and skinning (S4e groundwork)

- Fixed tools/convert_models.py decode_vertices: the layout loop variable shadowed the mesh-centre argument, so Python never added the centre. The C++ parser was correct. A regression assertion was added.
- Moved posing into tools/convert_anims.py: skin_matrices() and rigid_inverse() (0x24c5a0). preview_model calls it, and char_bear walk frame 12 renders pixel-identically.
- Added src/assets/anim.cpp (chase_assets):
  - parseSkeleton
  - parseAnimation (consumes and checks every block, keeps the +0x08 per-joint curves)
  - decodeChannels (0x248910)
  - eulerToQuaternion (0x248248)
  - localMatrix (0x24c734 with 0x248870)
  - skinMatrices (single-layer 0x252cb0)
  - multiply()
  Vertex gains skin weights and joints (flag 2, after colour).
- Factored the pooled block reader and cString/readF32 into src/assets/block_reader.hpp, shared by model, level and anim.
- assets-core: synthetic bit stream, signed base, raw-matrix header, local-matrix rotation/scale/translation and shear, skeleton parse and unconsumed-block rejection, animation parse, and root/child skin matrices.
- assets-real-data: 376 skeletons / 5280 joints and 1954 animations / 30081 joint curves, equal to the Python reference. The char_bear walk skin-matrix |sum| over 48 frames is 5397.1758, within 1e-3 of Python.
- `ctest --test-dir build --output-on-failure` — PASS 7/7; 0 warnings. Python PASS 44/44.
- Still a hypothesis: skin index = joint − 1. Blending of more than one layer, the 0xfa raw-matrix form (the bind local is used instead), and the per-joint s16/256 post-scale are not replicated.

## 2026-10-12T21:30+02:00 — Bone palettes recovered; skinned playback in chase-viewer

- The vertex-buffer block at header +0x24 (previously skipped) is the bone palette table, with its count at header +0x20 (1 or 2).
  - Each 0x110-byte record is u32 triangles, u32 palette size, u32 first vertex, u32 vertex count, then u8 palette[256] of skeleton joints. At most 26 entries are used, matching the 26-entry "Bones" shader uniform.
  - Batches cover consecutive triangle runs. A first reading of the third word as "first triangle" was wrong (it failed on 7 parts, e.g. squirrel.bfnmdl 282 triangles vs 284).
- Checked over all 1457 skinned parts (data, not traced in code):
  - triangle counts sum to the index count
  - the block size is count × 0x110
  - every batch's indices stay inside [first vertex, first vertex + count)
  - no skin index is outside its palette
  - no vertex shared between batches resolves to different joints
  - 514 vertices are referenced by no triangle
- This replaces the joint − 1 hypothesis, which only matched char_bear because its palette is 1..21. The bear's eye part has palette [5] (Head). char_squirrel's palette contains joint 0.
- Python: convert_models.bone_batches() and skin_joints() (the +0x24 block is kept as buffer 'b24'); preview_model skins through resolved joints. C++: ModelPart::boneBatches, and parseModel resolves Vertex::joints to skeleton joints, rejecting conflicts and out-of-range indices. skinVertex uses the resolved joints.
- Bug fixed: the C++ rigidInverse used −t·column instead of −t·row (0x24c5a0 uses rows). Bear binds are translation-only, so the bear reference missed it; it was found through char_squirrel. Added a synthetic rotated-bind identity check, and char_squirrel run frame 5 as a second real-data reference.
- assets-real-data (Python references): 1457 skinned parts / 1465 batches / resolved-joint sum 3492824 over drawn vertices; |posed position| sums char_bear walk@12 1754.6224 and char_squirrel run@5 440.7128 (within 1e-3).
- chase-viewer: `--skeleton S.bfnskl --anim A.bfnanm [--frame N]` poses the first model argument. Skinned parts are skinned on the CPU and re-uploaded when the frame changes. The playback rate is not recovered; 30 fps is assumed.
- Offscreen screenshots: char_bear walk 0/12 and char_squirrel run 0/3 show coherent gait poses. The squirrel renders almost black; the material/combiner setup is not recovered.
- `ctest --test-dir build --output-on-failure` — PASS 7/7; 0 warnings. Python PASS 45/45.

## 2026-10-12T22:10+02:00 — Frame interpolation recovered; playback rate traced to the instance

- 0x2533e0 calls 0x24bf80 (ev_d1801bf3…488) per active animation slot (0x58-byte slots at instance+0x2c). It returns frame A, frame B and their weights. 0x114b58 is then called twice (layer 0 = A with weight 1−frac, layer 1 = B with frac), so the game always interpolates neighbouring frames. Layers of weight 0 are skipped, so whole frames use the single-layer path.
- 0x24cbd4 (ev_01398…5a6) computes frame = (clock − start) × slot+0x30 / clock+0xc + start offset:
  - The clock is 0x256af4 (ev_5678829b…3ea6), a generic timer in units of clock+0xc per second, so slot+0x30 is the playback rate in frames per second.
  - Modes are slot u16 +4 & 7: 0 clamp, 1/6 loop, 2 reverse, 3 ping-pong, 5 reverse loop. Frame B uses time + 1 frame and wraps the same way.
  - The rate is written by the caller that starts the animation. It is not in the .bfnanm header (all 1954 headers tallied; words +0x14/+0x18/+0x24 are always 0) or in options*.txt. It remains unknown.
- Multi-layer blend in 0x252cb0, from the existing pseudo-code (ev_31aa99b7…9b9) and disassembly of 0x24c898/0x24c924:
  - Quaternions are summed with weights, negated when dot(first, q) < 0, then normalized (identity when the length is ≤ 2⁻²³, bits 0x34000000).
  - Scale is summed over every layer when any layer has scale.
  - Shear and translation are summed only over the layers whose own mask has them.
- C++: JointPose, jointPose, blendPoses, poseMatrix; skinMatrices takes a double frame and blends floor/next with wrap-around. Python: joint_pose, blend_poses, pose_matrix, fractional skin_matrices; preview `--frame` accepts fractions.
- Tests: synthetic halfway translation (1.525), last→first wrap, sign flip, translation-only-from-carriers. Real-data |posed| sums at char_bear walk 47.5 (1741.8781) and char_squirrel run 2.25 (372.6377) match Python within 1e-3.
- chase-viewer: poses every rendered frame at fractional time; `--frame` takes fractions; `--fps` (default 30, an assumption) sets the rate.
- ctest PASS 7/7; Python PASS 46/46; 0 warnings.

## 2026-10-13T01:30+02:00 — Animation frame count is u16 +4 & 0x3fff

- In cutscene animations (csdata, 679 files), header u16 +4 reads 0x4000 + frames, e.g. 2_7_a: 16683 = 0x4000 + 299. The camera's per-frame block in section +0xc is 300 bytes (299 padded to 4), and the same holds for 1_1_a/d/e and 1_2_a.
- The game masks it: 0x114b58 passes `*(u16*)(anim + 4) & 0x3fff` (ev_732ce83c…7ff). Python frame_count() and C++ Animation::frames now mask it. Gameplay animations (no bit 14) are unchanged, and the bear/squirrel references still pass.
- Check: with the masked count, 29966 of 30081 joint curves fit their blob exactly. The 115 that overflow are all joint 0 with headers 0x48/0x49/0x4b/0x08, in gameplay animations (use, kick_open, idling). Their root-curve encoding is not understood; unknown.

## 2026-10-13T03:00+02:00 — First Python→C++ binding; Python posing removed (rule 6 fix)

- New shared library chase_assets_c (src/assets/c_api.cpp → build/libchase_assets_c.so), a C ABI over chase_assets: chase_skin_matrices(skeleton, animation, frame) and chase_last_error(). chase_assets is now built position-independent.
- tools/chase_native.py: a stdlib ctypes wrapper. It finds the library in build/ or at CHASE_ASSETS_LIB.
- Deleted from tools/convert_anims.py: decode_channels, Bits, euler_to_quaternion, quaternion_matrix, joint_pose, blend_poses, pose_matrix, local_matrix, rigid_inverse, frame_count and skin_matrices. assets-test covers them in C++. preview_model.py now poses through chase_native.skin_matrices, and the minifig balance_walk frame 5 renders the same pose as chase-viewer.
- tests/test_convert_anims.py: the three Python posing tests are replaced by test_native_skin_matrices. It checks the same synthetic curve as assets-test (1.55 at frame 1) and skips when the library is missing.
- ctest PASS 8/8; Python PASS 55 (1 skip when the library is absent).

## 2026-10-13T04:10+02:00 — Overflowing joint curves explained: one curve stores frames + 1

- 0x12a2b4 (ev_210a33ed…3420) passes frames + 1 to the sampler 0x12d4b0 for one curve when its flag 0x10 is set: the extra curve when flag 0x20 is set, otherwise curve 0. These flags are a runtime argument (0x114b58 → 0x1205f8, ev_34ecb50a…32df) and were not traced to the file.
- Data rule: if bit 29 of the joint-table word is set, the long curve is the extra curve (when bit 28 is set) or else curve 0. Across all 1954 animations, every selected curve fits its blob only with frames + 1 (149 need it; 441 fit either way), and every other curve fits with frames. This replaces the earlier "115 overflowing joint-0 curves" unknown.
- Animation::longCurve: skinMatrices samples that curve with frames + 1. assets-real-test pins 590 animations with a long curve. Bear, squirrel and minifig references are unchanged.
- Still unknown: what the long or extra curve means (root motion?), and the 0xfa raw-matrix form.

## 2026-10-13T15:08:53+02:00 — tools/convert_anims.py removed

- tools/convert_anims.py (`--census`, `parse_skeleton`, `parse_animation`) and tests/test_convert_anims.py were removed. The binding test for `skin_matrices` moved to tests/test_chase_native.py. The census (376 skeletons, 5280 joints, 1954 animations) and the char_bear walk sums stay valid and are pinned in tests/assets_real_test.cpp. Provenance: `python3 tools/convert_anims.py --census artifacts/fib/files` and the preview_model renders at 9d04c91. To reproduce, check out that commit (for example `git worktree add /tmp/old 9d04c91`) and run the command there.

## 2026-10-13T17:02:03+02:00 — Joint table word, playback rate and the extra (root motion) curve (see player.md derivation entry)

- The first word of the joint table block (`Animation::jointTableWord`): bits 0..12 curve count (without the extra curve), bits 13..14 rate index, bits 15..27 sample count, bit 28 extra curve present (it is the last curve, index = bits 0..12), bit 29 long curve (+1 sample). Checked on police land (0x10070015: 21 curves + extra, 14 samples), fall (0x30050015: 10 samples + long), sprint (0x200a2015: rate index 1).
- **Playback rate** (`0x108190`): `{15, 30, 60, 0}[bits 13..14]` frames per second, or header u16 +4 >> 14 when there is no joint table (cutscene animations carry bit 14, 30 fps). **Correction:** the earlier "rate unknown, 30 assumed" entries; police walk/run/jump/land/idle are 15 fps, sprint 30 fps. `animationFramesPerSecond`, `animationDuration` ((frames - 1) / rate).
- **Extra curve = root motion joint.** `0x292ef8` samples it for every frame, relative to frame 0, and `0x200e70` scales it to a target height (player.md). `rootMotion` returns the relative translation per frame and the extent with minimum and maximum starting at 0. For animations without bit 28 the game reads the root joint index from the skeleton (`0x24b8bc`); not replicated.
