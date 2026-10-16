# Image, RomFS and FUSE archive

Reverse-engineering findings, moved verbatim from the progress log. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis.

## Current state

- The image is a decrypted CCI (NCCH NoCrypto, product CTR-P-AA8P, `.code` compressed, SHA256 f895faae...); the original is never modified. [§2026-10-12T14:43:19](#ts-20261012-144319)
- RomFS: IVFC with 4 KiB blocks, 51 files (`lego_city3ds.fib`, its CSV index, 49 `cutscenes/*.moflex`); `tools/extract_romfs.py` verifies the full hash tree. [§2026-10-12T15:25:37](#ts-20261012-152537)
- FUSE archive: 20-byte header, then entries `{CRC, offset, size<<5 | flags}` sorted by CRC; codec = flags&3, block size 0x8000 << ((flags>>2)&7); open routine 0x17dea8, entry read 0x12894c. [§2026-10-12T15:32:32](#ts-20261012-153232)
- Codec 1 is a headerless RefPack/QFS variant (0x12efe0) with rearranged bit fields; only codecs 0 and 1 occur in all 12836 entries. [§2026-10-12T15:32:32](#ts-20261012-153232)
- Extraction (`tools/extract_fib.py`) yields 12836 entries, 12824 named through the CSV; sizes and offsets match the CSV. [§2026-10-12T15:32:32](#ts-20261012-153232)
- Open: codec 3 (0x12d50c) is not analysed and does not occur in this image.

<a id="ts-20261012-144319"></a>

## 2026-10-12T14:43:19.863039+02:00

- Original CCI untouched; NCCH NoCrypto flag 0x04, exheader hash valid.
- Product CTR-P-AA8P; program name LEGOCITY; .code compressed (exheader flag bit 0).
- .code SHA256 f895faaea826ee470e056d36fed72d66922b27ec9e5923e53f341567431b6923.
- REA rejected CCI container; ordinary Python probe is local evidence, not REA Evidence.

<a id="ts-20261012-152537"></a>

## 2026-10-12T15:25:37+02:00 — RomFS extraction (S1 extension, prerequisite for S4 assets)

- Added tools/extract_romfs.py and tests/test_extract_romfs.py (6 tests: tree/offsets, level 2/3 hash corruption, unsafe `..`/`a/b` and duplicate names, directory cycle, out-of-bounds data, end-to-end image extract, existing output refused, superblock mismatch, encrypted flag).
- Layout confirmed on the real image: IVFC v0x10000, master hash 384 B, all three levels with 4 KiB blocks; level 3 at RomFS+0x1000, then level 1, then level 2, each aligned. The RomFS superblock hash is at NCCH+0x1E0 over 1 media unit; NCCH+0x1C0 is the ExeFS one.
- `python3 tools/extract_romfs.py <CCI> --output artifacts/romfs` — PASS exit 0, 0.93 s; the superblock and full IVFC tree were verified; 51 files, 763156067 B; romfs-manifest.json sha256 d5fd75e6…be90e. CCI size and mtime were unchanged (1073741824, 851484720).
- Contents: `lego_city3ds.fib` (259357972 B, the main asset archive, format unknown), `lego_city3ds.csv` (1090227 B, probably its index), and 49 `cutscenes/*.moflex` (Mobiclip video; `_de` variants are German), including `splash5` and 18 `superbuildit_*`. Every non-video asset is inside the .fib.
- `python3 -m unittest discover -s tests -p 'test_*.py' -v` — PASS 15/15.

<a id="ts-20261012-153232"></a>

## 2026-10-12T15:32:32+02:00 — FUSE (.fib) archive format and codec recovered

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

