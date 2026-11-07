# Particles

## 1. Scope

`particledefs.bprt` definitions and the particle runtime: start entry, emitter update, particle init and step, stop, the quad builders and the draw state of the particle pass, in `chase_runtime` and `chase_render`. Not covered: the callers that start effects (script natives, level objects, vehicles), the brick debris system (0x21e2e4, VEH-112) and the vehicle break (VEH-110).

## 2. Status

IN_PROGRESS, updated 2026-11-02T22:43:51+01:00. Emitters and particles run with the original's rules and are drawn in `chase-game`; the first quad of a siren-light emitter equals the oracle bit for bit. The game starts three kinds of emitters: the smoke of a vehicle that loses a heart (0x22a124), the `ExplosionParticleDef` of a destroyed prop (0x2aa5c8) and the particle objects a script triggers (class `particles`, PART-3); the development option `--particle "DEFINITION X Y Z"` starts any definition.

## 3. Verified behaviour

- Start entry 0x23de24, spawner 0x216244 (count shortening, seed, basis from the axis, scale, initial burst, ground ray), stop 0x23ddb4, update 0x20db0c with the game's 2 sub-steps, init 0x222550 (LCG draw order), step 0x221e9c and the quad builders 0x2b5970 / 0x2b5f54 are ported from the read functions (`src/runtime/particles.*`). evidence: [particles.md §2026-10-18T09:40](../../re/particles.md#ts-20261018-094004).
- Oracle (save state driving a traffic van after mission 1-1, emitter of record 242, siren light): angle, size, uv words, flip bits, colour, alpha and the spiral accumulator of its three particles equal the port bit for bit from the recorded LCG state; the emitter limit is 25 and the creation accumulators of eight fresh emitters follow the sub-step rule (CTest `particles-real-data`). evidence: [particles.md §2026-10-18T09:40](../../re/particles.md#ts-20261018-094004).
- CTest `particles-core` pins values computed by hand from the rules (basis, count shortening, emitter limit, LCG box offsets, step, colour and alpha keys, bounce, ageing and stop, sub-step ages, quad corners, uv, near fade, atlas cells); `particles-real-data` runs all 298 records for 3 s and checks record 175, record 53 and the siren light.
- The draw state: blend byte 0 and 1 of `+0x177` as alpha blend and additive (the only values in the shipped records), depth test less or equal, no depth write, no culling, alpha test with the emitter fade for flag 1. The quads of `chase-game` show the right orientation (question marks of record 22 upright). evidence: [particles.md §2026-10-18T09:40](../../re/particles.md#ts-20261018-094004).

## 4. Stand-ins

- PART-S1 The random numbers: the definitions with seed 0 and the global generator 0x2345dc use a fixed seed (`game_particles::kStandInSeed`), not the game's time based one.
- PART-S2 Emitters sit in the world only: no parent node, no room lookup, no mode 1 and 2 attachment, no visibility removal and no bounds. needs: PART-4. evidence: [particles.md §2026-10-18T09:40](../../re/particles.md#ts-20261018-094004).
- PART-S3 Without a world ray cast (`chase-game` passes none) the ground of a bouncing particle is 1000 below the emitter; the original leaves a stack value there. needs: PART-6. evidence: [particles.md §2026-10-18T09:40](../../re/particles.md#ts-20261018-094004).
- PART-S4 Sine and cosine come from the C library instead of 0x247ac8 / 0x247c90, the far-to-near sort uses the exact depth instead of the quantised key 0x25688c, and the prewarm repetition is skipped for a time step of 0 (the original would not end).

## 5. Open items

- [ ] PART-2 — Most callers do not start emitters: the script natives (`EffectPlayParticles`, `EffectAttachParticles`, `EffectAttachParticlesOffset`, `EffectAttachParticlesToPath`), the vehicle's other emitter slots (+0x2ac, +0x2c4) and the other callers of 0x23de24 (about 68 sites); the vehicle smoke 0x22a124, the prop explosion of 0x2aa5c8 and `PlayParticlesGO` (SCRIPT-S300) are ported. blocks: SCRIPT-38, PART-3. evidence: [particles.md §2026-10-18T09:40](../../re/particles.md#ts-20261018-094004).
- [ ] PART-3 — The rest of the level object class `particles` (kind 1): read in full, ported are only TriggerObject's trigger and untrigger and the one-shot timer; open are the endless self-start, the retrigger times, damage, sound, path, parent node and the objects nested in class files. needs: PART-2. blocks: SCRIPT-S325, SCRIPT-438. evidence: [particles-objects.md §2026-11-02T22:39:38](../../re/particles-objects.md#ts-20261102-223938).
- [ ] PART-4 — Parent nodes and rooms: spawner modes 1 and 2 with a parent object, the room lookup (`*0x376c78`, 0x239838), the visibility removal and bounds of 0x20db0c, E+0x148 / +0x160 / +0x164 and the callback owner. blocks: PART-S2. evidence: [particles.md §2026-10-18T09:40](../../re/particles.md#ts-20261018-094004).
- [ ] PART-5 — The update and step have no recorded trace: only the init is compared with the oracle. Record a young emitter (a driving car's smoke, a siren light being created) for many updates with `memtrace` and replay it with the recorded frame times. evidence: [particles.md §2026-10-18T09:40](../../re/particles.md#ts-20261018-094004).
- [ ] PART-6 — The ground height of bouncing particles needs the world ray cast 0x22f978 (3 casts, collision world `*0x376b9c`) from the level collision. blocks: PART-S3. evidence: [particles.md §2026-10-18T09:40](../../re/particles.md#ts-20261018-094004).
- [ ] PART-7 — Remaining definition flags: the draw 0x2aeaf0 of flag 0x800, planar flag 8 (no shipped record has either), the colour lookup 0x221064 of flag 0x20000 (4 records) and the texture swap of the setter key 4. evidence: [particles.md §2026-10-18T09:40](../../re/particles.md#ts-20261018-094004).

## 6. Findings summary

- Container: [particles.md §2026-10-13T09:00](../../re/particles.md#ts-20261013-0900); spawner, init, emitter update, step, texture index: [particles.md §2026-10-13T09:01](../../re/particles.md#ts-20261013-0901), [particles.md §2026-10-13T09:03](../../re/particles.md#ts-20261013-0903), [particles.md §2026-10-13T09:08](../../re/particles.md#ts-20261013-0908).
- Runtime, draw, callers and the oracle check: [particles.md §2026-10-18T09:40](../../re/particles.md#ts-20261018-094004).
- The level object class `particles`: [particles-objects.md §2026-11-02T22:39:38](../../re/particles-objects.md#ts-20261102-223938).

## 7. Code and tests

- `src/assets/particles.cpp` (definitions), `src/runtime/particles.*` (system), `src/runtime/particle_object.*` (class `particles`, CTests `particle-object`, `particle-object-real-data`), `src/render/particles.*` with `src/render/shaders/particle.{vert,frag}.glsl` (drawing), `src/game_particles.*` and the option `--particle` (chase-game glue, see docs/commands.md); CTest `particles-core`, `particles-real-data`, `game-options-core`, `assets-core`, `assets-real-data`.
