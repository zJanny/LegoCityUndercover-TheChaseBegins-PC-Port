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
| S4a | Native asset runtime (C++) | COMPLETE | FUSE, pooled, textures, models, levels, sounds; real-data totals equal Python |
| S4b | Level viewer (SDL2 + GL) | COMPLETE | chase-viewer; offscreen smoke test; placement awaits S4c |
| S4c | Level composition (.blvl) | IN_PROGRESS | loader replicated (Py+C++), placements in viewer; materials/lighting, streaming, other sections open |
| S4d | Audio playback | IN_PROGRESS | mixer + SDL sound test with loops; event routing not recovered |
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

### 2026-10-12T17:45+02:00 — S4c: native level placement and the mesh-centre rule

- Mesh record: byte 0 bit 0 means the vertices are stored relative to the float3 at mesh+8 (the mesh bbox centre). It was found because room3/room5 geometry sat at the origin while their props were at x≈−278/+235. Census over all models:
  - bit clear: 3620 meshes have bbox centre == +8, and 145 more have both at 0
  - bit set: 2883 meshes are centred at 0
  - no exceptions
- tools/convert_models.py gains mesh_offset() and an offset parameter on decode_vertices; preview_model applies it. The C++ parseModel adds the centre when the bit is set, and ModelMesh.f8 is renamed to centre/relative.
- Added src/assets/level.cpp: nameHash (CRC-32 of the lower-cased name), parseLevel (a 0x1cfc50 replica that keeps the objects), placements(), meshModelPath().
- assets-core now includes a synthetic level and hash checks. assets-real-data adds 856 levels / 18469 objects / 6563 placements, equal to the Python reference.
- Viewer:
  - New options: `--level`, `--room`, `--cull` (default ccw), `--brightness` (inspection only).
  - Models are cached and drawn per instance with T·Ry·Rx·Rz (degrees). That Euler order is an assumption.
  - Police station: 130 placements and 0 unresolved meshnames.
  - room3 with `--room room3`: 55 props sit inside the room shell (plants along the wall, emblem, furniture), which is visual confirmation of the placements.
  - The ceilings are culled with CCW front faces.
- Observed but unknown: many floor/wall surfaces render black with texture × vertex colour. They are probably multi-texture/lightmap materials (tex=2, m3∈{1,5,7}) whose PICA combiner setup is not recovered.
- `ctest --test-dir build --output-on-failure` — PASS 5/5; Python PASS 39/39; 0 build warnings.

### 2026-10-12T18:05+02:00 — Vertex shaders, UV scales and lightmaps

- The material post-load calls 0x115ec4 (ev_e7652328…b4c5) and 0x115718 (ev_5cfc022b…7939) are stubs that return 1. 0x115128 (ev_adf59b90…39aa) builds a draw-sort key from m1/m2/m3 and the textures. The material shading selection is not in the loader.
- Added tools/dump_dvlb.py, which reads the public SHBIN layout. code.bin contains one DVLB at file offset 0x2779a0 (data segment) with 11 vertex programs, 342 code words and 83 operand descriptors.
  - The symbol tables name the inputs: vPos, vUV0, vUV1, vColour, vNormal, vWeights and vIndex. This confirms the 8-byte vertex attribute (flag 2) is skin weights and indices.
  - They also name the uniforms: ViewComposite, ModelView, MaterialColour, UVScales, UVOffset, TexTrans, Bones (0x22–0x6f), Skinned, Shadows, Cubemap, SimpleTex, VColourTint, ObjspaceCamPos and OutlineScale.
  - Program 1 outputs normal_quat and view, i.e. it uses PICA fragment lighting.
  - The shader names are game-derived output and are not committed beyond this log.
- UV decoding fix: the vertex-buffer header holds the UV scales as f32 at +0x50 (uv0.x, uv0.y, uv1.x, uv1.y), and the shaders multiply the raw s16 values by them.
  - room60/room3 values range from 1.42e-5 to 1.94e-4. The earlier fixed 1/32767 matched only untiled parts; tiled walls/floors repeat 2–6×.
  - The Python and C++ decoders now use the stored scales; fixtures updated.
- In room3, the parts with uv1 carry a second texture "LIGHTMAP_lvl_00_PoliceStation_Basement000.tga" and black vertex colour (000000ff). Lit props carry "cubemap_abstract_c.tga" as their second texture.
- The viewer modulates by the lightmap on uv1 instead of vertex colour. This is an approximation, because the TEV combiner configuration is not recovered.
  - room3 now shows tiled floor and wall textures with lightmap shading.
  - The overall level stays dark, so a lightmap scale (e.g. ×2) is likely but unverified.
- `ctest` PASS 5/5 (real-data totals unchanged), Python PASS 39/39.

### 2026-10-12T18:25+02:00 — S4a complete (CWAV in C++), S4d mixer and sound test

- Added src/assets/audio.cpp: decodeSound splits the .bwav through parsePooled (0x2c record plus CWAV parts) and decodes PCM8/PCM16/DSP-ADPCM/IMA. The DSP sum is computed in 64-bit and floor-shifted, as in the reference.
- assets-core gains a synthetic DSP vector ([112, −16, −16], matching the Python test) and a malformed-record rejection.
- assets-real-data: 1361 sounds / 122501647 samples / 123 looping, equal to audio-manifest.json. The FNV-1a over act_bass_break's interleaved PCM is 0xdcbf1f85, bit-identical to the Python WAV output.
- Added the chase_audio library (src/audio/mixer.{hpp,cpp}, no SDL): voices with linear resampling, mono→stereo, gain, loop wrap to loopStart, and one-shot expiry.
- Added CTest `audio-mixer` (tests/mixer_test.cpp).
- Added chase-soundtest (SDL audio callback at 48 kHz float stereo). It plays archive sounds and keeps loops going for 10 s by default or `--seconds`.
- Added the opt-in CTest `soundtest-real-smoke` (SDL_AUDIODRIVER=dummy).
- `ctest --test-dir build --output-on-failure` — PASS 7/7; 0 warnings.

### 2026-10-12T18:40+02:00 — Skeleton and animation containers (REA, Ghidra)

- Loaders:
  - 0x108f30 rewrites the path to .bfnanm and calls 0x10edec (ev_1b7f5557…8fc1).
  - The skeleton loader 0x10f26c (ev_fbc95e75…a557) opens .bfnskl itself.
  - The track reader is 0x253764 (ev_8cc8ebc4…997f).
  - Neither pushes a pool, and both formats use only the −14 pool, so blocks are handed out in order.
- Skeleton: the header's u8 at +1 is the joint count, followed by blocks for +8, +0xc and +4. Joint records are 0x98 bytes, each with a name block.
- Animation (0x10edec):
  - Optional sections: +8 (count from bits), +0xc (u8 +1 entries × 3 blocks), +0x10 (u8 +2 × 2), +0x14 (u8 +3 × 3), +0x18 tracks (u16 +6).
  - +0x1c sub-animations (u32 +0x20), each made of a fixup list, a count, tracks, a data block and fixup targets.
  - A track (0x18) consists of a first block (u32 name hash), its own pointer-array block, u16(+6) entry blocks, then +0x10 (key times, f32 0..1) and +0x14 (key values).
- Example (csdata donut1_0000): bone-like entries "Donut" and "carryhandle", each with a 0xdf0 data block and a 0x198 per-frame block. One sub-animation has 8 tracks with incrementing hashes and 10 keys of 20 bytes each.
- Added tools/convert_anims.py and tests/test_convert_anims.py (2 tests). `--census`: 2330/2330 files consume every block (376 skeletons / 5280 joints; 1954 animations / 3804 sub-animations). Python PASS 41/41.
- Not recovered: the per-bone keyframe encoding (0xdf0/0x198 blocks), the track value layout, and playback/evaluation code.
- Joint record (0x98), recovered from data:
  - +0 name pointer
  - +4 s32 parent index (−1 for the root)
  - +8 world bind matrix and +0x48 parent-relative matrix, both row-major 4×4 with the translation in the last row
  - +0x88 four floats, unknown (often 0,1,0,0,0,1)
- check_hierarchy() verifies, across all 376 skeletons / 5280 joints, that parents precede children and that world == local·parent.world (row vectors). The maximum relative error is 4.65e-5.
- Skin attribute (vertex flag 2, 8 bytes): shipped data reads as u8 weights then u8 indices (e.g. ff000000 00000000). This matches the shader inputs vWeights/vIndex and the 26-entry "Bones" uniform (regs 0x22–0x6f, 3 regs per bone). The order and the bone-palette mapping are not yet confirmed in code.
- Python PASS 42/42.

### 2026-10-12T19:05+02:00 — Animation keyframe decoding (REA, Ghidra)

- Found by searching for the `lsl #19; lsr #19` extraction of the per-joint table count (anim+8 word: (w & 0x1fff) + bit 28).
- Call chain:
  - 0x1205f8 (ev_34ecb50a…32df) is a pose cache that allocates 0x40 bytes per joint and calls the sampler.
  - 0x12a2b4 (ev_210a33ed…3420) loops over the joints, calling 0x12d4b0 (ev_c0f59246…1227) per joint. 0x12d4b0 clamps the frame (or wraps to 0) and calls the curve decoder 0x248910 (ev_6bc461f5…852c).
  - It also uses the joint record's +0x88 float4 as a default, together with optional per-joint s16/256 corrections.
- Curve format (0x248910), read with the LSB-first bit reader 0x247a48 (ev_29ed4ed7…29b6):
  - 8-bit header. 0xfa means raw f32 4×4 matrices per frame at blob+1+frame·0x40. Otherwise shift = header>>4 (bit 3 adds 2 more base bits to translation), and header bits 0–3 enable four 3-bit channel-mask groups.
  - 12 channels. Absent channels default to 1.0 for 3–5 and 0 otherwise.
  - Each present channel: a signed base (12 bits, 16 for channels 9–11; plus shift), a delta width (4/5 bits; plus shift), then one fixed-width unsigned delta per frame. value = (base + delta[frame]) · scale / 2^shift, and scale channels are clamped to ≥ 0.0001.
- Channel table: 0x376494 (data) holds the bit widths. Its scales are written by the static initializer at 0x347c5c (decoded with `llvm-objdump --mattr=+vfp2`): 0.02 for channels 0–2 (rotation, radians), 0.01 for 3–5 (scale), 6–8 (unknown) and 9–11 (translation).
- 0x248248 (ev_676da22a…9b72) converts channels 0–2 into a quaternion from half-angle sin/cos (0x249038 calls 0x247ac8 = sin by its polynomial, ev_4caeb96f…49a, then 0x247c90 = cos). The formula is replicated in euler_to_quaternion().
- Verification: char_bear idle frame 0 decodes per-joint translations equal to the skeleton's local bind translations (LeftForeArm, Head, Knees, Feet and Hands exact; Pelvis 1.344 vs 1.363, i.e. animated). Rotations are small radian values.
- Added decode_channels() and euler_to_quaternion() to tools/convert_anims.py, plus a synthetic bit-stream test. Python PASS 43/43.
- 19:15 visual test with tools/preview_model.py (`--skeleton --anim --frame [--transpose-rotation]`) on char_bear walk, frames 0/12/24:
  - Skin attribute: u8 weights (summing to 255), then u8 indices. An index is joint − 1 (the root "Character" is skipped); this is a hypothesis.
  - Local pose = S·R·T in row-vector order, with the skin matrix = inverse(bind world)·animated world.
  - With `--transpose-rotation`, the result is a natural quadruped walk cycle. Without it the bear rears up unnaturally, so the transposed quaternion matrix is the likely convention. This is a visual inference only, not traced in code.
- Not recovered: how the pose is composed into joint matrices (order of q/scale/translation and the role of channels 6–8 and joint +0x88), root motion (the extra 24th blob), and the 0xfa raw-matrix decomposition 0x1324f8.
- Named in REA with a _candidate suffix (ev_d6ee55638e4b4ce712665d235d1d66a9d8c1add97c17e5f210b671225ddb1b28): srvGetServiceHandle_candidate, strlen_candidate, dspInit_candidate, fsUserInit_candidate, ndmInit_candidate. Hopper does not define procedures at 0x109fc0, 0x10f8e4 or 0x11006c.

### 2026-10-12T20:10+02:00 — Graphify set up for the repository

- graphify (uv tool install, `~/.local/bin/graphify`) was installed but not initialised. Added `.graphifyignore` (docs/, artifacts/, build*/, graphify-out/, .claude/, *.cci, *.3ds, caches). Detection confirms 0 files from docs/, artifacts/ or build/.
- Full build: AST over 38 code files (656 nodes, 1464 edges), plus semantic extraction of CLAUDE.md, CMakeLists.txt and README.md by one Haiku subagent (20 nodes, 18 edges, 2 hyperedges; all node types valid, 0 dangling semantic edges). Graph: 711 nodes, 1449 edges, 38 labelled communities.
- Health check: 184 AST edges point at external symbols (std headers/types) that are not nodes, and 33 same-endpoint edges collapse in the undirected graph.
- `graphify query` answers from graphify-out/graph.json. `graphify hook install` added post-commit/post-checkout rebuild hooks. graphify-out/ stays local (gitignored); the hook's .gitattributes merge driver was not kept, because the graph is not a tracked team file.

### 2026-10-12T20:15+02:00 — Pose composition recovered (REA, Ghidra)

- Call chain: 0x10f800 (ev_4dd35b1b…6e5) clears a blend state, then 0x2533e0 (ev_ef46d59d…730) adds weighted pose layers through 0x114b58 (ev_732ce83c…7ff, which calls the pose cache 0x1205f8). 0x2530f0 (ev_83cb1c23…509) then calls 0x252cb0 (ev_31aa99b7…9b9), which builds the joint matrices.
- Pose entry (0x40 bytes, written by 0x12d4b0): floats 0–3 quaternion, 4–6 channels 3–5, 7–9 channels 6–8, 10–12 channels 9–11, word 13 the channel mask.
- 0x252cb0, per joint:
  - With more than one layer, it blends quaternions with a sign flip on a negative dot product (0x24c924), normalizes them (0x24c898), and weight-sums the other groups.
  - It then calls 0x24c734(out, quat|0, scale|0, shear|0, translation|0). Each pointer is NULL when that mask group (bits 0–2 / 3–5 / 6–8 / 9–11) is empty, as confirmed by the call-site disassembly at 0x252f34–0x252fb4.
- 0x24c734 (ev_94f85e30…81f):
  - It starts from the rotation 0x248870 (ev_8c943203…64e), or identity.
  - If a shear is present: row2 += row1·h2, row1 += row0·h1, row0 += row2·h0.
  - It scales each row by 0x248850 (row i × s_i).
  - The translation goes into row 3; when it is absent, a vector from 0x56d400 (BSS, runtime value unknown, assumed zero) is used.
  - So channels 6–8 are a shear.
- 0x248870 is the rotation matrix of quaternion (x,y,z,w): rows [1−2(y²+z²), 2(xy−wz), 2(xz+wy)], [2(xy+wz), 1−2(x²+z²), 2(yz−wx)], [2(xz−wy), 2(yz+wx), 1−2(x²+y²)]. This is the transpose of the former preview default, which confirms in code the convention that the 19:15 visual test favoured.
- Hierarchy: world = local · parent.world (0x258a10 multiplies in place, A = A·B, ev_538f88ae…5f0). The skin matrix is rigid-inverse(bind world at joint+8) · world (0x24c5a0, ev_ba75c37d…587, which uses the transposed 3×3).
- After posing, 0x2530f0 applies optional per-joint s16/256 scales (skipped when all three are 0x100) through 0x24c2c4. This step is not replicated.
- tools/convert_anims.py: quaternion_matrix() and local_matrix(); the preview uses them, and `--transpose-rotation` is removed. tests: test_local_matrix. Python PASS 44/44.
- The char_bear walk frames 0/12 render as a natural walk.

### 2026-10-12T20:45+02:00 — Native skeletons, animation curves and skinning (S4e groundwork)

- Fixed tools/convert_models.py decode_vertices: the layout loop variable shadowed the mesh-centre argument, so Python never added the centre. The C++ parser was correct. A regression assertion was added.
- Moved posing into tools/convert_anims.py: skin_matrices() and rigid_inverse() (0x24c5a0). preview_model calls it, and char_bear walk frame 12 renders pixel-identically.
- Added src/assets/anim.cpp (chase_assets):
  - parseSkeleton
  - parseAnimation (consumes and checks every block, keeps the +0x08 per-joint curves)
  - decodeChannels (0x248910)
  - eulerToQuaternion (0x248248)
  - localMatrix (0x24c734 with 0x248870)
  - skinMatrices (single-layer 0x252cb0)
  - multiply()
  Vertex gains skin weights and joints (flag 2, after colour).
- Factored the pooled block reader and cString/readF32 into src/assets/block_reader.hpp, shared by model, level and anim.
- assets-core: synthetic bit stream, signed base, raw-matrix header, local-matrix rotation/scale/translation and shear, skeleton parse and unconsumed-block rejection, animation parse, and root/child skin matrices.
- assets-real-data: 376 skeletons / 5280 joints and 1954 animations / 30081 joint curves, equal to the Python reference. The char_bear walk skin-matrix |sum| over 48 frames is 5397.1758, within 1e-3 of Python.
- `ctest --test-dir build --output-on-failure` — PASS 7/7; 0 warnings. Python PASS 44/44.
- Still a hypothesis: skin index = joint − 1. Blending of more than one layer, the 0xfa raw-matrix form (the bind local is used instead), and the per-joint s16/256 post-scale are not replicated.

### 2026-10-12T21:30+02:00 — Bone palettes recovered; skinned playback in chase-viewer

- The vertex-buffer block at header +0x24 (previously skipped) is the bone palette table, with its count at header +0x20 (1 or 2).
  - Each 0x110-byte record is u32 triangles, u32 palette size, u32 first vertex, u32 vertex count, then u8 palette[256] of skeleton joints. At most 26 entries are used, matching the 26-entry "Bones" shader uniform.
  - Batches cover consecutive triangle runs. A first reading of the third word as "first triangle" was wrong (it failed on 7 parts, e.g. squirrel.bfnmdl 282 triangles vs 284).
- Checked over all 1457 skinned parts (data, not traced in code):
  - triangle counts sum to the index count
  - the block size is count × 0x110
  - every batch's indices stay inside [first vertex, first vertex + count)
  - no skin index is outside its palette
  - no vertex shared between batches resolves to different joints
  - 514 vertices are referenced by no triangle
- This replaces the joint − 1 hypothesis, which only matched char_bear because its palette is 1..21. The bear's eye part has palette [5] (Head). char_squirrel's palette contains joint 0.
- Python: convert_models.bone_batches() and skin_joints() (the +0x24 block is kept as buffer 'b24'); preview_model skins through resolved joints. C++: ModelPart::boneBatches, and parseModel resolves Vertex::joints to skeleton joints, rejecting conflicts and out-of-range indices. skinVertex uses the resolved joints.
- Bug fixed: the C++ rigidInverse used −t·column instead of −t·row (0x24c5a0 uses rows). Bear binds are translation-only, so the bear reference missed it; it was found through char_squirrel. Added a synthetic rotated-bind identity check, and char_squirrel run frame 5 as a second real-data reference.
- assets-real-data (Python references): 1457 skinned parts / 1465 batches / resolved-joint sum 3492824 over drawn vertices; |posed position| sums char_bear walk@12 1754.6224 and char_squirrel run@5 440.7128 (within 1e-3).
- chase-viewer: `--skeleton S.bfnskl --anim A.bfnanm [--frame N]` poses the first model argument. Skinned parts are skinned on the CPU and re-uploaded when the frame changes. The playback rate is not recovered; 30 fps is assumed.
- Offscreen screenshots: char_bear walk 0/12 and char_squirrel run 0/3 show coherent gait poses. The squirrel renders almost black; the material/combiner setup is not recovered.
- `ctest --test-dir build --output-on-failure` — PASS 7/7; 0 warnings. Python PASS 45/45.

### 2026-10-12T22:10+02:00 — Frame interpolation recovered; playback rate traced to the instance

- 0x2533e0 calls 0x24bf80 (ev_d1801bf3…488) per active animation slot (0x58-byte slots at instance+0x2c). It returns frame A, frame B and their weights. 0x114b58 is then called twice (layer 0 = A with weight 1−frac, layer 1 = B with frac), so the game always interpolates neighbouring frames. Layers of weight 0 are skipped, so whole frames use the single-layer path.
- 0x24cbd4 (ev_01398…5a6) computes frame = (clock − start) × slot+0x30 / clock+0xc + start offset:
  - The clock is 0x256af4 (ev_5678829b…3ea6), a generic timer in units of clock+0xc per second, so slot+0x30 is the playback rate in frames per second.
  - Modes are slot u16 +4 & 7: 0 clamp, 1/6 loop, 2 reverse, 3 ping-pong, 5 reverse loop. Frame B uses time + 1 frame and wraps the same way.
  - The rate is written by the caller that starts the animation. It is not in the .bfnanm header (all 1954 headers tallied; words +0x14/+0x18/+0x24 are always 0) or in options*.txt. It remains unknown.
- Multi-layer blend in 0x252cb0, from the existing pseudo-code (ev_31aa99b7…9b9) and disassembly of 0x24c898/0x24c924:
  - Quaternions are summed with weights, negated when dot(first, q) < 0, then normalized (identity when the length is ≤ 2⁻²³, bits 0x34000000).
  - Scale is summed over every layer when any layer has scale.
  - Shear and translation are summed only over the layers whose own mask has them.
- C++: JointPose, jointPose, blendPoses, poseMatrix; skinMatrices takes a double frame and blends floor/next with wrap-around. Python: joint_pose, blend_poses, pose_matrix, fractional skin_matrices; preview `--frame` accepts fractions.
- Tests: synthetic halfway translation (1.525), last→first wrap, sign flip, translation-only-from-carriers. Real-data |posed| sums at char_bear walk 47.5 (1741.8781) and char_squirrel run 2.25 (372.6377) match Python within 1e-3.
- chase-viewer: poses every rendered frame at fractional time; `--frame` takes fractions; `--fps` (default 30, an assumption) sets the rate.
- ctest PASS 7/7; Python PASS 46/46; 0 warnings.

### 2026-10-12T22:40+02:00 — Texture v orientation corrected (most "black material" cases)

- Corrects the S4b/18:05 note "GL's first row is v=0, which the game also treats as the bottom": UV v=0 is the TOP row of the decoded (top-down) image.
  - The decoder itself is right (the splash1top text reads upright).
  - char_squirrel: the body UVs (v 0.47–0.99) must hit the orange lower half of SquirrelALPHAT64, and the eye part (v 0.02–0.54) the eye decal in the upper half. With the old convention both were swapped, so the body sampled the black decal area.
  - pickup_chasetoken_imposter: the minifig face is upside down under the old convention and upright under the new one.
- chase-viewer uploads decoded rows unflipped. preview_model samples row v·h.
- Result: char_squirrel renders orange with black eyes. lvl_00_policestation room3 shows the textured floor, emblem, plants and lightmap shading where large areas were black before. The scene is still dim; the combiner/lighting setup is unrecovered.
- Graphics API, from strings: the game links Nintendo DMPGL (a dmp_* reserved-uniform table at 0x35b288, 0x10-byte entries {name, id, count, GL type}). The dmp_TexEnv names are referenced only from that table, and no static PICA TEV command headers (regs 0xc0–0xf7) exist in code.bin, so the combiner setup must be traced through the game's uniform-location usage. Not done.
- ctest PASS 7/7.

### 2026-10-12T23:05+02:00 — Placement Euler order recovered (replaces the Ry·Rx·Rz assumption)

- "initialorientation" (string at 0x1a49c4, referenced at 0x1a4860 in object setup 0x1a4130, ev_eb095374…8f7 / ev_8c852395…6e0): the parameter is looked up by hash (0x24da28 + 0x217080). Its three floats are multiplied by 0.0174533 (literal at 0x1a49d8) and passed to 0x237c48.
- 0x237c48 (disassembly), using sincos 0x249038 (r0 = sin, r1 = cos), writes the rotation rows [cy·cz, sx·sy·cz + cx·sz, sx·sz − cx·sy·cz], [−cy·sz, cx·cz − sx·sy·sz, sx·cz + cx·sy·sz], [sy, −sx·cy, cx·cy]. Rows 3 and column 3 are left untouched.
  - A numeric search over all 48 axis-order and sign combinations matches only Rz·Ry·Rx for row vectors (x applied first), i.e. Rx·Ry·Rz in column-vector terms.
  - The viewer's Ry·Rx·Rz assumption mirrored the yaw of objects with ±180 on X and Z: the game gives Ry(180 − y), the assumption gave Ry(180 + y).
- Only object setup 0x1a4130 was traced. That other classes using "initialorientation" (12 more string copies) share 0x237c48 is assumed, not checked.
- lvl_00_policestation_main: 120 of 130 placements are yaw-only or zero, so only a few props change in room3.
- chase_assets placementMatrix (level.cpp) and convert_levels.placement_matrix replicate it, with tests for (90,0,0) mapping +y to +z and (180,30,180) equalling a yaw of 150. chase-viewer uses it directly (same memory layout as GL column-major).
- Material combiners: the renderer keeps bit-packed GPU state (e.g. 0x126f0c, a 4 KB state packer). Not traced further.
- ctest PASS 7/7; Python PASS 47/47.

### 2026-10-12T23:25+02:00 — Whole levels in the viewer

- Meshname resolution: 6561 of 6563 placements over all 856 levels resolve to archive models; only xx_goldstud and Prop_Hotspot_Mission do not.
- Level models: models/<level>/ holds lvl_*.bfnmdl (plus _lod1) and room<N>.bfnmdl (403, plus 288 _l1 detail levels). chase-viewer `--level` without model arguments loads that folder minus the _l1/_lod1 models, or just room<N> with `--room`. This is a viewer convention; the game's streaming logic (Pregen/*.bin) is not recovered.
- `--eye X,Y,Z` sets the camera position.
- lvl_01_cherrytreehills_main: 1161 instances, 845580 triangles, 0 missing textures, about 1.1 s to load offscreen. A street view at (800,60,1060) shows the plaza, arch, roads, vehicles and water plane, textured and placed.

### 2026-10-12T23:55+02:00 — Navigation grids (.bfnnav) decoded

- Loader 0x1971c4 (ev_1af6d00b…a39a): the ".bfnnav" string at 0x197660 (ev_16ef5cae…3b5) builds "<level>_<room>.bfnnav". Counts come from the level, not the file. Block order (−14 pool):
  - grid records (0x3c each)
  - per grid: width × depth cell bytes (padded to 4), then (u16 +0x34 & 0x3fff) × 4 extra bytes when non-zero
  - path records (0x20 each)
  - per path: u16 indices (count u32 +0x18, padded)
- Grid record (data): u16 +0 x cell, +2 height, +4 z cell, +6 width, +8 unknown (~1400 in lvl_00), +0xa depth; +0xc and +0x38 runtime pointers; 16 u16 links at +0x14 (0xffff = none). Path record: f32 position, f32 ×3, u32 count.
- The level's section +0x30 (s30, previously only consumed) holds the navigation descriptors:
  - each 0x28 record has a name list (the rooms) and floats at +0xc..+0x1c: height unit, cell size (0.3 in lvl_00), origin x, y, z
  - its block holds one 0x18 descriptor per room: u16 grid count, u16 path count
  - The census checks all 700 nav files against their level descriptor counts: 0 mismatches. convert_levels.nav_rooms() and C++ Level::navRooms expose them.
- World mapping (data fit, not traced in code): x/z = origin + cell × index (bounds match the room models). y = origin.y + height × unit / 256: per-room least-squares slopes give unit/256 within 1–6%, and the result sits ~0.1 under the floor (room5: −0.10 vs 0.0).
- Cell flags (from the overlay): bit 0 is set on edges and obstacles (429583 of 3895948 cells); bits 2–4 look like edge directions. The cell semantics are inferred.
- Added tools/convert_nav.py (`--census`, cell_world) and tests/test_convert_nav.py (3 tests). Added src/assets/nav.cpp parseNav and NavRoom.
  - assets-core: synthetic nav file and rejections.
  - assets-real-data: 700 files / 28646 grids / 4607 paths / 429583 flagged cells; 700 descriptors match.
- chase-viewer `--nav`: cyan/orange quads, 0.25 above the fitted height. In lvl_00 room3 the grid follows the floor plan with orange cells along walls and props.
- ctest PASS 7/7; Python PASS 50/50.

### 2026-10-13T00:25+02:00 — Localisation tables (.loc) decoded

- Layout (data): "LOCA", u32 version 2, u32 count, u32 table bytes (= count × 8), u32 string bytes (header + table + strings = file size); then (u32 key, u32 offset) pairs sorted by key; then NUL-terminated strings. All 66 files (25207 entries) parse.
- Keys are the sdbm hash h = h × 65599 + c of the upper-case identifier.
  - Found by brute-forcing the multiplier on "OK" → 0x4f13bc (only 65599 fits) and confirmed on "NO" and "ON".
  - 37 of 351 gametext_uk entries hash from their own upper-cased text, and 212 keys over all files resolve from code.bin strings (e.g. PAUSEMENU, CLASS_MINER).
  - The hash routine was not found in code (no 0x1003f literal or shift pattern); data-only evidence.
- Text is Latin-1 (e.g. "Français" 0xe7). 0xac stands where ™ belongs ("Nintendo\xac3DS"); not confirmed.
- Byte 0x01 + 2 bytes is a glyph escape (Russian, Polish). It is neither an offset into extended_chars_utf8.txt nor a constant code-point offset, and probably indexes the .tnf glyph table (8-byte x, y, w, h records). Not decoded.
- Added tools/convert_loc.py (dump with `--names`, `--census`), tests/test_convert_loc.py (3), and src/assets/text.cpp (textKey, parseLocalisation) with synthetic and real-data (66/25207) checks.
- ctest PASS 7/7; Python PASS 53/53.

### 2026-10-13T00:45+02:00 — Text escapes decoded (Cyrillic/Polish)

- Escape 0x01 '2' c → character c − 0x30 of localisation/extended_chars_utf8.txt (84 characters, in file order).
  - Found by frequency: the most common ru codes ':' 'C' '7' '>' '<' 'A' map to о а и н е т at offset 0x30, matching Russian letter frequency.
  - Confirmed by readable text ("Начальник тюрьмы Филдинг", "Сохранить и выйти"). After decoding, 0 escapes remain over all 66 files.
  - The earlier brute force had stopped at offset 0x2b. The meaning of the '2' byte is unknown (5295 of 5306 ru escapes use it; the rest look like data, not escapes).
- Data oddity: the files with the "_pl" suffix hold Dutch text (e.g. "Opslaan en stoppen").
- Python decode_text (`--extended`); C++ extendedCharacters/decodeText (UTF-8 output).
- Tests: synthetic escape/Latin-1/unknown-escape cases. Real data: FNV-1a of decoded gametext_ru = 0xc0c84ff0 in both implementations.
- ctest PASS 7/7; Python PASS 54/54.
