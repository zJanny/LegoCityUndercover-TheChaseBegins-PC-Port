# Player traversal

## 1. Scope

The climbing and sliding moves of the player: zipline and tightrope (object type 101), pole and drain pipe (type 187), climbing bricks (type 20), free climb wall (type 30), rails, climb bars and flag poles (type 29, corner type 218) and the ledge states. Mission 1-2 needs the zipline, the drain pipe, the bricks and the rail before the dog cage (oracle states `m12-9-zipline` to `m12-12-railing`). Walking, jumping and falling are in [player-movement.md](player-movement.md).

## 2. Status

IN_PROGRESS, updated 2026-10-18T16:09:49+02:00. The zipline is ported in `chase-game`: grab from a jump under the line, use press at the high anchor, Get On, Slide, Dismount and the fall after it, with the clips `zipline_grab`, `zipline_slide`, `zipline_dismount` and the loop sound. The climbing bricks (states 141..152) are ported: grab from a jump, hang, reach, jump to the adjacent brick, drop with A, jump off with B; the m12-11 climb is pinned against the oracle. The rails of mission 1-2 are ported: grab from a jump, arrival, hang, shuffle left and right, the corner rail onto the next rail, A to drop and B to jump off. The jump off the last brick reaches the rail grab through the chained player step. The pole's use handler, mount and climb updates are read; the pole is not ported yet.

## 3. Verified behaviour

- Zipline ride replayed against the oracle (m12-9, dt 1/30): grab in the 7th jump update, rate 0.27367 per second, heading 15683, t and positions per update, Get On 38 updates, end event, dismount root motion, state 5 after 30 updates with fall speed 0.22844 and fall distance 0.45688 (CTest `zipline-real-data`).
- A B press during the slide starts the dismount in the same update (oracle run `zipb`, rule in CTest `zipline`).
- lvl_01: 24 zipline anchors, `1-8_Zip_Line05` / `04` paired and 05 in the line list as in the oracle read.
- Climbing bricks against m12-11 (dt 1/30): grab of 08 in the 7th jump update at (908.9421, 65.6976, 673.1696), heading 22209, 22 updates of state 141; reach in the first stick update; the jump up to 07 height per update and the grab up after 16 updates, idle after 14; B on 07 starts state 4 (CTests `brick-grab-real-data`, `game-script-brick-grab`).

- Rails (m12-12 replays, CTest `climb-bar-real-data`): shuffle along 03 to its end (60 updates, clip root motion at half frames 0.09406 / 0.09375 per update, first update 0.065), left over the corner rail (spline samples, 15 updates, t0 = 0.2) onto 02 and along it, jump off 03 and grabbed again in update 21, arrival snap, A drop. Tolerance 3e-4.
- The bar box test uses the player box (0, 1.15, 0.08) +- (0.6, 0.425, 0.52) against `GrabBound`; the bar is grabbed in the 7th jump update as the zipline ([evidence](../../re/player-traversal.md#ts-20261018-160503)).

## 4. Stand-ins

- TRAV-S1 `kZiplineGrabForwardOffset` — grab point = player position moved 0.08 along the facing with no height (entity+0x60 and the empty attachment set 18 of Chase read in m12-9; their source in the character data not traced). evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- TRAV-S2 `ZiplineAnchor::order` — level order decides which anchor of a pair is in the line list; the original compares the entity id +0x14. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- TRAV-S3 — fall distance during the dismount follows the m12-9 trace; the rule of 0x215804 is not read. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- TRAV-S10 `kStandInPlayerGrabOffset`, `kStandInPlayerGrabHalf` — the player's brick grab box uses entity+0x60 and +0x6c as read in m12-11; their source in the character data is not traced. evidence: [player-traversal.md §2026-10-18T15:57:46](../../re/player-traversal.md#ts-20261018-155746)
- TRAV-S11 — brick events: one state change per update in the order B, A, check, clip done, stick, and the released event at every state change; the original's order is not read. evidence: [player-traversal.md §2026-10-18T15:57:46](../../re/player-traversal.md#ts-20261018-155746)
- TRAV-S12 — the scaled brick jump (149..151) and state 152 move by the root track along the hanging axes without the collision of 0x20cd30; the x / z axis use is inferred. evidence: [player-traversal.md §2026-10-18T15:57:46](../../re/player-traversal.md#ts-20261018-155746)
- TRAV-S13 — the camera row 2 for the no-stick brick jump direction is built from the follow camera's yaw and pitch in `src/game_brick_grab.cpp`. evidence: [player-traversal.md §2026-10-18T15:57:46](../../re/player-traversal.md#ts-20261018-155746)
- TRAV-S4 — the loop sound 107 starts at the player's position with owner 0 and does not follow him; the original plays it on the player entity (0x23ae84). evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)

- TRAV-S15 `kClimbBarHangOffsetY/Z` — the hang offset (0, -1.0606986, 0.3636877) of Chase measured in m12-12; the original samples the hand joint of `ledge_idle` (0x27086c). evidence: [player-traversal.md §2026-10-18T16:05:03](../../re/player-traversal.md#ts-20261018-160503)
- TRAV-S16 — the bar state comp+2 (1..3) and the ability check 0x227b5c are taken as always passed; the bar list is in level order; the grab sound ATTR_SFX_GRAB plays at the player with owner 0 when set. evidence: [player-traversal.md §2026-10-18T16:05:03](../../re/player-traversal.md#ts-20261018-160503)
- TRAV-S17 — the order of state changes inside one update is the measured sequence per case (hang request next update, corner exit arrival in the same update, jump grab arrival next update, idle placement update without input), not the state machine 0x2eafc8. evidence: [player-traversal.md §2026-10-18T16:05:03](../../re/player-traversal.md#ts-20261018-160503)
- TRAV-S18 — simplifications: bar yaw only (no pitch or roll), the stick direction is the world dot (camera transform 0x229a40 and pitch ignored), the hang move has no world collision (0x215804), blocked means a horizontal delta under 1e-5, the root motion loop wraps continuously. evidence: [player-traversal.md §2026-10-18T16:05:03](../../re/player-traversal.md#ts-20261018-160503)
- TRAV-S19 — a bar of the `Ledge`, `NarrowBar`, `AcrobatPole` or crumbling kind behaves as a normal bar. evidence: [player-traversal.md §2026-10-18T16:05:03](../../re/player-traversal.md#ts-20261018-160503)

## 5. Open items

- [ ] TRAV-1 — Pole and drain pipe (type 187, states 0x11d..0x121) not ported; mount and climb updates read, the use button, pole bounds, climb-down and dismount updates and an oracle run in m12-10 missing. blocks: PLAYER-7. evidence: [player-traversal.md §2026-10-18T14:55:41](../../re/player-traversal.md#ts-20261018-145541)
- [ ] TRAV-2 — Free climb wall (type 30, entered by its handler 0x1b5454 into state 99; states 99..110) not read or ported; the bricks are ported, the wall is not on the mission 1-2 route. blocks: PLAYER-7. evidence: [player-traversal.md §2026-10-18T15:57:46](../../re/player-traversal.md#ts-20261018-155746)
- [ ] TRAV-19 — Brick jumps to the side (150 / 151) and the axes of their scaled root motion are not measured (bricks 1-2_Brick_Grab_01..06, 09). needs: TRAV-S12. evidence: [player-traversal.md §2026-10-18T15:57:46](../../re/player-traversal.md#ts-20261018-155746)
- [ ] TRAV-20 — Brick state 152 (`IsTopBrick`) not measured; its ground probe 0x20b48c and the collision of 0x20cd30 in the brick jumps are not ported. evidence: [player-traversal.md §2026-10-18T15:57:46](../../re/player-traversal.md#ts-20261018-155746)
- [ ] TRAV-21 — Brick use handler 0x1b53a8 (messages 3 / 4 into state 145): sender unknown; `TriggerObject` switching (0x244f40 / 0x244f88) not ported. evidence: [player-traversal.md §2026-10-18T15:57:46](../../re/player-traversal.md#ts-20261018-155746)
- [ ] TRAV-22 — Brick checks not ported: the character test 0x227b5c for type 20, brick +0x8c bit 4 and entity+0xc bit 4 (the port checks only the disabled flag and carrying). evidence: [player-traversal.md §2026-10-18T15:57:46](../../re/player-traversal.md#ts-20261018-155746)
- [ ] TRAV-27 — Narrow bars (`NarrowBar`, clip 0x2a, no sideways move), acrobat poles (`AcrobatPole`, ability 9, clip 0x2c), crumbling bars (state 0x5c `HOT`, update 0x2b7528) and the `Ledge` variant (jump off to state 24, state 23) are not ported. needs: TRAV-S19. evidence: [player-traversal.md §2026-10-18T16:05:03](../../re/player-traversal.md#ts-20261018-160503)
- [ ] TRAV-28 — The hang offset comes from the hand joint of the ledge clip (0x27086c), not from a constant. needs: TRAV-S15. evidence: [player-traversal.md §2026-10-18T16:05:03](../../re/player-traversal.md#ts-20261018-160503)
- [ ] TRAV-29 — The fall-state grab path for bars (state 5 is accepted by 0x227b5c), the exact time ctrl+0x138 is set and the bar state comp+2 are not read. needs: TRAV-S16. evidence: [player-traversal.md §2026-10-18T16:05:03](../../re/player-traversal.md#ts-20261018-160503)
- [ ] TRAV-31 — The corner in the other direction (state 0x17a, t falling) and the hang move beyond the end without a corner are not measured. evidence: [player-traversal.md §2026-10-18T16:05:03](../../re/player-traversal.md#ts-20261018-160503)
- [ ] TRAV-32 — `PrimaryNextBar` chains and the secondary bars are read by the AI follower (0x2c7108) only as far as seen; whether the player uses them is not found. evidence: [player-traversal.md §2026-10-18T16:05:03](../../re/player-traversal.md#ts-20261018-160503)
- [ ] TRAV-33 — The hang move is applied without the world collision of 0x215804, and the stick direction ignores the camera transform. needs: TRAV-S18. evidence: [player-traversal.md §2026-10-18T16:05:03](../../re/player-traversal.md#ts-20261018-160503)
- [ ] TRAV-5 — The other grab tests of the jump update (0x206668 .. 0x21e038, 0x273f6c, 0x20614c) and the fall-state path 0x2010d4 (a zipline grabbed while falling) are not read. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- [ ] TRAV-6 — Ground contact during the dismount (0x215804): the port moves by the root track without collision until state 5. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- [ ] TRAV-7 — Rope between the anchors (draw 0x1a9dc4 -> 0x297b7c, `RopeTexture`, `RenderWidth` 0.03, fade near the camera) is not drawn. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- [ ] TRAV-8 — The use press at a high anchor (0x1b6664) is ported but not measured; needs an oracle state on a zipline start platform. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- [ ] TRAV-9 — Tightrope (`GrappleLineType` 0, states 39..41) not ported. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- [ ] TRAV-10 — The hanging pose on the line (clip root against the line point) is not compared with the original's picture. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)

## 6. Findings summary

- Rails, climb bars, corner rail, ledge states: [player-traversal.md §2026-10-18T16:05:03](../../re/player-traversal.md#ts-20261018-160503).

- Zipline class, grab test, states 42..44, oracle ride and the survey of the other moves: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230).
- Climbing bricks, the m12-11 runs and the climb wall's entry: [player-traversal.md §2026-10-18T15:57:46](../../re/player-traversal.md#ts-20261018-155746).
- Pole use handler, mount and climb updates: [player-traversal.md §2026-10-18T14:55:41](../../re/player-traversal.md#ts-20261018-145541). Oracle states of mission 1-2: [oracle-states.md §2026-10-18T12:27:39](../../re/oracle-states.md#ts-20261018-122739).

## 7. Code and tests

- `src/runtime/climb_bar.{hpp,cpp}` (bars, corner rails, box test, spline, hang update, corner update), `src/game_climb_bar.cpp` (`LevelSession::climbBarMovePlayer` wraps `movePlayer`: while a bar state owns the player it replaces the locomotion, otherwise the grab test follows the jump update), clips `ledge_idle` / `ledge_left` / `ledge_right` loaded in `src/game_player.cpp`. CTest `climb-bar` (made-up bar), `climb-bar-real-data` (lvl_01 bars and the m12-12 replays).
- `src/runtime/zipline.{hpp,cpp}` (anchors, link, grab and use tests, attach, slide, dismount, jump counter), `src/game_zipline.cpp` (LevelSession::movePlayer: zipline states around the player step), clips loaded in `src/game_player.cpp`.
- CTest `zipline` (made-up line, hand values), `zipline-real-data` (lvl_01 anchors and the m12-9 ride replay), `game-script-zipline` (chase-game from the m12-9 position: grab at update 8, fall at update 140).
- `src/runtime/brick_grab.{hpp,cpp}` (bricks, grab test 0x273fe4, states 141..152), `src/game_brick_grab.cpp` (LevelSession::moveClimbingPlayer around movePlayer), brick clips loaded in `src/game_player.cpp`. CTest `brick-grab` (made-up bricks), `brick-grab-real-data` (lvl_01 bricks, m12-11 runs `up` and `drop`), `game-script-brick-grab` (chase-game from the m12-11 landing point: grab, jump up, jump off).
