# Vehicles

## 1. Scope

Vehicle objects (class type 183, `levels/vehicles/vehicle_*`): creation and parked state, handling data, the driving update and physics, entering and leaving, the vehicle script natives, the vehicle camera, sounds and the events the mission scripts wait for. Needed for four of the eleven steps of mission 1-1 (get into `1-1_Police_Car`, drive, drive the donut van, reach the vehicle waypoints, jump out). Not covered: traffic and pursuit AI ([script-system-ai-navigation.md](script-system-ai-navigation.md)), the waypoint classes (agent work on objectives: [script-system-missions.md](script-system-missions.md)), race and RC vehicles, vehicle drops, helicopters and boats.

## 2. Status

IN_PROGRESS, updated 2026-10-14T23:59:31+02:00. Vehicles of a level are created as records, drawn with their wheels, collide with the player and take the five mission 1-1 flag natives; nobody drives yet (VEH-4 and later). [vehicles.md §2026-10-14T23:59:31](../../re/vehicles.md#ts-20261014-235931).

## 3. Verified behaviour

- Replicated (`src/runtime/vehicle.*`, test `vehicle-core`, `vehicle-real-data`): the parameter reader 0x23ed0c (start state, flags, health, camera values) against the oracle records of the three parked vehicles of lvl_01, the wheel children, the collision item of the model with mask 6 (oracle: 7 of 7 vehicle entities have such a grid entry) and the natives VehicleSetLocked, VehicleSetForcedStop, SetForcedStopOnMyVehicle, VehicleAllowJumpOut, VehicleSetUnshovable. Read from the code and the class files: the class descriptor and its functions, the parameter reader (`SpawnParked` sets state 4, `SpawnUnshovable` state 10, `VehicleType` indexes `tablescharacters` sheet 4), the six SCRIPT-36 natives of mission 1-1 (flag writes on the vehicle record), `0x24255c`, the vehicle search `0x225d94` and the enter / leave event `0x322aa4`; see [vehicles.md](../../re/vehicles.md#ts-20261014-224438).
- Oracle (GDB, no pokes): all 10 loaded vehicles run 0x1571dc / 0x16ce9c every update; parked vehicles are state 4 with owner flags 0x1340, traffic is state 1 with flags 0x1300 / 0x1301; X next to a parked vehicle raises event 200 → 0x322a04 → state 0x11a, about 11 updates later state 0x11c (ctrl+0x6c = 0x011c); with A held the vehicle goes to state 5, 0x29cd9c runs for it and it moved 5.7 m in 15 updates.

## 4. Stand-ins

- VEH-S1 The player is never in a vehicle: `0x24255c` is taken as 0 ([script-system-triggers.md](script-system-triggers.md) SCRIPT-S29). needs: VEH-4. evidence: [vehicles.md](../../re/vehicles.md#ts-20261014-224438).

- VEH-S2 Owner flags (0x1340 parked, 0x1300 traffic, bit 0 disabled) are not modelled: a vehicle collides while it is enabled at the level start (one enabled later has none: COLL-20); the meaning of 0x40 is unknown. needs: VEH-16, COLL-20. evidence: [vehicles.md §2026-10-14T23:59:31](../../re/vehicles.md#ts-20261014-235931).
- VEH-S3 (to introduce with VEH-5) The physics body is replaced by a simple kinematic stand-in (no suspension, tyres or gears). needs: VEH-6. evidence: [vehicles.md](../../re/vehicles.md#ts-20261014-224438).
- VEH-S4 Health byte +0x30e omits the +1 of global bit 16 (`0x2369a8(0x10)`, word 0x374724, clear in the oracle states). evidence: [vehicles.md §2026-10-14T23:59:31](../../re/vehicles.md#ts-20261014-235931).
- VEH-S5 The first update copies the requested state +4 to +2; the original transition (and the 10 to 4 change seen in the oracle) is not found. needs: VEH-13. evidence: [vehicles.md §2026-10-14T23:59:31](../../re/vehicles.md#ts-20261014-235931).
- VEH-S6 Vehicles stand at their placement height; the original body rests 0.04 to 0.11 away (physics). needs: VEH-6. evidence: [vehicles.md §2026-10-14T23:59:31](../../re/vehicles.md#ts-20261014-235931).
- VEH-S7 Wheels are drawn unrotated at their class-file pose, always attached (no LOD detach, no suspension). needs: VEH-6. evidence: [vehicles.md §2026-10-14T23:59:31](../../re/vehicles.md#ts-20261014-235931).

## 5. Open items

Ordered for mission 1-1; sizes are agent sessions (S below one, M one, L two or more). Each item needs the oracle for its numbers (fixed dt, traps) unless stated.

- [ ] VEH-4 — Enter and leave from the player side: prompt rule 0x1835d0 / 0x225d94, the button event 0x322aa4, driving states 0x11a / 0x11b / 0x11c, animation and node hand-over, `0x24255c` real, PlayerVehicleEntered and PlayerVehicleBoundEntered firing. Enter is traced (X, event 200, 0x322a04, states 0x11a then 0x11c); the leave path, the animation and the node hand-over still need traces (a state with the player seated: not saved yet). Size L. needs: VEH-1, VEH-3. blocks: SCRIPT-76, SCRIPT-77, VEH-5, VEH-8. evidence: [vehicles.md](../../re/vehicles.md#ts-20261014-224438).
- [ ] VEH-5 — Driving without physics detail: player input 0x29cd9c, input copy 0x23019c, vehicle state machine and transition (+2 / +4), forced stop braking, stuck rule 0x16ce9c, with the physics body as a stand-in (VEH-S3). Size M. needs: VEH-4. blocks: VEH-6, VEH-9. evidence: [vehicles.md](../../re/vehicles.md#ts-20261014-224438).
- [ ] VEH-6 — Physics body (0x33ce74 constructor, update 0x33aea8, integration 0x33b084, wheel ray casts and suspension, engine and gear model, tyre friction, gravity -13, damping) with the per-gear and per-wheel parameters whose hashes are unresolved; replace VEH-S3. Size L (two to three sessions: body and gravity; wheels and suspension; engine and tyres), measured with fixed-dt traces of a drive. needs: VEH-5. blocks: VEH-7. evidence: [vehicles.md](../../re/vehicles.md#ts-20261014-224438).
- [ ] VEH-7 — Driving collision: vehicle against the level (`Collision_Vehicle` meshes, mask 17 boxes), against placed objects and other vehicles, `BumpCollision`, damage; also the player's vehicle search box. Size L. needs: VEH-6, VEH-2. evidence: [vehicles.md](../../re/vehicles.md#ts-20261014-224438).
- [ ] VEH-8 — Vehicle camera: InVehicle preset (block 0x376fdc), `ATTR_Camera*` overrides, follow behind the vehicle, switch at enter and leave. Size M. needs: VEH-4, CAMERA-9. blocks: VEH-10. evidence: [vehicles.md](../../re/vehicles.md#ts-20261014-224438).
- [ ] VEH-9 — Jump out and the remaining natives for step 8: VehicleJumpOut (0x1da8d8, event 200), VehicleJumpIn (0x1cdec0), StopPlayerVehicle (0x27c040), VehicleSetTopSpeed and the vehicle part of PositionPlayer. Size M. needs: VEH-5. blocks: SCRIPT-36. evidence: [vehicles.md](../../re/vehicles.md#ts-20261014-224438).
- [ ] VEH-10 — Sounds and music: engine start / rev loop / stop, horn (table row +0x12), skid, impact, siren, `MusicTrack` and VehicleEnableDrivingMusic. Size M. needs: VEH-5, AUDIO-3. evidence: [vehicles.md](../../re/vehicles.md#ts-20261014-224438).
- [ ] VEH-11 — Vehicle waypoints in a vehicle (`map_getintovehiclewaypoint`, `map_vehiclewaypoint`: reach test while `0x24255c` is set) and the donut van scene of room33 (build-it parts, driveable van); linked to the waypoint work. Size M. needs: VEH-4, SCRIPT-76. evidence: [objective-hud.md](../../re/objective-hud.md#ts-20261014-212357).
- [ ] VEH-12 — Traffic and AI drivers (input provider 0x1d8df0, `settrafficmaxvehicles`, pursuit); not needed for mission 1-1. Size L. needs: VEH-6. evidence: [vehicles.md](../../re/vehicles.md#ts-20261014-224438).
- [ ] VEH-13 — Find what changes the state 10 of a `SpawnUnshovable` vehicle to 4 (user state `m11-2-patrol-car`: +2 = +4 = 4) and the meaning of the record states; candidates are the parked input provider 0x1d8df0 and the state functions. blocks: VEH-S5. evidence: [vehicles.md §2026-10-14T23:59:31](../../re/vehicles.md#ts-20261014-235931).
- [ ] VEH-14 — Vehicle class 243 (0xf3, `vehicle_aircraft_ufo`, accepted by the vehicle search) and type 0xb3 have no record or collision. evidence: [vehicles.md §2026-10-14T23:59:31](../../re/vehicles.md#ts-20261014-235931).
- [ ] VEH-15 — Wheel orientation of the left and right side, wheel node LOD attach (record +0x318 by camera distance) and the vehicle light children. needs: VEH-6. evidence: [vehicles.md §2026-10-14T23:59:31](../../re/vehicles.md#ts-20261014-235931).
- [ ] VEH-16 — Measure a player walking into a parked vehicle in the oracle (needs a settled camera) and find the meaning of owner flag 0x40. blocks: VEH-S2. evidence: [vehicles.md §2026-10-14T23:59:31](../../re/vehicles.md#ts-20261014-235931).

## 6. Findings summary

[vehicles.md §2026-10-14T22:44:38](../../re/vehicles.md#ts-20261014-224438): class 183 and its functions, the handling data in the class files (about 120 parameters each) and the vehicle table, the per-update chain 0x1571dc → 0x2424d0 → 0x29cd9c / 0x16ce9c → 0x23019c → physics body, the player enter / leave path (states 0x11a, 0x11b, 0x11c, event 0x322aa4, search 0x225d94), the natives of mission 1-1, and what stays unknown.

## 7. Code and tests

`src/runtime/vehicle.{hpp,cpp}` (records, parameter reader, wheels, flag natives; glue in `ScriptRunner` and `game_level.cpp`), vehicle collision in `src/assets/object_collision.cpp`; CTest `vehicle-core`, `vehicle-real-data`. Planned: `src/runtime/vehicle_physics.*`, tests `vehicle-core` and an oracle-trace replay.