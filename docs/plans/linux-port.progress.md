# Progress — Linux x86_64 accessible port

## Status

- Overall: IN_PROGRESS
- Current Phase: IMPLEMENT
- Current Step: S4f (paused 2026-10-13T12:35+02:00 at user request)
- Last Updated: 2026-10-13T12:30+02:00
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
| S4d | Audio playback | IN_PROGRESS | mixer + SDL sound test with loops; event routing not recovered |
| S4e | Gameplay | IN_PROGRESS | skeletons/animation/cutscenes/nav/tables/loc/fonts decoded; logic not recovered |
| S4f | Saves/settings | IN_PROGRESS | chase_save: header, CRC, validate, atomic XDG storage; block contents opaque |

## Current Work

### Current Step

S4f, paused at a clean point.

### What Is Being Done

Nothing in progress. chase_save (src/save/) handles the header, CRC, validation, atomic XDG storage, option defaults and the system language mapping.

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

User approved Git init, no extra guidance, Linux devices emit keyboard/mouse. Use C++17/SDL2 and preserve ARM code as analysis artifact. Native lab is a foundation, not game runtime.

### Decision: commit authorization (2026-10-12T15:02+02:00)

**Question:** Plan section 16 left commits out of scope. **Decision:** User added to CLAUDE.md: commit at logical steps; push only when asked. **Reason:** Supersedes the no-commit instruction; merge/push remain unauthorized.

## Blockers

Full game logic, hardware APIs and asset formats not recovered yet.

## Deviations From Plan

- 2026-10-12: Plan section 16 said no commits. Actual: commits made per user's CLAUDE.md workflow rule. Plan revision 2 records this; no code steps reopened.

## Next Action

- S4f: meaning of the options nibbles (+0, +3) and the game language index order; slot contents (story progress, unlocks); how live state is copied into the save buffer.
- S3: find the GSP vsync call and the scheduler step (30 vs 60 fps); name the binding slots at 0x3769e8.
- S4e: the code that applies .bxaml track properties; .cam field semantics; particle definitions (.bprt).
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
