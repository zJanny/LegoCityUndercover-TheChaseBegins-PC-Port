# Progress — Linux x86_64 accessible port

## Status

- Overall: IN_PROGRESS
- Current Phase: IMPLEMENT
- Current Step: S3
- Last Updated: 2026-10-12T14:53:00+02:00
- Branch: feature/linux-port (unborn)
- Target Branch: Not specified
- Requested Scope: PLAN, IMPLEMENT, TEST; start reverse engineering and native port
- Plan: docs/plans/linux-port.md
- Plan Revision: 2
- Planning Baseline: no Git/source; supplied CCI only

## Summary

Image identified and its executable ExeFS hashes verified in a read-only probe. Extraction and accessible input foundation underway. Game is not playable on PC yet.

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
| S3 | Focused ARM analysis | IN_PROGRESS | R1, S1 complete |
| S4 | Recover gameplay and replace platform | NOT_STARTED | R4, needs S3 subsystem plan |

## Current Work

### Current Step

S3.

### What Is Being Done

Open artifacts/game/game.arm.elf in REA; inspect entry/startup, platform service calls, and strings; record evidence.

### Files Being Modified

docs/plans/linux-port.progress.md (evidence); README.md (planned in section 20, still absent).

### Expected Result

Recorded entry/startup chain, SVC usage, subsystem leads, and caveats sufficient to draft S4 subsystem plans.

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

feature/linux-port (unborn)

### Commits

Logical commits for S1 (extraction), S2 (input lab), docs; hashes recorded in next update.

### Uncommitted Changes

Plan/progress and .gitignore newly created; source pending.

## Decisions

User approved Git init, no extra guidance, Linux devices emit keyboard/mouse. Use C++17/SDL2 and preserve ARM code as analysis artifact. Native lab is a foundation, not game runtime.

### Decision: commit authorization (2026-10-12T15:02+02:00)

**Question:** Plan section 16 left commits out of scope. **Decision:** User added to CLAUDE.md: commit at logical steps; push only when asked. **Reason:** Supersedes the no-commit instruction; merge/push remain unauthorized.

## Blockers

Full game logic, hardware APIs and asset formats not recovered yet.

## Deviations From Plan

- 2026-10-12: Plan section 16 said no commits. Actual: commits made per user's CLAUDE.md workflow rule. Plan revision 2 records this; no code steps reopened.

## Next Action

Continue S3 analysis in REA on artifacts/game/game.arm.elf; then write README.md (section 20).

## Session Log

### 2026-10-12T14:43:19.863039+02:00

- Original CCI untouched; NCCH NoCrypto flag 0x04, exheader hash valid.
- Product CTR-P-AA8P; program name LEGOCITY; .code compressed (exheader flag bit 0).
- .code SHA256 f895faaea826ee470e056d36fed72d66922b27ec9e5923e53f341567431b6923.
- REA rejected CCI container; ordinary Python probe is local evidence, not REA Evidence.
