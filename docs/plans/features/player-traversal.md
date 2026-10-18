# Player traversal

## 1. Scope

The climbing and sliding moves of the player: zipline and tightrope (object type 101), pole and drain pipe (type 187), climbing bricks (type 20), free climb wall (type 30), rails, climb bars and flag poles (type 29, corner type 218) and the ledge states. Mission 1-2 needs the zipline, the drain pipe, the bricks and the rail before the dog cage (oracle states `m12-9-zipline` to `m12-12-railing`). Walking, jumping and falling are in [player-movement.md](player-movement.md).

## 2. Status

IN_PROGRESS, updated 2026-10-18T16:38:10+02:00. Ported in `chase-game` from the original, with values pinned against the user's mission 1-2 states: the zipline (grab from a jump, use press at the high anchor, Get On, Slide, Dismount, fall), the pole and drain pipe (walk-in and A-press mount, climb up and down, A / B / bottom exits, dismount at the top), the climbing bricks (grab, hang, reach, jump to the adjacent brick, drop, jump off) and the rails (grab, arrival, hang, shuffle, the corner rail onto the next rail, A drop, B jump off). The player step chains them: rails around bricks around the pole around the usual step, so the jump off the last brick grabs the rail as in the original. The free climb wall (not on the mission route) is not ported.

## 3. Verified behaviour

- Zipline ride replayed against the oracle (m12-9, dt 1/30): grab in the 7th jump update, rate 0.27367 per second, heading 15683, t and positions per update, Get On 38 updates, end event, dismount root motion, state 5 after 30 updates with fall speed 0.22844 and fall distance 0.45688 (CTest `zipline-real-data`).
- A B press during the slide starts the dismount in the same update (oracle run `zipb`, rule in CTest `zipline`).
- lvl_01: 24 zipline anchors, `1-8_Zip_Line05` / `04` paired and 05 in the line list as in the oracle read.
- Climbing bricks against m12-11 (dt 1/30): grab of 08 in the 7th jump update at (908.9421, 65.6976, 673.1696), heading 22209, 22 updates of state 141; reach in the first stick update; the jump up to 07 height per update and the grab up after 16 updates, idle after 14; B on 07 starts state 4 (CTests `brick-grab-real-data`, `game-script-brick-grab`).

## 4. Stand-ins

- TRAV-S1 `kZiplineGrabForwardOffset` — grab point = player position moved 0.08 along the facing with no height (entity+0x60 and the empty attachment set 18 of Chase read in m12-9; their source in the character data not traced). evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- TRAV-S2 `ZiplineAnchor::order` — level order decides which anchor of a pair is in the line list; the original compares the entity id +0x14. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- TRAV-S3 — fall distance during the dismount follows the m12-9 trace; the rule of 0x215804 is not read. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- TRAV-S10 `kStandInPlayerGrabOffset`, `kStandInPlayerGrabHalf` — the player's brick grab box uses entity+0x60 and +0x6c as read in m12-11; their source in the character data is not traced. evidence: [player-traversal-bricks.md §2026-10-18T15:57:46](../../re/player-traversal-bricks.md#ts-20261018-155746)
- TRAV-S11 — brick events: one state change per update in the order B, A, check, clip done, stick, and the released event at every state change; the original's order is not read. evidence: [player-traversal-bricks.md §2026-10-18T15:57:46](../../re/player-traversal-bricks.md#ts-20261018-155746)
- TRAV-S12 — the scaled brick jump (149..151) and state 152 move by the root track along the hanging axes without the collision of 0x20cd30; the x / z axis use is inferred. evidence: [player-traversal-bricks.md §2026-10-18T15:57:46](../../re/player-traversal-bricks.md#ts-20261018-155746)
- TRAV-S13 — the camera row 2 for the no-stick brick jump direction is built from the follow camera's yaw and pitch in `src/game_brick_grab.cpp`. evidence: [player-traversal-bricks.md §2026-10-18T15:57:46](../../re/player-traversal-bricks.md#ts-20261018-155746)
- TRAV-S4 — the loop sound 107 starts at the player's position with owner 0 and does not follow him; the original plays it on the player entity (0x23ae84). evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- TRAV-S20 — Use registry order: the pole use test runs before the zipline use test in the player chain, so a pole zone and an anchor zone that overlap give the press to the pole; the original walks one registry in order (0x1fef10). evidence: [player-traversal.md §2026-10-18T16:37:26](../../re/player-traversal.md#ts-20261018-163726)

## 5. Open items

- [ ] TRAV-2 — Free climb wall (type 30, entered by its handler 0x1b5454 into state 99; states 99..110) not read or ported; the bricks are ported, the wall is not on the mission 1-2 route. blocks: PLAYER-7. evidence: [player-traversal-bricks.md §2026-10-18T15:57:46](../../re/player-traversal-bricks.md#ts-20261018-155746)
- [ ] TRAV-19 — Brick jumps to the side (150 / 151) and the axes of their scaled root motion are not measured (bricks 1-2_Brick_Grab_01..06, 09). needs: TRAV-S12. evidence: [player-traversal-bricks.md §2026-10-18T15:57:46](../../re/player-traversal-bricks.md#ts-20261018-155746)
- [ ] TRAV-20 — Brick state 152 (`IsTopBrick`) not measured; its ground probe 0x20b48c and the collision of 0x20cd30 in the brick jumps are not ported. evidence: [player-traversal-bricks.md §2026-10-18T15:57:46](../../re/player-traversal-bricks.md#ts-20261018-155746)
- [ ] TRAV-21 — Brick use handler 0x1b53a8 (messages 3 / 4 into state 145): sender unknown; `TriggerObject` switching (0x244f40 / 0x244f88) not ported. evidence: [player-traversal-bricks.md §2026-10-18T15:57:46](../../re/player-traversal-bricks.md#ts-20261018-155746)
- [ ] TRAV-22 — Brick checks not ported: the character test 0x227b5c for type 20, brick +0x8c bit 4 and entity+0xc bit 4 (the port checks only the disabled flag and carrying). evidence: [player-traversal-bricks.md §2026-10-18T15:57:46](../../re/player-traversal-bricks.md#ts-20261018-155746)
- [ ] TRAV-34 — The A-press mount of a pole (0x2011b0 -> 0x1fef10, ported) is not measured; `m12-10a` / `m12-10b` do not fit (user state deferred). needs: oracle state facing a pipe in its use zone. evidence: [player-traversal.md §2026-10-18T20:38:04](../../re/player-traversal.md#ts-20261018-203804)
- [ ] TRAV-35 — The zipline handler 0x1b6664 answers 0xff unless the pressing entity has ability 0xb or 0xc; the port does not test it (Chase has it, other characters unknown). evidence: [player-traversal.md §2026-10-18T16:37:26](../../re/player-traversal.md#ts-20261018-163726)
- [ ] TRAV-5 — The other grab tests of the jump update (0x206668 .. 0x21e038, 0x273f6c, 0x20614c) and of the fall-state chain 0x2068f4 -> 0x2010d4 (order read; a zipline or brick grabbed while falling) are not read or ported; only the bar test runs in a fall. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- [ ] TRAV-6 — Ground contact during the dismount (0x215804): the port moves by the root track without collision until state 5. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- [ ] TRAV-7 — Rope between the anchors (draw 0x1a9dc4 -> 0x297b7c, `RopeTexture`, `RenderWidth` 0.03, fade near the camera) is not drawn; the user sees no zipline on the mission 1-2 roof in `chase-game` (playtest 2026-10-18), while the ride works. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- [ ] TRAV-8 — The use press at a high anchor (0x1b6664, A press) is ported but not measured; `m12-9-zipline` is on the ground below the anchor (user state deferred); needs an oracle state on a zipline start platform (stand 0.2 m before the high anchor of a zipline on the mission 1-2 route, facing it). evidence: [player-traversal.md §2026-10-18T20:38:04](../../re/player-traversal.md#ts-20261018-203804)
- [ ] TRAV-9 — Tightrope (`GrappleLineType` 0, states 39..41) not ported. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- [ ] TRAV-10 — The hanging pose on the line (clip root against the line point) is not compared with the original's picture. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)

## 6. Findings summary

- Subtopic files: [player-traversal-pole.md](player-traversal-pole.md) (pole and drain pipe), [player-traversal-rails.md](player-traversal-rails.md) (rails, climb bars, corner rail).
- Zipline class, grab test, states 42..44, oracle ride and the survey of the other moves: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230).
- Zipline use press is A: [player-traversal.md §2026-10-18T16:37:26](../../re/player-traversal.md#ts-20261018-163726).
- Climbing bricks, the m12-11 runs and the climb wall's entry: [player-traversal-bricks.md §2026-10-18T15:57:46](../../re/player-traversal-bricks.md#ts-20261018-155746).

## 7. Code and tests

- `src/runtime/zipline.{hpp,cpp}` (anchors, link, grab and use tests, attach, slide, dismount, jump counter), `src/game_zipline.cpp` (LevelSession::movePlayer: zipline states around the player step), clips loaded in `src/game_player.cpp`.
- CTest `zipline` (made-up line, hand values), `zipline-real-data` (lvl_01 anchors and the m12-9 ride replay), `game-script-zipline` (chase-game from the m12-9 position: grab at update 8, fall at update 140), `game-script-zipline-use-a` / `-use-b` (A gets on at 1-8_Zip_Line05, B does not).
- `src/runtime/brick_grab.{hpp,cpp}` (bricks, grab test 0x273fe4, states 141..152), `src/game_brick_grab.cpp` (LevelSession::moveClimbingPlayer around movePlayer), brick clips loaded in `src/game_player.cpp`. CTest `brick-grab` (made-up bricks), `brick-grab-real-data` (lvl_01 bricks, m12-11 runs `up` and `drop`), `game-script-brick-grab` (chase-game from the m12-11 landing point: grab, jump up, jump off).
