# Traffic: the route functions ahead and nearest

Subtopic of [traffic.md](traffic.md); the AI driver and the route mover that call them are in [traffic-ai-driver.md](traffic-ai-driver.md), the junction lanes they run on in [traffic-junction.md](traffic-junction.md). This file: slots +8 and +0xc of the route type table 0x3581d4.

## Current state

- The route type table 0x3581d4 has five words per path type byte (path record +8). Its words are the same as those of 0x357f34 and 0x358ed8. Type 0 (Catmull-Rom): 0x1c17c0, 0x293540, 0x29fb6c, 0x21d5b0, 0x29fa64. Type 1 (linear): 0x1b33fc, 0x293494, 0x29fa58, 0x2878f0, 0x29fa4c. Every shipped traffic route and every junction lane is an open type 0 path. [§2026-11-07T18:14:58](#ts-20261107-181458)
- Slot +8 `ahead` (type 0: 0x29fb6c) walks the parameter on in fixed steps until the walked length reaches the asked length or the end of the path. Below an asked length of 100 it sums the squared step lengths and compares them with the length squared, so the AI driver's "8 ahead" walks until the squared steps add up to 64. [§2026-11-07T18:14:58](#ts-20261107-181458)
- Slot +0xc `nearest` (type 0: 0x21d5b0) returns the sample k / samples, from k = 1, nearest to a position; parameter 0 itself is never sampled. Type 1 has stubs that return 0 in both slots. [§2026-11-07T18:14:58](#ts-20261107-181458)
- Port: `src/runtime/traffic_route_functions.*` (tests `traffic-route-functions`, `traffic-route-functions-real-data`), registered in `trafficRouteFunctions`; the AI driver is not connected to them yet (TRAFFIC-2).

<a id="ts-20261107-181458"></a>

## 2026-11-07T18:14:58+01:00 — The route functions ahead 0x29fb6c and nearest 0x21d5b0 of table 0x3581d4 (llvm-objdump only, no REA session, no emulator)

Method: `llvm-objdump -d --triple=armv6k-none-eabi --mattr=+vfp2` on `artifacts/game/game.arm.elf`, words read from the ELF segments, worker 20261107-traffic2-B, 2026-11-07. Functions read: 0x29fb6c, 0x21d5b0, 0x29fa64, 0x29fa58, 0x2878f0, 0x29fa4c, the helpers 0x2536a8, 0x24ef70, 0x2507dc, and the calls of slots +8 and +0xc in state 5 of 0x26d934 (0x26de0c..0x26de54). Table words read at 0x3581d4, 0x357f34 and 0x358ed8. Port: `src/runtime/traffic_route_functions.*`, tests `traffic-route-functions`, `traffic-route-functions-real-data`. This answers the open point on slots +8 and +0xc in [traffic-junction.md §2026-11-07T12:09:40](traffic-junction.md#ts-20261107-120940).

**The table.** 0x3581d4, 0x357f34 and 0x358ed8 hold the same ten words, then text (0x3581fc: `Traf…`) or other data. Per type, in slot order:
- type 0: 0x1c17c0 (position and tangent), 0x293540 (the chord sum the path length 0x22a6c0 uses), 0x29fb6c (ahead), 0x21d5b0 (nearest), 0x29fa64 (nearest with another length function);
- type 1: 0x1b33fc, 0x293494, 0x29fa58 (`vldr s0, =0.0; bx lr`), 0x2878f0 (the same stub), 0x29fa4c (the same stub).

**The callers in state 5 of 0x26d934.** The driver's route (driver +4) gives the type byte (route +8). Slot +0xc is called as (route +8, r1 = node position (0x250810 +0x30), r2 = 10, r3 = 1, s0 = 1.0). Slot +8 is called as (route +8, s0 = 8.0 (0x26dcfc), s1 = 0.5, s2 = the result of slot +0xc). Slot 0 then gives the point and tangent at the result of slot +8. This is the order `ai_driver_input` already has ([traffic-ai-driver.md §2026-10-25T12:26:09](traffic-ai-driver.md#ts-20261025-122609)). The junction choice 0x2cb1e8 calls 0x21d5b0 directly on the lane with (node position, 10, 1, 1.0).

**Nearest 0x21d5b0(path, r1 position, r2 samples, r3 flag, s0 scale).**
- inverse = 1.0 / scale (`vdiv`). best = 0x7f7fffff (the largest float), result = 0.0.
- total = n × samples as a 32-bit product, where n is the point count (hword +2) for a closed path (byte +1 set) and the count − 1 for an open one. With total below 1 (unsigned) the result is 0.
- For k = 1 .. total: parameter = float(k) / float(samples) (both converted from u32). Slot 0 of the path's type (table 0x358ed8) runs with (path, sp, no tangent, r3 = the caller's flag, parameter). delta = (sample − position) × inverse, per axis (subtract, then multiply). d = 0x2536a8(delta) = sqrt((x x + y y) + z z) (`vmla`, not fused).
- When d < best (`movlo`, so neither equal values nor NaN), best = d and result = parameter. Of equal distances the first stays.
- Parameter 0 is never sampled: a car standing on the first point of a path gets 1 / samples.

0x29fa64 is the same function with 0x20e968 in place of 0x2536a8; no caller was looked for.

**Ahead 0x29fb6c(path, s0 ahead, s1 step, s2 from).**
- limit = float(n) (u32 to float), where n is the point count for a closed path and the count − 1 for an open one.
- When 100.0 (0x29fcc8) > ahead, ahead becomes ahead × ahead and the loop sums the squared distances 0x24ef70 = (dx dx + dy dy) + dz dz. Otherwise it sums the distances 0x2507dc, the square root of the same sum. For NaN the second loop runs and stops at once.
- walked = 0.0 (0x29fcc4), parameter = from. While walked < ahead and parameter < limit (each `bhs` leaves the loop on greater or equal and on NaN), the loop does three things. It samples the position at the parameter (slot 0 of table 0x358ed8, no tangent, flag 1). It adds step to the parameter and samples again. It adds the distance between the two samples to walked.
- The result is the parameter. It can pass the limit by less than one step, and a start at or past the limit comes back unchanged.
- With the driver's values (8, 0.5), the squared loop runs. On a road with points 10 apart each step is 5 long (25 squared), so the parameter moves three steps (1.5 points, 15 units) ahead, not 8 units.

**The flag r3 = 1** passes on to slot 0, where it applies the path's node transform (record +0x18). No traffic route or lane has one: only the natives EffectAttachParticlesToPath and EffectLinkPathToObject set it, and no script calls them ([pregen-traffic.md](pregen-traffic.md), `pedestrianLevelPath`).

**Port.**
- `trafficRouteAhead` and `trafficRouteNearest` run the type 0 functions on the type 0 position of `sampleDirectorPath`, and the stubs (0) for type 1. They throw for any other type (the original would jump through text) and for a type 0 path without points (the original's sampler writes nothing, and the unsigned count − 1 would make the loops run on for a long time).
- `trafficRouteFunctions` sets `ahead` and `nearest` on the junctions' road and lane paths. Only `follow` (the path follower 0x2bf2ec) is still unset, and it belongs to connecting the driver (TRAFFIC-2).
- lvl_01: the 57 routes and 97 lanes are all open type 0 paths. Each of their 1718 points from index 1 on is nearest to itself at its own index (10 samples, scale 1). Ahead from 0 moves in whole steps of 0.5 and stays at most one step past the end (`traffic-route-functions-real-data`).

Unknowns:
- The callers of slot 1 (0x293540 / 0x293494) and slot 4 (0x29fa64) among the traffic code, and 0x20e968 (used by 0x29fa64; [ai-states.md](ai-states.md) reads it as an x/z length).
- No oracle measurement of the two functions: the values are read from the code and checked on synthetic and level paths.
