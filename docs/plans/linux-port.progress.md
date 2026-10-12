# Progress — Linux x86_64 accessible port

## Status

- Overall: IN_PROGRESS
- Current Phase: IMPLEMENT
- Current Step: S4a (S3 continues in parallel)
- Last Updated: 2026-10-12T16:15+02:00
- Branch: feature/linux-port
- Target Branch: Not specified
- Requested Scope: PLAN, IMPLEMENT, TEST; start reverse engineering and native port
- Plan: docs/plans/linux-port.md
- Plan Revision: 3
- Planning Baseline: no Git/source; supplied CCI only

## Summary

The executable and the full RomFS are extracted with hash verification. The FUSE asset archive, textures, sounds and model geometry decode, and each decoder has been checked against every shipped file. The game is not playable on PC yet: gameplay, level composition, animation and rendering states are not recovered.

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
| S4a | Native asset runtime (C++) | IN_PROGRESS | FUSE, pooled, textures, models done + CTest; CWAV pending |
| S4b | Level viewer (SDL2 + GL) | COMPLETE | chase-viewer; offscreen smoke test; placement awaits S4c |
| S4c | Level composition (.blvl) | NOT_STARTED | pooled layout known; loader not traced |
| S4d | Audio playback | NOT_STARTED | WAV decode done |
| S4e | Gameplay | NOT_STARTED | needs per-subsystem RE plans |
| S4f | Saves/settings | NOT_STARTED | |

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

S4a: port the verified decoders to C++ (src/assets/, no SDL) with CTest fixtures. Start with FUSE+RefPack, then the pooled reader, textures and models. Then S4b viewer. Pending RE: the .blvl loader (S4c), skeleton/animation, and the meaning of the node/mesh float3 fields. Also pending from S3: improve function coverage (Hopper found 963 procedures vs ~6697 ARM `push {...,lr}` prologues). Then trace callers of svcSendSyncRequest (0x25947c) to group IPC wrappers by service (hid/gsp/fs/dsp/cfg/ptm), and identify the per-frame game update called from the dispatcher at 0x100448. Resolve the 8 srvGetServiceHandle_candidate call sites whose names come via rodata pointer tables.

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

### 2026-10-12T15:45+02:00 — Sound container decoded (.bwav)

- The .bwav wrapper starts with u32 version 0x400 or 0x600.
  - v4 (1263 files): one standard CWAV at 0x44.
  - v6 (98 files, all music): one mono CWAV per channel, back to back from 0x4c, with the second CWAV starting exactly where the first ends.
- The other wrapper fields (rate copy, sample/loop counts, a 4-byte value before CWAV that may be a hash) are not interpreted. The CWAV structure is the public BCWAV layout.
- Added tools/convert_audio.py (PCM8/PCM16/DSP-ADPCM/IMA-ADPCM to WAV, v6 parts merged as channels, loop points kept in audio-manifest.json) and tests/test_convert_audio.py (4 tests: DSP frame arithmetic including floor shift, PCM8/16, IMA, two-part merge + WAV output, malformed inputs).
- `python3 tools/convert_audio.py --census artifacts/fib/files` — 1361/1361 parsed; all DSP-ADPCM: 12000 Hz 1068 (+98 two-part), 11999 69, 12001 75; the rest are between 7025 and 12003 Hz.
- `python3 tools/convert_audio.py artifacts/fib/files --output artifacts/audio` — PASS, 1361 WAVs, 0 failures, 40.4 s.
- Checks:
  - The first DSP frame header equals the context pred/scale byte (0x39 in pickup_positive_1_notloop), which confirms data alignment at DATA+0x20.
  - ffmpeg astats: voice-over and music peak at about −0.2 to −2.4 dBFS with RMS between −10 and −23 dB, which reads as normalized audio rather than noise. Zero-crossing rate is 0.06 for the music sample.
  - music/act_01 decodes to silence because its frames are genuinely all zero, so it is a placeholder.
- `python3 -m unittest discover -s tests -p 'test_*.py' -v` — PASS 30/30.

### 2026-10-12T15:55+02:00 — Shared TT pooled-resource layout (all b* formats)

- 0x107288 rewrites a `.fnmdl` request to `.bfnmdl`, opens it and calls deserializer 0x10ccf4.
- The deserializer brackets reads with 0x250620(stream, marker), which pushes onto a pool stack at +0x138/+0x13c, and 0x250610, which pops it.
- 0x253800 returns the next block of the current pool (−15 at +0x14c, −16 at +0x160, −14 at +0x174; cursor, count, size array, base, offset). When the pool is exhausted it calls 0x24b740, which reads the next group's sizes from the table and then reads that group's data. A −16 group is not read there.
- Layout confirmed across all shipped files, implemented in tools/tt_resource.py:
  - Header: `u32 table_words<<8, u32 header_bytes`.
  - Table: table_words × u32, each a pool marker (−14/−15/−16) followed by that pool's block sizes.
  - −16 block data follows the table and is included in header_bytes.
  - −15 and −14 block data follow in table order and end exactly at the end of the file.
- Correction: the "version 0x400/0x600" noted earlier for .btga/.bwav is this word count (4 and 6). The texture "constants" are the table [−14, 0x20, −14, size]. The .bwav wrapper is a 0x2c record plus one CWAV block per part. convert_audio.py now splits it through this parser.
- `python3 tools/tt_resource.py --census artifacts/fib/files` — 12500/12500 pooled files close exactly:

  | Format | Files | Blocks | Pools |
  |---|---|---|---|
  | bdds | 35 | 70 | — |
  | bfnanm | 1954 | 114836 | −14 only |
  | bfnmdl | 3113 | 174607 | −16 44128, −15 65691, −14 64788 |
  | bfnnav | 700 | 60979 | — |
  | bfnskl | 376 | 6784 | — |
  | blvl | 856 | 282135 | −15 226702, −16 55433 |
  | bpng | 602 | — | — |
  | bprt | 1 | — | — |
  | bstxt | 69 | — | — |
  | btga | 3354 | — | — |
  | bwav | 1361 | — | — |
  | bxaml | 55 | 13880 | — |
  | bxls | 24 | — | — |

  The 336 files that are not pooled resources (.srt 204, .loc 66, .bin 35, .txt 15, .cam 10, .tnf 6) use other formats and are not yet examined.
- room60_l1.bfnmdl (1008 B, 31 words):
  - The −16 data is an 8-byte record (0x0b74ded8, …) and the string "Auburn_building_misc.tga".
  - −14 block 2 (0xc B) is the u16 index list 3,1,0,2,1,3.
  - The last −14 block (0x3c B) is vertices with a 20-byte stride: float3 position (±35.28, 0), 4 B of s16 values 0/0x7fff (UV-like), and 4 B unknown.
  - Inference only: the block-to-field mapping must come from the deserializer 0x10ccf4.
- Added tests/test_tt_resource.py (3 tests); suite PASS 33/33. The audio census after the refactor is 1361/1361.

### 2026-10-12T16:10+02:00 — .bfnmdl model format decoded (REA, Ghidra 12.1.2 provider)

- Opened game.arm.elf with provider_id=ghidra. Ghidra defines the procedures Hopper missed, and its pseudo-code evidence IDs are:

  | Function | Evidence ID | Recovered role |
  |---|---|---|
  | 0x10ccf4 | ev_4bdf7c11…c1e51c7 | model deserializer |
  | 0x114860 | ev_a1448f66…3d91e | material |
  | 0x11528c | ev_0346c245…f0f0d | vertex buffer |
  | 0x114640 | ev_080a4fc6…28d97 | model+0x1c section |
  | 0x11fd20 | ev_44944596…b7c3fc3b | sub-section |
  | 0x12095c | ev_91ccfb89…5c3d363 | vertex flags |
  | 0x1205bc | ev_259de55d…53f2 | buffer relocation callback |
  | 0x113600 | ev_3b555d22…adfc | render-state dirty flags (not attribute setup) |

- A size-0 block is a NULL pointer, because 0x253800 returns 0 for it.
- Model record (−15, 0x24):
  - u16 flags, u16 nodes, u16 meshes, u8 textures, then pointers at +8, +0xc, +0x10, +0x14, +0x1c and +0x20.
  - Node (0x18): name pointer, float3, optional pointer at +0x14.
  - Mesh (0x24): u8 part count at +2, parts pointer at +4, float3 at +8.
  - Part (0x24): material at +0x18, extra at +0x1c, vertex buffer at +0x20.
- Material record (−15, 0x60): texture count = (b10&0x3f)>>3 and UV sets = b10&7. The texture slots are −16 data (n×8 records, then a name per slot).
- Vertex buffer header (−14, 0x78): u16 vertex count at +2, u32 index count at +4, u16 stride at +0x1a. Optional blocks follow when +0x24/+0x48 are set, and morph data when +0x44 is set. The index and vertex data are separate −14 groups, read straight into GPU memory by 0x24d1c4.
- Vertex mask 0x12095c is replicated using the rodata tables at 0x3798c4/d1/e9/f8. The attribute order was recovered from vertex data:
  - position f32×3
  - uv0 s16×2 (bit 4), uv1 (bit 8)
  - normal s8×3+pad (0x40); decoded vectors have length ≈127
  - colour RGBA8 (0x80)
  - 8 bytes (bit 2, set when m0=1), probably skin weights/indices; unconfirmed
  - Bit 1 (m0=2) does not occur.
- `python3 tools/convert_models.py --census artifacts/fib/files`:
  - 3113/3113 models parse with every pool block consumed.
  - All 16122 parts have an index count divisible by 3 (triangle lists).
  - All 16122 computed strides equal the stored stride.
- Visual checks with tools/preview_model.py (software renderer):
  - csdata trolley: a textured hand truck with wheels and cargo.
  - char_bear: bind pose, recognisable.
  - lvl_00_policestation/room1 with the SkyBox texture excluded: cliff, grass and road pieces line up, so level vertices are stored in world space.
- Not recovered: the meaning of node/mesh float3 fields, the model+0x1c section's contents, skin field semantics, and material render states.
- Added tools/convert_models.py, tools/preview_model.py and tests/test_convert_models.py (3 tests); suite PASS 36/36.

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

### 2026-10-12T16:45+02:00 — S4a part 2: model geometry in C++

- Added src/assets/model.cpp. parseModel replays 0x10ccf4/0x114860/0x11528c through PoolCursor. vertexFlags is the 0x12095c replica, built on a byte-exact copy of the rodata tables at 0x3798c4–0x379907. Vertices decode to position/uv0/uv1/normal/colour; skin bytes are skipped. Indices are bounds-checked.
- tools/convert_models.py --census now reports totals: 16122 parts, 7211595 vertices, 4603391 triangles.
- assets-core adds a synthetic model (mirroring the Python fixture), an unconsumed-block rejection and the stride table. assets-real-data now parses all models and requires the Python totals.
- `cmake --build build` — 0 warnings. `ctest --test-dir build --output-on-failure` — PASS 4/4 (assets-real-data 10.2 s): 3113 models / 16122 parts / 7211595 vertices / 4603391 triangles, all equal to the Python reference.
- Remaining in S4a: CWAV decode in C++ (needed for S4d, not for the S4b viewer).

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

### 2026-10-12T17:20+02:00 — S4c: .blvl level format decoded (REA, Ghidra)

- The xref to "levels/%s.blvl" (0x1b1f30) comes from 0x1b1e58. The xref to ".blvl" (0x1d0028, ev_09591263…74f2) is in 0x1cfc50 (ev_fec8db4c…f2f1), the generic .blvl deserializer (pseudo-code ev_1eed109a…f773).
- Helpers:
  - 0x208ae0 (ev_9429bb98…bdf) deserializes a 12-byte parameter: {u32 name hash, u16, u16 type, u32 value}. Types 1/2/3/6 have one block, type 4 one block if the value is non-zero, type 8 two blocks, and the rest are inline.
  - 0x208a3c (ev_cd34ae96…a091) maps a script-function name to an index.
  - 0x24da28 (ev_bcab1d94…d0e) is the name hash: table CRC-32 of the lower-cased name. The table is standard; `zlib.crc32` reproduces it.
- Header (−15) u16 counts: +2 s10 params, +4 s14 params, +6 s18 (0x1c, script-function name lists in −16), +8 s1c (0x18, an event kind plus two strings in −16, e.g. "BoundEntered"), +0xa s20 (0x28), +0xc s24 (0x28, named entries such as "Lobby2Basement_SceneChange"), +0xe s2c objects (0x18). There is also +0x28, optional +0x34, and u8 +1 for s30 (0x28).
- Objects (s2c): the class name comes from −16 (e.g. "leprops/leprop_wobblydestructible"), +4 is the instance name, +8 the room ("room3"), and +0x10/+0x14 are parameter lists with counts at +0xd/+0xe.
- `python3 tools/convert_levels.py --census artifacts/fib/files` — 856/856 .blvl parse with every block consumed. Section totals: s10 20043, s14 7758, s18 2553, s1c 1901, s20 2280, s24 4308, s2c 18469, s30 53.
- Parameter names: hashing every printable string in code.bin resolves 422 of 556 distinct hashes, including initialposition 0x86166f20 (float3), initialorientation 0xa3b9aa7f (float3, degrees; values −185…359), meshname 0xacd1ebb8 (string), DebrisColour, Respawn, SpawnDir, TriggerObject, HeadModel, Speed and others. The names are computed locally (`--param-names artifacts/game/code.bin`) and never committed.
- Meshname "Dir/File" maps to models/dir/file.bfnmdl, and "Name" to models/name/name.bfnmdl. 1258 of 1642 distinct meshnames resolve this way. The rest (e.g. char_bits_*, Ability_Detective_Dog) follow other conventions that are not yet known.
- Euler order is unknown; the viewer assumes R = Ry·Rx·Rz in degrees. Most orientations are yaw-only, or yaw with ±180 on X and Z.
- lvl_00_policestation_main has 350 objects and 130 mesh placements.
- Added tools/convert_levels.py (`--census`, `--placements`, `--param-names`) and tests/test_convert_levels.py (3 tests); Python suite PASS 39/39.
- Named in REA with a _candidate suffix (ev_d6ee55638e4b4ce712665d235d1d66a9d8c1add97c17e5f210b671225ddb1b28): srvGetServiceHandle_candidate, strlen_candidate, dspInit_candidate, fsUserInit_candidate, ndmInit_candidate. Hopper does not define procedures at 0x109fc0, 0x10f8e4 or 0x11006c.
