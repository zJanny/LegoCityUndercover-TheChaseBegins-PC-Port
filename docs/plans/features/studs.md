# Studs (collectible coins)

## 1. Scope

Studs as the original handles them: pickup-system sprites (object type 0x3d, pickuptype 0..3: silver, gold, blue, purple), drawing and spin, pickup radius and skip counter, collect timer, counter values and limits. The on-screen counter belongs to [hud-ui.md](hud-ui.md); the collect sound to [audio.md](audio.md).

## 2. Status

IN_PROGRESS, updated 2026-10-13T21:29:01+0200. Studs spin, are picked up and counted in `chase-game`; the HUD part is missing.

## 3. Verified behaviour

- Sprite sheet `pickups_commonstud` (4x4) spun at 15 frames per second; values 10/100/1000/10000 added to the u64 counter 0x593418 after the 1.0 s collect timer.
- `studs-real-data` replays 10 oracle traces frame by frame.
- Level census: lvl_00 97 studs (silver 48, gold 46, blue 3), lvl_01 1040 (silver 495, gold 468, blue 73, purple 4).

## 4. Stand-ins

- STUD-S1 Pickup radius 1.5 and player pickup point (0, 1.153, 0.08) are oracle readings, not derived from their writers (0x2744d0, entity +0x60).
- STUD-S2 The skip-counter limit 17 stands in for FUN_0023ca4c.
- STUD-S3 The sprite clock phase is `chase-game`'s own.
- STUD-S4 Collected studs vanish instead of flying to the HUD counter.
- STUD-S5 Sprites are alpha-tested without the original's blend state.

## 5. Open items

- [ ] STUD-1 — Missing: multiplier mask, magnet, group enabling, dropped studs. evidence: [studs.md §2026-10-13T17:09](../../re/studs.md).
- [ ] STUD-2 — The on-screen counter and studs flying to it are missing. needs: HUD-1. evidence: [studs.md §2026-10-13T17:09](../../re/studs.md).
- [ ] STUD-3 — The collect sound was listed as missing in the handoff written before the sound merge; the sounds merge (2026-10-13T17:54) plays the original stud ids, so confirm against the oracle and close. see AUDIO-5. evidence: [audio.md §2026-10-13T17:54](../../re/audio.md).
- [ ] STUD-4 — How a group is enabled (0x28a2cc sequencer, `EnableStudsByGroup` and `SpawnStuds` script functions, `StudSpawn*` parameters, meaning of `GroupOrder` +0x33). needs: SCRIPT-1. evidence: [studs.md §2026-10-13T17:09](../../re/studs.md).
- [ ] STUD-5 — Where the per-frame clock origin of 0x5165c8 comes from and whether the game restarts it at level start. evidence: [studs.md §2026-10-13T17:09](../../re/studs.md).
- [ ] STUD-6 — The per-level stud total; whether the radius 1.5 holds for all characters and abilities (setter 0x2744d0). evidence: [studs.md §2026-10-13T17:09](../../re/studs.md).
- [ ] STUD-7 — No oracle screenshot of the same spot (the software renderer aborts when loading the state; an OpenGL screenshot needs `--rpc`).

## 6. Findings summary

- Studs are pickup-system sprites, not models; the `meshname` of the stud classes is never loaded: [studs.md §2026-10-13T17:09](../../re/studs.md), [objects.md §2026-10-13T17:09](../../re/objects.md).
- Pickup pool, drawing, pickup rule and counter (object type 0x3d registered at 0x2ed0b4, constructor 0x17f8e0): [studs.md §2026-10-13T17:09](../../re/studs.md).

## 7. Code and tests

- `StudField` (`src/runtime/studs.*`), `StudSprites` (`src/render/studs.*`), `DrawableObjects::studs`; counter printed by `--script`.
- CTest: `studs-core`, `studs-real-data`. Trace dir `CHASE_STUDS_TRACE_DIR`. Scenarios: `tools/oracle/studs-pickup.txt`, `studs-shot.txt`.
