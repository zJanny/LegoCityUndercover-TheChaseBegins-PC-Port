# Player traversal

## 1. Scope

The climbing and sliding moves of the player: zipline and tightrope (object type 101), pole and drain pipe (type 187), climbing bricks (type 20), free climb wall (type 30), rails, climb bars and flag poles (type 29, corner type 218) and the ledge states. Mission 1-2 needs the zipline, the drain pipe, the bricks and the rail before the dog cage (oracle states `m12-9-zipline` to `m12-12-railing`). Walking, jumping and falling are in [player-movement.md](player-movement.md).

## 2. Status

IN_PROGRESS, updated 2026-10-18T14:56:25+02:00. The zipline is ported in `chase-game`: grab from a jump under the line, use press at the high anchor, Get On, Slide, Dismount and the fall after it, with the clips `zipline_grab`, `zipline_slide`, `zipline_dismount` and the loop sound. The other moves are surveyed (classes, descriptors, state numbers) and the pole's use handler, mount and climb updates are read; none of them is ported.

## 3. Verified behaviour

- Zipline ride replayed against the oracle (m12-9, dt 1/30): grab in the 7th jump update, rate 0.27367 per second, heading 15683, t and positions per update, Get On 38 updates, end event, dismount root motion, state 5 after 30 updates with fall speed 0.22844 and fall distance 0.45688 (CTest `zipline-real-data`).
- A B press during the slide starts the dismount in the same update (oracle run `zipb`, rule in CTest `zipline`).
- lvl_01: 24 zipline anchors, `1-8_Zip_Line05` / `04` paired and 05 in the line list as in the oracle read.

## 4. Stand-ins

- TRAV-S1 `kZiplineGrabForwardOffset` — grab point = player position moved 0.08 along the facing with no height (entity+0x60 and the empty attachment set 18 of Chase read in m12-9; their source in the character data not traced). evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- TRAV-S2 `ZiplineAnchor::order` — level order decides which anchor of a pair is in the line list; the original compares the entity id +0x14. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- TRAV-S3 — fall distance during the dismount follows the m12-9 trace; the rule of 0x215804 is not read. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- TRAV-S4 — the loop sound 107 starts at the player's position with owner 0 and does not follow him; the original plays it on the player entity (0x23ae84). evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)

## 5. Open items

- [ ] TRAV-1 — Pole and drain pipe (type 187, states 0x11d..0x121) not ported; mount and climb updates read, the use button, pole bounds, climb-down and dismount updates and an oracle run in m12-10 missing. blocks: PLAYER-7. evidence: [player-traversal.md §2026-10-18T14:55:41](../../re/player-traversal.md#ts-20261018-145541)
- [ ] TRAV-2 — Climbing bricks (type 20) and climb wall (type 30, Climb Wall states 99..110) not ported; which class drives the Climb Wall states is not read (m12-11). blocks: PLAYER-7. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- [ ] TRAV-3 — Rails and climb bars (type 29, corner type 218, CLIMB BAR states 89..92) not ported (m12-12). blocks: PLAYER-7. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- [ ] TRAV-4 — Ledge states 23 and 24 and the "Ledge" use in the type 29 code not read. blocks: PLAYER-7. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- [ ] TRAV-5 — The other grab tests of the jump update (0x206668 .. 0x21e038, 0x273f6c, 0x20614c) and the fall-state path 0x2010d4 (a zipline grabbed while falling) are not read. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- [ ] TRAV-6 — Ground contact during the dismount (0x215804): the port moves by the root track without collision until state 5. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- [ ] TRAV-7 — Rope between the anchors (draw 0x1a9dc4 -> 0x297b7c, `RopeTexture`, `RenderWidth` 0.03, fade near the camera) is not drawn. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- [ ] TRAV-8 — The use press at a high anchor (0x1b6664) is ported but not measured; needs an oracle state on a zipline start platform. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- [ ] TRAV-9 — Tightrope (`GrappleLineType` 0, states 39..41) not ported. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- [ ] TRAV-10 — The hanging pose on the line (clip root against the line point) is not compared with the original's picture. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)

## 6. Findings summary

- Zipline class, grab test, states 42..44, oracle ride and the survey of the other moves: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230).
- Pole use handler, mount and climb updates: [player-traversal.md §2026-10-18T14:55:41](../../re/player-traversal.md#ts-20261018-145541). Oracle states of mission 1-2: [oracle-states.md §2026-10-18T12:27:39](../../re/oracle-states.md#ts-20261018-122739).

## 7. Code and tests

- `src/runtime/zipline.{hpp,cpp}` (anchors, link, grab and use tests, attach, slide, dismount, jump counter), `src/game_zipline.cpp` (LevelSession::movePlayer: zipline states around the player step), clips loaded in `src/game_player.cpp`.
- CTest `zipline` (made-up line, hand values), `zipline-real-data` (lvl_01 anchors and the m12-9 ride replay), `game-script-zipline` (chase-game from the m12-9 position: grab at update 8, fall at update 140).
