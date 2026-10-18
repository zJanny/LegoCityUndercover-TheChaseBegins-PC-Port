# Player traversal: rails, climb bars and the corner rail

## 1. Scope

Rails and climb bars (object type 29) and the corner rail (type 218) of [player-traversal.md](player-traversal.md): grab from a jump, arrival, hang, shuffle, the corner onto the next rail, leaving by A or B. Findings: [docs/re/player-traversal-rails.md](../../re/player-traversal-rails.md).

## 2. Status

IN_PROGRESS, updated 2026-10-18T16:15:36+02:00. Ported in `chase-game` for the route of mission 1-2 and replayed against `m12-12` (TRAV-3, TRAV-4 and TRAV-30 closed in [player-traversal.closed.md](player-traversal.closed.md)); the jump off the last climbing brick grabs the rail as in the original. The bar variants are not ported.

## 3. Verified behaviour

- Rails (m12-12 replays, CTest `climb-bar-real-data`): shuffle along 03 to its end (60 updates, clip root motion at half frames 0.09406 / 0.09375 per update, first update 0.065), left over the corner rail (spline samples, 15 updates, t0 = 0.2) onto 02 and along it, jump off 03 and grabbed again in update 21, arrival snap, A drop. Tolerance 3e-4.
- The bar box test uses the player box (0, 1.15, 0.08) +- (0.6, 0.425, 0.52) against `GrabBound`; the bar is grabbed in the 7th jump update as the zipline ([evidence](../../re/player-traversal-rails.md#ts-20261018-160503)).

## 4. Stand-ins

- TRAV-S15 `kClimbBarHangOffsetY/Z` — the hang offset (0, -1.0606986, 0.3636877) of Chase measured in m12-12; the original samples the hand joint of `ledge_idle` (0x27086c). evidence: [player-traversal-rails.md §2026-10-18T16:05:03](../../re/player-traversal-rails.md#ts-20261018-160503)
- TRAV-S16 — the bar state comp+2 (1..3) and the ability check 0x227b5c are taken as always passed; the bar list is in level order; the grab sound ATTR_SFX_GRAB plays at the player with owner 0 when set. evidence: [player-traversal-rails.md §2026-10-18T16:05:03](../../re/player-traversal-rails.md#ts-20261018-160503)
- TRAV-S17 — the order of state changes inside one update is the measured sequence per case (hang request next update, corner exit arrival in the same update, jump grab arrival next update, idle placement update without input), not the state machine 0x2eafc8. evidence: [player-traversal-rails.md §2026-10-18T16:05:03](../../re/player-traversal-rails.md#ts-20261018-160503)
- TRAV-S18 — simplifications: bar yaw only (no pitch or roll), the stick direction is the world dot (camera transform 0x229a40 and pitch ignored), the hang move has no world collision (0x215804), blocked means a horizontal delta under 1e-5, the root motion loop wraps continuously. evidence: [player-traversal-rails.md §2026-10-18T16:05:03](../../re/player-traversal-rails.md#ts-20261018-160503)
- TRAV-S19 — a bar of the `Ledge`, `NarrowBar`, `AcrobatPole` or crumbling kind behaves as a normal bar. evidence: [player-traversal-rails.md §2026-10-18T16:05:03](../../re/player-traversal-rails.md#ts-20261018-160503)

## 5. Open items

- [ ] TRAV-27 — Narrow bars (`NarrowBar`, clip 0x2a, no sideways move), acrobat poles (`AcrobatPole`, ability 9, clip 0x2c), crumbling bars (state 0x5c `HOT`, update 0x2b7528) and the `Ledge` variant (jump off to state 24, state 23) are not ported. needs: TRAV-S19. evidence: [player-traversal-rails.md §2026-10-18T16:05:03](../../re/player-traversal-rails.md#ts-20261018-160503)
- [ ] TRAV-28 — The hang offset comes from the hand joint of the ledge clip (0x27086c), not from a constant. needs: TRAV-S15. evidence: [player-traversal-rails.md §2026-10-18T16:05:03](../../re/player-traversal-rails.md#ts-20261018-160503)
- [ ] TRAV-29 — The fall-state grab path for bars (state 5 is accepted by 0x227b5c), the exact time ctrl+0x138 is set and the bar state comp+2 are not read. needs: TRAV-S16. evidence: [player-traversal-rails.md §2026-10-18T16:05:03](../../re/player-traversal-rails.md#ts-20261018-160503)
- [ ] TRAV-31 — The corner in the other direction (state 0x17a, t falling) and the hang move beyond the end without a corner are not measured. evidence: [player-traversal-rails.md §2026-10-18T16:05:03](../../re/player-traversal-rails.md#ts-20261018-160503)
- [ ] TRAV-32 — `PrimaryNextBar` chains and the secondary bars are read by the AI follower (0x2c7108) only as far as seen; whether the player uses them is not found. evidence: [player-traversal-rails.md §2026-10-18T16:05:03](../../re/player-traversal-rails.md#ts-20261018-160503)
- [ ] TRAV-33 — The hang move is applied without the world collision of 0x215804, and the stick direction ignores the camera transform. needs: TRAV-S18. evidence: [player-traversal-rails.md §2026-10-18T16:05:03](../../re/player-traversal-rails.md#ts-20261018-160503)

## 6. Findings summary

- Rails, climb bars, corner rail, ledge states: [player-traversal-rails.md §2026-10-18T16:05:03](../../re/player-traversal-rails.md#ts-20261018-160503).

## 7. Code and tests

- `src/runtime/climb_bar.{hpp,cpp}` (bars, corner rails, box test, spline, hang update, corner update), `src/game_climb_bar.cpp` (`LevelSession::climbBarMovePlayer` wraps `movePlayer`: while a bar state owns the player it replaces the locomotion, otherwise the grab test follows the jump update), clips `ledge_idle` / `ledge_left` / `ledge_right` loaded in `src/game_player.cpp`. CTest `climb-bar` (made-up bar), `climb-bar-real-data` (lvl_01 bars and the m12-12 replays).
