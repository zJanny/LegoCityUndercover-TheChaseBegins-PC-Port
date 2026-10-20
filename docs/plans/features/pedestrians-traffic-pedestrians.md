# Pedestrians: the pedestrian manager and pedestriansystem

## 1. Scope

This file covers the pedestrians of a level: the pedestrian manager 0x51b33c (slots `Pedestrian01` .. `Pedestrian20`, limit, countdown, spawn and removal), the `props/pedestriansystem` object (kind 198) with its `PedestrianPath%03u` paths, the enable flag of EnableTrafficAndPedestrians, and the walk of the `chars/pedestrian` characters (kind 197). It does not cover traffic vehicles (manager 0x527410) or the decoder of the `Pregen/Pedestrians` files. Native state: [script-system-missions.md](script-system-missions.md) (SCRIPT-230). Standing level characters: [level-objects.md](level-objects.md).

## 2. Status

IN_PROGRESS, updated 2026-10-20T11:19:32+02:00.
- The manager's rules are read from the original and ported as `PedestrianManager`: the level load, the enable and spawn-data flags, the countdown, the spawn tests and placement, and the one removal per update.
- Nothing is wired into `chase-game` yet. Pedestrians still stand in frame 0 (LEVOBJ-S1), and EnableTrafficAndPedestrians only records its flag.

## 3. Verified behaviour

The original's code (llvm-objdump) was transcribed into the test `pedestrian-manager`:
- Slot names: lvl_01 has `Pedestrian02` .. `Pedestrian15` (14 slots).
- Init and enable bits.
- Spawn only while live < limit <= slots, after the countdown, with the countdown at 7 x live after a spawn.
- The order of the random draws.
- The spawn window from the fade tables (x 0.5 / x 1.5).
- The facing limit 0.8 and the spacing of 8 on one path.
- At most one removal per update, with the countdown at 7 x (live - 1).
- A disabled manager removes every live pedestrian without the animation stop.
- The placement rows.

## 4. Stand-ins

- PED-S1 The caller supplies the spawn points of the leaf at the player's position as a plain list. The original reads them from `Pregen/Pedestrians/<level>_Pedestrians.bin` (loader 0x2ee868, tree 0x1eb254 / 0x1eb490); the PREGEN decoder (`parsePregenSpawns`, `findPregenSpawnCell`, [pedestrians-traffic.md](pedestrians-traffic.md)) replaces it once PED-1 feeds its leaf to the manager. needs: PED-1. evidence: [pedestrians.md §2026-10-20T11:17:42](../../re/pedestrians.md#ts-20261020-111742).
- PED-S2 The manager takes these from the caller instead of computing them: the pedestrian's fade tier, the blocked test 0x26cd38, the room, node and hidden tests of the removal, and the fade-node test. The distance fade table defaults to the values read in one oracle state, not to its source record 0x376794. The countdown +0xa8 starts at 0, because no initial store was found. needs: PED-3. evidence: [pedestrians.md §2026-10-20T11:17:42](../../re/pedestrians.md#ts-20261020-111742).

## 5. Open items

- [ ] PED-1 — Wire the manager into the game. The level load needs the `Pedestrian%02u` slots and the source of `Pedestrians_Max` (0x2451d0 on 0x23c188(level), not read). The spawn data flag and the leaf's spawn points need the decoded pregen file with the fields the loader derives (PREGEN-1) and the pedestriansystem step 0x2edc94. The spawn needs the view rows, the vehicle speed and the EnableTrafficAndPedestrians flag. Removals must disable the entity (0x2428c0) and stop its slots. needs: PREGEN-1. blocks: SCRIPT-230, PED-S1. evidence: [pedestrians.md §2026-10-20T11:17:42](../../re/pedestrians.md#ts-20261020-111742).
- [ ] PED-2 — The pedestrian's states and walk are not read (kind 197: create 0x1913f8, callbacks 0x277c24 / 0x281fc0 / 0x1c5448, the state 1 update). That covers the path following with speed +0x54, lateral +0x48, parameter +0x4c and direction +0x5c, the walk animation (mode 6) and the fade-in of a spawned node. blocks: LEVOBJ-16, LEVOBJ-S1. uses: chars/pedestrian. evidence: [pedestrians.md §2026-10-20T11:17:42](../../re/pedestrians.md#ts-20261020-111742).
- [ ] PED-3 — These spawn and removal inputs are not read: the blocked test 0x26cd38 / 0x1ffcd4, the node tests (node +4 == room +0x7c, 0x23de70 bits), the class byte +0x18 and fade tier of a pedestrian node, the initial countdown (the port starts at 0), the caller of the update (vtable +0x48), byte 0x375d5c and 0x2edf08. blocks: PED-S2. evidence: [pedestrians.md §2026-10-20T11:17:42](../../re/pedestrians.md#ts-20261020-111742).

## 6. Findings summary

- The manager, its update, spawn, pedestriansystem and pregen loader, and the correction of the swapped managers: [pedestrians.md §2026-10-20T11:17:42](../../re/pedestrians.md#ts-20261020-111742).
- The distance fade tables that set the spawn window: [script-natives-world.md](../../re/script-natives-world.md).

## 7. Code and tests

- `src/runtime/pedestrian_manager.*` (`PedestrianManager`, `pedestrianSpawnWindow`, `pedestrianPlacement`).
- CTest: `pedestrian-manager`.
