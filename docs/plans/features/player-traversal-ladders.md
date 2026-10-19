# Player traversal: ladders

## 1. Scope

The ladders (object kind 56: `props/prop_ladderlarge`, `props/prop_laddermedium`, `props/prop_crane_ladderlarge` and the other kind 56 classes) of [player-traversal.md](player-traversal.md): use zone, mount by the A press or by walking in, climb up and down, the clamber onto the top (state 23), the exits and their animations. Mission 1-3 climbs `1-4_Ladder01` to the roof (state `m13-2-ladder`). Findings: [docs/re/player-traversal-ladders.md](../../re/player-traversal-ladders.md).

## 2. Status

IN_PROGRESS, updated 2026-10-20T00:29:37+02:00. Ported in `chase-game` from the original and replayed against the oracle runs of `m13-2-ladder`: the A press and the walk-in mount, idle, climb up and down, the clamber onto the roof, the A drop, the B jump and the fall at the bottom; the mission 1-3 ladder takes Chase to the roof (TRAV-57, closed in [player-traversal-ladders.closed.md](player-traversal-ladders.closed.md)). The entry from the air, furled ladders, sounds and the camera are not ported.

## 3. Verified behaviour

- Ladder of m13-2-ladder replayed against the oracle (dt 1/30; CTest `ladder-real-data`): A-press mount in 4 updates to the snap point 0.45 in front of the ladder with positions to 2e-4 and the facing turned at the end, climb 0.05 per update, the top at height + 1.8325 >= 12.17547 in row 121, the clamber by the root motion of `ladder_dismount` (y to 2e-4, x/z to 2e-4 before the roof edge) and state 1 31 updates after the top; the climb down meets the platform, A drops and B jumps in the press update with the recorded facings.
- `chase-game` from the m13-2 position: A climbs `1-4_Ladder01` onto the roof (y 30.7), the stick towards the ladder walks in (CTests `game-script-ladder`, `game-script-ladder-walk-in`).

## 4. Stand-ins

- TRAV-S34 `loadLadders`, `ladderAccepts` — ladder inputs assumed: kind 56 objects nested in a class file are not loaded (their parent's bounds cover them; entity+0x80 of 0x239678 not traced), the node rows come from the yaw only, the climb step 0.05 (*0x377274, its writer not read) and the player half height 0.85 (entity+0x70) are values read in m13-2-ladder, ability 5 is taken as present for the player, the carry gate 0x1ff208 as "carrying nothing", and the A press offers the ladders before the poles and the ziplines. evidence: [player-traversal-ladders.md §2026-10-20T00:29:37](../../re/player-traversal-ladders.md#ts-20261020-002937)
- TRAV-S35 `LadderInput::floorBelow` / `floorHeight`, `ladderWalkInTest` — ladder moves without the original's collision: the walk-in touch is the box overlap of TRAV-S7, the climb-down ray 0x22f134 is a ground probe of 0.1 + the step, the clamber move 0x215804 has floor contact only (no wall stop at the roof edge), the clamber ends when the clip time reaches its length - 1e-3 (chosen by the port for the Default Anim Done Event), and the A drop has no push-out. needs: TRAV-59. evidence: [player-traversal-ladders.md §2026-10-20T00:29:37](../../re/player-traversal-ladders.md#ts-20261020-002937)

## 5. Open items

- [ ] TRAV-58 — Ladder entry from a jump or a fall (0x206668 in the jump-update test list 0x1d5254 and the fall chain 0x2010d4) is not ported, and furled ladders (`StartFurled`, messages 8 / 0xff and `UnfurlAnimation` of 0x18b458) are never unfurled. evidence: [player-traversal-ladders.md §2026-10-20T00:29:37](../../re/player-traversal-ladders.md#ts-20261020-002937)
- [ ] TRAV-59 — Collision of the ladder exits: the push-out 0.22 m off the ladder after the A drop (adrop row 14), the wall stop of the clamber move at the roof edge (lad-up row 131) and the message 0x39 that the jump-off and the top send to the player are not read. blocks: TRAV-S35. evidence: [player-traversal-ladders.md §2026-10-20T00:29:37](../../re/player-traversal-ladders.md#ts-20261020-002937)
- [ ] TRAV-60 — Ladder sounds (`sounds/movement/ladder_*`), the clip restart of 0x22f560 in the climbs, the hidden weapons and whether the camera changes on the ladder are not read or measured. evidence: [player-traversal-ladders.md §2026-10-20T00:29:37](../../re/player-traversal-ladders.md#ts-20261020-002937)

## 6. Findings summary

- Kind 56 descriptor, handler 0x18b458, states 33..37 and 23, the oracle runs of m13-2-ladder: [player-traversal-ladders.md §2026-10-20T00:29:37](../../re/player-traversal-ladders.md#ts-20261020-002937).
- The upper half of the crane ladder is a nested object: [object-kinds.md §2026-10-19T19:25:32](../../re/object-kinds.md#ts-20261019-192532). User report: [playtests.md §2026-10-19T20:23:38](../../re/playtests.md#ts-20261019-202338).

## 7. Code and tests

- `src/runtime/ladder.{hpp,cpp}` (ladder objects, zone, handler gate, walk-in and use tests, mount, idle, climbs, clamber, exits), `src/game_ladder.cpp` (`LevelSession::movePlayerWithLadder`, wraps `movePlayerWithPole`), clips `ladder_*` loaded in `src/game_player.cpp`; `playerBoxTouches` in `src/runtime/pole.cpp`.
- CTest `ladder` (made-up ladder, hand values), `ladder-real-data` (lvl_01 ladders and the runs lad-up, down, adrop, bjump), `game-script-ladder` (chase-game from the m13-2 position: A at update 2, clamber at update 121, on the roof at update 153), `game-script-ladder-walk-in`.
