# Traffic cars: the route mover and the ground probe

## 1. Scope

How a traffic car moves in AI driver states 2 and 3 through the junctions:
- the route mover 0x2613ac (turn, move, target advance, speed);
- the stop test at junction lights 0x2ca504;
- the junction choice 0x2cb1e8 (output, lane, queue entry, request of state 3);
- the junction queue 0x22a080.

Not covered:
- the AI driver states, the state change and the post step that call these ([pedestrians-traffic-cars-driver.md](pedestrians-traffic-cars-driver.md));
- the manager and the route list ([pedestrians-traffic-cars.md](pedestrians-traffic-cars.md));
- the junction set-up, the light cycle and the traffic lights ([pedestrians-traffic-cars-junction.md](pedestrians-traffic-cars-junction.md)).

## 2. Status

IN_PROGRESS, updated 2026-11-07T18:12:03+01:00.
- The four movement functions are ported as `src/runtime/traffic_route_mover.*`: pure steps on the driver record, the junction records and the route functions.
- The leave call 0x21d6b8 and the ground probe 0x20ae2c (the mover's hooks `leaveTraffic` and `snapTarget`) are ported as `src/runtime/traffic_leave_snap.*`; its ground ray cast and colour message are ported as `src/runtime/traffic_ground_world.*`.
- The junction records, their light cycle and the traffic lights: [pedestrians-traffic-cars-junction.md](pedestrians-traffic-cars-junction.md).
- The route functions `evaluate`, `ahead`, `nearest`, `length` and `signal` read the junctions' route paths (level roads and lanes); `ahead` and `nearest` are `src/runtime/traffic_route_functions.*` ([pedestrians-traffic-cars-driver.md](pedestrians-traffic-cars-driver.md)).
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

## 4. Stand-ins

- TRAFFIC-S11 Four cases where the original loops for ever, wraps a count or reads garbage throw instead: no enabled output among an input's choices, no lane joining the input and the chosen output, a 53rd queue entry, and leaving an empty queue (the count byte would wrap to 255). evidence: [traffic-ai-driver.md §2026-11-02T23:44:38](../../re/traffic-ai-driver.md#ts-20261102-234438)
- TRAFFIC-S23 The ground cast tries the collision entries in the order of the world, not in the order of the grid cells, and without its cap of 100 entries; composite box owners (flag 0x800) and a slanted segment are not modelled (the probe's segment is vertical). needs: TRAFFIC-29. evidence: [traffic-ground-cast.md §2026-11-07T12:11:38](../../re/traffic-ground-cast.md#ts-20261107-121138)
- TRAFFIC-S24 The colour message is sent to the mesh nodes the caller names; the port keeps the three bytes (+0x118..+0x11a) and does not paint the white vertex colours of the layers, and no draw reads the bytes. A colour table (item flag word set) is refused; no shipped item sets it. needs: TRAFFIC-30, TRAFFIC-2. evidence: [traffic-ground-cast.md §2026-11-07T12:11:38](../../re/traffic-ground-cast.md#ts-20261107-121138)

## 5. Open items

- [ ] TRAFFIC-29 — Read the order in which the grid gathers the entries and its cap of 100, the composite box owners and the walk for a slanted segment, and give `castTrafficGround` the original order. blocks: TRAFFIC-S23. evidence: [traffic-ground-cast.md §2026-11-07T12:11:38](../../re/traffic-ground-cast.md#ts-20261107-121138)
- [ ] TRAFFIC-30 — Read the node tree of a car, the draw's use of the node's tint bytes and the vertex paint of the colour message, and send the message to the car's mesh nodes after the probe. needs: TRAFFIC-2. blocks: TRAFFIC-S24. evidence: [traffic-ground-cast.md §2026-11-07T12:11:38](../../re/traffic-ground-cast.md#ts-20261107-121138)

## 6. Findings summary

- [traffic-ai-driver.md §2026-11-03T08:37:32](../../re/traffic-ai-driver.md#ts-20261103-083732): the leave call 0x21d6b8, the ground probe 0x20ae2c with the triangle weights 0x2b51b8 and the colour 0x221064.
- [traffic-ai-driver.md §2026-11-02T23:44:38](../../re/traffic-ai-driver.md#ts-20261102-234438): the route mover 0x2613ac step by step, the junction choice 0x2cb1e8 with the choice table and the lanes, the stop test 0x2ca504 with the lights 0x2786a8, the junction queue 0x22a080 and its callers.
- [traffic-ground-cast.md §2026-11-07T12:11:38](../../re/traffic-ground-cast.md#ts-20261107-121138): the vertical ground ray cast, the colour message on mesh nodes and the path name lookup 0x23895c.
- The junction findings: [pedestrians-traffic-cars-junction.md](pedestrians-traffic-cars-junction.md) section 6.

## 7. Code and tests

`src/runtime/traffic_route_mover.{hpp,cpp}` (`runTrafficRouteMover`, `chooseTrafficJunction`, `trafficStopAtLights`, `trafficLightControls`, `leaveTrafficJunctionQueue`, `TrafficJunctionRecord`, `TrafficJunctionWorld`, no SDL); CTest `traffic-route-mover` (`tests/traffic_route_mover_test.cpp`). `src/runtime/traffic_junction_fill.{hpp,cpp}` (`trafficJunctionFills`, `fillTrafficJunction`, `trafficJunctionChoices`, `trafficJunctionLanePath`, `trafficLanesCross`, `runTrafficJunctionLights`, no SDL); CTest `traffic-junction-fill`, `traffic-junction-fill-real-data` (`tests/traffic_junction_fill_test.cpp`, the second skips with 77 without the RomFS).

`src/runtime/traffic_ground_world.{hpp,cpp}` (`castTrafficGround`, `trafficGroundWorld`, `applyTrafficColourMessage`, no SDL); CTest `traffic-ground-world`, `traffic-ground-world-real-data` (`tests/traffic_ground_world_test.cpp`, the second skips with 77 without the RomFS). `src/assets/path_name_hash.{hpp,cpp}` (`pathNameHash`, `pathLookupName`); CTest `path-name-hash`.

`src/runtime/traffic_leave_snap.{hpp,cpp}` (`leaveTraffic`, `trafficGroundSegment`, `trafficTriangleWeights`, `trafficGroundColour`, `snapTrafficTarget`, `TrafficGroundWorld`, no SDL); CTest `traffic-leave-snap` (`tests/traffic_leave_snap_test.cpp`). `leaveTraffic` of the leave call 0x21d6b8 is a public method of `TrafficManager` (`src/runtime/traffic_manager.*`), because the fade bytes and `despawnCar` are private; `traffic_leave_snap` calls it.
