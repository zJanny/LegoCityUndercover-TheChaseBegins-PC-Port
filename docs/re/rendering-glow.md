# Glow: the post-process 0x2bf830

Reverse-engineering findings. Entries are chronological. Addresses are for the Europe v1.0 build. Feature file: [rendering](../plans/features/rendering.md). Related: [abilities-scanner.md](abilities-scanner.md), [materials.md](materials.md).

## Current state

- 0x2bf830(p1, p2, blend destination, mask, colour) copies the 3D screen to a 256 x 128 target, to a 128 x 64 target, blurs that three times (vertical table mode 0xc, then horizontal mode 0xd, each iteration with a smaller offset) and adds the blurred copy back, `(a x T.a + c) x T x T` with the doubled scale for the scanner call, with table mode 0x12 and blend one / one. [§2026-10-19T14:44:55](#ts-20261019-144455)
- It runs when p1 is not 0, when 0x52ff94 is not 0 (reader 0x1a3888) or when a mask texture with a non-zero alpha is given. Two callers: pass 5 of the frame function 0x2e4858 (scanner view: the literals (0.6, 1.4); otherwise (0x516410, 0.7)) and pass 5 of the frame function 0x303e78 (the globals 0x37594c / 0x375950, read (0.0, 0.6) in the scanner view, so off there). [§2026-10-19T14:44:55](#ts-20261019-144455)
- Oracle: poking the literal to 0 turns the glow off; the glow adds about 0.6 x T squared per channel (T the blurred screen) and does not depend on the second strength. [§2026-10-19T14:44:55](#ts-20261019-144455)
- Port: `src/runtime/glow.*` (parameters), `src/render/glow.*` with `glow.vert.glsl` / `glow.frag.glsl` (the pass), glue in `LevelSession::draw`; test `glow`. The mean glow delta of the scanner picture is (6.3, 48.2, 21.0) in the oracle and (6.0, 49.6, 23.0) in the port. [§2026-10-19T14:44:55](#ts-20261019-144455)

<a id="ts-20261019-144455"></a>

## 2026-10-19T14:44:55+02:00 — 0x2bf830: downsample, blur tables and the final add (REA, llvm-objdump, oracle m12-8-roof-sound-scanner)

### Method

REA (Ghidra 12.1.2) pseudocode of 0x2bf830, 0x23c3f4, 0x128dc8, 0x1f46a0, 0x1f45d0, 0x254350, 0x12d780, 0x1a3888 and 0x303e78; callers by REA and a search of the llvm-objdump listing; the stage-table layout read from 0x128dc8 (modes 9 and above copy `count x 0x18` bytes from `0x5957fc + mode * 0x94`). Oracle: state `m12-8-roof-sound-scanner` entered with A (OpenGL, `--rpc --rpc-input`, port 45652): `read` of the tables below, then screenshots with the glow on, with the literal 0x2e50c8 (p1) poked to 0, and with 0x2e50c4 (p2) poked to 0, 0.7 and 1.4 (p1 = 0.6). Screenshots are in the agent's scratch folder, not committed.

### The passes

- Texture headers (u16 width, u16 height first): 0x52a698 256 x 512 (the 3D screen buffer, the 400 x 240 screen rotated), 0x52a6b8 128 x 256, 0x52a6d8 and 0x52a6f8 64 x 128 (read in the oracle). The quads of 0x2bf830 are drawn in screen orientation, so the first target is 256 (x) x 128 (y), the blur targets 128 x 64: the quad of the first copy has x in 0..256, y in 0..128 with u in 0..0.9375 (240 of 256) and v in 0.21875..1 (400 of 512).
- Copy 1: 0x1f46a0(0x52a6b8) sets the target, a default material (0x254350: mode 0, lighting key 0 = modulate by the vertex colour, alpha key 1, blend source one, destination zero) draws the screen texture with vertex colour white: a plain bilinear copy. Copy 2: target 0x52a6d8, source 0x52a6b8, 0x1f45d0(0xffffffff): a plain bilinear 2 x 2 copy to 128 x 64.
- Loop i = 0..2: material mode 0xc with three texture units on 0x52a6d8 into 0x52a6f8, then mode 0xd on 0x52a6f8 into 0x52a6d8. Per iteration the material holds offset 4.5 - 1.5 i (+0x30) and weight 1.5 - 0.28 i, at least 1.0 (+0x3c). 0x128dc8 turns them into a vertex shader constant (shader program 2, 0x12d780(2)): offset divided by the texture's first dimension (mode 0xc, 64) or second (mode 0xd, 128), and into the alpha of the stage-0 constant `(w x 128 + 0.5) << 24 | 0x808080`.
- Stage tables read in the oracle (each stage: source, header 0x804f00c0 + 8 n, operand, combiner, constant, scale): modes 0xc and 0xd are identical: stage 0 source 0x00e300e3 (texture 0 x constant), combiner 0x00010001 (modulate), constant 0x80808080 (alpha replaced as above); stage 1 source 0x0fe40fe4 (texture 1 x constant + previous), combiner 0x00080008 (multiply-add), constant 0x40404040; stage 2 source 0x0fe50fe5 (texture 2), the same combiner and constant; count 3. So colour = 0.502 T0 + 0.251 T1 + 0.251 T2. The vertex shader that places texture 1 and 2 (program 2) was not decoded; the port samples at +offset and -offset.
- Mode 0x12 (0x596260): count 3; stage 0 source 0x30 (vertex colour, texture 0), operand 0x22, combiner 8 (multiply-add): `colour.a x T.a + colour.rgb`; stages 1 and 2 source 0x000f003f, combiner 1: times T each. The final stage scale is 2 when the strengths add up to more than 1 (local_175 bit 5, flag in 0x2bf830).
- Final draw: the strengths are halved when p1 + p2 > 1 (scanner: 0.3 and 0.7, bytes 77 and 179 = `(s x 255 + 0.5)` clamped), `0x1f45d0(r = g = b = byte(p1), a = byte(p2))` draws the full-screen quad with the material of mode 0x12 on 0x52a6d8, blend source one (material +8 = 1) and destination `param_3` (the 3 = 1 of both callers: one). With a mask texture (param 4, not given by either caller) a second quad with the mask is drawn first.
- Strength arguments: 0x2e4858 passes the literals 0x2e50c8 = 0.6 (p1) and 0x2e50c4 = 1.4 (p2) in the scanner branch (read from the ELF and in the oracle), and `[0x516410]` and the literal 0x2e50cc = 0.7 otherwise; the blend destination is `[0x516414]` (1). 0x516410 = 0.0 and 0x52ff94 = 0 in the scanner view. Nothing in the listing writes 0x516410 through an absolute reference (the one literal reader is 0x2e4f18); its writer is not found.
- Second caller: 0x303e78 (a frame function with the same pass switch) calls 0x2bf830(*0x37594c, *0x375950, 1, 0, 0) in its case 5; the globals read 0.0 and 0.6 in the oracle in the scanner view, so the glow is off for that caller there. Which mode it belongs to is not identified.

### Oracle pictures (m12-8, top screen, 400 x 240, overlay included)

- Glow on against off (poke of 0x2e50c8 to 0): mean per channel 73.5 / 225.8 / 240.7 on and 67.2 / 177.6 / 219.7 off, mean delta (6.28, 48.21, 21.0). Setting p2 to 0, 0.7 or 1.4 with p1 = 0.6 gives means within 1.1 of each other (73.3 / 225.3 / 241.7, 73.4 / 225.6 / 240.9, 73.4 / 225.9 / 240.1): the second strength has no visible effect, so the blurred target's alpha is 0 and the add is `0.604 x T squared` (2 x 77 / 255). The regression of the green delta against the screen blurred with sigma 10 gives a ratio to T squared of 0.52 to 0.60 for T between 0.3 and 0.8.
- Port (`chase-game` at the m12-8 spot, update 243, the zone forced usable for the picture, window 400 x 480): on (67.0, 224.7, 241.8), off (61.0, 175.1, 218.8), mean delta (5.95, 49.6, 23.0) against the oracle's (6.28, 48.21, 20.98). Sky pixel (20, 50): oracle off (66, 224, 255) and on (71, 255, 255), port off (66, 224, 255) and on (71, 255, 255); before the glow the port gave (64, 218, 255) against (74, 255, 255). The remaining differences of the means come from the picture without the glow (the overlay and the character of the original), not from the pass.

Unknowns:
- The vertex shader of program 2 (the texture coordinates of units 1 and 2 for modes 0xc / 0xd; the port assumes uv, uv + offset, uv - offset), the texture wrap mode of the blur sources (the port clamps to the edge) and the format of the targets (the port uses RGBA8).
- Why the blurred alpha is 0 (the port takes 0 from the oracle pictures).
- The writer of 0x516410 and 0x52ff94 and the mode that owns 0x303e78.
