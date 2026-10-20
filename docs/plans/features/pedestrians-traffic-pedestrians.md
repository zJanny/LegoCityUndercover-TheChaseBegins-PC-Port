# Pedestrians: the pedestrian manager and pedestriansystem

## 1. Scope

This file covers the pedestrians of a level: the pedestrian manager 0x51b33c (slots `Pedestrian01` .. `Pedestrian20`, limit, countdown, spawn and removal), the `props/pedestriansystem` object (kind 198) with its `PedestrianPath%03u` paths, the enable flag of EnableTrafficAndPedestrians, and the walk of the `chars/pedestrian` characters (kind 197). It does not cover traffic vehicles (manager 0x527410) or the decoder of the `Pregen/Pedestrians` files. Native state: [script-system-missions.md](script-system-missions.md) (SCRIPT-230). Standing level characters: [level-objects.md](level-objects.md).

## 2. Status

IN_PROGRESS, updated 2026-10-20T12:28:51+02:00.
- The manager's rules are read from the original and ported as `PedestrianManager`. That covers the level load with `Pedestrians_Max`, the scene enter and leave, the enable and spawn-data flags, the countdown, the spawn tests and placement, the one removal per update, and the node and room tests.
- The update's caller is known: the system list, once per gameplay step. The lead wires it next (PED-1).
- The pedestrian character's update is read and ported as `PedestrianWalk`: the state step with the entries of states 1 and 3 .. 7, the walk along the path, the avoidance and the hold counter. The steady updates of states 3, 4, 5 and 7 are not run (PED-4).
- Nothing is wired into `chase-game` yet. Pedestrians still stand in frame 0 (LEVOBJ-S1), and EnableTrafficAndPedestrians only records its flag.
- The fade-in of a spawned node is the manager's (0x2ee068 sets node +0xdd and +0xdc) and the node draw's, not the character's; the character update has no fade code.

## 3. Verified behaviour

The original's code (llvm-objdump) was transcribed into the tests `pedestrian-manager` and `pedestrian-limit-real-data`:
- Slot names: lvl_01 has `Pedestrian02` .. `Pedestrian15` (14 slots).
- `Pedestrians_Max` comes from the scene's level table: 5 in lvl_01, 10 in lvl_07, and the default 5 in lvl_04, which has none.
- Init and enable bits. Every scene enter sets the countdown to 0, and the leave clears bit 0.
- Spawn only while live < limit <= slots, after the countdown, with the countdown at 7 x live after a spawn.
- The order of the random draws.
- The spawn window from the fade tables (x 0.5 / x 1.5), with the fade tier from the node's radius and fade bits.
- The room and layer test of the spawn point and of the removal.
- The facing limit 0.8 and the spacing of 8 on one path.
- At most one removal per update, with the countdown at 7 x (live - 1). It applies to a hidden node, a node under the room's node +0x7c, or an idle pedestrian. The animation stop happens only with an animation player.
- A disabled manager removes every live pedestrian without the animation stop.
- The placement rows, the clear of the walk token 0x375d5c at the start of the update, and the other live slots in slot order that the avoidance 0x2edf08 compares with. The token take and the avoidance are `PedestrianWalk`'s.

The pedestrian character, transcribed from the code into the test `pedestrian-walk`:
- Clip names of the create and +0x61 from `IsMale` (4 / 8).
- Entry 1: the target at parameter +/- 0.1 with the lateral offset, hold = random % 50 + 10, the walk clip in mode 1; state 1 requests 2.
- Entries 3 / 4 (dodge clip by +0x5f, root motion), 5 (bump clip random % 2, heading to the player), 6 (splat, 3.0 s), 7 (splat_up), and the return to state 1 when the clip has finished.
- Walk: the turn at dt x 5, the move along row 2, the rows rebuilt with up, one ground probe per update or when tilted over 20 degrees, the touch push 0.1 and the bump for player states 2 and 3.
- Path parameter + / - 0.3 near the target with the wrap at the point count, the turn back by 0.75, the sidestep 1.3 with the avoid delay 30.
- Avoidance: xz length below 2.0 and side below 1.0, the player first, then the other pedestrians.
- The hold counter: at least 10 while drawn and not hidden, else counting down.

## 4. Stand-ins

- PED-S1 The caller supplies the spawn points of the leaf at the player's position as a plain list. The original reads them from `Pregen/Pedestrians/<level>_Pedestrians.bin` (loader 0x2ee868, tree 0x1eb254 / 0x1eb490); the PREGEN decoder (`parsePregenSpawns`, `findPregenSpawnCell`, [pedestrians-traffic.md](pedestrians-traffic.md)) replaces it once PED-1 feeds its leaf to the manager. needs: PED-1. evidence: [pedestrians.md §2026-10-20T11:17:42](../../re/pedestrians.md#ts-20261020-111742).
- PED-S2 The caller supplies the blocked test 0x26cd38 and the layer table *0x376ad4. It also supplies each pedestrian's class byte, node and room state and the mesh node type *0x37674c. The distance fade table defaults to the values read in one oracle state, not to its source record 0x376794. needs: PED-7, PED-8. evidence: [pedestrians.md §2026-10-20T12:27:05](../../re/pedestrians.md#ts-20261020-122705).
- PED-S3 `pedestrianLevelPath` samples every path as type 0 (0x1c17c0) and does not apply the path's node transform (path +0x18). needs: PED-6. evidence: [pedestrians.md §2026-10-20T12:20:49](../../re/pedestrians.md#ts-20261020-122049).
- PED-S4 `PedestrianWalk` takes these from the caller: the blocked result of 0x1ffcd4 (PED-7), the ground probe 0x2058cc and ray 0x21ab20, the clip finished test, the draw and hidden tests, 0x278b04, and the arrival radius (entity +0x5c). In states 3, 4, 5 and 7 it runs no steady update. needs: PED-4, PED-6, PED-7. evidence: [pedestrians.md §2026-10-20T12:20:49](../../re/pedestrians.md#ts-20261020-122049).
- PED-S5 A `Pedestrians_Max` stored as a slot parameter gives its table value; the level root's slot values are not read. No shipped level stores it as a slot. needs: PED-9. evidence: [pedestrians.md §2026-10-20T12:27:05](../../re/pedestrians.md#ts-20261020-122705).

## 5. Open items

- [ ] PED-1 — Wire the manager and the walk (`PedestrianWalk`) into the game. It runs as slot +0x48 of the system list, once per gameplay step before the script runner. `enterScene` runs on every scene enter, and `loadLevel` for each loaded scene. The spawn data flag and the leaf's spawn points need the decoded pregen file with its derived fields and the pedestriansystem step 0x2edc94. The spawn needs the view rows, the vehicle speed and the EnableTrafficAndPedestrians flag. Removals must disable the entity (0x2428c0) and stop its slots. blocks: SCRIPT-230, PED-S1, LEVOBJ-16, LEVOBJ-S1. evidence: [pedestrians.md §2026-10-20T12:27:05](../../re/pedestrians.md#ts-20261020-122705).
- [ ] PED-4 — The steady updates of the pedestrian's states 3, 4 and 7 (root motion 0x277afc with the wall test 0x2059ec and the probe) and of state 5 (0x1fd8c8 with the vector 0x56d400) are not ported. blocks: PED-S4. uses: chars/pedestrian. evidence: [pedestrians.md §2026-10-20T12:20:49](../../re/pedestrians.md#ts-20261020-122049).
- [ ] PED-5 — The pedestrian's message slot 0x19efbc is read only at its start: which messages request the dodge (3, 4) and splat (6) states and set +0x5f, and message 50. uses: chars/pedestrian. evidence: [pedestrians.md §2026-10-20T12:20:49](../../re/pedestrians.md#ts-20261020-122049).
- [ ] PED-6 — These walk inputs are not read: the probe floats 0x374494 / 0x374498, the bounds 0x242df4 behind entity +0x5c, the draw test 0x2930e0, 0x278b04, path type 1 (0x1b33fc) and the path node at path +0x18. blocks: PED-S3, PED-S4. evidence: [pedestrians.md §2026-10-20T12:20:49](../../re/pedestrians.md#ts-20261020-122049).
- [ ] PED-7 — The blocked test is not ported. 0x26cd38 calls 0x1ffcd4 for each entity of the list 0x5863b4 + 0x305c; 0x1ffcd4 tests the point against the vehicle's box and can write the pedestrian's requested state. blocks: PED-S2, PED-S4. evidence: [pedestrians.md §2026-10-20T12:27:05](../../re/pedestrians.md#ts-20261020-122705).
- [ ] PED-8 — The sources of the state the caller supplies are not read: the layer table *0x376ad4 (callers of the setter 0x2405a0), the class byte +0x18 of a pedestrian, which room node +0x7c is, and the run-time values of *0x37674c and the fade record 0x376794. blocks: PED-S2. evidence: [pedestrians.md §2026-10-20T12:27:05](../../re/pedestrians.md#ts-20261020-122705).
- [ ] PED-9 — Two callers are not traced: the second update caller 0x164240 (record 0x376d18 +8, no reader found), and which scene's level load sets the slots and the limit when several scenes are loaded. The level root's slot values are not read either. blocks: PED-S5. evidence: [pedestrians.md §2026-10-20T12:27:05](../../re/pedestrians.md#ts-20261020-122705).

## 6. Findings summary

- The manager, its update, spawn, pedestriansystem and pregen loader, and the correction of the swapped managers: [pedestrians.md §2026-10-20T11:17:42](../../re/pedestrians.md#ts-20261020-111742).
- The pedestrian character: create, state step, walk and avoidance: [pedestrians.md §2026-10-20T12:20:49](../../re/pedestrians.md#ts-20261020-122049).
- The limit source, the system list and the update caller, the scene enter, the node and room tests, the blocked test and the walk helpers: [pedestrians.md §2026-10-20T12:27:05](../../re/pedestrians.md#ts-20261020-122705).
- The distance fade tables that set the spawn window: [script-natives-world.md](../../re/script-natives-world.md).

## 7. Code and tests

- `src/runtime/pedestrian_manager.*` (`PedestrianManager`, `pedestriansMaxOf`, `pedestrianFadeTier`, `pedestrianNodeHidden`, `pedestrianRoomActive`, `pedestrianSpawnWindow`, `pedestrianPlacement`).
- `src/runtime/pedestrian_walk.*` (`PedestrianWalk`, `PedestrianBody`, `pedestrianLevelPath`).
- CTest: `pedestrian-manager`, `pedestrian-limit-real-data`, `pedestrian-walk`.
