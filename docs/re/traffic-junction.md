# Traffic: the junction class and its lights

Subtopic of [traffic.md](traffic.md): the `props/traffic_junction` objects (type 192), their set-up 0x1ca5a4 (ends, sides, choice table, lanes, conflict pairs, light pairs), the `props/prop_trafficlight` objects (type 204) and the light cycle 0x1d8924. The route list, the junction choice and the queue that use these records are in [traffic-ai-driver.md](traffic-ai-driver.md). Feature file: [pedestrians-traffic-cars-movement.md](../plans/features/pedestrians-traffic-cars-movement.md).

## Current state

- A junction's inputs and outputs are the level paths named by `InputPath01..08` / `OutputPath01..08`; an empty name or a name without a path is skipped (0x22a3b0 returns 0). An input ends at its last point, an output starts at its first; each keeps the x and z of its unit travel direction there. Two parallel ends (direction dot at least 0.9) get the sides 1 and 2 (0x159ab8). [§2026-11-03T08:53:43](#ts-20261103-085343)
- Each input may leave by every output that does not turn back (dot above -0.85); with sides, a sided input keeps to its side and turns only to its side's direction; with one input and one output they always join. Every choice is a lane: a 4-point type 0 path whose inner points are a cubic Hermite curve (0x20b0c0) between the two ends. Two lanes with different ends conflict when segment i of one and segment i of the other (the same index) come within 0.1 in x and z. [§2026-11-03T08:53:43](#ts-20261103-085343), [§2026-11-07T11:05:36](#ts-20261107-110536)
- `TrafficLight01..08` name `props/prop_trafficlight` objects; each found one makes a light pair with its `SlaveLight`. A light lists the roads of its `Path01..04`. The cycle starts with pair 0 green: green 6 s, yellow 2 s, all red 3 s, and the next pair turns green once no car is queued in the junction. A car waits at a light that lists its road unless that light's pair is the current one in green or yellow. [§2026-11-03T08:53:43](#ts-20261103-085343)
- The port fills the junction records from the level (`traffic_junction_fill.*`) and runs the light cycle as a pure step; neither is wired into the traffic system yet. The route list now skips path names without a level path, so every listed route is a pregen point path. [§2026-11-03T08:53:43](#ts-20261103-085343) A path name is looked up by its part after the last `.` (0x23895c); the port hashes the whole name here, in the DCam paths and in the pedestrian path count (TRAFFIC-21). [§2026-11-07T11:05:36](#ts-20261107-110536)

<a id="ts-20261103-085343"></a>

## 2026-11-03T08:53:43+01:00 — The junction set-up 0x1ca5a4, its lanes, conflicts and lights, and the light cycle 0x1d8924 (llvm-objdump only, no REA session, no emulator)

Corrected by [traffic-junction.md §2026-11-07T11:05:36](#ts-20261107-110536): the conflict test compares segment i of one lane only with segment i of the other, not every pair of segments; the shipped conflict counts are 25, 49, 0, 0, 0, 11 and 2, at most 20 in one junction, and lvl_01 `Junction1` has the one pair 1-2.

[Corrects traffic-ai-driver.md §2026-11-02T23:46:29](traffic-ai-driver.md#ts-20261102-234629): the port's route list held one bogus route per level built from empty or unknown junction path names; the shipped route counts are 57, 30, 2, 4, 4, 38 and 13, not 58, 31, 3, 5, 4, 40 and 14.

Method: `llvm-objdump -d --triple=armv6k-none-eabi --mattr=+vfp2` on `artifacts/game/game.arm.elf`, words and strings read from the ELF segments, worker 20261103-large-A, 2026-11-03. Functions read: 0x1ca5a4, 0x1d8924, 0x1d889c, 0x19ff70, 0x1afae8, 0x159ab8, 0x20b0c0, 0x20af64, 0x20af40, 0x20b19c, 0x22a3b0, 0x2a553c, 0x34cdf0 and the helpers 0x23f14c, 0x2408bc, 0x2501ac, 0x2554c0, 0x257f14, 0x257ea0, 0x257e78, 0x248850, 0x22b62c, 0x23b92c, 0x2408f8, 0x2408c4, 0x25c030 (with 0x24d234), 0x25c1b8. Class types from `ObjectType` of `levels/props/traffic_junction.blvl` (192) and `levels/props/prop_trafficlight.blvl` (204). Shipped data: a throwaway probe over the levels, then test `traffic-junction-fill-real-data`. Port: `src/runtime/traffic_junction_fill.*`, tests `traffic-junction-fill`, `traffic-junction-fill-real-data`. This answers the unknowns of [traffic-ai-driver.md §2026-11-02T23:44:38](traffic-ai-driver.md#ts-20261102-234438) on the writers of the choice table, the lane ends, the light pairs, the lights' route words and +0x300 / +0x308, and the lane construction of [§2026-10-25T12:19:38](traffic-ai-driver.md#ts-20261025-121938).

**Classes.** The junction descriptor is built at 0x1bc31c (`mov r0,#192; bl 0x246240`): create 0x1d889c (+0), set-up 0x1ca5a4 (+4), 0x26137c (+8), update 0x1d8924 (+0xc), 0x2613a4 (+0x14). The create allocates the record +0x7c of 0x428 bytes with 0x25c030(size, 1, 1), which zero-fills (its third argument runs 0x24d234). The light descriptor is built at 0x19283c (`mov r0,#204`): create 0x1afae8 (+0), set-up 0x19ff70 (+4), 0x15e404 (+8), update 0x15aa70 (+0xc), 0x1bc38c (+0x14), 0x15aa58 (+0x18), 0x15a640 (+0x1c). The light create makes a zero-filled record of 276 bytes and stores the node parts `green` (+0x104), `yellow` (+0x108) and `red` (+0x10c) of 0x23645c, all three hidden by 0x2363d4(node, part, 0). 0x2a553c fills the global junction list 0x515f40 with every object of class byte +0x12 = 192 from the object list it is given (`[r0 + 0x28] + 0x24`, count +0x1c) and writes the count to 0x374590.

**Ends (0x1ca5d8..0x1ca7e0).** For slots 1..8 the string attribute `InputPath%02u` (0x241624, type flags 0x1000010) goes to 0x22a3b0(scene, name, 0). That function hashes the part of the name after its last `.` and returns the 40-byte path record of the scene's table (scene +0x24, records at +0x20, count hword +10) with that hash in word 0, or 0. A found path becomes input entry `count` (40 bytes at record +0, count byte +0x421) with byte +0x24 = 0; `OutputPath%02u` fills +0x140 (count +0x422) the same way. Then, for each input: +4 = the last point, +0x10 = the point before it, +0x1c = x and z of 0x257ea0(last − before) (the unit 3D vector, (0, 0, 1) when the bits of the length are at most 0x34000000; the x/z pair is not normalised again). For each output: +4 = the first point, +0x10 = the second, +0x1c from second − first.

**Sides 0x159ab8(entries, count)**, called for the inputs and then the outputs. For each pair a < b whose direction dot (0x23f14c, x0 x1 + z0 z1) is not below 0x3f666666 (0.9) as signed bits: d = (−b.z, b.x) · (a.point − b.point) in x and z. With d < 0, byte +0x24 of a = 1 and of b = 2; otherwise a = 2 and b = 1 (also for NaN). A later pair overwrites an earlier one.

**Light pairs (0x1ca800..0x1ca8d8).** For slots 1..8 the object attribute `TrafficLight%02u` (0x24242c, flags 0x4000010, the referenced instance) gives a light. When found, pair n (+0x2c0 + 8 n) = (light, the light's `SlaveLight` object or 0) and n grows. Then +0x304 = n, +0x300 = 0, byte +0x308 = 2 and +0x30c = *0x37459c (6.0). The look: inside the slot loop 0x20b19c(word at pair n + 4 × slot, 0, 0) runs twice for each found light; the index is the slot, not the pair's light, so with consecutive slots it reaches the light of pair 0, the slave of pair 1 and then words not written yet (0, skipped). After the loop 0x20b19c(light, 2, 0) runs for both lights of pair 0.

**Light set-up 0x19ff70.** For slots 1..4 the string attribute `Path%02u` is resolved with 0x22a3b0; a found path is appended to light record +0xf0 (word count +0x100). It also looks up `SlaveLight` and the slave's `SlaveLight` without keeping them, then tail-calls 0x1d82a0.

**Choice table (0x1ca8ec..0x1caae4).** Byte +0x423 = 0. For each input i:
- with exactly one input and one output the only choice is output 0;
- otherwise each output o in order: dot = input direction · output direction (x/z). It is skipped when the bits of dot, unsigned, are at least 0xbf59999a (dot at or below −0.85, turning back). With both sides set it is taken when they are equal. Otherwise it is taken when the bits of dot, signed, are above 0x3f666666 (straight on), or when the input's side is 0; else cross = in.x × out.z − in.z × out.x decides: above 0 it is taken for input side 2, otherwise (also NaN) for side 1;
- the choices go to the words at +0x310 + 32 i, their count to byte +0x418 + i, and +0x423 grows by it. (A perpendicular of the input direction is written to sp+0x248 and overwritten unused.)

**Lanes (0x1caae8..0x1cac88).** 96 × (+0x423) bytes from 0x25c030(size, 1, 1), zero-filled, at +0x410. Lane k is made for each input i and each of its choices in order:
- +0 = the input entry, +4 = the output entry;
- +8 is the lane's own path record: word 0 (name) 0, type byte +8 = 0 (type 0, Catmull-Rom 0x1c17c0), point count hword +10 = 4, point pointer +0xc = lane +0x30;
- points at +0x30, +0x3c, +0x48, +0x54: P0 = input +4, P3 = output +4, and P1, P2 = 0x20b0c0({P0, P3}, {T0, T1}, 1, out, t) at t = 0x3eaaaaab (1/3) and 0x3f2aaaab (2/3), with r = the x/z distance of P0 and P3 (0x22b62c), T0 = (in.x, 0, in.z) × r and T1 = (out.x, 0, out.z) × r (0x2501ac, 0x248850).

0x20b0c0(points, tangents, n, out, t) is a cubic Hermite step: with n ≤ 1 it uses segment 0 and t as given (with n > 1, segment (int)(t n) and t from 0x158fdc). With t2 = t t and t3 = t2 t: h00 = (t3 × 2 − t2 × 3) + 1, h10 = (t3 − t2 × 2) + t, h01 = t2 × 3 − t3 × 2, h11 = t3 − t2; out = P0 h00, then + T0 h10, + P1 h01, + T1 h11, each a separate multiply and add (`vmla`). So a lane is the Hermite curve from the input's end to the output's start, kept as four points of a Catmull-Rom path of length 4; this is the route length 4 of the mover's 0.15 step ([traffic-ai-driver.md §2026-11-02T23:44:38](traffic-ai-driver.md#ts-20261102-234438)), and the junction choice's direct call of the type 0 nearest-sample function matches the lane's type.

**Conflict pairs (0x1cac8c..0x1caea4).** For each lane pair a < b with different input entries and different output entries: for the segments i = 1..3 of a and j = 1..3 of b, 0x20af64(a[i−1], a[i], b[j−1], b[j], pa, pb); when it returns 1, the pair (a, b) is recorded and the search goes to the next b if the x/z squared distance 0x20af40(pa, pb) is at most 0.1 × 0.1 (`bhi` skips a larger distance and NaN). The pairs collect in 64 slots of 8 bytes at sp+8 with no capacity test (a 65th would overwrite the record pointer at sp+0x208). Then 2 × count bytes at +0x414 get the lane indices and byte +0x420 the count.

0x20af64(a0, a1, b0, b1, pa, pb): u = a1 − a0, v = b1 − b0, w = a0 − b0; den = (u·u)(v·v) − (u·v)². With den ≤ 0 it returns 0. s = ((u·v)(v·w) − (u·w)(v·v)) / den is refused when 0 > s or its bits exceed 1.0 (signed); t = ((u·u)(v·w) − (u·v)(u·w)) / den is refused when t < 0 or its bits exceed 1.0. Otherwise pa = a0 + u s (+12 = s), pb = b0 + v t (+12 = t), result 1.

**Light cycle 0x1d8924(object, dt in s0).** Only with +0x304 above 1 (unsigned):
- timer +0x30c −= dt, raised to 0 below 0; nothing more while it is not 0;
- at 0, by phase byte +0x308: 2 → 1 with 2.0 s (*0x374598); 1 → 0 with 3.0 s (*0x374594); 0 → 2 with 6.0 s (*0x37459c) and pair +0x300 = (pair + 1) % count (0x25c1b8) when the queue count +0x424 is 0, else it stays 0 with timer 0 and is checked again at the next update; any other value → 0 with 0;
- then 0x20b19c(light, new, old) for both lights of the (new) pair, and +0x300, +0x308, +0x30c are stored.

0x20b19c(light, new, old) hides the part of `old` and shows the part of `new` (0 → +0x10c `red`, 1 → +0x108 `yellow`, 2 → +0x104 `green`). The times 0x374594 = 3.0, 0x374598 = 2.0, 0x37459c = 6.0 are written by the static constructor 0x34cdf0 (relative entry 0x36a2cc of the constructor table); the other code users of 0x374590 read word 0, the junction count. So each pair is green 6 s, yellow 2 s, then every light is red 3 s; the next pair turns green once no car is queued. Only the current pair's lights change, so the other pairs keep their last part (red after their turn). With the stop test 0x2ca504 a car waits at a light listing its road unless the light's pair is +0x300 and the phase is not 0.

**Shipped data** (test `traffic-junction-fill-real-data`): junctions, lanes, conflict pairs, light pairs, lights and sided ends:

| Level | Junctions | Lanes | Conflicts | Pairs | Lights | Sided ends |
|---|---|---|---|---|---|---|
| lvl_01 | 16 | 97 | 66 | 27 | 38 | 48 |
| lvl_03 | 7 | 64 | 99 | 12 | 20 | 24 |
| lvl_04 | 2 | 0 | 0 | 0 | 0 | 0 |
| lvl_05 | 4 | 4 | 0 | 0 | 0 | 0 |
| lvl_06 | 4 | 4 | 0 | 0 | 0 | 0 |
| lvl_07 | 14 | 63 | 32 | 32 | 32 | 0 |
| test playground | 4 | 16 | 3 | 4 | 7 | 16 |

At most 40 conflict pairs and 18 lanes in one junction (lvl_03). The two lvl_04 junctions have no lane: each has one output and no input. Example, lvl_01 `Junction1`: inputs RoadPath02, RoadPath07, RoadPath57, outputs RoadPath01, RoadPath08, RoadPath51; each input leaves by the two other roads (6 lanes, conflicts 1-2, 1-5, 2-5); pair 0 is Trafficlight01 (RoadPath02), pair 1 is Trafficlight02 (RoadPath57) with its slave Trafficlight03 (RoadPath07). Junction and light path attributes hold empty names in every traffic level but lvl_05 and lvl_06, and `0` in three junctions; none of them names a level path.

**Port.**
- `traffic_junction_fill.*`: `trafficJunctionInputEnd`, `trafficJunctionOutputEnd`, `markTrafficJunctionSides`, `trafficJunctionChoices`, `trafficJunctionLanePath`, `trafficSegmentClosest`, `trafficLanesCross`, `fillTrafficJunction`, `trafficJunctionFills` (the level objects, paths by name hash, lights by object reference) and `runTrafficJunctionLights` (returns the 0x20b19c change).
- `TrafficJunctionRecord` holds the phase byte +0x308 (`lightPhase`, a bool before) and the timer +0x30c (`lightTimer`). A lane is named by a port id from 0xf1a00000 up (the original uses the record's address); its `LevelPath` carries the id as its name.
- `trafficJunctions` (the route set-up) now returns the fills' layouts: the path names without a level path are skipped, so the route list loses the bogus route of an empty name, and the junction layouts carry their lanes and conflict pairs.
- Stand-in TRAFFIC-S13: a junction path of fewer than two points, more than 64 conflict pairs and a lane id equal to a level path's name are refused; paths and lights are searched in all loaded levels (the original searches the object's scene). Not wired into the traffic system (TRAFFIC-19); the light look is not ported (TRAFFIC-20).

Unknowns:
- Which junction objects update and when (the caller of the class update, rooms not streamed in) and the order of the light cycle against the traffic update in a frame (TRAFFIC-19, TRAFFIC-15).
- The light class's own update 0x15aa70 (it follows an object at record +0x80) and the slots +8, +0x14, +0x18, +0x1c of both descriptors (TRAFFIC-20).
- Which scene 0x22a3b0 searches (`[object + 0x20]` through 0x23c188) and which objects 0x238354 finds for an object reference.

<a id="ts-20261107-110536"></a>

## 2026-11-07T11:05:36+01:00 — The conflict test compares same-index segments; the path name split 0x23895c (llvm-objdump only, no REA session, no emulator)

[Corrects traffic-junction.md §2026-11-03T08:53:43](#ts-20261103-085343): the conflict pairs, their shipped counts and the lvl_01 `Junction1` example.

Method: `llvm-objdump -d --triple=armv6k-none-eabi --mattr=+vfp2` on `artifacts/game/game.arm.elf`, re-read of 0x1cac8c..0x1caea4 and 0x23895c by worker 20261103-large-A, 2026-11-07, after the code review of the branch found the conflict loop mis-read. Shipped counts: a throwaway probe over `trafficJunctionFills`, then test `traffic-junction-fill-real-data`.

**Conflict loop (0x1cad30..0x1cadec).** r9 counts lane a's segments from 1 while it is below a's point count (hword +10 of the lane's path record, read at r8 + 2), r4 counts from 1 while it is below b's point count (r7 + 2). Both segment addresses come from r9 alone: r6 = 3 r9, r5 = 12 r9 − 12, so 0x20af64 gets a.points + r5, a.points + 4 r6, b.points + r5, b.points + 4 r6. r4 is used for no address. So segment i of a is tested only against segment i of b, the same test repeated (b's count − 1) times; with b's count at most 1 the inner loop is skipped, and with fewer points in b than in a the test reads past b's points (never with shipped lanes, which all have 4). The rest holds as read: different input and output entries, the hit of 0x20af64, the x/z squared distance at most 0.1 × 0.1, then the next b.

**Shipped data** (test `traffic-junction-fill-real-data`, re-pinned): conflict pairs lvl_01 25, lvl_03 49, lvl_04 0, lvl_05 0, lvl_06 0, lvl_07 11, test playground 2; at most 20 in one junction (lvl_03). The lanes, light pairs, lights and sided ends of the table in the entry above still hold. lvl_01 `Junction1` has one conflict pair, lanes 1-2.

**Path names (0x23895c).** 0x22a3b0 passes the name to 0x23895c(name, before, after): it scans the whole name for the last `.` (index r5, 0 when none). With r5 not 0 it copies the part after that `.` into the buffer that 0x24da28 hashes; with r5 = 0 (no `.`, or the only `.` first) it copies the whole name. The port hashes the whole name in the junction fill (`traffic_junction_fill.*`), in the DCam path lookup (`dcam.cpp`) and in the pedestrian path count of 0x2edc94 (`pedestrian_system.cpp`). No shipped result changes: every route the junctions list is a pregen point path. One shared helper in `chase_assets` would close all three (TRAFFIC-21).

**Port.** `trafficLanesCross` tests same-index segments only; it returns false when either lane has fewer than 2 points. Stand-in TRAFFIC-S13 now also covers a lane b with fewer points than a (refused, the original reads past b's points) and the whole-name hashing.

Unknowns:
- None new; the unknowns of the entry above stay open.
