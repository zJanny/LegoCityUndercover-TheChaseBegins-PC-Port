# Traffic car vehicle body

## 1. Scope

The physics body of a spawned traffic car while the AI driver moves it: the kinematic switch, the body's place in the physics world and what the driver writes into the vehicle. The driver itself is in [pedestrians-traffic-cars-driver.md](pedestrians-traffic-cars-driver.md), the vehicle physics in [vehicles-physics-collision.md](vehicles-physics-collision.md).

## 2. Status

IN_PROGRESS, updated 2026-11-12T01:15:05+01:00. Each traffic car has its kinematic vehicle body (`traffic_car_body`, TRAFFIC-11 done): the spawn seeds its floor cell, it joins the physics world with the entity, takes the node before each world step and spins its wheel values. Nothing collides with it yet (TRAFFIC-39), and the rest of the vehicle side is a stand-in (TRAFFIC-S28, TRAFFIC-S29).

## 3. Verified behaviour

- Read from the code (tests `traffic-car-body`, `entity-update-list`), from [traffic-car-body.md §2026-11-12T00:10:13](../../re/traffic-car-body.md#ts-20261112-001013): the kinematic switch and body commands, the world membership on the join and leave messages, the node copy before the world step, the kinematic body step, the spawn's floor cell and the update list rule.

## 4. Stand-ins

- TRAFFIC-S28 What is left of the vehicle side of a driven car: the kinematic body is not in the port's vehicle world step, so nothing collides with it; the mover's engine value is not applied; the driver keeps the vehicle record fields it writes; the wheel spin is not shown. needs: TRAFFIC-39, TRAFFIC-40. evidence: [traffic-car-body.md §2026-11-12T00:10:13](../../re/traffic-car-body.md#ts-20261112-001013)
- TRAFFIC-S29 Facts of the traffic bodies the glue chooses: layer class 0 and room type 0 for every car (as lvl_01's ten cars), the room test run every update instead of at the enable and disable, the body step only for a body in the world, body flag +0xc4 bit 0 clear, the wheel value w+0x60 as 0, the body link's add and remove left out at the load, entity +0x10 bit 5 always clear (`beforeWorldStep` passes only the disabled bit, `trafficBodyFollowsNode` gets flag20 false), and a floor cell tracker of its own next to the one of the dynamic vehicle body. needs: TRAFFIC-41. evidence: [traffic-car-body.md §2026-11-12T00:10:13](../../re/traffic-car-body.md#ts-20261112-001013)

## 5. Open items

- [ ] TRAFFIC-39 — Put the kinematic traffic bodies into the vehicle world step so that the player's car collides with them, and send the contact message that makes a hit car dynamic, with `TrafficCarBodies::World::copyInputs` connected to `copyVehicleRecordInputs`. blocks: TRAFFIC-S28. evidence: [traffic-car-body.md §2026-11-12T00:10:13](../../re/traffic-car-body.md#ts-20261112-001013)
- [ ] TRAFFIC-40 — Move the fields the driver writes into the vehicle record (brake, hand brake, hearts), apply the mover's engine value and show the kinematic wheel spin on the wheel nodes. blocks: TRAFFIC-S28. evidence: [traffic-car-body.md §2026-11-12T00:10:13](../../re/traffic-car-body.md#ts-20261112-001013)
- [ ] TRAFFIC-41 — Read the removal's disable down to the update list test, the layer class and room of the other levels' cars, the body step out of the world, the body link's order and flag +0xc4 bit 0; measure body +0x8c right after a spawn. blocks: TRAFFIC-S29. evidence: [traffic-car-body.md §2026-11-12T00:10:13](../../re/traffic-car-body.md#ts-20261112-001013)

## 6. Findings summary

- [traffic-car-body.md §2026-11-12T00:10:13](../../re/traffic-car-body.md#ts-20261112-001013): the kinematic body, the update list and its messages, the readers of the body's floor cell.

## 7. Code and tests

`src/runtime/traffic_car_body.{hpp,cpp}` (`TrafficBodyHooks`, `TrafficCarBodies`, `switchTrafficBodyKinematic`, `stepTrafficBody`, no SDL); CTest `traffic-car-body` (`tests/traffic_car_body_test.cpp`). Glue `src/game_traffic_bodies.cpp`; CTest `game-traffic-bodies`. The driver calls it through `TrafficDriveStep::bodies` in `src/runtime/traffic_driver_connect.{hpp,cpp}`. The entity update list is in [vehicles-physics-collision.md](vehicles-physics-collision.md).
