# Traffic: the calls of the AI driver in states 5 and 8

Subtopic of [traffic.md](traffic.md); the AI driver is in [traffic-ai-driver.md](traffic-ai-driver.md), its steering in [traffic-ai-driver-steering.md](traffic-ai-driver-steering.md). This file: the writer of the launch byte +0x7a (VehicleSetRaceTrim), the body placement 0x23091c, the contact slide 0x21a73c on the gathered characters, the chase markers of state 5, the traffic stop of the cop whistle 0x2c87a8, the brake lights 0x278b78, and the writers of +0xaf bits 1, 2 and 4.

## Current state

- Driver +0x7a and +0xaf bit 2 are set only by the script command VehicleSetRaceTrim (0x286080): bit 2 follows the flag, and a set flag also gives +0x7a = 1 and holds the first such car in the global 0x3745d4. In lvl_01 the command follows ShowRaceStart and SetRaceInProgress (a race); the chase of mission 1-4 uses VehicleSetChased without it. [§2026-11-08T12:12:24](#ts-20261108-121224)
- +0xaf bit 1 is set only by VehicleSetThrowObjects (0x2ad940), bit 4 only by VehicleSetDCamPathFollow (0x27be1c, VehicleSetChased plus bit 4). Only VehicleSetAI (0x1796e8) and VehicleSetChased (0x1afe3c) create driver records. [§2026-11-08T12:12:24](#ts-20261108-121224)
- The placement 0x23091c(body, matrix, room) finds the room at the matrix position, puts a dynamic body and its node at the matrix (a kinematic body is reset from its node), clears the velocities, forces, rpm and wheel spin, wakes a dynamic body and links the node to the room. [§2026-11-08T12:12:24](#ts-20261108-121224)
- With the mode 255 of the contact step, the wall slide 0x21a73c only runs 0x2c8f80: each gathered character is pushed through 0x2baef0 (message 45 and a move of the character), and the move is cut to half the car's radius after a push. [§2026-11-08T12:12:24](#ts-20261108-121224)
- Entering state 5 gives the car an enemy arrow and a place in the enemy group of the carry-it waypoint (0x230dbc, 0x230e60); leaving 5 removes both. [§2026-11-08T12:12:24](#ts-20261108-121224)
- 0x2c87a8 stops all traffic while the clock 0x241098 is below a time the cop whistle state (0x1b2 COPWHISTLE) sets to clock + 120 + random % 60. The lights 0x278b78 are the brake lights: two mesh parts with an alpha level and up to four glow effects. [§2026-11-08T12:12:24](#ts-20261108-121224)
- The push 0x2baef0, the characters' message 45 and the whistle state 0x1b2 are read and ported in [traffic-car-push.md](traffic-car-push.md) (`src/runtime/traffic_car_push.*`, test `traffic-car-push`).
- The brake light parts, glow parts and glow effect of 0x278b78 are written by the vehicle's parameter reader 0x1658a8 from the class values `BrakeLightModel%02u`, `BrakeLightLocator%02u` and `BrakeLightParticle`; the port draws them. [traffic-car-lights.md §2026-11-12T00:10:29](traffic-car-lights.md#ts-20261112-001029)
- Port: `src/runtime/traffic_driver_calls.*` (test `traffic-driver-calls`), not connected to `ai_driver_input` yet (TRAFFIC-2).

<a id="ts-20261108-121224"></a>

## 2026-11-08T12:12:24+01:00 — VehicleSetRaceTrim and the launch byte, the body placement 0x23091c, the contact slide 0x21a73c, the chase markers, the cop whistle stop 0x2c87a8 and the brake lights 0x278b78 (llvm-objdump only, no REA session, no emulator)

Method: `llvm-objdump -d --triple=armv6k-none-eabi --mattr=+vfp2` on `artifacts/game/game.arm.elf`, words read from the ELF segments, worker 20261108-traffic-A, 2026-11-08. Script command names from `docs/re/script-natives-census.md`; the order of the commands in lvl_01 from the strings of `lvl_01_cherrytreehills_main.blvl` (extracted RomFS). Port: `src/runtime/traffic_driver_calls.*`, test `traffic-driver-calls`. This answers the unknowns of [traffic-ai-driver-steering.md §2026-11-07T12:12:03](traffic-ai-driver-steering.md#ts-20261107-121203) (0x26d8c4 and its caller, 0x21a73c, 0x23091c) and part of those of [traffic-ai-driver.md §2026-10-25T12:26:09](traffic-ai-driver.md#ts-20261025-122609).

**VehicleSetRaceTrim (handler 0x286080).** Driver = record +0x2a4 of the entity (argument 0); nothing without one. The flag is the word of argument 1 compared with 0, so any bits but zero count (also −0.0). +0xaf bit 2 = flag. With the flag set, 0x26d8c4(entity):
1. 0x230f04(0): the hearts display of a chased target (0x517f34, portrait sprites, record +0x30d / +0x30e; VehicleSetChased calls it with the car) is switched off.
2. Driver +0x7a = 1.
3. When *0x3745d4 is 0: when the node word & 31 equals *0x37674c, the entity bytes +0xdd = 255 and +0xde &= ~0x30 (the entity, not the node, as in VehicleSetChased); then *0x3745d4 = the entity.
With the flag clear, 0x22a1f4(entity) runs: nothing unless the entity is *0x3745d4 (not 0). Then node +0xde |= 0x30 for a mesh node, +0xaf bit 0 cleared, +0x79 = 0, vehicle record hword +4 = 4 and *0x3745d4 = 0.
- In lvl_01 VehicleSetRaceTrim comes after ShowRaceStart, SetRaceInProgress, VehicleSetTopSpeed and VehicleSetChased, so it marks a race car. The other VehicleSetChased of lvl_01 comes with VehicleSetLocked and VehicleSetForcedStop, without it.
- The third store to a +0x7a, at 0x211504, writes a record that 0x2f2b3c takes from the pool 0x514088 (callers 0x1b7348, 0x1c9ea0), not a driver. So +0x7a is written only at 0x26d8e4 and 0x2789d8.

**Writers of +0xaf.** Bit 1: VehicleSetThrowObjects (0x2ad940) sets or clears it by the word of argument 1. Bit 2: VehicleSetRaceTrim. Bit 4: VehicleSetDCamPathFollow (0x27be1c → 0x28548c: VehicleSetChased 0x1afd38, then bit 4 set). Bit 0 is also cleared by 0x22a1f4 and by the vehicle message code at 0x17a424 (when r7 equals the word at r11; not read further); 0x188770 is a plain byte setter of another class. The census counts VehicleSetThrowObjects once in all levels and not in lvl_01.

**Creators of driver records.** The stores to +0x2a4 at 0x262794, 0x290de8 and 0x2a60c4 write fields of global objects (the clock 0x241098, zero), not vehicle records. So only 0x1796e8 (VehicleSetAI and the traffic manager) and 0x1afe3c (VehicleSetChased) make driver records.

**Chase markers of state 5.** 0x230dbc(entity, on) is the enemy arrow (list 1) of the sub-objective manager *0x51a3a8: on appends the entity unless listed (timer 0.5, byte 0), off tail-calls 0x1f5fa8. 0x230ef4 returns *(0x529c18 + 0x24), the carry-it waypoint. 0x230e60(key, entity, on, 0): on is 0x1fce14(*0x51a3a8, key, entity, 1) (the fifth argument is not read), off is 0x1fcc68(*0x51a3a8, key, 1, entity) and the removal by the last entry. So a car in state 5 gets the enemy arrow and is a target of the enemy group (type 1) of the carry-it waypoint ([objective-hud.md](objective-hud.md)); VehicleSetDCamPathFollow's bit 4 keeps both off.

**Body placement 0x23091c(body, matrix, room).** body = entity +0x84, rigid = body +0x7c.
1. Body +0xc4 &= ~0x88000.
2. room 0: room = 0x239838(matrix +0x30), the first room of the list 0x589a18 that holds the point (0x22d204). No room: return 0.
3. Rigid +0xcc bit 1 (kinematic): body vtable +0x18 (0x33c434) with the node; the matrix is not used. Otherwise 0x2429fc(body, matrix) (the rigid transform from the matrix through body +0x80) and vtable +0xc (0x33c3e8: 0x33a8ac and 0x33929c for each wheel) with the matrix and the node.
4. 0x229920(body, 0x56d400) and 0x21315c(body, 0x56d3a0): rigid +0x13c..+0x148 and +0x14c..+0x158 from the zero vectors of the static initialiser 0x344140. Rigid +0x84..+0xa0 and +0x1a0..+0x1bc = 0.
5. Body +0x27c = +0x26c (rpm = IdleSpeed), +0x28c = 0, the float +0x88 of every wheel record (+0x2b8 + 0x154 i, count +0x2b4) = 0, +0xda0 = 0.
6. 0x23f0d4: command 9 of the body queue (0x1fb648(0x5863b4, rigid, 9, 1, 0)).
7. 0x2308e4(rigid, 0): nothing when rigid +0xcc & 3; else +0xd8 = 1 unless it is 4 or 5, and +0xdc = 0.
8. When node +4 is not room +0x14: 0x245048 (when node +4 is set), 0x245008(room +0x14, node), 0x229bf0(entity, room).
9. With byte 0x376c35 set: 0x20c0e4(body). Return 1.

**Contact slide 0x21a73c(car, move, list, count, 255, …).**
1. Each entry whose owner (+0x10) 0x1fc508(car, owner, 0) reports as a pair without collision gets word bit 0x80.
2. r = 0x2c8f80(car, move, list, count).
3. A zero move in x and z ends here unless y is not 0 and the car is the player *0x3772c0; the mode 255 ends here too, so the wall part (0x208d84 and the slide) never runs for a traffic car.
4. The bits 0x80 of all entries are cleared; the result is r.
**0x2c8f80(car, move, list, count).** slide = car hword +0x10 without bit 0x100. For each entry in order: skipped when its owner has no node, when the car's node is the owner node's parent or the car node's parent is the owner node, or when the entry word has bit 1. Otherwise 0x2baef0(car, move, entry, list, count, slide); a push with slide goes on to the next entry, a push without slide ends the loop. After any push, a move longer than 0.5 (0x2c9070) × car +0x5c is scaled to that length (0x248850). The result is whether anything was pushed.
**0x2baef0(car, move, entry, list, count, slide)** (read, not ported): nothing for a pair without collision or unless both hwords +0x10 have bit 0x200. The push vector starts at 0x56d3a0 and is filled by 0x1f49d8(car, move, entry, vector, slide). With a vector, message 45 goes to the character with (car, vector). Without slide that is all (result 1). With slide, when the character's word +0xc has bit 3 and 0x245b00(car) is true, 0x233594(car, character, 0) runs and the result is 0. Otherwise the other entries with hword bit 0x100 are tested against the character with 0x1f49d8; a car with hword bit 0x2000 or a character with bit 0x100 gets message 45 back with (character, 0), else the character's node moves by the vector (0x23d39c, 0x24c118), by the vector of a blocking entry when one was found.

**Cop whistle stop 0x2c87a8.** Returns whether 0x241098() is below *(*(0x3746d4 + 0x20) + 0x18), unsigned. The only writer besides the reset 0x18772c (0, with +0x8 and +0x14) is 0x26e254: clock + 0x235498(60) + 120. It is the tail of 0x276338, the enter function of the player state 0x1b2 `COPWHISTLE` (state table at 0x357a08: id 0x1b2, update 0x29a2e0, enter 0x276338, name at 0x3661f3); 0x276338 starts the animation row 709 (0x21941c) first. The post step's only caller (0x1bc7bc) then sets +0x70 = 0 for every AI car.

**Brake lights 0x278b78(entity, s0 = brake, s1 = 0.0, s2 = dt).** The post step passes brake 1.0 when driver +0x70 or +0x68 is 0, else 0.0. Record = entity +0x7c. Nothing when record +0x2ac < 0.
1. Level: 1.0 (and lit) when brake > 0 and (the entity has no body or body +0x294, the gear, is not 0). Otherwise record +0x2d4 − dt × 2.0 (0x278d34), at least 0.
2. For each of the four glow parts +0x2b4..+0x2c0 (not below 0): when lit and the node word lacks bit 0x40000, a slot without an effect (+0x2c4 + 4 i) gets 0x23de24(+0x2d8, the part matrix 0x236368(node, part) +0x30, node, 1, 0.0, 0, 0), its flag 0x2000 cleared, its matrix 0x241540 and the end call 0x2cb42c with the entity (0x23de14). Otherwise a running effect is stopped with 0x23ddb4(effect, 0.0); its slot is cleared later by 0x2cb42c at event 2.
3. When the level differs from +0x2d4: +0x2d4 = level, alpha = (int)(255 × level) & 255; for the parts +0x2ac and +0x2b0 (not below 0): alpha not 0 gives 0x2363d4(node, part, 1) and 0x23f124(node, alpha, part, 0), alpha 0 gives 0x2363d4(node, part, 0).

**Other calls of the post step.** 0x23de70 is ported as `pedestrianNodeHidden` ([pedestrians.md](pedestrians.md)); 0x2308e4 is the activation of `vehicle_body`; 0x20ae2c is the ground probe of `traffic_leave_snap` ([traffic-ai-driver.md §2026-11-03T08:37:32](traffic-ai-driver.md#ts-20261103-083732)).

Unknowns:
- 0x1f49d8 (the push vector), the squash 0x233594 and what the characters do with message 45 from a car.
- The writers of the brake light parts and effect (record +0x2ac..+0x2d8), the draw of 0x2363d4 / 0x23f124, the glow effect, and who sets node word bit 0x40000.
- The cop whistle state 0x1b2 (its update 0x29a2e0, when the player enters it) and the unit of the clock 0x241098.
- Body queue command 9 (0x23f0d4), body +0x28c, the body vtable functions 0x33c434 and 0x33c3e8.
- The state 6 calls 0x229be4, 0x23cee8 and 0x22a124(0x11f), and 0x1bc660 of state 5 (bit 1).
