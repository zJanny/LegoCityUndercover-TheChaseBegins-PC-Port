# Traffic: the end of a car chase

Subtopic of [traffic-ai-driver.md](traffic-ai-driver.md): how a chased car's driver reaches state 6 and fires VehicleChaseEnded. Feature file: [script-system-missions-1-4.md](../plans/features/script-system-missions-1-4.md).

## Current state

- The only writer of the requested driver state 6 is the vehicle handler 0x19293c: a hard contact of the player's vehicle on a car in state 5 sends the car a damage message, and the hit on the last heart requests state 6. Entering state 6 fires VehicleChaseEnded at the car. Ported as `vehicle_chase_events`; the car's driver is not stepped yet, so it does not happen in play. [§2026-11-08T12:09:08](#ts-20261108-120908)

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
