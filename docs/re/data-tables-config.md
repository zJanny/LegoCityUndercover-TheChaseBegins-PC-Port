# Data tables and config text

Reverse-engineering findings, moved verbatim from the progress log. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis.

<a id="ts-20261013-0350"></a>

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

<a id="ts-20261013-1015"></a>

## 2026-10-13T10:15+02:00 — Brace-block config text parser (.cam, trigger and option lists)

- New parseConfigText / ConfigBlock and a typed parseCameraFollow (src/assets/config_text.cpp, in chase_assets).
- Grammar: `Name {` blocks with `Key(value)` entries and nested blocks, CRLF and `//` comment lines. A value runs to the matching `)` on its line, with balanced inner parentheses. The parser is strict.
- Census: the 10 .cam files hold 10 CameraFollow, 1822 Bound and 253 Room blocks: 2075 camera setups, each with the same 15 keys (Name, Type, RotationY, RotationX, Distance, Location, Lookat, Offset, FOV, Filters, BlendTime, ShakeAmp, ShakeRate, ShakeRoll, 3DOffset), 31125 entries.
- options3ds.txt, triggers.txt and letriggers.txt also parse (37 blocks, 57 entries).
- The strict grammar rejects options.txt (header only), levels/scriptfns.txt line 843 (missing `)`) and levels/gescriptfns.txt line 264 (extra `)`). Whether the game tolerates these is unknown; the test pins each reason.
- Unknown: the meaning of Type, Filters and the camera values, and script function and trigger semantics. The game's loader for these files was not traced.
- Tests: synthetic configText in assets-test; real-data totals in assets-real-test (provenance: line-based Python census, manual inspection). ctest PASS 8/8.

<a id="ts-20261013-0910"></a>

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


<a id="ts-20261013-0928"></a>

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

<a id="ts-20261013-0932"></a>

## 2026-10-13T09:32+02:00 — .cam record selection, blend weights, Type bit 0x400 and 0x100 (REA Ghidra 12.1.2; llvm-objdump)

- Evidence IDs: record picker 0x1d34ac ev_a69314f3…; volume filter 0x20bf6c ev_4b20c0b7…; overlap dedupe 0x1aa734 ev_8bf13740…; 0x23f50c ev_a62a1112…; pose blend 0x298cd8 ev_02030dc2…; blend set-up 0x19b8c8 ev_578623fa…; XZ distance 0x22b62c ev_2829763a….
- **Selection (0x1d34ac(point, out_records, out_weights, max))**:
  1. 0x20bf6c queries the spatial structure at 0x598300 around the point (mask 0x200, max count = param 4, DAT_003770d4), then keeps each record whose bound object (record+0x20) is null or enabled (object+0x0a != 0). The point is moved into the object's local space (0x21bb78, 0x2536c0). Shape byte object+0x09: 2 = sphere (distance to +0x10 <= radius +0x1c); 4 or 6 = test 0x204528 (+0x1c, +0x20, point, +0x10); anything else = box test 0x2397a0 (centre +0x10, extents +0x1c). So selection is purely spatial: the point against the Room/Bound volumes. No priority field is read here.
  2. If none contain the point, the single fallback record is `*(DAT_003770d0+0x20) + *(DAT_00376c78+0x10)` (the level's default room record; the exact meaning of the two globals is not decoded).
  3. 0x1aa734 drops a later record when its volume overlaps an earlier one by <= 0.5 on the axis of least overlap (axis-aligned boxes: centre object+0x10.. plus the parent node offset at node+0x30.., half extents +0x1c/+0x20/+0x24).
  4. First record in list order with Focus (+0x4a) or Blocker (+0x4b) non-zero wins alone (count 1, weight 1.0). Otherwise count < 2 gives weight 1.0.
  5. For 2 or more: weight[i] = 0x22b62c(point2, point) = XZ distance (sqrt of dx^2 + dz^2) between the query point and a point derived from the volume by 0x2554c0/0x257f14/0x257ea0/0x248850/0x23d39c/0x21c1e4 (a point built from the volume centre +0x10 and radius +0x0c x 2; not decoded further). Records with a null object get weight 1.0. If the sum is <= 0, only the first record with weight 1.0. Else weights are divided by the sum. **Note:** the weight is the distance itself, not an inverse; whether 0x21c1e4's output makes this behave as "closer = stronger" is open.
- **Blend (0x19b8c8)**: the record list is copied to blend block +0x10, the weights to +0x30 (count at +0xc). 0x298cd8 reads the weights from there, so they are fixed when the set changes, not recomputed per frame (observed: no other writer was looked for). BlendTime: when param_6 == 0 the fade duration is the plain (unweighted) mean of the records' BlendTime (+0x30), mode field 5; with param_6 != 0 no fade is set up. The easing curve in 0x224094/0x224080 is not decoded. With count 1 and Room (+0x4e) set and DAT_003770c8 == 0, the path 0x1b4864 is used instead.
- **Pose output layout of 0x298cd8** (correction to the 09:28 wording): the weighted sums go to out+0x00..0x08 = look point L, out+0x24..0x2c = camera position P, out+0x30 = FOV (+0x49 as unsigned byte), out+0x38 = 3DOffset (+0x40 float). Weighted look distance sum(|L-P| x w) feeds the renormalisation 0x23b92c when more than one record is active. Slot 0 with Blocker (+0x4b) set (or null) skips all of it and uses the fixed pose at 0x58d42c..0x58d464.
- **Type bit 0x400**: 0x23f50c(out, a, b) is a plain vec3 add (out = a + b). In 0x298cd8 the call is 0x23f50c(L, P, (float)(+0x46,+0x47,+0x48 as signed bytes)) at 0x299008, guarded by `tst r1, #1024` at 0x298fbc on the u16 at +0x44. So with 0x400 the look point is overwritten as **L = P + Offset**: Offset becomes a direction/vector from the camera and the 0x08/0x10/0x20 look rules for that record are ignored. Offset is the unscaled signed bytes.
- **Type bit 0x100**: no reader found. A scan of the whole disassembly (0x100000..0x560000) found 64 `ldrh ..., [rX, #68]` sites; within 8 instructions of each, the only mask test for 0x100/0x400 is the single `tst #1024` at 0x298fbc. The other tests are #1, #2, #4, #8, #16, #32, #128, #512 and compares with 0 (for unrelated structures or the 0x80 loaded flag). Bit 0x100 is stored (Type 256 appears 3 times, 1280 never) but never tested by an +0x44 u16 read. Limit: a read through a different base/offset form, a word load, or a computed pointer would not match the pattern.
- Unknown: what 0x21c1e4 and its helpers compute (so the real behaviour of the distance weight); the default-room globals DAT_003770d0/DAT_00376c78; easing in 0x224094/0x224080; path 0x1b4864; whether anything rewrites the weights after set-up; meaning of Type 256; the 0x1aa734 overlap rule's purpose.

<a id="ts-20261013-144113"></a>

## 2026-10-13T14:41:13+02:00 — Corrections from the camera work (see camera.md)

- **Correction of the 2026-10-13T09:32 entry:** Type bit 0x100 does have a reader: 0x2821f8, called from the pose step 0x181fe0, tests `[record+0x44] & 0x100` and `& 0x200` (the record pointer is passed as an argument, which the earlier `ldrh` scan could not match). Without these bits its shift decays to zero.
- **Correction of the 2026-10-13T09:28 entry:** the record pose blend 0x298cd8 is not the only per-frame camera. With exactly one active Room record and DAT_003770c8 == 0 (0x19b8c8), the follow controller 0x2a8bfc is used instead; that is the street at gameplay-start. Details and unknowns in [camera.md](camera.md).
