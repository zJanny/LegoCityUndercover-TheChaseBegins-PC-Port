# Script system: missions, the end of mission 1-3

## 1. Scope

The way from the end of the generator scene to the end of mission 1-3 in `chase-game`: `1-3_Start_MowerMan_rescue`, the cart crate, the push cart build-it and the push cart (kind 71, its handles and the player's handle states), `1-3_Push_Done`, the crane build-it `1-3_Crane_bits`, the crane switch and the mission end script `1-3_Start_Rescue_Lawnmower_man`, and the ledge platform on the other way up. Split from [script-system-missions-1-3.md](script-system-missions-1-3.md) (the garage, the parts crate and the generator scene); build-its in general are in [level-objects-buildit.md](level-objects-buildit.md).

## 2. Status

IN_PROGRESS, updated 2026-10-28T01:21:46+01:00. From the start of `1-3_Start_MowerMan_rescue` the port runs the rest of mission 1-3: five punches break `1-3_Cart_crate`, A builds `1-3_Push_Cart_bits` (a `leprops/leprop_buildit`, kind 21, now loaded with the other build-its), the player walking into the push cart from behind grabs its back handle and pushes it with the stick to `1-3_Push_cart_to`, which starts `1-3_Push_Done` (CTest `game-push-cart-1-3`); that script enables `1-3_Crane_bits`, A builds it, the crane switch takes A and `1-3_Start_Rescue_Lawnmower_man` sets `StoryProgress` 4 and spawns `1-4_MissionStart` (CTest `game-crane-1-3`). The ledge platform `1-3_Ledge_Platform` lifts the player from its plate to the top (CTest `game-ledge-lift-1-3`); B vaults over the fence on the way to the cart (`1-3_Parkour_fence_climb`, CTest `game-fence-vault-1-3`, [player-traversal.md](player-traversal.md)); the way from the lift over the fence to the cart is not run in one test (the push cart test starts on the track). Not yet: the platform `1-3_Push_Platform` that takes over the cart, `SpawnDebris2Colour`, and the open parts of the oracle comparison (SCRIPT-391 to SCRIPT-393).

## 3. Verified behaviour

- The push cart's parameters, its handles (only the back one active) and the push along its z axis to `1-3_Push_cart_to` within `TriggerDistance` 0.1 follow the class code read with REA. Measured in the oracle: from the cart's measured start 72 moves of 0.06, state 1 and 2 in consecutive updates, rest position (1019.4844, 33.4108, 1034.3561), the first move 8 updates after state 85 (one update after message 9); the blend step 5 x dt; CTests `pushable`, `pushable-real-data`. evidence: [pushables.md §2026-10-28T01:21:46](../../re/pushables.md#ts-20261028-012146).
- The end of `1-3_Start_Rescue_Lawnmower_man` equals the oracle in time: the switch states 20, 76, 1 (8 and 49 updates), the bridge jump 6.0 s after the script start, the jump's horizontal end (987.288, 1077.788), the salute 380 updates after the script start and 61 long; the player falls after the jump and lands at y 16.80; CTest `game-crane-1-3`. evidence: [pushables.md §2026-10-28T01:21:46](../../re/pushables.md#ts-20261028-012146).
- Build-its are kind 21 objects, `leprops/leprop_buildit` included (18 in lvl_01; `1-3_Crane_bits`, `1-3_Push_Cart_bits`); CTests `buildit-real-data`, `stud-callers-real-data`. evidence: [buildit.md §2026-10-26T23:06:15](../../re/buildit.md#ts-20261026-230615).

## 4. Stand-ins

- SCRIPT-S303 The pushable: the handle distance is the bounds face (the handle ray is not cast), the move is never cut by collision, the ground ray is taken as finding ground, pushables without `NoGravity` get neither the lift nor the fall, the landing and destroy messages and the room move are not ported, and the destination is its level placement. evidence: [pushables.md §2026-10-26T23:04:40](../../re/pushables.md#ts-20261026-230440).
- SCRIPT-S304 The player's handle states: the touch is the overlap of the player's box with the pushable's bounds box, the character's use test is taken as passed, record +0xc bit 2 is read as the action button, the grab is tested after the walk step, the height is left to itself, the enter functions' resets and the A-press grab of handles without `AutoPush` are not ported. evidence: [pushables.md §2026-10-26T23:04:40](../../re/pushables.md#ts-20261026-230440).
- SCRIPT-S311 The fall after the bridge jump: the fall speed and distance at the end of the flight are taken from the port's own descent. evidence: [pushables.md §2026-10-28T01:21:46](../../re/pushables.md#ts-20261028-012146).
- SCRIPT-S312 The player's height during an `AISuperJump` flight follows the `jump_run` clip scaled to the climb, not the original's curve (port 29.52 at the flight's end, original 25.71). blocks: SCRIPT-391. evidence: [pushables.md §2026-10-28T01:21:46](../../re/pushables.md#ts-20261028-012146).

## 5. Open items

- [ ] SCRIPT-390 — The pull switch on the lift at the top: its use point (USE LERP) comes from the placement, so a use at the top would walk the player to the bottom position; the ride down and the ground switch `1-3_Ledge_Platform_Switch_Ground` are not tested. uses: SetPlatformOrigin, SetPlatformTarget. evidence: [props-rocking-platforms.md §2026-10-28T00:09:40](../../re/props-rocking-platforms.md#ts-20261028-000940).
- [ ] SCRIPT-391 — The original's height curve of locomotion state 73 (the oracle rows are in the entry) and the fall after 28 counts; the landing clip's move of (-0.879, -0.526) after the hard landing is not applied. needs: SCRIPT-S312. evidence: [pushables.md §2026-10-28T01:21:46](../../re/pushables.md#ts-20261028-012146).
- [ ] SCRIPT-392 — The camera shots of the end (`StartDCam` 1-3_Crane_Cam_01/02, the zoom cam) are not compared with the three measured shots (eye and look at vblank 0, 258, 448). evidence: [pushables.md §2026-10-28T01:21:46](../../re/pushables.md#ts-20261028-012146).
- [ ] SCRIPT-393 — The state in front of the cart has the cart 18 moves ahead of its level placement; whether the build-it places it there or a push came first is not known. evidence: [pushables.md §2026-10-28T01:21:46](../../re/pushables.md#ts-20261028-012146).

## 6. Findings summary

- The pushable class, the handles, the player's handle states and the push cart of mission 1-3: [pushables.md §2026-10-26T23:04:40](../../re/pushables.md#ts-20261026-230440).
- Build-its of class `leprops/leprop_buildit` are kind 21: [buildit.md §2026-10-26T23:06:15](../../re/buildit.md#ts-20261026-230615).
- The ledge lift (the scripts, `Lift_Up`, the ride): [props-rocking-platforms.md §2026-10-28T00:09:40](../../re/props-rocking-platforms.md#ts-20261028-000940).
- The push cart and the end sequence against the oracle: [pushables.md §2026-10-28T01:21:46](../../re/pushables.md#ts-20261028-012146).
- The chain to the end of mission 1-3 in the level data: [props-rocking.md §2026-10-26T13:39:27](../../re/props-rocking.md#ts-20261026-133927).

## 7. Code and tests

- `src/runtime/pushable.*` (`PushableSet`, `pushGrabTest`, `pushGripUpdate`), glue `src/runtime/game_push_cart_glue.cpp` (log lines `pushable NAME: grabbed at HANDLE, player state 84`, `player state N`, `state 1`, `state 2`), `BuilditSet::load` by kind 21; CTests `pushable`, `pushable-real-data`, `game-push-cart-1-3` (slow), `game-crane-1-3` (slow; from a called `1-3_Push_Done` to `1-4_MissionStart`), `game-ledge-lift-1-3` (slow; from the lift's plate to the top); `src/runtime/mission13_end.*` (`AiMoveWatch`, the log lines `player AI move: ...`, and `AiSuperJumpVertical`), CTest `mission13-end`.
