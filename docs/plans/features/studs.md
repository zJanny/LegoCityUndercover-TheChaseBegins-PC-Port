# Studs (collectible coins)

## 1. Scope

Studs as the original handles them: pickup-system sprites (object type 0x3d, pickuptype 0..3: silver, gold, blue, purple), drawing and spin, pickup radius and skip counter, collect timer, counter values and limits. The on-screen counter belongs to [hud-ui.md](hud-ui.md); the collect sound to [audio.md](audio.md).

## 2. Status

IN_PROGRESS, updated 2026-10-13T21:46:46+0200. Studs spin, are picked up, fly to the HUD counter and are counted and shown in `chase-game`. Resolved and removed: STUD-S2 (skip limit is the read 30), STUD-S4 (studs fly to the counter), STUD-7 (oracle pictures exist).

## 3. Verified behaviour

- Sprite sheet `pickups_commonstud` (4x4) spun at 15 frames per second; values 10/100/1000/10000 added to the u64 counter 0x593418 after the 1.0 s collect timer.
- `studs-real-data` replays 10 oracle traces frame by frame.
- Skip counter limit 30 (`FUN_0023ca4c`), read in the whole-pool trace `skip1.mem`. Flight to the counter: each update `(target - position) / (30 * timer)` towards the sink unprojected at depth 5.5, positions reproduced to 2e-3 (`studs-core`). The new total is handed to the HUD ([hud-ui.md](hud-ui.md)).
- Level census: lvl_00 97 studs (silver 48, gold 46, blue 3), lvl_01 1040 (silver 495, gold 468, blue 73, purple 4).

## 4. Stand-ins

- STUD-S1 Pickup radius 1.5 and player pickup point (0, 1.153, 0.08) are oracle readings, not derived from their writers (0x2744d0, entity +0x60).
- STUD-S3 The sprite clock phase is `chase-game`'s own.
- STUD-S5 Sprites are alpha-tested without the original's blend state.

## 5. Open items

- [ ] STUD-1 — Missing: multiplier mask, magnet, group enabling, dropped studs. evidence: [studs.md §2026-10-13T17:09](../../re/studs.md#ts-20261013-170941).
- [ ] STUD-3 — Blue and purple collect sound (id 63, coinblue) has no recorded oracle pickup. Silver (id 64) is replayed from a recorded pickup (`audio-real-data`, trace `stud.csv`); gold (id 64) has no recording either. see AUDIO-5. evidence: [audio.md §2026-10-13T17:54](../../re/audio.md#ts-20261013-175424).
- [ ] STUD-4 — How a group is enabled (0x28a2cc sequencer, `EnableStudsByGroup` and `SpawnStuds` script functions, `StudSpawn*` parameters, meaning of `GroupOrder` +0x33). needs: SCRIPT-1. evidence: [studs.md §2026-10-13T17:09](../../re/studs.md#ts-20261013-170941).
- [ ] STUD-9 — `EnableStudsByGroup(group, value)` only records the request (the sequencer entry 0x58f710 and its per-frame update 0x28a2cc are not ported; `value` is 6 or 8 in lvl_01); hidden studs stay hidden. blocks: SCRIPT-64. evidence: [script-natives-progress.md §2026-10-14T14:41](../../re/script-natives-progress.md#ts-20261014-144155).
- [ ] STUD-10 — `AddStuds` adds to the counter (`StudField::addToCounter`) but not the timestamp +0x44 and the HUD hook 0x240fc0 of 0x1584e0; the HUD does not show the added amount. evidence: [script-natives-progress.md §2026-10-14T14:41](../../re/script-natives-progress.md#ts-20261014-144155).
- [ ] STUD-5 — Where the per-frame clock origin of 0x5165c8 comes from and whether the game restarts it at level start. evidence: [studs.md §2026-10-13T17:09](../../re/studs.md#ts-20261013-170941).
- [ ] STUD-6 — The per-level stud total; whether the radius 1.5 holds for all characters and abilities (setter 0x2744d0). evidence: [studs.md §2026-10-13T17:09](../../re/studs.md#ts-20261013-170941).
- [ ] STUD-8 — Whether other stud types use other HUD sinks (the type table +0x5c byte is 0 for all four), the flying sprite's blend state, dropped and magnet studs. needs: STUD-1. evidence: [hud.md](../../re/hud.md).

## 6. Findings summary

- Studs are pickup-system sprites, not models; the `meshname` of the stud classes is never loaded: [studs.md §2026-10-13T17:09](../../re/studs.md#ts-20261013-170941), [objects.md §2026-10-13T17:09](../../re/objects.md#ts-20261013-170941).
- Flight, banking hook and the skip limit: [studs.md §2026-10-13T21:35](../../re/studs.md#ts-20261013-213500); the stud bar: [hud.md](../../re/hud.md).
- Pickup pool, drawing, pickup rule and counter (object type 0x3d registered at 0x2ed0b4, constructor 0x17f8e0): [studs.md §2026-10-13T17:09](../../re/studs.md#ts-20261013-170941).

## 7. Code and tests

- `StudField` (`src/runtime/studs.*`, flight target and banked totals), `unprojectTopScreen`, `StudSprites` (`src/render/studs.*`), `DrawableObjects::studs`; counter printed by `--script`.
- CTest: `studs-core`, `studs-real-data`. Trace dir `CHASE_STUDS_TRACE_DIR`. Scenarios: `tools/oracle/studs-pickup.txt`, `studs-shot.txt`, `hud-bar.txt`, `hud-skip.txt`, `hud-camera.txt`.
