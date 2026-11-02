# Script system: missions, mission 1-4, the roof scene and the natives of the roof and the chase

## 1. Scope

The roof scene of mission 1-4 (`1-4_SO_Phase2`, `1-4_AI_Goon_10_Run_Off_Script`, `1-4_RooftopReached`, `1-4_Jump_On_Roof`, the FallArrest bound scripts) and the four natives of the roof and the chase that had no handler (BulletTime, VehicleSetAI, VehicleSetChased, SpawnDebris2Colour). Split from [script-system-missions-1-4](script-system-missions-1-4.md), which keeps the gap list and ranking of the mission; the chase itself is SCRIPT-415 and SCRIPT-422 there.

## 2. Status

IN_PROGRESS, updated 2026-11-02T23:12:54+01:00. In the roof scene BulletTime slows the whole game update to one clock tick per update and back, with the slow-motion sounds; the goon's ladder kick fires the particle objects `LadderKick` and `LadderLand`; the FallArrest bounds are collected but test no characters yet (SCRIPT-425). VehicleSetAI, VehicleSetChased and SpawnDebris2Colour run as stand-ins (SCRIPT-S323): the stolen car's driver has no effect yet, the skylight burst starts bricks. Open: SCRIPT-414, SCRIPT-425 and SCRIPT-426.

## 3. Verified behaviour

- The four natives are pinned by CTest `bullet-time-vehicle-debris-natives` (made-up scripts with the argument lists of the shipped calls, the scaled scheduler step), `game-bullet-time-1-4` (the roof scene `1-4_SO_Phase2`: sound 97, the time scale 0.01, then 97 and 98 stopped, 99 and the scale 1) and `game-skylight-debris-1-4` (`1-4_Hatch_Open` bursts six bricks after the four skylights break). evidence: [scripts-missions-1-4.md §2026-11-02T22:40:54](../../re/scripts-missions-1-4.md#ts-20261102-224054).
- The ladder kick: the class `particles` follows the code read (CTest `particle-object`), `LadderKick` (effect 148), `LadderLand`, `BoosterLand` (effect 11) and the six FallArrest bounds of lvl_01 are pinned by `particle-object-real-data`; in `chase-game` the roof scene starts `LadderKick` before the ladder platform comes down and `LadderLand` after it (`game-ladder-kick-1-4`, port update numbers). evidence: [particles-objects.md §2026-11-02T22:39:38](../../re/particles-objects.md#ts-20261102-223938).

## 4. Stand-ins

- SCRIPT-S323 The natives of the roof scene and the chase run partly as stand-ins. BulletTime steps at the start of the level update (the original's order is not read) and scales the whole level update, HUD and camera included; the stream pause, the flag branch and the mode byte calls are not ported (SCRIPT-426). Its real clock is the scheduler's clamped period, not the original's timer, and the sound 98 state test runs only with an open audio device. VehicleSetAI and VehicleSetChased fill the traffic AI driver record of the car (no rotation in the node matrix, `TrafficHeightOffRoad` 1.0, route distance 0) and nothing steps the driver, so the stolen car does not drive. SpawnDebris2Colour starts its bricks at the object's position, without the box centre and half extent of its entity and without the original's room-flag check (it skips the burst for rooms with certain flags). needs: SCRIPT-422, SCRIPT-426. evidence: [scripts-missions-1-4.md §2026-11-02T22:40:54](../../re/scripts-missions-1-4.md#ts-20261102-224054).
- SCRIPT-S325 The class `particles` is ported only for TriggerObject's messages and the one-shot timer: the endless self-start, `ReTriggerTime` above 0 and -5, message 0xfb, `Instance` sharing, the parent node, path, damage, sound, shake, room test, the visibility delay and the objects nested in class files are not, an ended emitter counts as gone (its callback is not read) and the class update runs where the port chose. FallArrestBoundSystem collects its bounds but has no engaged characters to test. needs: SCRIPT-425, PART-3. evidence: [particles-objects.md §2026-11-02T22:39:38](../../re/particles-objects.md#ts-20261102-223938).

## 5. Open items

- [ ] SCRIPT-414 — The rest of the roof scene: Booster Strange's `AISuperJump` flights (height is a stand-in, SCRIPT-103), ShowCharacterIntro against the oracle (HUD-30), `1-4_SO_LadderFallen` (fired by nothing in lvl_01), the music scripts and `sfx_rooftime`; the ladder kick and the FallArrest reader are ported (SCRIPT-S325, SCRIPT-425). Rank 2. needs: SCRIPT-410. evidence: [scripts-missions-1-4.md §2026-11-01T15:04:22](../../re/scripts-missions-1-4.md#ts-20261101-150422).
- [ ] SCRIPT-425 — FallArrestBoundSystem in play: the engaged table (who adds and removes goons), the airborne flag and fall of a thrown goon, so that `fallArrestUpdate` marks it and the takedown enters AUTO ARRESTED; its sound. Rank 2. needs: COMBAT-8. blocks: SCRIPT-S325. evidence: [scripts-missions-1-4.md §2026-11-02T22:40:54](../../re/scripts-missions-1-4.md#ts-20261102-224054).
- [ ] SCRIPT-426 — The rest of BulletTime: the other reader of the scale, which processes (HUD, character intro, camera) it slows, the stream pause, the flag branch, the mode byte calls, the other callers (`SlowMotion*`) and the slow motion's length against the oracle. blocks: SCRIPT-S323. uses: BulletTime. evidence: [scripts-missions-1-4.md §2026-11-02T22:40:54](../../re/scripts-missions-1-4.md#ts-20261102-224054).

## 6. Findings summary

- BulletTime, VehicleSetAI, VehicleSetChased and SpawnDebris2Colour read in the executable: [scripts-missions-1-4.md §2026-11-02T15:04:48](../../re/scripts-missions-1-4.md#ts-20261102-150448).
- The roof scene: the ladder kick, FallArrestBoundSystem and BulletTime's time scale and sounds: [scripts-missions-1-4.md §2026-11-02T22:40:54](../../re/scripts-missions-1-4.md#ts-20261102-224054); the class `particles`: [particles-objects.md §2026-11-02T22:39:38](../../re/particles-objects.md#ts-20261102-223938).

## 7. Code and tests

- `src/runtime/bullet_time.*`, `vehicle_ai_natives.*`, `script_debris.*`, `script_runner_bullet_time_vehicle_debris_natives.cpp` (the four natives), the time scale in `scheduler.*` and `src/game_level_natives.cpp`, CTests `bullet-time-vehicle-debris-natives` (made-up scripts, no game data), `game-bullet-time-1-4` and `game-skylight-debris-1-4` (slow).
- `src/runtime/particle_object.*` (class `particles`), `fall_arrest_bounds.*`, glue `src/runtime/game_particle_objects.cpp` (log lines `particle objects: N, fall arrest bounds: M`, `particle object NAME: effect N started` / `refused` / `stopped over F s` / `disabled`); CTests `particle-object`, `particle-object-real-data` and `game-ladder-kick-1-4` (slow), the last two `needs-game-data`.
