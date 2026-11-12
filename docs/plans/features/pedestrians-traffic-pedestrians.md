# Pedestrians: the pedestrian manager and pedestriansystem

## 1. Scope

This file covers the pedestrians of a level: the pedestrian manager 0x51b33c (slots `Pedestrian01` .. `Pedestrian20`, limit, countdown, spawn and removal), the `props/pedestriansystem` object (kind 198) with its `PedestrianPath%03u` paths, the enable flag of EnableTrafficAndPedestrians, and the walk of the `chars/pedestrian` characters (kind 197). It does not cover traffic vehicles (manager 0x527410) or the decoder of the `Pregen/Pedestrians` files. Native state: [script-system-missions.md](script-system-missions.md) (SCRIPT-230). Standing level characters: [level-objects.md](level-objects.md). The spawn fade-in and mesh node alpha: [pedestrians-traffic-pedestrians-fade.md](pedestrians-traffic-pedestrians-fade.md). The vehicle test and the room layer table: [pedestrians-traffic-pedestrians-vehicles.md](pedestrians-traffic-pedestrians-vehicles.md).

## 2. Status

IN_PROGRESS, updated 2026-11-12T08:05:44+01:00.
- The manager's rules are read from the original and ported as `PedestrianManager`. That covers the level load with `Pedestrians_Max`, the scene enter and leave, the enable and spawn-data flags, the countdown, the spawn tests and placement, the one removal per update, and the node and room tests.
- `chase-game` runs both as `PedestrianSystem`: the manager in a system phase before the runner, the characters in the object phase, sorted after the other object steps by their class type 197. The level load disables the slots, a spawn enables one, a removal disables it and stops its clip. The scene enter loads the level's pregen file.
- The pedestrian character's update is read and ported as `PedestrianWalk`: the state step with the entries of states 1 and 3 .. 7, the walk along the path, the avoidance, the hold counter and the steady updates of states 3, 4, 5 and 7. The draw test, the vehicle test of state 6, the entity box and the type 1 path are ported too; the root delta and the wall slide come from the caller (PED-S8).
- In lvl_01 up to five pedestrians appear in front of the camera and walk their paths with the walk clip; EnableTrafficAndPedestrians sets the manager's enable bit.
- `chase-game` runs the vehicle test 0x1ffcd4 for the spawn and the walk and takes the set-up room layer table ([vehicles](pedestrians-traffic-pedestrians-vehicles.md)).
- Walking pedestrians change room by the room pass 0x2e6ce4 (`movePedestrianRooms`); the spawn view and the bump use the vehicle's forward speed and the player's controller state.

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

- PED-S2 The caller keeps each pedestrian's class byte, node and room state. `chase-game` supplies class byte 0 (measured), room type 0 (measured 0 and 1 in two rooms of one level) and room +0x7c as a node other than the +0x14 a pedestrian hangs on. The mesh node type *0x37674c = 2 is read from the code and measured. needs: PED-16. evidence: [pedestrians-fade.md §2026-10-25T14:40:22](../../re/pedestrians-fade.md#ts-20261025-144022).
- PED-S4 `PedestrianWalk` takes these from the caller: the ground probe 0x2058cc with its ray and room set, the bump ray 0x21ab20 and the clip finished test. `chase-game` supplies the player's ground probe, the end of the newest clip, false for the hidden test and the draw test of the mesh node with the camera look point as fade point (PED-S11), and no ray. evidence: [pedestrians-character.md §2026-10-20T13:09:29](../../re/pedestrians-character.md#ts-20261020-130929).
- PED-S6 `chase-game` draws the pedestrians' random words from its own generator with the level characters' fixed seed. evidence: [pedestrians.md §2026-10-20T13:13:47](../../re/pedestrians.md#ts-20261020-131347).
- PED-S17 The object steps of `chase-game` other than the pedestrians (traffic cars, junctions, doors, switches, spawners, pushables) run in call order before the pedestrians; the original sorts them by class type at priority 128, so the cars (183) would run before the junctions (192). needs: PED-31. evidence: [pedestrians-character.md §2026-11-12T08:05:25](../../re/pedestrians-character.md#ts-20261112-080525).
- PED-S18 The pedestrians make no sound in `chase-game`: the footstep message 50 with its hearing distance, the clip sound events of the animation event message and the preload 252 are ported as functions only. needs: PED-30. evidence: [pedestrians-character.md §2026-11-12T08:05:25](../../re/pedestrians-character.md#ts-20261112-080525).
- PED-S8 The steady updates take from the caller the root translation of 0x2327b4, the wall slide 0x21a73c with its near list, the fade point 0x52a1b4 (`chase-game` supplies the camera look point, PED-S11) and the node bounds behind entity +0x5c (`assets::modelBounds`). Without a wall slide the move is not changed; `chase-game` supplies the bounds but no root translation and no wall slide. needs: PED-27. evidence: [pedestrians-character.md §2026-10-20T13:09:29](../../re/pedestrians-character.md#ts-20261020-130929).
- PED-S13 The room pass tests every enabled pedestrian: the entity bits 2 and 10 of +0x10 are taken as clear and the node flag bit 19 as set, and the player's room is the current room. needs: PED-24. evidence: [pedestrians-inputs.md §2026-10-20T14:10:27](../../re/pedestrians-inputs.md#ts-20261020-141027).
- PED-S14 The player's controller state +0x6c is derived from the posture and the playing locomotion role (1 / 2 / 3 on the ground, 4 .. 8 in the air); every carry, vehicle, action or script state counts as unknown (0). needs: PED-26. evidence: [pedestrians-inputs.md §2026-10-20T14:10:27](../../re/pedestrians-inputs.md#ts-20261020-141027).
- PED-S15 The pose flag 0x40000000 of the camera pose is never set in `chase-game`, so the fade point is always the look point (the original takes the eye while the flag is set). needs: PED-27. evidence: [pedestrians-fade.md §2026-10-25T14:19:23](../../re/pedestrians-fade.md#ts-20261025-141923).
- PED-S16 The inputs 0x52a1cc (jump) and 0x52a1cd (force fade-in) of the fade step are always false in `chase-game`. needs: PED-28. evidence: [pedestrians-fade.md §2026-10-25T14:19:23](../../re/pedestrians-fade.md#ts-20261025-141923).
- PED-S5 A `Pedestrians_Max` stored as a slot parameter gives its table value; the level root's slot values are not read. No shipped level stores it as a slot. needs: PED-9. evidence: [pedestrians.md §2026-10-20T12:27:05](../../re/pedestrians.md#ts-20261020-122705).

## 5. Open items

- [ ] PED-24 — What a pedestrian does on message 0x51 sent by the room move, and the run-time values of the node flag bit 19 and of the entity bits 2 and 10 of +0x10. blocks: PED-S13. evidence: [pedestrians-inputs.md §2026-10-20T14:10:27](../../re/pedestrians-inputs.md#ts-20261020-141027).
- [ ] PED-26 — The controller state +0x6c in the carry, vehicle, action and script cases, and whether 2 and 3 stay run and walk there. blocks: PED-S14. evidence: [pedestrians-inputs.md §2026-10-20T14:10:27](../../re/pedestrians-inputs.md#ts-20261020-141027).
- [ ] PED-9 — The start values of the level root's slots (+0x2c / +0x30) are not read, and which worlds the list 0x589a18 holds together at run time is not known. blocks: PED-S5. evidence: [pedestrians-inputs.md §2026-10-20T13:20:02](../../re/pedestrians-inputs.md#ts-20261020-132002).
- [ ] PED-27 — Which cameras make the fade point the pose eye (pose flag 0x40000000): clear on foot and in a vehicle camera; the cutscene, scripted and scanner cameras are not measured, and the port's camera rig has no such flag. blocks: PED-S15, PED-S8. evidence: [pedestrians-fade.md §2026-10-25T14:35:54](../../re/pedestrians-fade.md#ts-20261025-143554).
- [ ] PED-28 — The jump input of the fade step is true when the fade point moved 9 m or more from its previous point; whether that point is refreshed every frame is not read. blocks: PED-S16. evidence: [pedestrians-fade.md §2026-10-25T14:19:23](../../re/pedestrians-fade.md#ts-20261025-141923).
- [ ] PED-16 — The room object's byte +0x3a (the room type of the removal test) is measured 0 and 1 in two rooms of one street level; other rooms and levels are not measured. blocks: PED-S2. evidence: [pedestrians-fade.md §2026-10-25T14:40:22](../../re/pedestrians-fade.md#ts-20261025-144022).
- [ ] PED-29 — The writer and reader of the entity byte +0x3a (128 .. 254, the place where the entity was last placed; not the room type of the removal test) are not found. evidence: [pedestrians-fade.md §2026-10-25T14:40:22](../../re/pedestrians-fade.md#ts-20261025-144022).

- [ ] PED-30 — Whether the pedestrian classes' walk, dodge, bump and splat clips carry the footstep and character sound tracks of the animation event message and who sends the preload 252; then wire `PedestrianVoice` (voice row of +0x61, listener = camera, hearing distance = `SoundEffect::paramB`) and `ClipSounds` to the live pedestrians. blocks: PED-S18. evidence: [pedestrians-character.md §2026-11-12T08:05:25](../../re/pedestrians-character.md#ts-20261112-080525).
- [ ] PED-31 — The kinds of the other object steps relative to each other (cars 183 before junctions 192, doors, switches, spawners, pushables): give each step its class type with `atObjectKind`. blocks: PED-S17. evidence: [pedestrians-character.md §2026-11-12T08:05:25](../../re/pedestrians-character.md#ts-20261112-080525).

## 6. Findings summary

- The manager, its update, spawn, pedestriansystem and pregen loader, and the correction of the swapped managers: [pedestrians.md §2026-10-20T11:17:42](../../re/pedestrians.md#ts-20261020-111742).
- The pedestrian character: create, state step, walk and avoidance: [pedestrians-character.md §2026-10-20T12:20:49](../../re/pedestrians-character.md#ts-20261020-122049).
- The steady updates of states 3, 4, 5 and 7, the probe, the wall test, the draw and vehicle tests, the entity box and the type 1 path: [pedestrians-character.md §2026-10-20T13:09:29](../../re/pedestrians-character.md#ts-20261020-130929).
- The limit source, the system list and the update caller, the scene enter, the node and room tests, the blocked test and the walk helpers: [pedestrians.md §2026-10-20T12:27:05](../../re/pedestrians.md#ts-20261020-122705).
- The wiring in `chase-game`, step 0x2edc94's path count and the spawn room: [pedestrians.md §2026-10-20T13:13:47](../../re/pedestrians.md#ts-20261020-131347).
- The message slot 0x19efbc (the shared sound handler, no state requests, 0x80000003 never sent to kind 197) and the pedestrians' place in the entity pass: [pedestrians-character.md §2026-11-12T08:05:25](../../re/pedestrians-character.md#ts-20261112-080525).
- The room move, the priority 128, the vehicle speed and the controller state: [pedestrians-inputs.md §2026-10-20T14:10:27](../../re/pedestrians-inputs.md#ts-20261020-141027).
- The distance fade tables that set the spawn window: [script-natives-world.md](../../re/script-natives-world.md); their source record 0x376794: [pedestrians-inputs.md §2026-10-20T13:20:02](../../re/pedestrians-inputs.md#ts-20261020-132002).
- The room objects' type byte and the layer table rows on a street: [pedestrians-fade.md §2026-10-25T14:40:22](../../re/pedestrians-fade.md#ts-20261025-144022).
- The measured fade point in a vehicle camera, *0x37674c and the entity bytes in four places: [pedestrians-fade.md §2026-10-25T14:35:54](../../re/pedestrians-fade.md#ts-20261025-143554).
- The writer of the fade point 0x2ae1ac, the third layer table push 0x17e544 and *0x37674c from code: [pedestrians-fade.md §2026-10-25T14:19:23](../../re/pedestrians-fade.md#ts-20261025-141923).
- The room layer table, the class byte, room +0x7c, *0x37674c and the cutscene update caller: [pedestrians-inputs.md §2026-10-20T13:20:02](../../re/pedestrians-inputs.md#ts-20261020-132002).

## 7. Code and tests

- `src/runtime/pedestrian_manager.*` (`PedestrianManager`, `pedestriansMaxOf`, `pedestrianFadeTier`, `pedestrianNodeHidden`, `pedestrianRoomActive`, `pedestrianSpawnWindow`, `pedestrianPlacement`).
- `src/runtime/pedestrian_walk.*` (`PedestrianWalk`, `PedestrianBody`, `pedestrianLevelPath`, `pedestrianLinearPathSample`, `pedestrianEntityBox`, `pedestrianGroundProbe`, `pedestrianNodeDrawn`, `pedestrianUnderVehicle`).
- `src/runtime/pedestrian_system.*` (`PedestrianSystem`, `pedestrianPathCount`, `levelHasPedestrianSystem`, `pedestrianRoomObject`); session glue `src/game_pedestrians.*`; the system phase in `src/runtime/update_order.*`.
- `src/runtime/pedestrian_messages.*` (`pedestrianMessage`, `PedestrianVoice`); the kind sort `UpdateSequence::atObjectKind` in `src/runtime/update_order.*`.
- `src/runtime/pedestrian_rooms.*` (`movePedestrianRooms`, `pedestrianRoomTestPoint`), `src/runtime/player_controller_state.*` (`playerControllerState`); glue `src/game_pedestrian_rooms.cpp`.
- CTest: `pedestrian-rooms`, `player-controller-state`, slow `game-pedestrian-rooms` (pins slot 5 Room8 -> Room7 at update 281 after the fade-in round: the port's own run, not an oracle value), `pedestrian-manager`, `pedestrian-limit-real-data`, `pedestrian-walk`, `pedestrian-messages`, `pedestrian-system`, `pedestrian-system-real-data` (skips with 77 without the RomFS), `update-order`.
