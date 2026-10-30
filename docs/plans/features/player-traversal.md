# Player traversal

## 1. Scope

The climbing and vaulting moves of the player: zipline and tightrope (object type 101), pole and drain pipe (type 187), climbing bricks (type 20), free climb wall (type 30), rails, climb bars and flag poles (type 29, corner type 218), ladders (kind 56), the fence vault (kind 189, `props/parkour_fencevault`) and the ledge states. Mission 1-2 needs the zipline, the drain pipe, the bricks and the rail before the dog cage (oracle save state under the zipline of mission 1-2 to the save state climbing along the railing of mission 1-2); mission 1-3 needs the fence vault in the lawn mower rescue. Walking, jumping and falling are in [player-movement.md](player-movement.md).

## 2. Status

IN_PROGRESS, updated 2026-10-30T15:04:16+01:00 (the fence vault is ported and measured, TRAV-64 and TRAV-65 closed: B near the mission 1-3 fence vaults over it). Ported in `chase-game` from the original: the fence vault (measured at the mission 1-3 fence), the zipline, the pole and drain pipe, the climbing bricks (grab, hang, reach, jump to the adjacent brick, drop, jump off), the rails and the ladders (subtopic files below). The player step chains them: rails around bricks around the pole around the usual step, so the jump off the last brick grabs the rail as in the original. The free climb wall (not on the mission route) is not ported.

## 3. Verified behaviour

- Fence vault at the mission 1-3 fence against the oracle (RPC 20261030-qolstores-F-1, dt 1/30): use point (1020.330, 32.849, 1019.643), heading 13143, lerp of 0.1 s (state 20 for u0..u3), state 326 from u4 with the root track from u5, peak 36.302 = fence top, end at u60 with fall speed 0.3619 (CTest `fence-vault-real-data`).
- Climbing bricks against the save state climbing the bricks of mission 1-2 (dt 1/30): grab of 08 in the 7th jump update at (908.9421, 65.6976, 673.1696), heading 22209, 22 updates of state 141; reach in the first stick update; the jump up to 07 height per update and the grab up after 16 updates, idle after 14; B on 07 starts state 4 (CTests `brick-grab-real-data`, `game-script-brick-grab`).

## 4. Stand-ins

- TRAV-S10 `kStandInPlayerGrabOffset`, `kStandInPlayerGrabHalf` — the player's brick grab box uses entity+0x60 and +0x6c as read in the save state climbing the bricks of mission 1-2; their source in the character data is not traced. evidence: [player-traversal-bricks.md §2026-10-18T15:57:46](../../re/player-traversal-bricks.md#ts-20261018-155746)
- TRAV-S11 — brick events: one state change per update in the order B, A, check, clip done, stick, and the released event at every state change; the original's order is not read. evidence: [player-traversal-bricks.md §2026-10-18T15:57:46](../../re/player-traversal-bricks.md#ts-20261018-155746)
- TRAV-S12 — the scaled brick jump (149..151) and state 152 move by the root track along the hanging axes without the collision of 0x20cd30; the x / z axis use is inferred. blocks: TRAV-19. evidence: [player-traversal-bricks.md §2026-10-18T15:57:46](../../re/player-traversal-bricks.md#ts-20261018-155746)
- TRAV-S38 `movePlayerWithVault`, `FenceVaulter` — the fence vault's gate is the port's jump press on the ground (the original's state and flag tests not read), only the fences are asked, the lerp and the vault move without collision (the lerp's dip of 0.05 at the press update is missing), the end's first fall step stands for the ground probe, the posture is Falling during the sequence and USE LERP shows no clip of its own; the state's second end condition and the clip frame's loop and clamp handling are not ported (the port sums dt x rate), and `FenceVaulter::stepLerp` repeats the lever's USE LERP step instead of sharing one helper. evidence: [player-traversal-fence-vault.md §2026-10-30T15:02:37](../../re/player-traversal-fence-vault.md#ts-20261030-150237)
- TRAV-S13 — the camera row 2 for the no-stick brick jump direction is built from the follow camera's yaw and pitch in `src/game_brick_grab.cpp`. evidence: [player-traversal-bricks.md §2026-10-18T15:57:46](../../re/player-traversal-bricks.md#ts-20261018-155746)

## 5. Open items

- [ ] TRAV-66 — The other kinds that share the fence's B press list are not identified or ported (their order decides who takes a B press first), and the fence's B prompt (`UseHintSystem`) is not drawn. uses: parkour_fencevault. evidence: [player-traversal-fence-vault.md §2026-10-30T14:14:36](../../re/player-traversal-fence-vault.md#ts-20261030-141436)
- [ ] TRAV-2 — Free climb wall (type 30, entered by its handler 0x1b5454 into state 99; states 99..110) not read or ported; the bricks are ported, the wall is not on the mission 1-2 route. blocks: PLAYER-7. evidence: [player-traversal-bricks.md §2026-10-18T15:57:46](../../re/player-traversal-bricks.md#ts-20261018-155746)
- [ ] TRAV-19 — Brick jumps to the side (150 / 151) and the axes of their scaled root motion are not measured (bricks 1-2_Brick_Grab_01..06, 09). needs: TRAV-S12. evidence: [player-traversal-bricks.md §2026-10-18T15:57:46](../../re/player-traversal-bricks.md#ts-20261018-155746)
- [ ] TRAV-20 — Brick state 152 (`IsTopBrick`) not measured; its ground probe 0x20b48c and the collision of 0x20cd30 in the brick jumps are not ported. evidence: [player-traversal-bricks.md §2026-10-18T15:57:46](../../re/player-traversal-bricks.md#ts-20261018-155746)
- [ ] TRAV-21 — Brick use handler 0x1b53a8 (messages 3 / 4 into state 145): sender unknown; `TriggerObject` switching (0x244f40 / 0x244f88) not ported. evidence: [player-traversal-bricks.md §2026-10-18T15:57:46](../../re/player-traversal-bricks.md#ts-20261018-155746)
- [ ] TRAV-22 — Brick checks not ported: the character test 0x227b5c for type 20, brick +0x8c bit 4 and entity+0xc bit 4 (the port checks only the disabled flag and carrying). evidence: [player-traversal-bricks.md §2026-10-18T15:57:46](../../re/player-traversal-bricks.md#ts-20261018-155746)
- [ ] TRAV-34 — The A-press mount of a pole (0x2011b0 -> 0x1fef10, ported) is not measured; the save state at the foot of the drain pipe of mission 1-2 / the save state beside the drain pipe of mission 1-2 do not fit (save state deferred). needs: oracle state facing a pipe in its use zone. evidence: [player-traversal.md §2026-10-18T20:38:04](../../re/player-traversal.md#ts-20261018-203804)
- [ ] TRAV-5 — The other grab tests of the jump update (0x206668 .. 0x21e038, 0x273f6c, 0x20614c) and of the fall-state chain 0x2068f4 -> 0x2010d4 (order read; a zipline or brick grabbed while falling) are not read or ported; only the bar test runs in a fall. evidence: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230)

## 6. Findings summary

- Subtopic files: [player-traversal-zipline.md](player-traversal-zipline.md) (zipline, tightrope, rope), [player-traversal-pole.md](player-traversal-pole.md) (pole and drain pipe), [player-traversal-rails.md](player-traversal-rails.md) (rails, climb bars, corner rail), [player-traversal-ladders.md](player-traversal-ladders.md) (ladders).
- The fence vault of mission 1-3 (class, B gate, approach, state 326) and its oracle run [player-traversal-fence-vault.md §2026-10-30T15:02:37](../../re/player-traversal-fence-vault.md#ts-20261030-150237): [player-traversal-fence-vault.md §2026-10-30T14:14:36](../../re/player-traversal-fence-vault.md#ts-20261030-141436).
- Climbing classes surveyed and the zipline: [player-traversal.md §2026-10-18T14:52:30](../../re/player-traversal.md#ts-20261018-145230).
- Climbing bricks, the save state climbing the bricks of mission 1-2 runs and the climb wall's entry: [player-traversal-bricks.md §2026-10-18T15:57:46](../../re/player-traversal-bricks.md#ts-20261018-155746).

## 7. Code and tests

- `src/runtime/fence_vault.{hpp,cpp}` (fences, zone, B press gate, use point, USE LERP and state 326), `src/game_fence_vault.cpp` (LevelSession::movePlayerWithVault between the pole and the push step, fenceVaultMovePlayer from the climb bar step), clips 286 / 287 loaded in `src/game_player.cpp`. CTests `fence-vault` (made-up fence), `fence-vault-real-data` (lvl_01 fences, `vault_fence`), `game-fence-vault-1-3` (chase-game from the recorded place in front of the mission 1-3 fence).
- `src/runtime/brick_grab.{hpp,cpp}` (bricks, grab test 0x273fe4, states 141..152), `src/game_brick_grab.cpp` (LevelSession::moveClimbingPlayer around movePlayer), brick clips loaded in `src/game_player.cpp`. CTest `brick-grab` (made-up bricks), `brick-grab-real-data` (lvl_01 bricks, the save state climbing the bricks of mission 1-2 runs `up` and `drop`), `game-script-brick-grab` (chase-game from the save state climbing the bricks of mission 1-2 landing point: grab, jump up, jump off).
