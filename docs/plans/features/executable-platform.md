# Executable and platform layer (S1, S3)

## 1. Scope

Extraction of the verified executable and the analysis ELF (S1), and the focused ARM analysis of the platform layer (S3): startup chain, SVC stubs, services, paths, channels, the main loop (dispatcher 0x100448 / 0x10057c), the fixed-step scheduler 0x100f44, GSP interrupt thread and vsync wait, present function. Pad reading is [input.md](input.md); the runtime built on it is the player/camera features.

## 2. Status

S1 DONE, S3 IN_PROGRESS (the game frame function is still open). Updated 2026-10-13T21:29:01+0200.

## 3. Verified behaviour

- `tools/extract_game.py` verifies every SHA256 (image 078faf7e...59ad, `.code` f895faae...6923, ELF sha256 5152ed14...0881, 3 LOAD segments); the CCI is never modified. Python unit tests and real extraction pass.
- The scheduler is a clamped variable step; pacing globals measured with the oracle; the scheduler replica runs `chase-game` (3000 Hz clock, about 33 updates/s).
- Present function 0x25a6f0: minimum frame interval 0x529de8 (1/2), 0x122908 = APT command 0x004F0080; GSP interrupt thread and vblank counters (ids 2/3), wait modes 0x400..0x402, mode byte 0x52a1f4.

## 4. Stand-ins

- EXEC-S1 `chase-game` runs the scheduler replica on SDL timing; the original's frame rate in gameplay (30 or 60) is not decided.

## 5. Open items

- [ ] EXEC-1 — Which writer of 0x529de8 applies when. evidence: [arm-executable.md §2026-10-13T09:41](../../re/arm-executable.md#ts-20261013-094132).
- [ ] EXEC-2 — Whether C+0x10/+0x14 and the cap change in other states (gameplay, cutscenes); only one state was measured; who writes C+0x10..+0x1c (not found statically). evidence: [arm-executable.md §2026-10-13T10:35](../../re/arm-executable.md#ts-20261013-103531).
- [ ] EXEC-3 — Which screen produced 3 vblanks per update and the update rate during gameplay (30/60 fps undecided). evidence: [arm-executable.md §2026-10-13T09:38](../../re/arm-executable.md#ts-20261013-093842).
- [ ] EXEC-4 — The game frame function and the rest of the main loop (S3 remains IN_PROGRESS). evidence: [arm-executable.md §2026-10-13T10:45](../../re/arm-executable.md#ts-20261013-1045).

## 6. Findings summary

- Startup chain, SVC stubs, services, paths: [arm-executable.md §2026-10-12T15:10](../../re/arm-executable.md#ts-20261012-151000), [arm-executable.md §2026-10-12T15:25](../../re/arm-executable.md#ts-20261012-152500).
- Scheduler fields and the oracle semantics: [arm-executable.md §2026-10-13T08:55](../../re/arm-executable.md#ts-20261013-0855), [arm-executable.md §2026-10-13T10:35](../../re/arm-executable.md#ts-20261013-103531).
- Vsync, counters, present: [arm-executable.md §2026-10-13T09:36](../../re/arm-executable.md#ts-20261013-093611), [arm-executable.md §2026-10-13T09:38](../../re/arm-executable.md#ts-20261013-093842), [arm-executable.md §2026-10-13T09:41](../../re/arm-executable.md#ts-20261013-094132).
- Container and version: [container-and-archive.md §2026-10-12T14:43](../../re/container-and-archive.md#ts-20261012-144319), [game-version.md](../../re/game-version.md).

## 7. Code and tests

- `tools/extract_game.py`, `tests/test_extract_game.py`, `src/runtime/scheduler.cpp`. `chase-game` is split into `src/game.cpp` (wiring, at most 400 lines, CTest `game-cpp-wiring`), `src/game_options.*`, `src/game_player.*`, `src/game_doors.*` and `src/game_level.*` (`LevelSession`: level setup and the per-update step in `src/game_level_update.cpp`).
- `python3 -m unittest discover -s tests -p 'test_*.py'`; CTest `runtime-core`.
