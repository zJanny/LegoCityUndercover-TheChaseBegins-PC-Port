# Vehicles: physics body

## 1. Scope

The physics body of a vehicle (entity+0x84, class at vtable 0x365910) and its rigid body: gravity, damping, orientation and angular motion, wheels, suspension, engine, gears, tyres, the body's collision with the level and the mode switch. Split from [vehicles.md](vehicles.md) (records, driving input and states, camera, natives). Driving collision against objects and other vehicles is VEH-7 there.

## 2. Status

IN_PROGRESS, updated 2026-10-15T13:41+02:00. The driven or moving vehicle is a rigid body with the original's gravity, damping, quaternion step and inverse inertia; it rests on four suspension rays with the original's spring and damper force, so it settles at the original's height, pitches and rolls with the ground and bounces over kerbs (VEH-30, VEH-31 closed). Speed, steering and heading still come from the kinematic body (VEH-S3, VEH-S33) until engine and tyres (VEH-32) are read.

## 3. Verified behaviour

- Rigid body (`src/runtime/vehicle_body.*`, CTest `vehicle-body`): gravity −39, damping `DampingLinear * (0.04 * speed)^2` and `DampingAngular`, 0x1f2eb4 (force, torque with the inverse world inertia, quarter-turn limit), 0x1f2c20, the quaternion step 0x1f2a50, force at a point 0x2d40f0, point velocity 0x341694; evidence [vehicles-physics.md §2026-10-15T13:41:02](../../re/vehicles-physics.md#ts-20261015-134102).
- Wheels (`src/runtime/vehicle_wheels.*`, `vehicle.cpp`; CTests `vehicle-wheels`, `vehicle-wheels-real-data`, `vehicle-real-data`): the per-wheel loader 0x243628 against the oracle wheel records of the trooper and the donut van; the ray of 0x33c718 (0.99 above the centre, reach max(travel − radius, 0.001) + radius, one-sided plane test of 0x155888); the force of 0x339030 against the traced wheel forces (within 0.01 N in clean updates); on the street of lvl_01 the trooper settles within 2 mm of the oracle's rest height and wheel distances.

## 4. Stand-ins

- VEH-S21 Gravity −39 and the damping formula are measured on the trooper and the van and used for every class; the origin of −39 (3 times the class `Gravity` −13) and of the factor 2 at body+0x84 (doubles the mass and the shock values) is not read. needs: VEH-33. evidence: [vehicles-physics.md §2026-10-15T12:21:00](../../re/vehicles-physics.md#ts-20261015-122100)
- VEH-S6 Parked vehicles stand at their placement height until they are driven; the original keeps a parked body where it was placed but not as the placement (0.04 to 0.11 away). needs: VEH-33. evidence: [vehicles-system.md §2026-10-14T23:59:31](../../re/vehicles-system.md#ts-20261014-235931)
- VEH-S7 Wheels are drawn at their class-file pose relative to the body (they tilt with it), without suspension travel, steering or spin, always attached (no LOD detach). needs: VEH-43. evidence: [vehicles-system.md §2026-10-14T23:59:31](../../re/vehicles-system.md#ts-20261014-235931)
- VEH-S30 The suspension settings 0.75 and 0.7 (0x376e58 +4, +8) and the two curve tables are the values read in the oracle; they are written at run time by code that is not read. needs: VEH-42. evidence: [vehicles-physics.md §2026-10-15T13:41:02](../../re/vehicles-physics.md#ts-20261015-134102)
- VEH-S31 The wheel ray hits the closest triangle of the player's query layer whose front faces the ray start (barycentric inside test); the triangle choice 0x23b5e8, the inside test of 0x155888 and the sweep against other bodies 0x1ea6a0 are not read. needs: VEH-41. evidence: [vehicles-physics.md §2026-10-15T13:41:02](../../re/vehicles-physics.md#ts-20261015-134102)
- VEH-S32 The inverse inertia is the one measured in the trooper and the donut van (rigid +0x190), the trooper's for every other class; the original computes it from the collision shape and `InertiaScale`. needs: VEH-42. evidence: [vehicles-physics.md §2026-10-15T13:41:02](../../re/vehicles-physics.md#ts-20261015-134102)
- VEH-S33 Engine, tyres and steering are the kinematic body (VEH-S3): before each step the horizontal velocity is set to its speed along its heading, after the step the body is turned about the vertical to its heading; height, pitch and roll are physics. No squat under acceleration or dive under braking (tyre forces). needs: VEH-32. evidence: [vehicles-physics.md §2026-10-15T13:41:02](../../re/vehicles-physics.md#ts-20261015-134102)
- VEH-S34 The collision pass and solver are replaced by a ground limit: after the step no wheel centre may lie below the ground a ray from 2 above it finds (8 below); the body is lifted and loses its downward velocity. needs: VEH-40. evidence: [vehicles-physics.md §2026-10-15T13:41:02](../../re/vehicles-physics.md#ts-20261015-134102)
- VEH-S35 The body is dynamic only while the vehicle is driven or still moving and freezes when the kinematic speed reaches 0 (no sleep rule, no mode switch 2 to 1); one world step per update with the update's dt. needs: VEH-33. evidence: [vehicles-physics.md §2026-10-15T12:21:00](../../re/vehicles-physics.md#ts-20261015-122100)

## 5. Open items

- [ ] VEH-6 — Physics body: parts 1 (rigid body, gravity, damping) and 2 (orientation, wheels, suspension: VEH-30, VEH-31) done; part 3 is VEH-32, VEH-33, VEH-40..43. Replaces VEH-S3 when done. needs: VEH-5. blocks: VEH-7, SCRIPT-200. evidence: [vehicles-physics.md §2026-10-15T13:41:02](../../re/vehicles-physics.md#ts-20261015-134102)
- [ ] VEH-32 — Engine, gears and tyres: the rest of 0x33aea8 from 0x33b564, the table at 0x376d74, tyre forces 0x339374 / 0x339538 and wheel spin 0x339a30, the per-wheel friction, brake and drift values of 0x243628; replaces the measured speed tables. Size L. needs: VEH-5. blocks: VEH-S3, VEH-S33. evidence: [vehicles-physics.md §2026-10-15T13:41:02](../../re/vehicles-physics.md#ts-20261015-134102)
- [ ] VEH-33 — Find where the rigid body mass (body+0x84 = 2 times `Mass`) and gravity (3 times `Gravity`) are set, the mode switch 2 to 1 (0x2308e4) and back to sleep, the world fixed step and whether 0x33aea8 runs per step; check the other classes. blocks: VEH-S21, VEH-S6, VEH-S35. evidence: [vehicles-physics.md §2026-10-15T12:21:00](../../re/vehicles-physics.md#ts-20261015-122100)
- [ ] VEH-40 — Collision pass and constraint solver 0x30cdf8: the body's `CollSpheres` against the level, so a body cannot sink into the street or kerbs. Size M. blocks: VEH-S34, VEH-7. evidence: [vehicles-physics.md §2026-10-15T13:41:02](../../re/vehicles-physics.md#ts-20261015-134102)
- [ ] VEH-41 — Wheel ray details: triangle choice 0x23b5e8 (the table at 0x376c68), the inside test of 0x155888, the sweep against bodies 0x1ea6a0 and the 5-unit body query 0x2e2f48. Size S. blocks: VEH-S31. evidence: [vehicles-physics.md §2026-10-15T13:41:02](../../re/vehicles-physics.md#ts-20261015-134102)
- [ ] VEH-42 — Inertia of the collision shape (virtual +0x20 called by 0x243f90, `InertiaScale`, 0x1f3124) and the writers of the suspension settings at 0x376e58 and of the two curve tables. Size S. blocks: VEH-S30, VEH-S32. evidence: [vehicles-physics.md §2026-10-15T13:41:02](../../re/vehicles-physics.md#ts-20261015-134102)
- [ ] VEH-43 — Wheel node pose: suspension travel, steer angle (w+0x84) and spin (w+0x80) on the wheel nodes (0x3396cc matrix, 0x33cb1c). Size S. blocks: VEH-S7. evidence: [vehicles-physics.md §2026-10-15T13:41:02](../../re/vehicles-physics.md#ts-20261015-134102)

## 6. Findings summary

[vehicles-physics.md §2026-10-15T13:41:02](../../re/vehicles-physics.md#ts-20261015-134102): matrix conventions, rigid body step with angular part, the class and per-wheel loaders, the wheel ray, the suspension force with its tables, and the traced trooper and van at rest and while driving.

[vehicles-physics.md §2026-10-15T12:21:00](../../re/vehicles-physics.md#ts-20261015-122100): rigid body layout, world step 0x30c768, gravity −39, damping `DampingLinear * (0.04 * speed)^2`, modes 2 and 1.

## 7. Code and tests

`src/runtime/vehicle_body.{hpp,cpp}` (rigid body; CTest `vehicle-body`), `src/runtime/vehicle_wheels.{hpp,cpp}` (wheels, suspension, the vehicle step and its stand-ins; CTests `vehicle-wheels`, `vehicle-wheels-real-data`), the shock reader in `src/runtime/vehicle.cpp` (CTest `vehicle-real-data`), glue in `src/game_vehicle.cpp` (CTests `game-vehicle-drive`, `game-van-escort`, `game-mission-1-1`).
