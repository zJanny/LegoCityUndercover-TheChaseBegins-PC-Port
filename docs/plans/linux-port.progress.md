# Progress — Linux x86_64 accessible port

## Status

- Overall: IN_PROGRESS
- Current Phase: IMPLEMENT
- Current Step: S4e runtime core — M4 reached (level objects, characters, studs, falling/jumping, object collision); front end and sound merged; open items moved to docs/plans/features/
- Last Updated: 2026-10-13T21:51:17+02:00
- Branch: main
- Target Branch: Not specified
- Requested Scope: PLAN, IMPLEMENT, TEST; start reverse engineering and native port
- Plan: docs/plans/linux-port.md
- Plan Revision: 4
- Planning Baseline: no Git/source; supplied CCI only

## Summary

The executable and the full RomFS are extracted with hash verification. The FUSE asset archive, textures, sounds and model geometry decode, and each decoder has been checked against every shipped file. Levels, nav grids, skeletons, animation, cutscenes, localisation, fonts and data tables also decode in C++, and the development viewer shows levels and cutscenes. The game is not playable on PC yet: gameplay logic, rendering states, UI, scripts and saves are not recovered.

## Where things live

- This file: work state only (status, steps, decisions, blockers, next action, one log line per session).
- [features/README.md](features/README.md): one file per feature with status, open items, stand-ins and a findings summary. Look here for known gaps.
- [docs/re/README.md](../re/README.md): evidence and details (addresses, layouts, evidence IDs, unknowns).

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
| S4e | Gameplay | IN_PROGRESS | chase_runtime replicas checked against the oracle: scheduler, pad pipeline, stick intent, locomotion, follow camera, player collision; chase-game runs them on lvl_00; scripts/AI/objects/UI not recovered |
| S4f | Saves/settings | IN_PROGRESS | chase_save: header, CRC, validate, atomic XDG storage, option fields, slot progress accessors, completion (0x1cd3dc, no cheat flags) |

## Current Work

### Current Step

Introducing the feature layer (docs/plans/features/): open items, stand-ins and findings moved out of this file. Resume from Next Action.

### What Is Being Done

Nothing else in progress.

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

Latest recorded verification (2026-10-13T21:17+02:00, details in the matching feature files): `cmake --build build` warning-free, `ctest --test-dir build` 38/38, `python3 -m unittest discover -s tests -p 'test_*.py'` 63 OK. S1: real extraction matched the manifest hashes (image sha256 078faf7e…59ad, `.code` sha256 f895faae…6923, ELF sha256 5152ed14…0881; [executable-platform](features/executable-platform.md)).

Failing: REA `open_binary` rejects the CCI (unsupported format); use the derived ELF. Not run: the original game outside the Azahar oracle.

## Git

Branch: main (`feature/linux-port` was the historical start branch). Early commits: `4642cf5` extraction tool (S1), `db355f9` input laboratory (S2), `a805bf3` plan, progress log and CLAUDE.md. Later commits: see `git log` (one commit per session-log entry).

## Decisions

### Decision: goal and scope (2026-10-13T17:01:07+02:00)

**Question:** what does the port have to be, and in which order. **Decision (user):** a faithful PC port that works and behaves exactly like the original 3DS game, playable start to finish, on Linux first; Windows only after the Linux version is finished and the user says so (no Windows work until then); quality-of-life features only after the game is finished for PC (no QoL and no deviations until then). Keyboard/mouse input mapping, including the `Walk` stand-in key, is part of the port, not QoL. Stand-ins are labelled and listed as open items. **Reason:** user instruction; recorded in CLAUDE.md "Goal and faithfulness" and "Platform".

### Decision: architecture and runtime oracle (2026-10-13T10:53+02:00)

**Question:** how to reach a playable game: execute the ARM code (emulation/recompilation) or reconstruct natively. **Decision:** native reconstruction in C++ (chase_runtime), with the installed Azahar emulator used only as a local oracle (tools/azahar_oracle.py) to measure the original game. **Reason:** executing the ARM code would rebuild Azahar; the oracle turns "unknown at run time" values into measurements and gives a reference for differential tests. The user's own image is only read; nothing game-derived is committed.

User approved Git init, no extra guidance, Linux devices emit keyboard/mouse. Use C++17/SDL2 and preserve ARM code as analysis artifact. Native lab is a foundation, not game runtime.

### Decision: commit authorization (2026-10-12T15:02+02:00)

**Question:** Plan section 16 left commits out of scope. **Decision:** User added to CLAUDE.md: commit at logical steps; push only when asked. **Reason:** Supersedes the no-commit instruction; merge/push remain unauthorized.

### Decision: feature layer between progress file and docs/re (2026-10-13T21:34+02:00)

**Question:** where do known gaps live so that an agent finds them without reading many files. **Decision (user):** one combined file per feature in `docs/plans/features/` (index `features/README.md`, about 18 fine-grained files, stable item IDs, `needs:`/`blocks:` between features); this file keeps work state only with one log line per session; docs/re stays the evidence layer; long old log entries shortened to one line. Plan revision 4. **Reason:** the Next Action section had grown into a mix of status, open items, stand-ins and findings. Rule in CLAUDE.md "Feature files".

## Blockers

Full game logic, hardware APIs and asset formats not recovered yet.

## Deviations From Plan

- 2026-10-12: Plan section 16 said no commits. Actual: commits made per user's CLAUDE.md workflow rule. Plan revision 2 records this; no code steps reopened.

## Next Action

Read this section, CLAUDE.md (incl. "Goal and faithfulness" and "Feature files"), [features/README.md](features/README.md) and, before any emulator use, docs/re/oracle.md. Open items, stand-ins and findings are in the feature files, not here. Feature layer introduced 2026-10-13T21:34:59+02:00; earlier handoff 2026-10-13T17:53:20+02:00.

**Goal (user, 2026-10-13):** faithful PC port, playable start to finish, Linux first; Windows only when the user says so; QoL only after the PC version is finished (Decisions, CLAUDE.md).

**State:** M1 runtime loop, M2 locomotion, M3 walkable level, M4 level objects, characters, studs, falling, jumping and object collision are on main; the front end (boot to the first level) and the sound hooks are merged. `chase-game` is a development runtime, not the game ([docs/status.md](../status.md)).

**Priorities (critical path):**
1. Script execution: doors and other scripted objects (frozen today), NPC animation, then mission flow: [script-system](features/script-system.md), [doors](features/doors.md), [level-objects](features/level-objects.md), [animation-characters](features/animation-characters.md).
2. World streaming and level switches: WSTREAM-1 (rule that picks loaded rooms), TRANS-1 (police entry bound, trap 0x303480): [world-streaming](features/world-streaming.md), [level-transitions](features/level-transitions.md).
3. Rendering part 2: RENDER-1 fragment lights (break at 0x12d884), RENDER-4 indoor oracle state, reflections: [rendering](features/rendering.md).
4. Player completeness: slide exit and the landing after a slide, the other `0x203d18` states: [player-movement](features/player-movement.md).
5. Front-end remainder (existing slots, Continue, loading screen scene): [front-end](features/front-end.md), [saves-options](features/saves-options.md).

**Workflow notes:** oracle route and states: `tools/azahar_oracle.py` with `--display xvfb --state gameplay-start --renderer opengl --rpc --rpc-input`, route from boot `tools/oracle/to-gameplay.txt`; only one agent may use `--rpc` at a time. The lead delegates to subagents in worktrees (at most 3 in parallel, user), every brief names the CLAUDE.md rules, reviews each result against CLAUDE.md, rebuilds and tests, squash-merges one commit per branch (`git merge --squash`), removes the worktree and branch (`git worktree remove -f -f`, `git branch -D`), then `graphify update .` and verifies. Resume a branch with `git log main..<branch>`.

## Session Log

One line per session: timestamp, what was worked on, how far it got, link to the feature file. Findings, open items and stand-ins do not belong here (CLAUDE.md "Feature files"). Reverse-engineering details are in docs/re/ (see docs/re/README.md).

- 2026-10-13T21:34:59+02:00 — Feature layer introduced: open items, stand-ins and findings moved from this file into docs/plans/features/ (one file per feature, index features/README.md); long log entries shortened to one line (git history keeps the originals) → [features/README.md](features/README.md)
- 2026-10-13T21:17:53+02:00 — Start state of placed objects and doors: level scripts decoded, `startup` script and reveal links give the disabled objects, doors wired into chase-game; ctest 38/38 → [script-system](features/script-system.md), [doors](features/doors.md), [docs/re/scripts.md](../re/scripts.md)
- 2026-10-13T17:36:00+02:00 — Placed objects block the player and the camera (world grid, registration, query layers, camera box slice); ctest 26/26 → [collision](features/collision.md), [camera](features/camera.md), [docs/re/collision.md](../re/collision.md)
- 2026-10-12T14:43:19.863039+02:00 → [docs/re/container-and-archive.md](../re/container-and-archive.md)
- 2026-10-12T15:10:00+02:00 — S3 focused analysis (REA, Hopper provider, ELF sha256 5152ed14…0881) → [docs/re/arm-executable.md](../re/arm-executable.md)
- 2026-10-12T15:25:00+02:00 — S3 DSP/service-handle follow-up → [docs/re/arm-executable.md](../re/arm-executable.md)
- 2026-10-12T15:25:37+02:00 — RomFS extraction (S1 extension, prerequisite for S4 assets) → [docs/re/container-and-archive.md](../re/container-and-archive.md)
- 2026-10-12T15:32:32+02:00 — FUSE (.fib) archive format and codec recovered → [docs/re/container-and-archive.md](../re/container-and-archive.md)
- 2026-10-12T15:40+02:00 — Texture container decoded (.btga/.bpng/.bdds) → [docs/re/textures.md](../re/textures.md)
- 2026-10-12T15:45+02:00 — Sound container decoded (.bwav) → [docs/re/audio.md](../re/audio.md)
- 2026-10-12T15:55+02:00 — Shared TT pooled-resource layout (all b* formats) → [docs/re/pooled-resources.md](../re/pooled-resources.md)
- 2026-10-12T16:10+02:00 — .bfnmdl model format decoded (REA, Ghidra 12.1.2 provider) → [docs/re/models.md](../re/models.md)
- 2026-10-12T16:30+02:00 — S4a part 1: `chase_assets` (FUSE, pooled, textures) with `assets-core` and `assets-real-data`; ctest 4/4, 0 warnings → [assets-formats](features/assets-formats.md)
- 2026-10-12T16:45+02:00 — S4a part 2: model geometry in C++ → [docs/re/models.md](../re/models.md)
- 2026-10-12T17:00+02:00 — S4b: development model viewer `chase-viewer` (SDL2, GL 3.3 core, libepoxy) with `viewer-real-smoke`; ctest 5/5 → [viewer-tools](features/viewer-tools.md)
- 2026-10-12T17:20+02:00 — S4c: .blvl level format decoded (REA, Ghidra) → [docs/re/levels.md](../re/levels.md)
- 2026-10-12T17:45+02:00 — S4c: native level placement and the mesh-centre rule → [docs/re/levels.md](../re/levels.md)
- 2026-10-12T18:05+02:00 — Vertex shaders, UV scales and lightmaps → [docs/re/models.md](../re/models.md)
- 2026-10-12T18:25+02:00 — S4a complete (CWAV in C++), S4d mixer and sound test → [docs/re/audio.md](../re/audio.md)
- 2026-10-12T18:40+02:00 — Skeleton and animation containers (REA, Ghidra) → [docs/re/skeleton-animation.md](../re/skeleton-animation.md)
- 2026-10-12T19:05+02:00 — Animation keyframe decoding (REA, Ghidra) → [docs/re/skeleton-animation.md](../re/skeleton-animation.md)
- 2026-10-12T20:10+02:00 — Graphify set up (`.graphifyignore`, AST graph, rebuild hooks; graphify-out/ stays local) → CLAUDE.md Graphify
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
- 2026-10-13T01:40+02:00 — Project rule "C++ is the source of truth" added to CLAUDE.md at the user's request → CLAUDE.md
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
- 2026-10-13T10:53:04+02:00 — Runtime oracle (`f5d43cf`), scheduler and pad pipeline replicas (`69d53b3`, `6c8e03d`), renderer moved to chase_render (`8d0d940`); ctest 13/13, Python 55/55 → [executable-platform](features/executable-platform.md), [viewer-tools](features/viewer-tools.md), [docs/re/arm-executable.md](../re/arm-executable.md)
- 2026-10-13T10:55+02:00 — chase-game development runtime skeleton (S4e): input, scheduler, level via chase_render; ctest 14/14 → [player-movement](features/player-movement.md)
- 2026-10-13T10:59:11+02:00 — Stick-to-intent replica (`player_intent`, library atan2f ported bit-exact) → [player-movement](features/player-movement.md), [docs/re/player.md](../re/player.md)
- 2026-10-13T11:08:12+02:00 — chase-game shows the player at Player1Start (idle_minifig frame 0, stand-in camera); ctest 14/14 → [player-movement](features/player-movement.md), [docs/re/player.md](../re/player.md)
- 2026-10-13T11:51:01+02:00 — Oracle fast workflow (`--keep`/`--attach`/`--stop`, save states); Python 55 OK → [viewer-tools](features/viewer-tools.md), [docs/re/oracle.md](../re/oracle.md)
- 2026-10-13T12:02:21+02:00 — Oracle RPC channel (`--rpc`, `--rpc-input`, screenshot 0.05 s); Python 63 OK → [viewer-tools](features/viewer-tools.md), [docs/re/oracle.md](../re/oracle.md)
- 2026-10-13T12:45+02:00 — Oracle launch robustness (Xvfb quick-load abort mitigated, launch retry); 16/16 runs, Python 69 OK → [viewer-tools](features/viewer-tools.md), [docs/re/oracle.md](../re/oracle.md)
- 2026-10-13T12:49+02:00 — World handedness: the game world is left-handed, renderer mirrors x → [assets-formats](features/assets-formats.md), [docs/re/levels.md](../re/levels.md)
- 2026-10-13T12:56+02:00 — Player head attachment fixed (`jointAttachment`); ctest 14/14 → [animation-characters](features/animation-characters.md), [docs/re/player.md](../re/player.md)
- 2026-10-13T13:45:00+02:00 — Player locomotion measured in the oracle: speeds by stick regime, facing step → [player-movement](features/player-movement.md), [docs/re/player.md](../re/player.md)
- 2026-10-13T13:59:00+02:00 — Player locomotion in chase-game (`player_locomotion`, `locomotion-real-data` 1114 updates); ctest 17/17 → [player-movement](features/player-movement.md), [docs/re/player.md](../re/player.md)
- 2026-10-13T15:10:11+02:00 — Level collision: ground height and wall sliding replicated (`collision-real-data`: 104 traces, 24 wall traces); ctest 18/18 → [collision](features/collision.md), [docs/re/collision.md](../re/collision.md)
- 2026-10-13T14:34:16+02:00 — Graphify compliance check: stale graph rebuilt with `graphify update .`; rule: update and verify the graph after each merge batch and query it first → CLAUDE.md Graphify
- 2026-10-13T14:41:13+02:00 — Gameplay camera recovered and replicated (`FollowCamera`, `camera-real-data` 9 traces) → [camera](features/camera.md), [docs/re/camera.md](../re/camera.md)
- 2026-10-13T15:10:43+02:00 — Python tool audit: five converters moved onto the C++ binding (byte-identical output), five Python tools removed (last present at `9d04c91`) → [viewer-tools](features/viewer-tools.md)
- 2026-10-13T15:22:58+02:00 — Merged player collision, Python tools moved onto the C++ binding, docs audit, CLAUDE.md rule 4 updated; graph rebuilt → [collision](features/collision.md), [viewer-tools](features/viewer-tools.md)
- 2026-10-13T15:34:18+02:00 — Racy oracle launch test fixed; README quick start starts chase-game; new guide docs/game-runtime.md; Python 60 OK → [viewer-tools](features/viewer-tools.md)
- 2026-10-13T15:39:33+02:00 — Level objects in chase-game and chase-viewer (class files supply meshname and transform); ctest 19/19 → [level-objects](features/level-objects.md), [docs/re/objects.md](../re/objects.md)
- 2026-10-13T16:19:35+02:00 — Camera obstruction recovered (two slabs, pull-in, side and vertical push) and replicated (`CameraSweep`); ctest 22/22 → [camera](features/camera.md), [docs/re/camera.md](../re/camera.md)
- 2026-10-13T16:06:01+02:00 — Falling and landing recovered and replicated (`fall-real-data` 18 traces); oracle tool got `teleport` → [player-movement](features/player-movement.md), [docs/re/player.md](../re/player.md)
- 2026-10-13T16:31:00+02:00 — Jumping recovered and replicated (`jump-real-data` 12 jumps); jump on B, own `Walk` action → [player-movement](features/player-movement.md), [input](features/input.md)
- 2026-10-13T16:33:40+02:00 — Lead merge of falling/jumping into main (conflicts in src/game.cpp, CMakeLists.txt and docs resolved); ctest 26/26, Python 62 OK → [player-movement](features/player-movement.md)
- 2026-10-13T16:35:00+02:00 — Level characters drawn standing in place (lvl_00 32, lvl_01 99); ctest 22/22 → [level-objects](features/level-objects.md), [docs/re/objects.md](../re/objects.md)
- 2026-10-13T17:02:03+02:00 — Speed and scale derivation: walk/run speeds, jump scales and landing stun derived from code and data, fitted constants removed; ctest 26/26 → [player-movement](features/player-movement.md), [docs/re/player.md](../re/player.md)
- 2026-10-13T17:12:29+02:00 — Studs recovered (pickup sprites, radius, counter) and replicated (`StudField`, `studs-real-data` 10 traces); ctest 28/28 → [studs](features/studs.md), [docs/re/studs.md](../re/studs.md)
- 2026-10-13T17:42:29+02:00 — Oracle tool: `free_display()` reuses stale X display locks; lead studs check against an oracle screenshot (the original shows that door open, chase-game closed) → [viewer-tools](features/viewer-tools.md), [doors](features/doors.md)
- 2026-10-13T17:54:24+02:00 — Sounds in chase-game (jump, footsteps, landings, studs; level music rule); `audio-core`, `audio-real-data` added → [audio](features/audio.md), [docs/re/audio.md](../re/audio.md)
- 2026-10-13T17:53:20+02:00 — Paused at the user's request near the usage limit: front-end agent stopped with a WIP commit, sound agent asked to wrap up; handoff rewritten in Next Action (M4, unfinished branches, next candidates).
- 2026-10-13T21:17:22+02:00 — Player runtime: landing stun by the animation-finished test, slide (state 8), slope slow-down, exact library sin/cos; ctest 32/32 → [player-movement](features/player-movement.md), [docs/re/player.md](../re/player.md)
- 2026-10-13T20:50:00+02:00 — Front end finished from boot to the first level (`chase_frontend`, `chase_movie`, `chase-frontshot`, chase-game front-end mode); ctest 35/35 → [front-end](features/front-end.md), [docs/re/front-end.md](../re/front-end.md)
- 2026-10-13T20:58:38+02:00 — Lead fix: 47 of 49 movies alternate eye frames; `chase_movie` keeps the first picture of each pair; ctest 35/35 → [movies-cutscenes](features/movies-cutscenes.md), [docs/re/fmv.md](../re/fmv.md)
- 2026-10-13T21:38:53+02:00 — Lead: docs/re stand-in references (collision, scripts, audio, studs, player) now point to the feature files instead of the progress file; INPUT-1 kept open for possible controller support (user) → [input](features/input.md).
- 2026-10-13T21:39:06+02:00 — In-game HUD: portrait, hearts, stud counter, studs flying to it, bottom-screen map (stand-in fit); skip limit corrected to 30; ctest 42/42 → [hud-ui](features/hud-ui.md), [studs](features/studs.md), [docs/re/hud.md](../re/hud.md)
- 2026-10-13T21:46:54+0200 — chase-viewer plays animations at their stored rate (ANIM-S1 removed) → [animation-characters](features/animation-characters.md).
- 2026-10-13T21:46:58+0200 — STUD-3 checked: still open: blue and purple collect sound (id 63) has no recorded oracle pickup; silver id 64 is pinned by `audio-real-data` → [studs](features/studs.md).
- 2026-10-13T22:24:39+02:00 — Material texture-environment stages, blend, alpha test and cull recovered and run in the renderer (levels no longer dark, Chase's eyes no longer holes); lighting still a stand-in; ctest 42/42 → [rendering](features/rendering.md), [docs/re/materials.md](../re/materials.md)
- 2026-10-13T21:51:17+02:00 — Effect sound stop (0x23ae50, owner key, fade from the table) and the door sound sequence in `chase-game`; AUDIO-4 and DOOR-5 closed; ctest 42/42 → [audio](features/audio.md), [doors](features/doors.md)
- 2026-10-13T22:17:29+02:00 — Progress report tool (`tools/progress_report.py`, README table, `progress-readme` gate) over C++ tables for script commands, object behaviours and file decoders → [viewer-tools](features/viewer-tools.md), [docs/progress.md](../progress.md)
- 2026-10-13T22:20:38+02:00 — Lead: progress report merged; README progress section is a workflow rule (CLAUDE.md "Git", gate `progress-readme`) → [viewer-tools](features/viewer-tools.md), [docs/progress.md](../progress.md).
- 2026-10-13T22:35:00+0200 — Mapped world levels, room links, scene change chain; decoded `.blvl` +0x34 room links and +0x1c event bindings (`streaming-real-data`); oracle teleport into the police entry bound did not fire → [world-streaming](features/world-streaming.md), [level-transitions](features/level-transitions.md).
- 2026-10-13T22:31:09+02:00 — Lead: paused near the usage limit; lighting part 1 and the streaming map merged, all agents stopped, priorities updated in Next Action.
- 2026-10-13T22:34:50+02:00 — Lead: two user-reported police-station bugs classified and recorded: lift door half open and blocking (DOOR-7, with DOOR-3), grey wall behind the start (WSTREAM-7); both need an indoor oracle state (RENDER-4) → [doors](features/doors.md), [world-streaming](features/world-streaming.md).
- 2026-10-13T22:41:25+02:00 — Lead: user-reported subtitle overflow recorded as FE-10 → [front-end](features/front-end.md).
- 2026-10-13T22:45:08+02:00 — Lead: oracle tool `--display desktop` (visible emulator for the user) → [viewer-tools](features/viewer-tools.md), [docs/re/oracle.md](../re/oracle.md).
- 2026-10-13T23:11:46+02:00 — Lead: user checked a real 3DS: no camera turning in the police station; WSTREAM-7 reclassified, CAMERA-6 and stand-in CAMERA-S3 added → [camera](features/camera.md).
- 2026-10-13T23:13:43+02:00 — Lead: camera lock lead from the user's real-3DS observation (crossed-out camera icon): native CameraRotFixedSet 0x27bb9c, icon UI_CameraLocked 0x193a5c → [camera](features/camera.md).
- 2026-10-13T23:14:58+02:00 — Lead: police-station camera issue made definitive (CAMERA-6, CAMERA-S3); WSTREAM-7 closed as superseded; white quad split off as RENDER-6 → [camera](features/camera.md).
- 2026-10-13T23:20:34+02:00 — Lead: oracle checkpoint police-station from the user's own quick save; tool supports New 3DS states ("new_3ds" in the state json) → [rendering](features/rendering.md), [docs/re/oracle.md](../re/oracle.md).
- 2026-10-14T09:50:11+02:00 — Agent B: RENDER-4 closed (lobby compared), CAMERA-6 cause found (record pose camera 0x298cd8, not CameraRotFixedSet; not implemented, needs CAMERA-3), DOOR-7 fixed in code (per-leaf instances, not oracle-verified), RENDER-6 classified (Checker1 quad) → [camera](features/camera.md), [doors](features/doors.md), [rendering](features/rendering.md).
- 2026-10-14T10:00:14+02:00 — Agent D: links from features to docs/re now carry #ts- anchors (130 entry anchors, 193 links); tools/check_doc_links.py and CTest doc-links added → [viewer-tools](features/viewer-tools.md)
