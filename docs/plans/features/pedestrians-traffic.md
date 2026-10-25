# Pedestrians and traffic

Subtopics: [pedestrians-traffic-pedestrians.md](pedestrians-traffic-pedestrians.md), [pedestrians-traffic-cars.md](pedestrians-traffic-cars.md)

## 1. Scope

The spawn data of the pedestrian manager 0x51b33c and the traffic manager 0x527410: the `Pregen/Traffic` and `Pregen/Pedestrians` files (spawn points on level paths, a quadtree of spawn lists). The pedestrian manager's rules are in [pedestrians-traffic-pedestrians.md](pedestrians-traffic-pedestrians.md), the traffic manager's in [pedestrians-traffic-cars.md](pedestrians-traffic-cars.md); the native flag is SCRIPT-230 in [script-system-missions.md](script-system-missions.md); traffic driving is VEH-12 in [vehicles.md](vehicles.md); related: VEH-91 (vehicle activation), VEH-106 (traffic smashing props), LEVOBJ-16 (pedestrian walk state).

## 2. Status

IN_PROGRESS, updated 2026-10-25T12:43:03+01:00. Both spawn files decode in `chase_assets` (all 14 shipped files, census pinned) together with the cell lookup 0x1eb490. The pedestrian file is used by `PedestrianSystem` and the traffic file by `TrafficSystem` in `chase-game` ([pedestrians-traffic-pedestrians.md](pedestrians-traffic-pedestrians.md), [pedestrians-traffic-cars.md](pedestrians-traffic-cars.md)). `deriveSpawnFields` computes the fields the loaders derive (path record, tangent, room) for both kinds; `deriveTrafficFloorCells` computes the traffic floor cell.

## 3. Verified behaviour

- File layout from the loaders 0x2e02d4 (traffic) and 0x2ee868 (pedestrians) and the cell readers 0x1edbe4 / 0x1eb254; every shipped file parses to its last byte with the recorded counts and checksums (`pregen-traffic-real-data`).
- The lookup 0x1eb490 descends the quadtree as the original (low edge inclusive, unordered compares take the low side); every point is found in the list of its own cell except 16 points of lvl_07_downtown traffic that are in no list (`pregen-traffic-real-data`).
- Per point the path record (first loaded level that holds the hash), the unit tangent at the parameter and the room are derived as the loaders do; on all 14 shipped files every point finds its path in the level's `_main.blvl` and a room (`pregen-derive-real-data`).
- Every point's path hash names a type 0 path of its level, and the position equals the path sample at the stored parameter on open paths (data check, not a test).
- The traffic floor cell (point +40) is the `collision_floor` cell that contains the point, found by the bisection and outward scan of 0x1edaac: 2418 of the 2421 points of the six levels with a master model find a cell (`pregen-derive-real-data`, the port's own run).

## 4. Stand-ins

- PREGEN-S2 The loaders walk the loaded scenes of the list 0x589a18 (pool order, loaded scenes only); a `chase-game` session holds one level, so its list has only that level. evidence: [pregen-traffic.md §2026-10-20T13:13:47](../../re/pregen-traffic.md#ts-20261020-131347)
- PREGEN-S1 The decoder rejects input the original reads without checks: list indices beyond the points, lists longer than the reader's stack buffer (200 traffic, 256 pedestrians), trees deeper than 16 levels and bytes after the tree. No shipped file has any of them. evidence: [pregen-traffic.md §2026-10-20T11:16:20](../../re/pregen-traffic.md#ts-20261020-111620)

- PREGEN-S4 `findTrafficFloorCell` throws where the original's lookup never ends (one cell, or two cells with the first closer to the point); no shipped level has so few cells. evidence: [pregen-traffic.md §2026-10-25T12:36:20](../../re/pregen-traffic.md#ts-20261025-123620)

## 5. Open items

- [ ] PREGEN-6 — 3 traffic points (1 in lvl_01, 2 in lvl_04) lie in no cell of the floor graph; what the original stores and how the spawn then uses it. needs: PREGEN-7. evidence: [pregen-traffic.md §2026-10-25T12:36:20](../../re/pregen-traffic.md#ts-20261025-123620)
- [ ] PREGEN-7 — What reads the floor cell in the vehicle body (+0x8c, written by the traffic spawn). blocks: PREGEN-6, TRAFFIC-11. evidence: [pregen-traffic.md §2026-10-25T12:36:20](../../re/pregen-traffic.md#ts-20261025-123620)

## 6. Findings summary

- Spawn files, loaders, quadtree, census; which manager loads which file: [pregen-traffic.md §2026-10-20T11:16:20](../../re/pregen-traffic.md#ts-20261020-111620).
- The derived fields and the one-axis deviation of 72 stored positions: [pregen-traffic.md §2026-10-20T12:11:03](../../re/pregen-traffic.md#ts-20261020-121103).
- The loaded-level list and the current room when the loaders run: [pregen-traffic.md §2026-10-20T13:13:47](../../re/pregen-traffic.md#ts-20261020-131347).
- Manager flags and EnableTrafficAndPedestrians: [script-natives-audio.md §2026-10-15T15:10:38](../../re/script-natives-audio.md#ts-20261015-151038) (its manager names are swapped: [pedestrians.md §2026-10-20T11:17:42](../../re/pedestrians.md#ts-20261020-111742)).
- No path gets a node at run time, so the samplers never transform: [pedestrians-character.md §2026-10-20T13:09:29](../../re/pedestrians-character.md#ts-20261020-130929).
- The traffic floor cell 0x1edaac and the points without one: [pregen-traffic.md §2026-10-25T12:36:20](../../re/pregen-traffic.md#ts-20261025-123620).

## 7. Code and tests

- `src/assets/pregen_traffic.*` (`parsePregenSpawns`, `findPregenSpawnCell`, `pregenSpawnPath`), registered in `src/assets/file_decoders.cpp`.
- `src/runtime/pregen_derive.*` (`deriveSpawnFields`, `findTrafficFloorCell`, `floorCellContains`, `deriveTrafficFloorCells`).
- CTest: `pregen-derive`, `pregen-derive-real-data` (skips with 77 without the RomFS), `pregen-traffic` (synthetic files, rejections, lookup), `pregen-traffic-real-data` (the 14 shipped files; skips with 77 without the RomFS).
