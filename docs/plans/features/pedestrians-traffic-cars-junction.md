# Traffic cars: the junction records, the light cycle and the traffic lights

## 1. Scope

The junctions the traffic cars move through, and their lights:
- the junction set-up 0x1ca5a4 (ends, sides, choice table, lanes, conflict pairs, light pairs);
- the light cycle 0x1d8924 and the junction update that runs it (update list, order);
- the look of the traffic lights (the parts red, yellow and green) and the delivery of the cycle's changes to it.

Not covered:
- the route mover, the stop test, the junction choice, the queue and the ground probe ([pedestrians-traffic-cars-movement.md](pedestrians-traffic-cars-movement.md));
- the AI driver ([pedestrians-traffic-cars-driver.md](pedestrians-traffic-cars-driver.md)) and the manager ([pedestrians-traffic-cars.md](pedestrians-traffic-cars.md)).

## 2. Status

IN_PROGRESS, updated 2026-11-07T18:12:03+01:00.
- The junction records are filled from the level at the scene enter and kept by the traffic system (`src/runtime/traffic_junction_update.*`). The light cycle of every junction runs in the object phase of `chase-game` (after the runner, as the entity pass 0x1a3f80 runs it), prints a line per light change and delivers the change to the lights' looks.
- The traffic lights show the look of their set-up first (pair 0 green, the slot quirk) and then follow the cycle, node by node (`src/runtime/traffic_light_parts.*`, glue `src/game_traffic_lights.*`).
- Which junctions are in the update list depends on the room type of their room, which is not read (TRAFFIC-31). The priority bytes are read: cars update before junctions.

## 3. Verified behaviour

Read from the code (tests `traffic-junction-fill`, `traffic-junction-fill-real-data`, `traffic-route-setup`), all from [traffic-junction.md §2026-11-03T08:53:43](../../re/traffic-junction.md#ts-20261103-085343):
- The ends of the input and output paths (end point, the point before, the x/z travel direction) and the sides 1 / 2 of parallel ends (0x159ab8).
- The choice table: no turning back (dot -0.85), the side rule, one input with one output always joined.
- The lanes: four points, the Hermite points 0x20b0c0 at 1/3 and 2/3, type 0; the conflict pairs of lanes whose same-index segments come within 0.1 in x and z (0x20af64; [traffic-junction.md §2026-11-07T11:05:36](../../re/traffic-junction.md#ts-20261107-110536)).
- The light pairs `TrafficLight01..08` with their `SlaveLight`, the lights' routes `Path01..04`, and the start at pair 0, green, 6 s.
- The light cycle: green 6 s, yellow 2 s, red 3 s, the next pair green once the queue is empty; nothing with one pair.
- Shipped levels: lanes, conflicts, light pairs and sided ends per level, and lvl_01 `Junction1` in detail; names without a level path are skipped. A path is looked up by the part of its name after the last `.` (`assets::pathNameHash`, 0x23895c).

Read from the code and wired (tests `traffic-junction-update`, `traffic-junction-update-real-data`, `game-traffic`), from [traffic-junction.md §2026-11-07T12:09:40](../../re/traffic-junction.md#ts-20261107-120940):
- The junction update 0x1d8924 is the class's slot +0xc and runs in the entity pass 0x1a3f80 with its dt: after the traffic manager (system phase) and the runner.
- The records of the scene enter keep their light state; the route set-up and the records use the same fills, so the junction index of the route -> junction pairs is the record's index.
- The route paths: the level path of each junction input and output, and each lane's own 4-point path.

Read from the code and wired (tests `traffic-light-parts`, `traffic-junction-update-real-data`), from [traffic-junction.md §2026-11-07T12:14:46](../../re/traffic-junction.md#ts-20261107-121446) and [§2026-11-07T18:12:03](../../re/traffic-junction.md#ts-20261107-181203):
- The part flags 0x2363d4, the changes 0x20b19c, the set-up slot quirk and the cycle's changes.
- Each change of the cycle reaches the looks of its junction in the update that makes it: on lvl_01 the first pair of junction 1 shows yellow only from update 181 and red only from update 242.
- The priority byte +0x19 is 128 for every placed object (loader 0x1a4214) and neither the junction nor the traffic car create changes it, so the sort puts the cars (class 183) before the junctions (192). The junctions have no `DeferredLoad` and no create flags in any shipped level.

## 4. Stand-ins

- TRAFFIC-S13 The junction fill refuses a junction path of fewer than two points, a 65th conflict pair (the original writes over its stack) and a lane id equal to a level path's name; lanes are named by port ids, not record addresses; paths and lights are searched in all loaded levels, not in the object's scene; a lane tested against one with fewer points is refused (the original reads past its points).  evidence: [traffic-junction.md §2026-11-07T11:05:36](../../re/traffic-junction.md#ts-20261107-110536)
- TRAFFIC-S17 Every junction of the level data runs its light cycle at every update, in object order, in the object phase; the original runs the junction entities of its update list (room type of the room, skip bit 0x04, filter) in the list's sorted order. needs: TRAFFIC-31. evidence: [traffic-junction.md §2026-11-07T18:12:03](../../re/traffic-junction.md#ts-20261107-181203)
- TRAFFIC-S20 The junction set-up is taken to run after all light creates (the order is not read), and `TrafficLightLooks` computes its own junction fills of the level; a light listed by two junctions is drawn from the last one. evidence: [traffic-junction.md §2026-11-07T12:14:46](../../re/traffic-junction.md#ts-20261107-121446)


## 5. Open items

- [ ] TRAFFIC-26 — Read what the physical prop update of the traffic light does for a traffic light and port it with the prop's Mass and Buoyancy attributes. evidence: [traffic-junction.md §2026-11-07T12:14:46](../../re/traffic-junction.md#ts-20261107-121446)
- [ ] TRAFFIC-31 — Read the writer of the room object's type byte +0x3a (the types of the rooms that hold shipped junctions) and the writers of bit 0x04 of entity hword +0x10, so the junction update list can be modelled. blocks: TRAFFIC-S17. evidence: [traffic-junction.md §2026-11-07T18:12:03](../../re/traffic-junction.md#ts-20261107-181203)

## 6. Findings summary

- [traffic-junction.md §2026-11-07T18:12:03](../../re/traffic-junction.md#ts-20261107-181203): the priority byte 128 of junctions and cars, the skip bits, the rooms of the shipped junctions and the delivery of the light changes to the looks.
- [traffic-junction.md §2026-11-07T12:09:40](../../re/traffic-junction.md#ts-20261107-120940): the caller of the junction update (entity pass 0x1a3f80, slot +0xc), which junctions are in the update list (0x173534, layer class 2), the sort by priority and class type, the order against the traffic manager, the runner and the cars.
- [traffic-junction.md §2026-11-07T12:14:46](../../re/traffic-junction.md#ts-20261107-121446): the node part flags 0x2363d4, 0x20b19c, the set-up slot index quirk, and the light class update being the physical prop update.
- [traffic-junction.md §2026-11-07T11:05:36](../../re/traffic-junction.md#ts-20261107-110536): the conflict test compares same-index segments (0x1cad58), the re-pinned conflict counts, and the path name split 0x23895c.
- [traffic-junction.md §2026-11-03T08:53:43](../../re/traffic-junction.md#ts-20261103-085343): the junction and light classes, the set-up 0x1ca5a4 (ends, sides 0x159ab8, choices, Hermite lanes 0x20b0c0, conflicts 0x20af64, light pairs), the light set-up 0x19ff70, the light cycle 0x1d8924 and its times, the shipped junctions.

## 7. Code and tests

`src/runtime/traffic_light_parts.{hpp,cpp}` (`trafficLightSetupOps`, `trafficLightPhaseOps`, `TrafficJunctionLightParts`, `TrafficLightPartStates`, `trafficLightPartNode`, no SDL); CTest `traffic-light-parts`, `traffic-light-parts-real-data` (`tests/traffic_light_parts_test.cpp`, the second skips with 77 without the RomFS). Glue `src/game_traffic_lights.{hpp,cpp}` (`TrafficLightLooks`, `applyChange` called from `LevelSession::updateTrafficJunctions` in `src/game_traffic.cpp`).

`src/runtime/traffic_junction_fill.{hpp,cpp}` (`trafficJunctionFills`, `fillTrafficJunction`, `trafficJunctionChoices`, `trafficJunctionLanePath`, `trafficLanesCross`, `runTrafficJunctionLights`, no SDL); CTest `traffic-junction-fill`, `traffic-junction-fill-real-data` (`tests/traffic_junction_fill_test.cpp`, the second skips with 77 without the RomFS).

`src/runtime/traffic_junction_update.{hpp,cpp}` (`TrafficRoutePaths`, `TrafficJunctions`, `trafficJunctionLayouts`, `keepTrafficJunctions`, `runTrafficJunctionUpdates`, `trafficRouteFunctions`, no SDL); CTest `traffic-junction-update`, `traffic-junction-update-real-data` (`tests/traffic_junction_update_test.cpp`, the second skips with 77 without the RomFS). Kept by `TrafficSystem` (`junctions()`, `updateJunctions`); glue `LevelTraffic::updateJunctions` and `LevelSession::updateTrafficJunctions` in `src/game_traffic.cpp`, one line in the object phase of `src/game_level_update.cpp`; CTest `game-traffic` (slow) pins the first light changes.
