# Pedestrians: the pedestrian manager and pedestriansystem

## 1. Scope

This file covers the pedestrians of a level: the pedestrian manager 0x51b33c (slots `Pedestrian01` .. `Pedestrian20`, limit, countdown, spawn and removal), the `props/pedestriansystem` object (kind 198) with its `PedestrianPath%03u` paths, the enable flag of EnableTrafficAndPedestrians, and the walk of the `chars/pedestrian` characters (kind 197). It does not cover traffic vehicles (manager 0x527410) or the decoder of the `Pregen/Pedestrians` files. Native state: [script-system-missions.md](script-system-missions.md) (SCRIPT-230). Standing level characters: [level-objects.md](level-objects.md).

## 2. Status

IN_PROGRESS, updated 2026-10-20T13:13:47+02:00.
- The manager's rules are read from the original and ported as `PedestrianManager`. That covers the level load with `Pedestrians_Max`, the scene enter and leave, the enable and spawn-data flags, the countdown, the spawn tests and placement, the one removal per update, and the node and room tests.
- `chase-game` runs both as `PedestrianSystem`: the manager in a system phase before the runner, the characters in the object phase. The level load disables the slots, a spawn enables one, a removal disables it and stops its clip. The scene enter loads the level's pregen file.
- The pedestrian character's update is read and ported as `PedestrianWalk`: the state step with the entries of states 1 and 3 .. 7, the walk along the path, the avoidance, the hold counter and the steady updates of states 3, 4, 5 and 7. The draw test, the vehicle test of state 6, the entity box and the type 1 path are ported too; the root delta and the wall slide come from the caller (PED-S8).
- In lvl_01 up to five pedestrians appear in front of the camera and walk their paths with the walk clip; EnableTrafficAndPedestrians sets the manager's enable bit.
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
- Steady updates: states 3, 4 and 7 move by the root translation through the node rows, slid in x and z by the wall test (reach +0x5c x 1.5), then probe the ground 0.25 up and down. State 5 runs the contact with wall test and probe, and moves by the push again on a touch. Without a root delta nothing happens.
- The draw test 0x2930e0 (type, fade bits, fade-in distance of the tier), the vehicle box test 0x278b04, the entity box 0x242df4 and the linear type 1 path with its clamp and wrap.

## 4. Stand-ins

- PED-S2 The caller supplies the blocked test 0x26cd38 and the layer table *0x376ad4. It also supplies each pedestrian's class byte, node and room state and the mesh node type *0x37674c. The distance fade table defaults to the values read in one oracle state, not to its source record 0x376794. `chase-game` supplies every layer active, class byte 0, room type 0, room +0x7c as a node other than the +0x14 a pedestrian hangs on, and no blocking vehicle. needs: PED-7, PED-8. evidence: [pedestrians.md §2026-10-20T12:27:05](../../re/pedestrians.md#ts-20261020-122705).
- PED-S4 `PedestrianWalk` takes these from the caller: the blocked result of 0x1ffcd4 (PED-7), the ground probe 0x2058cc with its ray and room set, the bump ray 0x21ab20 and the clip finished test. `chase-game` supplies the player's ground probe, the end of the newest clip, false for the draw and hidden tests, no ray and no blocking. needs: PED-7. evidence: [pedestrians-character.md §2026-10-20T13:09:29](../../re/pedestrians-character.md#ts-20261020-130929).
- PED-S6 `chase-game` takes the speed 0x34163c of the player's vehicle as the length of the driven body's velocity and the player state +0x6c as 0 (no bump), and draws the random words from its own generator with the level characters' fixed seed. needs: PED-10. evidence: [pedestrians.md §2026-10-20T13:13:47](../../re/pedestrians.md#ts-20261020-131347).
- PED-S7 The pedestrian characters update in the object phase of the entity pass, and a pedestrian keeps the room of its spawn point. needs: PED-11. evidence: [pedestrians.md §2026-10-20T13:13:47](../../re/pedestrians.md#ts-20261020-131347).
- PED-S8 The steady updates take from the caller the root translation of 0x2327b4, the wall slide 0x21a73c with its near list, the fade point 0x52a1b4 and the node bounds behind entity +0x5c (`assets::modelBounds`). Without a wall slide the move is not changed; `chase-game` supplies the bounds but no root translation and no wall slide. needs: PED-13. evidence: [pedestrians-character.md §2026-10-20T13:09:29](../../re/pedestrians-character.md#ts-20261020-130929).
- PED-S5 A `Pedestrians_Max` stored as a slot parameter gives its table value; the level root's slot values are not read. No shipped level stores it as a slot. needs: PED-9. evidence: [pedestrians.md §2026-10-20T12:27:05](../../re/pedestrians.md#ts-20261020-122705).

## 5. Open items

- [ ] PED-5 — The pedestrian's message slot 0x19efbc is read only at its start: which messages request the dodge (3, 4) and splat (6) states and set +0x5f, and message 50. uses: chars/pedestrian. evidence: [pedestrians-character.md §2026-10-20T12:20:49](../../re/pedestrians-character.md#ts-20261020-122049).
- [ ] PED-13 — The writer and run-time value of the distance fade point 0x52a1b4 (0x529e30 +0x384), which the draw test 0x2930e0 measures from, are not found. blocks: PED-S8. evidence: [pedestrians-character.md §2026-10-20T13:09:29](../../re/pedestrians-character.md#ts-20261020-130929).
- [ ] PED-7 — The blocked test is not ported. 0x26cd38 calls 0x1ffcd4 for each entity of the list 0x5863b4 + 0x305c; 0x1ffcd4 tests the point against the vehicle's box and can write the pedestrian's requested state. blocks: PED-S2, PED-S4. evidence: [pedestrians.md §2026-10-20T12:27:05](../../re/pedestrians.md#ts-20261020-122705).
- [ ] PED-8 — The sources of the state the caller supplies are not read: the layer table *0x376ad4 (callers of the setter 0x2405a0), the class byte +0x18 of a pedestrian, which room node +0x7c is, and the run-time values of *0x37674c and the fade record 0x376794. blocks: PED-S2, PED-12. evidence: [pedestrians.md §2026-10-20T12:27:05](../../re/pedestrians.md#ts-20261020-122705).
- [ ] PED-10 — The speed 0x34163c of the player's vehicle body and the player component state +0x6c that the walk's bump reads are not read. blocks: PED-S6. evidence: [pedestrians.md §2026-10-20T13:13:47](../../re/pedestrians.md#ts-20261020-131347).
- [ ] PED-11 — The priority byte +0x19 of the pedestrian entity (copied from the template by 0x1913f8) and whether the room move 0x2e6ce4 moves a walking pedestrian into another room are not read. blocks: PED-S7. evidence: [pedestrians.md §2026-10-20T13:13:47](../../re/pedestrians.md#ts-20261020-131347).
- [ ] PED-12 — The fade-in of a spawned pedestrian (node +0xdd / +0xdc and 0x275434 on its children when its node type is *0x37674c) is not drawn: it appears at once. needs: PED-8. evidence: [pedestrians.md §2026-10-20T11:17:42](../../re/pedestrians.md#ts-20261020-111742).
- [ ] PED-9 — Two callers are not traced: the second update caller 0x164240 (record 0x376d18 +8, no reader found), and which scene's level load sets the slots and the limit when several scenes are loaded. The level root's slot values are not read either. blocks: PED-S5. evidence: [pedestrians.md §2026-10-20T12:27:05](../../re/pedestrians.md#ts-20261020-122705).

## 6. Findings summary

- The manager, its update, spawn, pedestriansystem and pregen loader, and the correction of the swapped managers: [pedestrians.md §2026-10-20T11:17:42](../../re/pedestrians.md#ts-20261020-111742).
- The pedestrian character: create, state step, walk and avoidance: [pedestrians-character.md §2026-10-20T12:20:49](../../re/pedestrians-character.md#ts-20261020-122049).
- The steady updates of states 3, 4, 5 and 7, the probe, the wall test, the draw and vehicle tests, the entity box and the type 1 path: [pedestrians-character.md §2026-10-20T13:09:29](../../re/pedestrians-character.md#ts-20261020-130929).
- The limit source, the system list and the update caller, the scene enter, the node and room tests, the blocked test and the walk helpers: [pedestrians.md §2026-10-20T12:27:05](../../re/pedestrians.md#ts-20261020-122705).
- The wiring in `chase-game`, step 0x2edc94's path count and the spawn room: [pedestrians.md §2026-10-20T13:13:47](../../re/pedestrians.md#ts-20261020-131347).
- The distance fade tables that set the spawn window: [script-natives-world.md](../../re/script-natives-world.md).

## 7. Code and tests

- `src/runtime/pedestrian_manager.*` (`PedestrianManager`, `pedestriansMaxOf`, `pedestrianFadeTier`, `pedestrianNodeHidden`, `pedestrianRoomActive`, `pedestrianSpawnWindow`, `pedestrianPlacement`).
- `src/runtime/pedestrian_walk.*` (`PedestrianWalk`, `PedestrianBody`, `pedestrianLevelPath`, `pedestrianLinearPathSample`, `pedestrianEntityBox`, `pedestrianGroundProbe`, `pedestrianNodeDrawn`, `pedestrianUnderVehicle`).
- `src/runtime/pedestrian_system.*` (`PedestrianSystem`, `pedestrianPathCount`, `levelHasPedestrianSystem`, `pedestrianRoomObject`); session glue `src/game_pedestrians.*`; the system phase in `src/runtime/update_order.*`.
- CTest: `pedestrian-manager`, `pedestrian-limit-real-data`, `pedestrian-walk`, `pedestrian-system`, `pedestrian-system-real-data` (skips with 77 without the RomFS), `update-order`.
