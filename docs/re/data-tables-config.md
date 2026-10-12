# Data tables and config text

Reverse-engineering findings, moved verbatim from the progress log. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis.

## 2026-10-13T03:50+02:00 — Data tables (.bxls) decoded (REA, Ghidra 12.1.2)

- Code names: "Tables/" + "TablesCutscenes.xls" etc. (0x1eecb4; loader 0x1eec18, ev_c15cf16e…4192). The "xls" extension handler is registered at 0x103da8 (ev_db44b1bb…232c) with the callback 0x107f08 (ev_b86a6c1d…2641), which swaps in ".bxls" and calls the parser 0x10e1cc (ev_5e7ab97d…a567). Cell row access is 0x20f844 (ev_f0bc83a6…437a).
- Layout (−14 pool):
  - The header has u16 sheet count and u16 flags, then 0x1c-byte sheet records: u16 columns, u16 rows, u16 stride, then pointers to types (+8), rows (+0xc), string pool (+0x10) and stored-column list (+0x14), plus u8 list count (+0x18).
  - Block order per sheet: types, rows, the list (if +0x14 ≠ 0), then the strings (if +0x10 ≠ 0).
- Column types, from the 0x10e1cc walk:
  - 0: not stored. 1: 4 bytes (int). 2: 2 bytes. 3: 1 byte. 4 and 7: 4 bytes (4 holds floats such as 1.75 and 5.0). 5 and 6: 4-byte string-pool offsets, which the game relocates.
  - 8, 9 and 10: 1, 2 or 4 bytes, but only when the column index appears in the stored list.
  - Each field is aligned to its size, and the row is padded to the largest alignment among types 1, 2 and 4–7.
- Validation: all 24 tables (54 sheets, 23512 stored cells) parse. Every computed stride equals the stored stride, and every block is consumed.
- Content examples:
  - tablescutscenes: per-level FMV names (01_gameintro, 02_outro_residential, …). These are videos, not the csdata in-engine cutscenes.
  - tableslevels: level names and stud thresholds 10000/50000/… with ui_studx2/4/… icons.
  - tablesanimations: animation names per class.
  - tablescharacters: 192 characters.
- Hypothesis: the listed type-8 columns sit at indices 7, 15, 23, … and hold mask-like bytes (128, 206, 192), so a run of 8 bool columns probably shares one byte. The bit order and column meanings are unknown. Column names are not in the files.
- Added src/assets/table.cpp (parseTable, tableValue), src/tabledump.cpp (chase-tabledump), a synthetic assets-test case and real-data totals.
- ctest PASS 8/8.

## 2026-10-13T10:15+02:00 — Brace-block config text parser (.cam, trigger and option lists)

- New parseConfigText / ConfigBlock and a typed parseCameraFollow (src/assets/config_text.cpp, in chase_assets).
- Grammar: `Name {` blocks with `Key(value)` entries and nested blocks, CRLF and `//` comment lines. A value runs to the matching `)` on its line, with balanced inner parentheses. The parser is strict.
- Census: the 10 .cam files hold 10 CameraFollow, 1822 Bound and 253 Room blocks: 2075 camera setups, each with the same 15 keys (Name, Type, RotationY, RotationX, Distance, Location, Lookat, Offset, FOV, Filters, BlendTime, ShakeAmp, ShakeRate, ShakeRoll, 3DOffset), 31125 entries.
- options3ds.txt, triggers.txt and letriggers.txt also parse (37 blocks, 57 entries).
- The strict grammar rejects options.txt (header only), levels/scriptfns.txt line 843 (missing `)`) and levels/gescriptfns.txt line 264 (extra `)`). Whether the game tolerates these is unknown; the test pins each reason.
- Unknown: the meaning of Type, Filters and the camera values, and script function and trigger semantics. The game's loader for these files was not traced.
- Tests: synthetic configText in assets-test; real-data totals in assets-real-test (provenance: line-based Python census, manual inspection). ctest PASS 8/8.

