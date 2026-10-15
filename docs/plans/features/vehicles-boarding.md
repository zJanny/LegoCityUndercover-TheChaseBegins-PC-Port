# Vehicles: getting in and out

## 1. Scope

Entering and leaving vehicles on foot: the vehicle search, the jump in and jump out states and paths, the events, the exit candidates and the HUD while seated. Split from [vehicles.md](vehicles.md) (records, driving, physics, camera, traffic).

## 2. Status

IN_PROGRESS, updated 2026-10-15T08:45+02:00. The player gets into and out of an unlocked vehicle with the original's timing and paths (VEH-4, closed); the gates and probes are partly stand-ins (VEH-S8, VEH-S10).

## 3. Verified behaviour

- Boarding (`src/runtime/vehicle_boarding.*`, glue in `src/game_vehicle.cpp`, tests `vehicle-boarding`, `vehicle-boarding-real-data`; X is ignored while a director camera holds the player: `game-director-hold`, `game-director-release`): X press near a vehicle (search 0x225d94: 2.0 m on x and z to the entity box = root node of the first collision item, not locked, not disabled, no big carried object), Drive Jump In 0x11a with the jump animation (2200 ticks, path = line to the seat plus a 2.1 m sine arc, heading turned to the vehicle's), Drive 0x11c (player hidden at the body origin = node + CentreOfMass, `PlayerVehicleEntered` fired, `0x24255c` real, record +4 = 5 then +2), X again (not when `VehicleAllowJumpOut 0`): Jump Out 0x11b from the seat to the first free of eight exit candidates, `PlayerVehicleExiting`, record +4 = 3. Paths checked against 21 oracle positions per jump (police car, van_dulmen, worker), the box and CentreOfMass of four vehicles, the trigger `PlayerVehicleEntered` of `1-1_Police_Car` starting `1-1_Release_Car_Script`; evidence: [vehicles-boarding.md §2026-10-15T08:15:49](../../re/vehicles-boarding.md#ts-20261015-081549).

## 4. Stand-ins

- VEH-S8 The exit search takes the first of the original's eight candidates that has floor below it (the player's ground probe, depth 4.0): the line test 0x22b108 from the vehicle, the ray 0x21ab20 with depth 12.0 or 4.0 (0x229e6c) and the lift loop 0x2d8da0 are not ported. needs: VEH-19. evidence: [vehicles-boarding.md §2026-10-15T08:15:49](../../re/vehicles-boarding.md#ts-20261015-081549).
- VEH-S9 The seated player stays on the body origin of the unmoving vehicle (node + CentreOfMass): the first Drive update's body settling (0.1 to 0.2 lower) is missing. needs: VEH-6. evidence: [vehicles-boarding.md §2026-10-15T08:15:49](../../re/vehicles-boarding.md#ts-20261015-081549).
- VEH-S10 The enter and leave gates are taken as open: ctrl+0xe0+0x131 bit 2 / +0x130 bit 2, the global byte 0x27bb24, vehicle word +0xc bit 4 (leave refusal; the refusal by record float +0x31c above 0 is wired to the stuck timer of VEH-S3, which never runs because the stand-in body never tips over), the height rule of the search is ported unmeasured, and a press during the jump out is ignored (measured for the jump in). needs: VEH-19. evidence: [vehicles-boarding.md §2026-10-15T08:15:49](../../re/vehicles-boarding.md#ts-20261015-081549).

## 5. Open items

- [ ] VEH-17 — The X button prompt beside the player next to an enterable vehicle (drawn by 0x1835d0 at the player's node position while 0x225d94 finds a vehicle on foot; the call `0x22d7c0(9, 0)` at 0x183760 when entering becomes possible is read as the icon setup, not verified). evidence: [vehicles-boarding.md §2026-10-15T08:15:49](../../re/vehicles-boarding.md#ts-20261015-081549).
- [ ] VEH-18 — HUD while seated: the portrait shows the vehicle's icon and the hearts are blue and show the vehicle's health (seen in the oracle; the code was not read). needs: VEH-5. evidence: [vehicles-boarding.md §2026-10-15T08:15:49](../../re/vehicles-boarding.md#ts-20261015-081549).
- [ ] VEH-19 — Measure and port the unmodelled gates and probes of boarding (VEH-S8, VEH-S10): the exit line test, floor ray and lift loop, `0x229e6c`, the refusal inputs, the height rule and a press during the jump out. The player's controls during a director camera are done (SCRIPT-47). blocks: VEH-S8, VEH-S10. evidence: [vehicles-boarding.md §2026-10-15T08:15:49](../../re/vehicles-boarding.md#ts-20261015-081549).
- [ ] VEH-20 — Parts of the boarding states that only the original has: the vehicle's reaction to messages 83, 84 and 85, the sounds of the jumps, the second PlayerVehicleEntered call, the camera and HUD calls of the Drive begin (0x262188, 0x1ea27c, 0x1e7448), the role 3 variants of the jump. evidence: [vehicles-boarding.md §2026-10-15T08:15:49](../../re/vehicles-boarding.md#ts-20261015-081549).

## 6. Findings summary

[vehicles-boarding.md §2026-10-15T08:15:49](../../re/vehicles-boarding.md#ts-20261015-081549): the state tables of the driving states, the vehicle search rule (x and z distance 2.0 to the collision item box), the jump in / out paths and timing (2200 ticks, line plus 2.1 sine arc), the hidden player at the body origin (CentreOfMass), the events, the exit candidates and the record states, with the oracle numbers.

## 7. Code and tests

`src/runtime/vehicle_boarding.{hpp,cpp}` (search, boarding states, exit candidates; glue `src/game_vehicle.cpp`); CTest `vehicle-boarding`, `vehicle-boarding-real-data`.
