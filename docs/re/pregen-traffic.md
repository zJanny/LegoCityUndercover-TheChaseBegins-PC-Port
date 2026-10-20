# Pregen traffic and pedestrian spawn files

Format and loaders of `Pregen/Traffic/<level>_Traffic.bin` and `Pregen/Pedestrians/<level>_Pedestrians.bin`: spawn points on level paths and a quadtree of spawn point lists. The managers that use them (spawn rules, limits) are in their own entries; this file covers the files and what the loaders build from them. Feature: [pedestrians-traffic.md](../plans/features/pedestrians-traffic.md).

## Current state

- The traffic manager 0x527410 loads the Traffic file with 0x2e02d4, and the pedestrian manager 0x51b33c loads the Pedestrians file with 0x2ee868. Both run from slot +0x40 of their manager on every scene enter, not from the init. The path strings start at 0x2e0560 and 0x2eeaa0. [§2026-10-20T11:16:20](#ts-20261020-111620), [pedestrians.md §2026-10-20T12:27:05](pedestrians.md#ts-20261020-122705)
- Both files share one layout: a u32 point count, 24-byte points (position, a stale pointer, a level path name hash, a path parameter), then a quadtree of 20-byte cells (half size, centre x and z) whose leaves list point indices. [§2026-10-20T11:16:20](#ts-20261020-111620)
- The loaders resolve each point's path in the loaded levels, store the unit tangent of the path at the parameter, the room and (traffic only) the floor cell; the lookup 0x1eb490 descends to the leaf of a point. [§2026-10-20T11:16:20](#ts-20261020-111620)
- All 14 shipped files (7 levels each) decode exactly; every point's path exists in its level, and each position equals the path sample at the parameter (inference: the spawn points are the editor's samples along the paths). [§2026-10-20T11:16:20](#ts-20261020-111620)
- Port: `src/assets/pregen_traffic.*` (`parsePregenSpawns`, `findPregenSpawnCell`), tests `pregen-traffic`, `pregen-traffic-real-data`. Not wired into the game.
- The derived fields are ported as `deriveSpawnFields` (`src/runtime/pregen_derive.*`) for both kinds: path record, tangent, room. The 72 closed-path pedestrian points off the path sample deviate on one axis of the stored position; the sampler matches all other points exactly. [§2026-10-20T12:11:03](#ts-20261020-121103)
- No path gets a node (path +0x18) at run time: only the natives EffectAttachParticlesToPath and EffectLinkPathToObject set it, and no script calls them. So the samplers never transform the position or the tangent. [pedestrians.md §2026-10-20T13:09:29](pedestrians.md#ts-20261020-130929)
- Open: how the managers choose a point from a cell list, the radius the cell lists were built with, the traffic floor cell (+40), the loaded-level order and current room at manager init.

<a id="ts-20261020-111620"></a>

## 2026-10-20T11:16:20+02:00 — Traffic and pedestrian spawn files: loaders, layout, quadtree lookup, census (llvm-objdump only, no REA session, no emulator)
Corrected by [pregen-traffic.md §2026-10-20T12:11:03](#ts-20261020-121103): the 47 points off the closed-path sample are 72 (threshold 1e-3) and differ on one axis of the stored position, not in the sampler.
Corrected by [pedestrians.md §2026-10-20T12:27:05](pedestrians.md#ts-20261020-122705): both loaders run from slot +0x40 of their manager (scene enter: 0x2edbf8 -> 0x2edc94, 0x2dedbc -> 0x2df574), not from the init.

Method: `llvm-objdump -d --triple=armv6k-none-eabi --mattr=+vfp2` on `artifacts/game/game.arm.elf` (Europe v1.0); a throwaway Python scan for `bl` targets and literal words; throwaway Python and C++ walks of the shipped files under `artifacts/fib/files/pregen/` and of the level paths (`Level::paths`, sampled with `runtime::sampleDirectorPath`), session 2026-10-20, not kept. [Corrects data-tables-config.md §2026-10-15T12:09:48](data-tables-config.md#ts-20261015-120948): the two path strings start at 0x2e0560 and 0x2eeaa0 (0x2e0567 and 0x2eeaa7 are 7 bytes into them, after `Pregen/`).

**Who loads which file.** The traffic file loader 0x2e02d4 is called once, at 0x2df884, from the init 0x2dfa48 of the manager 0x527410 (its strings `Traffic%02u`, `Traffic_MaxVehicles`, `Traffic_FadeBound%02u`); the result (1 loaded, 0 no file) goes into bit 0 of byte +0x390. The pedestrian file loader 0x2ee868 is called once, at 0x2eded4, from the init 0x2ee4b4 of the manager 0x51b33c (strings `Pedestrian%02u`, `Pedestrians_Max`); the result goes into bit 0 of byte +0xa4. So 0x527410 is the traffic manager and 0x51b33c the pedestrian manager, the other way round from [script-natives-audio.md §2026-10-15T15:10:38](script-natives-audio.md#ts-20261015-151038).

**Loader (both the same, field for field).** `sprintf(path, "Pregen/Traffic/%s_Traffic.bin" (0x2e0560) or "Pregen/Pedestrians/%s_Pedestrians.bin" (0x2eeaa0), name)` with name = word +0x30 of row `(s8)[0x5163e8 + 38]` (84-byte rows) of the level table `*0x375914`; open 0x24eeec; on failure return 0. Reads (0x2538c8, little-endian as stored): u32 count into manager +0x3c; allocates count × 44 bytes (traffic) or × 40 (pedestrians) at +0x38; per point reads 12 bytes into +0, then 4 into +24, 4 into +28, 4 into +32. Then per point: for each level of the loaded-level list 0x589a18 (count +8, array +0xc), 0x23c188 gives its data and 0x1b3d04 searches its path section (+0x24 → count u16 +0xa, 0x28-byte records at +0x20, u32 +0 = name hash) for word +28; the result is stored at +24 (so the file's +24 value is always replaced, and is 0 when no level has the path); for the first level that has it, the path sampler of its type (5-word table 0x358d58 for traffic, 0x358a14 for pedestrians, same entries as 0x358f9c: type 0 → 0x1c17c0) is called with (path +8, scratch, out = point +12, 1, s0 = float +32). The sampler's third argument receives the unit tangent (normalised by 0x257ea0); the position output goes to the scratch. Then +36 = 0x239838(point) (room lookup, see [particles.md](particles.md)) and, traffic only, +40 = 0x1edaac(x, y, z, 0.0) (floor cell search, [vehicles-physics-collision.md](vehicles-physics-collision.md)). Last, the cell reader (0x1edbe4 traffic, 0x1eb254 pedestrians) fills the root cell at manager +0x24, and the file is closed.

**Point (24 bytes in the file).** +0 position x, y, z (floats; kept as read); +12 u32 not used (overwritten; values like 0x037f1614 are pointers of the tool that wrote the file); +16 u32 level path name hash; +20 float path parameter (0 .. point count - 1 on an open type 0 path).

**Cell (20 bytes; reader 0x1edbe4 / 0x1eb254, recursive).** +0 u16 half size, +2 u16 list length n, +4 float centre x, +8 float centre z, +12 u32, +16 u32 (both pointers of the tool; the reader clears both, then reads them). If n > 0: n u16 point indices follow, read into a stack buffer of 400 bytes (traffic) or 512 bytes (pedestrians), so at most 200 or 256 fit; the reader does not check n or the indices; it replaces +12 with an array of n point pointers. If n = 0 and +16 = 0: an empty cell. If n = 0 and +16 ≠ 0: it allocates 4 child cells (80 bytes, at +16), reads 4 cell headers into them, then reads each child in full, header again included; the second read overwrites the first. In every shipped file both header copies are equal.

**Lookup 0x1eb490(manager, cell, point).** While the cell has n = 0: no children → returns 0; otherwise, with child 0 (c0) and child 1 (c1): if not (c0.centre x + c0.half < point x), goes to c0 when not (c0.centre z + c0.half < point z), else to child 2; otherwise to c1 when not (c1.centre z + c1.half < point z), else to child 3 (the half size converted u16 → float; `bhs` after `vcmpe`, so an unordered compare takes the low side). Returns the first cell with n > 0. The root's own square is not tested, so points outside go to the nearest corner. Its only caller is 0x2ee114 in the pedestrian spawn 0x2ee068; the traffic side was not searched.

**Census of the shipped files (all parse to the last byte).**

| File | Size | Points | Cells | Lists | Empty | Entries | Longest | SHA-256 |
|---|---|---|---|---|---|---|---|---|
| lvl_01_cherrytreehills_traffic | 27612 | 649 | 85 | 53 | 11 | 4326 | 168 | ff910ac5ad0af755dbb410aa3b922332a6acc6ffda5882b26b5aa556f5bd1f86 |
| lvl_03_auburn_traffic | 20078 | 469 | 85 | 61 | 3 | 2719 | 87 | f3aee76b34e763b249276fa38b49e884f1b95cce00485ef32f3163f4ee4e7cb7 |
| lvl_04_fortmeadows_traffic | 22340 | 231 | 341 | 88 | 168 | 1586 | 42 | 04646e9deb78259f8783e7a11aa5799b2dac9c345ed7a0fa6d216ae95afc24d0 |
| lvl_05_bluebellwood_traffic | 10126 | 187 | 85 | 38 | 26 | 1127 | 59 | 434770422da69810d8c9d90aa6cf7779f9015257a2f0160bc33179cc7b650fb1 |
| lvl_06_apolloisland_traffic | 8946 | 158 | 85 | 41 | 23 | 885 | 41 | e7cf7cfed759e738f8caee112a375fae29d984e6eb87d542ae2fc934bc388fe0 |
| lvl_07_downtown_traffic | 40370 | 727 | 341 | 141 | 115 | 4649 | 70 | 46d6f055683a3bfaaff91963a606b162cf317975190fd77fab581a4315a98fd3 |
| lvl_test_playground_traffic_traffic | 5114 | 123 | 21 | 16 | 0 | 669 | 71 | 69001c77a62b185b403b1a5e6feec5cf7f1380b5f33114cda7a3f370042a4886 |
| lvl_01_cherrytreehills_pedestrians | 44490 | 1115 | 85 | 64 | 0 | 7173 | 237 | a7a1d3ce1ce4ea0466ff72a2c983db0e069b0498b6c1a2a522aa2570a6c2ba1e |
| lvl_03_auburn_pedestrians | 25942 | 615 | 85 | 64 | 0 | 3899 | 117 | 4f65a3151f8afa99e5f85fd1c0a8d5b13cca55261eb4458b9f53ad72ceeff1ed |
| lvl_04_fortmeadows_pedestrians | 1164 | 32 | 5 | 4 | 0 | 106 | 30 | 870e838973c7ea313d24f695961c31eff608678e5db7414106aae95579923cc9 |
| lvl_05_bluebellwood_pedestrians | 16624 | 354 | 85 | 53 | 11 | 2372 | 109 | 37fed8dc348b6d99be023f51fed2b8f94009c73a9b1cf3c6ce8bf901daef062e |
| lvl_06_apolloisland_pedestrians | 7478 | 198 | 21 | 16 | 0 | 951 | 96 | a0dc289caede66095e166f00761d7a74a2e955176e49f55ac943eb228c62d637 |
| lvl_07_downtown_pedestrians | 84938 | 1906 | 341 | 178 | 78 | 12785 | 169 | bf63024090ff2c3508dab2a97d3278e718a5fd015be8374cf516af693fbb870f |
| lvl_test_playground_traffic_pedestrians | 3892 | 90 | 21 | 16 | 0 | 454 | 43 | 62f0daf95cd9cd76217bf8ff4658524b18017f4b13e3757b9c3e685655e3227b |

lvl_00_policestation and lvl_02_albatrossisland have neither file, so both loaders return 0 there. The trees are complete to their depth (5, 21, 85 or 341 cells). The longest lists stay below the reader's capacity (237 of 256, 168 of 200).

**What the data shows (inference, not from code).**
- Every point's path hash names a path of its level (all 6854 points; all paths type 0). With the open paths, each stored position equals the Catmull-Rom sample of the path at the stored parameter (difference 0.000). So a point is a spawn position on a path, and the tangent the loader stores gives the direction along it.
- A cell's list holds the points near the cell, not only those inside its square (about 85 % of the entries lie outside it), and most points are listed in several cells. The lookup for a point's own position finds a list holding that point for every point except 16 of lvl_07_downtown_traffic, which are in no list at all.

Unknowns:
- How the spawn routines (pedestrians 0x2ee068, the traffic equivalent) pick a point from a list, and the distance the lists were built with.
- 47 pedestrian points on closed paths (42 of the test playground, 5 of lvl_07_downtown) lie up to 0.4 units from the port's sample at their parameter; whether the parameters were rounded or the closed sampler differs is not known.
- The sampler's fourth argument (1) and its node transform (0x250810, 0x254250); type 1 paths (0x1b33fc) are not used by these files.
- What the traffic manager does with the floor cell (+40).

<a id="ts-20261020-121103"></a>

## 2026-10-20T12:11:03+02:00 — Pregen derived fields: path record, tangent, room; the closed-path deviation (llvm-objdump of `artifacts/game/game.arm.elf`, no REA session, no emulator)

[Corrects pregen-traffic.md §2026-10-20T11:16:20](#ts-20261020-111620): the unknown about 47 closed-path points; the loaders' derived fields are now ported.

**Read from the original.** Loader 0x2ee868 (the traffic loader 0x2e02d4 is field for field the same), points of 40 bytes (traffic 44): for point k and each loaded level in list order (list 0x589a18, `0x23c188` gives the level data), `0x1b3d04(path section, hash, 0)` returns the path record; the result is stored at point +24 (0 when no level has it). For the first level that has it, the sampler `table[type byte of record +8]` (0x358a14 pedestrians, 0x358d58 traffic; entry 0 = 0x1c17c0) is called with (record + 8, scratch on the stack, point +12, 1, parameter at point +32). Then, for every point whether or not a path was found, `point +36 = 0x239838(point)`.
- **0x1c17c0 (type 0 sampler).** Open path: the parameter is clamped to 0 .. count - 1; closed path: no clamp. `floor` by 0x252c14, indices base - 1, base, base + 1, base + 2, wrapped by the signed remainder 0x24e500 with count added when negative (closed) or clamped to 0 .. count - 1 (open). The position block (r5 not null) is the Catmull-Rom as `sampleDirectorPath` has it. The tangent block (r6 not null) is the derivative p1 (6t^2 - 6t) + p2 (-6t^2 + 6t) + (p2 - p0)(3t^2 - 4t + 1) / 2 + (p3 - p1)(3t^2 - 2t) / 2 per axis (constants at 0x1c1ae4..0x1c1b04: 0, 2, 3, 1, 0.5, -2, 6, -6, 4), normalised by 0x257ea0; this is `detectivePathTangent`, whose formula I compared with the disassembly term by term. The tail (0x1c1a98): when the fourth argument and the position pointer are set and `[record + 8 + 0x10]` (record +0x18, stored by 0x1c1b48) is not 0, the position is transformed by the node matrix 0x250810 / 0x254250 and the tangent by 0x249ee8; with the node unset nothing more happens. The callers of the setter 0x1c1b48 are 0x2a83b4 and 0x2bac44 (not read), so the node is taken as unset (PREGEN-S3).
- **Room 0x239838(point).** For every scene of the list 0x589a18 in order it calls `0x22d204(scene, point, first)` (first = 1 for the first scene) and returns the first non-zero result, else 0. The rule of 0x22d204 is the one of [streaming-rooms.md §2026-10-14T20:51:26](streaming-rooms.md#ts-20261014-205126) (position lifted by 0.1; the current room `[0x376c78]` and its list first for the first scene; rooms in array order; smallest vertical gap of the rooms whose x/z range holds the point), applied to the stored position without any box offset. The result stored at +36 is the room object; the port keeps the room number.

**Port.** `deriveSpawnFields(spawns, loadedLevels, context)` returns per point: path found, level and path index, tangent, room (`objectRoomAfterMove` with no own room). Not computed: the traffic floor cell 0x1edaac (+40). The caller supplies the loaded levels in order and the current room (0 = none): stand-in PREGEN-S2.

**Check (shipped data, test `pregen-derive-real-data`).** With the level's `_main.blvl` as the only loaded level, all 6854 points of the 14 files find their path (all type 0; pedestrian paths are all closed, traffic paths all open) and a room (room boxes of `models/<level>`; distinct rooms per file pinned in the test). The tangent of every point is a unit vector and agrees (dot > 0.99) with a central difference of the sampler.

**PREGEN-2.** 72 pedestrian points (6 of lvl_07_downtown, 66 of the test playground; threshold 1e-3, so the earlier 47 used a coarser one) lie off `sampleDirectorPath` at their stored parameter. Every one differs on a single axis only (the other two match to 1e-3), by up to 2.3 units (largest in the playground); the parameters have one decimal. The other 3,900+ closed-path points match exactly, including parameters near the wrap, so the closed branch of 0x1c17c0 is not the cause and rounding of the parameter would move several axes. Inference, not from code: the tool that wrote the files changed one coordinate of these positions (for example snapped a height to the ground). The loader overwrites nothing of the stored position (the sampler's position goes to the scratch), so the stored value is the spawn position as read.

Unknowns:
- The order of the loaded-level list and the current room when 0x2ee4b4 / 0x2dfa48 run (PREGEN-4); only one level per file was needed here.
- Whether any pedestrian or traffic path has its node set (PREGEN-5).
- The traffic floor cell (PREGEN-3).
- Which axis the 72 deviations are on (not tabulated) and why.

