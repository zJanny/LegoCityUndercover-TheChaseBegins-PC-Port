# Feature: Linux x86_64 accessible port

## 1. Executive Summary

Start a Linux x86_64 reconstruction of The Chase Begins from the supplied decrypted 3DS image. Deliver an auditable extraction pipeline and accessible native input laboratory first; gameplay reconstruction remains required work.

## 2. Confirmed Requirements

R1: reverse engineer supplied game; accept verified executable extraction and mapped analysis, S1/S3. R2: build native Linux x86_64 foundation; accept native build and smoke test, S2. R3: keyboard/mouse device support; accept configurable hold/toggle input, focus reset, touch mapping, S2. R4: eventual playable port; accept original gameplay, assets, sound and saves on PC, S4; not yet executable-ready.

## 3. Assumptions

C++17 and SDL2 are reversible platform defaults. Action names initially describe 3DS buttons, not recovered gameplay semantics. Do not infer native pointer sizes from ARM addresses. Original image remains unchanged; generated game-derived files stay local.

## 4. Repository Findings

Revision 2 (2026-10-12: commit authorization via CLAUDE.md; CLAUDE.md is now project guidance). Revision 1. Baseline: no Git, no source, only 1 GiB CCI. Git now initialized on feature/linux-port, unborn HEAD. No applicable AGENTS/README found. User approved Git initialization and confirmed no guidance, Linux keyboard/mouse devices. Installed cmake, c++, python3, SDL2 2.32.74. NCSD at 0x100; executable NCCH at 0x4000. NoCrypto flag and matching exheader/ExeFS SHA256 show decrypted content. REA open_binary rejects CCI format; route extracted mapped ARM ELF instead. Existing patterns/tests/database/deployment: none.

## 5. Architecture

New Python NCSD/NCCH reader -> verified ExeFS extraction -> backwards-LZ decompression -> analysis-only ARM ELF -> REA disassembler -> recovered subsystem contracts. Separate SDL events -> configurable input state -> laboratory display; later connect to recovered game logic.

## 6. Detailed File Change Plan

Add .gitignore; tools/extract_game.py for bounded extraction, manifests, ELF; tests/test_extract_game.py for malformed formats and BLZ; CMakeLists.txt, src/input.hpp, src/input.cpp, src/main.cpp, config/controls.cfg, tests/input_test.cpp for native input; README.md and this plan/progress for commands, evidence and limitations. No deletions or migrations. artifacts/ and build/ generated and ignored.

## 7. Step-by-Step Implementation Plan

S1 (R1): implement tools/extract_game.py and tests/test_extract_game.py, preserve source, require decrypted NCCH/hash matches, bound all reads/names, reject existing outputs, decompress with backreference checks, emit program headers for text/rodata/data+BSS and manifest hashes. Verify python unit tests and real-image extraction. S2 (R2/R3), independent of S1: implement listed SDL C++ files/config/CMake, all binding validation, toggle debounce, focus-loss reset, opposing cancellation and lower-screen touch coordinate bounds. Verify build, CTest and SDL dummy smoke. S3 (R1), after S1: open ELF in REA, inspect entry/startup/platform calls and strings; record exact evidence IDs and mapped-artifact caveats here in progress. S4 (R4), after S3: define executable-ready per-subsystem plans once evidence exists, recover input polling and main loop, replace kernel services/render/audio/storage incrementally. Never mark gameplay complete because lab builds.

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

Local tool only: extract, build, run lab. Playable rollout deferred until R4 verified.

## 18. Rollback Plan

Source-only changes are reversible. Original image untouched. Generated outputs are isolated. No destructive cleanup automatically.

## 19. Observability

Manifest records reproducible offsets and hashes; REA evidence IDs recorded in progress. Native title/display exposes input state; stderr reports failures.

## 20. Documentation

README explains extraction/build/run/config and distinction between lab and playable port. Progress retains observed findings, coverage boundaries and remaining work.

## 21. Final Implementation Checklist

[ ] S1 extraction tested
[ ] S2 native build/input tested
[ ] S3 focused analysis evidence recorded
[ ] S4 playable port recovered
[ ] source/docs Git tracked
[ ] current verification recorded
Commits authorized (rev 2); push/merge outside scope.

## Instructions for the Implementation Agent

Read docs/plans/linux-port.md and docs/plans/linux-port.progress.md completely before resuming. Revision 1; baseline unborn feature/linux-port, no extra project guidance. Discover new applicable guidance and inspect Git status/diffs. Protect unrelated changes and the game image. Execute S1/S2 independently, S3 after S1; S4 requires evidence-derived subsystem plans. Update progress before and after steps, record commands/cwd/toolchain/time/revision/results. Reconcile stale progress against source and test evidence. Commit at logical steps per CLAUDE.md; no permission to push, merge or publish unless asked. Record unknowns rather than inventing game behavior.
