# Studs (collectible coins)

How the level's stud objects are created, drawn, collected and counted. Replica: `StudObject` / `drawableObjects` (src/assets/level.cpp), `StudField` (src/runtime/studs.*), `StudSprites` (src/render/studs.*), wiring in src/game.cpp. Level objects in general: [objects.md](objects.md).

Subtopics: [studs-drops.md](studs-drops.md)

## Current state

- Stud classes are pickup objects (object type 0x3d, `pickuptype` 0 silver, 1 gold, 2 blue, 3 purple), drawn as camera-facing sprites from `Pickups_commonstud.tga`, never as models. [§2026-10-13T17:09](#ts-20261013-170941)
- The record (0x4c bytes) sits 0.56 above `initialposition`; `DisabledOnStartUp` hides it until a group sequencer enables it. [§2026-10-13T17:09](#ts-20261013-170941)
- Sprite frame comes from a clock of 3000 ticks per second (15 frames per second); quad half size 0.19, tint per type. [§2026-10-13T17:09](#ts-20261013-170941)
- Pickup rule 0x180168: 3D distance from the stud to the player's pickup point (offset (0, 1.153, 0.08)) below radius 1.5; the skip counter limit is 30 (not 17). [§2026-10-13T17:09](#ts-20261013-170941), [§2026-10-13T21:35](#ts-20261013-213500)
- Collecting flies the stud to the HUD sink for 1.0 s (step `(target - pos) / (30 * timer)`), then adds 10 / 100 / 1000 / 10000 (times the multiplier mask) and sets the HUD bar. [§2026-10-13T21:35](#ts-20261013-213500), [§2026-10-13T17:09](#ts-20261013-170941)
- The collect sound is played: 64 for silver and gold, 63 for blue and purple. [§2026-10-13T21:48](#ts-20261013-214838)
- Open: groups and `GroupOrder`, magnet, the callers of the stud drop and the dropped heart, multiplier mask, clock origin, radius for other characters, gold/blue/purple recorded pickups.
- Dropped studs, their callers and which classes drop studs: [studs-drops.md](studs-drops.md) (Current state there).

<a id="ts-20261013-170941"></a>

## 2026-10-13T17:09:41+02:00 — Pickup pool, sprite drawing, pickup rule and counter (REA Ghidra 12.1.2 pseudo-code, llvm-objdump, oracle port 24770 without RPC)

Corrected by [studs.md §2026-10-13T21:35](#ts-20261013-213500): the skip limit is 30
Corrected by [studs.md §2026-10-13T21:48](#ts-20261013-214838): the collect sound is played

**[Corrects objects.md §2026-10-13T15:39:15](objects.md#ts-20261013-153915)** ("Left out and why", "How studs ... are created"): the stud classes have a `meshname` (`xx_goldstud` ...) but the game never loads it as a model. The names are not used by any code (no `xx_` string in the executable). The stud class is a pickup object (object type 0x3d) that the pickup system draws as a camera-facing sprite.

**Classes and parameters.** `levels/pickups/pickup_stud{silver,gold,blue,purple}.blvl` and `levels/lepickups/lepickup_stud{silver,gold}.blvl` list (dump with parseLevel, names from code.bin strings): `ObjectType` = 0x3d (flags 5, class value), `pickuptype` = 0 silver, 1 gold, 2 blue, 3 purple (flags 5), slot parameters `DisabledOnStartUp` (float, flags 6), `GroupOrder` (float, flags 6), `initialposition`, `initialorientation`, `meshname` (flags 6). `pickups/pickup_heart` has the same object type with pickuptype 4; `pickup_redbrick`, `pickup_goldbrick`, `pickup_charactertoken` have type 0x84 (not studs). The lepickup classes carry the same values as the pickups ones.
- Level census (parseLevel): lvl_00 97 studs (silver 48, gold 46, blue 3, purple 0), lvl_01 1040 (silver 495, gold 468, blue 73, purple 4; the lepickup classes are 31 of them). Studs enabled at the start (`DisabledOnStartUp` absent or 0): lvl_00 97, lvl_01 881 (silver 416, gold 419, blue 42, purple 4).

**Class registration.** 0x2ed0b4 (ev_b1ccfa81…2d8) registers object type 0x3d with the constructor 0x17f8e0 (`FUN_00246240(0x3d, ...)`), clears the group sequencers at 0x58f710 and fills the type table `0x58ef7c + type * 0x8c` for types 0..10 (sprite sheet `Sprites/CommonLEGO/Pickups_commonstud.tga` for types 0..3, `Pickups_heart.tga` for 4). Table fields (the values at the gameplay start were read back in the oracle and equal the constants of 0x2ed0b4):
- +0x44 tint 0xAABBGGRR: silver 0xffeeeeee, gold 0xff24bcff, blue 0xffff4240, purple 0xd0ff42a0. +0x48 size factor 0 (studs; 1.0 for the heart). +0x4c/+0x50 UV origin (0, 0), +0x54/+0x58 UV scale (1, 1).
- +0x60 collect time 1.0 s, +0x64 magnet group 0xffffffff (magnet off), +0x68 0.1, +0x6c 5.0, +0x70 1.0, +0x80 1.0, +0x84 0.3 (bounce factor).
- +0x88 u16 landing sound 65 (`pickups/coinsland`), +0x8a u16 collect sound 64 (`pickups/coin1`) for silver and gold, 63 (`pickups/coinblue`) for blue and purple (tablessoundfx rows, `chase-tabledump`).

**Constructor 0x17f8e0 (objdump).** The record is 0x4c bytes in a pool (`*(*(*(*0x376c78 + 0x28) + 0x10) + *(0x58ef1c + 0x1c) + 8)`, in the oracle 1130 records at 0x8dec3d0). It stores: position = initialposition with y + 0.56 (literal 0x3f0f5c29), +0x44 state 1, +0x45 a category byte (u16 at +14 of the class lookup 0x22d64c, plus 1), +0x46 = `pickuptype`, and reads `DisabledOnStartUp` into +0x32/+0x34 and `GroupOrder` into +0x33/+0x35; a non-zero `DisabledOnStartUp` sets the state to 0 (hidden until a group sequencer enables it, 0x28a2cc). Check against the oracle pool at the gameplay start: 1038 of the 1040 lvl_01 studs matched a record by position (+ 0.56 in y); the record state is 1 exactly when `DisabledOnStartUp` is absent or 0, the +0x32 byte equals its value, the record type equals pickuptype (silver 495 -> 0, gold 468 -> 1, blue -> 2, purple -> 3).

**Drawing.** 0x17fc38 (ev_651a36a2…cfbb4) per frame: frame index = bits 6..9 of `(clock * 0x51eb851f) >> 32`, `clock` = `*(*0x37750c)` evaluated through 0x256af4; u = (index & 3) * 0.25, v = (index >> 2) * 0.25 (stored in 0x58f580..0x58f58c, shared by every pickup). The clock object at 0x5165c8 advances 210 per update at dt 0.07, i.e. 3000 ticks per second (memtrace, 20 updates), so the sheet shows 15 frames per second and one turn takes 3200 ticks = 1.067 s. Idle pickups (state 1 or 2) inside the view are collected per type and drawn by 0x1d1bb8 (ev_40957fd2…2b50): a quad of half size 0.19 * `*0x37728c` (1.0, read) along the camera's right and up axes (camera node rows 0 and 1), vertex colour = the type's tint, UV = origin + frame cell * scale, depth range up to 2000. The texture is the 128x128 sheet pickups_commonstud (4x4 frames of a spinning grey stud, `artifacts/textures/sprites/commonlego/pickups_commonstud.btga.png`).
- The render state (blend, depth, alpha test: 0x252254 / 0x254d90(5, 0x67)) was not decoded.

**Pickup rule.** Update 0x180168 (ev_cb51b368…c9ac468; called once per game update with param_1 = 0, param_2 = 0 in the cases seen) walks the pool ranges of the active rooms. For a state 1 or 2 record it first decrements the skip counter +0x30 and tests only when the counter is below 1: distance = Euclidean 3D distance (0x2507dc) between the stud centre (record +0..8) and the player's pickup point = player node matrix applied to the offset at entity +0x60. Collected when radius > distance with radius `*0x3772b0`; otherwise the counter becomes `(distance - radius) / *0x37753c` (int, truncated) limited to FUN_0023ca4c. Values read in the oracle at the gameplay start: radius 1.5 (0x3fc00000), `*0x37753c` 0.23, entity +0x60 = (0, 1.153, 0.08), so the point is 1.153 above the feet and 0.08 along the facing direction. The skip counters of the pool were 16 or 17 (studs within the loaded rooms) and 0 (rooms not updated).
- Measured: the skip counter read 11 after the first update that followed the teleport and fell by 1 per update; the stud inside the radius was collected in the update in which it reached 0 (update 42, 11 updates later). At distances from the pickup point (along x) 1.40, 1.45, 1.49 the stud was collected, at 1.505 and 1.52 not; the same vertically (1.40 and 1.49 yes, 1.52 no; the falling player changed the vertical distance in those runs), diagonal 1.414 and 1.485 yes. Replayed frame by frame by `studs-real-data` (10 traces, 30 updates each, state of both records, skip counter and counter).
- Magnet: if the type's +0x64 (0xffffffff by default) is 0 or equals the record's group the record enters state 3 inside radius +0x6c (5.0) and flies towards the player's `chest` joint; the condition that changes +0x64 was not traced.

**Collect and counter.** 0x2277c4 (ev_3bfba678…3d99e): state 4, timer +0x3c = the type's +0x60 (1.0 s), plays the type's collect sound (`FUN_0023fa54(id, record, index, "Pickup collected")`) and prepares the flight to the HUD (screen-space target built from camera data; not decoded further). In the update, state 4 subtracts dt from the timer each update; while it is above 0 the record is drawn flying towards the stud counter (0x1c3fe4); when it runs out, 0x28a128 (ev_eb6221cf…ac1) adds the value and sets the state to 0: silver 10, gold 100 (also type 4), blue 1000, purple 10000, each times 2, 4, 6, 8 and 10 for every set bit of the multiplier mask `*0x3772b4` (bit indices at 0x377279..0x37727d) unless record flag +0x49 bit 0 is set; the u64 counter at 0x593418 (low) / 0x59341c (high) is clamped at 4200000000. The save file's u64 at +0x5148 (save-format.md) is the matching field. Measured: one silver stud collected at update 42, state 4 for the next 14 updates (dt 0.07), counter 0 -> 10 at update 57 (15 updates = 1.05 s). A newly collected record does not run the state 4 branch in the same update.
- Other counters near it (0x593420..0x59342c, flag 0x593438) are the per-level stud total and the "stud bar full" state; not traced.

**Implemented.** `StudField::update` (idle: skip counter and distance test; collecting: timer; done: value added), `studSpriteFrame`, `studPickupPoint`, `StudSprites` (quads in the shared shader, alpha test), chase-game keeps the pickup point from the player's position and facing, draws idle studs and prints `studs: counter ...` after a `--script` run. Tests: `studs-core` (synthetic), `studs-real-data` (traces `artifacts/oracle/studs-traces`, scenario tools/oracle/studs-pickup.txt), counts in `assets-real-data`.

**Stand-ins (all listed in [features/studs.md](../plans/features/studs.md)):**
- `kMeasuredStudPickupRadius` (1.5) and `kMeasuredPlayerPickupPoint` are values read in the oracle, not derived from their writers (0x2744d0 with its caller 0x1abe60 sets the radius; the writer of entity +0x60 was not found). They may change with abilities or characters.
- `kProvisionalStudSkipLimit` 17: the observed largest skip counter; the limit is the result of FUN_0023ca4c (0x23ca4c, reads a thread-local rate with the default 30), not decoded.
- The clock phase of the sprite animation starts at 0 when chase-game starts (`provisionalStudClockSeconds`); the original's clock object (0x5165c8) has its own origin.
- Collected studs vanish at once in chase-game; the original draws them flying to the HUD counter for the 1.0 s before the value is added (the counter itself follows the original timing).
- Rendering: alpha-tested sprites in the model shader, no blend state, no depth sorting (original: per-type batches, state set by 0x254d90(5, 0x67)).
- Not replicated: multiplier mask (always x1), magnet, groups (hidden studs stay hidden), dropped studs (state 2: gravity, bounce factor 0.3, landing sound 65), the collect sound (id recorded in `StudCollected::sound`; chase-game has no audio).

Unknowns:
- Where the per-frame clock origin of 0x5165c8 comes from, and whether the game restarts it at level start.
- How a group is enabled (0x28a2cc sequencer, `EnableStudsByGroup` and `SpawnStuds` script functions, `StudSpawn*` parameters of the spawner classes), and the meaning of `GroupOrder` (+0x33).
- The stud counter's display (HUD, 0x15b2f0 loads the same sheet for the stud bar) and the per-level stud total.
- Whether the radius 1.5 holds for all characters and abilities (setter 0x2744d0).
- The oracle screenshot of the same spot was not taken: the software renderer aborts when loading the state (three attempts, SIGABRT/SIGSEGV) and an OpenGL screenshot needs `--rpc`, which another agent used.

<a id="ts-20261013-213500"></a>

## 2026-10-13T21:35:00+02:00 — Flight to the HUD counter, banking hook (REA, oracle port 24690 with RPC; details in [hud.md](hud.md))

**Corrections of the entry above**
- "prepares the flight to the HUD (screen-space target built from camera data; not decoded further)" is decoded: the flight is the state 4 branch of 0x180168. Every update `timer -= dt`; while the timer is above 0 the record moves `(target - position) / (30 * timer)`, the target being the HUD counter's sink 0 (screen point, depth 5.5) unprojected by 0x24c1d0 with the top-screen camera; the record is drawn at its moving position. Reproduced against `bar1.mem` (tools/oracle/hud-bar.txt): the positions of updates 19..24 agree to 2e-3, the camera read in the oracle unprojects the sink to the converged target (864.6103, 20.8361, 1064.3806). Native: `StudField::setFlightTarget`, `unprojectTopScreen`; chase-game draws the flying studs.
- "0x28a128 adds the value ... " ends with a hook call `(*0x377298)(type, record)` = 0x29c090 that sets the HUD stud bar to the new total (`SetDisplay` 0x22ddf4) and shows it for 2 s; there is no roll-up for pickups (the number jumps to the new total and the icon bounces for 5 updates). Native: `StudField::takeBanked` hands the new total to `Hud::studsBanked`.
- The stud counter display listed under Unknowns is the stud bar of hud.md. 0x15b2f0 is the bar's init (it loads `Pickups_commonstud.tga` for the bar's icon).
- **[Corrects studs.md §2026-10-13T17:09:41](#ts-20261013-170941)** (stand-in `kProvisionalStudSkipLimit` 17, first entry): the limit of the skip counter is `FUN_0023ca4c()`, which returns 30 (0x376af8 +0x8 and +0x10 are zero in the oracle; `kStudSkipLimit`). Measured: the whole pool over 45 updates from the gameplay start (`skip1.mem`, tools/oracle/hud-skip.txt): counters of idle studs run 11 -> 0 and are set to 30 when tested again (30 seen in update 10, then 29, 28 ...). The earlier "16 or 17" were the values of a few updates after the level's first test. The same 30 is the divisor of the flight step, `kStudFlightRate`.

Unknowns
- Whether other stud types fly to sinks 1 or 2 (the type table +0x5c byte is 0 for silver, gold, blue and purple), the blend state of the flying sprite, dropped and magnet studs.

<a id="ts-20261013-214838"></a>

## 2026-10-13T21:48:38+02:00 — Correction: the collect sound is played

**[Corrects studs.md §2026-10-13T17:09:41](#ts-20261013-170941)** ("the collect sound … chase-game has no audio"): since the sound merge (docs/re/audio.md 17:54) chase-game plays the stud's own collect id (`kStudCollectSounds` {64, 64, 63, 63} for silver, gold, blue, purple in src/runtime/studs.hpp, emitted by src/runtime/studs.cpp, played through `studCollectCall`). The silver pickup is replayed against a recorded oracle pickup (`audio-real-data`, trace `stud.csv`, trap 0x227880, id 64); the ids 63 and gold's 64 come from the table/code only (STUD-3 in [features/studs.md](../plans/features/studs.md)).

Unknowns:
- A recorded original pickup of a gold, blue or purple stud.
