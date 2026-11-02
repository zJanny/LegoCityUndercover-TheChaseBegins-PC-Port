# Traffic cars: the route mover and the junctions

## 1. Scope

How a traffic car moves in AI driver states 2 and 3:
- the route mover 0x2613ac (turn, move, target advance, speed);
- the stop test at junction lights 0x2ca504;
- the junction choice 0x2cb1e8 (output, lane, queue entry, request of state 3);
- the junction queue 0x22a080.

Not covered:
- the AI driver states, the state change and the post step that call these ([pedestrians-traffic-cars-driver.md](pedestrians-traffic-cars-driver.md));
- the manager, the junction set-up and the light cycle (TRAFFIC-4 in [pedestrians-traffic-cars.md](pedestrians-traffic-cars.md)).

## 2. Status

IN_PROGRESS, updated 2026-11-02T23:44:38+01:00.
- The four functions are ported as `src/runtime/traffic_route_mover.*`: pure steps on the driver record, the junction records and the route functions.
- They are not connected to the AI driver or the spawned cars yet (TRAFFIC-2 in [pedestrians-traffic-cars-driver.md](pedestrians-traffic-cars-driver.md)), so the cars still stand still in `chase-game`.

## 3. Verified behaviour

Read from the code (test `traffic-route-mover`), all from [traffic-ai-driver.md §2026-11-02T23:44:38](../../re/traffic-ai-driver.md#ts-20261102-234438):
- The mover:
  - the counter +0xae with the call of 0x20ae2c every sixth update;
  - the turn toward the target above speed 1.0, by dt x 4 or dt x 6 above 20, and the copy of the rows to +0x18;
  - the move of dt x speed along row 2;
  - the target advance within entity +0x5c: 0.4 per step, 0.15 on a route of length 4, past the length along row 2;
  - at the route end, the junction choice with the request of state 3, or the leave call without a junction;
  - the kinematic velocity, the engine blend body +0x27c, and the speed that moves 0.18 of the way to +0x70 (it takes +0x70 within 0.001).
- The junction choice: no choice before length − 0.5; the random enabled output (random % (count x 8) >> 3, drawn again while the route bit is clear), the lane joining the input and output, the nearest sample on the lane, the queue entry and bit 3.
- The stop test: the lights of the junction's pairs that list the car's input road stop it unless their pair is +0x300 with +0x308 set. A car on a lane never stops for lights.
- The queue: the search from the last entry, the swap with the last entry, the count that drops even when the car is not found, and bit 3 cleared.

## 4. Stand-ins

- TRAFFIC-S11 Four cases where the original loops for ever, wraps a count or reads garbage throw instead: no enabled output among an input's choices, no lane joining the input and the chosen output, a 53rd queue entry, and leaving an empty queue (the count byte would wrap to 255). evidence: [traffic-ai-driver.md §2026-11-02T23:44:38](../../re/traffic-ai-driver.md#ts-20261102-234438)
- TRAFFIC-S12 The junction choice takes the nearest lane sample from the caller's route functions (slot 3 of the route's type); the original calls the type 0 entry for every type. All shipped traffic routes are type 0; the type of a lane path is not read. needs: TRAFFIC-18. evidence: [traffic-ai-driver.md §2026-11-02T23:44:38](../../re/traffic-ai-driver.md#ts-20261102-234438)

## 5. Open items

- [ ] TRAFFIC-17 — The leave call of a car whose route leads into no junction (hook `leaveTraffic`) and the ground snap of the target (hook `snapTarget`) are not read or ported. blocks: TRAFFIC-2. evidence: [traffic-ai-driver.md §2026-11-02T23:44:38](../../re/traffic-ai-driver.md#ts-20261102-234438)
- [ ] TRAFFIC-18 — Fill `TrafficJunctionRecord` from the level: the choice table +0x418 / +0x310, the lane ends, the lane paths with their length and type, the light pairs +0x2c0 / +0x304 with the lights' route words +0xf0, and +0x300 / +0x308 from the light update. needs: TRAFFIC-4. blocks: TRAFFIC-S12, TRAFFIC-2. evidence: [traffic-ai-driver.md §2026-11-02T23:44:38](../../re/traffic-ai-driver.md#ts-20261102-234438)

## 6. Findings summary

- [traffic-ai-driver.md §2026-11-02T23:44:38](../../re/traffic-ai-driver.md#ts-20261102-234438): the route mover 0x2613ac step by step, the junction choice 0x2cb1e8 with the choice table and the lanes, the stop test 0x2ca504 with the lights 0x2786a8, the junction queue 0x22a080 and its callers.

## 7. Code and tests

`src/runtime/traffic_route_mover.{hpp,cpp}` (`runTrafficRouteMover`, `chooseTrafficJunction`, `trafficStopAtLights`, `trafficLightControls`, `leaveTrafficJunctionQueue`, `TrafficJunctionRecord`, `TrafficJunctionWorld`, no SDL); CTest `traffic-route-mover` (`tests/traffic_route_mover_test.cpp`).
