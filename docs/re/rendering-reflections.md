# Reflections: cube maps on shiny materials

Reverse-engineering findings. Entries are chronological. Addresses are for the Europe v1.0 build. Feature file: [rendering-reflections](../plans/features/rendering-reflections.md). Related: [materials.md](materials.md) (material bytes, stages), [rendering-lighting.md](rendering-lighting.md), [models.md](models.md) (the shader blob), [textures.md](textures.md) (cube container).

## Current state

- A material with texture byte 5, 6 or 7 sets the reflection flag 0x37796d in the material setup 0x129560; the flag picks the cube path of the vertex program and makes the texture binding 0x12df94 bind the slots in reverse, so the cube map (slot role 5) is texture unit 0. [§2026-10-27T18:36:08](#ts-20261027-183608)
- The vertex program's cube subroutine reflects the view vector at the normal in object space and turns it with the model matrix (`TexTrans` = the transposed model matrix, uploaded by 0x24a314): the lookup direction is the world-space reflection vector. [§2026-10-27T18:36:08](#ts-20261027-183608)
- The six faces of a cube texture are stored one after the other and go to the PICA face address registers 0x85..0x8a in stored order. [§2026-10-27T18:36:08](#ts-20261027-183608)
- Census: 354 parts are reflection parts, all with a cube map on unit 0; three cube maps are used (`cubemap_abstract_c`, `cubemap_showroom_c`, `cubemap_vehicle_windows_c`, RGBA8, 128 x 128 faces). [§2026-10-27T18:36:08](#ts-20261027-183608)
- A first oracle picture (traffic van from behind) shows no reflection on the body panels, which agrees with the data (only the windscreen reflects); a reflecting surface seen from the front is still to be compared. [§2026-10-27T18:48:32](#ts-20261027-184832)
- Open: the oracle look comparison, the filter and wrap bits of the cube unit, and whether `TexTrans` can stay stale when the matrices are not dirty.

<a id="ts-20261027-183608"></a>

## 2026-10-27T18:36:08+01:00 — Cube sampling: reflection flag, vertex program, face registers (llvm-objdump, PICA200 disassembly of the shader blob)

Method (20261027-graphics-C): llvm-objdump (`--triple=armv6k --mattr=+vfp2`) of `artifacts/game/game.arm.elf` at 0x129560..0x129bd0 (material setup), 0x12df94..0x12e3c0 (texture binding), 0x24a314..0x24a504 (per-draw matrix upload), 0x250ed0, 0x2487d8 and 0x2536c0; the bytes of the tables 0x3798e3 and 0x3798e9 in `code.bin`; a scratch PICA200 decoder (not committed) over the DVLB at `code.bin` 0x2779a0 (`tools/dump_dvlb.py` lists its programs and uniforms). REA was not used. Census with a throwaway program over `parseModel`, pinned in CTest `reflection-real-data`.

**Program choice and the reflection flag (0x129560).** For modes below 9 the program index is the byte at 0x3798e9 + lighting byte (lighting 0, 3, 13, 15: 0; 1, 2, 4, 9: 1; the others 11, for which 0x129560 returns 0; that path was not followed); when that is 0 and the texture byte is 5, 6 or 7 the index becomes 2. Program records are 212 bytes at 0x595410. The reflection flag 0x37796d is set for texture bytes 5, 6 and 7 (0x1297a8) and cleared when the program has no `Cubemap` uniform. The vertex shader bool register 0x2b0 gets: `Skinned` (material byte 0 is 1, 3 or 6), `Cubemap` = the reflection flag, `Shadows` (shadow flag 0x37796c, only without the reflection flag), `SimpleTex` when neither, `VColourTint`.

**Vertex programs.** Program 0 (unlit) and program 1 (lit) call the cube subroutine at code word 0x39 when `Cubemap` (b2) is set; program 2 (unlit, with `vNormal`) always calls it. Program 1 scales the normal by 1/127 (constant c16.z). The subroutine (r0 = object-space position after skinning, r1 = normal):
- r2 = `ObjspaceCamPos` - r0, then r1 and r2 normalised (rsq);
- r2 = r2 - 2 r1 (r2 · r1): minus the reflection of the eye ray;
- texture coordinate 1 = uv0 x `UVScales`.xy + `UVOffset`.xy, texture coordinate 2 = uv1 x `UVScales`.zw + `UVOffset`.zw;
- texture coordinate 0 (s, t) and 0w (r) = the dot products of the three `TexTrans` rows with -r2.
The `SimpleTex` subroutine (0x35) gives unit 0 uv0 and unit 1 uv1 instead; so with the cube, the diffuse texture on unit 1 samples uv0.

**Uniforms.** `ObjspaceCamPos` = 0x2536c0(camera position +460 of 0x529e30, model matrix +608): (eye - row 3) · transposed rows, the eye in object space. `TexTrans` for the cube: 0x24a314 at 0x24a4ac uploads 0x2487d8 (transpose) of the model matrix +608 when the reflection flag is set; 0x129560 leaves `TexTrans` alone then (0x129a94). The model matrix +608 is the one 0x24a314 multiplies with the view (+672) and projection (+736) into `ViewComposite`, so the lookup direction is the reflection vector turned into world space by the model matrix. In GL terms: R = mat3(model) · (2 n (v · n) - v) with v, n the normalised object-space eye direction and normal.

**Binding (0x12df94).** Units run over the material slots from count - 1 down when the flag is set. A texture whose object byte +0xb (cube flag) is set is skipped (the unit stays off) while the flag is clear; otherwise the unit parameter gets bit 28 (texture type cube), and after the parameter block 0x81..0x85 (address of face 0 in 0x85) the faces 1..5 go to 0x86..0x8a (0x12e260..0x12e2a0), each one face size further on. The filter bits come from slot byte 6 through the table 0x379908 (not decoded further).

**Stages.** The cube colour is Texture0 of the stages of texture bytes 5, 6 and 7 ([materials.md §2026-10-13T22:24:39](materials.md#ts-20261013-222439)): 5 lerp(cube, previous, diffuse alpha), 6 cube x diffuse alpha + previous, 7 cube squared x diffuse alpha x 2 + buffer. On the police car the windows are texture 5 with `cubemap_vehicle_windows_c` (sky and clouds).

**Census (all 3113 models).** 354 reflection parts: texture 5 with lighting 1 (3) or 2 (276), texture 6 with lighting 0 (6), texture 7 with lighting 2 (69); each has two textures, normals, one UV set, the cube map (role 5) on unit 0 and a 2D texture on unit 1. 8 parts outside reflection materials (`prop_water_cannon_bi_tank`) carry `cubemap_vehicle_windows_c` as their second texture; their unit 1 is switched off and no stage reads it. No reflection part lacks a cube map, so the average colour of the earlier stand-in is no longer needed anywhere. Cube faces decoded top-down: in `cubemap_showroom_c` faces 0, 1, 4 and 5 show walls with the ceiling at the top, face 2 the ceiling lights and face 3 the floor, which fits +X, -X, +Y, -Y, +Z, -Z with the top row at t = 0.

**Port.** `runtime::unitSampling`, `unitUvSet`, `cubeFace` (src/runtime/reflection.cpp), the cube upload in src/render/reflection.cpp, the vector in `scene.vert.glsl` (`cubeOn`) and the cube lookup of unit mode 2 in `scene.frag.glsl`. CTest `reflection-core`, `reflection-real-data`.

Unknowns:
- Which direction each face register stands for and the per-face coordinates follow the PICA convention as Citra and Azahar emulate it (the GL cube map table); not measured on a console or in the oracle yet.
- The filter and wrap bits of the cube unit (slot byte 6, table 0x379908); the port uses linear filtering with mipmaps and clamp to edge.
- 0x24a314 returns early when no matrix is dirty (+0x13c bits 5..7); whether `TexTrans` can then still hold the value of an earlier draw was not checked.
- The look against the oracle (request 20261027-graphics-C-1).

<a id="ts-20261027-184832"></a>

## 2026-10-27T18:48:32+01:00 — Oracle pictures of a van from behind (RPC 20261027-graphics-C-1)

Method: oracle answer from the servant (round 20261027-graphics, agent 20261027-graphics-servant, 2026-10-27, recorded from a save state, RPC channel, OpenGL): the player drives a traffic van (`veh_van_dulmen`, blue body, grey roof) after mission 1-1; two loads, `wait 1`, screenshot, R held 0.5 s, `wait 2`, screenshot. Pose (both loads): player node (613.370, 17.072, 1062.515), van forward (-0.199, -0.002, 0.980), camera eye (615.916, 22.784, 1047.937), look (613.3692, 20.2841, 1062.5164), FOV 40; after R the yaw went from -0.173 to about -0.12 and the eye moved by up to about 1 m in x.

Picture (described): the van is seen from behind and above, about 100 pixels wide on the 400 x 240 top screen. The rear door panel is dark grey inside a blue frame, the roof light grey; no sky-like reflection is visible on the body panels or the rear at this size. In the model data only the part with `WindScreen.tga` (texture 5 with `cubemap_vehicle_windows_c`) is a reflection part; the windscreen faces forward and is not visible from this camera.

Unknowns:
- The look of the reflection itself is still not compared: it needs a picture of a reflecting window or prop seen from the front (for example a police car parked in the open, which needs a user save state) and `chase-game` at the same pose. RENDER-75 stays open.
