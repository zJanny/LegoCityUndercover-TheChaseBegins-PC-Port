# Player traversal: zipline and tightrope

## 1. Scope

The zipline and the tightrope (object type 101, `props/parkour_zipline`, `props/parkour_tightrope`) of [player-traversal.md](player-traversal.md): the grab from a jump, the use press at the high anchor, the states Get On, Slide and Dismount, the rope and the anchor poles. Mission 1-2 needs the zipline (oracle save state under the zipline of mission 1-2). Findings: [docs/re/player-traversal.md](../../re/player-traversal.md).

## 2. Status

IN_PROGRESS, updated 2026-10-27T18:29:07+01:00 (the zipline's handcuffs are drawn, TRAV-56 closed in [player-traversal.closed.md](player-traversal.closed.md)). Ported in `chase-game` from the original, with values pinned against the save state under the zipline of mission 1-2: the grab from a jump, the use press at the high anchor, Get On, Slide, Dismount and the fall; the rope and the anchor poles are drawn. The tightrope is not ported. Split from [player-traversal.md](player-traversal.md) on 2026-10-30.

## 3. Verified behaviour

- Zipline ride replayed against the oracle (the save state under the zipline of mission 1-2, dt 1/30): grab in the 7th jump update, rate 0.27367 per second, heading 15683, t and positions per update, Get On 38 updates, end event, dismount root motion, state 5 after 30 updates with fall speed 0.22844 and fall distance 0.45688 (CTest `zipline-real-data`).
- A B press during the slide starts the dismount in the same update (oracle run `zipb`, rule in CTest `zipline`).
- lvl_01: 24 zipline anchors, `1-8_Zip_Line05` / `04` paired and 05 in the line list as in the oracle read.

## 4. Stand-ins

- TRAV-S1 `kZiplineGrabForwardOffset` — grab point = player position moved 0.08 along the facing with no height (entity+0x60 and the empty attachment set 18 of Chase read in the save state under the zipline of mission 1-2; their source in the character data not traced). evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- TRAV-S2 `ZiplineAnchor::order` — level order decides which anchor of a pair is in the line list; the original compares the entity id +0x14. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- TRAV-S3 — fall distance during the dismount follows the save state under the zipline of mission 1-2 trace; the rule of 0x215804 is not read. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- TRAV-S29 `ziplineRopeStrip` — the ribbon is turned by the game's view direction (`viewDirection`); the row at 0x529fec was measured equal to the camera's view direction in one view of the save state under the zipline of mission 1-2 only. evidence: [player-traversal.md §2026-10-19T10:00:18](../../re/player-traversal.md#ts-20261019-100018)
- TRAV-S31 `RopeRenderer` fog — the rope uses the scene's current fog (same table, colour and projection planes) because the fog registers are global state; the draw path 0x297b7c was not read for its own fog writes. needs: TRAV-52. evidence: [player-traversal.md §2026-10-19T10:38:41+02:00](../../re/player-traversal.md#ts-20261019-103841)
- TRAV-S30 `RopeRenderer` — the texture is clamped across, repeated along and linear filtered without mipmaps (the original's sampler is not read); the disabled flag 0x10 bit 0 of the draw gate is taken from the placed object's disabled state. needs: TRAV-49. evidence: [player-traversal.md §2026-10-19T10:00:18](../../re/player-traversal.md#ts-20261019-100018)
- TRAV-S32 `ziplineRopeDraws` — flag bits 2 and 5 of the gate 0x226644 (mask 0x25) are taken as clear for the target; their meaning is not read. evidence: [player-traversal.md §2026-10-19T11:04:28+02:00](../../re/player-traversal.md#ts-20261019-110428)
- TRAV-S4 — the loop sound 107 starts at the player's position with owner 0 and does not follow him; the original plays it on the player entity (0x23ae84). evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- TRAV-S20 — Use registry order: the pole use test runs before the zipline use test in the player chain, so a pole zone and an anchor zone that overlap give the press to the pole; the original walks one registry in order (0x1fef10). evidence: [player-traversal.md §2026-10-18T16:37:26](../../re/player-traversal.md#ts-20261018-163726)

## 5. Open items

- [ ] TRAV-35 — The zipline handler 0x1b6664 answers 0xff unless the pressing entity has ability 0xb or 0xc; the port does not test it (Chase has it, other characters unknown). evidence: [player-traversal.md §2026-10-18T16:37:26](../../re/player-traversal.md#ts-20261018-163726)
- [ ] TRAV-6 — Ground contact during the dismount (0x215804): the port moves by the root track without collision until state 5. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- [ ] TRAV-49 — The rope texture's wrap and filter come from the texture object (the slot bytes 4 and 5 are zero and unused by 0x25511c); the loader 0x250244 was not read. blocks: TRAV-S30. evidence: [player-traversal.md §2026-10-19T10:00:18](../../re/player-traversal.md#ts-20261019-100018)
- [ ] TRAV-52 — The fogged rope was never compared with an oracle picture (RPC screenshot at 220 .. 260 m from a rope, or the fog registers read while 0x297b7c draws). blocks: TRAV-S31. evidence: [player-traversal.md §2026-10-19T10:38:41+02:00](../../re/player-traversal.md#ts-20261019-103841)
- [ ] TRAV-48 — The ropes of the tightrope anchors (`GrappleLineType` 0) use the same draw 0x1a9dc4 and are not loaded, so they are not drawn. needs: TRAV-9. blocks: TRAV-54. uses: parkour_tightrope. evidence: [player-traversal.md §2026-10-19T08:51:31](../../re/player-traversal.md#ts-20261019-085131)
- [ ] TRAV-8 — The use press at a high anchor (0x1b6664, A press) is ported but not measured; the save state under the zipline of mission 1-2 is on the ground below the anchor (save state deferred); needs an oracle state on a zipline start platform (stand 0.2 m before the high anchor of a zipline on the mission 1-2 route, facing it). evidence: [player-traversal.md §2026-10-18T20:38:04](../../re/player-traversal.md#ts-20261018-203804)
- [ ] TRAV-9 — Tightrope (`GrappleLineType` 0, states 39..41) not ported. blocks: TRAV-48. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- [ ] TRAV-10 — The hanging pose on the line (clip root against the line point) is not compared with the original's picture. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)
- [ ] TRAV-54 — The poles of the tightrope anchors (`props/parkour_tightrope`, 4 in lvl_01) are drawn at `initialorientation`; the link 0x19b074 would turn them towards their target. needs: TRAV-48. uses: parkour_tightrope. evidence: [player-traversal.md §2026-10-19T18:16:50](../../re/player-traversal.md#ts-20261019-181650)
- [ ] TRAV-55 — The zipline poles (`props/parkour_zipline` anchors) can be walked through in `chase-game` (user report); in the original they are solid. Where their collision comes from is not read. uses: parkour_zipline. evidence: [playtests.md §2026-10-19T20:15:35](../../re/playtests.md#ts-20261019-201535)
- [ ] TRAV-63 — The handcuffs on the rope: whether they stay in the hand through the `zipline_left*` / `zipline_right*` clips (no track, the port keeps the last state) and how they sit on the line is not compared with the oracle. needs: DETECT-19. evidence: [hand-items.md §2026-10-27T18:27:37](../../re/hand-items.md#ts-20261027-182737)

## 6. Findings summary

- The handcuffs on the zipline and every other hand item: [hand-items.md §2026-10-27T18:27:37](../../re/hand-items.md#ts-20261027-182737).
- Zipline class, grab test, states 42..44, oracle ride and the survey of the other moves: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230).
- Zipline rope: [player-traversal.md §2026-10-19T08:51:31](../../re/player-traversal.md#ts-20261019-085131); fog on the rope: [player-traversal.md §2026-10-19T10:38:41+02:00](../../re/player-traversal.md#ts-20261019-103841).
- Zipline use press is A: [player-traversal.md §2026-10-18T16:37:26](../../re/player-traversal.md#ts-20261018-163726).
- Zipline pole model and its node matrix: [player-traversal.md §2026-10-19T18:16:50](../../re/player-traversal.md#ts-20261019-181650).

## 7. Code and tests

- `src/runtime/zipline.{hpp,cpp}` (anchors, link, grab and use tests, attach, slide, dismount, jump counter), `src/game_zipline.cpp` (LevelSession::movePlayer: zipline states around the player step), clips loaded in `src/game_player.cpp`.
- CTest `zipline` (made-up line, hand values), `zipline-real-data` (lvl_01 anchors and the save state under the zipline of mission 1-2 ride replay), `game-script-zipline` (chase-game from the save state under the zipline of mission 1-2 position: grab at update 8, fall at update 140), `game-script-zipline-use-a` / `-use-b` (A gets on at 1-8_Zip_Line05, B does not).
- `src/runtime/zipline_rope.{hpp,cpp}` (rope data, fade, strip, draw filter), `src/render/rope.{hpp,cpp}` (GL), `LevelSession::drawZiplineRopes` in `src/game_zipline.cpp`; CTests `zipline-rope` (made-up line) and `zipline-rope-real-data` (lvl_01 ropes and the strip of 1-8_Zip_Line05).
- `src/runtime/zipline_pole.{hpp,cpp}` (anchor node of the pole), `LevelSession::placeZiplinePoles` in `src/game_zipline.cpp`; CTests `zipline-pole` (hand values) and `zipline-pole-real-data` (24 lvl_01 poles lie under their anchors).
