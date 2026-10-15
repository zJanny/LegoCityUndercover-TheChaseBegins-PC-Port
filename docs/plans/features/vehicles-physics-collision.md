# Vehicles: physics body collision

## 1. Scope

The vehicle body's collision in the physics world: the floor collision pass 0x1c54f8 on the floor graph, the sleep rule, and the Bullet contacts with the level's static boxes (compound child box, box-box detector, manifolds, sequential impulse solver). Split from [vehicles-physics.md](vehicles-physics.md) (rigid body, wheels, engine, tyres). Contacts with other vehicles, kinematic objects and placed objects are VEH-7 in [vehicles.md](vehicles.md).

## 2. Status

IN_PROGRESS, updated 2026-10-15T19:13+02:00. The body collides with the level floor through the original's floor pass and sleeps by the original's rule (VEH-40, VEH-53 closed), and collides with the 217 static Bullet boxes of the level through the original's Bullet 2.78 contacts and solver (VEH-60, VEH-63 closed). Still missing: the sphere sweep and second cell search of the floor pass (VEH-61), the compound shape's builder (VEH-62), details of the Bullet contacts (VEH-70).

## 3. Verified behaviour

- Floor collision and sleep (`src/runtime/floor_walk.*`, `src/runtime/vehicle_collision.*`, floor graph in `src/assets/model.cpp`; CTests `floor-graph-real-data`, `vehicle-collision`, `vehicle-collision-real-data`, `vehicle-drive-real-data`): the floor graph 0x11fd20 and the walk 0x23b5e8 (13 traced walks); the pass 0x1c54f8 with 0x15c4a4, 0x19c0a8, 0x1b79ac, the tracker 0x20c0e4 and the search 0x1edaac; a traced landing at 24.6 m/s replays to 0.1 mm and 0.001 m/s; a 1.9 m drop bounces to the traced lowest point and sleeps in the traced update (0x30cdf8); at most 2 world steps per update; every `vehicle-drive` oracle sample also holds on the street of lvl_01; evidence [vehicles-physics-collision.md §2026-10-15T18:07:23](../../re/vehicles-physics-collision.md#ts-20261015-180723).
- Static Bullet boxes (`src/runtime/vehicle_box_contact.*`; CTests `vehicle-box-contact`, `vehicle-box-contact-real-data`): the 217 boxes of 0x28b070, the compound algorithm 0x32367c, dBoxBox2 0x2cf72c, the manifold result 0x2f0888 and refresh 0x1e518c, the solver 0x331814 after the floor pass; one traced solve matches to 1e-4, one traced collision detection to 2e-5; from 4.6 m before the obstacle the trooper hits it in the traced update within 3 mm and 0.02 m/s and keeps pushing against it as traced; evidence: [vehicles-physics-collision.md §2026-10-15T19:04:12](../../re/vehicles-physics-collision.md#ts-20261015-190412).

## 4. Stand-ins

- VEH-S50 The box of the body's collision points and the Bullet child box (the AABB of the compound shape, its centre the child offset) is the one read in the oracle for the trooper and the donut van, the trooper's for every other class; the shape's builder is not read. needs: VEH-62. evidence: [vehicles-physics-collision.md §2026-10-15T18:07:23](../../re/vehicles-physics-collision.md#ts-20261015-180723)
- VEH-S51 A point more than 0.1 below a floor cell with flag bit 0 keeps that cell; the second cell search 0x164328 is not read. needs: VEH-61. evidence: [vehicles-physics-collision.md §2026-10-15T18:07:23](../../re/vehicles-physics-collision.md#ts-20261015-180723)
- VEH-S52 The floor pass leaves out the sphere sweep of the CollSpheres against wall edges (0x1b6ef8, 0x16788c) and the contact message of 0x181e68 (a reply 2000 drops a contact). needs: VEH-61. evidence: [vehicles-physics-collision.md §2026-10-15T18:07:23](../../re/vehicles-physics-collision.md#ts-20261015-180723)
- VEH-S53 The sleep values 0.8 m/s, 1 rad/s and 0.75 s and the world step record (1/30 s, at most 2 steps) are the values read in the oracle; their writers are not read. needs: VEH-33. evidence: [vehicles-physics-collision.md §2026-10-15T18:07:23](../../re/vehicles-physics-collision.md#ts-20261015-180723)
- VEH-S60 Manifolds of boxes that start touching in the same step are created in box order (the dispatcher's pair order is not ported). needs: VEH-70. evidence: [vehicles-physics-collision.md §2026-10-15T19:04:12](../../re/vehicles-physics-collision.md#ts-20261015-190412)
- [x] VEH-S61 — removed 2026-10-15: the vehicle body's Bullet friction is the constant 0.5 that 0x243f90 stores (it reads `BodyFriction` and ignores it); the boxes' restitution 0 makes the vehicle's restitution irrelevant for box contacts. evidence: [vehicles-physics-collision.md §2026-10-15T19:12:47](../../re/vehicles-physics-collision.md#ts-20261015-191247)
- VEH-S62 A sleeping vehicle takes no box contacts and no solver impulses, as measured (its solver deltas stay unchanged); the code that keeps the solver off it is not found. needs: VEH-70. evidence: [vehicles-physics-collision.md §2026-10-15T19:04:12](../../re/vehicles-physics-collision.md#ts-20261015-190412)

## 5. Open items

- [ ] VEH-61 — The sphere sweep of 0x1c54f8 (0x1b6ef8, 0x16788c, sphere set-up 0x33a4fc), the second cell search 0x164328 and the message of 0x181e68. Size M. blocks: VEH-S51, VEH-S52. evidence: [vehicles-physics-collision.md §2026-10-15T18:07:23](../../re/vehicles-physics-collision.md#ts-20261015-180723)
- [ ] VEH-62 — Where the compound shape and its box come from: the builder is 0x2184a4 (class `CollisionBound` records, otherwise the entity box, a compound when the box centre differs from `CentreOfMass`); the trooper's child box is exactly its entity box (`modelCollisionBox`) minus `CentreOfMass`, the donut van's reaches 0.139 lower than its entity box (not explained: the scale argument, the mesh shape branch for node type 0x37674c, or more children). Size S. blocks: VEH-S50. evidence: [vehicles-physics-collision.md §2026-10-15T19:12:47](../../re/vehicles-physics-collision.md#ts-20261015-191247)
- [ ] VEH-70 — Rest of the Bullet contacts: the dispatcher's pair order, what keeps the solver off a sleeping vehicle, the contact callback 0x3865a4 (flag word +0xb4) and the float 0x376c5c. Size S. blocks: VEH-S60, VEH-S62. evidence: [vehicles-physics-collision.md §2026-10-15T19:04:12](../../re/vehicles-physics-collision.md#ts-20261015-190412)

## 6. Findings summary

[vehicles-physics-collision.md §2026-10-15T19:12:47](../../re/vehicles-physics-collision.md#ts-20261015-191247): the vehicle friction is the constant 0.5 of 0x243f90; the shape builder 0x2184a4; the trooper's child box is its entity box minus `CentreOfMass`, the van's is not.

[vehicles-physics-collision.md §2026-10-15T19:04:12](../../re/vehicles-physics-collision.md#ts-20261015-190412): Bullet 2.78 contacts of the vehicle with the static boxes: the boxes of 0x28b070, the compound and box-box algorithms, dBoxBox2, the manifold rules, the solver and its rows, the step order, traced solve, collision detection and hit.

[vehicles-physics-collision.md §2026-10-15T18:07:23](../../re/vehicles-physics-collision.md#ts-20261015-180723): the physics world is Bullet (layouts, solver info, the 217 static boxes), the world step with at most 2 steps, the floor graph, the walk, the floor collision pass with its impulses, the tracker and the sleep rule; landing and drop traces.

## 7. Code and tests

`src/runtime/floor_walk.{hpp,cpp}` and the floor graph in `src/assets/model.cpp` (CTest `floor-graph-real-data`), `src/runtime/vehicle_collision.{hpp,cpp}` (floor pass, impulses, tracker; CTests `vehicle-collision`, `vehicle-collision-real-data`), `src/runtime/vehicle_box_contact.{hpp,cpp}` (static Bullet boxes, box-box contacts, manifolds, solver; CTests `vehicle-box-contact`, `vehicle-box-contact-real-data`), the step order in `src/runtime/vehicle_wheels.cpp`, glue in `src/game_vehicle.cpp` and `src/game_level.cpp`.
