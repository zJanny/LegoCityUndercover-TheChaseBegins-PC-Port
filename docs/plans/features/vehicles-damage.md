# Vehicles: contact damage and the break

## 1. Scope

What a hit does to the vehicle the player drives: the contact message 0x80000002 and the damage message 0 of the vehicle handler 0x1797c4, the hearts (+0x30d) and health points (+0x30c) of the vehicle record, the cool-downs, the brick burst of a contact, and the break of the car (message 1, 0x16cccc, the wreck state). Split from [vehicles.md](vehicles.md) (VEH-7, VEH-93, VEH-99). The physics contacts that send the message are in [vehicles-physics-collision.md](vehicles-physics-collision.md).

## 2. Status

IN_PROGRESS, updated 2026-10-16T10:37+02:00. The player's vehicle takes damage from contacts as the original does: the closing speed of every Bullet manifold point (static boxes and other vehicles) and of every floor-pass contact is computed (0x19b5c4), a contact above 20 m/s costs one heart per 0.5 s (a hit of 27.9 m/s took the oracle trooper from 5 to 4 hearts, as the port does), the HUD shows the hearts. The break of the car, its trigger in the oracle (a water test 98 updates after the impact that sends a type 6 message with all hearts) and the way back are not ported.

## 3. Verified behaviour

- Damage rules (`src/runtime/vehicle_damage.*`, tests `vehicle-damage`, `vehicle-box-contact`): speed thresholds 10 / 20 / 60, `(min(speed, 60) - 20) x 0.025 x 5` split into whole hearts and a fraction, the points take `floor(fraction x 100)`, one heart per message that cannot take the points, at most one heart for a contact message, 0.5 s cool-down, 1.0 s brick cool-down, the hearts are the vehicle table's health (trooper 5). Values are from the disassembly; the first hit of the oracle run (hearts 5 to 4, +0x304 = 0.5, +0x308 = 1.0, health byte back at 100) agrees ([vehicles-damage.md §2026-10-16T10:21:53](../../re/vehicles-damage.md#ts-20261016-102153)).
- The pair rule of the callback (flag word bit 0 / 1 for a driven body, the wheel rule, `BumpCollision` friction 0.2 and restitution 1.1; `applyPairContactRule`, test `vehicle-damage`) replaces the earlier flag 2 stand-in; the recorded pair hit (`vehicle-world-real-data`) still holds.
- The processed-callback hook of Bullet's manifold refresh (`refreshManifold` observer, 0x1e518c) and the floor-pass message before the impulse (0x1c5cec) deliver the messages in the original's order.

## 4. Stand-ins

- VEH-S90 The damage check 0x25207c (a timer of the game state that refuses damage) is taken as false. needs: VEH-115. evidence: [vehicles-damage.md §2026-10-16T10:21:53](../../re/vehicles-damage.md#ts-20261016-102153)
- VEH-S91 A vehicle whose hearts reach 0 is marked destroyed (no further damage) but is not broken: the model, the wheels and the player stay as they are, because the player's way out of the Drive state and the wreck state are not read. needs: VEH-110. evidence: [vehicles-damage.md §2026-10-16T10:21:53](../../re/vehicles-damage.md#ts-20261016-102153)
- VEH-S93 The pair contact rule of 0x271764 takes the body flags +0xc4 bit 7 and bit 2 as clear and counts a body without wheel states as having its wheels on the ground (`applyPairContactRule`). needs: VEH-115. evidence: [vehicles-damage.md §2026-10-16T10:21:53](../../re/vehicles-damage.md#ts-20261016-102153)
- VEH-S92 The brick burst and the smoke effects of a hit are log lines only. needs: VEH-112. evidence: [vehicles-damage.md §2026-10-16T10:21:53](../../re/vehicles-damage.md#ts-20261016-102153)

## 5. Open items

- [ ] VEH-110 — The break: class handler 0x159bd0 message 1 -> 0x16cccc (body leaves the world, state 6, node flag 0x20, launch vectors of the wheels at +0x320, children hidden, effect 0x23aebc), the wreck state 6 (what reads the launch vectors, the wheel pieces), the player's reaction (message 10 to the attached driver; on foot beside the wreck in the oracle) and the return of the car after 30 s with 5 hearts. Size L. blocks: VEH-S91, VEH-7. evidence: [vehicles-damage.md §2026-10-16T10:21:53](../../re/vehicles-damage.md#ts-20261016-102153)
- [ ] VEH-111 — The water test that destroys a vehicle: 0x23019c -> 0x2d8d08 (volumes of the table 0x593b60, probe point along the up axis, factor 0.75 or 0.35) sends the type 6 message with all hearts; the oracle car broke so 98 updates after it stopped at the static box at (846.0, 17.26, 1078.9) although both tests are static: why? Measurement (state `drv-police-car-seated`, the same run: A held, pad (100, 0) for 24 updates, into the box, A held on): per update from the impact to the break the record R = [E+0x7c] u16 +2 and +4, float +0x31c, the node matrix [E+0x3c] -> 0x250810 +0x10..+0x18 (up vector) and +0x30..+0x38, and the rigid orientation; at the break and 10 updates before: the table 0x593b60 (u32 count at +0x24, pointers from +0x28), per volume V the byte [[V+0x88]+10], the type byte [V+0x88]+9, the centre +0x10 and size +0x1c..+0x24, and the node of [V+0x20] -> 0x23c188 + 0x24. Also the port has no such volumes (they come from level objects) and the stuck rule 0x16ce9c with its real upright test is kept off (a tipped car could not be left). Size M. blocks: VEH-110, VEH-56. evidence: [vehicles-damage.md §2026-10-16T10:21:53](../../re/vehicles-damage.md#ts-20261016-102153)
- [ ] VEH-112 — The brick burst 0x229fb8 (colours `DebrisColour`, `DebrisColour2`, count 1 to 3) the smoke effects 0x22a124 (ids 0x11e, 0x11f) and the model flash 0x230ae8 of a hit (the counter +0x30f restarts at 0 and counts to 0xf over 15 updates in the oracle; its writer is not read). Size M. blocks: VEH-S92. evidence: [vehicles-damage.md §2026-10-16T10:21:53](../../re/vehicles-damage.md#ts-20261016-102153)
- [ ] VEH-113 — The vehicle bump pass 0x2d4af0 (speed above 20 against neighbouring dynamic vehicles: group test 0x2d4224, swept sphere, box sweep 0x1bfd24, message 0x271764 with a synthesised point, impulses 0x1bfff8), not ported; the contacts of vehicles in the Bullet world already send the message. Size M. blocks: VEH-7. evidence: [vehicles-damage.md §2026-10-16T10:21:53](../../re/vehicles-damage.md#ts-20261016-102153)
- [ ] VEH-115 — The rest of the contact message 0x179ca4 and of message 0: record +0x311 bit 3 (sound 95, message 190), the target logic for state 2 and 3 (+0x2a4), the impact sound at +0x28, the timer 0x25207c, "7-9_RexCar", the flash 0x230ae8 and the revive rule's users. Size S. blocks: VEH-S90. evidence: [vehicles-damage.md §2026-10-16T10:21:53](../../re/vehicles-damage.md#ts-20261016-102153)

## 6. Findings summary

[vehicles-damage.md §2026-10-16T10:21:53](../../re/vehicles-damage.md#ts-20261016-102153): health bytes and their start values, the static initialiser of the cool-downs (0x3745a4), where the contact message comes from (gContactProcessedCallback 0x271764, the floor pass 0x181e68), the closing speed, the contact message tail (bricks, damage message), message 0 and its cases, the destroy call 0x23cd80 and the break 0x16cccc, the oracle run (hearts, timers, the break 98 updates later, the wreck, the return after 30 s), the bump pass details.

## 7. Code and tests

`src/runtime/vehicle_damage.{hpp,cpp}` (speeds, contact reply, damage, timers; glue `LevelSession::vehicleImpact` in `src/game_vehicle.cpp`); observers in `src/runtime/vehicle_box_contact.*` (`ManifoldPointObserver`), `src/runtime/vehicle_collision.*` (`FloorContactObserver`) and `src/runtime/vehicle_world.*` (`VehicleImpactSink`); CTest `vehicle-damage`, `vehicle-box-contact` (callback count 8 on the recorded manifold).
