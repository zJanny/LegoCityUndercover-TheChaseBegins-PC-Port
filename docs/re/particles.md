# Particle definitions

Reverse-engineering findings for `particles/particledefs.bprt`. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis.

<a id="ts-20261013-0900"></a>

## 2026-10-13T09:00+02:00 — particledefs.bprt container (REA Ghidra 12.1.2; read-only)

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

- Correction to the 09:01 entry: +0x178 is not a spawn mode. 0x222550 reads +0x178 and +0x179 as 1/x divisors: atlas columns and rows. The spawner's "+0x178 == 0 → no spawn" check guards that division. The 09:01 field `spawnMode` was renamed to `atlasColumns`.
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

- Correction to the 09:00/09:01 entries: +0x88..+0xf7 is a second key list, not colour keys. +0x170 keys at +0x88 + 8k, each (f32 time, u8 alpha, 3 zero bytes). This is the emitter fade over its duration: t = 1 − (duration − age) / duration, lerped between keys, then / 255. Flags bit 1 squares the result. It is stored ×128 in E+0x155. The spawner's +0x8c is the first key's alpha.
- +0x28 is confirmed as particles per second. The accumulator E+0xe4 += dt × +0x28 × E+0xe8; its floor is the number of new particles this frame. These go into free slots (slot +0x38 == −1.0). Optional sub-stepping splits dt into param_3 parts and interpolates the emitter position (0x221e5c).
- +0x38 is confirmed as the emitter duration. Age E+0xe0 advances only while +0x38 ≤ 98. Larger values (100 in 197 records) do not age; they count down E+0x120 instead and stop when it reaches 0. The emitter is removed when age ≥ duration.
- Each particle is advanced by 0x221e9c(dt, fade, emitter, particle, matrix, frames, uv-scale, bounds…), which also grows the emitter bounds (E+0xa0 centre, E+0xac half-size). The bounds are refreshed every 4–7 frames (random, 0x235498) unless +0x74 ≥ 0 keeps the larger extent.
- When a removed emitter's texture has type 2 and is no longer used, it is released about a second later (list 0x586328, up to 20 entries).
- Implemented: ParticleDef::rate, duration, lifetimeMin/Max, fadeKeys. Real-data check: 648 fade keys, 197 endless emitters.
- Unknown: the per-particle step 0x221e9c (velocity integration, gravity/drag fields, size over life), E+0xe8 (rate multiplier source), and the meaning of +0x74's sign.

<a id="ts-20261013-0907"></a>

## 2026-10-13T09:07+02:00 — Particle step 0x221e9c and the four key lists (REA Ghidra 12.1.2 ev_8d5bac72…; read-only)

- Correction to the 09:03 and 09:05 entries. The record has four key lists, each of (f32 time, 4 value bytes), 8 bytes per key, at most 7 keys:
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
