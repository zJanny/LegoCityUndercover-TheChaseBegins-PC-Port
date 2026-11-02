# Traffic: the junction routes and the AI driver

Subtopic of [traffic.md](traffic.md): the junction routes (route list, EnableTrafficPath, path follower, route type function, junction test) and the AI driver (record, input provider 0x26d934, state change 0x1cb148, post step 0x1bc79c, kinematic traffic cars). Feature file: [pedestrians-traffic-cars.md](../plans/features/pedestrians-traffic-cars.md).

## Current state

- The AI driver 0x26d934 switches on driver +0x78 (states 2 to 8). States 2 and 3 run the obstacle stop rule. State 5 steers 8 m ahead along the route. State 8 steers back to driver +0x18 and puts the car back on the route when it faces away. States 4, 6 and 7 give no input. [traffic.md §2026-10-20T11:18:14](traffic.md#ts-20261020-111814), [§2026-10-25T12:26:09](#ts-20261025-122609)
- The routes are the input and output paths of the `props/traffic_junction` objects within 9000 of the origin (0x2df574). EnableTrafficPath (0x2dece4) sets or clears a route's bit, or, before the routes are loaded, queues a disable in the pending list +0xb9c, which the room set-up replays. 0x2dfc84 pairs each route with the junctions it enters and leaves; 0x20ff40 gives the driver's junction. [§2026-10-25T12:19:38](#ts-20261025-121938)
- The port wires the routes into the traffic system: the junction objects of the level, the route list with the manager's bit set, a pregen point's route as its index in the list (no spawn without a junction within 9000), the pending replay and the native EnableTrafficPath (disables before the scene enter are queued). Junction lanes are not read. [§2026-11-02T23:46:29](#ts-20261102-234629)
- The path follower 0x2bf2ec takes, from the pregen cell under a point 8 units ahead of the car, the nearest point on an enabled route whose direction is less than 60 degrees off the car's forward axis, and returns its route, position and path parameter (point +0x20 and driver +0x64 are the parameter, not a speed). The route type function (table 0x3581d4) then samples the route there; all shipped routes are open type 0 paths (0x1c17c0). [§2026-10-25T12:19:38](#ts-20261025-121938)
- The junction test 0x2cdde4 stops a car on a junction lane while a lane paired with it in the junction's conflict list is listed at +0x284. [§2026-10-25T12:19:38](#ts-20261025-121938)
- Driver +0x78 is written only by the spawn (0) and by the state change 0x1cb148. Each update the vehicle update runs 0x1cb148, which copies the request +0x79 into +0x78. After a spawn this gives 0 → 1 → 2, one update per step (code and servant). State 3 (junction, next route +8) follows from 2 at the route end. State 4 follows a contact, 7 destroys the car, and 6 leads to 7 after the car has stood for 90 updates. [§2026-10-25T12:26:09](#ts-20261025-122609)
- The traffic post step 0x1bc79c replaces the input copy for AI cars. It moves the car in states 2 and 3 (0x2613ac) and takes the junction, the recovery after a hit (4 → 8) and the break (6 → 7). [§2026-10-25T12:26:09](#ts-20261025-122609)
- Traffic cars are kinematic Bullet bodies (flags 0x102) through 0x22a270 → 0x213c74 → body command 4: at creation, in state 1 and when they leave 8. They become dynamic in states 4 and 6 and at a contact. Message 0x80000005 does nothing, 0x80000006 stops the car's sounds and effects, 0x80000007 sets the node fade. [§2026-10-25T12:26:09](#ts-20261025-122609)

<a id="ts-20261025-121938"></a>

## 2026-10-25T12:19:38+01:00 — Route list, EnableTrafficPath, the path follower 0x2bf2ec, the route type function, junction pairs and the junction test 0x2cdde4 (llvm-objdump only, no REA session, no emulator)

Corrected by [traffic-ai-driver.md §2026-11-02T23:46:29](#ts-20261102-234629): the routes are wired into the traffic system (the "Not wired" line of the Port paragraph no longer holds).

[Corrects traffic.md §2026-10-20T11:18:14](traffic.md#ts-20261020-111814): the "route objects" of 0x2df574 are the `props/traffic_junction` objects; point +0x20 and driver +0x64 are the path parameter, not a speed; there are two route → junction lists.

Method: `llvm-objdump -d --triple=armv6k-none-eabi --mattr=+vfp2` on `artifacts/game/game.arm.elf`, words read from the ELF segments, worker 20261025-traffic-A. Shipped data: a throwaway probe over the levels and pregen files of the RomFS, then test `traffic-route-real-data`. Port: `src/runtime/traffic_route.*`, tests `traffic-route`, `traffic-route-real-data`.

**Junctions are the routes' owners.** The global list 0x515f40 (count `*0x374590`) holds the objects of class `props/traffic_junction` (string `trafficjunction` at 0x1d8910, record +0x7c of 0x428 bytes). Their set-up 0x1ca5a4 reads `InputPath01..08` and `OutputPath01..08` (name hashes, resolved with 0x22a3b0 to level path records): inputs are 40-byte entries at record +0, count byte +0x421; outputs at +0x140, count +0x422. It also builds lanes (96-byte entries at `*(+0x410)`, count +0x423, a path record at lane +8), conflict pairs of lane indices (bytes at `*(+0x414)`, count +0x420), `TrafficLight%02u` pairs (+0x2c0) and a light cycle (update 0x1d8924); these were only located. Shipped data: 16, 7, 2, 4, 4, 14 and 4 junctions in lvl_01, 03, 04, 05, 06, 07 and the test playground, all with `initialposition` within 9000 of the origin in x and z; every traffic pregen point lies on one of their paths, and all those paths are open type 0 paths.

**Route list (0x2df574, first part).** The count +0x2d4 is set to 0. 0x2c8718(origin, out, 32, 9000.0) takes the junctions of the global list in order whose x/z distance (0x22b62c) is below 9000 (strict), at most 32. With none it returns at once (bit 0 and the pregen data stay as they were). Otherwise, per junction, first its inputs, then its outputs: a path not yet in the list +0xc4 gets bit `count` of +0x2c4 set, `path record +0x24 = count`, and is appended. Bits are only set, never cleared here. The rest builds the bounding square of the junctions (centre, the largest x/z distance as u16 at +0x24) and loads the pregen file (bit 0 = its result).

**Route → junction pairs (0x2dfc84).** 0x2b4218 copies up to 128 junctions of the global list (no distance test). For each route of the list, each junction and each entry: an input entry equal to the route appends (route at +0x594, junction at +0x394, count +0x794); an output entry appends to +0x998 / +0x798, count +0xb98. No capacity test. The trailing empty double loops do nothing. **0x20ff40(route)** returns the junction of the first input pair of the route, else 0: the driver's +0 is the junction the car's road leads into (by the attribute names). Shipped data: 54 of 57, 26 of 30, 0 of 2, 4 of 4, 4 of 4, 36 of 38 and 13 of 13 pregen route paths are a junction input.

**EnableTrafficPath (native, handler 0x27bc30 → 0x2a7004 → 0x2dece4(manager, path, on)).** With bit 0 of +0x390 set: the path is searched in the route list; when found at i, bit i is set or cleared; otherwise nothing. With bit 0 clear: on = 1 does nothing; on = 0 searches the pending list +0xb9c (count +0xd9c) and appends the path unless it is there (no capacity test; the list has room for 128 words before its count). This is the only writer of the pending list. **Replay in the room set-up 0x2dedbc**, after 0x2df574 and 0x2dfc84: for each pending path, when bit 0 is set and the path is in the route list, its bit is cleared (with bit 0 clear the search finds the path in the pending list itself and nothing changes); then the count +0xd9c = 0.

**Path follower 0x2bf2ec(position, forward, out position, out route, stack: out parameter).** The cell is 0x1f47f0(manager, manager +0x24, position): the same descent as 0x1eb490, except that a cell without list and without children is returned instead of 0; its count is then 0. best = 9999.0 (0x2bf40c). For each point i of the cell list (hword +2, pointers at +0xc): route = point +0x18 (path record); skipped when route +0x24 is not below +0x2d4 or its bit of +0x2c4 is clear. delta = point − position, d = 0x257ea0(delta): the length, and delta becomes the unit vector, or (0, 0, 1) when the bits of the length are at most 0x34000000 (signed compare). Skipped unless best > d (`ble`, so a NaN is skipped). Then dot = 0x257e78(delta, forward); skipped when the bits of dot, compared as a signed integer, are at most 0x3f000000 (so dot must exceed 0.5; negative values fail). A pick sets best = d, out route = point +0x18, out position = point +0, out parameter = point +0x20 (the path parameter, [pregen-traffic.md §2026-10-20T11:16:20](pregen-traffic.md#ts-20261020-111620)), result 1. Ties keep the first. Nothing is written when no point passes. The caller in state 8 of 0x26d934 passes position = node translation + 8.0 (0x26dcfc) × the node's z axis (0x23b92c), forward = the z axis, out position driver +0xc, out route +4, out parameter +0x64.

**Route type function.** After a pick, state 8 calls `table 0x3581d4 [5 × route byte +8]` with (route +8, driver +0x48, driver +0x38, 1, s0 = driver +0x64). Table 0x3581d4 has the same words as 0x358f9c, 0x358d58 and 0x357e24: type 0 slot 0 = 0x1c17c0 (Catmull-Rom, unit tangent), type 1 slot 0 = 0x1b33fc (linear); the words after the tenth are text, so only types 0 and 1 exist. Driver +0x48 is the path position at the parameter, +0x38 the unit tangent there. All shipped traffic routes are open type 0 paths, so 0x1c17c0 is the only entry they use.

**Junction test 0x2cdde4(junction, car).** k = the first lane i (below +0x423) with `*(+0x410) + 8 + 96 i` equal to the car's route (driver +4); k = -1 when none. For each conflict pair j (below +0x420): when byte 0 equals k the partner is byte 1, else when byte 1 equals k the partner is byte 0, else the pair is skipped (the bytes are zero-extended, so k = -1 matches none; the `cmn #1` test of the partner is dead). For each entry m (below +0x424) of the 8-byte list at +0x284: when (word 0 − `*(+0x410)`) / 96 (signed, `smull` with 0x2aaaaaab, `asr #4`) equals the partner, the result is 0. Otherwise 1. So a car on a lane stops while a conflicting lane is listed; a car on any other path (its road into the junction) is never stopped by it. In state 0 the light update 0x1d8924 moves to the next phase only while the +0x424 count is 0 (located, not read in full).

**Port.** `collectTrafficRoutes`, `trafficRouteIndex`, `trafficRouteEnabled`, `enableTrafficPath`, `applyPendingTrafficRoutes`, `linkTrafficRouteSignals`, `trafficSignalForRoute`, `trafficJunctionAllowsCar`, `trafficRouteCell`, `findTrafficRoutePoint`, `sampleTrafficRoute` (reuses `pedestrianLevelPath`, the same table). Routes are named by the name hash of their path. Stand-ins: TRAFFIC-S3 (a path missing from the route list counts as disabled; the original reads the record's stored +0x24, never reached by shipped points), TRAFFIC-S4 (the route list +0xc4, which holds 128 words before the bit set +0x2c4, and the pending and pair lists refuse a 129th entry; the original writes past them). Not wired; the manager's own route bits (`TrafficManager::setRouteEnabled`) are separate from `TrafficRouteTable` (TRAFFIC-5).

Unknowns:
- The lane construction of 0x1ca5a4 (which input and output each lane joins, its path, the conflict pairs), the writer of the +0x284 list and its second word, and the light cycle 0x1d8924 (TRAFFIC-4).
- Who moves a car's route (driver +4) from its road onto a lane and on to the next road; only the spawn and state 8 write it among the code read.
- The stored value of path record +0x24 in the level files (not decoded; no shipped traffic point needs it).

<a id="ts-20261025-122609"></a>

## 2026-10-25T12:26:09+01:00 — The AI driver: record, states 5 and 6, the state change 0x1cb148, the post step 0x1bc79c, kinematic traffic cars (llvm-objdump, servant measurements)

Method: `llvm-objdump -d --triple=armv6k-none-eabi --mattr=+vfp2` on `artifacts/game/game.arm.elf`, words read from the ELF segments (agent 20261025-traffic-B); no REA session. Measurements: 20261025-traffic-servant, idle job TRAFFIC-2, recorded from two save states (on a street with traffic after mission 1-1; the player in a car behind other cars on a street), 2026-10-25, RPC reads of the ten driver records of the manager 0x527410. Port: `src/runtime/ai_driver_input.*`, test `ai-driver-input`. [Corrects traffic.md §2026-10-20T11:18:14](traffic.md#ts-20261020-111814): driver +0x64 is the distance along the route (the route functions take it as their parameter), not a speed; state 8 does not move the car with 0x27872c, it steers it toward driver +0x18.

**The record (0x1796e8).** The traffic manager's level load calls it for every `Traffic%02u` car. It allocates 176 bytes with 0x25c030(176, 1, 1); the third argument makes the allocator zero-fill them (0x24d234). Then +0x74 = attribute `TrafficHeightOffRoad` (0x241280, default 1.0), +0x18 = the node matrix, +0xaf |= 1, and 0x22a270(entity, 1) runs. Layout: +0 signal object, +4 route, +8 next route, +0xc target point, +0x18 a matrix (rows +0x18, +0x28, +0x38, +0x48), +0x58 the node position at the last entry to state 2, 5 or 8, +0x64 distance along the route, +0x68 speed, +0x6c cruise speed, +0x70 target speed, +0x74 height offset, +0x78 state, +0x79 requested state, +0xa4 updates in the state (0x27872c also clears it), +0xa8 time of the last contact, +0xac hold, +0xad keep counter, +0xae a counter of 0x2613ac, +0xaf flags. 0x20af24(entity) = driver +0x74 − body +0x54.

**Who moves +0x78.** Only two places write it: the spawn 0x2def8c (0) and the state change 0x1cb148, which copies +0x79 into it. The writers of +0x79 are:
- the spawn: 1;
- 0x1cb148: 2 at the end of state 1, and 3 on entering 2 or 5 at the route end;
- the route mover 0x2613ac: 0x261594, after 0x2cb1e8, not read further;
- 0x1bc79c: 2 from state 3, 8 from 4, 7 from 6;
- 0x26d934: 2 from state 8;
- the contact message 0x80000002 (0x179d30): 4 while in 2 or 3;
- 0x22a1f4: 0. That is the car held in the global 0x3745d4: it also clears +0xaf bit 0 and sets the vehicle request +4 = 4.

The vehicle update 0x1571dc calls 0x1cb148 at 0x1574a4 for every car with +0xaf bit 0, after the input provider. The servant saw the same order after each spawn: +0x78 / +0x79 = 0 / 1 for less than one update, 1 / 2 for one update (33 ms at 30 updates per second), then 2 / 2. Live cars stay in 2 / 2 (98 to 99 percent of the samples). With the player in a car behind other cars, cars went from 2 / 2 to 3 / 3 several seconds after their spawn, with +0xaf = 9 (bit 3). One went back to 2. A removed car keeps its +0x78 / +0x79.

**State change 0x1cb148.** When +0x78 ≠ +0x79:
1. Leaving a state:
   - leaving 5: 0x230dbc(entity, 0), 0x230ef4, 0x230e60(…, entity, 0, 0) (a list at `*(*0x230e58 + 0x24)` +0x30c), and +0xaf bit 4 cleared;
   - leaving 8: 0x22a270(entity, 1).
2. Entering the requested state:
   - 1: +0xaf bit 3 cleared. +0x6c = 8 + 0x2345dc() × 4. Record +0x30d = +0x30e and +0x30c = 100. The brake input (record +0x290) = 0. +0xc = the route point at +0x64 + 0.1 (route function +0, position only), with y + 0x20af24, then 0x20ae2c and +0xae = 0. Rows 0..2 of +0x18 = the rigid body's basis transposed: 0x1cb244..0x1cb2b0 builds on the stack row 0 = (rigid +4, +0x14, +0x24), row 1 = (+8, +0x18, +0x28), row 2 = (+0xc, +0x1c, +0x2c), and 0x232788 is a plain copy of three rows, so +0x48 keeps the spawn position. +0xad = 0x235498(60) + 30. Entity +0xc bit 0x20000 cleared. Then 0x22a270(entity, 1).
   - 2: +0x6c drawn as for 1, then the same as 5.
   - 5: with +0xaf bit 4 clear, first 0x230dbc(entity, 1), 0x230ef4, 0x230e60(…, entity, 1, 0). Then +0x58 = the node position. When +0x64 ≥ the route length (hword +10), +0x64 = the length. In that case, when also +0x70 > 0 and 0x2cb1e8(entity, &out) returns true: out ≠ 0 gives +0x79 = 3, out = 0 gives 0x21d6b8 (the manager removes the car). Finally +0xc is refreshed as for 1.
   - 4: 0x22a080 (leaves the signal queue when bit 3 is set, and clears bit 3), then 0x22a270(entity, 0).
   - 6: 0x22a270(entity, 0), 0x229be4(entity, 1), +0xa4 = 0 (0x1cb54c, overwritten by the change anyway), 0x23cee8(*0x375fe0, entity, entity, 0, 1), a second 0x23cee8 only when *0x376c78 ≠ 0 (0x1cb568..0x1cb594), then 0x22a124(entity, 0x11f).
   - 7: message 0 to itself with word +0xc = 1000, the destroy.
   - 8: +0x58 = the node position.
   - 0 and 3: nothing.
3. Then +0xa4 = 0 and +0x78 = +0x79. The copy reads +0x79 after the entry, so a 3 requested on entering 2 is taken at once.

Every update, by the current state:
- 1: +0x79 = 2.
- 4: brake and hand brake inputs (record +0x290, +0x294) = 1.0 while 0x229948(body) > 0. 0x2308e4(rigid, 1) runs when rigid +0xd8 is 2 or 5.
- 5: when +0xa4 & 63 = 0 and +0xaf bit 1 is set, 0x1bc660 runs at the node point ((r − 0.5) × 2, 0.5, −2) (r = 0x2345dc).
- Then +0xa4 += 1.

**Input provider 0x26d934** (jump table 0x26d97c, index +0x78 − 2):
- **States 2 and 3**, as read before, with three details:
  - the period check uses an unsigned +0xa4 % 5;
  - while a hold is still running, the resume is skipped but the period check still runs;
  - each entity that stops the car sets +0x70 = 0 and hold 8, and the loop goes on. The forward and side limits use entity +0x6c..+0x74, the half extents (own +0x74 + other +0x74 + sqrt(+0x68) + 3 forward; |rotated other half · own x| + 0.5 + own +0x6c sideways).
  - The state does not call 0x23019c. The post step moves the car instead.
- **State 5** (0x26dd2c):
  1. When the player is in a vehicle (0x24255c(*0x3772b8) ≠ 0): +0x70 = min(+0x6c, 0x20362c(entity +0x84) × k). k is 1.0 with +0xaf bit 2 set and 0.8 without it.
  2. When body +0x7c +0xcc has bit 0x200 and 0x1ffbd8(entity byte +0x18, room byte +0x3a) is true, bit 0x200 is cleared.
  3. t0 = route function +0xc (nearest sample to the node position, 10 samples, scale 1.0). t1 = route function +8 (8 m ahead of t0 in steps of 0.5). Route function +0 gives the point and tangent at t1. 0x237a0c builds a matrix from them (row 2 = tangent, row 3 = point).
  4. +0x64 = t1 and +0x18 = that matrix.
  5. The matrix with y + 0x20af24 goes to 0x27872c with dt, then 0x23019c runs. A running hold counts down by 1.
- **State 6** (0x26dee8) tail-calls the empty 0x1d8df0: no input, like states 4 and 7.
- **State 8**:
  1. 0x27872c steers toward a copy of +0x18 with y + 0x20af24.
  2. When dot(node z, +0x38) < 0: 0x2bf2ec runs from the probe = node position + 8 × node z. When it succeeds, the node gets the matrix rebuilt by 0x237a0c from the follower's row 2 (0x24c118). Then +0 = 0x20ff40(route), +0x48 / +0x38 = the route point and tangent at +0x64, and +0x18 is rebuilt.
  3. Otherwise, within r of +0x48 (r = 10 when the dot is at least 0.9, compared as integer bits, else 4), +0x79 = 2.
  4. Every path calls 0x23019c once.

**Post step 0x1bc79c.** The post step 0x16ce9c calls it instead of 0x23019c when +0xaf bit 0 is set, and sets body +0xc4 bit 0x100000 (it clears that bit otherwise). In order:
1. 0x2c87a8() ≠ 0 gives +0x70 = 0.
2. 0x278b78(entity, 1 when +0x70 or +0x68 is 0, else 0, 0, dt) (the lights).
3. By state:
   - 0: node matrix = +0x18.
   - 2: when +0x64 ≥ length − 2, the stop test runs: 0x2ca504 true gives brake input 1.0 and +0x70 = 0. Otherwise, with hold 0 and a brake input of exactly 1.0, the brake input becomes 0. Then the route mover 0x2613ac runs.
   - 3: the same stop test without the length check, then 0x2613ac. When +0x64 > 2.8 (integer bits), these follow in order: 0x22a080, +4 = +8, +0 = 0x20ff40(+4), +0x79 = 2, +0x64 = 0x2345dc() × 0.4.
   - 4: when 0x230174(body) < 0.5 and 0x2355c0() − +0xa8 ≥ 0.5, the follower runs as in state 8 and the node is placed on the route. With body +0xda0 ≥ body +0x2b4 − 1 (signed; wheels on the ground against the wheel count), +0x79 = 8. Then 0x23019c runs.
   - 5 and 8: +0x68 = 0x229948(body), 0x242518(body, *0x376c40) (gravity back to the global, −39.0 in the data image), then 0x29cc38.
   - 6: 0x23019c. When 0x229948(body) ≤ 0.1 (bits) and +0xa4 ≥ 90, +0x79 = 7.
4. When 0x23de70(entity) (a node flag test) is true, +0xad counts down to 0. Otherwise +0xad is raised to at least 30.

**Kinematic traffic cars (VEH-91, traffic part).** 0x22a270(entity, on) calls 0x213c74(body, on).
- With on: gravity 0 (0x242518), rigid activation state 4 (0x1e3ccc), the wheel offsets +0x398 refreshed, body +0xc4 = (+0xc4 & ~0x8000) | 0x20. While the rigid body is not yet kinematic, its transform is reset from the node (body vtable +0x18), and command 4 of the body queue goes to the world 0x5863b4 (0x2432d8 → 0x1fb648).
- The queue handler 0x2e3390 runs command 4 at 0x2e3760: the rigid flags +0xcc lose bit 1 and get bit 2, so 0x100 (set by the body build 0x243398) becomes 0x102.
- With off: activation state 1, the global gravity, +0xc4 bit 0x20 cleared, and command 3 (0x23f0fc) makes the body dynamic.
- So a traffic car is kinematic from its creation, and again from state 1 and from leaving state 8. It becomes dynamic in states 4 and 6. The contact message 0x80000002 also makes a kinematic car dynamic at once (0x179d40..0x179d7c): through 0x2e2f50 (clears flags 3) when the sender's body has +0xc4 bit 0x20000, else through command 3.

**Car messages in the vehicle handler** (dispatch 0x179850..0x1798b0):
- 0x80000004 (0x17a618): 0x243300(body, 1) unless entity +0x10 has bit 0.
- 0x80000005 (0x17a348): only 0x22dac8 with "%s enabled". 0x22dac8 is a bare `bx lr`, so the message does nothing.
- 0x80000006 (0x17a368): 0x22e620(entity, 1), record +0x2d4 = s18, the sounds +0x2ac (2) and effects +0x2c4 (4) and +0x2e4 stopped, and 0x22a1f4.
- 0x80000007 (0x17a5d0): 0x23f124(node, fade byte, −1, 1).
- 0x80000003 (0x17a648): 0x243300(body, 0) under the conditions already read.

Unknowns:
- The route mover 0x2613ac (states 2 and 3, and its own request of state 3), the stop test 0x2ca504, 0x29cc38, the lights 0x278b78, 0x23de70, and the steering controller 0x27872c (it also reads +0x7a / +0x7c and clears +0xa4 at 90 updates).
- The junction choice 0x2cb1e8 (signal connection table +0x418 / +0x310, which output becomes driver +8), the signal queue 0x22a080 (the junction test 0x2cdde4 is read in [§2026-10-25T12:19:38](#ts-20261025-121938)), and the list of 0x230dbc / 0x230e60 (who sets +0xaf bit 4).
- The state 6 calls 0x229be4, 0x23cee8 and 0x22a124 (0x11f), 0x1bc660, 0x2308e4, 0x20ae2c and 0x20362c. Who writes +0xaf bits 1 and 2. What 0x2c87a8 tests.
- The other creators of a driver record (stores to +0x2a4 at 0x1afe50, 0x262794, 0x290de8, 0x2a60c4); which cars they serve is not read.

<a id="ts-20261102-234629"></a>

## 2026-11-02T23:46:29+01:00 — The routes wired into the traffic system: junction list, route index of a point, EnableTrafficPath (llvm-objdump re-read of 0x27bc30 and 0x2a7004, no REA session, no emulator)

[Corrects traffic-ai-driver.md §2026-10-25T12:19:38](#ts-20261025-121938): the routes are no longer "not wired"; the manager's route bits are the `TrafficRouteTable` bit set. [Corrects traffic.md §2026-10-25T12:35:43](traffic.md#ts-20261025-123543): the port numbers the points' paths by the route list, not by first use.

Method: `llvm-objdump -d --triple=armv6k-none-eabi --mattr=+vfp2` on `artifacts/game/game.arm.elf` for the native handler, worker 20261102-traffic-A; the rest is the port's own wiring over the code read in the entry above. Port: `src/runtime/traffic_route_setup.*` (own file), `TrafficManager::routeTable`, `TrafficSystem::enablePath` / `routes`, the native in `natives_progress.*`; tests `traffic-route-setup`, `traffic-route-setup-real-data`, `traffic-system`, `traffic-manager`, `natives-progress`, `game-traffic`, `game-traffic-path`.

**The native.** The script function list (`levels/scriptfns.txt`) gives EnableTrafficPath two arguments: a `path` ("traffic path") and a number (1 = enable, 0 = disable). Handler 0x27bc30 reads word 0 of the second argument and calls 0x2a7004(first argument word, word != 0), which tail-calls 0x2dece4 with the traffic manager 0x527410. The port names the path by its name hash (case-insensitive, as every path name) and takes the second argument as its truncated integer.

**What the port does now.**
- The junction list (global list 0x515f40) is taken from the objects of class `props/traffic_junction` of the loaded levels (`trafficJunctions`): `initialposition`, `InputPath01..08`, `OutputPath01..08` (name hashes, empty slots skipped).
- The room set-up runs the route part of 0x2df574 (`collectTrafficRoutes`) and the pairs 0x2dfc84 (`linkTrafficRouteSignals`). Only with at least one junction within 9000 does it load the pregen file and set bit 0 (`table.loaded`); without a junction nothing spawns, as the original returns before the pregen load. Then the pending list is replayed (0x2dedbc).
- A pregen point's route is the index of its path in the route list (path record +0x24 as 0x2df574 writes it), replacing the port's earlier numbering by first use. The spawn test reads the bit of that index. Every shipped traffic point lies on a listed path, so no shipped point is skipped. The route counts per shipped level (lvl_01 58, lvl_03 31, lvl_04 3, lvl_05 5, lvl_06 4, lvl_07 40, test playground 14) are those of the entry above, reproduced by the setup test; `chase-game` in lvl_01 now prints 58 routes where it printed 57 numbered paths before.
- EnableTrafficPath calls are recorded by the progress natives and handed to the traffic system in call order before its next scene enter or update. Calls before the scene enter are queued (disables) and replayed by it, as 0x2dece4 does with bit 0 clear. Example in the shipped data: `3-7_MissionStart` of lvl_03 disables `RoadPath27` to `RoadPath30` before the traffic set-up (run of `chase-game`, 20261102-traffic-A).

Stand-ins: TRAFFIC-S9 (junction lanes, conflicts and listed lanes are not read, so `trafficJunctionAllowsCar` never stops a car), TRAFFIC-S10 (every junction object of the level is taken, where the original's list holds the created objects only; the native's calls take effect at the next traffic update, not at the call).

Unknowns:
- The initial value of path record +0x24 in the level files and what a spawn does for a path outside the list (TRAFFIC-14).
- Which junction objects the global list holds (creation flags, rooms not streamed in) and whether a native call and the traffic update can interleave inside one frame (TRAFFIC-15).
