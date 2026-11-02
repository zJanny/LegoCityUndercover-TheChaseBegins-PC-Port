# Traffic manager and the AI driver input

Subtopics: [pedestrians-traffic-cars-driver.md](pedestrians-traffic-cars-driver.md)

## 1. Scope

The traffic manager 0x527410 and the AI driver input of the spawned cars. The manager spawns and removes the level's `Traffic%02u` vehicles around the player, keeps the vehicle limit (`Traffic_MaxVehicles`, SetTrafficMaxVehicles) and fades cars in and out at `Traffic_FadeBound` boxes. The AI driver input 0x26d934 drives a spawned car along its route. Not covered: pedestrians (the pedestrian manager 0x51b33c, [pedestrians-traffic-pedestrians.md](pedestrians-traffic-pedestrians.md)), the pregen file format ([pedestrians-traffic.md](pedestrians-traffic.md)), the vehicle physics body ([vehicles-physics.md](vehicles-physics.md)), pursuit.

## 2. Status

IN_PROGRESS, updated 2026-11-02T23:47:41+01:00. The manager is wired into `chase-game` by `TrafficSystem`: at the scene enter it sets up the level's routes (the junction objects, the route list, the route to junction pairs), loads the level's pregen traffic file when a junction is within reach and runs the prefill, and every update (system phase, after the pedestrians) it spawns and removes the level's `Traffic%02u` cars at pregen points around the player on enabled routes; EnableTrafficAndPedestrians, SetTrafficMaxVehicles and EnableTrafficPath reach it. The cars stand still (TRAFFIC-S7) because the AI driver is not connected to them ([pedestrians-traffic-cars-driver.md](pedestrians-traffic-cars-driver.md)). The route port `traffic_route.*` and its set-up `traffic_route_setup.*` are wired to the data side; the follower and the junction test are not called yet (TRAFFIC-2).

## 3. Verified behaviour

- Read from the code (test `traffic-manager`), all from [traffic.md §2026-10-20T11:18:14](../../re/traffic.md#ts-20261020-111814):
  - the level load: the vehicle limit is min(`Traffic_MaxVehicles` default 5, car count); the spawn order is shuffled with the game's RNG; every car starts disabled with fade 255;
  - SetTrafficMaxVehicles: the argument is truncated and compared unsigned with the count;
  - the update: removal of every car while traffic is off; one spawn per update while below the limit when the countdown reaches 0, with countdown (live + live / 2) x 3; one removal per update (room, driver state, fade and distance rules); cars with owner flag 0x20 are kept;
  - the spawn: 10 tries in the leaf, the route bit, the room test, the LOD distance band (0.7 x near to 1.75 x far, 0 to far in the prefill), 16 m route spacing in x and z;
  - the 64 prefill updates of the room set-up;
  - the fade inside a `Traffic_FadeBound` box.
- Read from the code (tests `traffic-route`, `traffic-route-real-data`), all from [traffic-ai-driver.md §2026-10-25T12:19:38](../../re/traffic-ai-driver.md#ts-20261025-121938):
  - the route list: input then output paths of the junctions within 9000 of the origin (at most 32), each new one with its bit set; EnableTrafficPath and its pending list, replayed by the room set-up (these are the manager's route bits since 2026-11-02);
  - the route → junction pairs 0x2dfc84 and the driver's junction 0x20ff40;
  - the path follower 0x2bf2ec (nearest enabled point of the cell, below 9999, dot above 0.5 by its float bits) and the route type function (table 0x3581d4: type 0 0x1c17c0, type 1 0x1b33fc);
  - the junction test 0x2cdde4;
  - shipped levels: every traffic point lies on an open type 0 junction path; junction, route and path counts pinned.

- Wired, from [traffic-ai-driver.md §2026-11-02T23:46:29](../../re/traffic-ai-driver.md#ts-20261102-234629) (tests `traffic-route-setup`, `traffic-route-setup-real-data`, `traffic-system`, `traffic-manager`, `natives-progress`, `game-traffic-path`):
  - the junction objects of the level (`props/traffic_junction`: position, InputPath01..08, OutputPath01..08) give the route list in the manager's `TrafficRouteTable`; a pregen point's route is the index of its path in that list; the spawn test reads its bit; the pairs are kept for TRAFFIC-2;
  - without a junction within 9000 the pregen file is not loaded and nothing spawns; shipped levels have 16, 7, 2, 4, 4, 14 and 4 junctions and 58, 31, 3, 5, 4, 40 and 14 routes;
  - EnableTrafficPath (native 0x27bc30, 40 calls in lvl_03 and lvl_07) names a path and a flag; calls before the scene enter queue disables that the set-up replays (lvl_03 `3-7_MissionStart` disables `RoadPath27..30`).
- Wired, from [traffic.md §2026-10-25T12:35:43](../../re/traffic.md#ts-20261025-123543) (tests `traffic-system`, `traffic-system-real-data`, `game-traffic`, `game-traffic-natives`):
  - a spawn places the car at the point with y raised by `TrafficHeightOffRoad` minus the centre of mass y and the point's direction (12 oracle spawns equal to four decimals); point +0x20 and driver +0x64 are the path parameter;
  - the LOD table is the pedestrian distance fade table, the ten cars of lvl_01 are LOD class 1 (spawn band 50.4 to 133, prefill 0 to 76) and have the removal radius 50; lvl_01 has ten cars and the limit 10;
  - the driver bytes after a spawn (0/1, 1/2, 2/2) and the grace byte are the ones of the state change in the entry above; the oracle shows the same.

## 4. Stand-ins

- TRAFFIC-S1 Facts the port chooses or measured on lvl_01 only: the cars' rooms use the pedestrian room object (PED-S2); layer class 0 and the node flags (LOD class 1) are those of lvl_01's ten cars for every level; no car is pinned; the generator is the fixed-seed one of the pedestrians (PED-S6); the traffic update runs after the pedestrian update (slot order not read). needs: TRAFFIC-12. evidence: [traffic.md §2026-10-25T12:35:43](../../re/traffic.md#ts-20261025-123543)
- TRAFFIC-S2 Two cases the original leaves unread are refused by the port's own choice: an empty leaf makes the spawn fail (the original would compute random % 0), and a LOD class outside 0..2 throws (the original's -1 indexes the table one entry low). needs: TRAFFIC-3. evidence: [traffic.md §2026-10-20T11:18:14](../../re/traffic.md#ts-20261020-111814)
- TRAFFIC-S3 A route missing from the route list counts as disabled in the follower and a pregen point whose path is not in the list has no route (the spawn skips it); the original uses the stored +0x24 word of the path record. Every shipped traffic point lies on a junction path (`traffic-route-real-data`, `traffic-route-setup-real-data`). needs: TRAFFIC-14. evidence: [traffic-ai-driver.md §2026-10-25T12:19:38](../../re/traffic-ai-driver.md#ts-20261025-121938)
- TRAFFIC-S4 The route list +0xc4, the pending route list and the two route → junction lists refuse a 129th entry, and a route type above 1 throws; the original writes past the lists and reads words after its table. needs: TRAFFIC-4. evidence: [traffic-ai-driver.md §2026-10-25T12:19:38](../../re/traffic-ai-driver.md#ts-20261025-121938)
- TRAFFIC-S8 The removal radius 0x26e130 is 50 for every car (the row comes from record +0x20, whose writer is not read; the ten cars of lvl_01 read radius 50). The LOD table 0x529e30 is no stand-in: its values are pinned in `traffic-system` (request TRAFFIC-1 of 20261025-traffic-servant). needs: TRAFFIC-12. evidence: [traffic.md §2026-10-25T12:35:43](../../re/traffic.md#ts-20261025-123543)

- TRAFFIC-S9 The junction lanes, conflict pairs and listed lanes are not read: `trafficJunctions` leaves them empty, so the junction test never stops a car. needs: TRAFFIC-4. evidence: [traffic-ai-driver.md §2026-11-02T23:46:29](../../re/traffic-ai-driver.md#ts-20261102-234629)
- TRAFFIC-S10 Every junction object of the level's data is taken (the original's global junction list holds the created objects only), and the EnableTrafficPath calls reach the traffic system at its next scene enter or update, not at the call. needs: TRAFFIC-15. evidence: [traffic-ai-driver.md §2026-11-02T23:46:29](../../re/traffic-ai-driver.md#ts-20261102-234629)

## 5. Open items

- [ ] TRAFFIC-3 — Read the rest of the manager: the layer table of 0x1ffbd8, the LOD class -1 of 0x28781c, and a leaf without points. blocks: TRAFFIC-S2. evidence: [traffic.md §2026-10-20T11:18:14](../../re/traffic.md#ts-20261020-111814)
- [ ] TRAFFIC-4 — The junction class `props/traffic_junction` (set-up 0x1ca5a4, update 0x1d8924): its lanes, conflict pairs, the lane list +0x284 that 0x2cdde4 reads, and the traffic lights. blocks: TRAFFIC-S4, TRAFFIC-S9, TRAFFIC-18. evidence: [traffic-ai-driver.md §2026-10-25T12:19:38](../../re/traffic-ai-driver.md#ts-20261025-121938)

- [ ] TRAFFIC-14 — The initial value of path record +0x24 in the level files and what the spawn and the follower do for a path outside the route list; the port gives such a point no route. blocks: TRAFFIC-S3. evidence: [traffic-ai-driver.md §2026-11-02T23:46:29](../../re/traffic-ai-driver.md#ts-20261102-234629)
- [ ] TRAFFIC-15 — Which junction objects the original's global list holds (creation flags, rooms not streamed in) and the timing of an EnableTrafficPath call against the traffic update inside one frame; the port takes every object of the class and applies the calls at the next update. blocks: TRAFFIC-S10. evidence: [traffic-ai-driver.md §2026-11-02T23:46:29](../../re/traffic-ai-driver.md#ts-20261102-234629)
- [ ] TRAFFIC-12 — The cars of the other levels: the node flags (LOD class), layer class, the writer of record +0x20 (removal radius row) and the room-type byte; only lvl_01's ten cars were read. blocks: TRAFFIC-S1, TRAFFIC-S8. evidence: [traffic.md §2026-10-25T12:35:43](../../re/traffic.md#ts-20261025-123543)
- [ ] TRAFFIC-13 — The removal radius (50, TRAFFIC-S8) is not an input of `TrafficSystemStep` (it is set inside `TrafficSystem::syncCars`), so a QoL hook cannot scale it; make it an input with default 50. blocks: QOLVD-6. evidence: [view-edges.md §2026-11-02T21:25:47](../../re/view-edges.md#ts-20261102-212547)

## 6. Findings summary

- [traffic.md §2026-10-20T11:18:14](../../re/traffic.md#ts-20261020-111814): which object is the traffic manager (0x527410, not 0x51b33c), its layout, level load, room set-up and prefill, the update with its spawn and removal rules, the spawn 0x2def8c, the fade 0x2df8b8, SetTrafficMaxVehicles, the choice of input provider in 0x1571dc, and the states of the AI driver 0x26d934 read so far.
- [traffic-ai-driver.md §2026-10-25T12:19:38](../../re/traffic-ai-driver.md#ts-20261025-121938): the junctions as owners of the routes, the route list, EnableTrafficPath and the pending list, the route → junction pairs, the path follower 0x2bf2ec, the route type function and the junction test 0x2cdde4.
- [traffic-ai-driver.md §2026-10-25T12:26:09](../../re/traffic-ai-driver.md#ts-20261025-122609): the driver record, the writers of +0x78 / +0x79, the state change 0x1cb148, all states of 0x26d934, the post step 0x1bc79c, the kinematic switch 0x22a270 → 0x213c74 (flags 0x102), the car messages 0x80000003..7.
- [traffic-ai-driver.md §2026-11-02T23:46:29](../../re/traffic-ai-driver.md#ts-20261102-234629): the routes wired into the traffic system: junction list, route index of a point, the EnableTrafficPath native and its queue before the scene enter.
- [traffic.md §2026-10-25T12:35:43](../../re/traffic.md#ts-20261025-123543): the wiring in `chase-game`, the spawn placement (height offset and direction, the point's path parameter), the cars' node and driver values, the LOD table and class, the removal radius, the driver steps after a spawn.

## 7. Code and tests

`src/runtime/traffic_manager.{hpp,cpp}` (`TrafficManager`, `runSetTrafficMaxVehicles`, no SDL); CTest `traffic-manager` (`tests/traffic_manager_test.cpp`). `src/runtime/traffic_route_setup.{hpp,cpp}` (`trafficJunctions`, `setUpTrafficRoutes`, `trafficPointRoute`, `runEnableTrafficPath`, no SDL); CTest `traffic-route-setup`, `traffic-route-setup-real-data` (`tests/traffic_route_setup_test.cpp`, the second skips with 77 without the RomFS). `src/runtime/traffic_route.{hpp,cpp}` (route list, `enableTrafficPath`, `findTrafficRoutePoint`, `sampleTrafficRoute`, `trafficJunctionAllowsCar`, no SDL); CTest `traffic-route`, `traffic-route-real-data` (`tests/traffic_route_test.cpp`, the second skips with 77 without the RomFS). `src/runtime/ai_driver_input.{hpp,cpp}` (`createAiDriverRecord`, `runAiDriverInput`, `runAiDriverStateChange`, `runAiDriverPostStep`, no SDL); CTest `ai-driver-input` (`tests/ai_driver_input_test.cpp`). `src/runtime/traffic_system.{hpp,cpp}` (`TrafficSystem`: cars, pregen file, route numbers, frame, stand-in driver steps, no SDL); glue `src/game_traffic.{hpp,cpp}` (`LevelTraffic`, and the `LevelSession` methods that place the car's vehicle, take the natives and apply the fade). CTest `traffic-system`, `traffic-system-real-data`, `game-traffic`, `game-traffic-natives` (slow). `game.cpp` is untouched. The native EnableTrafficPath is in `natives_progress.*` (`takeTrafficPathRequests`); CTest `game-traffic-path` (slow).
