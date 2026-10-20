# Pedestrians: the vehicle test and the room layer table

## 1. Scope

The pedestrian's vehicle test 0x1ffcd4 with its list loop (spawn 0x26cd38, walk 0x2b9340) and dodge ray 0x22b108, the vehicle list 0x5863b4 + 0x305c, and the room layer table *0x376ad4 with its cutscene push. The manager, walk and rooms are in [pedestrians-traffic-pedestrians.md](pedestrians-traffic-pedestrians.md).

## 2. Status

IN_PROGRESS, updated 2026-10-20T14:29:50+02:00.
- The vehicle test with its list loop and dodge ray is ported as `pedestrianBlocked`, and the room layer table as `startupRoomLayers` / `cutsceneRoomLayers`. `chase-game` runs the vehicle test for the spawn and the walk over the vehicles of its vehicle world and takes the set-up layer table; the cutscene table is not pushed (PED-18).

## 3. Verified behaviour

Transcribed from the code into the tests `pedestrian-blocked`, `pedestrian-walk` and `room-layers`:
- The distance gate, the slow branch (turn back 4 and the face slides) and the fast branch (the dodge rays at +/-5.0 along row 0, states 3 / 4, the dodge side, the splat 6). The list skips kinematic bodies.
- The walk adds the step vector after the vehicle loop (0x2b93c0), slid or not. The dodge ray hits only static level collision (include 1, exclude 16, no surface mask), and a vehicle's radius is the length of its box half extents.
- The layer table after the game set-up, and the cutscene copy of it with class 6 cleared.

## 4. Stand-ins

- PED-S9 `chase-game` lists the vehicles that have a body in its vehicle world (VEH-S82), not wrecked and not disabled, in its vehicle order, at the pedestrian update instead of at the physics step 0x2e2bc8; body flag +0xc4 bit 0 and entity bit 5 are not read, and no listed body is kinematic. needs: PED-19. evidence: [pedestrians-inputs.md §2026-10-20T14:11:29](../../re/pedestrians-inputs.md#ts-20261020-141129).
- PED-S10 `chase-game` uses the set-up layer table also during a cutscene. needs: PED-18. evidence: [pedestrians-inputs.md §2026-10-20T14:11:29](../../re/pedestrians-inputs.md#ts-20261020-141129).

## 5. Open items

- [ ] PED-18 — `chase-game` does not push the cutscene layer table at a cutscene start (0x196734, 0x16c060) or pop it at its end (0x2fae68), and does not run the cutscene mode's pedestrian update 0x164240. blocks: PED-S10. evidence: [pedestrians-inputs.md §2026-10-20T14:11:29](../../re/pedestrians-inputs.md#ts-20261020-141129).
- [ ] PED-19 — Which vehicle bodies have the flag +0xc4 bit 0 that the list rebuild 0x2e2bc8 requires, what entity bit 5 of +0x10 is, and whether a vehicle's +0x5c comes from 0x23aff8. needs: VEH-91. blocks: PED-S9. evidence: [pedestrians-inputs.md §2026-10-20T14:11:29](../../re/pedestrians-inputs.md#ts-20261020-141129).

## 6. Findings summary

- The vehicle test 0x1ffcd4 and the room layer table: [pedestrians-inputs.md §2026-10-20T13:20:02](../../re/pedestrians-inputs.md#ts-20261020-132002).
- The dodge ray's descriptor, the walk's vehicle loop, the list rebuild, the vehicle radius and the wiring in `chase-game`: [pedestrians-inputs.md §2026-10-20T14:11:29](../../re/pedestrians-inputs.md#ts-20261020-141129).

## 7. Code and tests

- `src/runtime/pedestrian_blocked.*` (`pedestrianVehicleBlock`, `pedestrianBlocked`, `pedestrianSpawnBlocked`, `pedestrianDodgeRayBlocked`, `pedestrianBlockVehicle`, `PedestrianBlockVehicle`); the descriptor masks in `src/runtime/camera_reset_probe.*`.
- `src/runtime/room_layers.*` (`startupRoomLayers`, `cutsceneRoomLayers`, `setRoomLayers`).
- Glue `src/game_pedestrian_blocking.*`.
- CTest: `pedestrian-blocked`, `room-layers`, `pedestrian-walk`.
