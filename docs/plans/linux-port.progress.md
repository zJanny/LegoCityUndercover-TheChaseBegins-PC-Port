# Progress — Linux x86_64 accessible port

## Status

- Overall: IN_PROGRESS
- Current Phase: IMPLEMENT
- Current Step: S4e runtime core — milestone M2 (the player character moves with the measured locomotion rules)
- Last Updated: 2026-10-13T13:59:00+02:00
- Branch: main
- Target Branch: Not specified
- Requested Scope: PLAN, IMPLEMENT, TEST; start reverse engineering and native port
- Plan: docs/plans/linux-port.md
- Plan Revision: 3
- Planning Baseline: no Git/source; supplied CCI only

## Summary

The executable and the full RomFS are extracted with hash verification. The FUSE asset archive, textures, sounds and model geometry decode, and each decoder has been checked against every shipped file. Levels, nav grids, skeletons, animation, cutscenes, localisation, fonts and data tables also decode in C++, and the development viewer shows levels and cutscenes. The game is not playable on PC yet: gameplay logic, rendering states, UI, scripts and saves are not recovered.

## Phase Status

| Phase | Status | Started | Completed |
|---|---|---|---|
| PLAN | COMPLETE | 2026-10-12T14:43:19.863039+02:00 | 2026-10-12T14:43:19.863039+02:00 |
| IMPLEMENT | IN_PROGRESS | 2026-10-12T14:43:19.863039+02:00 | - |
| TEST | IN_PROGRESS | 2026-10-12T14:51:33+02:00 | - |
| COMMIT | IN_PROGRESS | 2026-10-12T15:02:00+02:00 | - |
| MERGE | SKIPPED | - | - |

Commits authorized by CLAUDE.md "User specified workflows" (commit at logical steps; push only on request). Merge not requested.

## Implementation Progress

| Step | Description | Status | Notes |
|---|---|---|---|
| S1 | Verified executable extraction and analysis ELF | COMPLETE | R1; unit tests + real extraction 2026-10-12T14:51 |
| S2 | Native accessible input laboratory | COMPLETE | R2/R3; warning-free build, CTest 2/2 |
| S3 | Focused ARM analysis | IN_PROGRESS | R1; startup, SVC stubs, service-handle helper, services, paths recorded; game frame function still open |
| S4 | Recover gameplay and replace platform | IN_PROGRESS | R4; split into S4a–S4f (plan rev 3) |
| S4a | Native asset runtime (C++) | COMPLETE | FUSE, pooled, textures, models, levels, sounds; real-data totals equal Python |
| S4b | Level viewer (SDL2 + GL) | COMPLETE | chase-viewer; offscreen smoke test; placement awaits S4c |
| S4c | Level composition (.blvl) | IN_PROGRESS | loader replicated (Py+C++), placements in viewer; materials/lighting, streaming, other sections open |
| S4d | Audio playback | IN_PROGRESS | mixer + SDL sound test with loops; sound ids (tablessoundfx) decoded, play by id; script routing partly known |
| S4e | Gameplay | IN_PROGRESS | chase_runtime: scheduler 0x100f44 and pad pipeline replicas, checked against the running game (Azahar oracle); player entity located; locomotion not recovered |
| S4f | Saves/settings | IN_PROGRESS | chase_save: header, CRC, validate, atomic XDG storage, option fields, slot progress accessors, completion (0x1cd3dc, no cheat flags) |

## Current Work

### Current Step

M1, the first runtime milestone: a native loop (scheduler replica) that reads input through the game's pad pipeline, updates and renders a real level. Acceptance: chase-game loads lvl_00_policestation, prints updates/s and stick channel values matching the pipeline, offscreen smoke test passes. Not gameplay.

### What Is Being Done

Subagents in worktrees: chase-game front end; player stick-to-intent replica (0x2007a0); oracle scenario to reach gameplay. Results are reviewed and verified before merging.

### Files Being Modified

None.

### Expected Result

Next session picks up from Next Action.

## Completed Work

- [x] Target identity and decrypted-content probes
- [x] S1
- [x] S2
- [ ] S3
- [ ] S4

## Tests

### Passing

- `python3 -m unittest discover -s tests -p 'test_*.py' -v` — PASS; cwd repo; Python 3.14.7; synthetic fixtures; tested unborn HEAD + untracked source; at 2026-10-12T14:51:33+02:00; 9 tests OK; R1.
- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build --clean-first` — PASS, no warnings; cmake 4.4.4, GCC 16.2.1, SDL2 2.32.74; same revision; R2.
- `ctest --test-dir build --output-on-failure` — PASS 2/2 (input-core, input-sdl-smoke with SDL_VIDEODRIVER=dummy); same revision; R2/R3.
- `python3 tools/extract_game.py <CCI> --output artifacts/game` — PASS exit 0, 0.75 s; manifest: image sha256 078faf7e…59ad, .code sha256 f895faae…6923 (matches probe), decoded code 2650112 B = 647 pages (599 text/20 rodata/28 data), sha256 4b91f1e3…439e; ELF 2654208 B sha256 5152ed14…0881; `readelf -h -l` shows 3 LOAD (0x100000 RX, 0x357000 R, 0x36b000 RW memsz 0x239f68); CCI mtime/size unchanged; R1.
Read-only Python probe: exheader SHA256 and four ExeFS hashes match. cwd repository, Python3, unborn source, 2026-10-12T14:43:19.863039+02:00; does not test extractor implementation.

### Failing

REA open_binary CCI: unsupported format, not a provider setup fault. Use derived ELF.

### Not Yet Run

Game runtime unavailable (R4 oracle).

## Git

### Branch

main (feature/linux-port is the historical start branch)

### Commits

- `4642cf5` — Add verified 3DS executable extraction tool (S1)
- `db355f9` — Add SDL2 accessible input laboratory (S2)
- `a805bf3` — Add port plan, progress log and CLAUDE.md

Later commits: see `git log` (one commit per session-log entry).

### Uncommitted Changes

None at 2026-10-13T12:35+02:00.

## Decisions

### Decision: architecture and runtime oracle (2026-10-13T10:53+02:00)

**Question:** how to reach a playable game: execute the ARM code (emulation/recompilation) or reconstruct natively. **Decision:** native reconstruction in C++ (chase_runtime), with the installed Azahar emulator used only as a local oracle (tools/azahar_oracle.py) to measure the original game. **Reason:** executing the ARM code would rebuild Azahar; the oracle turns "unknown at run time" values into measurements and gives a reference for differential tests. The user's own image is only read; nothing game-derived is committed.

User approved Git init, no extra guidance, Linux devices emit keyboard/mouse. Use C++17/SDL2 and preserve ARM code as analysis artifact. Native lab is a foundation, not game runtime.

### Decision: commit authorization (2026-10-12T15:02+02:00)

**Question:** Plan section 16 left commits out of scope. **Decision:** User added to CLAUDE.md: commit at logical steps; push only when asked. **Reason:** Supersedes the no-commit instruction; merge/push remain unauthorized.

## Blockers

Full game logic, hardware APIs and asset formats not recovered yet.

## Deviations From Plan

- 2026-10-12: Plan section 16 said no commits. Actual: commits made per user's CLAUDE.md workflow rule. Plan revision 2 records this; no code steps reopened.

## Next Action

- 2026-10-13T14:00:43+02:00 — M2 reached in its measured scope (`ba0490f`): chase-game moves the player with the original's walk/run/sprint speeds, thresholds, run timer and turn rule; locomotion-real-data replays 33 oracle traces (1114 updates, max position error 7.8e-5). Lead check: a scripted 40-update run moves 10.1329 units, then the player runs into the lobby desk platform and the camera ends inside geometry, because height is flat and there is no collision.
- Next (critical path to a walkable level): (1) collision and ground height: find the level collision data and the game's ground/collision query (RE + oracle traces of y over the ramp seen in the loco traces); (2) the game camera law (camera yaw writer, follow distance/height, .cam records docs/re/data-tables-config.md); (3) where the locomotion speeds come from (0x2327b4 velocity path). Also: the keyboard walk modifier uses the key bound to the 3DS L button, which the game uses for the camera; give it its own binding when the camera is recovered.

- M1 status: chase-game runs the scheduler and pad pipeline replicas over lvl_00_policestation and shows the player character (char_police appearance, guessed) at Player1Start (traced, 0x2e6264). The mirrored-world defect is fixed: the game world is left-handed and chase_render's projection mirrors x ([docs/re/levels.md](../re/levels.md)); the free camera now uses the game's yaw convention (forward = (sin yaw, cos yaw)). The head attachment defect is fixed (2026-10-13T12:56+02:00): the head model is attached at the head joint's bind position with its bind orientation dropped ([docs/re/player.md](../re/player.md)); the draw-time consumer of the attachment was not read.
- M2 status (2026-10-13T13:59:00+02:00): chase-game moves the player with `locomotionUpdate` (src/runtime/player_locomotion.*): regime from the pad pipeline, run timer, sprint role, facing step, movement along the last intent. Differential test `locomotion-real-data` against 33 recorded traces: movement within 7.8e-5 u per update, timer exact, facing exact except 3 skipped updates ([docs/re/player.md](../re/player.md)). Provisional: the speeds are measured at the output (the walk/run/sprint animation data has no root travel), follow camera, flat height, no collision, assumed animation rate without blending.
- Next (2026-10-13T15:10:11+02:00): collision of objects and vehicles, falling and jumping, the slope slow-down of 0x1abe60 (ground normal ctrl+0x1ec is now recorded); level collision for the player is done (collision.md). Earlier: ground height and slope (nav grid or collision query 0x21a73c), the camera law (writer of 0x376894), animation blending and playback rate; then jump and other moves.
- Oracle fidelity: per-frame breakpoint stops inflate the game clock about 30 % against vblanks (free run 51.3 ticks per vblank, stepped 65.9); measure rates in free-run intervals. Open: a frame counter readable without breakpoints, to measure update rate in free runs.

- S4f: all completion inputs found (docs/re/save-format.md 09:25, total 711). chase-savecheck prints slot completion from the real tables (total 711); next: check its output against the percentage the game shows for the same save. Not modelled: cheat flags 0x375961/0x375962. Still open: options +0 nibbles and +3 bits 2..5 (no reader found); per-level block contents beyond the counters; how live state is copied into the save buffer.
- S3: vsync wait found (0x257a68, interrupt thread 0x1300c8); vblank counters are ids 2/3, default present waits one vblank; swap 0x25a6f0 enforces a minimum interval of word 0x529de8 (1 or 2 → 60/30 cap, 11 writers); open: which writer applies when, timer 0x529e30 unit, step C+0x10; callers of stick-to-D-pad 0x2289b8; readers of axis slots 16/17; other writers of channel 7.
- S4e: the code that applies .bxaml track properties; .cam weight source 0x21c1e4 and blend easing; Type bit 0x100 (no reader found); particle rendering (vertex/quad build) and a native particle simulation.
- Open unknowns are listed at the end of the matching docs/re/ files.

## Session Log

Reverse-engineering findings live in docs/re/ (see docs/re/README.md). Entries marked with an arrow were moved there.

- 2026-10-12T14:43:19.863039+02:00 → [docs/re/container-and-archive.md](../re/container-and-archive.md)
- 2026-10-12T15:10:00+02:00 — S3 focused analysis (REA, Hopper provider, ELF sha256 5152ed14…0881) → [docs/re/arm-executable.md](../re/arm-executable.md)
- 2026-10-12T15:25:00+02:00 — S3 DSP/service-handle follow-up → [docs/re/arm-executable.md](../re/arm-executable.md)
- 2026-10-12T15:25:37+02:00 — RomFS extraction (S1 extension, prerequisite for S4 assets) → [docs/re/container-and-archive.md](../re/container-and-archive.md)
- 2026-10-12T15:32:32+02:00 — FUSE (.fib) archive format and codec recovered → [docs/re/container-and-archive.md](../re/container-and-archive.md)
- 2026-10-12T15:40+02:00 — Texture container decoded (.btga/.bpng/.bdds) → [docs/re/textures.md](../re/textures.md)
- 2026-10-12T15:45+02:00 — Sound container decoded (.bwav) → [docs/re/audio.md](../re/audio.md)
- 2026-10-12T15:55+02:00 — Shared TT pooled-resource layout (all b* formats) → [docs/re/pooled-resources.md](../re/pooled-resources.md)
- 2026-10-12T16:10+02:00 — .bfnmdl model format decoded (REA, Ghidra 12.1.2 provider) → [docs/re/models.md](../re/models.md)

### 2026-10-12T16:30+02:00 — S4a part 1: native asset runtime (C++)

- Added the `chase_assets` library: src/assets/assets.hpp, fuse.cpp (FUSE archive, RefPack, CSV names, case-insensitive lookup), pooled.cpp (parsePooled, PoolCursor) and texture.cpp (all 14 PICA formats, ETC1/ETC1A4, cubemap strip). It has no SDL dependency.
- Tests:
  - tests/assets_test.cpp (CTest `assets-core`) mirrors the Python fixtures.
  - tests/assets_real_test.cpp (CTest `assets-real-data`, SKIP_RETURN_CODE 77 when the RomFS is absent) reads every archive entry, parses every pooled resource, decodes every texture, and compares 5 texture FNV-1a checksums with `tools/convert_textures.py --checksum`.
- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build` — PASS, 0 warnings (GCC 16.2.1).
- `ctest --test-dir build --output-on-failure` — PASS 4/4: input-core, input-sdl-smoke, assets-core and assets-real-data (9.1 s).
  - Real data: 12836 entries and 478387766 bytes, matching the Python totals; 12500 pooled resources; 3991 textures; 5/5 checksums match.
  - The skip path (`assets-real-test /nonexistent x`) exits 77.
- Set CMAKE_EXPORT_COMPILE_COMMANDS so clangd finds the `src` include path. The editor diagnostics about a missing `assets/assets.hpp` came only from the missing compile database.
- Still to do in S4a: CWAV decode and .bfnmdl geometry in C++.

- 2026-10-12T16:45+02:00 — S4a part 2: model geometry in C++ → [docs/re/models.md](../re/models.md)

### 2026-10-12T17:00+02:00 — S4b: development model viewer

- Added src/viewer.cpp (chase-viewer). It uses SDL2 with a GL 3.3 core context through libepoxy 1.x (pkg-config `epoxy`); CMake builds it only when epoxy is found.
- It loads archive paths given on the command line through chase_assets, uploads textures and draws a textured, vertex-coloured, normal-shaded triangle list with alpha discard below 0.25.
- Textures are looked up by stem, preferring `<model dir>/textures/`. `--exclude` skips parts by texture name, e.g. the sky box.
- The camera uses the input core and controls.cfg; focus loss resets input. `--screenshot` saves a BMP after the second frame.
- Added FuseArchive::names().
- Headless verification with `SDL_VIDEODRIVER=offscreen` (EGL):
  - csdata/models/1_1_e/trolley: 850 triangles, 0 missing textures. The image shows the trolley, donut crate and label texture, matching the Python preview.
  - lvl_00_policestation (4 models, `--exclude sky`): 48016 triangles, 0 missing textures. Terrain, plaza and building pieces line up in world space. Placed props and buildings are absent, which is expected until .blvl is decoded (S4c).
- CTest `viewer-real-smoke` is registered only when the extracted RomFS exists at configure time.
- `cmake --build build` — 0 warnings. `ctest --test-dir build --output-on-failure` — PASS 5/5.

- 2026-10-12T17:20+02:00 — S4c: .blvl level format decoded (REA, Ghidra) → [docs/re/levels.md](../re/levels.md)
- 2026-10-12T17:45+02:00 — S4c: native level placement and the mesh-centre rule → [docs/re/levels.md](../re/levels.md)
- 2026-10-12T18:05+02:00 — Vertex shaders, UV scales and lightmaps → [docs/re/models.md](../re/models.md)
- 2026-10-12T18:25+02:00 — S4a complete (CWAV in C++), S4d mixer and sound test → [docs/re/audio.md](../re/audio.md)
- 2026-10-12T18:40+02:00 — Skeleton and animation containers (REA, Ghidra) → [docs/re/skeleton-animation.md](../re/skeleton-animation.md)
- 2026-10-12T19:05+02:00 — Animation keyframe decoding (REA, Ghidra) → [docs/re/skeleton-animation.md](../re/skeleton-animation.md)

### 2026-10-12T20:10+02:00 — Graphify set up for the repository

- graphify (uv tool install, `~/.local/bin/graphify`) was installed but not initialised. Added `.graphifyignore` (docs/, artifacts/, build*/, graphify-out/, .claude/, *.cci, *.3ds, caches). Detection confirms 0 files from docs/, artifacts/ or build/.
- Full build: AST over 38 code files (656 nodes, 1464 edges), plus semantic extraction of CLAUDE.md, CMakeLists.txt and README.md by one Haiku subagent (20 nodes, 18 edges, 2 hyperedges; all node types valid, 0 dangling semantic edges). Graph: 711 nodes, 1449 edges, 38 labelled communities.
- Health check: 184 AST edges point at external symbols (std headers/types) that are not nodes, and 33 same-endpoint edges collapse in the undirected graph.
- `graphify query` answers from graphify-out/graph.json. `graphify hook install` added post-commit/post-checkout rebuild hooks. graphify-out/ stays local (gitignored); the hook's .gitattributes merge driver was not kept, because the graph is not a tracked team file.

- 2026-10-12T20:15+02:00 — Pose composition recovered (REA, Ghidra) → [docs/re/skeleton-animation.md](../re/skeleton-animation.md)
- 2026-10-12T20:45+02:00 — Native skeletons, animation curves and skinning (S4e groundwork) → [docs/re/skeleton-animation.md](../re/skeleton-animation.md)
- 2026-10-12T21:30+02:00 — Bone palettes recovered; skinned playback in chase-viewer → [docs/re/skeleton-animation.md](../re/skeleton-animation.md)
- 2026-10-12T22:10+02:00 — Frame interpolation recovered; playback rate traced to the instance → [docs/re/skeleton-animation.md](../re/skeleton-animation.md)
- 2026-10-12T22:40+02:00 — Texture v orientation corrected (most "black material" cases) → [docs/re/textures.md](../re/textures.md)
- 2026-10-12T23:05+02:00 — Placement Euler order recovered (replaces the Ry·Rx·Rz assumption) → [docs/re/levels.md](../re/levels.md)
- 2026-10-12T23:25+02:00 — Whole levels in the viewer → [docs/re/levels.md](../re/levels.md)
- 2026-10-12T23:55+02:00 — Navigation grids (.bfnnav) decoded → [docs/re/levels.md](../re/levels.md)
- 2026-10-13T00:25+02:00 — Localisation tables (.loc) decoded → [docs/re/text-localisation-fonts.md](../re/text-localisation-fonts.md)
- 2026-10-13T00:45+02:00 — Text escapes decoded (Cyrillic/Polish) → [docs/re/text-localisation-fonts.md](../re/text-localisation-fonts.md)
- 2026-10-13T01:05+02:00 — Bitmap fonts (.tnf) decoded → [docs/re/text-localisation-fonts.md](../re/text-localisation-fonts.md)
- 2026-10-13T01:30+02:00 — Animation frame count is u16 +4 & 0x3fff → [docs/re/skeleton-animation.md](../re/skeleton-animation.md)

### 2026-10-13T01:40+02:00 — Project rule: C++ is the source of truth

- At the user's request, CLAUDE.md now has the section "C++ source of truth (no parallel implementations)":
  - C++ libraries hold all runtime decoders and logic.
  - Python is for exploration and CLI tools, calling C++ through bindings when it needs a decoder.
  - New formats get one implementation; existing Python decoders are frozen legacy and are replaced by calls through the C++ binding when they need a change (never fixed in both).
  - Real-data tests pin frozen reference values with recorded provenance.
  - Violations found later are to be fixed in focused commits, or recorded here as open items.
- This replaces the earlier "change both decoders" rule. No Python/C++ binding exists yet.

- 2026-10-13T02:30+02:00 — Cutscene tracks, cameras and joint orientations (REA, Ghidra 12.1.2) → [docs/re/cutscenes.md](../re/cutscenes.md)
- 2026-10-13T03:00+02:00 — First Python→C++ binding; Python posing removed (rule 6 fix) → [docs/re/skeleton-animation.md](../re/skeleton-animation.md)
- 2026-10-13T03:20+02:00 — Cutscene playback rate and soundtrack → [docs/re/cutscenes.md](../re/cutscenes.md)
- 2026-10-13T03:50+02:00 — Data tables (.bxls) decoded (REA, Ghidra 12.1.2) → [docs/re/data-tables-config.md](../re/data-tables-config.md)
- 2026-10-13T04:10+02:00 — Overflowing joint curves explained: one curve stores frames + 1 → [docs/re/skeleton-animation.md](../re/skeleton-animation.md)
- 2026-10-13T10:15+02:00 — Brace-block config text parser (.cam, trigger and option lists) → [docs/re/data-tables-config.md](../re/data-tables-config.md)
- 2026-10-13T10:30+02:00 — S4f save format recovered (REA, Ghidra 12.1.2; read-only) → [docs/re/save-format.md](../re/save-format.md)
- 2026-10-13T10:45+02:00 — S3 HID input and main loop (REA, Ghidra 12.1.2; read-only, partial) → [docs/re/arm-executable.md](../re/arm-executable.md)
- 2026-10-13T11:00+02:00 — .bxaml UI layouts decoded (REA, Ghidra 12.1.2) → [docs/re/ui-layouts.md](../re/ui-layouts.md)
- 2026-10-13T11:05+02:00 — FMV player with subtitles: WIP, not merged → [docs/re/fmv.md](../re/fmv.md)
- 2026-10-13T11:40+02:00 — FMV player finished and merged → [docs/re/fmv.md](../re/fmv.md)
- 2026-10-13T12:10+02:00 — S4f chase_save library (src/save/) and save-core test; `cmake --build build` warning-free, `ctest --test-dir build` 10/10 pass. Header size field confirmed (REA) → [docs/re/save-format.md](../re/save-format.md)
- 2026-10-13T12:30+02:00 — Options defaults and system language mapping added to chase_save; save-core passes → [docs/re/save-format.md](../re/save-format.md)
- 2026-10-13T08:51+02:00 — chase-fmv black screen fixed: audio start deadlocked when the frame queue filled first (`4b3b75f`); ctest 10/10.
- 2026-10-13T08:53+02:00 — Options fields (sound/music volume, invert camera, subtitles) and language order; chase_save accessors; ctest 10/10 → [docs/re/save-format.md](../re/save-format.md)
- 2026-10-13T08:55+02:00 — S3 scheduler fields recorded; step value still unknown (time-boxed) → [docs/re/arm-executable.md](../re/arm-executable.md)
- 2026-10-13T08:56+02:00 — Options gift list and +0x1ec records → [docs/re/save-format.md](../re/save-format.md)
- 2026-10-13T09:00+02:00 — particledefs.bprt container decoded (parseParticleDefs, pjwHash); ctest 10/10 → [docs/re/particles.md](../re/particles.md)
- 2026-10-13T09:01+02:00 — Particle spawner field use; named fields in ParticleDef; ctest 10/10 → [docs/re/particles.md](../re/particles.md)
- 2026-10-13T09:03+02:00 — Particle init fields (atlas, frames, flips, colour gradient); ctest 10/10 → [docs/re/particles.md](../re/particles.md)
- 2026-10-13T09:05+02:00 — Particle emitter update (rate, duration, fade keys); ctest 10/10 → [docs/re/particles.md](../re/particles.md)
- 2026-10-13T09:07+02:00 — Particle step: four key lists, gravity, drag, bounce, size growth; ctest 10/10 → [docs/re/particles.md](../re/particles.md)
- 2026-10-13T09:08+02:00 — Particle texture hash-to-index conversion found (0x1ed440) → [docs/re/particles.md](../re/particles.md)
- 2026-10-13T09:08+02:00 — Session timestamps corrected to commit times (system clock). The earlier 2026-10-13 entries (02:30–12:35) do not match their commit times (e.g. `ab62d67` at 01:43) and were left unchanged.
- 2026-10-13T09:10+02:00 — .cam loader 0x3062e4: key handlers and record layout; RotationY/X, Distance, Lookat are editor-only → [docs/re/data-tables-config.md](../re/data-tables-config.md)
- 2026-10-13T09:12+02:00 — Storyboard runtime layout (0x1dbb80); property evaluator not found (time-boxed) → [docs/re/ui-layouts.md](../re/ui-layouts.md)
- 2026-10-13T09:13+02:00 — Slot progress layout: character bits, level blocks, completion formula (0x1cd3dc) → [docs/re/save-format.md](../re/save-format.md)
- 2026-10-13T09:14+02:00 — Per-level counters (0x20a630, 0x203464) and totals source TablesSuperCounters (0x2f63dc) → [docs/re/save-format.md](../re/save-format.md)
- 2026-10-13T09:15+02:00 — Paused at user request. Tree clean after this commit; build warning-free and ctest 10/10 at `537a033`.
- 2026-10-13T09:19+02:00 — TablesSuperCounters matches the 13 × 7 totals layout (worker, bytes re-checked with xxd) → [docs/re/save-format.md](../re/save-format.md)
- 2026-10-13T09:20+02:00 — Input slot table 0x3769e8: 18 slots → pad channels (worker, REA Ghidra; fill at 0x168c9c re-checked with objdump) → [docs/re/arm-executable.md](../re/arm-executable.md)
- 2026-10-13T09:22+02:00 — Input slots: 6/7 gameplay vs 4/5 UI readers, channel 7 unfed by pad, stick → D-pad 0x2289b8 (worker, REA; 0.6 literal re-checked) → [docs/re/arm-executable.md](../re/arm-executable.md)
- 2026-10-13T09:25+02:00 — Level/character tables, exclusion uses char byte +0xc, completion total 711 (worker, REA + objdump; loop re-checked) → [docs/re/save-format.md](../re/save-format.md)
- 2026-10-13T09:26+02:00 — 0x25c1b8 is an unsigned divide; completion formula settled → [docs/re/save-format.md](../re/save-format.md)
- 2026-10-13T09:28+02:00 — .cam Type is a bit set read by pose blend 0x298cd8; per-frame camera is 0x1b4a30 (worker, REA; bit tests re-checked) → [docs/re/data-tables-config.md](../re/data-tables-config.md)
- 2026-10-13T09:30+02:00 — chase_save slot progress accessors + completion (worker, Sonnet); reviewed, offsets re-checked against 0x203464/0x20a630; build warning-free, ctest 10/10.
- 2026-10-13T09:32+02:00 — Camera selection is spatial (0x20bf6c), weights by XZ distance, bit 0x400 = look offset add (worker, REA; 0x23f50c re-checked) → [docs/re/data-tables-config.md](../re/data-tables-config.md)
- 2026-10-13T09:33+02:00 — chase_save_tables + chase-savecheck, save-real-data test (total 711) (worker, Sonnet); empty-slot wrap shown as empty; build warning-free, ctest 11/11.
- 2026-10-13T09:33+02:00 — Sound play path 0x23fa54(id, pos, …), ids = tables/tablessoundfx.bxls (worker, REA; table re-checked) → [docs/re/audio.md](../re/audio.md)
- 2026-10-13T09:36+02:00 — GSP interrupt thread and vsync wait 0x257a68; 30/60 fps still unknown (worker, REA; 0x130042 literal re-checked) → [docs/re/arm-executable.md](../re/arm-executable.md)
- 2026-10-13T09:38+02:00 — Vblank counter callbacks (ids 2/3), wait modes 0x400..0x402, mode byte 0x52a1f4; fps still undecided (worker, REA; callback re-checked) → [docs/re/arm-executable.md](../re/arm-executable.md)
- 2026-10-13T09:39+02:00 — SoundEffect table decoder + chase-soundtest --id (worker, Sonnet); 757/757 names resolve to sounds/<name>.bwav; build warning-free, ctest 12/12 → [docs/re/audio.md](../re/audio.md)
- 2026-10-13T09:41+02:00 — Swap 0x25a6f0: min frame interval 0x529de8 (1/2), 0x122908 = APT cmd 0x004F0080 (worker, REA; literals re-checked) → [docs/re/arm-executable.md](../re/arm-executable.md)
- 2026-10-13T09:42+02:00 — Music: tablessoundfx sheet 1 (39 tracks → music/<name>.bwav), OverrideMusic/StopMusic/PauseMusic handlers (worker, REA; sheet re-checked) → [docs/re/audio.md](../re/audio.md)
- 2026-10-13T10:53:04+02:00 — Runtime oracle (Azahar 2126.2 + gdb stub, tools/azahar_oracle.py; `f5d43cf`); scheduler is a clamped variable step; pacing globals measured → [docs/re/arm-executable.md](../re/arm-executable.md). chase_runtime: scheduler replica (`69d53b3`) and pad pipeline replica matching the live stick value (`6c8e03d`); renderer moved to chase_render, viewer screenshots byte-identical (`8d0d940`). Player entity pointer 0x3772b8 (worker, re-checked) → [docs/re/player.md](../re/player.md). Build warning-free, ctest 13/13, Python 55/55.
- 2026-10-13T10:55+02:00 — chase-game development runtime skeleton (S4e): SDL events -> InputState -> PadState -> PadDevice, Scheduler driven by a 3000 Hz clock, level via chase_render, placeholder camera on pad slots. `cmake --build build` warning-free, ctest 14/14 incl. game-real-smoke; offscreen screenshot identical to the viewer's default framing; ~33 updates/s.
- 2026-10-13T10:59:11+02:00 — Stick-to-intent replica in chase_runtime (player_intent), library atan2f ported bit-exact (qemu-arm differential, 300441 pairs) (worker); build warning-free, ctest 13/13 → [docs/re/player.md](../re/player.md)
- 2026-10-13T11:08:12+02:00 — chase-game shows the player at Player1Start (Chase McCain, idle_minifig frame 0, stand-in camera behind him); `Level::defaults`, `playerStart`, `characterAppearance`, Scene texture overrides; build warning-free, ctest 14/14, screenshot checked → [docs/re/player.md](../re/player.md)
- 2026-10-13T11:51:01+02:00 — Oracle fast workflow (worker): --keep/--attach/--stop, Xvfb + XTEST hotkeys, savestate/loadstate/--state; language screen in 37 s free-running, state load 13.5 s, attach+report 0.4 s; python3 -m unittest 55 OK → [docs/re/oracle.md](../re/oracle.md)
- 2026-10-13T12:02:21+02:00 — Oracle RPC channel (worker): --rpc/--rpc-input, sample command, screenshot 0.05 s instead of 10.7 s, free-run 50.14 clock ticks per vblank (stepped 65.5), breakpoint-free pad input verified (channel 16 = 1.0); python3 -m unittest 63 OK → [docs/re/oracle.md](../re/oracle.md)
- 2026-10-13T12:45+02:00 — Oracle launch robustness (worker): Xvfb quick-load abort traced to a Qt paint vs state-load race, render window unmapped, launch retry (max 3) and cleanup of own processes; 16/16 verification runs, python3 -m unittest 69 OK → [docs/re/oracle.md](../re/oracle.md)
- 2026-10-13T12:49+02:00 — World handedness (worker): game world is left-handed; chase_render perspective() mirrors x, view/cameraAxes use the game yaw convention, setFrontFace added; chase-game and chase-viewer (free camera and cutscene) now unmirrored, matching gameplay-start.png → [docs/re/levels.md](../re/levels.md)
- 2026-10-13T12:56+02:00 — Player head attachment fixed (jointAttachment in chase_assets, test pinned); `cmake --build build` warning-free, `ctest --test-dir build` 14/14 pass; chase-game offscreen screenshot compared with gameplay-start.png → [docs/re/player.md](../re/player.md)
- 2026-10-13T13:45:00+02:00 — Player locomotion measured in the oracle (worker): walk 2.301 / run 6.900 then 10.349 u/s stepped by stick regime (thresholds raw 60, 117/118), no acceleration, facing step ceil(0.3 d) capped at 6400 per update, movement follows the intent heading; animation root motion via 0x20cd30/0x1abe60. Tool: watch, trap, dt, trace mem, attach loadstate focus fix; python3 -m unittest 19 oracle tests OK → [docs/re/player.md](../re/player.md), [docs/re/oracle.md](../re/oracle.md)
- 2026-10-13T13:59:00+02:00 — Player locomotion in chase-game (worker): `player_locomotion` in chase_runtime replicates the measured rules; root motion could not be derived from the animation data (measured speeds kept behind `kMeasuredRootMotionSpeeds`); `locomotion-real-data` differential test (1114 updates), runtime-core thresholds/timer/facing checks, `--script` stick option with `game-script-run`/`game-script-walk`; `cmake --build build` warning-free, `ctest --test-dir build` 17/17 pass → [docs/re/player.md](../re/player.md)
- 2026-10-13T15:10:11+02:00 — Level collision: ground height and wall sliding (worker). Collision data found in the model+0x1c section, ground probe and wall slide traced (0x20b48c/0x1f9884/0x2023e4, 0x21a73c/0x208d84/0x2092d8) and replicated in `chase_runtime` (collision.*), wired into chase-game. `collision-real-data` replays 104 traces (4104 updates, 249 height changes, 0 wrong) and 24 wall traces (305 slide updates, 2 millimetre-contact differences) → [docs/re/collision.md](../re/collision.md). Commands: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCHASE_ROMFS_DIR=... -DCHASE_LOCO_TRACE_DIR=... -DCHASE_COLLISION_TRACE_DIR=...`, `cmake --build build` (0 warnings), `ctest --test-dir build` 18/18; oracle scenarios tools/oracle/coll-wall*.txt (port 24698, home artifacts/oracle/coll-home). Lobby run `--script "0 150 40"`: 8.10 units, height 1.42 to 2.51, stops at the desk platform.
- 2026-10-13T14:34:16+02:00 — Graphify compliance check: graph was stale since 13:45 (post-commit hook did not rebuild after the cherry-picked merges); `graphify update .` → 1823 nodes / 3818 edges, new runtime files present, docs/, artifacts/ and .claude/ absent; query verified. From now on: update and verify the graph after each merge batch, query it first for repository architecture questions, and say so in agent briefs.
- 2026-10-13T14:41:13+02:00 — Gameplay camera recovered (worker): writer of 0x376894 is the camera manager update 0x23f174 (atan2 of the camera node's forward axis); the street camera is the follow controller 0x2e0bf0 with the OnFoot preset (look point and eye smoothing 1 − 0.75^(30 dt), 10 m horizontal rope, vertical spring 0.3, yaw (L/R ±2 + stick term) · dt · 0.6, projection 0x23458c); `FollowCamera` in chase_runtime replaces the chase-game placeholder; `camera-real-data` replays 9 oracle traces (323 updates, eye error <= 6.1e-4); `memtrace` step added to the oracle tool; unknowns: wall and player push-out, pitch clamp, record pose blend rooms, other presets → [docs/re/camera.md](../re/camera.md)
- 2026-10-13T15:10:43+02:00 — Python tool audit (worker): extract_fib, convert_textures, convert_audio, convert_loc, convert_font and convert_levels (`--param-names` only) now call the C++ runtime through `chase_native` (new C ABI entry points; outputs byte-identical on the real data: 12836 files, 3991 PNGs, 1361 WAVs plus manifest, 66 localisation tables, 856 levels); tt_resource, convert_models, convert_anims, convert_nav and preview_model removed with their tests (last present at `9d04c91`; chase-viewer `--screenshot` replaces the preview); CLAUDE.md rule 4 updated; `cmake --build build` warning-free, ctest 18/18, python3 -m unittest 60 OK
