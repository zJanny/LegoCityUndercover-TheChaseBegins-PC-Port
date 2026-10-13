# Particle definitions

Reverse-engineering findings for `particles/particledefs.bprt`. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis.

## 2026-10-13T15:00+02:00 — particledefs.bprt container (REA Ghidra 12.1.2; read-only)

- Loader 0x27d71c (ev_ed8196cc…) opens "Particles/ParticleDefs.bprt" (pooled resource, see pooled-resources.md) and reads, in this order:
  - Pool −15: six u32 blocks. Word 0 → 0x376b84 (10; the block pointer is also kept at 0x376bc4). Word 1 → 0x376b88 (25). Word 2 = texture count N (97). Word 3 → 0x376b90 (298). Word 4 → 0x376bbc (0). Word 5 is a float → 0x376bc0 (2.328); 0 becomes 1.0.
  - Pool −16: N texture-name blocks.
  - Pool −14: one data block → 0x376b8c (113240 bytes).
- 0x271e6c (ev_312eca7f…) loads each name as "Particles/<name>.tga". It stores the name hash 0x1886f0 in a parallel table at *0x376bb4. A name whose texture is missing is dropped, and the loop bound shrinks with it.
- Hash 0x1886f0 is PJW/ELF over the ASCII-lower-cased name, with signed bytes: h = (h << 4) + c; when bits 28–31 are set, h ^= high >> 24 and those bits are cleared.
- The data block is exactly 298 × 380 bytes, and word 3 is 298. The 380-byte record size is inferred from that. Record +0x2c equals the PJW hash of a texture name in all 298 records.
- Record fields, observed only:
  - Many float pairs look like min/max ranges (+0x0c/+0x10, +0x14/+0x18, +0x1c/+0x20).
  - +0x90/+0x94 and +0x138..+0x144 hold (float, u32) pairs that look like timed colour or alpha keys.
  - +0x104 holds colour-like words (0xff666666).
  - +0x168..+0x178 hold small packed values.
- Spawners address definitions by index. 0x163f00 (ev_f0d2eaec…) reads the float attribute "ExplosionParticleDef" as an index for 0x23de24. It also compares word 0 >> 1 with a count from 0x288744, so word 0 may be a capacity (inferred).
- Implemented: chase::assets::parseParticleDefs and pjwHash (src/assets/particles.cpp). Synthetic tests cover the format. Real-data totals (97 textures, 298 records, all hashes resolved, FNV-1a of the records) come from the tools/tt_resource.py parse.
- Unknown: the meaning of every record field except +0x2c; header words 0, 1 and 4; the float in word 5. Next step: field use inside the spawner 0x23de24 and the emitter update.
