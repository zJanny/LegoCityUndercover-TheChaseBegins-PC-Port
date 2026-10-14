# Save format

Reverse-engineering findings, moved verbatim from the progress log. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis.

<a id="ts-20261013-1030"></a>

## 2026-10-13T10:30+02:00 — S4f save format recovered (REA, Ghidra 12.1.2; read-only)

Corrected by [save-format.md §2026-10-13T12:30](save-format.md#ts-20261013-1230): +4..+7 are two per-slot pairs
Corrected by [save-format.md §2026-10-13T08:53](save-format.md#ts-20261013-0853): options +4/+5 are sound volume, +6/+7 music volume
Corrected by [save-format.md §2026-10-13T08:56](save-format.md#ts-20261013-0856): the slot summary guess is unconfirmed

- Path "data:/savegame.dat" (0x36741c). 0x24bc0c mounts "data:" and opens the file with mode 7, read/write/create (ev_7bc912c1…, ev_7004d64d…). 0x24baa0 maps FS results to statuses 1/4/5; 4 = corrupted and 5 = not formatted are inferred.
- I/O runs on "saveiothread" 0x1688cc (ev_b8f03e66…, ev_56a85bfb…), by mode:
  - 1: probe (0x2a7928).
  - 2: load (0x29ff4c), a whole-buffer read; a short read gives status 3.
  - 3: save (0x1166f4), a write with flush, then 0x120a58("data:"), inferred to be the commit (ev_7b43e130…).
  - 5: 0x2ae218, probably format (inferred).
- Save manager: context at *0x379978 (0x15baa8, 0x2281c8, 0x228058, 0x228044); the buffer is set by 0x228244 (ev_0c1844a8…).
- Layout descriptor from 0x1a1fb8 → 0x1569d8 → 0x272948 (ev_56a85bfb…, ev_3efeec49…): options 0x21c bytes, slot 0x5230 bytes, 2 slots, version 0x1e. File size 0xa694 (42644 bytes), little-endian:
  - 0x00 u32 CRC, 0x04 u16 size, 0x06 u16 version 30, 0x08 "LEGO_ENGINE_SAVE" (16 bytes, no NUL). The header is written by 0x1fc604 (ev_de1097ad…).
  - 0x18: options block, a raw copy of live 0x51b490.
  - 0x234: slot 0. 0x5464: slot 1. Each is a raw copy of live 0x51b6b0 (0x1d0ba0, 0x1d0b80 save; 0x1c3478 load).
- CRC (0x248308, ev_3aed9451…, table ev_712d1c1c…): reflected polynomial 0xedb88320, initial value 0xffffffff, no final XOR, computed over [0x18, end). This equals ~zlib.crc32.
- Load check 0x1d0acc (ev_3b887da8…, ev_2d9d94d6…): a bad magic gives status 3; a CRC, size or version mismatch gives 4. Either resets the data to defaults (0x2081cc).
- Options fields:
  - +2 is the last-used slot (0xff = none).
  - +4..+7 hold defaults 10 and 8, inferred to be music/SFX volume (the UI has Music, SFX, InvertCamera and Language).
  - +0x1ec holds 2 × 0x18 slot summaries (inferred).
- Slot fields:
  - +0x07 bit 2 marks the slot as in use (inferred).
  - +0x5148 is a u64 stud count (inferred: a cheat sets 10000000).
  - +0x5190 is a u32 stud total (inferred).
  - +0x51ee is a 64-bit field cleared from the table at 0x372cb8 (meaning unknown).
- options.txt and options3ds.txt are engine logging configuration, not user settings; their loader is unknown. User settings live only in the options block.
- Unknown: story progress, unlocks and collectibles inside the slot; how the options block is restored on load; exact FS commands behind 0x120a58 and 0x325a04. The defaults callback 0x1a25cc unlocks character IDs 9..140 (table 0x372e10, stride 0x28); how these IDs relate to the 192 tablescharacters rows is unknown.
- Proposed next step (not started): chase_save under src/save/ (no SDL). It keeps the image as opaque bytes with header, CRC and validate (statuses 3/4), stores it at $XDG_DATA_HOME/chase/savegame.dat with an atomic write (temp file, fsync, rename), and has synthetic tests only (no real save is available).


<a id="ts-20261013-1210"></a>

## 2026-10-13T12:10+02:00 — Header size and version sources (REA, Ghidra 12.1.2; read-only)

- Confirms the 10:30 entry. Descriptor builder 0x272948 (ev_ff261669…) stores the running total 0x18 + options + slots × slot size at descriptor+0x1c. The header writer 0x1fc604 (ev_60bfa19c…) writes that total, truncated to u16, at file +4, and descriptor+10 (version) at +6. So the size field holds 0xa694, the whole file size.
- Load check 0x1d0acc (ev_53486e92…) compares the u16 at +4 with descriptor+0x1c and the u16 at +6 with descriptor+10. The magic check counts matching bytes over all 16, so a partial match still fails.
- 0x272948 also formats "Profile data [slot %u]" per slot; the label's use is unknown.
- Implemented as chase_save (src/save/), opaque blocks only. Unknowns: the defaults written by 0x2081cc, so a reset image is zero-filled rather than the game's defaults.

<a id="ts-20261013-1230"></a>

## 2026-10-13T12:30+02:00 — Reset defaults 0x2081cc (REA, Ghidra 12.1.2; read-only)

Corrected by [save-format.md §2026-10-13T08:53](save-format.md#ts-20261013-0853): options +4/+5 are sound volume, +6/+7 music volume

- [Corrects save-format.md §2026-10-13T10:30](save-format.md#ts-20261013-1030): +4..+7 are two per-slot pairs, not one music/SFX pair. The descriptor from 0x1a1fb8 (ev_c8c0b40e…) holds sizes, slot count 2, version 0x1e, the slot callback 0x1a25cc and the options callback 0x1a25a0.
- 0x2081cc (ev_4e9c96fe…) zeroes the whole buffer (0x24d234), then sets the options block:
  - +0 = 0x8a: high nibble = global 0x379d64 (8), low nibble = 0x379d66 (10). Initial globals 08 08 0a (ev_a37531a1…). 0x1c33c0 applies the same nibbles from these globals, apparently a second reset path.
  - +1 = language: 0x21b650 (ev_b616888e…) maps the 3DS CFG system language (0x3292fc reads block 0xA0002, ev_57575aae…) as 2→2, 3→4, 4→3, 5→5, 7→12, 8→6, 9→8, 10→9, otherwise 1. 0x2babe0 (ev_4d5865e7…) falls back to 1 when the index is not in a runtime list at *0x376ae8 (stride 0x18; the pointer is 0 in the static image).
  - +2 = 0xff (last slot: none). +3 bits 2..5 = 0x379d65 (8).
  - Callback 0x1a25a0 (disassembled with llvm-objdump; not a Ghidra function): +4 = +5 = 10, +6 = +7 = 8, +3 bit 0 set, bit 1 cleared. Result: options start 8a LL ff 21 0a 0a 08 08, the rest zero.
- Then each slot is zeroed (0x2594d8, ev_66267ca4…) and the slot callback 0x1a25cc (ev_bea3ecb3…) runs. In the slot buffer it sets +0x51b3/+0x51b4 from a runtime object (*0x375914 +0x97/+0x98), ORs +0x51e0 with 0x10 and +0x520b with 0x20, and applies cheat stud bonuses (+0x5148 u64, +0x5190 u32; +0x520b bits 0/1 mark the 100000/300000 bonuses as given). It also writes live globals (0x51b6b6, 0x52086b, 0x52089d.., 0x52089e bitset, character unlocks 0x231fc0 for IDs 9..140) and options +4+slot = 10, +6+slot = 8, +0x1ec + slot×0x18 = 0.
- Implemented: SaveImage::resetOptions and languageForSystem in chase_save. The nibble meanings (likely volume or similar sliders) and the language index order are unknown. Slot defaults are not reproduced: they depend on runtime values and on how live globals reach the buffer.

<a id="ts-20261013-0853"></a>

## 2026-10-13T08:53+02:00 — Option fields and language order (REA Ghidra 12.1.2 + llvm-objdump; read-only)

Corrected by [arm-executable.md §2026-10-13T08:55](arm-executable.md#ts-20261013-0855): slot-table entries 12 and 13 hold channels 8 (L) and 10 (R)

- [Corrects save-format.md §2026-10-13T10:30](save-format.md#ts-20261013-1030) and [Corrects save-format.md §2026-10-13T12:30](save-format.md#ts-20261013-1230): options +4/+5 are the **sound** volume and +6/+7 the **music** volume, one byte per slot.
- Options menu handler 0x31af98:
  - Row 0 steps global 0x37751d by 1, stores it at options +4 + last slot (only when options +2 ≤ 1), and calls 0x1f4408 → 0x1dca5c with value / 10.0. 0x1dca5c stores the gain and marks 24 voices dirty.
  - Row 1 does the same with 0x37751e, options +6 + slot and 0x1f36ec (clamps to 0..1 and scales a stream at 0x376b30).
  - Row 2 XORs options +3 with 2.
- Draw code 0x1e4804 labels the rows with text keys SOUND_VOLUME (0x4a6848ca), MUSIC_VOLUME (0x3e17bd14) and INVERTCAMERA (0x27bcbd1b). The value is YES (0x2c187527) when +3 bit 1 is set and NO (0x4e1381) otherwise. French (language 2) uses a different label/value separator string. Keys were resolved with the sdbm hash from text-localisation-fonts.md.
- Options +3 bit 1 = invert camera. 0x2009e8 adds two inputs (slot table entries 12 and 13, *0x376a18/*0x376a1c; channels L and R, see arm-executable.md 08:55) times ±2.0 to *0x589a70 +0x140; the bit swaps the signs.
- Options +3 bit 0 = subtitles (inferred). The second options screen (handler 0x2fb53c, draw 0x1e9358) toggles bit 0 on menu row 4. Its draw code prints the debug strings "subtitles!!!!" and "inv cam!!!!" for rows 4 and 5. 0x2fb484 snapshots sound volume, language and +3 bits 0/1, and starts a save when any of them changed.
- Language index order, from two independent tables:
  - The sprite switch at 0x31dc84 maps 1 UK, 2 French, 3 Italian, 4 German, 5 Spanish, 6 Dutch, 7 Danish, 8 Portuguese, 9 Russian (UI_Language_*.png).
  - The text-key switch at 0x175d34 maps 1..9 to LANG_ENGLISH, LANG_FRENCH, LANG_ITALIAN, LANG_GERMAN, LANG_SPANISH, LANG_DUTCH, LANG_DANISH, LANG_PORTUGUESE, LANG_RUSSIAN. Other indices use key 0xad698cda (unresolved).
  - 0x2e7bd4 writes a pending language (0x3741d8) to options +1 and reloads the top-screen logos. Index 4 (German) gets a separate "_ger" logo.
  - The language picker at 0x3077ac maps menu rows 0..8 to indices 1, 2, 3, 4, 5, 7, 6, 9, 8.
  - 0x22b650 returns the live language from 0x376adc. The list 0x1ecc40 returns 24-byte entries whose first byte is a language index; the list ends with 0.
- Implemented in chase_save: the Language enum, language/setLanguage, lastSlot, subtitles, invertCamera, and soundVolume/musicVolume per slot (setters clamp to 10). Test values were taken from manual disassembly.
- Unknown:
  - Options +0 (both nibbles) and +3 bits 2..5 have no reader among the 33 direct literal loads of 0x51b490/0x51b494. They are only written from globals 0x379d64..66 by 0x2081cc and 0x1c33c0 (callers 0x156a94, 0x1ddb88).
  - What language index 12 (from Korean system language) does.
  - Key 0xad698cda.

<a id="ts-20261013-0856"></a>

## 2026-10-13T08:56+02:00 — Options per-slot records (REA Ghidra 12.1.2 ev_3b2796d6…, llvm-objdump; read-only)

- Options +0xc + slot × 0x18 is a gift list, read by 0x1b0f5c (ev_3b2796d6…) from 0x322ec4 with index *(*0x374a30):
  - +0 u8 count (at most 5).
  - Then `count` pairs of (u8 type, u8 id).
  - Type 0 = character portrait (Sprites/Portraits/%s.png, table *0x372e10 stride 0x28).
  - Type 1 = portrait from table *0x372e14 (stride 0x18).
  - Type 2 = red brick (Sprites/UI_Shop/RedBricks/%s.png, table *0x374728 stride 0xc, name at +8).
  - Type 3 = postcard. Type 4 = story clip.
  - Empty entries show text key 0xac54bb90 and UI_Gifts_EmptySlot.png. The tables are runtime pointers (0 in the static image).
- Options +0x1ec + slot × 0x18 is a 24-byte record: u32 (non-zero = present) followed by 5 floats.
  - 0x2d86c0 copies the whole record to *0x374a30 +0x114, indexed by the signed byte *0x375df0.
  - 0x1cbf54 and 0x2be874 skip slots whose first byte is 0 and pass the record address to 0x20a8d4 (arguments 48, 64, 3200, 3; purpose unknown). [Corrects save-format.md §2026-10-13T10:30](save-format.md#ts-20261013-1030): the "slot summary" guess is unconfirmed; the floats suggest a saved position or camera state (inferred).
- Probable original bug: the slot-delete path 0x31ba64 sets the selected slot (*0x3741dc +0x55) to 0xff and then clears options[0x1ec + (s8)0xff × 0x18] = options +0x1d4 instead of the deleted slot's record (0x31baf4..0x31bb18).
- Unknown: the meaning of the five floats; the bytes between +0x3c and +0x1d4; how gifts are granted.

<a id="ts-20261013-0913"></a>

## 2026-10-13T09:13+02:00 — Slot progress layout and completion (REA Ghidra 12.1.2 ev_852ce460…, llvm-objdump; read-only)

Corrected by [save-format.md §2026-10-13T09:25](save-format.md#ts-20261013-0925): the exclusion loop reads byte +0xc, not +0x24

Offsets are relative to the slot block (live copy at 0x51b6b0; file slot 0 at 0x234, slot 1 at 0x5464).

- Character bits at +0x51bc (0x52086c), 2 bits per character id, ids below 141:
  - Bit 2·id = unlocked. Reader 0x213444 returns true when cheat flag 0x375961 or 0x375962 bit 0 is set.
  - Bit 2·id + 1 = second state, meaning unknown ("bought"/"available" fits). Reader 0x21793c: ids 1..8 use the unlocked bit instead; cheat bit 0 forces true.
  - Setter 0x232028(id, which): the first time a bit is set for any id except 9, it also sets bit id of a "new" set at +0x5211 (0x5208c1).
  - Unlock 0x231fc0(id) sets which = 0. It sets which = 1 too when character-table field +4 is 0. It then recursively unlocks the ids in table bytes +0x22 and +0x23 (table *0x372e10, stride 0x28).
- Completion 0x1cd3dc (ev_852ce460…) returns per-mille, capped at 1000, = collected × 1000 / total:
  - Per-level blocks of 0x820 bytes. Level L in 2..8 is at +0x8 + (L − 2) × 0x820 (up to +0x38e8). Level 1 is at +0x4928 (ends at +0x5148, where the stud count starts). Byte +0x81f of a block is that level's collected count; the total comes from the level table *0x375914 (stride 0x54, byte +0x40). The space at +0x38e8..+0x4928 would hold indices 9 and 10; no reader was seen.
  - Two 21-bit sets at +0x51b5 and +0x51b8: 42 items.
  - Characters 9..140: both bits each (264 items). Character-table entries with byte +0x24 == 1 are taken out of both counts (132 entries checked).
  - +0x51e3: 41 ids × 2 bits (ids 1..41). Cheat flag bit 1 forces both bits, and 0x375962 bit 1 the first bit. The meaning is unknown; vehicles would fit (inferred).
  - For 13 categories × levels 2..8: collected 0x20a630(category, level), total 0x20a7fc(category).
- Unknown: the per-level block contents other than +0x81f; the meaning of the 21-bit sets, the 41-id set and the 13 categories; the cheat flag bit assignment; where indices 9 and 10 go.
- Addendum (ev_90d48bc5…, ev_9bc72d3a…; llvm-objdump 0x203464, 0x1ffa74):
  - 0x203464(level) returns the level block above (level 1 → +0x4928, levels 2..8 → +0x8 + (L − 2) × 0x820). The block starts with u16[13] collected counts, one per category.
  - 0x20a630(category, level) returns that u16, except for category 7. Category 7 is the sum of four flags 0x1ffa74(4 × (L − 2) + k), k = 0..3. These are bits 0..27 of a set at +0x51fa (0x5208aa); cheat flag 0x375961 bit 6 forces them.
  - 0x20a7fc(category, level) reads the total from the runtime table *0x375fb8 (13 × 8 bytes, byte [category × 8 + L − 2]). So totals are not in the save.
  - 0x2f63dc fills *0x375fb8 from "Tables/TablesSuperCounters.xls" (shipped as tables/tablessupercounters.bxls) through 0x20f844. The same loader fills *0x3745fc from TablesWiretap.xls and *0x3746f8 from TablesPickups.xls. 0x1ffc14 adds to a level counter only while it stays at or below that total. Not checked: whether the table's columns match the 13 × 7 layout. If they do, completion can be computed offline from a save plus the tables (still missing: the level table *0x375914 and character-table byte +0x24).

<a id="ts-20261013-0919"></a>

## 2026-10-13T09:19+02:00 — TablesSuperCounters vs the 13 × 7 totals layout (chase-tabledump; no REA)

Command: `build/chase-tabledump tables/tablessupercounters.bxls` (source: `artifacts/fib/files/tables/tablessupercounters.bxls`, 184 bytes, `xxd` checked).

- Verdict: matches the layout read by 0x20a7fc (13 categories × levels 2..8, one byte per cell, 8-byte row stride).
- Dump: sheet 0 has 13 rows and 9 columns, types `0 3 3 3 3 3 3 3 3`.
  - Column 0 (type 0) is empty ("-") in every row.
  - Columns 1..7 (type 3, u8) are the totals for levels 2..8 in that order.
  - Column 8 (type 3) is 0 in every row, so it fits the unused 8th byte of each `*0x375fb8` row.
  - Raw data in the file: 13 × 8 bytes from file offset 0x50 (after the column types and three 0xcd padding bytes), each row = 7 totals + 0x00.
- Representative rows (levels 2..8): category 0 = 5 1 5 4 4 4 5; category 1 = 8 0 5 1 1 2 7; category 7 = 4 4 4 4 4 4 4; category 9 = 1 1 1 1 1 1 1; category 12 = 2 0 2 2 1 2 2.
- Cross-check: category 7 totals are 4 at every level, which equals the four flags per level that 0x20a630 sums for category 7. Categories 2, 4, 5, 6 are also all 4. Zero totals (categories 1, 10, 11, 12 at level 3) are consistent with a category absent from that level.
- The evidence is structural only (shape, stride, category 7). The loader 0x2f63dc / 0x20f844 was not re-read in this entry, so the copy of columns 1..8 into the 8-byte rows is inferred.
- Unknown: the meaning of the 13 categories; whether column 0 is stored anywhere (it is empty here); the level table *0x375914 and character-table byte +0x24, which are still needed for an offline completion value.

<a id="ts-20261013-0925"></a>

## 2026-10-13T09:25+02:00 — Level table, character table and the offline completion total (REA Ghidra 12.1.2 ev_6865bb9a…, ev_424ca108…, ev_9962dde4…, ev_f0bc83a6…; llvm-objdump; read-only)

- Loader: FUN_002f5c78 (body 0x2f5c78..0x2f6703; ev_aa4109a7… for the containing function of 0x2f5dd4) loads `Tables/TablesLevels.xls` (string 0x3586fc) and `Tables/TablesCharacters.xls` (0x358710), waits until loaded, then stores row-data pointers with 0x20f844(sheet, sheetIndex, 0) (ev_f0bc83a6…). 0x20f844 returns `*(sheet+4)[sheetIndex*0x1c + 0xc] + row × u16 *(…+4)`, i.e. the base of the sheet's packed rows. Vaddr to ELF file offset is `vaddr − 0x100000 + 0x1000` (text), checked with readelf.
  - `*0x375914` = TablesLevels sheet 0 (stored at 0x2f5dd4, pool 0x2f60ac); `*0x374728` = sheet 1.
  - `*0x372e10` = TablesCharacters sheet 0 (stored at 0x2f5e6c, pool 0x2f60b8); `*0x372e18` = sheet 1, `*0x372cb8` = sheet 2, `*0x372e14` = sheet 4, `*0x372e1c` = sheet 5.
- Level table (`artifacts/fib/files/tables/tableslevels.bxls` sheet 0: 28 rows, stride 0x54, which equals the stride in the code). Offsets come from the layout rules in docs/re/data-tables-config.md, applied to the `chase-tabledump` types and stored columns (script, not committed). The computed stride equals 0x54. Row index = level id.
  - +0x00 string (name, e.g. "the_city" or ""), +0x04 u8, +0x08..+0x2c ten ints, +0x30 string (level scene name, e.g. "lvl_01_cherrytreehills"), +0x34 string ("generic1"/"hubstart"), +0x38 u8, +0x39 u8 (0x2f67fc uses it as an index below 14 into the u16 table at `*0x375d24`), +0x3c int, +0x40 u8, +0x41 u8, +0x42..+0x50 u8.
  - Confirmed from code (ev_9962dde4…): 0x2f67fc compares the string at +0x30 (and at +0x84, the next row's +0x30) with the current level name over 28 rows to set the level id (0x51640e), and compares the collected count with byte +0x41 for ids 8 down to 2.
  - The completion formula 0x1cd3dc (llvm-objdump 0x1cd3f4..0x1cd450) adds byte +0x40 of rows 2..8 to the total. Values: row 2 = 7, 3 = 8, 4 = 5, 5 = 8, 6 = 9, 7 = 8, 8 = 8 (sum 53). Rows 2..8 are lvl_01_cherrytreehills, 02_albatrossisland, 03_auburn, 04_fortmeadows, 06_apolloisland, 05_bluebellwood, 07_downtown (id order differs from the number in the name for rows 6 and 7). Byte +0x41 for rows 2..8 is 10, 10, 8, 10, 11, 10, 10 (use unconfirmed; the check in 0x2f67fc compares a count against it).
- Character table (`tablescharacters.bxls` sheet 0: 192 rows, stride 0x28 = stride used by 0x231fc0, computed layout agrees). Layout: +0 string name, +4 int (cost: 0, 200000, …), +8 int (hash), +0xc u8, +0xd u8, +0xe u8, then bool bytes and u8 fields up to +0x22 / +0x23 (the two unlock-chain ids) and a u16 at +0x24.
- [Corrects save-format.md §2026-10-13T09:13](save-format.md#ts-20261013-0913) ("character-table byte +0x24 == 1"): the exclusion loop at 0x1cd5bc..0x1cd5e4 starts at `*0x372e10 + 372` and steps 0x28 for 132 entries. 372 = 9 × 0x28 + 0xc, so it reads byte **+0xc** of ids 9..140, not +0x24.
  - Data: byte +0xc over ids 9..140 holds 1 only for id 9 ("police"); the other 131 values are 2..9 (value 9 occurs 19 times, each other value 16 times). So exactly one entry is excluded; each match subtracts 1 from both the collected and the total counter.
  - Byte +0xc looks like a character class or group (1..9); the meaning is unknown. The u16 at +0x24 is 0 in all 192 rows and is not read by the completion function.
- Offline total (inference from the loops, llvm-objdump 0x1cd3dc..0x1cd5e8, not run): the total counter r6 = 53 (levels) + 42 (two 21-bit sets) + 2 × 132 (characters 9..140) + 2 × 41 (ids 1..41) + 271 (sum of the 13 × 7 cells of TablesSuperCounters, summed from `chase-tabledump`) − 1 (id 9) = **711**. The result is `min(1000, collected × 1000 / 711)` if the divide is a plain unsigned division by the total (the 0x25c1b8 helper was not read).
- Unknown: the meaning of the level-row bytes +0x04, +0x38, +0x39, +0x3c and +0x41..+0x50, and of character-row byte +0xc; whether 0x25c1b8 is an unsigned divide; whether runtime code ever writes to either table (the static files are used as-is, no writer was searched for); the cheat-flag bits and the collected side of the 41-id and 21-bit sets are still as in the 09:13 entry.

<a id="ts-20261013-092607"></a>

## 2026-10-13T09:26:07+02:00 — Completion divide (llvm-objdump; read-only)

- 0x1cd5e8..0x1cd604: r0 = 1000 × collected (`mul r0, r5, r0`, 0x1cd5f0), r1 = total (r6), `bl 0x25c1b8`, clamp to 1000 (`movhi`), `uxth`. 0x25c1b8 is the compiler's unsigned shift-and-subtract divide (`rsbs r3, r1, r0, lsr #n` ladder). This settles the 09:25 caveat: completion = min(1000, (collected × 1000) / total) as u16, unsigned, truncating. A 32-bit overflow of collected × 1000 needs collected > 4 294 967, so it cannot happen.
- Unknown: nothing new.

<a id="ts-20261014-115300"></a>

## 2026-10-14T11:53:00+02:00 — Level state blocks, unlock bits and the progress natives (llvm-objdump, REA-free static reading of artifacts/game/game.arm.elf; real save copy; oracle `read` on `street-after-1-1`, OpenGL renderer, no RPC)

Replica: `src/save/slot_progress.{hpp,cpp}` (`chase_save_progress`), `ScriptProgress` in `ScriptRunner`, `chase-game --save FILE --slot N` (`src/game_save.hpp`). Tests: `slot-progress` (synthetic), `slot-progress-real-data` (copy of the user's save, skips without it).

### Level block (0x820 bytes, 0x203464; completes the 09:13 entry)

- +0x00 u16[13] category counters, **+0x1a u16 length of the state data, +0x1c u16 StoryProgress at save time, +0x1e u8 "state saved", +0x1f.. 2048 bytes of state data**, +0x81f collected (read by 0x1cd3dc and GetGameComplete). The block of the current scene is found by 0x21d4b8(scene), which matches the scene file name against the TablesLevels scene-name column (+0x30 of rows 1..8; row 1 = lvl_00_policestation, row 2 = lvl_01_cherrytreehills).
- Writer: 0x2e80fc (scene unload) calls the serializer 0x1c26dc(scene, block+0x1f, 0x800), stores the length at +0x1a, sets +0x1e = 1 and stores the scene variable "StoryProgress" (string 0x358354, lookup 0x241624 / 0x2451d0) at +0x1c. The same pair is at 0x192f60 (called from 0x21cf58).
- Reader: 0x2e8054 (scene start) calls 0x192e14(scene) after the scene setup (0x1d9594, 0x1bcf94, 0x197948, 0x29d6e8) and before it starts the scene's threads. 0x192e14: when +0x1e is set it calls the restore 0x1c2400(scene, block+0x1f, length). Then, only for a scene that is not the main scene (compare with *(0x589a18+4)) and whose level has the second unlock bit (below) set, it overwrites StoryProgress with byte +0x41 of the level row (level table `*0x375914`, stride 0x54).
- State data (restore 0x1c2400, save 0x1c26dc): records {u32 name hash of an object, u8 size, 3 bytes, `size` bytes of payload}; the record of hash 0 is the level data; the object records (hash found by binary search in the scene's object array, `scene+0x28`) go to the class's restore callback (class table 0x583124 stride 0x28, callback at +0x24); a final marker 0xffffffff is followed by the bit set of `scene+0x12f4` entries (flag 4 of the mode word 0x376aac).
- Level-data record (hash 0): the default parameters of the level file (section +0x10) with record flag bit 1 (`LevelParam::flags & 2`), in file order, written by value: types 0, 5, 7 four bytes (type 0 float, 5 and 7 integers, the port converts them to float as for the defaults), type 1 a NUL-terminated string, type 2 twelve bytes, types 3, 4, 6 nothing. The mode word has the parameter bit (2) only; the bit sets of bounds (bit 1) are not in the saved data. Checked on the real save: for lvl_01 the record is 219 bytes = 43 floats and integers (172) + the string "lvl_01_CherryTreeHills" (23) + two vectors (24), consumed exactly.

### Unlock bits and the natives (all read the live slot, base 0x51b6b0)

- 0x21d450(level, which): level >= 9 gives 0; otherwise bit `2*level + which` of the bytes at 0x520890 = slot +0x51e0 (byte `(2*level+which)>>3`). which 0 = unlocked, 1 = second state ("completed / visited"). With the cheat flag 0x375961 bit 2 it returns true. The slot defaults 0x1a25cc OR byte +0x51e0 with 0x10, so a new game has level 2 unlocked. `IsLevelUnlocked` (handler 0x2636e4, native table entry name at 0x162338) calls 0x21d450(level, 0) and writes 1.0 / 0.0 to its second argument.
- `GetGameComplete` (0x2635d0): the collected byte +0x81f of level block 8 (0x203464(8)) >= byte +0x40 of TablesLevels row 8 (`*0x375914 + 0x2e0`, value 8): 1.0 else 0.0.
- `IsRedBrickExtraActive` (handler 0x2a6d34, table entry at 0x162304) calls 0x2369a8(index) = bit `index` of the word `*0x374724`. That word is a runtime mask: its only writers are the setter 0x1e8cb4(bit, on) (callers 0x2fe2d4, 0x316ddc, 0x316e24: the menu toggles) and the reset 0x19c4c0 (called at 0x2d544c). No loader copies the slot into it (literal search of 0x374724: 0x19c4d8, 0x1e8cd8, 0x2369c0, 0x2f67b8 only), so after a load it is 0.

### The user's save (copy at artifacts/oracle/agentJ-saves/savegame.dat; the originals under ~/.local/share/azahar-emu/sdmc/Nintendo 3DS/<id0>/<id1>/title/00040000/000ad600/data/00000001/savegame.dat were only read)

- File 42644 bytes, valid (`chase-savecheck`: slot 0 2/711, slot 1 empty), stored CRC 0x937dacf8, options +2 = 0 (last slot 0), slot 0 byte +7 = 4 (in use), slot 1 all zero. The Azahar file layout is `data/00000001/savegame.dat` (plus a 16-byte `00000001.metadata`); the file is the same format as the game's.
- Slot 0: unlock byte +0x51e0 = 0x10 (level 2 unlocked only, no second-state bits); level 1 (police station) block has state data 201 bytes, StoryProgress 0; level 2 block has state data 823 bytes, StoryProgress 2, collected 1. Decoded level-data record of lvl_01: StoryProgress 2, 1-1_IntroPlayed 1, Entered_Police_Station 1, Enter_Police_Car 1, DIALOG_RETURNCODE 2, Music_NumberGen 1, Language_Spanish 2 (type 5; meaning unknown), other saved values 260, 1, 160, 10 and two type-5 values of 1. This is the progress "after mission 1-1".

### Oracle comparison (state `street-after-1-1`, `read 0x08e02284 19280` = the 482 bound records of the level data, GDB, `--renderer opengl`; the software renderer crashed three times at the state load in `LookupTexture`)

- The `chase-game` start with this slot (saved variables + `startup`) gives the same bound enable bytes (+0xa) as the oracle for 479 of 482 bounds; the three that differ (`CameraBound_Police_Helipad`, `CameraBound_VehicleDrop_01/02`) are the known SCRIPT-10 differences. The earlier route (`--call-script 1-1_MissionSkip` after a new-game startup) differs in 5 bounds (1-1_Get_Back_To_Donuts is enabled, `Police_Station_Transition_Upper` is off). Files: artifacts/oracle/agentJ-out/compare/bounds-street-after-1-1-vs-save-route.txt and ...-oracle-read.txt. Object enable flags (entities) were not compared.

Unknowns:
- The object records of the state data (class restore callbacks: doors, pickups, build-its, ...), the end-marker bit set (`scene+0x12f4`, 34 bytes in lvl_01) and what the hub block (level 1) holds besides its level-data record.
- Which level (and position) a Continue starts: the slot's own fields for it were not found (options +0x1ec records are floats; see 08:56). `chase-game` needs `--level`.
- Whether the override of StoryProgress by byte +0x41 (0x192e14) matters for the main scene (it is skipped for it) and what the second unlock bit (which = 1) is set by.
- The mode word 0x376aac: its value is inferred from the saved data (parameter bit only), not read.
- `Language_Spanish` is a saved parameter (type 5) of unknown meaning.

<a id="ts-20261014-134146"></a>
<a id="ts-20261014-134146"></a>

## 2026-10-14T13:41:46+02:00 — Object records, end bit set, current level and the second unlock bit (llvm-objdump on artifacts/game/game.arm.elf; oracle `read` on `street-after-1-1`, OpenGL, no RPC; real save copy)

Code: `objectRecords`, `slotCurrentLevel`, `tasksCompleteVariables` in `src/save/slot_progress.*`; tests `slot-progress`, `slot-progress-real-data`.

- Class table 0x583124 (stride 0x28, index = object type byte +0x12, save callback +0x20, restore callback +0x24) is filled at run time; read at `street-after-1-1`: exactly four classes have callbacks. Callback is called only when the object's halfword +0x82 has bit 0.
  - Type 0x15 (21, `props/prop_buildit`, string "BUILD IT "): save 0x15d380, restore 0x15d348. 1 byte: bit 0 = bit 4 of byte +0x37 of the object's data (+0x7c), bit 1 = inverted bit 0 of the halfword +0x10 of the node at data +0x1c. Restore: bit 0 calls 0x2428c0 (enable), bit 1 with a node calls 0x242cf0 (disable).
  - Type 0x43 (67, `props/prop_prop`): save 0x156550 stores `*(u16*)(obj+0x10) & 1`; restore 0x15654c: bit 0 calls 0x2428c0 (enable). Never disables.
  - Type 0x62 (98, `props/prop_superbuildit`): save 0x192608 stores `!(data+0x92 & 8) & (data+0x92 >> 1)`; restore 0x192548: only when bit 0 is set it sets bit 1 of data+0x92 and runs the enable and disable calls (0x2428c0, 0x242cf0, 0x244f40) and, by data+0x8c, 0x2258bc with 6 or 8.
  - Type 0xfc (252): 4 bytes. Save 0x192804 stores the scene variable "TASKS_COMPLETE" (lookup 0x241624); restore 0x1927a8 sets it back (0x236968) and, if 0x239678(obj) is true, calls 0x244fc8(obj, 0x58, 0). The 4-byte records of a state are all this class.
- Hash of a record = object field +8. For instances placed in a level file it is `nameHash(instance)` (checked: `FP_SuperBuildit_Statue_BP` = 0x91737ed7, type 98). The other records (47 of 48 in level 2, 1 in level 1) belong to objects that no .blvl of the archive lists; at `street-after-1-1` 66 live objects of the level scene (0x085abd20, +0x1c = 3338 objects, array at +0x24; object headers read with `read` for 3333 pointers) carry these hashes: all type 0xfc for the 4-byte records and type 0x62 for the 1-byte ones. Their names and creator are unknown.
- Records are written in the order of the scene object array (+0x24). The first object (index 0) is the level object: its class callback result is the offset before the level-data record, the bounds bit set (mode bit 1) and the parameters (mode bit 2) follow.
- **Mode word 0x376aac is always 2** (read 2 at `street-after-1-1`; its only writer is the setter 0x29444c, called once at 0x2f5d1c with 2). So the bounds bit set (bit 1) and the end bit set (bit 4) are never filled. The serializer still adds `(scene+0x12f4 + 7) >> 3` bytes after the end marker (0x1c2984 is reached by both paths), so the 34 bytes (level 2) and 7 bytes (level 1) are zero padding (the buffer is cleared by 0x24d234 first); the restore skips them with the same count only when mode bit 4 is set and does not read them. Nothing to apply.
- User's save, slot 0: level 2 holds 48 object records (6 of 1 byte, 42 of 4 bytes), level 1 holds 1 (1 byte); every payload is 0. Applying them changes nothing: a 4-byte record sets TASKS_COMPLETE to 0.0, a 1-byte record with bit 0 clear is a no-op for types 0x43 and 0x62 and for type 0x15 sets nothing visible.
- **Slot byte +6 = current level id** (signed byte 0x5163e8+0x26, the level of the running game). Writers: 0x1947e0 (called at 0x20a520 when a level is entered, together with `bit 2·level` of the unlock set for levels 2..8), 0x2f68ec, 0x2e14c0, default 0x1a2608. Readers 0x2b1100 and 0x2e7d50 copy it into a front-end structure, then call 0x217f88(0). The user's save holds 1 (police station), matching its level-1 block with saved state. Slot +4 (u16) is the completion per-mille written by 0x17cb3c / 0x23cbe8 (0x1cd3dc); +7 bit 2 = in use. No position or vehicle field exists in the slot.
- **Second unlock bit (`which` 1)**: the only code that sets a bit of the set at 0x520890 is the setter 0x1b2098(level, which) (bit `2·level + which`) and 0x1947a0 (`which` 0). Both callers of 0x1b2098 (0x1b2780, a script native taking one level argument, and 0x2f68e0) pass which = 0. No other literal reference to 0x520890 exists (0x1947f0, 0x1b20b8, 0x21d4b0 only), so no code writes `which` 1 and the 0x192e14 override of StoryProgress by level-row byte +0x41 never fires in a game played from a new save. (Slot data copied from a file could in theory carry the bit: not seen in the real save.)

Unknowns:
- Names, creator and meaning of the type 0xfc and 0x62 objects behind the unmatched hashes (`TASKS_COMPLETE` with 42 objects in level 2).
- Behaviour of type 0x15 and 0x43 records with bit 0 set (never seen set: their objects had flag +0x82 bit 0 clear in all 3333 headers read).
- Whether the front end starts the level in slot +6 on Continue and at which position (the copy at 0x2b1100 and 0x2e7d50 was read, its consumer not followed).
- What `scene+0x12f4` counts (0 at `street-after-1-1`, 265..272 when the user's save was written).
