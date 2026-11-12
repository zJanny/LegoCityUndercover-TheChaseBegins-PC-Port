# Traffic car vehicle body

## 1. Scope

The physics body of a spawned traffic car while the AI driver moves it: the kinematic switch, the body's place in the physics world and what the driver writes into the vehicle. The driver itself is in [pedestrians-traffic-cars-driver.md](pedestrians-traffic-cars-driver.md), the vehicle physics in [vehicles-physics-collision.md](vehicles-physics-collision.md).

## 2. Status

IN_PROGRESS, updated 2026-11-12T08:20:34+01:00. Each traffic car has its kinematic vehicle body (`traffic_car_body`, TRAFFIC-11 done) and it is in the vehicle world step: the player's car collides with it (TRAFFIC-39 done), the contact message makes a hit car dynamic and request driver state 4, the driver's record fields are copied to and from the vehicle record and the kinematic wheel spin shows on the wheel nodes (TRAFFIC-40 done). Not read: the engine value, the sender flag of the immediate switch, Bullet's kinematic velocity (TRAFFIC-50); the hit is not measured in the original (TRAFFIC-51); the glue's own choices are TRAFFIC-S29, TRAFFIC-S37 and TRAFFIC-S38.

## 3. Verified behaviour

- Read from the code (tests `traffic-car-body`, `entity-update-list`), from [traffic-car-body.md §2026-11-12T00:10:13](../../re/traffic-car-body.md#ts-20261112-001013): the kinematic switch and body commands, the world membership on the join and leave messages, the node copy before the world step, the kinematic body step, the spawn's floor cell and the update list rule.

## 4. Stand-ins

- TRAFFIC-S28 What is left of the vehicle side of a driven car: the mover's engine value (body +0x264, +0x26c, +0x27c) is not applied. needs: TRAFFIC-50. evidence: [traffic-car-body.md §2026-11-12T08:20:03](../../re/traffic-car-body.md#ts-20261112-082003)
- TRAFFIC-S29 Facts of the traffic bodies the glue chooses: layer class 0 and room type 0 for every car (as lvl_01's ten cars), the room test run every update instead of at the enable and disable, the body step only for a body in the world, body flag +0xc4 bit 0 clear, the wheel value w+0x60 as 0, the body link's add and remove left out at the load, entity +0x10 bit 5 always clear (`beforeWorldStep` passes only the disabled bit, `trafficBodyFollowsNode` gets flag20 false), and a floor cell tracker of its own next to the one of the dynamic vehicle body. needs: TRAFFIC-41. evidence: [traffic-car-body.md §2026-11-12T00:10:13](../../re/traffic-car-body.md#ts-20261112-001013)
- TRAFFIC-S37 Facts of the contact the port chooses: the game clock is seconds from the update count at 30 updates per second (contact time, state 4), the other body's flag that picks the immediate dynamic switch is taken as clear (the queued switch runs), the solver uses the velocity the mover stored for a kinematic body, and a kinematic body counts the wheels on the ground it had at the spawn. needs: TRAFFIC-50. evidence: [traffic-car-body.md §2026-11-12T08:20:03](../../re/traffic-car-body.md#ts-20261112-082003)
- TRAFFIC-S38 Facts of a hit car and its wheels the port chooses: a hit car runs as the port's dynamic vehicle body (like a chased car) started from its node heading with the mover's velocity, mass scale 1 and the driver's inputs flagged as driven (for the pair contact rule only; a hit traffic car is never taken for the player's vehicle), its stuck timer is not run, and the sign of the wheel turn about x in the kinematic pose is that of the dynamic pose. needs: TRAFFIC-50, TRAFFIC-51. evidence: [traffic-car-body.md §2026-11-12T08:20:03](../../re/traffic-car-body.md#ts-20261112-082003)

## 5. Open items

- [ ] TRAFFIC-41 — Read the removal's disable down to the update list test, the layer class and room of the other levels' cars, the body step out of the world, the body link's order and flag +0xc4 bit 0; measure body +0x8c right after a spawn. blocks: TRAFFIC-S29. evidence: [traffic-car-body.md §2026-11-12T00:10:13](../../re/traffic-car-body.md#ts-20261112-001013)
- [ ] TRAFFIC-50 — Read the rest of the vehicle side of a driven car: the engine value the mover stores (body +0x264, +0x26c, +0x27c) and its use, whose body has the flag that picks the immediate dynamic switch, the velocity Bullet gives a kinematic body in the solver, the unit of the game clock, the handler's return value 2000 and the record +0x311 bit 3 effect. blocks: TRAFFIC-S28, TRAFFIC-S37, TRAFFIC-S38. evidence: [traffic-car-body.md §2026-11-12T08:20:03](../../re/traffic-car-body.md#ts-20261112-082003)
- [ ] TRAFFIC-51 — Measure a hit in the original: the player's car driving into a traffic car on a street (the car's driver state 4 then 8, the body flags and velocity after the hit, the player's car's reaction) and compare with the port's run. Needs a state: the player in a car on a street with traffic cars ahead. blocks: TRAFFIC-S38. evidence: [traffic-car-body.md §2026-11-12T08:20:03](../../re/traffic-car-body.md#ts-20261112-082003)

## 6. Findings summary

- [traffic-car-body.md §2026-11-12T08:20:03](../../re/traffic-car-body.md#ts-20261112-082003): the contact message branch of a traffic car's driver, the immediate dynamic switch, what a kinematic traffic body collides with, the port's world step entries.
- [traffic-car-body.md §2026-11-12T00:10:13](../../re/traffic-car-body.md#ts-20261112-001013): the kinematic body, the update list and its messages, the readers of the body's floor cell.

## 7. Code and tests

`src/runtime/traffic_car_body.{hpp,cpp}` (`TrafficBodyHooks`, `TrafficCarBodies`, `switchTrafficBodyKinematic`, `stepTrafficBody`, no SDL); CTest `traffic-car-body` (`tests/traffic_car_body_test.cpp`). Glue `src/game_traffic_bodies.cpp`; CTest `game-traffic-bodies`. `src/runtime/traffic_car_contact_world.{hpp,cpp}` (the kinematic rigid body, `trafficCarContact`, the record field copies; CTest `traffic-car-contact-world`, `tests/traffic_car_contact_world_test.cpp`), glue `src/game_traffic_contact.cpp`, the kinematic entries in `src/runtime/vehicle_world.*`, `kinematicWheelPoseMatrix` in `src/runtime/vehicle_wheel_pose.*`. The driver calls it through `TrafficDriveStep::bodies` in `src/runtime/traffic_driver_connect.{hpp,cpp}`. The entity update list is in [vehicles-physics-collision.md](vehicles-physics-collision.md).
