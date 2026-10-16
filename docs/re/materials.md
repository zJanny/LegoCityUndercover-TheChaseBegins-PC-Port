# Materials, texture-environment stages and draw state

Reverse-engineering findings. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis. Addresses are for the Europe v1.0 build.

## Current state

- Per-draw setup is 0x128dc8 (32-bit key from the material, regenerated only when it changes), the stage generator is 0x12e460, texture binding 0x12df94, blend/alpha/depth/cull 0x1292fc, colour packing 0x12edfc. [§2026-10-13T22:24:39](#ts-20261013-222439)
- The material record is 0x60 bytes; bytes 1, 2, 3, 5 give mode, lighting, texture and alpha, byte 0x13 is the material alpha, and the texture slot roles are 0 diffuse, 1 second texture, 2 lightmap, 5 reflection cube. [§2026-10-13T22:24:39](#ts-20261013-222439)
- The census of 16122 parts in 3113 models has 38 distinct types, none unsupported. [§2026-10-13T22:24:39](#ts-20261013-222439)
- Level surfaces (lighting 1) are baked vertex colour x texture x2 with fragment primary 0; the global ambient is the per-frame sum of the type 0 lights (this corrects the single read of 0x5943d0): [rendering-lighting.md §2026-10-15T17:47](rendering-lighting.md#ts-20261015-174725). [§2026-10-13T22:24:39](#ts-20261013-222439)
- The texture alpha of Chase's face is ignored by the combiner, so the eyes are drawn, not discarded. [§2026-10-13T22:24:39](#ts-20261013-222439)
- Lightmap surfaces match the oracle on the police-station lobby wall (the floor is about 8 percent lighter in the port). [§2026-10-14T09:50:11](#ts-20261014-095011)
- The white quad is the `Checker1.tga` part of room1 (also room3, room5): flat y = 0, 4.2 x 4.2, opaque. [§2026-10-14T09:50:11](#ts-20261014-095011)
- Open: whether the original draws the `Checker1.tga` quads, texture-unit to UV-set mapping, wrap flags, cube sampling, fog conditions, colour pulse over time, draw sort order 0x115128.


<a id="ts-20261013-222439"></a>

## 2026-10-13T22:24:39+02:00 — Material record to PICA texture-environment stages (REA Ghidra 12.1.2, llvm-objdump, oracle reads)

Corrected by [rendering-lighting.md §2026-10-15T17:47](rendering-lighting.md#ts-20261015-174725): the global ambient is the per-frame sum of the type 0 lights (lvl_01 (136, 159, 179) / 255, register 0x1c0 = 0x08827cb3), and the fragment lighting is recovered.

**Where the setup is** (all in `.code`, written straight into the GPU command list at `*0x37614c`)
- 0x128dc8 (ev_0ff86efc…): per-draw setup. Builds a 32-bit key from the material, skips regeneration when it equals the last key (`0x377974`), and calls the stage generator for modes below 9. Key fields: byte 1 (bits 0-4, "mode"), byte 2 & 0xf (bits 5-8, "lighting"), byte 3 & 0xf (bits 9-12, "texture"), byte 5 & 7 (bits 13-15, "alpha"), byte 10 bit 6 (vertex colour), byte 0xc bit 6 (material colour), byte 0xd bit 1 (bump). Modes 12 and 13 use a precomputed table at 0x5957f8 (0x94 bytes each; not used by any shipped model).
- 0x12e460 (ev_f3f2e9e9…): the stage generator. Each stage is a 6-word record from 0x24c56c (ev_304505a2…): SOURCE, header (register | 0x804f0000, five consecutive registers), OPERAND, COMBINER, COLOR, SCALE. Stage registers come from the table at 0x379940 (0xc0, 0xc8, 0xd0, 0xd8, 0xf0, 0xf8). Stages after the first get the alpha source "previous" (|0xf0000).
- 0x12df94 (ev_be0109a5…): binds the textures. Unit k takes material slot k, or slot count-1-k when 0x37796d is set. 0x37796d is set in 0x1292fc's caller (0x129800 region) when byte 3 is 5, 6 or 7 (reflection materials).
- 0x1292fc (ev_9610b8db…): blend, alpha test, depth and cull from the material.
- 0x12edfc (ev_2c8ead7c…): packs the material colour (floats at +0x30..+0x38, alpha = +0x3c with a pulse from +0x40/+0x44 and the game clock) to RGBA8.

**Material bytes** (record of 0x60 bytes, `ModelPart::material`; texture slot records of 8 bytes, byte 7 = role, `ModelPart::textureSlots`)
- 1 mode, 2 lighting, 3 texture, 5 alpha, 6 alpha-test reference, 7 blend equation (0-4), 8 and 9 blend source and destination (index into 0x37628c: 0 zero, 1 one, 2 src colour, 3 1-src colour, 4 src alpha, 5 1-src alpha, 6 dst colour, 7 1-dst colour, 8 dst alpha, 9 1-dst alpha), 10 low 3 bits UV sets, bits 3-5 texture count, bit 6 vertex colour, 0xb low 3 bits depth function index (table 0x3762b4; 6 means no depth test), bit 3 depth write, bit 4 alpha test (greater than byte 6), bit 5 x2 scale of the last stage, 0xc bits 0-1 cull (0 culls counter-clockwise, 1 clockwise, 2 none), bit 6 material colour, 0xd bit 1 bump, 0x13 material alpha.
- Slot roles (byte 7): 0 diffuse, 1 second texture (specular mask, or lightmap in mode 1), 2 lightmap (lighting 4), 5 reflection cube (no TEV source of its own; texture 5/6/7 use Texture0).

**Stages** (source values: 0 vertex colour, 1 fragment lighting primary, 2 secondary, 3.. texture units, 0xd previous buffer, 0xe constant, 0xf previous). D, L, E are the unit of the diffuse, second and third role.
- Mode: 0 `D`; 1 modulate(L alpha, vertex alpha) then lerp(D, L, that); 2 D + L; 3 constant white.
- Lighting: 0 previous x vertex colour; 1 (vertex colour + fragment primary) x previous (add-multiply); 2, 5, 6, 7, 9 fragment primary x previous; 3 (E + vertex colour) x previous; 4 buffer holds the base, then (E + vertex colour), then (that + fragment primary) x buffer; 13 none. Bump (0xd bit 1) turns the last stage into replace/add and appends three dot3 stages with constants 0x7f7f7f and 0x8ecba6.
- Vertex colour bit: one more stage previous x vertex colour. Material colour bit: lerp towards the constant by its alpha.
- Texture: 1, 2 add fragment secondary x diffuse alpha; 5 lerp(Texture0, previous, diffuse alpha); 6 Texture0 x diffuse alpha + previous; 7 three stages (Texture0 squared, x diffuse alpha x2, x vertex colour + buffer).
- Alpha: stage 0 constant alpha = byte 0x13; alpha 0: vertex alpha x constant; 1, 6 diffuse alpha x constant; 2, 7 diffuse alpha x constant x vertex alpha (extra stage when needed); 3 (diffuse + vertex alpha) x constant; 4 and 5 as 1 and 2 with the second texture.
- The last stage's RGB scale is x2 when byte 0xb bit 5 is set (all level surfaces with 0x28/0x38). Register 0xe0 buffer bits: bit n set means the buffer takes the output of stage n-1 before stage n.
- Fragment lighting registers (light setup 0x12d884, ev_55882372…): per light a diffuse/ambient colour from the light object (+0x00 colour, +0x10 second colour, +0x24 intensity, +0x44 direction) times material colours (+0x10, +0x14), global ambient from 0x5943d0. The light list at 0x5943b0 is reset each frame (0x10c8a0), so reads between frames show it empty.

**Census** (`chase-materials`, all 16122 parts of 3113 models; none unsupported; 38 distinct types by mode/lighting/texture/alpha/texture count/vertex colour). Largest: 5012 mode 0, lighting 2, texture 1 (lit props with vertex colour and a specular mask); 3186 lighting 1 (baked level surface); 2094 lighting 1 alpha 1; 1181 lighting 2 texture 1; 1164 mode 1 lighting 1; 711 lighting 0; 218 lighting 4 (lightmap); 242 texture 5; 44 texture 7. The full list is pinned in `assets-real-data`.

**Oracle measurement** (gameplay-start, OpenGL screenshot via RPC, same camera as `chase-game --facing 225.35`)
- Level surfaces (lighting 1) match the oracle with fragment primary 0 (mean colour difference 7 to 11 on road crops, was 64): baked vertex colour x texture x2.
- Light object at 0x08e0b380 (read after patching the reset 0x10c8a0 to return): colour (0.996, 0.819, 0.663), second colour 0.498 grey, type 1, intensity 1, direction (-0.650, 0.623, -0.433). Global ambient at 0x5943d0 read (138.1, 161.5, 181.8) at one moment; an earlier read gave about 4 to 5.6, so it changes. Using (138, 161, 182) / 255 as the fragment primary of lit props and characters gives a mean colour difference of 0.8 on the player crop.
- Chase's face material (char_head_chase, two parts): mode 0, lighting 2, texture 1, alpha 0, no alpha test, blend one/zero. The texture alpha is ignored by the combiner (alpha = vertex x constant); 66 face texels have alpha 0. The earlier viewer shader discarded alpha below 0.25, so the eyes were holes. With the recovered setup the eyes are drawn.

Unknowns:
- Fragment lighting itself (light list per draw, light mask, LUT configuration, specular), so fragment primary and secondary are stand-ins.
- Vertex shader colour output when the vertex has no colour attribute (white assumed), `VColourTint` and `MaterialColour` uniforms.
- How texture units map to texture coordinate sets (unit 0 uv0, others uv1 when present: assumed); texture wrap flags in slot bytes 4-5 other than repeat; cube texture sampling (reflection vector not recovered).
- Fog (byte 9 equals 1 disables it; 0x3760fc and 0x3760fd conditions), the material colour pulse over time, depth write and colour mask details of register 0x107, draw sort order (0x115128).
- Writers of 0x37796c (shadow texture) and 0x530834 (override flags); assumed 0 in the main pass.

<a id="ts-20261014-095011"></a>

## 2026-10-14T09:50:11+02:00 — Police station lobby against the oracle (RENDER-4) and the white quad (RENDER-6)

Pictures (local, `artifacts/oracle/agentB-out/`): `ps0.png` (oracle, state police-station, Azahar New 3DS, OpenGL RPC screenshot, player at (-1.970, 1.436, 14.843), camera yaw pi) and `cg1.png` (`chase-game --gameplay --start "-1.970 1.436 14.843" --facing 180 --frames 200 --window 400x480`).
- Lightmap surfaces (lighting 4, `LIGHTMAP_lvl_00_PoliceStation_Lobby000`): the brown stair wall crop matches (mean RGB oracle 79.6 / 71.0 / 70.5, chase-game 80.6 / 71.5 / 70.8). The lobby floor is about 8 percent lighter in chase-game (oracle 89.5 / 84.1 / 98.9, chase-game 97.9 / 91.3 / 108.8; crop positions differ slightly because the cameras differ).
- Differences: (1) the camera: the original station uses the record pose camera 0x298cd8, so its view is higher and further back (see camera.md 2026-10-14T09:49); (2) the police pillars and other lit props are darker in chase-game (RENDER-S1, RENDER-1/5); (3) the HUD matches; (4) the original shows a crossed-out camera icon when L or R is pressed.
- RENDER-6: the white quad is the part with the texture `Checker1.tga` of `room1.bfnmdl` (and one in `room3`, `room5`): 4 vertices, 6 indices, flat at y = 0, 4.2 x 4.2 units (room1 x -2.10..2.10, z 47.26..51.46), material lighting 1, opaque, depth test and write on, no blend, no lightmap. Hiding the model part by texture name removes it from the picture (verified), hiding the other models does not. So it is not a blended part drawn out of order (not RENDER-S3). Whether the original draws it is unknown: teleporting the player to z = 56 or 40 in the oracle left the player in the void (those rooms are not loaded there), so no picture of that spot exists.

Unknowns:
- Whether the original draws the `Checker1.tga` quads (placeholder markers); chase-game draws every room at once (WSTREAM-3).
- The lobby light on lit props (RENDER-1).

<a id="ts-20261014-134000"></a>

## 2026-10-14T13:40:00+02:00 — Details moved from feature items RENDER-6

Moved from the feature files so that the items stay short (CLAUDE.md, "Documentation rules"). The text below is the former wording of each item.

- **RENDER-6** (rendering.md): The white quad above the police-station entrance is the `Checker1.tga` part (4 vertices, flat y = 0, 4.2 x 4.2, opaque) of `room1.bfnmdl` (also room3, room5), seen far away outside; not RENDER-S3. Unknown whether the original draws it (rooms outside are not loaded in the oracle). Not fixed (related: WSTREAM-3). evidence: [materials.md §2026-10-14T09:50](#ts-20261014-095011).
