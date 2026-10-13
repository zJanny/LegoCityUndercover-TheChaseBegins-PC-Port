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

## 2026-10-13T09:10+02:00 — .cam loader 0x3062e4 (REA Ghidra 12.1.2 ev_9636e324…, llvm-objdump; read-only)

- Partly answers the 10:15 unknowns. 0x3062e4 builds the camera list for a level: one 0x50-byte record per level object whose name starts with "Camera" and does not contain "LookAt".
  - Name contains "Focus" → +0x4a = 1. Name contains "Blocker" → +0x4b = 1.
  - Names starting with "CameraVehicle" are set up with mode 2, the others with mode 1 (0x1e9290).
  - Rooms get their own records (+0x4e = 1).
- It then opens "3dsdata/levels/<level>.cam", falling back to "levels/<level>.cam", and registers key handlers for Room and Bound blocks:
  - Type (0x275584) first resets the defaults: Filters 32/28, FOV 43, BlendTime 0.85, 3DOffset 5.0. It then stores u16 (atoi(Type) | 0x80) at +0x44.
  - Location: three floats at +0x24..+0x2c.
  - Offset: atoi of each component into bytes +0x46..+0x48 (integers only).
  - FOV: byte +0x49.
  - Filters: two bytes at +0x4c and +0x4d, each clamped to 0..50.
  - BlendTime: float at +0x30, clamped to [0, 20].
  - ShakeAmp, ShakeRate, ShakeRoll: floats at +0x34, +0x38, +0x3c. 3DOffset: float at +0x40.
  - Room Name → 0x2a38b8. Bound Name → 0x2ab0b8, which looks up the level object of that name.
- RotationY, RotationX, Distance and Lookat have no handler, so the game ignores them (editor data).
- Data check (Python census over all 10 files, 2075 setups): for Type 0, Location = Distance × (sin ry · cos rx, sin rx, cos ry · cos rx), with RotationX/Y in 1/256 turns, in 1723 of 1724 setups (error < 1e-3). Other types store unrelated (world-like) Locations. So Type 0 Location is an offset baked from the editor angles. No shipped value is changed by the atoi truncation or the clamps.
- CameraSetup (chase_assets) keeps the raw values; its header comment marks the editor-only fields.
- Unknown: the meaning of the Type values (0..58, 256, 1024..1027) and Filters; how +0x44 and the records drive the camera at run time (0x1e9290 and the camera update); why "Offset" is an integer byte triple.


## 2026-10-13T09:28+02:00 — .cam Type is a flag word, and the per-frame camera blend (REA Ghidra 12.1.2; llvm-objdump)

- Evidence IDs: Type handler 0x275584 ev_aea05ee8…; per-frame camera update 0x1b4a30 ev_6375ca51…; record picker 0x1d34ac ev_0eff67cc…; blend set-up 0x19b8c8 ev_578623fa…; pose blend 0x298cd8 ev_02030dc2…; record lookup 0x268678 ev_5057c3df…; shake 0x1c607c ev_14d33f71…; record binder 0x1e9290 ev_161c6427…, 0x1fc5b8 ev_6c8215c4…; FOV getter 0x2ab2e4 ev_470bbc59….
- **Correction to the 09:10 entry:** 0x1e9290 is not the runtime camera. It runs once per record while the level loads. It binds the 0x50-byte record to its level object (record+0x20 = object entry, via 0x1fc5b8) and, for mode 2 or 4 on an object without a parent node, copies the object's +0x1c value to +0x20/+0x24. The per-frame code is 0x1b4a30.
- **Type is not an enum. It is a bit set** stored in the u16 at record+0x44. The loader stores atoi(Type) | 0x80, and bit 0x80 means "record loaded". 0x1c60e8 and 0x2686bc test it and copy the defaults at 0x58d854.. (Type 0, Offset 0,0,2, FOV 43, Filters 32/28, BlendTime 0.85, 3DOffset 5.0) into records that never got one. No jump table or compare chain on Type was found.
- Only reader of the other bits: pose blend 0x298cd8, called through the blend controller built in 0x19b8c8. For each active record it builds a camera position P (record+0x24..0x2c, Location) and a look point L from the Offset bytes (+0x46..0x48, signed), with T = the followed target position (player or the level's focus object). Bits (observed in the code):
  - 0x01 / 0x02 / 0x04: P.x / P.y / P.z. Clear: P = T + Location (x and z scaled by 1.1 while a second target is active, else 1.0). Set: P = Location as a world coordinate.
  - 0x08 / 0x10 / 0x20: L.x / L.y / L.z. Clear: L = T + Offset. Set with the matching position bit clear: L = T and Offset is ignored. Set with the matching position bit set: L = P + Offset.
  - 0x400: after the above, calls 0x23f50c(L, P, Offset-as-vector). What it does is not decoded.
  - Names by behaviour, as far as the code shows: Type 0 = chase camera (offset from the target, aimed at target + Offset); 1/2/4 = that axis fixed in world space; 7 = all three position axes fixed (camera at a world position); 8/16/32 = look at the target on that axis (when the position bit is clear).
- Census of all 10 .cam files (2075 setups, throwaway script, not in the repo): 0 x1724, 1 x44, 2 x129, 3 x11, 4 x43, 5 x37, 6 x27, 7 x24, 9 x2, 11 x2, 13 x1, 15 x2, 16 x2, 17 x1, 18 x2, 32 x2, 38 x1, 47 x8, 58 x2, 256 x3, 1024 x4, 1025 x3, 1027 x1. Every value uses only the bits 0x01..0x20, 0x100 and 0x400, which matches the bit reading above (so "0..58" in the earlier note is a set of bit combinations, not 59 modes).
- Per frame (0x1b4a30):
  - It takes the follow target (or the free-camera point 0x58d8ac) and calls 0x1d34ac, which collects up to 100 Room/Bound records that contain the point. If any collected record has Focus (+0x4a) or Blocker (+0x4b) set, that single record wins. Otherwise each record gets a weight from the distance to its volume (0x22b62c), normalised to sum 1. With no hit, the level's default room record is used.
  - It compares the new record set with last frame's (0x58d880, up to 8). On a change it calls 0x19b8c8. That sets up the blend controller (callback 0x298cd8). The fade time is the mean of the records' BlendTime (+0x30) when blending. The controller is reused when the set of level objects is unchanged.
  - 0x19b8c8 also turns Filters (+0x4c, +0x4d) into 1 - clamp(value x 0.02) strengths for two filter parameters (0 and 1 of the object at 0x226a40), or resets them at 0. A lone Room record (+0x4e) with DAT_003770c8 == 0 takes the separate path 0x1b4864.
  - 0x298cd8 outputs a pose: position, look point, FOV and 3DOffset, each as the weight-weighted sum over the records (FOV from +0x49, 3DOffset from +0x40). With more than one record it also renormalises the direction (0x23b92c). Records with the Blocker flag in slot 0 skip this and use a stored fixed pose (0x58d42c..0x58d464).
  - ShakeAmp, ShakeRate and ShakeRoll (+0x34, +0x38, +0x3c) are used by 0x1c607c, which adds a position offset (+0x34 x +0x38 per axis from helper calls 0x241098/0x217b70) and a roll (+0x3c) to the view matrix. The exact waveform is not decoded.
- Unknown: bit 0x100 (Type 256) was not read by 0x298cd8, 0x19b8c8, 0x1b4a30 or 0x1d34ac, so its meaning is open; what 0x23f50c does for 0x400; the exact easing inside the blend controller (0x224094, 0x298cd8's caller); the single-room path 0x1b4864; whether 0x1d34ac's weights are distance or volume based (only the call chain was read); the other ~60 sites that read a u16 at +0x44 of unrelated structures were not checked.
