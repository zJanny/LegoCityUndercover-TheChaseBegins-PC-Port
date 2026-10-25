# Traffic manager and the AI driver input

## 1. Scope

The traffic manager 0x527410 and the AI driver input of the spawned cars. The manager spawns and removes the level's `Traffic%02u` vehicles around the player, keeps the vehicle limit (`Traffic_MaxVehicles`, SetTrafficMaxVehicles) and fades cars in and out at `Traffic_FadeBound` boxes. The AI driver input 0x26d934 drives a spawned car along its route. Not covered: pedestrians (the pedestrian manager 0x51b33c, [pedestrians-traffic-pedestrians.md](pedestrians-traffic-pedestrians.md)), the pregen file format ([pedestrians-traffic.md](pedestrians-traffic.md)), the vehicle physics body ([vehicles-physics.md](vehicles-physics.md)), pursuit.

## 2. Status

IN_PROGRESS, updated 2026-10-25T12:28+01:00. The rules of the manager are read: level load, room set-up and prefill, update, spawn, fade, SetTrafficMaxVehicles. They are ported as `TrafficManager`. How the vehicle update picks its input provider is read. The routes (junction paths, EnableTrafficPath, the path follower, the route type function, the junction pairs and test) are ported as `traffic_route.*` (TRAFFIC-5). The AI driver is read and ported as `ai_driver_input`: its record, the input provider 0x26d934 with every state, the state change 0x1cb148 and the traffic post step 0x1bc79c. Not ported yet: the functions those call (route mover, steering controller, junction choice; TRAFFIC-7..9). None of them is wired into `chase-game` (TRAFFIC-1).

## 3. Verified behaviour

- Read from the code (test `traffic-manager`), all from [traffic.md §2026-10-20T11:18:14](../../re/traffic.md#ts-20261020-111814):
  - the level load: the vehicle limit is min(`Traffic_MaxVehicles` default 5, car count); the spawn order is shuffled with the game's RNG; every car starts disabled with fade 255;
  - SetTrafficMaxVehicles: the argument is truncated and compared unsigned with the count;
  - the update: removal of every car while traffic is off; one spawn per update while below the limit when the countdown reaches 0, with countdown (live + live / 2) x 3; one removal per update (room, driver state, fade and distance rules); cars with owner flag 0x20 are kept;
  - the spawn: 10 tries in the leaf, the route bit, the room test, the LOD distance band (0.7 x near to 1.75 x far, 0 to far in the prefill), 16 m route spacing in x and z;
  - the 64 prefill updates of the room set-up;
  - the fade inside a `Traffic_FadeBound` box.
- Read from the code (tests `traffic-route`, `traffic-route-real-data`), all from [traffic.md §2026-10-25T12:19:38](../../re/traffic.md#ts-20261025-121938):
  - the route list: input then output paths of the junctions within 9000 of the origin (at most 32), each new one with its bit set; EnableTrafficPath and its pending list, replayed by the room set-up;
  - the route → junction pairs 0x2dfc84 and the driver's junction 0x20ff40;
  - the path follower 0x2bf2ec (nearest enabled point of the cell, below 9999, dot above 0.5 by its float bits) and the route type function (table 0x3581d4: type 0 0x1c17c0, type 1 0x1b33fc);
  - the junction test 0x2cdde4;
  - shipped levels: every traffic point lies on an open type 0 junction path; junction, route and path counts pinned.
- Read from the code, not ported: the vehicle update uses 0x26d934 for a car whose driver record has +0xaf bit 0, 0x29cd9c for the player's car and nothing for the others (0x1d8df0 is empty).
- Read from the code (test `ai-driver-input`), all from [traffic.md §2026-10-25T12:26:09](../../re/traffic.md#ts-20261025-122609):
  - the driver record of 0x1796e8: zero-filled, +0x74 = `TrafficHeightOffRoad`, +0x18 = the node matrix, +0xaf bit 0, kinematic body;
  - the state change 0x1cb148: +0x78 takes the request +0x79 with its leave and enter steps, and after a spawn the state goes 0 → 1 → 2, one update per step (the servant measured the same on a street with traffic after mission 1-1);
  - the input provider 0x26d934: the stop rule of states 2 and 3, state 5 steering 8 m ahead along the route, state 8 steering back to the path and putting the car back on it when it faces away; states 4, 6 and 7 give no input;
  - the post step 0x1bc79c: the junction step of state 3, the recovery 4 → 8, the break 6 → 7, the keep counter +0xad;
  - traffic cars are kinematic bodies (flags 0x102) except in states 4 and 6 and after a contact.

## 4. Stand-ins

- TRAFFIC-S1 The spawn cell, the route count, the LOD bands and the per-car facts (room tests, driver bytes, despawn radius, LOD class) are supplied by the caller of `TrafficManager`. The pregen decoder now exists (`src/assets/pregen_traffic.*`: `parsePregenSpawns`, `findPregenSpawnCell`, [pregen-traffic.md](../../re/pregen-traffic.md)), but the manager is not fed from it yet; the LOD table 0x529e30 and the radius table `*0x375f40` are not read. needs: TRAFFIC-1. evidence: [traffic.md §2026-10-20T11:18:14](../../re/traffic.md#ts-20261020-111814)
- TRAFFIC-S2 Two cases the original leaves unread are refused by the port's own choice: an empty leaf makes the spawn fail (the original would compute random % 0), and a LOD class outside 0..2 throws (the original's -1 indexes the table one entry low). needs: TRAFFIC-3. evidence: [traffic.md §2026-10-20T11:18:14](../../re/traffic.md#ts-20261020-111814)
- TRAFFIC-S3 A route missing from the route list counts as disabled in the follower; the original tests the stored +0x24 word of the path record. Every shipped traffic point lies on a junction path (`traffic-route-real-data`). evidence: [traffic.md §2026-10-25T12:19:38](../../re/traffic.md#ts-20261025-121938)
- TRAFFIC-S4 The route list +0xc4, the pending route list and the two route → junction lists refuse a 129th entry, and a route type above 1 throws; the original writes past the lists and reads words after its table. needs: TRAFFIC-4. evidence: [traffic.md §2026-10-25T12:19:38](../../re/traffic.md#ts-20261025-121938)
- TRAFFIC-S5 The route type functions (table 0x3581d4: point, distance ahead, nearest sample), the route length, its signal object and the path follower 0x2bf2ec reach `ai_driver_input` as callbacks (`AiRouteFunctions`, labelled in the code) until the route port `traffic_route.*` is connected. needs: TRAFFIC-2. evidence: [traffic.md §2026-10-25T12:26:09](../../re/traffic.md#ts-20261025-122609)

## 5. Open items

- [ ] TRAFFIC-1 — Wire the manager into `chase-game`: the pregen traffic data (quadtree leaf, routes), the route set-up 0x2df574 / 0x2dfc84, the values of the LOD table 0x529e30 and of the radius table `*0x375f40` (an oracle read), the car messages 0x80000005 / 6 / 7, and the natives SetTrafficMaxVehicles and EnableTrafficAndPedestrians routed to the manager. needs: TRAFFIC-2, TRAFFIC-5, PREGEN-1. blocks: SCRIPT-230, TRAFFIC-S1. evidence: [traffic.md §2026-10-20T11:18:14](../../re/traffic.md#ts-20261020-111814)
- [ ] TRAFFIC-2 — Connect the AI driver (`ai_driver_input`) to the path follower 0x2bf2ec and the route type functions (table 0x3581d4) of the route port, and port the functions it calls. needs: TRAFFIC-5, TRAFFIC-7, TRAFFIC-8, TRAFFIC-9. blocks: TRAFFIC-1, VEH-12, TRAFFIC-S5. evidence: [traffic.md §2026-10-25T12:26:09](../../re/traffic.md#ts-20261025-122609)
- [ ] TRAFFIC-3 — Read the rest of the manager: the layer table of 0x1ffbd8, the LOD class -1 of 0x28781c, and a leaf without points. blocks: TRAFFIC-S2. evidence: [traffic.md §2026-10-20T11:18:14](../../re/traffic.md#ts-20261020-111814)
- [ ] TRAFFIC-4 — The junction class `props/traffic_junction` (set-up 0x1ca5a4, update 0x1d8924): its lanes, conflict pairs, the lane list +0x284 that 0x2cdde4 reads, the traffic lights, and who moves a car's route onto a lane and on. blocks: TRAFFIC-S4. evidence: [traffic.md §2026-10-25T12:19:38](../../re/traffic.md#ts-20261025-121938)
- [ ] TRAFFIC-5 — Wire `traffic_route.*`: the junctions of the level, the manager's route bits taken from `TrafficRouteTable`, the native EnableTrafficPath (40 calls, 2 levels) routed to `enableTrafficPath`, the follower and the junction test called by the AI driver. blocks: TRAFFIC-1, TRAFFIC-2. uses: EnableTrafficPath. evidence: [traffic.md §2026-10-25T12:19:38](../../re/traffic.md#ts-20261025-121938)
- [ ] TRAFFIC-6 — `TrafficSpawnPoint::speed` and `TrafficCar::cruiseSpeed` hold the path parameter (point +0x20, driver +0x64), not a speed; rename them. evidence: [traffic.md §2026-10-25T12:19:38](../../re/traffic.md#ts-20261025-121938)
- [ ] TRAFFIC-7 — The kinematic movement of states 2 and 3: the route mover 0x2613ac (with its own request of state 3), the stop test 0x2ca504, the junction choice 0x2cb1e8 and the junction queue 0x22a080. blocks: TRAFFIC-2. evidence: [traffic.md §2026-10-25T12:26:09](../../re/traffic.md#ts-20261025-122609)
- [ ] TRAFFIC-8 — The steering of states 5 and 8: the controller 0x27872c (it also reads +0x7a / +0x7c and clears +0xa4), the step 0x29cc38 and the gear speed 0x20362c. blocks: TRAFFIC-2. evidence: [traffic.md §2026-10-25T12:26:09](../../re/traffic.md#ts-20261025-122609)
- [ ] TRAFFIC-9 — The other calls of the state change and the post step: the list of 0x230dbc (+0xaf bit 4), the state 6 calls (0x229be4, 0x23cee8, 0x22a124 with 0x11f), 0x1bc660, 0x2308e4, 0x20ae2c, the lights 0x278b78, 0x23de70, 0x2c87a8, the writers of +0xaf bits 1 and 2, and the other creators of driver records (stores to +0x2a4 at 0x1afe50, 0x262794, 0x290de8, 0x2a60c4). blocks: TRAFFIC-2. evidence: [traffic.md §2026-10-25T12:26:09](../../re/traffic.md#ts-20261025-122609)

## 6. Findings summary

- [traffic.md §2026-10-20T11:18:14](../../re/traffic.md#ts-20261020-111814): which object is the traffic manager (0x527410, not 0x51b33c), its layout, level load, room set-up and prefill, the update with its spawn and removal rules, the spawn 0x2def8c, the fade 0x2df8b8, SetTrafficMaxVehicles, the choice of input provider in 0x1571dc, and the states of the AI driver 0x26d934 read so far.
- [traffic.md §2026-10-25T12:19:38](../../re/traffic.md#ts-20261025-121938): the junctions as owners of the routes, the route list, EnableTrafficPath and the pending list, the route → junction pairs, the path follower 0x2bf2ec, the route type function and the junction test 0x2cdde4.
- [traffic.md §2026-10-25T12:26:09](../../re/traffic.md#ts-20261025-122609): the driver record, the writers of +0x78 / +0x79, the state change 0x1cb148, all states of 0x26d934, the post step 0x1bc79c, the kinematic switch 0x22a270 → 0x213c74 (flags 0x102), the car messages 0x80000003..7.

## 7. Code and tests

`src/runtime/traffic_manager.{hpp,cpp}` (`TrafficManager`, `runSetTrafficMaxVehicles`, no SDL); CTest `traffic-manager` (`tests/traffic_manager_test.cpp`). `src/runtime/traffic_route.{hpp,cpp}` (route list, `enableTrafficPath`, `findTrafficRoutePoint`, `sampleTrafficRoute`, `trafficJunctionAllowsCar`, no SDL); CTest `traffic-route`, `traffic-route-real-data` (`tests/traffic_route_test.cpp`, the second skips with 77 without the RomFS). `src/runtime/ai_driver_input.{hpp,cpp}` (`createAiDriverRecord`, `runAiDriverInput`, `runAiDriverStateChange`, `runAiDriverPostStep`, no SDL); CTest `ai-driver-input` (`tests/ai_driver_input_test.cpp`). There is no glue in `game.cpp`.
