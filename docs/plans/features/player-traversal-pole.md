# Player traversal: pole and drain pipe

## 1. Scope

The pole and drain pipe (object type 187, player states 0x11d..0x121) of [player-traversal.md](player-traversal.md): mount by walking in or by the A press, climb, exits and the dismount at the top. Findings: [docs/re/player-traversal-pole.md](../../re/player-traversal-pole.md).

## 2. Status

IN_PROGRESS, updated 2026-10-18T16:15:36+02:00. Ported in `chase-game`: walk-in and A-press mount, idle, climb up and down, A / B / bottom exits and the dismount at the top of a drain pipe, replayed against the mission 1-2 states `m12-10` and `m12-10a` (TRAV-1, closed in [player-traversal.closed.md](player-traversal.closed.md)).

## 3. Verified behaviour

- Pole of m12-10 replayed against the oracle (dt 1/30; CTest `pole-real-data`): walk-in in the 3rd walking update, mount blend 5 updates with positions to 2e-4, climb steps 0.05625 / 0.0559375 up and 0.15844 / 0.15875 down per update from the clip root motion, top at 21.8 <= height + 1.8325, A / B / bottom exits at 0.77038 m from the axis, dismount 34 updates with x/z to 6e-3 and y to 6e-3.

## 4. Stand-ins

- TRAV-S5 `kPoleReach` 1.8325 — ctrl+0x250, the reach added to the climb height at the top, read in m12-10 for the player; its source in the character data is not traced. evidence: [player-traversal-pole.md §2026-10-18T16:08:28](../../re/player-traversal-pole.md#ts-20261018-160828)
- TRAV-S6 `kPoleExitMargin` — after any exit the player stands at CollisionBound half depth + 0.6 + 0.02038 from the pole axis (0.77038 measured for 1-5_Drain_Pipe05); the original's collision push-out is not ported. evidence: [player-traversal-pole.md §2026-10-18T16:08:28](../../re/player-traversal-pole.md#ts-20261018-160828)
- TRAV-S7 `poleTouched` — the walk-in touch is an overlap of the player box with the pole's CollisionBound box; the original uses the object gather of 0x21a73c. evidence: [player-traversal-pole.md §2026-10-18T16:08:28](../../re/player-traversal-pole.md#ts-20261018-160828)
- TRAV-S8 `PoleInput::floorBelow` / `floorHeight` — the bottom ray 0x22f134 is a ground probe of the step length, and the dismount's ground contact (raise to the floor, snap down within 0.1 while descending) stands in for 0x20cd30; rows 96 and 97 of the recording rise gradually, the port at once. evidence: [player-traversal-pole.md §2026-10-18T16:08:28](../../re/player-traversal-pole.md#ts-20261018-160828)
- TRAV-S9 pole inputs assumed — ability 32 is taken as present for the player, the node rows come from the yaw only, and the idle -> up test uses a step of 0 for the idle clip. evidence: [player-traversal-pole.md §2026-10-18T16:08:28](../../re/player-traversal-pole.md#ts-20261018-160828)

## 5. Open items

- [ ] TRAV-11 — Pole entry from the air (clip 218 `vpole_mount`, 0x206510 inside the jump-update test list 0x1d5254) not ported; only the ground entry is. evidence: [player-traversal-pole.md §2026-10-18T16:08:28](../../re/player-traversal-pole.md#ts-20261018-160828)
- [ ] TRAV-13 — The touch that starts the walk-in (DAT_00377134, set by the object gather 0x21a73c at 0x21a920) is approximated by TRAV-S7; read the gather and message 0x2d. evidence: [player-traversal-pole.md §2026-10-18T16:08:28](../../re/player-traversal-pole.md#ts-20261018-160828)
- [ ] TRAV-14 — Ground and wall contact of the pole exits and the dismount (push-out from the pole box, ray 0x22f134, 0x20cd30 in 0x215804, ext+0xc = 0.25 of the A drop) are modelled by TRAV-S6 and TRAV-S8. evidence: [player-traversal-pole.md §2026-10-18T16:08:28](../../re/player-traversal-pole.md#ts-20261018-160828)
- [ ] TRAV-15 — Pole sounds and the ability check: the hand sound ids (0x374518 / 0x374519 hold 0 in m12-10) and the ability table behind 0x23a87c for ability 32 are not read; no sound plays. needs: TRAV-S9. evidence: [player-traversal-pole.md §2026-10-18T16:08:28](../../re/player-traversal-pole.md#ts-20261018-160828)
- [ ] TRAV-16 — Poles with a tilted orientation, `prop_poleclimb` (no `CanDismount`) and the other 11 poles of lvl_01 are not measured; the node rows come from the yaw only (TRAV-S9). evidence: [player-traversal-pole.md §2026-10-18T16:08:28](../../re/player-traversal-pole.md#ts-20261018-160828)

## 6. Findings summary

- Pole walk-in, A press, climb, exits and dismount, measured in m12-10 / m12-10a: [player-traversal-pole.md §2026-10-18T16:08:28](../../re/player-traversal-pole.md#ts-20261018-160828).
- Pole use handler, mount and climb updates: [player-traversal-pole.md §2026-10-18T14:55:41](../../re/player-traversal-pole.md#ts-20261018-145541). Oracle states of mission 1-2 (user states made in mission 1-2 past the roof scanner): `m12-10-pole-climb` (on the pole, climbing up the house).

## 7. Code and tests

- `src/runtime/pole.{hpp,cpp}` (pole objects, zone and touch tests, walk-in and use gates, mount, idle, climb, exits, dismount), `src/game_pole.cpp` (`LevelSession::movePlayerWithPole`, wraps `movePlayer`), clips `vpole_*` loaded in `src/game_player.cpp`.
- CTest `pole` (made-up pole, hand values), `pole-real-data` (lvl_01 poles and the m12-10 / m12-10a replays), `game-script-pole` (chase-game from the m12-10a position: walk-in at update 5, A drop at update 15).
