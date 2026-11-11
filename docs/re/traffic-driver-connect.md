# Traffic: the AI driver connected to the cars

Subtopic of [traffic.md](traffic.md); the driver's states, the route mover and the junction functions it runs are in [traffic-ai-driver.md](traffic-ai-driver.md). This file: how the port runs the AI driver on the spawned traffic cars, and the driver's part of the removal message 0x80000006. Feature file: [pedestrians-traffic-cars-driver.md](../plans/features/pedestrians-traffic-cars-driver.md).

## Current state

- The vehicle message handler 0x1797c4 first calls 0x19293c for a car whose driver record has +0xaf bit 0. For the removal message 0x80000006, 0x19293c calls the junction queue 0x22a080 with the driver's junction (+0) when that is set, then returns 0 and the vehicle handler goes on with its own part (with the release 0x22a1f4). A car removed on a junction lane therefore leaves the junction's queue. [§2026-11-08T12:18:49](#ts-20261108-121849)
- The port runs the AI driver on every live traffic car once per update: the input provider 0x26d934, the state change 0x1cb148 and the post step 0x1bc79c, with the ported route functions, path follower, stop test, junction choice and queue, route mover, leave call and ground probe. The cars drive along their roads, stop at red lights, take the junction lanes and leave at the end of a road that leads into no junction. States 4 to 8 are not reached. [§2026-11-08T12:18:49](#ts-20261108-121849)
- The keep counter reads the culling bit 18 of the car's node (0x23de70) through `TrafficDriveStep::lights`; the draw sets it from the camera's view volume ([traffic-car-culling.md §2026-11-12T00:12:13](traffic-car-culling.md#ts-20261112-001213)), and the post step's brake lights 0x278b78 run on the car's brake light parts ([traffic-car-lights.md §2026-11-12T00:10:29](traffic-car-lights.md#ts-20261112-001029)).
- Port: `src/runtime/traffic_driver_connect.*` (test `traffic-driver-connect`), `TrafficSystem::driveCars`, glue `LevelSession::driveTrafficCars` in `src/game_traffic.cpp`. [§2026-11-08T12:18:49](#ts-20261108-121849)

<a id="ts-20261108-121849"></a>

## 2026-11-08T12:18:49+01:00 — The AI driver connected to the traffic cars, and the queue leave of the removal message (llvm-objdump, no REA session, no emulator)

Corrected by [traffic-car-culling.md §2026-11-12T00:12:13](traffic-car-culling.md#ts-20261112-001213): the culling bit 18 of the car's node is set by the port from the camera's view volume, so the stand-in TRAFFIC-S7 below is replaced by TRAFFIC-S35.

Method: `llvm-objdump -d --triple=armv6k-none-eabi --mattr=+vfp2` on `artifacts/game/game.arm.elf` (worker 20261108-traffic-B, 2026-11-08), read at 0x1797c4..0x179808 and 0x19293c..0x192d54. Port: `src/runtime/traffic_driver_connect.*`, test `traffic-driver-connect` (made-up roads, junction and light), `traffic-system` and `traffic-system-real-data` (the cars driven in the made-up level and on lvl_01), CTest `game-traffic`. No emulator was used.

**The driver's part of the car messages (0x19293c).** The vehicle message handler 0x1797c4(entity, message, data) loads the vehicle record (entity +0x7c) and its driver (+0x2a4). With a driver whose +0xaf bit 0 is set it calls 0x19293c(entity, message, data) first (0x179800); a non-zero result ends the handler (0x17a44c). 0x19293c dispatches on the message: 0x80000004, 0x80000002, 0x80000003 and 0 have their own branches (0x192b98, 0x1929c0, 0x192d20, 0x192ae0, not read here). For 0x80000006 (0x192998): when driver +0 (the junction) is not 0, 0x22a080(junction, entity) runs; then the result is 0 (0x192d48), so the vehicle handler's own part of 0x80000006 follows (the sounds and effects, 0x22a1f4, [traffic-ai-driver.md §2026-10-25T12:26:09](traffic-ai-driver.md#ts-20261025-122609)). 0x22a080 only acts with +0xaf bit 3 ([traffic-ai-driver.md §2026-11-02T23:44:38](traffic-ai-driver.md#ts-20261102-234438)). The manager sends 0x80000006 at every removal and the leave call 0x21d6b8 at its own. This is the caller "message 0x80000006 at 0x1929b0" of the queue listed in that entry.

**What the port runs.** Each car has its driver record from the level load (0x1796e8) and the vehicle record fields the driver writes. The spawn 0x2def8c writes +0x79 = 1, +0x78 = 0, +0xaf bit 0, +4 = the route, +0 = the route's junction (0x20ff40), +0x64 = the point's parameter and +0x18 = the placement matrix, in that order before the placement. Each update, for every car the manager has live and that has a vehicle, in car order:
1. 0x26d934 with the obstacle list (states 2 and 3) and the junction test 0x2cdde4 for a queued car;
2. 0x1cb148, with the node rotation as the rigid body basis (the port has no body; the spawn resets the body from the node in the original, so both are the same at state 1);
3. 0x1bc79c with the stop test 0x2ca504 and the route mover 0x2613ac. The mover's calls are the junction choice 0x2cb1e8, the leave call 0x21d6b8 (the manager's removal) and the ground probe 0x20ae2c (a ray on the level collision; its colour is not shown).
4. The post step's two calls before its state switch are the ported ones of [traffic-ai-driver-calls.md §2026-11-08T12:12:24](traffic-ai-driver-calls.md#ts-20261108-121224): the whistle stop 0x2c87a8 (`trafficWhistleStops`) and the brake lights 0x278b78 (`runTrafficBrakeLights`, which does nothing while the car's light parts are not set up, TRAFFIC-33).
5. The new node matrix is set on the car's vehicle. The manager's view of the car takes +0x78, +0x79, +0xad, +0x68, +0x64, the route's index in the route list and the node position. A removal (manager or leave call) then runs the driver's part of 0x80000006 and the release 0x22a1f4 of the held car (`releaseTrafficHeldCar`).

The route functions are those of the junctions' paths. The path follower 0x2bf2ec is now set: the pregen cell under the probe, the route bits of the manager, the forward axis of the matrix, and on a hit the route point, route and parameter.

**Result in `chase-game`.** From the start of lvl_01 (`--no-start-cutscenes --script "0 0 300"`, the port's own run, not a measurement of the original), the cars drive off along their roads. They leave earlier than before because they drive away from the player; the first removal is Traffic03 at update 68 (Traffic09 at 67 before). The first stop at a red light is Traffic03 at junction 15 at update 208. In a run of 1500 updates a car takes a junction lane at update 517. Before the queue leave of 0x80000006 was ported, a car removed on that lane kept junction 15 red for the rest of the run, because the light waits for an empty queue.

**Stand-ins.** Listed in the feature file:
- TRAFFIC-S27: the obstacle list (0x5863b4 + 0x335c, count +0x3058, then the player) is every enabled boardable vehicle with its entity box, then the player with the player box (0.6, 0.85, 0.52). The cars run in the object phase right after the junction update 0x1d8924, in table order. A car keeps the room of its spawn point. The whistle stop 0x2c87a8 gets no game clock 0x241098 (its unit is TRAFFIC-34); while the cop whistle is not ported its end stays 0 and it stops no car.
- TRAFFIC-S28: 0x22a270 → 0x213c74 only sets a flag; there is no Bullet body, so the velocity 0x229920 of the mover and body +0x27c are not applied, and the record fields +0x290, +0x294, +0x30c..+0x30e stay with the driver.
- TRAFFIC-S7: 0x23de70 (`pedestrianNodeHidden`) is taken as true for the keep counter, because the port sets no culling bit 18 on a car's node.
- States 4 to 8 call hooks that are not set (`steerToward`, `copyInputs`, `steerStep`, `normalGravity`, `wakeBody`, `setListed`, `enterStopping`, `destroy`, `rearEffect`); they throw when reached. Nothing in the port requests these states: their writers are the contact message 0x80000002, 0x22a1f4 and the chase code.

Unknowns:
- Where the vehicle update 0x1571dc and the post step 0x16ce9c run in a frame, against the manager update and the junction update; which entities the list 0x5863b4 + 0x335c holds.
- The other branches of 0x19293c (0x80000004, 0x80000002, 0x80000003, message 0).
- The culling bit 18 of the car's node, which 0x23de70 reads for the keep counter +0xad.
