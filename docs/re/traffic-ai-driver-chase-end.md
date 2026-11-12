# Traffic: the end of a car chase

Subtopic of [traffic-ai-driver.md](traffic-ai-driver.md): how a chased car's driver reaches state 6 and fires VehicleChaseEnded. Feature file: [script-system-missions-1-4.md](../plans/features/script-system-missions-1-4.md).

## Current state

- The only writer of the requested driver state 6 is the vehicle handler 0x19293c: a hard contact of the player's vehicle on a car in state 5 sends the car a damage message, and the hit on the last heart requests state 6. Entering state 6 fires VehicleChaseEnded at the car. [§2026-11-08T12:09:08](#ts-20261108-120908)
- Port: `vehicle_chase_events` turns the contact into the request of state 6; the stolen car's driver is stepped and enters state 6 at its next update ([traffic-driver-states.md §2026-11-12T00:08:58](traffic-driver-states.md#ts-20261112-000858)).
- The contact message also scales the car's body velocity by a factor of the closing speed (0.9 below 12 m/s, 0.8 below 24, 0.7 below 36, else 0.4); the damage message is delivered at once; the player's police car takes the stolen car through five hits (hearts 5 to 1, 128 updates apart) to the chase end and the chaser's jump out in `chase-game`. [§2026-11-12T10:03:42+01:00](#ts-20261112-100342)

<a id="ts-20261108-120908"></a>

## 2026-11-08T12:09:08+01:00 — The end of a car chase: the vehicle handler 0x19293c, the request of state 6 and VehicleChaseEnded (llvm-objdump of game.arm.elf; no emulator, no REA session)

Evidence: round 20261108-traffic, agent 20261108-traffic-C, 2026-11-08; llvm-objdump (armv6k, vfp2) of 0x19293c..0x192d48 and 0x1cb148..0x1cb6d4. Pinned by CTest `vehicle-chase-events` and, with the game data, `vehicle-chase-events-real-data`.

**Who requests state 6.** Of the writers of driver +0x79 listed in [§2026-10-25T12:26:09](traffic-ai-driver.md#ts-20261025-122609) the only one that writes 6 is 0x192b38, inside the message handler 0x19293c of a car that has a driver record (`vehicle +0x7c`, `+0x2a4`). The handler wraps the base handler 0x1797c4 (return 1 = consumed, return 0 = the base handler goes on).

**Contact message 0x80000002 (0x1929c0..0x192a60).** Nothing happens unless the driver +0x78 is 5 and +0xac is 0, the message's other entity is not 0 and equals `0x24255c(*0x3772b8)` (the vehicle the player drives), that entity's word +0xc has bit 0x10 clear (not destroyed) and the closing speed 0x19b5c4 is above 3.0 (`vcmpe` 3.0 against the speed, `bhs` leaves). Then the car sends itself message 0 (0x244fc8) with word +0xc = 1 (hearts), +0x10 = 0, byte +0x14 = 11 and bytes +0x15 and +0x17 = 0, and gives its body a velocity change from 0x24092c / 0x248850 / 0x229920 whose size depends on speed / 12 (0.9, 0.8, 0.7 or 0.4; not read further).

**Damage message 0 (0x192ae0..0x192b88).** With driver +0x79 = 6 already: return 1. With vehicle +0x30d (hearts) = 0: return 0. When the hearts are above the message's word +0xc, or the message's type byte +0x17 is 6, or the driver's +0x78 is not 5: kind 11 sets driver +0xac = 128 (the input provider counts it down), return 0, and the base handler applies the damage. Otherwise (hearts at most the word, type not 6, state 5): driver +0x79 = 6, hearts +0x30d = 0, then the compare of the car's name hash with `7-9_RexCar` (0x24da28 at 0x192b3c). Only for that car (the `bne` at 0x192b50 and `beq` at 0x192b64 skip the rest) the record r4 is reloaded at 0x192b68 as the record of the player's vehicle, 0x22b224 runs on it when its record state is 6 or 7, and its +0x311 |= 4 (Invulnerable) is set. Every other car returns 1 at 0x192b88 with the flags unchanged.

**Entering state 6 (0x1cb52c..0x1cb5ac).** 0x22a270(entity, 0) (not kinematic), 0x229be4(entity, 1) (forced stop, record +0x310 = 1), `0x23cee8(*0x375fe0, entity, entity, 0, 1)` (event VehicleChaseEnded with the car as owner), a second `0x23cee8` with the object `0x23c188(*(*0x376c78 + 0x28))` and r3 = 255 only when `*0x376c78` is not 0, then the smoke 0x22a124(entity, 0x11f). In state 6 the update calls only 0x23019c and, once the body speed 0x229948 is at most 0.1 and +0xa4 is at least 90, requests 7 (the destroy), as read before.

**In the port.** `src/runtime/vehicle_chase_events.*` holds the three steps; `LevelSession::chasedCarImpact` runs them for a vehicle with a chased driver record when its body touches the player's vehicle, sets the driver state, starts the smoke and fires `vehiclechaseended` at the car, which starts `1-4_Car_Crash` (test with the game data). The car is not a moving body yet and its driver is not stepped, so the state 5 the contact needs is never reached in play (SCRIPT-422).

Port choices, all under SCRIPT-S336: `chasedCarImpact` sets the driver state to 6 and fires the event at once, while the original only requests +0x79 = 6 at 0x192b38 and the state change 0x1cb148 enters it on the next driver step; for the chased car the base handler 0x1797c4 is not run after the contact message although 0x192d48 returns 0. The unread parts of the handler are SCRIPT-S336 too; SCRIPT-S337 belongs to the cutscene timing in [scripts-missions-1-4-spawners.md §2026-11-08T12:09:48+01:00](scripts-missions-1-4-spawners.md#ts-20261108-120948).

Unknowns:
- The velocity change on the contact (0x24092c, 0x248850, 0x229920), whether message 0 is delivered at once or queued by 0x244fc8, the global at 0x3772b8 and 0x376c78 (SCRIPT-S336).
- How many hearts `1-4_Stolen_Vehicle` has (the vehicle table row and `HealthOverride`) and so how many hits the chase takes (SCRIPT-415).

<a id="ts-20261112-100342"></a>

## 2026-11-12T10:03:42+01:00 — The contact's velocity change, the delivery of the damage message and the chase end in a real run (llvm-objdump of game.arm.elf, `chase-game` runs; no REA session, no emulator)

Evidence: round 20261112-trafficfeed, agent 20261112-trafficfeed-A, 2026-11-12; llvm-objdump (armv6k, vfp2) of 0x1929c0..0x192adc with the literals 0x192cf8..0x192d0c (read as floats from the code image), 0x24092c, 0x229920, 0x248850, 0x244fc8, 0x252c14. Pinned by CTests `chased-car-contact` (made-up cars) and `game-chase-end-1-4` (slow, needs the game data). Answers the first unknown of [§2026-11-08T12:09:08](#ts-20261108-120908).

**The velocity change (0x192a64..0x192ad0).** After sending the damage message the handler computes n = trunc(closing speed / 12.0) (0x192cfc; 0x252c14 truncates) and takes the factor 0.9 (0x192d00) for n = 0, 0.8 (0x192d04) for 1, 0.7 (0x192d0c) for 2 and 0.4 (0x192d08) for every other n. It then takes the body (0x242554 = entity +0x84; with no body nothing follows), reads the linear velocity (0x24092c: three floats from body +0x7c, offsets 0x13c..0x144, the fourth is 0), multiplies the three components by the factor (0x248850) and writes them back with the fourth component 0 (0x229920). The change comes after the message and does not depend on its reply: the hit on the last heart scales the velocity too, and so does any contact in state 5 with the hold at 0 until the driver step enters state 6.

**The delivery of the message.** 0x244fc8(entity, 0, message) reads the kind byte +0x12 of the entity, takes the pointer at +0x14 of its 40-byte row in the table at 0x583124 and, when it is not 0, tail-calls it with the entity and the message: the damage message is delivered at once, not queued. This answers the delivery unknown of SCRIPT-S336.

**What hits the stolen car's hearts.** Only this contact path in state 5: each contact of the player's vehicle above 3 m/s sends one heart (word +0xc = 1, kind 11); with more than one heart left the base handler takes it and the driver's hold +0xac = 128 refuses further contacts for 128 updates; on the last heart state 6 is requested. `1-4_Stolen_Vehicle` has 5 hearts (the target display of VehicleSetChased shows 5 of 5), so the chase takes five hits at least 128 updates apart.

**In the port.** `src/runtime/chase_end_flow.*` (`chasedCarContact`, `chasedCarVelocityScale`) joins the contact message, the reply and the factor; `LevelSession::chasedCarImpact` scales the car's rigid body velocity inside the contact callback of the vehicle world and logs `vehicle V: heart lost (S m/s), N left at update U`.

**The real run.** `chase-game --call-script 1-4_Hatch_Open` seats the player in `1-4_Police_Car02` at update 713 (VehicleJumpIn by the script), the chase starts at update 770. The test places the seated car in the stolen car's way with `at X Y Z FACING` (facing 90 is +x, 270 is -x, 0 is +z, 180 is -z) five times, at positions taken from the stolen car's own line in a port run: heart lost at updates 840 (4 left), 1124 (3), 1310 (2), 1718 (1), then the driver requests state 6 at update 1864 and enters it at 1865 (VehicleChaseEnded at the car, `1-4_Car_Crash` starts), the chaser starts to exit at update 1897 (1 s after the script start plus the exit's own updates) and has left the car at 1919, where the arrest after the crash makes him arrestable. Update numbers are the port's.

Port choices (SCRIPT-S340): the velocity is scaled in the port's contact callback, which runs per contact point before the solver of the same step; where the original's contact message arrives relative to its solver is not read, and the closing speed 0x19b5c4 is the port's `manifoldImpactSpeed`, not compared with the original's.

Unknowns:
- How the original's chase goes (the speed and line of the stolen car, how many hits a player needs, the jump out's timing): no oracle state of mission 1-4 exists. A state would need to be recorded in mission 1-4 after the skylight scene, with the player seated in the police car behind the stolen car while the chase runs, ideally with the stolen car on its last heart (SCRIPT-457).
- The special case of the car `7-9_RexCar` and the base handler after the contact message (SCRIPT-S336).

