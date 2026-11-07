# Traffic cars: the route mover and the junctions

## 1. Scope

How a traffic car moves in AI driver states 2 and 3, and the junctions it moves through:
- the route mover 0x2613ac (turn, move, target advance, speed);
- the stop test at junction lights 0x2ca504;
- the junction choice 0x2cb1e8 (output, lane, queue entry, request of state 3);
- the junction queue 0x22a080;
- the junction set-up 0x1ca5a4 (ends, sides, choice table, lanes, conflict pairs, light pairs) and the light cycle 0x1d8924.

Not covered:
- the AI driver states, the state change and the post step that call these ([pedestrians-traffic-cars-driver.md](pedestrians-traffic-cars-driver.md));
- the manager and the route list ([pedestrians-traffic-cars.md](pedestrians-traffic-cars.md)).

## 2. Status

IN_PROGRESS, updated 2026-11-07T12:14:46+01:00.
- The four movement functions are ported as `src/runtime/traffic_route_mover.*`: pure steps on the driver record, the junction records and the route functions.
- The leave call 0x21d6b8 and the ground probe 0x20ae2c (the mover's hooks `leaveTraffic` and `snapTarget`) are ported as `src/runtime/traffic_leave_snap.*`; its ground ray cast and colour message are ported as `src/runtime/traffic_ground_world.*`.
- The junction records are filled from the level at the scene enter and kept by the traffic system (`src/runtime/traffic_junction_update.*`). The light cycle of every junction runs in the object phase of `chase-game` (after the runner, as the entity pass 0x1a3f80 runs it), and each light change prints a line.
- The traffic lights show the look of their set-up in `chase-game` (pair 0 green, the slot quirk), node by node (`src/runtime/traffic_light_parts.*`, glue `src/game_traffic_lights.*`); the cycle's part changes are ported but nothing delivers them yet (TRAFFIC-25).
- The route functions `evaluate`, `length` and `signal` read the junctions' route paths (level roads and lanes); `ahead` and `nearest` are not ported (TRAFFIC-23).
- The AI driver is not connected to the spawned cars (TRAFFIC-2 in [pedestrians-traffic-cars-driver.md](pedestrians-traffic-cars-driver.md)), so the cars still stand still and no car is ever queued.

## 3. Verified behaviour

Read from the code (test `traffic-route-mover`), all from [traffic-ai-driver.md §2026-11-02T23:44:38](../../re/traffic-ai-driver.md#ts-20261102-234438):
- The mover:
  - the counter +0xae with the call of 0x20ae2c every sixth update;
  - the turn toward the target above speed 1.0, by dt x 4 or dt x 6 above 20, and the copy of the rows to +0x18;
  - the move of dt x speed along row 2;
  - the target advance within entity +0x5c: 0.4 per step, 0.15 on a route of length 4 (a lane), past the length along row 2;
  - at the route end, the junction choice with the request of state 3, or the leave call without a junction;
  - the kinematic velocity, the engine blend body +0x27c, and the speed that moves 0.18 of the way to +0x70 (it takes +0x70 within 0.001).
- The junction choice: no choice before length − 0.5; the random enabled output (random % (count x 8) >> 3, drawn again while the route bit is clear), the lane joining the input and output, the nearest sample on the lane, the queue entry and bit 3.
- The stop test: the lights of the junction's pairs that list the car's input road stop it unless their pair is +0x300 and the phase +0x308 is not red. A car on a lane never stops for lights.
- The queue: the search from the last entry, the swap with the last entry, the count that drops even when the car is not found, and bit 3 cleared.
- The leave call: a pinned car is left alone; else an enabled car is despawned; the fade byte is 255 ([traffic-ai-driver.md §2026-11-03T08:37:32](../../re/traffic-ai-driver.md#ts-20261103-083732)).
- The probe: a ray 1.5 above to 1.5 below the target, the weights and colour of the hit triangle sent to the car's node; it moves nothing ([traffic-ai-driver.md §2026-11-03T08:37:32](../../re/traffic-ai-driver.md#ts-20261103-083732)).
- The ground ray cast (test `traffic-ground-world`, `traffic-ground-world-real-data`), from [traffic-ground-cast.md §2026-11-07T12:11:38](../../re/traffic-ground-cast.md#ts-20261107-121138): a vertical segment against the level collision with include mask 1, exclude 0 and no surface mask (vehicle boxes count); the first accepted entry that is hit ends the cast, in a mesh the first front-facing triangle in tree order; a box hit gives no triangle, so no colour. The colour message sets three bytes of a mesh node.

Read from the code (tests `traffic-junction-fill`, `traffic-junction-fill-real-data`, `traffic-route-setup`), all from [traffic-junction.md §2026-11-03T08:53:43](../../re/traffic-junction.md#ts-20261103-085343):
- The ends of the input and output paths (end point, the point before, the x/z travel direction) and the sides 1 / 2 of parallel ends (0x159ab8).
- The choice table: no turning back (dot −0.85), the side rule, one input with one output always joined.
- The lanes: four points, the Hermite points 0x20b0c0 at 1/3 and 2/3, type 0; the conflict pairs of lanes whose same-index segments come within 0.1 in x and z (0x20af64; [traffic-junction.md §2026-11-07T11:05:36](../../re/traffic-junction.md#ts-20261107-110536)).
- The light pairs `TrafficLight01..08` with their `SlaveLight`, the lights' routes `Path01..04`, and the start at pair 0, green, 6 s.
- The light cycle: green 6 s, yellow 2 s, red 3 s, the next pair green once the queue is empty; nothing with one pair.
- Shipped levels: lanes, conflicts, light pairs and sided ends per level, and lvl_01 `Junction1` in detail; names without a level path are skipped (the route list lost one bogus route per level). A path is looked up by the part of its name after the last `.` (`assets::pathNameHash`, 0x23895c), in the junction fill, the DCam paths and the pedestrian path count ([traffic-ground-cast.md §2026-11-07T12:11:38](../../re/traffic-ground-cast.md#ts-20261107-121138)).

Read from the code and wired (tests `traffic-junction-update`, `traffic-junction-update-real-data`, `game-traffic`), all from [traffic-junction.md §2026-11-07T12:09:40](../../re/traffic-junction.md#ts-20261107-120940):
- The junction update 0x1d8924 is the class's slot +0xc and runs in the entity pass 0x1a3f80 with its dt: after the traffic manager (system phase) and the runner, after the cars' vehicle update when the priority bytes are equal.
- The records of the scene enter keep their light state; the route set-up and the records use the same fills, so the junction index of the route → junction pairs is the record's index.
- The route paths: the level path of each junction input and output, and each lane's own 4-point path.

## 4. Stand-ins

- TRAFFIC-S11 Four cases where the original loops for ever, wraps a count or reads garbage throw instead: no enabled output among an input's choices, no lane joining the input and the chosen output, a 53rd queue entry, and leaving an empty queue (the count byte would wrap to 255). evidence: [traffic-ai-driver.md §2026-11-02T23:44:38](../../re/traffic-ai-driver.md#ts-20261102-234438)
- TRAFFIC-S13 The junction fill refuses a junction path of fewer than two points, a 65th conflict pair (the original writes over its stack) and a lane id equal to a level path's name; lanes are named by port ids, not record addresses; paths and lights are searched in all loaded levels, not in the object's scene; a lane tested against one with fewer points is refused (the original reads past its points).  evidence: [traffic-junction.md §2026-11-07T11:05:36](../../re/traffic-junction.md#ts-20261107-110536)
- TRAFFIC-S17 Every junction of the level data runs its light cycle at every update, in object order, in the object phase; the original runs the junction entities of its update list (room placement, skip bits, filter) in the list's sorted order. needs: TRAFFIC-24. evidence: [traffic-junction.md §2026-11-07T12:09:40](../../re/traffic-junction.md#ts-20261107-120940)
- TRAFFIC-S23 The ground cast tries the collision entries in the order of the world, not in the order of the grid cells, and without its cap of 100 entries; composite box owners (flag 0x800) and a slanted segment are not modelled (the probe's segment is vertical). needs: TRAFFIC-29. evidence: [traffic-ground-cast.md §2026-11-07T12:11:38](../../re/traffic-ground-cast.md#ts-20261107-121138)
- TRAFFIC-S24 The colour message is sent to the mesh nodes the caller names; the port keeps the three bytes (+0x118..+0x11a) and does not paint the white vertex colours of the layers, and no draw reads the bytes. A colour table (item flag word set) is refused; no shipped item sets it. needs: TRAFFIC-30, TRAFFIC-2. evidence: [traffic-ground-cast.md §2026-11-07T12:11:38](../../re/traffic-ground-cast.md#ts-20261107-121138)
- TRAFFIC-S19 The cycle's part changes reach the look only through `TrafficLightLooks::applyChange`, which nothing calls yet, so every light keeps the look of the set-up (pair 0 green, the others as the slot quirk leaves them). needs: TRAFFIC-25. evidence: [traffic-junction.md §2026-11-07T12:14:46](../../re/traffic-junction.md#ts-20261107-121446)
- TRAFFIC-S20 The junction set-up is taken to run after all light creates (the order is not read), and `TrafficLightLooks` computes its own junction fills of the level; a light listed by two junctions is drawn from the last one. evidence: [traffic-junction.md §2026-11-07T12:14:46](../../re/traffic-junction.md#ts-20261107-121446)

## 5. Open items

- [ ] TRAFFIC-23 — The route functions `ahead` and `nearest` of the route type table for the traffic routes and lanes; `trafficRouteFunctions` leaves them unset. blocks: TRAFFIC-2. evidence: [traffic-junction.md §2026-11-07T12:09:40](../../re/traffic-junction.md#ts-20261107-120940)
- [ ] TRAFFIC-24 — Which junctions are in the update list (the rooms of the shipped junctions, room type 16, the writers of entity hword +0x10 bits 0x24) and the priority byte +0x19 of the junction and car entities (whether the cars update before the junctions). blocks: TRAFFIC-S17. evidence: [traffic-junction.md §2026-11-07T12:09:40](../../re/traffic-junction.md#ts-20261107-120940)
- [ ] TRAFFIC-25 — Call `TrafficLightLooks::applyChange` with the result of `runTrafficJunctionLights` for each junction of the level in the junction update, so the lights change colour. blocks: TRAFFIC-S19. evidence: [traffic-junction.md §2026-11-07T12:14:46](../../re/traffic-junction.md#ts-20261107-121446)
- [ ] TRAFFIC-26 — Read what the physical prop update of the traffic light does for a traffic light and port it with the prop's Mass and Buoyancy attributes. evidence: [traffic-junction.md §2026-11-07T12:14:46](../../re/traffic-junction.md#ts-20261107-121446)
- [ ] TRAFFIC-29 — Read the order in which the grid gathers the entries and its cap of 100, the composite box owners and the walk for a slanted segment, and give `castTrafficGround` the original order. blocks: TRAFFIC-S23. evidence: [traffic-ground-cast.md §2026-11-07T12:11:38](../../re/traffic-ground-cast.md#ts-20261107-121138)
- [ ] TRAFFIC-30 — Read the node tree of a car, the draw's use of the node's tint bytes and the vertex paint of the colour message, and send the message to the car's mesh nodes after the probe. needs: TRAFFIC-2. blocks: TRAFFIC-S24. evidence: [traffic-ground-cast.md §2026-11-07T12:11:38](../../re/traffic-ground-cast.md#ts-20261107-121138)

## 6. Findings summary

- [traffic-junction.md §2026-11-07T12:09:40](../../re/traffic-junction.md#ts-20261107-120940): the caller of the junction update (entity pass 0x1a3f80, slot +0xc), which junctions are in the update list (0x173534, layer class 2), the sort by priority and class type, the order against the traffic manager, the runner and the cars.
- [traffic-ai-driver.md §2026-11-03T08:37:32](../../re/traffic-ai-driver.md#ts-20261103-083732): the leave call 0x21d6b8, the ground probe 0x20ae2c with the triangle weights 0x2b51b8 and the colour 0x221064.
- [traffic-ai-driver.md §2026-11-02T23:44:38](../../re/traffic-ai-driver.md#ts-20261102-234438): the route mover 0x2613ac step by step, the junction choice 0x2cb1e8 with the choice table and the lanes, the stop test 0x2ca504 with the lights 0x2786a8, the junction queue 0x22a080 and its callers.
- [traffic-ground-cast.md §2026-11-07T12:11:38](../../re/traffic-ground-cast.md#ts-20261107-121138): the vertical ground ray cast, the colour message on mesh nodes and the path name lookup 0x23895c.
- [traffic-junction.md §2026-11-07T12:14:46](../../re/traffic-junction.md#ts-20261107-121446): the node part flags 0x2363d4, 0x20b19c, the set-up slot index quirk, and the light class update being the physical prop update.
- [traffic-junction.md §2026-11-07T11:05:36](../../re/traffic-junction.md#ts-20261107-110536): the conflict test compares same-index segments (0x1cad58), the re-pinned conflict counts, and the path name split 0x23895c.
- [traffic-junction.md §2026-11-03T08:53:43](../../re/traffic-junction.md#ts-20261103-085343): the junction and light classes, the set-up 0x1ca5a4 (ends, sides 0x159ab8, choices, Hermite lanes 0x20b0c0, conflicts 0x20af64, light pairs), the light set-up 0x19ff70, the light cycle 0x1d8924 and its times, the shipped junctions.

## 7. Code and tests

`src/runtime/traffic_light_parts.{hpp,cpp}` (`trafficLightSetupOps`, `trafficLightPhaseOps`, `TrafficJunctionLightParts`, `trafficLightPartNode`, no SDL); CTest `traffic-light-parts`, `traffic-light-parts-real-data` (`tests/traffic_light_parts_test.cpp`, the second skips with 77 without the RomFS). Glue `src/game_traffic_lights.{hpp,cpp}` (`TrafficLightLooks`).

`src/runtime/traffic_route_mover.{hpp,cpp}` (`runTrafficRouteMover`, `chooseTrafficJunction`, `trafficStopAtLights`, `trafficLightControls`, `leaveTrafficJunctionQueue`, `TrafficJunctionRecord`, `TrafficJunctionWorld`, no SDL); CTest `traffic-route-mover` (`tests/traffic_route_mover_test.cpp`). `src/runtime/traffic_junction_fill.{hpp,cpp}` (`trafficJunctionFills`, `fillTrafficJunction`, `trafficJunctionChoices`, `trafficJunctionLanePath`, `trafficLanesCross`, `runTrafficJunctionLights`, no SDL); CTest `traffic-junction-fill`, `traffic-junction-fill-real-data` (`tests/traffic_junction_fill_test.cpp`, the second skips with 77 without the RomFS).

`src/runtime/traffic_route_mover.{hpp,cpp}` (`runTrafficRouteMover`, `chooseTrafficJunction`, `trafficStopAtLights`, `trafficLightControls`, `leaveTrafficJunctionQueue`, `TrafficJunctionRecord`, `TrafficJunctionWorld`, no SDL); CTest `traffic-route-mover` (`tests/traffic_route_mover_test.cpp`).

`src/runtime/traffic_junction_update.{hpp,cpp}` (`TrafficRoutePaths`, `TrafficJunctions`, `trafficJunctionLayouts`, `keepTrafficJunctions`, `runTrafficJunctionUpdates`, `trafficRouteFunctions`, no SDL); CTest `traffic-junction-update`, `traffic-junction-update-real-data` (`tests/traffic_junction_update_test.cpp`, the second skips with 77 without the RomFS). Kept by `TrafficSystem` (`junctions()`, `updateJunctions`); glue `LevelTraffic::updateJunctions` and `LevelSession::updateTrafficJunctions` in `src/game_traffic.cpp`, one line in the object phase of `src/game_level_update.cpp`; CTest `game-traffic` (slow) pins the first light changes.

`src/runtime/traffic_ground_world.{hpp,cpp}` (`castTrafficGround`, `trafficGroundWorld`, `applyTrafficColourMessage`, no SDL); CTest `traffic-ground-world`, `traffic-ground-world-real-data` (`tests/traffic_ground_world_test.cpp`, the second skips with 77 without the RomFS). `src/assets/path_name_hash.{hpp,cpp}` (`pathNameHash`, `pathLookupName`); CTest `path-name-hash`.

`src/runtime/traffic_leave_snap.{hpp,cpp}` (`leaveTraffic`, `trafficGroundSegment`, `trafficTriangleWeights`, `trafficGroundColour`, `snapTrafficTarget`, `TrafficGroundWorld`, no SDL); CTest `traffic-leave-snap` (`tests/traffic_leave_snap_test.cpp`). `leaveTraffic` of the leave call 0x21d6b8 is a public method of `TrafficManager` (`src/runtime/traffic_manager.*`), because the fade bytes and `despawnCar` are private; `traffic_leave_snap` calls it.
