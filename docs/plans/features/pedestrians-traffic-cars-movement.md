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

IN_PROGRESS, updated 2026-11-07T11:05:36+01:00.
- The four movement functions are ported as `src/runtime/traffic_route_mover.*`: pure steps on the driver record, the junction records and the route functions.
- The leave call 0x21d6b8 and the ground probe 0x20ae2c (the mover's hooks `leaveTraffic` and `snapTarget`) are ported as `src/runtime/traffic_leave_snap.*`.
- The junction records are filled from the level and the light cycle is ported as a pure step (`src/runtime/traffic_junction_fill.*`).
- None of this is connected to the AI driver or the spawned cars yet (TRAFFIC-19, TRAFFIC-2 in [pedestrians-traffic-cars-driver.md](pedestrians-traffic-cars-driver.md)), so the cars still stand still in `chase-game`.

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

Read from the code (tests `traffic-junction-fill`, `traffic-junction-fill-real-data`, `traffic-route-setup`), all from [traffic-junction.md §2026-11-03T08:53:43](../../re/traffic-junction.md#ts-20261103-085343):
- The ends of the input and output paths (end point, the point before, the x/z travel direction) and the sides 1 / 2 of parallel ends (0x159ab8).
- The choice table: no turning back (dot −0.85), the side rule, one input with one output always joined.
- The lanes: four points, the Hermite points 0x20b0c0 at 1/3 and 2/3, type 0; the conflict pairs of lanes whose same-index segments come within 0.1 in x and z (0x20af64; [traffic-junction.md §2026-11-07T11:05:36](../../re/traffic-junction.md#ts-20261107-110536)).
- The light pairs `TrafficLight01..08` with their `SlaveLight`, the lights' routes `Path01..04`, and the start at pair 0, green, 6 s.
- The light cycle: green 6 s, yellow 2 s, red 3 s, the next pair green once the queue is empty; nothing with one pair.
- Shipped levels: lanes, conflicts, light pairs and sided ends per level, and lvl_01 `Junction1` in detail; names without a level path are skipped (the route list lost one bogus route per level).

## 4. Stand-ins

- TRAFFIC-S11 Four cases where the original loops for ever, wraps a count or reads garbage throw instead: no enabled output among an input's choices, no lane joining the input and the chosen output, a 53rd queue entry, and leaving an empty queue (the count byte would wrap to 255). evidence: [traffic-ai-driver.md §2026-11-02T23:44:38](../../re/traffic-ai-driver.md#ts-20261102-234438)
- TRAFFIC-S13 The junction fill refuses a junction path of fewer than two points, a 65th conflict pair (the original writes over its stack) and a lane id equal to a level path's name; lanes are named by port ids, not record addresses; paths and lights are searched in all loaded levels, not in the object's scene; path names are hashed whole, not after the last `.`; a lane tested against one with fewer points is refused (the original reads past its points). needs: TRAFFIC-19, TRAFFIC-21. evidence: [traffic-junction.md §2026-11-07T11:05:36](../../re/traffic-junction.md#ts-20261107-110536)
- TRAFFIC-S16 The ground probe's ray cast, the triangle data and the receiver of the colour message are supplied by the caller; the port has no ground ray cast and does not know what command 1 does to the node. needs: TRAFFIC-22. evidence: [traffic-ai-driver.md §2026-11-03T08:37:32](../../re/traffic-ai-driver.md#ts-20261103-083732)

## 5. Open items

- [ ] TRAFFIC-19 — Wire the junction records into the traffic system: keep the fills at the scene enter, run the light cycle for each junction every update, give the route functions the lane paths; which junctions update and in what order against the traffic update is not read. blocks: TRAFFIC-2, TRAFFIC-S13. evidence: [traffic-junction.md §2026-11-03T08:53:43](../../re/traffic-junction.md#ts-20261103-085343)
- [ ] TRAFFIC-20 — The look of the traffic lights: the parts green, yellow and red of `props/prop_trafficlight`, the part changes of the set-up (with its slot index quirk) and of the cycle, and the light class's own update are not ported. evidence: [traffic-junction.md §2026-11-03T08:53:43](../../re/traffic-junction.md#ts-20261103-085343)
- [ ] TRAFFIC-21 — The original looks a path up by the part of its name after the last `.`, but the port hashes the whole name in its three users: the junction fill (`traffic_junction_fill.cpp`), the DCam paths (`dcam.cpp`) and the pedestrian path count (`pedestrian_system.cpp`); one shared helper in `chase_assets` would close all three. blocks: TRAFFIC-S13. evidence: [traffic-junction.md §2026-11-07T11:05:36](../../re/traffic-junction.md#ts-20261107-110536)
- [ ] TRAFFIC-22 — Read the ground ray cast and the colour message on the car's node, and supply them to `TrafficGroundWorld`. blocks: TRAFFIC-2, TRAFFIC-S16. evidence: [traffic-ai-driver.md §2026-11-03T08:37:32](../../re/traffic-ai-driver.md#ts-20261103-083732)

## 6. Findings summary

- [traffic-ai-driver.md §2026-11-03T08:37:32](../../re/traffic-ai-driver.md#ts-20261103-083732): the leave call 0x21d6b8, the ground probe 0x20ae2c with the triangle weights 0x2b51b8 and the colour 0x221064.
- [traffic-ai-driver.md §2026-11-02T23:44:38](../../re/traffic-ai-driver.md#ts-20261102-234438): the route mover 0x2613ac step by step, the junction choice 0x2cb1e8 with the choice table and the lanes, the stop test 0x2ca504 with the lights 0x2786a8, the junction queue 0x22a080 and its callers.
- [traffic-junction.md §2026-11-07T11:05:36](../../re/traffic-junction.md#ts-20261107-110536): the conflict test compares same-index segments (0x1cad58), the re-pinned conflict counts, and the path name split 0x23895c.
- [traffic-junction.md §2026-11-03T08:53:43](../../re/traffic-junction.md#ts-20261103-085343): the junction and light classes, the set-up 0x1ca5a4 (ends, sides 0x159ab8, choices, Hermite lanes 0x20b0c0, conflicts 0x20af64, light pairs), the light set-up 0x19ff70, the light cycle 0x1d8924 and its times, the shipped junctions.

## 7. Code and tests

`src/runtime/traffic_route_mover.{hpp,cpp}` (`runTrafficRouteMover`, `chooseTrafficJunction`, `trafficStopAtLights`, `trafficLightControls`, `leaveTrafficJunctionQueue`, `TrafficJunctionRecord`, `TrafficJunctionWorld`, no SDL); CTest `traffic-route-mover` (`tests/traffic_route_mover_test.cpp`). `src/runtime/traffic_junction_fill.{hpp,cpp}` (`trafficJunctionFills`, `fillTrafficJunction`, `trafficJunctionChoices`, `trafficJunctionLanePath`, `trafficLanesCross`, `runTrafficJunctionLights`, no SDL); CTest `traffic-junction-fill`, `traffic-junction-fill-real-data` (`tests/traffic_junction_fill_test.cpp`, the second skips with 77 without the RomFS).

`src/runtime/traffic_route_mover.{hpp,cpp}` (`runTrafficRouteMover`, `chooseTrafficJunction`, `trafficStopAtLights`, `trafficLightControls`, `leaveTrafficJunctionQueue`, `TrafficJunctionRecord`, `TrafficJunctionWorld`, no SDL); CTest `traffic-route-mover` (`tests/traffic_route_mover_test.cpp`).

`src/runtime/traffic_leave_snap.{hpp,cpp}` (`leaveTraffic`, `trafficGroundSegment`, `trafficTriangleWeights`, `trafficGroundColour`, `snapTrafficTarget`, `TrafficGroundWorld`, no SDL); CTest `traffic-leave-snap` (`tests/traffic_leave_snap_test.cpp`). `leaveTraffic` of the leave call 0x21d6b8 is a public method of `TrafficManager` (`src/runtime/traffic_manager.*`), because the fade bytes and `despawnCar` are private; `traffic_leave_snap` calls it.
