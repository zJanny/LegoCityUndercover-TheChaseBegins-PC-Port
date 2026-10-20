# Pedestrians and traffic

## 1. Scope

The pedestrian manager 0x51b33c and the traffic manager 0x527410 and their spawn data: the `Pregen/Traffic` and `Pregen/Pedestrians` files (spawn points on level paths, a quadtree of spawn lists). The managers' update and spawn rules are SCRIPT-230 in [script-system-missions.md](script-system-missions.md); traffic driving is VEH-12 in [vehicles.md](vehicles.md); related: VEH-91 (vehicle activation), VEH-106 (traffic smashing props), LEVOBJ-16 (pedestrian walk state).

## 2. Status

IN_PROGRESS, updated 2026-10-20T11:16:20+02:00. Both spawn files decode in `chase_assets` (all 14 shipped files, census pinned) together with the cell lookup 0x1eb490. Nothing is wired into `chase-game`: no manager uses the data yet, and the fields the loaders derive (path tangent, room, floor cell) are not computed.

## 3. Verified behaviour

- File layout from the loaders 0x2e02d4 (traffic) and 0x2ee868 (pedestrians) and the cell readers 0x1edbe4 / 0x1eb254; every shipped file parses to its last byte with the recorded counts and checksums (`pregen-traffic-real-data`).
- The lookup 0x1eb490 descends the quadtree as the original (low edge inclusive, unordered compares take the low side); every point is found in the list of its own cell except 16 points of lvl_07_downtown traffic that are in no list (`pregen-traffic-real-data`).
- Every point's path hash names a type 0 path of its level, and the position equals the path sample at the stored parameter on open paths (data check, not a test).

## 4. Stand-ins

- PREGEN-S1 The decoder rejects input the original reads without checks: list indices beyond the points, lists longer than the reader's stack buffer (200 traffic, 256 pedestrians), trees deeper than 16 levels and bytes after the tree. No shipped file has any of them. evidence: [pregen-traffic.md §2026-10-20T11:16:20](../../re/pregen-traffic.md#ts-20261020-111620)

## 5. Open items

- [ ] PREGEN-1 — The fields the loaders derive are not ported: the path record of each point (first loaded level whose path section holds the hash), the unit tangent of the path at the parameter (sampler table 0x358d58 / 0x358a14), the room (0x239838, +36) and, for traffic, the floor cell (0x1edaac, +40). blocks: SCRIPT-230, VEH-12. evidence: [pregen-traffic.md §2026-10-20T11:16:20](../../re/pregen-traffic.md#ts-20261020-111620)
- [ ] PREGEN-2 — 47 pedestrian points on closed paths (42 in the test playground, 5 in lvl_07_downtown) lie up to 0.4 units from the port's closed-path sample at their parameter: the closed branch of 0x1c17c0 or a rounding of the stored parameter is not checked. evidence: [pregen-traffic.md §2026-10-20T11:16:20](../../re/pregen-traffic.md#ts-20261020-111620)

## 6. Findings summary

- Spawn files, loaders, quadtree, census; which manager loads which file: [pregen-traffic.md §2026-10-20T11:16:20](../../re/pregen-traffic.md#ts-20261020-111620).
- Manager flags and EnableTrafficAndPedestrians: [script-natives-audio.md §2026-10-15T15:10:38](../../re/script-natives-audio.md#ts-20261015-151038) (its manager names are swapped, see the entry above).

## 7. Code and tests

- `src/assets/pregen_traffic.*` (`parsePregenSpawns`, `findPregenSpawnCell`, `pregenSpawnPath`), registered in `src/assets/file_decoders.cpp`.
- CTest: `pregen-traffic` (synthetic files, rejections, lookup), `pregen-traffic-real-data` (the 14 shipped files; skips with 77 without the RomFS).
