# Traffic manager and the AI driver input

## 1. Scope

The traffic manager 0x527410 and the AI driver input of the spawned cars. The manager spawns and removes the level's `Traffic%02u` vehicles around the player, keeps the vehicle limit (`Traffic_MaxVehicles`, SetTrafficMaxVehicles) and fades cars in and out at `Traffic_FadeBound` boxes. The AI driver input 0x26d934 drives a spawned car along its route. Not covered: pedestrians (the pedestrian manager 0x51b33c, [pedestrians-traffic-pedestrians.md](pedestrians-traffic-pedestrians.md)), the pregen file format ([pedestrians-traffic.md](pedestrians-traffic.md)), the vehicle physics body ([vehicles-physics.md](vehicles-physics.md)), pursuit.

## 2. Status

IN_PROGRESS, updated 2026-10-20T11:20+02:00. The rules of the manager are read: level load, room set-up and prefill, update, spawn, fade, SetTrafficMaxVehicles. They are ported as `TrafficManager`, which is not yet wired into `chase-game` (TRAFFIC-1). How the vehicle update picks its input provider is read. The AI driver 0x26d934 is read only in part (TRAFFIC-2).

## 3. Verified behaviour

- Read from the code (test `traffic-manager`), all from [traffic.md §2026-10-20T11:18:14](../../re/traffic.md#ts-20261020-111814):
  - the level load: the vehicle limit is min(`Traffic_MaxVehicles` default 5, car count); the spawn order is shuffled with the game's RNG; every car starts disabled with fade 255;
  - SetTrafficMaxVehicles: the argument is truncated and compared unsigned with the count;
  - the update: removal of every car while traffic is off; one spawn per update while below the limit when the countdown reaches 0, with countdown (live + live / 2) x 3; one removal per update (room, driver state, fade and distance rules); cars with owner flag 0x20 are kept;
  - the spawn: 10 tries in the leaf, the route bit, the room test, the LOD distance band (0.7 x near to 1.75 x far, 0 to far in the prefill), 16 m route spacing in x and z;
  - the 64 prefill updates of the room set-up;
  - the fade inside a `Traffic_FadeBound` box.
- Read from the code, not ported: the vehicle update uses 0x26d934 for a car whose driver record has +0xaf bit 0, 0x29cd9c for the player's car and nothing for the others (0x1d8df0 is empty).

## 4. Stand-ins

- TRAFFIC-S1 The spawn cell, the route count, the LOD bands and the per-car facts (room tests, driver bytes, despawn radius, LOD class) are supplied by the caller of `TrafficManager`. The pregen decoder now exists (`src/assets/pregen_traffic.*`: `parsePregenSpawns`, `findPregenSpawnCell`, [pregen-traffic.md](../../re/pregen-traffic.md)), but the manager is not fed from it yet; the LOD table 0x529e30 and the radius table `*0x375f40` are not read. needs: TRAFFIC-1. evidence: [traffic.md §2026-10-20T11:18:14](../../re/traffic.md#ts-20261020-111814)
- TRAFFIC-S2 Two cases the original leaves unread are refused by the port's own choice: an empty leaf makes the spawn fail (the original would compute random % 0), and a LOD class outside 0..2 throws (the original's -1 indexes the table one entry low). needs: TRAFFIC-3. evidence: [traffic.md §2026-10-20T11:18:14](../../re/traffic.md#ts-20261020-111814)

## 5. Open items

- [ ] TRAFFIC-1 — Wire the manager into `chase-game`: the pregen traffic data (quadtree leaf, routes), the route set-up 0x2df574 / 0x2dfc84, the values of the LOD table 0x529e30 and of the radius table `*0x375f40` (an oracle read), the car messages 0x80000005 / 6 / 7, and the natives SetTrafficMaxVehicles and EnableTrafficAndPedestrians routed to the manager. needs: TRAFFIC-2, PREGEN-1. blocks: SCRIPT-230, TRAFFIC-S1. evidence: [traffic.md §2026-10-20T11:18:14](../../re/traffic.md#ts-20261020-111814)
- [ ] TRAFFIC-2 — The AI driver 0x26d934 and its record (vehicle record +0x2a4). Open: who moves driver +0x78 from 0 to 2 or 3 after a spawn, states 5 and 6, the path follower 0x2bf2ec, the route type functions (table 0x3581d4), and how a spawned car becomes kinematic. needs: VEH-91. blocks: TRAFFIC-1, VEH-12. evidence: [traffic.md §2026-10-20T11:18:14](../../re/traffic.md#ts-20261020-111814)
- [ ] TRAFFIC-3 — Read the rest of the manager: the writer of the pending route list +0xb9c / +0xd9c, the signal objects of 0x2dfc84 and their test 0x2cdde4, the layer table of 0x1ffbd8, the LOD class -1 of 0x28781c, and a leaf without points. blocks: TRAFFIC-S2. evidence: [traffic.md §2026-10-20T11:18:14](../../re/traffic.md#ts-20261020-111814)

## 6. Findings summary

[traffic.md §2026-10-20T11:18:14](../../re/traffic.md#ts-20261020-111814): which object is the traffic manager (0x527410, not 0x51b33c), its layout, level load, room set-up and prefill, the update with its spawn and removal rules, the spawn 0x2def8c, the fade 0x2df8b8, SetTrafficMaxVehicles, the choice of input provider in 0x1571dc, and the states of the AI driver 0x26d934 read so far.

## 7. Code and tests

`src/runtime/traffic_manager.{hpp,cpp}` (`TrafficManager`, `runSetTrafficMaxVehicles`, no SDL); CTest `traffic-manager` (`tests/traffic_manager_test.cpp`). There is no glue in `game.cpp`.
