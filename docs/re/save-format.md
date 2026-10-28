# Save format

Reverse-engineering findings, moved verbatim from the progress log. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis.

Moved to [save-format-state.md](save-format-state.md): level state blocks, unlock bits, object records, Continue, save request chain and slot row fields.

## Current state

- File `data:/savegame.dat` is 0xa694 bytes: CRC u32 (reflected 0xedb88320, init 0xffffffff, no final XOR, over [0x18, end)), u16 size (whole file), u16 version 30, magic "LEGO_ENGINE_SAVE", options block at 0x18 (0x21c bytes), slot 0 at 0x234 and slot 1 at 0x5464 (0x5230 bytes each). [§2026-10-13T10:30](#ts-20261013-1030), [§2026-10-13T12:10](#ts-20261013-1210)
- Load check 0x1d0acc: bad magic gives status 3, CRC, size or version mismatch status 4; either resets to defaults (0x2081cc). [§2026-10-13T10:30](#ts-20261013-1030)
- Options: +0 nibbles 8 and 10 (no reader found), +1 language, +2 last slot (0xff none), +3 bit 0 subtitles (inferred), bit 1 invert camera; +4/+5 are the sound volume and +6/+7 the music volume per slot (default 10 and 8). The earlier "two per-slot pairs" and music/SFX readings are corrected. [§2026-10-13T08:53](#ts-20261013-0853)
- Each level load (loading-screen enter 0x303a40) copies the volumes of the slot at +2 into the live bytes 0x37751d (sound effects) and 0x37751e (music); +0 and +3 bits 2..5 are constant defaults that nothing reads; only the save task writes +2; movies play at the sound effects volume. [§2026-10-28T22:51:25](#ts-20261028-225125)
- Reset defaults: options start `8a LL ff 21 0a 0a 08 08`, language mapped from the 3DS system language (2 to 2, 3 to 4, 4 to 3, 5 to 5, 7 to 12, 8 to 6, 9 to 8, 10 to 9, else 1). [§2026-10-13T12:30](#ts-20261013-1230) The slot defaults of a new game (callback 0x1a25cc) are in [save-format-state.md §2026-10-19T20:24:11](save-format-state.md#ts-20261019-202411).
- Language indices 1..9: English, French, Italian, German, Spanish, Dutch, Danish, Portuguese, Russian; the picker rows map to 1, 2, 3, 4, 5, 7, 6, 9, 8. [§2026-10-13T08:53](#ts-20261013-0853)
- Options +0xc + slot x 0x18 is a gift list (count, then type/id pairs); +0x1ec + slot x 0x18 is a 24-byte record whose "slot summary" meaning is unconfirmed. [§2026-10-13T08:56](#ts-20261013-0856)
- Slot progress: character bits at +0x51bc (2 bits per id below 141), per-level blocks of 0x820 bytes (level 1 at +0x4928, levels 2..8 from +0x8) with u16[13] category counts; stud count u64 at +0x5148. [§2026-10-13T09:13](#ts-20261013-0913)
- Completion 0x1cd3dc is min(1000, collected x 1000 / total) as an unsigned u16; the offline total is 711, and the exclusion loop reads character byte +0xc (only id 9 is excluded), not +0x24. [§2026-10-13T09:25](#ts-20261013-0925), [§2026-10-13T09:26:07](#ts-20261013-092607)
- Totals come from `tablessupercounters.bxls` (13 categories x levels 2..8, one byte per cell), not from the save. [§2026-10-13T09:19](#ts-20261013-0919)
- Later save findings are in [save-format-state.md](save-format-state.md).
- Open: story progress inside the slot, the cheat-flag bits, meaning of the 13 categories, the five floats of the +0x1ec record, how the options block is restored on load.

<a id="ts-20261013-1030"></a>

## 2026-10-13T10:30+02:00 — S4f save format recovered (REA, Ghidra 12.1.2; read-only)

Corrected by [super-bricks.md §2026-10-19T19:20:44](super-bricks.md#ts-20261019-192044): slot +0x5190 is not a stud total but the super brick count (the u32 at 0x593430).

Corrected by [save-format.md §2026-10-13T12:30](#ts-20261013-1230): +4..+7 are two per-slot pairs
Corrected by [save-format.md §2026-10-13T08:53](#ts-20261013-0853): options +4/+5 are sound volume, +6/+7 music volume
Corrected by [save-format.md §2026-10-13T08:56](#ts-20261013-0856): the slot summary guess is unconfirmed

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

Corrected by [save-format.md §2026-10-13T08:53](#ts-20261013-0853): options +4/+5 are sound volume, +6/+7 music volume
Corrected by [save-format-state.md §2026-10-19T20:24:11](save-format-state.md#ts-20261019-202411): the callback unlocks only the characters whose TablesCharacters byte +0xc is below 2 (only id 9), and in the New Game call the "live globals" are the slot itself

- [Corrects save-format.md §2026-10-13T10:30](#ts-20261013-1030): +4..+7 are two per-slot pairs, not one music/SFX pair. The descriptor from 0x1a1fb8 (ev_c8c0b40e…) holds sizes, slot count 2, version 0x1e, the slot callback 0x1a25cc and the options callback 0x1a25a0.
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

- [Corrects save-format.md §2026-10-13T10:30](#ts-20261013-1030) and [Corrects save-format.md §2026-10-13T12:30](#ts-20261013-1230): options +4/+5 are the **sound** volume and +6/+7 the **music** volume, one byte per slot.
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
  - 0x1cbf54 and 0x2be874 skip slots whose first byte is 0 and pass the record address to 0x20a8d4 (arguments 48, 64, 3200, 3; purpose unknown). [Corrects save-format.md §2026-10-13T10:30](#ts-20261013-1030): the "slot summary" guess is unconfirmed; the floats suggest a saved position or camera state (inferred).
- Probable original bug: the slot-delete path 0x31ba64 sets the selected slot (*0x3741dc +0x55) to 0xff and then clears options[0x1ec + (s8)0xff × 0x18] = options +0x1d4 instead of the deleted slot's record (0x31baf4..0x31bb18).
- Unknown: the meaning of the five floats; the bytes between +0x3c and +0x1d4; how gifts are granted.

<a id="ts-20261013-0913"></a>

## 2026-10-13T09:13+02:00 — Slot progress layout and completion (REA Ghidra 12.1.2 ev_852ce460…, llvm-objdump; read-only)

Corrected by [save-format.md §2026-10-13T09:25](#ts-20261013-0925): the exclusion loop reads byte +0xc, not +0x24

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
- [Corrects save-format.md §2026-10-13T09:13](#ts-20261013-0913) ("character-table byte +0x24 == 1"): the exclusion loop at 0x1cd5bc..0x1cd5e4 starts at `*0x372e10 + 372` and steps 0x28 for 132 entries. 372 = 9 × 0x28 + 0xc, so it reads byte **+0xc** of ids 9..140, not +0x24.
  - Data: byte +0xc over ids 9..140 holds 1 only for id 9 ("police"); the other 131 values are 2..9 (value 9 occurs 19 times, each other value 16 times). So exactly one entry is excluded; each match subtracts 1 from both the collected and the total counter.
  - Byte +0xc looks like a character class or group (1..9); the meaning is unknown. The u16 at +0x24 is 0 in all 192 rows and is not read by the completion function.
- Offline total (inference from the loops, llvm-objdump 0x1cd3dc..0x1cd5e8, not run): the total counter r6 = 53 (levels) + 42 (two 21-bit sets) + 2 × 132 (characters 9..140) + 2 × 41 (ids 1..41) + 271 (sum of the 13 × 7 cells of TablesSuperCounters, summed from `chase-tabledump`) − 1 (id 9) = **711**. The result is `min(1000, collected × 1000 / 711)` if the divide is a plain unsigned division by the total (the 0x25c1b8 helper was not read).
- Unknown: the meaning of the level-row bytes +0x04, +0x38, +0x39, +0x3c and +0x41..+0x50, and of character-row byte +0xc; whether 0x25c1b8 is an unsigned divide; whether runtime code ever writes to either table (the static files are used as-is, no writer was searched for); the cheat-flag bits and the collected side of the 41-id and 21-bit sets are still as in the 09:13 entry.

<a id="ts-20261013-092607"></a>

## 2026-10-13T09:26:07+02:00 — Completion divide (llvm-objdump; read-only)

- 0x1cd5e8..0x1cd604: r0 = 1000 × collected (`mul r0, r5, r0`, 0x1cd5f0), r1 = total (r6), `bl 0x25c1b8`, clamp to 1000 (`movhi`), `uxth`. 0x25c1b8 is the compiler's unsigned shift-and-subtract divide (`rsbs r3, r1, r0, lsr #n` ladder). This settles the 09:25 caveat: completion = min(1000, (collected × 1000) / total) as u16, unsigned, truncating. A 32-bit overflow of collected × 1000 needs collected > 4 294 967, so it cannot happen.
- Unknown: nothing new.

<a id="ts-20261028-225125"></a>

## 2026-10-28T22:51:25+01:00 — Live volumes, option nibbles and who writes the options (llvm-objdump; servant 20261028-uiport-B-1; read-only)

By 20261028-uiport-B. Code read with llvm-objdump on the analysis ELF; values read by the emulator servant (20261028-uiport-servant, requests UI-4/SAVE-4 and 20261028-uiport-B-1, 2026-10-28, two loads that agree, recorded from a save state: paused on the Options page after mission 1-3, slot 1 with sound effects 5, music 5 and Invert camera on).

- **Live volume bytes.** 0x37751d is the sound effects volume and 0x37751e the music volume (0..10). Their gains are set by 0x1f4408 (a branch to 0x1dca5c: the float at 0x376370, 24 voices marked dirty) and 0x1f36ec (clamped to 0..1, the float at 0x376b3c), each with the byte / 10.
- **The volume source of a loaded save (SAVE-4).** The enter 0x303a40 of the loading-screen mode (vtable 0x3639d8, mode object 0x519c44; it loads `Fonts/frontendfont`, the loading ring `Prop_Special_LoadingRing` and `UI_LS_Generic1`) copies options +4 + slot to 0x37751d and +6 + slot to 0x37751e when the signed byte options +2 is 0 or 1, then sets both gains from the live bytes. So every level load takes the volumes of the last-used slot. The boot load callback 0x2c195c and the front-end enter 0x2d5430 write the defaults 10 and 8 into the live bytes (0x2d5430 then sets gain 1.0 and music 0.8; 0x2c195c calls 0x1f4408 with 1.0 and then with 0.8).
- **Oracle.** Live options `8a 01 01 23 0a 05 08 05` (rest zero), 0x37751d = 0x37751e = 5, gains 0.5 / 0.5: the loading screen had taken the slot-1 bytes +5 and +7.
- **Options +0 and +3 bits 2..5.** Only the two reset paths write them, 0x2081cc and the options callback 0x1c33c0 of the save descriptor (high nibble of +0 from 0x379d64, low nibble from 0x379d66, +3 bits 2..5 from 0x379d65). The only literal references of 0x379d64 are those two functions; no code writes the three globals, so they keep their initial values 8, 8 and 10. A scan of every function that loads 0x51b490 (33 literal loads) for byte reads at +0 and bit tests of +3 bits 2..5 finds none. They are defaults that nothing reads; the oracle's +0 = 0x8a and +3 bit 5 set (bits 2..5 = 8) agree.
- **Options +2.** Only the save task writes it (the slot index is stored at 0x2b63e4 and 0x2c8ea4; 0x22801c is only the getter of the options block); the slot screens do not. The Options page and the loading screen read it.
- **The movie volume and subtitles.** PlayCutscene 0x1bf9dc passes options +3 bit 0 to 0x20f32c and 0x37751d × 0.1 to 0x20f31c; the boot page 0x3206b4 does the same. So movies play at the sound effects volume and +3 bit 0 is the subtitles switch.
- The writes of the pause Options page (0x31af84, 0x31ada4, 0x31ae48) are in [ui-screens.md §2026-10-28T22:51:42](ui-screens.md#ts-20261028-225142).
- Port: `chase::runtime::loadingScreenVolumes` and `game_save::loadingScreen` at each level load, the live options block and bytes in `game_save::Progress`, the save request copying the live options with +2 = slot (`SaveRequestWriter::request`), the follow camera reading +3 bit 1.
- Unknown: what the second options screen of the front end (0x2fb53c: rows 4 and 6 toggle options +3 bit 0 and byte +0x20b bit 5) belongs to and what +0x20b bit 5 does; why 0x2c195c sets the sound effects gain to the music value.
