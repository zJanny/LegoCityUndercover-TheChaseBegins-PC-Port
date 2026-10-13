# Feature: Linux x86_64 accessible port

## 1. Executive Summary

Build a faithful PC port of The Chase Begins from the supplied decrypted 3DS image: everything works and behaves exactly like the original, playable from start to finish, on Linux x86_64 first. Windows follows only after the Linux version is finished and the user says so; quality-of-life features follow only after the game is finished for PC. Until then no Windows work and no deviations from the original. Delivered so far: an auditable extraction pipeline, an accessible native input laboratory, asset decoders and a development runtime; gameplay reconstruction remains required work.

## 2. Confirmed Requirements

R1: reverse engineer supplied game; accept verified executable extraction and mapped analysis, S1/S3. R2: build native Linux x86_64 foundation; accept native build and smoke test, S2. R3: keyboard/mouse device support; accept configurable hold/toggle input, focus reset, touch mapping, S2. R4: playable port that behaves exactly like the original, start to finish, on Linux x86_64; accept original gameplay, assets, sound and saves on PC, S4; not yet executable-ready. R5 (deferred, only on the user's explicit instruction): Windows port, after R4 is complete. R6 (deferred): quality-of-life features, after the game is finished for PC. No work on R5 or R6 until then.

## 3. Assumptions

C++17 and SDL2 are reversible platform defaults for the Linux version. Action names initially describe 3DS buttons, not recovered gameplay semantics. Do not infer native pointer sizes from ARM addresses. Original image remains unchanged; generated game-derived files stay local.

## 4. Repository Findings

Revision 4 (2026-10-13: S4 subsystem plans, open items and stand-ins live in docs/plans/features/; the progress file keeps work state only). Revision 3 (2026-10-12: S4 split into S4a–S4f after the asset formats were recovered; RomFS/FUSE/texture/audio/model tools added). Revision 2 (2026-10-12: commit authorization via CLAUDE.md; CLAUDE.md is now project guidance). Revision 1. Baseline: no Git, no source, only 1 GiB CCI. Git now initialized on feature/linux-port, unborn HEAD. No applicable AGENTS/README found. User approved Git initialization and confirmed no guidance, Linux keyboard/mouse devices. Installed cmake, c++, python3, SDL2 2.32.74. NCSD at 0x100; executable NCCH at 0x4000. NoCrypto flag and matching exheader/ExeFS SHA256 show decrypted content. REA open_binary rejects CCI format; route extracted mapped ARM ELF instead. Existing patterns/tests/database/deployment: none.

## 5. Architecture

New Python NCSD/NCCH reader -> verified ExeFS extraction -> backwards-LZ decompression -> analysis-only ARM ELF -> REA disassembler -> recovered subsystem contracts. Separate SDL events -> configurable input state -> laboratory display; later connect to recovered game logic.

## 6. Detailed File Change Plan

Add .gitignore; tools/extract_game.py for bounded extraction, manifests, ELF; tests/test_extract_game.py for malformed formats and BLZ; CMakeLists.txt, src/input.hpp, src/input.cpp, src/main.cpp, config/controls.cfg, tests/input_test.cpp for native input; README.md and this plan/progress for commands, evidence and limitations. No deletions or migrations. artifacts/ and build/ generated and ignored.

## 7. Step-by-Step Implementation Plan

S1 (R1): implement tools/extract_game.py and tests/test_extract_game.py, preserve source, require decrypted NCCH/hash matches, bound all reads/names, reject existing outputs, decompress with backreference checks, emit program headers for text/rodata/data+BSS and manifest hashes. Verify python unit tests and real-image extraction. S2 (R2/R3), independent of S1: implement listed SDL C++ files/config/CMake, all binding validation, toggle debounce, focus-loss reset, opposing cancellation and lower-screen touch coordinate bounds. Verify build, CTest and SDL dummy smoke. S3 (R1), after S1: open ELF in REA, inspect entry/startup/platform calls and strings; record exact evidence IDs and mapped-artifact caveats here in progress. S4 (R4), after S3: define executable-ready per-subsystem plans once evidence exists, recover input polling and main loop, replace kernel services/render/audio/storage incrementally. Never mark gameplay complete because lab builds.

Per-feature plans, status, open items and stand-ins (revision 4): docs/plans/features/README.md, one file per feature, rule in CLAUDE.md "Feature files". This plan and its progress file are the only plan/progress pair of the `feature-development` workflow; subsystem work updates its feature file and adds one progress log line, and no per-feature plan/progress pairs are created; the feature files are the evidence-backed subsystem plans that S4e requires.

S4 subsystem plan (revision 3, 2026-10-12, based on the asset evidence in the progress log):
- S4a — native asset runtime. C++17 code under src/assets/, with no SDL dependency, mirroring the verified Python tools:
  - FUSE .fib reader and RefPack codec
  - pooled-resource reader
  - PICA200 texture decode to RGBA8
  - CWAV DSP-ADPCM decode
  - .bfnmdl geometry
  Verify with CTest on synthetic fixtures, plus an opt-in real-data test (skipped when artifacts/ is absent) that compares results with the Python tools' outputs.
- S4b — level viewer: SDL2 + OpenGL 3.3 core. It loads one level's models with their textures and uses a free camera driven by the existing input core. This is a development viewer, not gameplay.
- S4c — level composition: decode .blvl (pooled; −15/−16 only) to place models, plus streaming (Pregen/*.bin). Requires RE of the .blvl loader.
- S4d — audio playback through SDL audio, including loop points. Mapping sound IDs to events requires RE.
- S4e — gameplay: the main loop (dispatcher 0x100448/0x10057c), entities and characters (.bfnskl/.bfnanm), scripts (scriptfns.txt, triggers), physics/navigation (.bfnnav), and UI (.bxaml/.bxls/.bstxt/.loc). Each needs its own evidence-backed plan before implementation.
- S4f — saves (data:/savegame.dat) and settings.

## 8. Database Changes

Not applicable: no database.

## 9. API / Interface Changes

CLI: python3 tools/extract_game.py IMAGE --output artifacts/game; output must be new. Manifest JSON includes image identity, original byte ranges, exheader/ExeFS hashes, ARM segment addresses and ELF identity. ELF is derived analysis wrapper, never original Nintendo binary. Native lab uses config/controls.cfg and --smoke-test. Detailed binding syntax documented with actual implementation.

## 10. UI Changes

Native laboratory shows current input states and touch area, with an explicit development-tool title. Default controls and settings documented. Focus loss clears all controls. No claim of recovered game UI.

## 11. Business Logic

Hold stays active while any bound source is held. Toggle flips on fresh press only; repeats ignored. Opposing directions cancel. Mouse touch only maps inside aspect-fit 320x240 lower-screen region. Escape quits laboratory.

## 12. Error Handling

Malformed/truncated image, encryption, mismatched hashes, invalid BLZ references or ELF segment size mismatch fail visibly. New output directory avoids overwrites. SDL/config failures produce clear error and nonzero exit. Partial generated output may remain for inspection; rerun chooses a new directory.

## 13. Security

Read original image only. Bound input sizes and range arithmetic, safe output basenames, cap decompression size. No keys, downloads or execution of ARM code. Do not stage commercial game data. No network service or authentication.

## 14. Testing Plan

R1: valid synthetic NCCH/ExeFS accepted; truncated headers, out-of-bounds partitions, encrypted flags, hash mismatch and invalid BLZ rejected; real .code extraction hash matched. R2/R3: build/CTest checks multiple bindings, toggle repeats, focus loss, opposing axes, touch bounds; SDL dummy smoke confirms event integration. R4: game runtime oracle not available; not tested, remains incomplete.

## 15. Verification Commands

Workspace cwd. Python3: python3 -m unittest discover -s tests -p test_*.py -v; requires synthetic fixtures only. python3 tools/extract_game.py <supplied CCI> --output artifacts/game; requires original image. cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug; cmake --build build; ctest --test-dir build --output-on-failure; SDL_VIDEODRIVER=dummy build/chase-input-lab --smoke-test (confirm actual executable name against CMake). Commands pending at plan creation. Compiler/SDL present; no install required.

## 16. Git Plan

Actual feature/linux-port. Revision 2: CLAUDE.md (user-specified workflows) authorizes commits at logical steps; push only when the user asks; merge not requested. Stage source/config/docs/tests only after inspection. Suggested future logical commits: extraction tooling; native accessible input; findings/documentation. Persist final progress in tracked staged files. Never add artifacts or CCI.

## 17. Rollout Plan

Local tool only: extract, build, run lab and development runtime. Playable rollout deferred until R4 verified; Windows rollout deferred until the user asks for it after R4.

## 18. Rollback Plan

Source-only changes are reversible. Original image untouched. Generated outputs are isolated. No destructive cleanup automatically.

## 19. Observability

Manifest records reproducible offsets and hashes; REA evidence IDs recorded in progress. Native title/display exposes input state; stderr reports failures.

## 20. Documentation

README explains extraction/build/run/config, the goal (faithful port, Linux first) and the distinction between development tools and the not-yet-playable game. The progress file retains work state only (it is the only progress file; subsystems use features/ and no separate per-feature pairs); features/ holds each feature's status, open items and stand-ins; docs/re holds evidence.

## 21. Final Implementation Checklist

[ ] S1 extraction tested
[ ] S2 native build/input tested
[ ] S3 focused analysis evidence recorded
[ ] S4 playable port recovered
[ ] source/docs Git tracked
[ ] current verification recorded
Commits authorized (rev 2); push/merge outside scope.

## Instructions for the Implementation Agent

Read docs/plans/linux-port.md and docs/plans/linux-port.progress.md completely before resuming, then docs/plans/features/README.md and the feature file you work on. Revision 1; baseline unborn feature/linux-port, no extra project guidance. Discover new applicable guidance and inspect Git status/diffs. Protect unrelated changes and the game image. Execute S1/S2 independently, S3 after S1; S4 requires evidence-derived subsystem plans. Update progress before and after steps, record commands/cwd/toolchain/time/revision/results. Reconcile stale progress against source and test evidence. Commit at logical steps per CLAUDE.md; no permission to push, merge or publish unless asked. Record unknowns rather than inventing game behavior.
