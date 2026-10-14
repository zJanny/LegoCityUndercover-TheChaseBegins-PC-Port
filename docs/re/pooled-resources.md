# Shared pooled-resource layout

Reverse-engineering findings, moved verbatim from the progress log. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis.

<a id="ts-20261012-1555"></a>

## 2026-10-12T15:55+02:00 — Shared TT pooled-resource layout (all b* formats)

- 0x107288 rewrites a `.fnmdl` request to `.bfnmdl`, opens it and calls deserializer 0x10ccf4.
- The deserializer brackets reads with 0x250620(stream, marker), which pushes onto a pool stack at +0x138/+0x13c, and 0x250610, which pops it.
- 0x253800 returns the next block of the current pool (−15 at +0x14c, −16 at +0x160, −14 at +0x174; cursor, count, size array, base, offset). When the pool is exhausted it calls 0x24b740, which reads the next group's sizes from the table and then reads that group's data. A −16 group is not read there.
- Layout confirmed across all shipped files, implemented in tools/tt_resource.py:
  - Header: `u32 table_words<<8, u32 header_bytes`.
  - Table: table_words × u32, each a pool marker (−14/−15/−16) followed by that pool's block sizes.
  - −16 block data follows the table and is included in header_bytes.
  - −15 and −14 block data follow in table order and end exactly at the end of the file.
- [Corrects audio.md §2026-10-12T15:45](audio.md#ts-20261012-1545): the "version 0x400/0x600" noted earlier for .btga/.bwav is this word count (4 and 6). The texture "constants" are the table [−14, 0x20, −14, size]. The .bwav wrapper is a 0x2c record plus one CWAV block per part. convert_audio.py now splits it through this parser.
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

<a id="ts-20261013-150853"></a>

## 2026-10-13T15:08:53+02:00 — tools/tt_resource.py removed

- tools/tt_resource.py (`--census`) was removed. `chase::assets::parsePooled` and the pooled checks in tests/assets_real_test.cpp replace it. The census (12500/12500 pooled files close exactly) and the particle and table reference values that name it stay valid. Provenance: `python3 tools/tt_resource.py --census artifacts/fib/files` at 9d04c91. To reproduce, check out that commit (for example `git worktree add /tmp/old 9d04c91`) and run the command there.
