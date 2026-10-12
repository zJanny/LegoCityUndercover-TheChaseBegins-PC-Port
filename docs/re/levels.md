# Levels, placement and navigation grids

Reverse-engineering findings, moved verbatim from the progress log. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis.

## 2026-10-12T17:20+02:00 — S4c: .blvl level format decoded (REA, Ghidra)

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

## 2026-10-12T17:45+02:00 — S4c: native level placement and the mesh-centre rule

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

## 2026-10-12T23:05+02:00 — Placement Euler order recovered (replaces the Ry·Rx·Rz assumption)

- "initialorientation" (string at 0x1a49c4, referenced at 0x1a4860 in object setup 0x1a4130, ev_eb095374…8f7 / ev_8c852395…6e0): the parameter is looked up by hash (0x24da28 + 0x217080). Its three floats are multiplied by 0.0174533 (literal at 0x1a49d8) and passed to 0x237c48.
- 0x237c48 (disassembly), using sincos 0x249038 (r0 = sin, r1 = cos), writes the rotation rows [cy·cz, sx·sy·cz + cx·sz, sx·sz − cx·sy·cz], [−cy·sz, cx·cz − sx·sy·sz, sx·cz + cx·sy·sz], [sy, −sx·cy, cx·cy]. Rows 3 and column 3 are left untouched.
  - A numeric search over all 48 axis-order and sign combinations matches only Rz·Ry·Rx for row vectors (x applied first), i.e. Rx·Ry·Rz in column-vector terms.
  - The viewer's Ry·Rx·Rz assumption mirrored the yaw of objects with ±180 on X and Z: the game gives Ry(180 − y), the assumption gave Ry(180 + y).
- Only object setup 0x1a4130 was traced. That other classes using "initialorientation" (12 more string copies) share 0x237c48 is assumed, not checked.
- lvl_00_policestation_main: 120 of 130 placements are yaw-only or zero, so only a few props change in room3.
- chase_assets placementMatrix (level.cpp) and convert_levels.placement_matrix replicate it, with tests for (90,0,0) mapping +y to +z and (180,30,180) equalling a yaw of 150. chase-viewer uses it directly (same memory layout as GL column-major).
- Material combiners: the renderer keeps bit-packed GPU state (e.g. 0x126f0c, a 4 KB state packer). Not traced further.
- ctest PASS 7/7; Python PASS 47/47.

## 2026-10-12T23:25+02:00 — Whole levels in the viewer

- Meshname resolution: 6561 of 6563 placements over all 856 levels resolve to archive models; only xx_goldstud and Prop_Hotspot_Mission do not.
- Level models: models/<level>/ holds lvl_*.bfnmdl (plus _lod1) and room<N>.bfnmdl (403, plus 288 _l1 detail levels). chase-viewer `--level` without model arguments loads that folder minus the _l1/_lod1 models, or just room<N> with `--room`. This is a viewer convention; the game's streaming logic (Pregen/*.bin) is not recovered.
- `--eye X,Y,Z` sets the camera position.
- lvl_01_cherrytreehills_main: 1161 instances, 845580 triangles, 0 missing textures, about 1.1 s to load offscreen. A street view at (800,60,1060) shows the plaza, arch, roads, vehicles and water plane, textured and placed.

## 2026-10-12T23:55+02:00 — Navigation grids (.bfnnav) decoded

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

