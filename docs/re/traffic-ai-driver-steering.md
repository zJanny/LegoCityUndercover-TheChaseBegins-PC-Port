# Traffic: the steering of the AI driver

Subtopic of [traffic.md](traffic.md); the AI driver that calls it is in [traffic-ai-driver.md](traffic-ai-driver.md). This file: the steering controller 0x27872c of states 5 and 8, the contact step 0x29cc38 of the post step 0x1bc79c, and the top gear speed 0x20362c.

## Current state

- The controller 0x27872c(entity, target, dt) turns the target matrix into the steer, accelerate and brake inputs of the vehicle record (+0x288, +0x28c, +0x290) with two controllers in the driver record (steering +0x7c..+0x8c, speed +0x90..+0xa0). It sets the target speed +0x70 from the cruise speed, slower when turning at speed. It places the car at the target when the target is 10 or more above it, or when the car has been stuck for 90 updates. [§2026-11-07T12:12:03](#ts-20261107-121203)
- The speed output is clamped by code that runs the speed controller up to three times, so below the limits the derivative part is lost and the output is 0.9 x the error. [§2026-11-07T12:12:03](#ts-20261107-121203)
- The contact step 0x29cc38 updates driver +0x58 to the node position, gathers the characters in a box around the car (half size entity +0x5c plus the move when it is below 20) and runs the wall slide 0x21a73c on them with the move; its result is not used. [§2026-11-07T12:12:03](#ts-20261107-121203)
- 0x20362c(body) is the speed at MaxRPM in the last gear, which is TopSpeed for the final drive 0x2435d0 sets. [§2026-11-07T12:12:03](#ts-20261107-121203)
- The writer of the launch byte +0x7a (VehicleSetRaceTrim), the slide 0x21a73c of the contact step and the placement 0x23091c: [traffic-ai-driver-calls.md §2026-11-08T12:12:24](traffic-ai-driver-calls.md#ts-20261108-121224).
- Port: `src/runtime/traffic_driver_steering.*` (test `traffic-driver-steering`), not connected: the cars reach no state that steers (TRAFFIC-36, [traffic-driver-connect.md](traffic-driver-connect.md)).

<a id="ts-20261107-121203"></a>

## 2026-11-07T12:12:03+01:00 — The steering controller 0x27872c, the contact step 0x29cc38 and the top gear speed 0x20362c (llvm-objdump only, no REA session, no emulator)

Method: `llvm-objdump -d --triple=armv6k-none-eabi --mattr=+vfp2` on `artifacts/game/game.arm.elf`, words read from the ELF segments, worker 20261107-traffic-C, 2026-11-07. Helpers read along the way: 0x237eb4, 0x2536a8, 0x257e78, 0x257f14, 0x2554c0, 0x230160, 0x258858, 0x242554, 0x2435d0. Port: `src/runtime/traffic_driver_steering.*`, test `traffic-driver-steering`. This answers the unknowns 0x27872c, 0x29cc38 and 0x20362c of [traffic-ai-driver.md §2026-10-25T12:26:09](traffic-ai-driver.md#ts-20261025-122609). The callers are state 5 (0x26debc) and state 8 (0x26d9dc) of 0x26d934 for the controller, and the post step 0x1bc79c (0x1bc96c) for the contact step.

**Top gear speed 0x20362c(body).** +0x268 (MaxRPM) / |table 0x376d84 [+0x298 − 1] × +0x29c (final drive) × word 0x376d78 (1.0)| × 0.10471975 (0x3dd6774f) × +0x2f8, in that order. +0x298 is the gear count 7, so the ratio is 0.848. The final drive 0x2435d0 is MaxRPM / (|0.848 × 1.0| × TopSpeed +0x2a0) × 0.10471975 × +0x2f8, so the result is TopSpeed up to rounding.

**Contact step 0x29cc38(entity).** Driver = record +0x2a4 of entity +0x7c.
1. moved = node position − driver +0x58 (0x257f14), then driver +0x58 = the node position (0x2554c0).
2. extra = 0. When moved · moved > 0 (0x230160): extra = |moved| (0x2536a8), and back to 0 when 20 (0x29cd94) ≤ |moved|.
3. The grid gather 0x237f30(0x578dfc, box, list on the stack, 128, descriptor). The box is the centre driver +0x58 and the half size entity +0x5c + extra on all three axes. The descriptor at sp+0x210 is {4, 0, −1, 0, 0, 16, 512, 3, entity, byte 1} in the word order of [collision.md](collision.md#ts-20261013-173400): mask 4 (characters), any kind, owner +0xc without 16, owner +0x10 with 0x200 and without bits 0 and 1, the car itself excluded.
4. When the gather lists anything, the first four words of the descriptor are overwritten with (moved, 0.0) and 0x21a73c(entity, that vector, list, count, 255, word sp+4, 0, 0) runs. Its result is ignored.

**Controller 0x27872c(entity, target r1, dt s0).** Record = entity +0x7c, driver = record +0x2a4, node = 0x250810(entity +0x3c), body = 0x242554(entity) = entity +0x84.
1. local = the target translation in the node's frame (0x237eb4: the difference to the node translation dotted with rows 0, 1 and 2).
2. When 10 (0x278a98) ≤ target y − node y, 0x23091c(body, target, 0).
3. When record byte +0x310 is 0 (no forced stop):
   - +0xa4 = 0 unless speed +0x68 ≤ 5 (0x278a9c) and |+0x68 − +0x70| > 5;
   - otherwise, once +0xa4 ≥ 90 (unsigned): 0x23091c(body, copy of the target with y − 0x20af24, 0) and +0xa4 = 0.
   - With +0x310 set, +0xa4 is left alone.
4. When +0x70 < 0: local x and z are negated.
5. angle = 0x24765c(local x, local z) (the game's atan2f).
6. +0x70 = +0x6c, except when |angle| ≥ 0.0872665 (0x3db2b8c3, 5 degrees), +0x68 ≥ 20 (0x278aac) and +0x68 ≥ +0x6c × 0.5. Then +0x70 = min × (0.5 + 0.5 × local · (0, 0, 1) / |local|). min is +0x6c when 0x20362c(body) × k > +0x6c, else 0x20362c(body) × k. The vector at 0x56d430 is the z unit vector ([pushables.md](pushables.md)). k is 1.0 with +0xaf bit 2 and 0.8 without it, as in state 5.
7. Steering controller: +0x84 = 5.0, +0x88 = 0, +0x8c = 1.0 when the new +0x70 ≥ 0 (or unordered), else 5.0. One update with the error angle:
   - +0x80 += angle × dt;
   - out = +0x88 × +0x80 + +0x84 × angle + +0x8c × (angle − +0x7c) / dt;
   - +0x7c = angle.
   The output is negated when +0x70 < 0. It is clamped by its bits: 1.0 when they are at least 0x3f800000 (signed), else −1.0 when they are at least 0xbf800000 (unsigned). The result goes to record +0x288.
8. Speed controller: +0x98 = 0.9 (0x278ac4), +0x9c = 0, +0xa0 = 0.002 (0x278ac8). When driver byte +0x7a has bit 0:
   - while +0x68 < +0x70 (or unordered) the gains are 100 (0x278acc) and 0;
   - otherwise +0x7a = 0.
   The error is (+0x70 − +0x68) × dt; the update is the same rule on +0x90 (previous error) and +0x94 (integral).
9. The clamp evaluates the update again for each test (0x278a1c..0x278a74):
   - the first output with bits at least 0x3f800000 (signed) gives 1.0;
   - else a second update; bits at least 0xbf800000 (unsigned) give −1.0;
   - else a third update is the output.
   The second and third updates see the previous error equal to the error, so their derivative part is 0, and the integral advances once per update. Below the limits the output is therefore 0.9 × the error (100 × the error while +0x7a holds).
10. An output above 0 goes to record +0x28c (accelerate). Otherwise its negation goes to record +0x290 (brake); an output of 0 writes −0.0. The other input of the two is not written.

Other findings:
- Driver +0x7a is written only at 0x26d8e4 (= 1, in 0x26d8c4, called from 0x2860bc) and at 0x2789d8 (= 0).
- The third store to a +0x7a, at 0x211504, writes the bytes +0x78..+0x7b of its record from four floats times one constant (by its shape a colour); whether that record can be a driver was not read.
- The driver bytes +0x7a..+0xa3 are zero after the record's creation 0x1796e8 (zero-filled allocation), so both controllers start at 0.

Unknowns:
- The function 0x26d8c4 that sets +0x7a and its caller 0x2860bc (which cars, when).
- What 0x21a73c does with the characters the traffic car moves into (the 5th argument 255; the 6th word sp+4 is not written by 0x29cc38 and holds an older stack value).
- The body placement 0x23091c with the third argument 0.
