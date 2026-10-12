# Progress — Linux x86_64 accessible port

## Status

- Overall: IN_PROGRESS
- Current Phase: IMPLEMENT
- Current Step: S3
- Last Updated: 2026-10-12T15:32:32+02:00
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
| S3 | Focused ARM analysis | IN_PROGRESS | R1; startup, SVC stubs, service-handle helper, services, paths recorded; game frame function still open |
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

- `4642cf5` — Add verified 3DS executable extraction tool (S1)
- `db355f9` — Add SDL2 accessible input laboratory (S2)
- `a805bf3` — Add port plan, progress log and CLAUDE.md

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

Assets are now unpacked (artifacts/romfs, artifacts/fib). Next, decode the asset formats starting with btga (textures; its header is likely simple, and the decoder can be found from the string `.btga` in code), then bwav and the bfn* model family. Then S3: improve function coverage (Hopper found 963 procedures vs ~6697 ARM `push {...,lr}` prologues). Then trace callers of svcSendSyncRequest (0x25947c) to group IPC wrappers by service (hid/gsp/fs/dsp/cfg/ptm), and identify the per-frame game update called from the dispatcher at 0x100448. Resolve the 8 srvGetServiceHandle_candidate call sites whose names come via rodata pointer tables.

## Session Log

### 2026-10-12T14:43:19.863039+02:00

- Original CCI untouched; NCCH NoCrypto flag 0x04, exheader hash valid.
- Product CTR-P-AA8P; program name LEGOCITY; .code compressed (exheader flag bit 0).
- .code SHA256 f895faaea826ee470e056d36fed72d66922b27ec9e5923e53f341567431b6923.
- REA rejected CCI container; ordinary Python probe is local evidence, not REA Evidence.

### 2026-10-12T15:10:00+02:00 — S3 focused analysis (REA, Hopper provider, ELF sha256 5152ed14…0881)

- Opened artifacts/game/game.arm.elf; binary_overview ev_58c1dadc79e269e074c9c5fe98ce23a3b06199eb833f901f82be0d66b789bc54: 3 segments, 963 procedures, 0 strings defined (no sections, so Hopper defines no strings).
- Entry 0x100000 (ev_e86237bf46c633b755ad47fe4e25bff928d851aafa8760d5bd0e03b49ecbdbd1): `bl` chain sub_100024, sub_10022c, sub_1002c8, sub_1002a0, sub_1336cc, sub_1001f8, sub_100280, sub_100108, then `b` to svc 0x3 (ExitProcess). This matches typical CTR SDK crt0; roles of individual init calls not yet confirmed.
- sub_100108 (ev_1f33c6d8d139ac39e591c882c9d2c37299641b8d9d97dbd19ad5ff226101a5ea): 0x930-byte frame; loop `while (sub_100448()) sub_10057c();`, then sub_100324 and sub_100954. Inference (not confirmed): a framework/applet dispatcher, not game logic.
- sub_100448 (ev_cd6d2d3e6cabe488c953db4d96d4c28ea3c6034219fdcef6e4ddd52b8342d324) walks a message list with kinds 1/2/3/7 and calls a virtual method at +0x10. sub_10057c (ev_d3565622686694c11b9731296e21efd0c14fc9f675cc63839da706915d17d043) calls sub_25cb2c(*0x376310, 1).
- SVC census from local llvm-objdump (armv6k, text only; local evidence, not REA). Each SVC appears exactly once in its own stub: 0x01 0x03 0x08 0x09 0x0A 0x0C 0x14 0x17 0x18 0x19 0x1F 0x20 0x21 0x22 0x23 0x25 0x27 0x28 0x2D 0x32 0x35 0x38 0x3A 0x3C. There are 2 spurious `svc #0xed6b90` hits in data. 24 stubs named in REA using standard 3DS SVC numbering (ev_4f958eba9c138769e7849bda90005ec82a681b6cf30b4e97375b17a151f88f98), e.g. svcSendSyncRequest 0x25947c, svcConnectToPort 0x105b08, svcExitProcess 0x25d4d4.
- Service names in rodata: cecd:u/s, dsp::DSP, fs:LDR, fs:USER, ndm:u, srv:pm, cfg:u/i/s, hid:USER/SPVR, ptm:sysm/s/gets/u/play, err:f, gsp::Lcd, gsp::Gpu. No "srv:" or "APT:U" string was found by this simple search; not yet explained.
- Paths: rom:/ (literal users at 0x128608, 0x1287d8, 0x159020, 0x188fa4), data:/savegame.dat (pointer at 0x377964), levels/triggers.txt, levels/letriggers.txt, pregen/*.bin, Pregen/Traffic|Pedestrians/%s_*.bin, TouchGestures/%s.dat, Localisation/extended_chars_utf8.txt, scriptfns.txt. Assets live in the 769 MB RomFS (not yet extracted).
- Coverage caveat: resolve_containing_procedure returned not_in_procedure for the fs:USER user 0x10f988 (ev_f52ce74dbc497c0f3ce93454138b76efb3281e5ccdc80c18256246e4cac469b2) and the rom:/ literal 0x128718 (ev_d887d2367ebb3881e7d6eef78c3e7ad82dd29c9e652671dfa00ddabd9e3293fe). The local disassembly has ~6697 `push {...,lr}` prologues, so Hopper covers a minority of functions.
- Engine strings (ATTR_*, StudSpawn*, CUFO*) suggest TT Games' engine. This is an unconfirmed inference.

### 2026-10-12T15:25:00+02:00 — S3 DSP/service-handle follow-up

- The user added a permission rule for mcp__rea__resolve_containing_procedure, and the call was retried. For 0x10a080 it returned not_in_procedure (ev_c0e45080d9f6d52a852b58e0a5d3377f6c66d5d7a7303707d43d9b204b691064), the same Hopper coverage gap. The earlier blocker is resolved.
- Local disassembly: function 0x109fc0 loads "dsp::DSP" (literal 0x10a080 at 0x109fe4), calls 0x258fd8 (length) then 0x258fa4(out, name, len, 0), initializes state, and calls 0x110948.
- 0x258fa4 has 11 call sites. Each site with its enclosing function (nearest preceding push…lr) and service name:
  - Name loaded directly:
    - 0x10a004 in 0x109fc0: dsp::DSP
    - 0x10f91c in 0x10f8e4: fs:USER
    - 0x1100a8 in 0x11006c: ndm:u
  - Name not resolved by the direct-literal scan (likely loaded through rodata pointer tables such as 0x359c64 → hid:USER):
    - 0x102f0c in 0x102ec8
    - 0x110164 in 0x110134
    - 0x117320 in 0x1172b4
    - 0x118e70 in 0x118e2c
    - 0x1220f0 in 0x122090
    - 0x12af28 in 0x12aea0
    - 0x131310 in 0x1312d0
    - 0x25b700 in 0x25b67c
  - Inference: 0x258fa4 is srv:GetServiceHandle.

### 2026-10-12T15:25:37+02:00 — RomFS extraction (S1 extension, prerequisite for S4 assets)

- Added tools/extract_romfs.py and tests/test_extract_romfs.py (6 tests: tree/offsets, level 2/3 hash corruption, unsafe `..`/`a/b` and duplicate names, directory cycle, out-of-bounds data, end-to-end image extract, existing output refused, superblock mismatch, encrypted flag).
- Layout confirmed on the real image: IVFC v0x10000, master hash 384 B, all three levels with 4 KiB blocks; level 3 at RomFS+0x1000, then level 1, then level 2, each aligned. The RomFS superblock hash is at NCCH+0x1E0 over 1 media unit; NCCH+0x1C0 is the ExeFS one.
- `python3 tools/extract_romfs.py <CCI> --output artifacts/romfs` — PASS exit 0, 0.93 s; the superblock and full IVFC tree were verified; 51 files, 763156067 B; romfs-manifest.json sha256 d5fd75e6…be90e. CCI size and mtime were unchanged (1073741824, 851484720).
- Contents: `lego_city3ds.fib` (259357972 B, the main asset archive, format unknown), `lego_city3ds.csv` (1090227 B, probably its index), and 49 `cutscenes/*.moflex` (Mobiclip video; `_de` variants are German), including `splash5` and 18 `superbuildit_*`. Every non-video asset is inside the .fib.
- `python3 -m unittest discover -s tests -p 'test_*.py' -v` — PASS 15/15.

### 2026-10-12T15:32:32+02:00 — FUSE (.fib) archive format and codec recovered

Static analysis used local llvm-objdump (armv6k) over game.arm.elf. This is local evidence, not REA evidence, because Hopper does not define these procedures.
- Literal `"FUSE"` (0x358E44) is used only by 0x17dea8, the fib open routine. It reads a 20-byte header: magic, `1.00`, +8 entry count, +0xC second-table count, and +0x10 directory offset. It then loads count×12 directory bytes and registers the archive in global array 0x52eb7c, whose count is at 0x376618.
- The directory entry is `{u32 name CRC, u32 offset, u32 size<<5 | flags}`, sorted by CRC.
- 0x128830 opens an entry: codec = flags&3; the block size is 0x8000 << ((flags>>2)&7).
- 0x12894c reads an entry. Codec 0 is a direct read. Otherwise the entry is a series of chunks of `{u32 len, data}`: codec 1 → 0x12efe0, codec 3 → 0x12d50c, and the remaining case is memcpy 0x25415c.
- 0x12efe0 is a RefPack/QFS variant without a header, and some bit fields are rearranged against EA RefPack:
  - `0xxxxxxx b1`: lit=(b>>2)&3, dist=((b&3)<<8|b1)+1, len=((b>>4)&7)+3
  - `10xxxxxx b1 b2`: lit=b1>>6, dist=((b1&63)<<8|b2)+1, len=(b&63)+4
  - `110xxxxx b1 b2 b3`: lit=(b>>3)&3, dist=((b&1)<<16|b1<<8|b2)+1, len=((b&6)<<7)+b3+5
  - `111xxxxx`: (b&31)*4+4 literals; above 112 (0xFC–0xFF), b&3 literals and end. Literals follow the operand bytes.
  - Codec 3 (0x12d50c) has not been analysed.
- Added tools/extract_fib.py and tests/test_extract_fib.py (5 tests covering all token forms, malformed streams, codecs 0/1/2, CSV names, unnamed fallback, codec 3 reported, unsafe name, CSV mismatch, size mismatch, duplicate CRC, second table).
- `python3 tools/extract_fib.py artifacts/romfs/romfs/lego_city3ds.fib --names …/lego_city3ds.csv --output artifacts/fib` — PASS, 23.2 s:
  - Entries: 12836/12836 extracted, 12824 named via the CSV. Codec counts are {1: 12280, 0: 556}, and no other codecs occur.
  - Every CSV offset, size and compressed size matched the directory, and the 12 unnamed entries are 0 bytes.
  - Unpacked total 478387766 B equals the CSV sum; fib-manifest.json sha256 c6a6c9c0…01f3.
  - Spot checks: options3ds.txt and levels/triggers.txt decode to clean engine script text.
- Asset census by extension: btga 3354, bfnmdl 3113, bfnanm 1954, bwav 1361, blvl 856, bfnnav 700, bpng 602, bfnskl 376, srt 204, bstxt 69, loc 66, bxaml 55, bdds 35, bxls 24, bin 23, txt 15, cam 10, tnf 6, bprt 1. None of these formats is decoded yet.
- Inference, not confirmed: a btga of 87352 B = a 56-byte header + 128×128 RGBA8 with mips down to 8×8 (0x15500), and 0x15500 appears as a u32 inside the header.
- `python3 -m unittest discover -s tests -p 'test_*.py' -v` — PASS 20/20.

### 2026-10-12T15:40+02:00 — Texture container decoded (.btga/.bpng/.bdds)

- Header layout, recovered from shipped files (not yet traced in code):
  - 56 bytes: constants 0x400, 0x10, -14, 0x20, -14
  - u32 data size; u16 width and u16 height; u32 data size again
  - u8 PICA200 format; 2 zero bytes; u8 cubemap flag; u32 mip count; 16 zero bytes
  - Data: the mip chain, largest level first, 8×8 Morton-tiled and stored bottom-up. A cubemap holds six chains.
- Added tools/convert_textures.py (all 14 PICA formats; ETC1/ETC1A4 decode 4×4 blocks in tile order (0,0),(4,0),(0,4),(4,4); u64 LE words; column-major pixel index) and tests/test_convert_textures.py (6 tests).
- `python3 tools/convert_textures.py --census artifacts/fib/files` — 3991/3991 headers valid. Each declared size equals the computed mip-chain size × faces. Counts: btga ETC1 1443, ETC1A4 985, L8 452, LA8 267, RGBA8 107, RGB8 68, A8 11, RGBA8 cube 21; bpng RGBA8 497, ETC1A4 70, LA8 10, A8 9, RGB8 9, ETC1 5, L8 2; bdds ETC1A4 22, ETC1 11, LA8 2.
- `python3 tools/convert_textures.py artifacts/fib/files --output artifacts/textures` — PASS, 3991 PNGs, 0 failures, 80 s.
- Visual checks (viewed PNGs):
  - pc01_lombardstreet postcard: ETC1
  - saveicon "POLICE" sign upright: ETC1, confirms the bottom-up flip
  - ui_shop_codes icon: RGBA8
  - atlas_lvl_00_policestation_main "CITY TV" atlas: ETC1A4 with alpha
  - ui_ls_generic1: RGB8, a 400×240 image in a 512×256 texture (the top-screen size)
  - cubemap_showroom_c: six plausible faces; face order unconfirmed
- `python3 -m unittest discover -s tests -p 'test_*.py' -v` — PASS 26/26.
- Named in REA with a _candidate suffix (ev_d6ee55638e4b4ce712665d235d1d66a9d8c1add97c17e5f210b671225ddb1b28): srvGetServiceHandle_candidate, strlen_candidate, dspInit_candidate, fsUserInit_candidate, ndmInit_candidate. Hopper does not define procedures at 0x109fc0, 0x10f8e4 or 0x11006c.
