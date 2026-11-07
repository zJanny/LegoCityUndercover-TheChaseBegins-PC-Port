# Traffic AI driver input

## 1. Scope

The AI driver input 0x26d934 of the spawned traffic cars, the driver record and its state change, and the vehicle side of a spawn. The manager, routes and spawn are in [pedestrians-traffic-cars.md](pedestrians-traffic-cars.md).

## 2. Status

IN_PROGRESS. The AI driver is ported as `ai_driver_input` (record, input provider with every state, state change 0x1cb148, post step 0x1bc79c) and is not wired to the cars. The route mover and junction functions it calls are ported, not connected ([pedestrians-traffic-cars-movement.md](pedestrians-traffic-cars-movement.md)); the steering and the other calls are not (TRAFFIC-8, TRAFFIC-9, TRAFFIC-22).

## 3. Verified behaviour

- Read from the code, not ported: the vehicle update uses 0x26d934 for a car whose driver record has +0xaf bit 0, 0x29cd9c for the player's car and nothing for the others (0x1d8df0 is empty).
- Read from the code (test `ai-driver-input`), all from [traffic-ai-driver.md §2026-10-25T12:26:09](../../re/traffic-ai-driver.md#ts-20261025-122609):
  - the driver record of 0x1796e8: zero-filled, +0x74 = `TrafficHeightOffRoad`, +0x18 = the node matrix, +0xaf bit 0, kinematic body;
  - the state change 0x1cb148: +0x78 takes the request +0x79 with its leave and enter steps, and after a spawn the state goes 0 → 1 → 2, one update per step (the servant measured the same on a street with traffic after mission 1-1);
  - the input provider 0x26d934: the stop rule of states 2 and 3, state 5 steering 8 m ahead along the route, state 8 steering back to the path and putting the car back on it when it faces away; states 4, 6 and 7 give no input;
  - the post step 0x1bc79c: the junction step of state 3, the recovery 4 → 8, the break 6 → 7, the keep counter +0xad;
  - traffic cars are kinematic bodies (flags 0x102) except in states 4 and 6 and after a contact.

## 4. Stand-ins

- TRAFFIC-S5 The route type functions (table 0x3581d4: point, distance ahead, nearest sample), the route length, its signal object and the path follower 0x2bf2ec reach `ai_driver_input` as callbacks (`AiRouteFunctions`, labelled in the code) until the route port `traffic_route.*` is connected. needs: TRAFFIC-2. evidence: [traffic-ai-driver.md §2026-10-25T12:26:09](../../re/traffic-ai-driver.md#ts-20261025-122609)
- TRAFFIC-S7 The AI driver is not connected to the cars, so a spawned car stands still. The driver bytes go 0/1, 1/2, 2/2 one update each (state change 0x1cb148); the grace byte +0xad is set to random % 60 + 30 by the 0 -> 1 step and counts down one per update as if the car's node were always hidden (0x23de70 taken as true; the original keeps the byte at 30 or more while the node is not hidden); the speed +0x68 is 10 (8.6 to 11.3 measured); the record state is set to 1 at once and the car is placed without the kinematic switch 0x22a270 (the port makes no physics body). The state-1 draws of 0x2345dc are not made. needs: TRAFFIC-2, TRAFFIC-11. evidence: [traffic.md §2026-10-25T12:35:43](../../re/traffic.md#ts-20261025-123543)

## 5. Open items

- [ ] TRAFFIC-2 — Connect the AI driver (`ai_driver_input`) to the path follower 0x2bf2ec and the route type functions (table 0x3581d4) of the route port, and port the functions it calls. needs: TRAFFIC-8, TRAFFIC-9, TRAFFIC-22, TRAFFIC-18; the routes, pairs and bits come from `TrafficSystem::routes()` and `TrafficManager::routeTable()`. blocks: VEH-12, TRAFFIC-S5, TRAFFIC-S7, TRAFFIC-11. evidence: [traffic-ai-driver.md §2026-10-25T12:26:09](../../re/traffic-ai-driver.md#ts-20261025-122609)
- [ ] TRAFFIC-8 — The steering of states 5 and 8: the controller 0x27872c (it also reads +0x7a / +0x7c and clears +0xa4), the step 0x29cc38 and the gear speed 0x20362c. blocks: TRAFFIC-2. evidence: [traffic-ai-driver.md §2026-10-25T12:26:09](../../re/traffic-ai-driver.md#ts-20261025-122609)
- [ ] TRAFFIC-9 — The other calls of the state change and the post step: the list of 0x230dbc (+0xaf bit 4), the state 6 calls (0x229be4, 0x23cee8, 0x22a124 with 0x11f), 0x1bc660, 0x2308e4, 0x20ae2c, the lights 0x278b78, 0x23de70, 0x2c87a8, the writers of +0xaf bits 1 and 2, and the other creators of driver records (stores to +0x2a4 at 0x1afe50, 0x262794, 0x290de8, 0x2a60c4). blocks: TRAFFIC-2. evidence: [traffic-ai-driver.md §2026-10-25T12:26:09](../../re/traffic-ai-driver.md#ts-20261025-122609)
- [ ] TRAFFIC-11 — The vehicle side of a spawn in the vehicle code: the port sets the record state 1 and makes no physics body; the kinematic switch 0x22a270 → 0x213c74 and the car messages (read in [traffic-ai-driver.md §2026-10-25T12:26:09](../../re/traffic-ai-driver.md#ts-20261025-122609)) are to be ported into the vehicle code. needs: VEH-91, TRAFFIC-2, PREGEN-7. blocks: TRAFFIC-S7. evidence: [traffic.md §2026-10-25T12:35:43](../../re/traffic.md#ts-20261025-123543)

## 6. Findings summary

- [traffic-ai-driver.md §2026-10-25T12:26:09](../../re/traffic-ai-driver.md#ts-20261025-122609): the driver record, the writers of +0x78 / +0x79, the state change 0x1cb148, all states of 0x26d934, the post step 0x1bc79c, the kinematic switch.

## 7. Code and tests

`src/runtime/ai_driver_input.{hpp,cpp}` (`createAiDriverRecord`, `runAiDriverInput`, `runAiDriverStateChange`, `runAiDriverPostStep`, no SDL); CTest `ai-driver-input` (`tests/ai_driver_input_test.cpp`).
