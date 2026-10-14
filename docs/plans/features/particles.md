# Particles

## 1. Scope

`particledefs.bprt` particle definitions: container, spawner fields, per-particle init, emitter update, particle step (key lists, gravity, drag, bounce, size growth) and the texture hash-to-index conversion. Not covered: drawing and emitting particles at run time.

## 2. Status

IN_PROGRESS, updated 2026-10-13T21:29:01+0200. Definitions decode in C++ (`parseParticleDefs`, `pjwHash`); no particles are emitted or drawn by any program.

## 3. Verified behaviour

- Container and field use recovered from the spawner (0x20db0c emitter update, 0x222550 init, 0x221e9c step); hash-to-index conversion at 0x1ed440.

## 4. Stand-ins

- None (nothing is drawn).

## 5. Open items

- [ ] PART-1 — Particle rendering (emitters, step, drawing) is not implemented; no level object creates emitters (`xx_particleemitter` creation is untraced, see LEVOBJ-5). blocks: SCRIPT-38. evidence: [particles.md §2026-10-13T09:05](../../re/particles.md#ts-20261013-0905), [particles.md §2026-10-13T09:07](../../re/particles.md#ts-20261013-0907).

## 6. Findings summary

- Container: [particles.md §2026-10-13T09:00](../../re/particles.md#ts-20261013-0900); spawner, init, emitter update, step, texture index: [particles.md §2026-10-13T09:01](../../re/particles.md#ts-20261013-0901), [particles.md §2026-10-13T09:03](../../re/particles.md#ts-20261013-0903), [particles.md §2026-10-13T09:08](../../re/particles.md#ts-20261013-0908).

## 7. Code and tests

- `src/assets/particles.cpp`; CTest `assets-core`, `assets-real-data`.
