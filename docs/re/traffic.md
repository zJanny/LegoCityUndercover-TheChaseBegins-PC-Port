# Traffic: the traffic manager and the AI driver input

The traffic manager 0x527410 spawns and removes the level's `Traffic%02u` vehicles. The AI driver input 0x26d934 steers the spawned cars. The pedestrian manager 0x51b33c is in [pedestrians.md](pedestrians.md); the manager swap is corrected in [pedestrians.md §2026-10-20T11:17:42](pedestrians.md#ts-20261020-111742). The earlier entry [script-natives-audio.md §2026-10-15T15:10:38](script-natives-audio.md#ts-20261015-151038) gave the two manager addresses the wrong way round. Feature file: [pedestrians-traffic-cars.md](../plans/features/pedestrians-traffic-cars.md).

## Current state

- Object 0x527410 is the traffic manager (vtable 0x361e34). Its level load 0x2dfa78 collects `Traffic01..Traffic32` and reads `Traffic_MaxVehicles` (default 5, limited to the count of cars) and `Traffic_FadeBound01..04`. SetTrafficMaxVehicles 0x2a6d70 → 0x286ca4 writes +0x36c = min(argument, count). [§2026-10-20T11:18:14](#ts-20261020-111814)
- Update 0x2dfe88: with bit 1 of +0x390 clear, every live car is removed. With bit 0 clear (no pregen route data), nothing happens. Otherwise it spawns at most one car per update while fewer than the limit are live and the countdown is 0. After a spawn the countdown becomes (live + live / 2) x 3. It also removes at most one car per update. [§2026-10-20T11:18:14](#ts-20261020-111814)
- Spawn 0x2def8c: 10 random tries in the pregen quadtree leaf under the player. A point is rejected when its route is disabled, its room is not active or its distance is outside the LOD band (0.7 x near to 1.75 x far; 0 to far while prefilling). It is also rejected when another driver on the same route stands within 16 m (x and z only). [§2026-10-20T11:18:14](#ts-20261020-111814)
- The room set-up 0x2dedbc runs the update 64 times with dt 0 and the player as focus (the prefill). The fade 0x2df8b8 is z / half z x 255 inside a `Traffic_FadeBound` box. [§2026-10-20T11:18:14](#ts-20261020-111814)
- The vehicle update 0x1571dc picks its input provider as follows: 0x26d934 (AI driver) when the driver record (vehicle record +0x2a4) has +0xaf bit 0 set; 0x29cd9c for the player's vehicle; otherwise 0x1d8df0, which is an empty function (`bx lr`). [§2026-10-20T11:18:14](#ts-20261020-111814)
- The AI driver 0x26d934 switches on driver +0x78 (states 2 to 8). States 2 and 3 run the obstacle stop rule, state 8 snaps the car back to its path, and states 5 and 6 are only located. [§2026-10-20T11:18:14](#ts-20261020-111814)
- Port: `src/runtime/traffic_manager.*`, test `traffic-manager`. The port is not wired into `chase-game`.
- Open: who moves driver +0x78 away from 0 after a spawn, the path follower 0x2bf2ec, the LOD band values (table 0x529e30 is filled at run time), the writer of the pending route list +0xb9c, and how the pregen format ([pregen-traffic.md](pregen-traffic.md)) feeds the manager.

<a id="ts-20261020-111814"></a>

## 2026-10-20T11:18:14+02:00 — Traffic manager 0x527410, spawn and removal rules, SetTrafficMaxVehicles, the AI driver provider 0x26d934 (llvm-objdump only, no REA session, no emulator)

Method: `llvm-objdump -d --triple=armv6k-none-eabi --mattr=+vfp2` on `artifacts/game/game.arm.elf`, with words and strings read from the ELF segments. Port: `src/runtime/traffic_manager.*`, test `traffic-manager`.

**Which manager.** The vtable at 0x361e34 (+0x18 init 0x2dfa48, +0x1c level load 0x2dfa78, +0x40 room set-up 0x2dedbc, +0x44 unload 0x2def0c, +0x48 update 0x2dfe88, +8 0x2dfe78) belongs to object 0x527410. The level load looks up the strings "Traffic%02u" (0x2dfc4c), "Traffic_MaxVehicles" (0x2dfc58) and "Traffic_FadeBound%02u" (0x2dfc6c). The enable 0x29f09c writes bit 1 of +0x390, and SetTrafficMaxVehicles writes +0x36c of the same object. Object 0x51b33c (vtable entries 0x2ee4b4, 0x2ee4c8, 0x2ee5e8) looks up "Pedestrian%02u" and "Pedestrians_Max", so it is the pedestrian manager. [script-natives-audio.md §2026-10-15T15:10:38](script-natives-audio.md#ts-20261015-151038) gave the two the wrong way round. The same was found independently in [pedestrians.md §2026-10-20T11:17:42](pedestrians.md#ts-20261020-111742), which records the correction. In lvl_01, Traffic01..10 are `vehicles/*` objects (the reading of the level data in that entry).

**Layout of 0x527410.** +0x40 the car pointers (32), +0xc0 the car count, +0xc4 the route list with count +0x2d4, +0x2c4 a bit set of enabled routes (route index = route +0x24), +0x2d8 the fade bounds (4) with count +0x2e8, +0x2ec the spawn order (32 words), +0x36c the vehicle limit, +0x370 one fade byte per car, +0x390 flags (bit 0 = routes and pregen loaded, bit 1 = enabled), +0x394 / +0x594 the pairs of route → signal object with count +0x794 (filled by 0x2dfc84), +0xb9c a pending route list with count +0xd9c, +0xda0 the spawn countdown, +0xda4 the prefill byte.

**Init 0x2dfa48** sets bit 1 (traffic on) and clears the bytes +0x2c4 and +0x2c5. **Level load 0x2dfa78(manager, level).** For n = 1..32 it looks up `Traffic%02u` with 0x23b350. Each found object is appended to the car list, gets fade byte 255 and message 0x80000007 with that byte, and is passed to 0x1796e8 and disabled with 0x2428c0. Limit +0x36c = count when count < `Traffic_MaxVehicles`, else the attribute (0x2451d0 with default 5). The order list starts as the identity 0..31. Then, count times, two draws `0x25b2b8() % count` (0x25c1b8, unsigned) select the entries a and b, and the two entries are swapped. Fade bounds: up to 4 objects named `Traffic_FadeBound01..04` (0x23e798).

**SetTrafficMaxVehicles 0x2a6d70.** The argument (float at argument +4) is converted with vcvt.s32.f32, then 0x286ca4 compares it unsigned with the count: +0x36c = count when count <= value, else the value. A negative value therefore gives the count, and NaN gives 0. It returns 1.

**Room set-up 0x2dedbc.** First 0x2df574. It collects the route objects within 9000 of the origin (0x2c8718, at most 32), gives every new route of their record lists (+0x421 entries at +0, +0x422 entries at +0x140, stride 40) an index (route +0x24) and sets its bit. It stores a centre and a radius (+0x24 hword, +0x28) of their bounding area and loads `Pregen/Traffic/%s_Traffic.bin` (0x2e02d4); the result becomes bit 0. Then the route → signal pairs are filled (0x2dfc84) and the countdown is set to 0. The pending routes +0xb9c clear their bits when bit 0 is set, and the pending count is reset. Then the prefill: byte +0xda4 = 1, 64 calls of the update with dt 0.0, byte +0xda4 = 0. The unload 0x2def0c frees the pregen data and clears bit 0.

**Update 0x2dfe88(manager).**
1. Bit 1 clear: each car that is not disabled (owner flags +0x10 bit 0) and not pinned (bit 0x20) gets message 0x80000006 and 0x2428c0, and its fade byte is set to 255. Return.
2. Bit 0 clear: return.
3. live = the number of cars with owner bit 0 clear. countdown = countdown - 1 when that is above 0, else 0.
4. When limit > live and countdown = 0: for i = 0 .. limit - 1, car = order[i % count]. A live car is skipped. For a disabled car the update calls the spawn 0x2def8c. On failure the loop ends. On success the car's order entry moves to the end of the list, live + 1, and countdown = (live + live / 2) x 3, unless a lower nonzero countdown is already set. Then the loop ends.
5. For each live car, with driver = vehicle record +0x2a4: skip when driver +0x79 = 1 or +0x78 = 1. When +0x78 is 2 or 3, refresh the fade (0x2df8b8). The car is then removed in either of two cases. Case 1: the room tests fail. These are: entity +0x38 is set, 0x2284bc is true (+4 → byte +8 = 2 and +0x14 set), the layer table 0x1ffbd8 has bit 1 for (entity byte +0x18, room byte +0x3a), and the node parent (node +4) is not the room's +0x7c. Case 2: +0x78 or +0x79 is 0, or the fade byte is 0, or the fade byte is 32 or less while driver +0x68 is not above 0. Otherwise the car is kept when driver +0xad is set or its squared distance to the player is at most r², where r = 0x26e130 (byte +0xb of the 16-byte row `record hword +0x20` of the table `*0x375f40`, 0.0 for row 0). A car beyond that distance is removed. Removal means message 0x80000006, 0x2428c0 and fade byte 255. A pinned car (0x20) is not removed. In both cases the update sets countdown = ((live - 1) + (live - 1) / 2) x 3 unless a lower nonzero countdown is set, and returns. So at most one car leaves per update.

**Spawn 0x2def8c(manager, car).** The focus is the player's node when the prefill byte is set, else the camera node `*0x3768b4`. The leaf is 0x1f47f0: a descent of the pregen quadtree under the player position. A node with a nonzero count (hword +2) or without children (+0x10) is the leaf. Its points are at +0xc. Each point has position +0, direction +0xc, route +0x18, speed +0x20, room object +0x24 and a body value +0x28. There are up to 10 tries. Each try takes the point `0x235498(count)` (random % count). The try is rejected in the following cases:
- the route is 0 or its bit is clear;
- the room test fails (room object set, 0x2284bc, 0x1ffbd8 for the car's byte +0x18);
- the 3D distance d to the focus is outside the band (near > d or d > far). The band is 0 .. table[c] +0x3b8 while prefilling. Otherwise it is 0.7 x table[c] +0x3ac .. 1.75 x table[c] +0x3b8. The table is 0x529e30, and c = 0x28781c(car node) is the LOD class 0..2 from the node's draw distance +0x9c against table +0x3a0 / +0x3a4 / +0x3a8, or -1;
- any other car (owner bit not tested) whose driver has the same route and +0x78 or +0x79 nonzero stands at an x/z squared distance below 256.

On success the following happens, in this order:
1. The node bytes +0xdd = 1, +0xdc = 0xff; the wheel nodes +0xdd = 255.
2. The fade refresh 0x2df8b8 runs, at the car's old position.
3. Driver +0x79 = 1, +0x78 = 0, +0xaf |= 1, +4 = route, +0 = the signal object paired with the route (or 0).
4. Record +0x311 |= 4.
5. The car's matrix is built from the point: y + 0x20af24(car), z axis = point direction. Driver +0x64 = point speed. The matrix goes to the node and to driver +0x18. Body +0x8c = point +0x28, and the body's vtable +0x18 resets it.
6. 0x23091c(body, …) can still fail here. It then returns 0, with the car moved and the driver fields written.
7. Otherwise: message 0x80000007 with the fade byte, record hword +4 = 1 (the traffic state request), owner flag 0x40 cleared, 0x242cf0 (enable), message 0x80000005, 0x22e7c4(node, 1, 1, 1). It returns 1.

**Fade 0x2df8b8(manager, car, index).** fade = 255.0. When the driver exists and +0xaf bit 0 is set, it walks the fade bounds. delta = car position − bound +0x10. When bound byte +8 is not 255, delta is turned by the room matrix (0x229a40: component i = dot(delta, row i) of the 64-byte record at room +0x28 + byte x 64). The first bound with |dx| < +0x1c, |dy| < +0x20 and |dz| < +0x24 decides: f = 0 for dz < 0, else dz / +0x24; when f <= 1.0, fade = f x 255. Then, when float(fade byte) != fade, the byte becomes (int) fade and message 0x80000007 is sent. A fractional fade therefore sends the message again every call.

**Input provider choice in the vehicle update 0x1571dc** (0x157274..0x1572c0). Frozen (record +0x312 bit 0): none. Driver record +0x2a4 set with +0xaf bit 0: 0x26d934. Player's vehicle (`0x24255c(*0x3772b8)` = entity): 0x29cd9c. Otherwise 0x1d8df0, which is a single `bx lr`. Parked cars therefore get no input at all, and a spawned traffic car uses 0x26d934. [Corrects vehicles-system.md §2026-10-14T22:44:38](vehicles-system.md#ts-20261014-224438) and [Corrects vehicles-driving.md §2026-10-15T09:30:35](vehicles-driving.md#ts-20261015-093035) (the provider list). Those entries named 0x1d8df0 the AI / parked driver and 0x26d934 a tow provider.

**AI driver 0x26d934(entity)** jumps by driver +0x78 − 2 through the table at 0x26d97c:
- **States 2 and 3** (0x26db1c): the obstacle stop rule.
  - A running hold counter (+0xac, byte) is decremented. While it is still above 0, only the periodic check below can run. When it reaches 0, the check runs at once.
  - When no hold is running (or one just ended) and the record float +0x290 is 0, the driver resumes: +0x70 = +0x6c.
  - Otherwise the check runs only when +0xa4 % 5 = 0. The writer of +0xa4 is not read.
  - The check: a signal object (driver +0, with +0xaf bit 3 set) for which 0x2cdde4(signal, car) = 0 stops the car: +0x70 = 0 and hold 8. Then come the other entities: the list at 0x5863b4 + 0x335c (count at +0x3058) without the car itself, plus the player. One of them stops the car the same way when all three tests hold:
    - it is ahead (forward dot > 0);
    - the forward distance is below the sum of entity +0x74 (own), entity +0x74 (other), sqrt(driver +0x68) and 3;
    - the side distance |delta · own x axis| is below |0x20b214(other +0x6c, other matrix) · own x axis| + 0.5 + own +0x6c.
- **States 4 and 7**: no input.
- **State 5** (0x26dd2c): located, not read. It reads the player's vehicle (0x24255c), writes +0x70 from 0x20362c, tests the layer table 0x1ffbd8 and runs 0x23019c.
- **State 6** (0x26dee8): located, not read.
- **State 8** (0x26d998): rebuilds the car's matrix from driver +0x18 at the ground height (0x20af24) and moves it with 0x27872c and the frame time.
  - When the forward axis has a dot product below 0 with driver +0x38, it re-projects the car onto the path (0x2bf2ec with driver +4, +0xc and +0x64), sets the node and driver +0 = 0x20ff40(route) (the signal object), and calls the route type's function (table 0x3581d4, route byte +8) for driver +0x48 and +0x38.
  - Otherwise, within r of driver +0x48, it sets +0x79 = 2. r is 10 m when that dot is at least 0.9 and 4 m below.
  - Every path ends in 0x23019c.

States 0 and 1 and values above 8 give no input. The post step 0x16ce9c still runs 0x23019c every update.

Unknowns:
- Who moves driver +0x78 from 0 (set by the spawn) to 2 or 3, and the driver record writers in general (124 users of +0x2a4). The path follower 0x2bf2ec and the states 5 and 6.
- The values of the LOD table 0x529e30 (+0x3a0..+0x3c0) and of the radius table `*0x375f40`, both filled at run time, and the meaning of LOD class -1 for the spawn band (it would index +0x3a8 / +0x3b4).
- What 0x235498(0) does for a leaf without points (unsigned divide by 0).
- The writer of the pending route list +0xb9c / +0xd9c, the signal objects of 0x2dfc84, 0x2cdde4 and the layer table of 0x1ffbd8.
- The meaning of messages 0x80000005, 0x80000006 and 0x80000007 in the vehicle handler 0x159bd0, and how the spawn relates to the activation message 0x80000004 and the kinematic flags 0x102 (VEH-91).
