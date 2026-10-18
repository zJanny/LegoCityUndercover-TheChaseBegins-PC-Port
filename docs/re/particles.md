# Particle definitions

Reverse-engineering findings for `particles/particledefs.bprt`. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis.

## Current state

- `particles/particledefs.bprt` is read by 0x27d71c: six header words, 97 texture names and a block of 298 records of 380 (0x17c) bytes. Word 1 is the particle limit (25, so 24 slots), words 4 and 5 are the near fade start (0) and scale (2.328) of the draw; word 0 (10) is replaced by the game with the emitter limit 25 (`*0x376b84` in the oracle). [§2026-10-13T09:00](#ts-20261013-0900), [§2026-10-18T09:40](#ts-20261018-094004)
- Names hash with PJW over the lower-cased name (0x1886f0); at load, record +0x2c becomes the index into the loaded texture table (0 when no match; all 298 match). [§2026-10-13T09:08](#ts-20261013-0908)
- The record has four key lists of (f32 time, 4 value bytes), at most 7 keys: emitter fade (+0x88), size (+0xc0), colour (+0xf8), particle alpha (+0x130); counts at +0x170 to +0x173. [§2026-10-13T09:07](#ts-20261013-0907)
- +0x28 is particles per second, +0x7c the lifetime range, +0x38 the emitter duration (98 and above do not age: 197 endless emitters, all 100), +0x175 the particle count. [§2026-10-18T09:40](#ts-20261018-094004)
- +0x00 is the emitter axis, +0x14 the start speed and +0x78 the end speed, +0x6c / +0x70 the sway, +0x4c / +0x50 the quad pivot, +0x177 the blend, +0x178 and +0x179 the atlas columns and rows, +0x176 the frame mode (0 whole, 1 random cell, 2 animated by +0x17b loops, 3 next cell), +0x17a the flip modes. [§2026-10-18T09:40](#ts-20261018-094004)
- Start entry 0x23de24 (spawner 0x216244), stop 0x23ddb4, update 0x1ece0c (2 sub-steps in the game loops), init 0x222550 (LCG at E+0x134), step 0x221e9c, quad builders 0x2b5970 / 0x2b5f54 of the draw 0x2722cc, the emitter and particle layouts and the draw state (blend by +0x177, depth less or equal, no depth write) are read; the init matches an oracle emitter bit for bit. [§2026-10-18T09:40](#ts-20261018-094004)
- Callers: about 70 sites of 0x23de24; the level object class `particles` (constructor 0x1a7f30, handlers 0x1a8394 and 0x1a7d30) and the vehicle smoke 0x22a124 are identified, none is ported. [§2026-10-18T09:40](#ts-20261018-094004)
- The decoder is `parseParticleDefs` and `pjwHash` in `src/assets/particles.cpp`; the runtime is `src/runtime/particles.*`, the drawing `src/render/particles.*`.
- Open: the room and parent node rules of the spawner, the bounds and culling, E+0x148 / +0x160 / +0x164, the combiner of the draw, the draw order and the world matrix of the view depth, flags 0x800 and 0x20000, the update and step against a recorded young emitter.

<a id="ts-20261013-0900"></a>

## 2026-10-13T09:00+02:00 — particledefs.bprt container (REA Ghidra 12.1.2; read-only)

Corrected by [particles.md §2026-10-13T09:05](#ts-20261013-0905): +0x88..+0xf7 is a fade key list, not colour keys

- Loader 0x27d71c (ev_ed8196cc…) opens "Particles/ParticleDefs.bprt" (pooled resource, see pooled-resources.md) and reads, in this order:
  - Pool −15: six u32 blocks. Word 0 → 0x376b84 (10; the block pointer is also kept at 0x376bc4). Word 1 → 0x376b88 (25). Word 2 = texture count N (97). Word 3 → 0x376b90 (298). Word 4 → 0x376bbc (0). Word 5 is a float → 0x376bc0 (2.328); 0 becomes 1.0.
  - Pool −16: N texture-name blocks.
  - Pool −14: one data block → 0x376b8c (113240 bytes).
- 0x271e6c (ev_312eca7f…) loads each name as "Particles/<name>.tga". It stores the name hash 0x1886f0 in a parallel table at *0x376bb4. A name whose texture is missing is dropped, and the loop bound shrinks with it.
- Hash 0x1886f0 is PJW/ELF over the ASCII-lower-cased name, with signed bytes: h = (h << 4) + c; when bits 28–31 are set, h ^= high >> 24 and those bits are cleared.
- The data block is exactly 298 × 380 bytes, and word 3 is 298. The 380-byte record size is inferred from that. Record +0x2c equals the PJW hash of a texture name in all 298 records.
- Record fields, observed only:
  - Many float pairs look like min/max ranges (+0x0c/+0x10, +0x14/+0x18, +0x1c/+0x20).
  - +0x90/+0x94 and +0x138..+0x144 hold (float, u32) pairs that look like timed colour or alpha keys (see the 09:05 entry: +0x88.. are emitter fade keys).
  - +0x104 holds colour-like words (0xff666666).
  - +0x168..+0x178 hold small packed values.
- Spawners address definitions by index. 0x163f00 (ev_f0d2eaec…) reads the float attribute "ExplosionParticleDef" as an index for 0x23de24. It also compares word 0 >> 1 with a count from 0x288744, so word 0 may be a capacity (inferred).
- Implemented: chase::assets::parseParticleDefs and pjwHash (src/assets/particles.cpp). Synthetic tests cover the format. Real-data totals (97 textures, 298 records, all hashes resolved, FNV-1a of the records) come from the tools/tt_resource.py parse.
- Unknown: the meaning of every record field except +0x2c; header words 0, 1 and 4; the float in word 5. Next step: field use inside the spawner 0x23de24 and the emitter update.

<a id="ts-20261013-0901"></a>

## 2026-10-13T09:01+02:00 — Spawner field use (REA Ghidra 12.1.2 ev_a61454c9…, ev_5f3e0e6f…; read-only)

Corrected by [particles.md §2026-10-18T09:40](#ts-20261018-094004): header word 0 is replaced by the game with 25 (the emitter limit)
Corrected by [particles.md §2026-10-13T09:03](#ts-20261013-0903): +0x178 is the atlas column count, not a spawn mode
Corrected by [particles.md §2026-10-13T09:05](#ts-20261013-0905): +0x88..+0xf7 is a fade key list, not colour keys

- 0x23de24 (ev_a61454c9…) indexes the data block with stride 0x17c (380) and rejects indices ≥ word 3. This confirms the record size and the count.
- Spawner 0x216244 (ev_5f3e0e6f…; Ghidra shifts the parameters by one, so the definition is its param_2):
  - It spawns only while live emitters (0x376b6c) < word 0, so word 0 = emitter limit (10). It returns nothing when record +0x178 is 0.
  - +0x175 (u8) is the particle count. When +0x28 ≠ 0 it is capped at (int)(+0x7c × +0x28) + 1, and then at word 1 − 1 (word 1 = 25). The emitter allocates 0x168 + count × 0x44 bytes ("geParticle").
  - +0x84 is a seed; 0 takes a random one from 0x25b2b8.
  - +0x168 bit 3 replaces the spawn z with −z.
  - +0x60, when non-zero, times global 0x376b94 gives a scale s. s multiplies +0x0c, +0x10, +0x14, +0x78, +0x1c, +0x20 and +0x3c when stored in the emitter.
  - +0x24 is copied unchanged. +0x44 − +0x40 is stored (a range).
  - +0x8c (u8) / 255 is passed to the per-particle init 0x222550.
  - +0x74 > 0 casts a ray 1000 units down (0x22f978, up to 3 hits) and pushes the emitter bounds above the hit (ground collision; inferred).
  - At run time +0x2c is used as an index into the texture table *0x376b78; texture type byte +8 == 3 gets extra setup (0x24d808). The file stores a hash, so a conversion from hash to index must happen after loading. That conversion was not found.
- Inferred names: +0x28 emission rate (per second) and +0x7c lifetime (seconds). The rate × lifetime cap fits these meanings. Data: +0x28 is 0 in 45 records and +0x7c is most often 1.0 or 0.5.
- Data census (298 records): +0x178 ∈ {1..5, 8}, never 0. +0x175 ∈ 1..25. 285 records have seed 0.
- Implemented: ParticleDefs::maxEmitters/maxParticles, ParticleDef::flags/seed/particles/spawnMode.
- Unknown: per-particle init 0x222550 and the emitter update (velocity, colour keys at +0x90/+0x138, size curves), the values of +0x178, the hash-to-index conversion, word 4 and the word-5 float.

<a id="ts-20261013-0903"></a>

## 2026-10-13T09:03+02:00 — Per-particle init 0x222550 (REA Ghidra 12.1.2 ev_57c912e6…; read-only)

Corrected by [particles.md §2026-10-18T09:40](#ts-20261018-094004): +0x14 is the speed at the emitter's start and +0x78 the speed at its end
Corrected by [particles.md §2026-10-13T09:07](#ts-20261013-0907): the record has four key lists

- [Corrects particles.md §2026-10-13T09:01](#ts-20261013-0901): +0x178 is not a spawn mode. 0x222550 reads +0x178 and +0x179 as 1/x divisors: atlas columns and rows. The spawner's "+0x178 == 0 → no spawn" check guards that division. The 09:01 field `spawnMode` was renamed to `atlasColumns`.
- Random numbers come from the emitter's LCG at emitter +0x134: x = x × 0x343fd + 0x269ec3; value = ((x & 0x7fffffff) >> 15) / 65536. It is seeded from record +0x84 (spawner).
- Fields (E = emitter; s = the scale from 09:01):
  - +0x54/+0x58/+0x5c: half-extents of a box offset, ±value × (2r − 1) per axis. Used when E+0x150 bit 2 is clear; otherwise E+0xd4..0xdc.
  - +0x174 is the spin mode. 0 = no spin. Otherwise the start angle is random in 0..8000 (8000 = one turn), and the spin speed is (+0x40 + r × (+0x44 − +0x40)) × 100. Mode 2 negates the speed; mode 3 picks a random sign. Mode 5 aims the angle along the velocity, using atan2 (flags bit 4 / bit 12 choose the frame).
  - +0x0c/+0x10 (× s) are the base size x/y. +0x64/+0x68 add r × value × E+0x108.
  - Colour gradient: +0x172 keys at +0xf8 + 8k, each (f32 time, u8[4] colour). The first time is 0 and the times rise in all 298 records. Flags bit 2 picks a random point on the gradient; otherwise the colour is +0xfc. The colour bytes are scaled by the alpha argument (+0x8c / 255 from the spawner).
  - +0x134 (u8) × that argument gives the particle alpha.
  - Direction: with E+0x48 == 0 (inferred from the branch), the azimuth is random in 0..8000 and the elevation random up to +0x18 degrees (× 22.222 = 8000/360). Otherwise the azimuth advances by +0x48 degrees per particle (a spiral) and the elevation is +0x18.
  - +0x7c..+0x80 give the lifetime, uniform in that range.
  - Speed is +0x78 × s. When +0x38 ≠ 0 it is lerped from +0x78 × s towards +0x14 × s by (+0x38 − E+0xe0) / +0x38, so +0x38 is probably the emitter duration. +0x16f (u8) adds ±percent random variation.
  - Flags (+0x168):
    - bit 3: velocity z = 0.
    - bit 5: start at the end point and move back (implode).
    - bit 6: per-particle counter E+0x156 += 250..750 (mod 1000), meaning unknown.
    - bit 17: colour from a lookup at the position (0x221064), averaged with the gradient colour.
  - +0x176 frame mode: 0 = whole texture; 1 = random atlas cell; 2 = first cell; 3 = the next cell per particle (wraps at columns × rows).
  - +0x17a: low nibble = U flip, high nibble = V flip. 1 = random, 2 = every other particle, 3 = always.
  - UVs are stored as u16 × 65535, clamped to 0.9999.
- Data census: frame modes 0:190, 1:53, 2:38, 3:17. Columns {1,2,3,4,5,8}, rows {1,2,3,4}. Key counts 2..7 (846 keys).
- Implemented: ParticleDef::startAlpha, atlasColumns, atlasRows, frameMode, flipModes and colourKeys.
- Unknown: emitter fields E+0x100..0x114 and E+0xd4..0xdc (set by the spawner from +0x0c, +0x10, +0x14, +0x1c, +0x20, +0x24, +0x3c, +0x78); the per-frame update (gravity, drag, size over time); +0x174 values 1 and 4; flags bits not listed; +0x16f's exact formula (it uses a second random source 0x2345dc).

<a id="ts-20261013-0905"></a>

## 2026-10-13T09:05+02:00 — Emitter update 0x20db0c (REA Ghidra 12.1.2 ev_a8436740…; read-only)

Corrected by [particles.md §2026-10-18T09:40](#ts-20261018-094004): durations of 98 and above do not age
Corrected by [particles.md §2026-10-13T09:07](#ts-20261013-0907): the record has four key lists

- [Corrects particles.md §2026-10-13T09:00](#ts-20261013-0900) and [Corrects particles.md §2026-10-13T09:01](#ts-20261013-0901): +0x88..+0xf7 is a second key list, not colour keys. +0x170 keys at +0x88 + 8k, each (f32 time, u8 alpha, 3 zero bytes). This is the emitter fade over its duration: t = 1 − (duration − age) / duration, lerped between keys, then / 255. Flags bit 1 squares the result. It is stored ×128 in E+0x155. The spawner's +0x8c is the first key's alpha.
- +0x28 is confirmed as particles per second. The accumulator E+0xe4 += dt × +0x28 × E+0xe8; its floor is the number of new particles this frame. These go into free slots (slot +0x38 == −1.0). Optional sub-stepping splits dt into param_3 parts and interpolates the emitter position (0x221e5c).
- +0x38 is confirmed as the emitter duration. Age E+0xe0 advances only while +0x38 ≤ 98. Larger values (100 in 197 records) do not age; they count down E+0x120 instead and stop when it reaches 0. The emitter is removed when age ≥ duration.
- Each particle is advanced by 0x221e9c(dt, fade, emitter, particle, matrix, frames, uv-scale, bounds…), which also grows the emitter bounds (E+0xa0 centre, E+0xac half-size). The bounds are refreshed every 4–7 frames (random, 0x235498) unless +0x74 ≥ 0 keeps the larger extent.
- When a removed emitter's texture has type 2 and is no longer used, it is released about a second later (list 0x586328, up to 20 entries).
- Implemented: ParticleDef::rate, duration, lifetimeMin/Max, fadeKeys. Real-data check: 648 fade keys, 197 endless emitters.
- Unknown: the per-particle step 0x221e9c (velocity integration, gravity/drag fields, size over life), E+0xe8 (rate multiplier source), and the meaning of +0x74's sign.

<a id="ts-20261013-0907"></a>

## 2026-10-13T09:07+02:00 — Particle step 0x221e9c and the four key lists (REA Ghidra 12.1.2 ev_8d5bac72…; read-only)

Corrected by [particles.md §2026-10-18T09:40](#ts-20261018-094004): +0x14 / +0x78, +0x6c / +0x70 (sway) and E+0x11c (ground height) are settled
- [Corrects particles.md §2026-10-13T09:03](#ts-20261013-0903) and [Corrects particles.md §2026-10-13T09:05](#ts-20261013-0905). The record has four key lists, each of (f32 time, 4 value bytes), 8 bytes per key, at most 7 keys:
  - +0x88, count +0x170: emitter fade (value[0] = alpha).
  - +0xc0, count +0x171: size change.
  - +0xf8, count +0x172: colour (value[0..2]).
  - +0x130, count +0x173: particle alpha over its life (value[0]).
  - So "+0x134 start alpha" (09:03) is the first alpha key's value, and the fade list holds at most 7 keys, not 14.
  - In all 298 records every list starts at time 0 and rises. Totals: fade 648, size 718, colour 846, alpha 1045.
- Per frame, for a live particle P (P+0x38 = age, P+0x34 = lifetime, f = age / lifetime):
  - position += velocity × dt (0x2408f8, 0x23d39c).
  - If +0x74 > 0 and the particle is at or below the emitter ground height E+0x11c: when moving down, vy = −vy × +0x74 and y is clamped to the ground. So +0x74 = bounce restitution.
  - Drag: when |E+0xf4| > 2^-23, velocity ×= 1 − min(E+0xf4 × dt, 1). E+0xf4 = +0x24. Flags bit 15 also damps the spin.
  - Spin: when E+0x110 (spin range) ≠ 0, angle += spin and wraps at 8000.
  - When +0x6c ≠ 0, P+0x3c advances by +0x70 × 6400 and wraps at 0x7d000 (meaning unknown).
  - Size: from the size keys, g = (lerp(value[0]) − 128) / 128 × dt. size x += E+0xec × g and size y += E+0xf0 × g, where E+0xec = +0x1c × s and E+0xf0 = +0x20 × s. Flags bit 9 clamps the size at 0.
  - Gravity: vy += E+0x10c × dt, where E+0x10c = +0x3c × s (spawner).
  - Frame mode 2 animates: cell = frac(+0x17b × f) × (columns × rows). So +0x17b counts atlas loops per life. The 09:03 "2 = first cell" is only the init value.
  - Age += dt. At the end of its life the particle becomes free (−1.0), or −2.0 with flags bit 8.
  - Colour: unless flags bit 2, the RGB comes from the colour keys at f.
  - Alpha = lerp(alpha keys at f) × the emitter fade.
- Speed (from 0x222550): it starts from E+0xf8 = +0x14 × s and E+0xfc = +0x78 × s, lerped by the emitter's remaining-time fraction. Which end belongs to which value is not settled; ParticleDef does not expose it.
- Implemented: ParticleKey and the four lists; sizeX/sizeY, growX/growY, gravity, drag, bounce, animationLoops. The 09:03 fields startAlpha and ColourKey/FadeKey were replaced.
- Unknown: E+0x11c (ground height source), +0x6c/+0x70, the flags not listed, the spin-damping detail, and +0x14 vs +0x78.

<a id="ts-20261013-0908"></a>

## 2026-10-13T09:08+02:00 — Hash-to-index conversion (llvm-objdump; read-only)

- Particle system init 0x1ed440 calls the loader 0x27d71c. For each record (stride 0x17c), it replaces +0x2c with the index of the first entry in the loaded-texture hash table (*0x376bb4, count *0x376b80) that equals the hash. With no match it stores 0. Because missing textures are dropped from that table (09:00 entry), the index refers to the loaded table, not the file's name list.
- The same init stores its third argument, when non-zero, into 0x376b84. So the caller can override header word 0 (the emitter limit). It then allocates the emitter list (*0x376b98, limit × 4 bytes) and a second array of limit × 16 bytes (*0x376ba8, unknown).
- ParticleDef::texture keeps −1 for no match; the game would use 0. All 298 shipped records match.

<a id="ts-20261018-094004"></a>

## 2026-10-18T09:40:04+02:00 — Particle runtime: start entry, update, init, step, draw and the oracle check (REA Ghidra 12.1.2 read-only, llvm-objdump of `artifacts/game/game.arm.elf`; Azahar state `in-vehicle`, memory read through the GDB stub)

- [Corrects particles.md §2026-10-13T09:01](#ts-20261013-0901): header word 0 (10) is not the emitter limit of the game. Init 0x1ed440 replaces it with its third argument; `*0x376b84` is 25 in the oracle (states `gameplay-start`, `in-vehicle`; 12 emitters alive in `in-vehicle`). The particle limit `*0x376b88` is the file's word 1 (25), so 24 slots at most.
- [Corrects particles.md §2026-10-13T09:05](#ts-20261013-0905): an emitter does not age when the duration is 98 or more (not only above 98); the shipped endless records all have 100.
- [Corrects particles.md §2026-10-13T09:07](#ts-20261013-0907) and [Corrects particles.md §2026-10-13T09:03](#ts-20261013-0903): +0x14 is the particle speed at the emitter's start and +0x78 the speed at its end; +0x6c / +0x70 are the sway (below); E+0x11c is the world height of the ground below the emitter.
- [Corrects carrying.md §2026-10-15T13:08](carrying.md#ts-20261015-130800): 0x23aebc is not a sound. It builds a position and calls 0x20c938, which reads `DebrisColour`, `DebrisColour2`, `DebrisRatio`, `DebrisAmount` and starts the debris burst 0x21e2e4 (bricks, not the particle system).

**Evidence.** Pseudo code of 0x216244 (ev_5f3e0e6f…), 0x222550 (ev_57c912e6…), 0x221e9c (ev_8d5bac72…), 0x20db0c (ev_a8436740…), 0x1ece0c (ev_44fae383…), 0x21539c (ev_287c23f2…), 0x2e50e0 (ev_963e93f7…), 0x23de24 (ev_a61454c9…), 0x20fff4 (ev_c364300d…), 0x2a848c (ev_85d1a8dd…), 0x23ddb4 (ev_08f733bd…), 0x23de14 (ev_f0698541…), 0x2722cc (ev_19b6be58…), 0x2b5970 (ev_0827f664…), 0x2b5f54 (ev_61f760c7…), 0x2ae968 (ev_8bb1afef…), 0x252254 (ev_8ae157eb…), 0x1ed440 (ev_71e32e5e…), 0x22a124 (ev_9f94d94c…), 0x23aebc (ev_c89fd17b…), 0x20c938 (ev_43a28388…), 0x1a7f30 (ev_db2becd5…), 0x1a7d30 (ev_10403a61…), 0x1a8394 (ev_48283cbd…), 0x20c7f0 (ev_c6ee9397…), 0x249ee8 (ev_24fe6fe4…), 0x20b214 (ev_df53d59e…). llvm-objdump of 0x249038 (sin and cos of one angle through 0x247ac8 and 0x247c90), 0x252c14 (floor), 0x253618 (ceil), 0x25c1b8 (unsigned divide with remainder), 0x23b960 (cross product a x b), 0x257ea0 (3D normalise, (0, 0, 1) at length 2^-23 or less), 0x23ba14 (2D normalise, (0, 1) then), 0x221e5c (out = a + (b - a) t), 0x235498 (random number modulo n), 0x2345dc (low 16 bits of 0x25b2b8 x 2^-16), 0x23602c (frame time of the game clock), 0x2355c0 (game seconds), 0x25688c and 0x1588c4 (sort key and comparator), 0x171e08 (parameter setter), the byte tables at 0x3762b4 and 0x376b68..0x376bc8.

**Start entry.** `0x23de24(prewarm s0, definition r0, position* r1, parent r2, no-room-check r3, stack: axis*, mode)` checks `definition < *0x376b90` and calls the spawner 0x216244 on record `*0x376b8c + index x 0x17c`; it returns the emitter or 0. 0x20fff4(scale s0, index, position, ...) does the same with the record's +0x60 replaced by scale x `*0x376b94` for the call; 0x2a848c(index, position, owner) needs `*0x376b74` (init count) and stores `owner` in E+0x160. The spawner refuses when the live count `*0x376b6c` is at the limit, when +0x178 (atlas columns) is 0, and (only with `*0x376c78` set, not read) when the room lookup 0x239838 of the position fails. Callers: about 70 sites of 0x23de24 (0x15aa70, 0x15d944, 0x163f00, 0x16d420, 0x16e25c, 0x174c70, 0x1797c4, 0x184724, 0x1848e0, 0x1858dc, 0x186e70, 0x19d568, 0x1a6d94, 0x1a7d30, 0x1a8394, 0x1abc5c, 0x1ac1a0, 0x1ad2d8, 0x1ae0c8, 0x1ae500, 0x1b59f0, 0x1ba5e8, 0x1cdb74, 0x1cf740, 0x1d4804, 0x1d8a08, 0x1f5244, 0x211530, 0x21c934, 0x225248, 0x22a124, 0x22a9ec, 0x23019c, 0x2398d8, 0x25e354, 0x25f460, 0x2670d4, 0x268324, 0x26c728, 0x273974, 0x278b78, 0x279bf4, 0x27f7f4, 0x2837b8, 0x284640, 0x285ddc, 0x287b48, 0x288430, 0x28c7d4, 0x28c95c, 0x291e68, 0x297514, 0x2a4604, 0x2a8388, 0x2aa5c8, 0x2afeac, 0x2b900c, 0x2bd834, 0x2bd974, 0x2c1498, 0x2c2048, 0x2c4b74, 0x2c7d80, 0x2c7ebc, 0x2ca0a0, 0x2cbc68, 0x2d8ec8, 0x2f1a08, 0x2f4e10, 0x30ea54 (the calling functions)), among them 0x22a124 (vehicle smoke: definition id in r1, position = the entity's point + direction x 0.85 x entity +0x74, parent 0, stop with fade 0.1 s through 0x23ddb4, handle kept at record +0x2e4, callback 0x2ad53c set with 0x23de14), 0x1a8394 and 0x1a7d30 (the level object class `particles`: constructor 0x1a7f30 reads `ParticleDef`, `ReTriggerTime`, `TimeRandomiser`, `Damage`, `DamageType`, `DamageAllObjects`, `AlwaysEmitSound`, `ATTR_SFX`, `Instance`, `Speed`, `JustMoveSpawnPoint`, `OrientToSpline`; 0x1a8394 starts the emitter, ends it with 0x23ddb4(0.1) and damages what stands in it; 0x1a7d30 handles the player in the bound: damage 0x244fc8 and the definition in record +0x34 at him).

**Stop and set.** 0x23ddb4(fade s0, emitter): duration 98 or less: age = duration - fade; otherwise fade 0: age = duration + 1 (removed by the next update), else E+0x120 = E+0x124 = fade (countdown to the removal); it follows the child link E+0x164. 0x171e08(emitter, key, value*): key 0 sets the alpha byte E+0x154, key 2 the size scale E+0x108, key 4 replaces the texture (E+0x15c).

**Emitter fields** (E, from the spawner and the update; the allocation is `geParticle` with 0x168 + count x 0x44 bytes, the particle array at +0x168): +0xbc local position, +0xc8 previous position (copied from +0xbc after every update by 0x1ece0c), +0xd4..+0xdc box override (used with flag 4 of +0x150), +0xe0 age, +0xe4 creation accumulator, +0xe8 rate scale (1.0), +0xec / +0xf0 size growth x scale (+0x1c, +0x20), +0xf4 drag (+0x24), +0xf8 start speed (+0x14 x scale), +0xfc end speed (+0x78 x scale), +0x100 / +0x104 start size (+0x0c, +0x10 x scale), +0x108 scale, +0x10c gravity (+0x3c x scale), +0x110 spin range (+0x44 - +0x40), +0x114 spiral azimuth, +0x118 last-visible time, +0x11c ground height, +0x120 / +0x124 end countdown, +0x128 / +0x12c prewarm time and elapsed, +0x130 creation delay, +0x134 LCG, +0x138 room object, +0x13c / +0x140 owner and callback, +0x144 record, +0x150 flags (1 active, 2 positioned inside the node, 4 box override, 0x10 no new particles, bits 5..7 bounds refresh counter), +0x153 frame counter, +0x154 alpha (255), +0x155 fade x 128 (128), +0x156 tumble counter, +0x15c texture override, +0x160 owner link, +0x164 child. Particle (0x44 bytes): +0x00 velocity, +0x0c position relative to the emitter, +0x18 rgba, +0x1c / +0x20 size, +0x24 / +0x26 u (first, second), +0x28 / +0x2a v (first, second) as u16 x 65535, +0x2c angle (8000 per turn), +0x30 spin, +0x34 life, +0x38 age (-1 free, -2 finished), +0x3c sway phase x 64 in bits 0..29 and the u / v flip in bits 30 / 31, +0x40 draw angle, +0x42 tumble. The static values: `*0x376b68` = 0xff, `*0x376b94` = 1.0.

**Update 0x1ece0c(dt s0, sub-steps r0, emitter r1)**: the game loops 0x2e5458 and 0x2faf0c call it with 2 sub-steps and no emitter (all); 0x2e50e0 flushes with `0x20db0c(0, 0, 1)`; 0x21539c repeats 0x20db0c(frame time, emitter, 1) until the given time has passed. `*0x376b69` set stops everything. Per emitter in list order: duration 98 or more: no ageing, a non-zero E+0x120 counts down and ends the emitter at 0 or below; else age += dt. The emitter stays while age is not above the duration and it has not ended; otherwise message 2 goes to the callback, the live count drops, the record is released (0x2559f8 for durations below 98, then 0x250b8c) and the last emitter takes its slot (the swapped one is looked at next). A kept emitter: the delay E+0x130 counts down to 0; the fade lerps the first value byte of the fade keys at t = 1 - (duration - age) / duration (clamped at 0), 1.0 when no segment holds, squared with flag 2, stored x 128 + 0.5 in E+0x155; creation: acc = E+0xe4 + dt x rate x E+0xe8, n = floor(acc), n = 0 when age + dt (age was already advanced, so dt counts twice) reaches the duration, message 1 when n > 0. With n < 1, flag 0x10 or fewer than 2 sub-steps: E+0xe4 = acc - n, every slot in order: a free slot (age exactly -1; -2 is never reused) takes a new particle while n lasts, then every slot steps by dt. With sub-steps: all slots step by dt x E+0xe8 first and the first free slot is noted (after stepping, so a particle that just ended counts); then for each sub-step: acc += dt / S x rate x E+0xe8, m = floor(acc), the position E+0xbc is lerped from E+0xc8 towards its saved value by (s + 1) / S, m free slots from the noted one on (the slot index only moves forward) are initialised and stepped by dt minus the time of the sub-steps so far, E+0xbc is restored. After an update with a prewarm E+0x128 above E+0x12c, the emitter is updated again at once (E+0x12c += dt). The bounds (E+0xa0, E+0xac), the room visibility rules (E+0x138, E+0x118: 0.5 s and 5 s) and the texture release list at 0x586328 (about 1 s after the last emitter of a type-2 texture) are not part of the port. Oracle: eight emitters of record 242 created in one update have E+0xe4 = 0.667282 (dt 1/30: 83.33 per sub-step, remainders 0.333 then 0.667), two have 0.0034 (dt 0.07).

**Spawner details.** The particle count: with a rate above 0 and ceil(lifetime min x rate) below the count, the record's +0x175 becomes that value + 1 (the shared record is shortened for good); then at most limit - 1 = 24. LCG seed: +0x84 or, when 0, the next word of 0x25b2b8. Position: the node position; with flag 8 of +0x168 z is negated. Orientation: the record's +0x00 vector (a unit vector in all 298 records) or the caller's: if |y| < 0.7071 (bits below 0x3f34fdf5) f = normalise(up x d), e = normalise(normalise(d x f)), else e = normalise(d x (1, 0, 0)), f = normalise(normalise(e x d)); the rows of the node matrix are f, e, d. Start values: the scale s = (+0x60 when non-zero) x `*0x376b94` as above. Initial burst: rate 0 gives all slots, else floor(frame time x rate) at least 1 and at most the count, with the alpha argument +0x8c / 255; the other slots are marked free. With bounce (+0x74) above 0 the ground is found by up to 3 world ray casts 0x22f978 from the emitter down 1000 units, each next cast ending 1.19e-7 above the previous hit (`*0x376b9c`, the collision world, was non-zero at `gameplay-start`); E+0x11c is the y of the last hit.

**Init 0x222550(alpha a, emitter, particle, matrix)** is skipped while E+0x130 is above 0. Draw order from the LCG (each draw = ((x & 0x7fffffff) >> 15) / 65536 after x = x x 0x343fd + 0x269ec3): box offsets for the non-zero extents in x, y, z order (extent x (draw - 0.5) x 2), turned by the node rows (position = v . M; with flag 2 added to E+0xbc unturned); sway phase when +0x6c is above 0 (draw x 8000 as an integer, x 64 into +0x3c); spin angle when +0x174 is not 0 (draw x 8000), the speed when the range is above 0 (mode 2 reversed, mode 3 a sign draw below 0.5 then a magnitude draw, x 100); the size draw when +0x64 or +0x68 is above 0 (size = start + draw x +0x64 / +0x68 x scale, one draw for both); the gradient draw with flag 4; the azimuth and elevation draws without a spiral (azimuth = draw x 8000, elevation = draw x +0x18 degrees x 22.222223 truncated; with a spiral azimuth = E+0x114 + step x 22.222223, wrapped at 8000, elevation +0x18); the life draw; the tumble draw with flag 0x40; the cell draws of frame mode 1 (column, row) and the flip draws (mode 1 only) at the end. Velocity = (sin el sin az, sin el cos az, cos el) . M x speed, the speed = +0x14 x s, or when +0x78 is above 0: +0x78 x s + (+0x14 x s - +0x78 x s) x t with t = (duration - age) / duration (1 for an endless emitter), times 1 +- the variation byte +0x16f per cent from the global generator 0x2345dc. Flag 8 zeroes z; flag 0x20 puts the particle at velocity x life and reverses the velocity; spin mode 5 sets the angle from the velocity (x, z with flag 0x10, in the emitter's axes with 0x1000, else in the camera's). The colour is the first key's four bytes, or with flag 4 the gradient at the draw x a; alpha = first alpha key x a. UV: frame mode 0 whole, 2 the first cell with the scale, 1 a random cell, 3 the next cell by E+0x153 (wraps at columns x rows); flip modes 1 (random half), 2 (odd slots), 3 (always) set bit 30 / 31 of +0x3c and swap the two values; every value is clamped to 0.9999 from 1.0 and stored x 65535 as u16. Oracle `in-vehicle`, emitter 0x08d94b20 (record 242, three particles): the angles 0x45f968a2, 0x454261b8, 0x44f46e38, sizes 0x3f52da33 / 0x3ed2da33, 0x3f507280 / 0x3ed07280, 0x3f5c9a67 / 0x3edc9a67, the flip words and E+0x114 = 4000 equal what these rules give from the LCG state 0x69e4f907 (found by searching the recorded draws; CTest `particles-real-data`).

**Step 0x221e9c(dt, fade, emitter, particle, matrix, cells, uv scale)** for a particle whose age is not -1 or below: position += velocity x dt; with bounce above 0 and the world height (y + node y) at or below E+0x11c and moving down: velocity y = -(y x bounce), y = ground - node y; drag factor = E+0xf4 x dt (1 at most) when |E+0xf4| bits are above 2^-23, velocity x (1 - factor), spin x (1 - factor) with flag 0x8000; spin range above 0: angle += spin, wrapped at 8000; sway above 0: phase += (int)(+0x70 x 6400), reset to 0 above 0x7cfff; the size keys at f = age / life: size += E+0xec / +0xf0 x ((lerp - 128) x 0.0078125 x dt), clamped at 0 with flag 0x200; velocity y += E+0x10c x dt; frame mode 2 cell = frac(loops x f) x cells (column = cell mod columns, row = trunc(cell / columns), flips from +0x3c); age += dt, at the end of its life age = -1 (-2 with flag 0x100); the colour keys at f (unless flag 4), the alpha keys at f x fade.

**Draw 0x2722cc** takes draw items of 0x10 bytes (word +8 points to a record whose first word is the emitter and whose +4 holds the node translation, the model matrix) and fills a render-state record (0x252254 defaults): no culling, blend byte pair (+0x177) 0: source alpha / one minus source alpha, 1: source alpha / one, 2: source alpha / one with equation byte 2 (reverse subtract in the PICA numbering), 3: zero / source colour (factor indexes of 0x37628c, [materials.md](materials.md)); depth function index 0 (table 0x3762b4: less or equal), depth write off, flag 1 of +0x168 turns the alpha test on with reference E+0x155, flag 0x800 draws alpha only, flag 8 planar, the texture is E+0x15c or `*0x376b78`[+0x2c]. The quads are built per slot from the definition: 0x2b5970 (billboards, used when spin mode is 4 or the flags 0x1050 are clear): the slots in reverse order, or sorted by view depth far to near with flag 0x2000; the alpha is (alpha x near fade x E+0x154) / 255 + 0.5, the near fade (depth - `*0x376bbc`) x `*0x376bc0` clamped to 0..1 (these are header words 4 and 5; 1 with flag 0x10000), the corners (pivot 0x4c / 0x50) from 0x2ae968: (ax - 1, ay + 1), (ax + 1, ay + 1), (ax + 1, ay - 1), (ax - 1, ay - 1) with ax = 1 - 2 pivot x and ay = 2 pivot y - 1, turned by the angle and scaled by the size (scaled first with flag 0x4000), then position + a x camera right + b x camera up (the rows of the camera matrix `0x529fcc`), the sway added along right x and z; the vertices take u (+0x24, +0x26, +0x26, +0x24) and v (+0x2a, +0x2a, +0x28, +0x28); 0x2b5f54 (flat quads, forward slot order, alpha without the near fade): corners (a, 0, b), (a, b, 0) turned by the emitter node with flag 0x1000, with flag 0x40 turned by a tumble matrix from the counter (+0x42 x 0.001 x pi).

**Unknowns.** What `*0x376c78` is and the room lookup 0x239838; the parent node attachment of mode 1 and 2 spawns and what 0x250810 / 0x245008 do to the node's world matrix (the port takes it as a translation); the bounds E+0xa0 / E+0xac and the culling they feed; E+0x148 (callback 0x1c29dc), E+0x160 (light 0x228c68 sets E+0x154), E+0x164 child chains; the combiner of the draw (the state bytes say texture x vertex colour with the texture alpha, not read down to the registers) and sampler wrap; the draw item order of several emitters and how semi-transparent objects interleave; the world matrix used for the view depth (the port takes the distance along the camera's forward axis); the ground height when no ray hits (the original leaves a stack value); the draw 0x2aeaf0 of flag 0x800 and the colour lookup 0x221064 of flag 0x20000 (no shipped record has flag 8 or 0x800 and 4 records have 0x20000); what E+0x124 does after a stop; the step and update against a recorded young emitter (only the init is compared with the oracle).
