# Script system: missions, the end of mission 1-3

## 1. Scope

The way from the end of the generator scene to the end of mission 1-3 in `chase-game`: `1-3_Start_MowerMan_rescue`, the cart crate, the push cart build-it and the push cart (kind 71, its handles and the player's handle states), `1-3_Push_Done`, the crane build-it `1-3_Crane_bits`, the crane switch and the mission end script `1-3_Start_Rescue_Lawnmower_man`, and the ledge platform on the other way up. Split from [script-system-missions-1-3.md](script-system-missions-1-3.md) (the garage, the parts crate and the generator scene); build-its in general are in [level-objects-buildit.md](level-objects-buildit.md).

## 2. Status

IN_PROGRESS, updated 2026-10-26T23:04:40+01:00. From the start of `1-3_Start_MowerMan_rescue` the port runs the rest of mission 1-3: five punches break `1-3_Cart_crate`, A builds `1-3_Push_Cart_bits` (a `leprops/leprop_buildit`, kind 21, now loaded with the other build-its), the player walking into the push cart from behind grabs its back handle and pushes it with the stick to `1-3_Push_cart_to`, which starts `1-3_Push_Done` (CTest `game-push-cart-1-3`); that script enables `1-3_Crane_bits`, A builds it, the crane switch takes A and `1-3_Start_Rescue_Lawnmower_man` sets `StoryProgress` 4 and spawns `1-4_MissionStart` (CTest `game-crane-1-3`). Not yet: the ledge platform lift (SCRIPT-372), the platform `1-3_Push_Platform` that takes over the cart, `SpawnDebris2Colour`, and an oracle comparison of the push and of the end sequence (SCRIPT-382).

## 3. Verified behaviour

- The push cart's parameters, its handles (only the back one active) and the push along its z axis to `1-3_Push_cart_to` within `TriggerDistance` 0.1 (90 moves of 0.06 in the port's own count) follow the class code read with REA; not measured in the oracle; CTests `pushable`, `pushable-real-data`. evidence: [pushables.md §2026-10-26T23:04:40](../../re/pushables.md#ts-20261026-230440).
- Build-its are kind 21 objects, `leprops/leprop_buildit` included (18 in lvl_01; `1-3_Crane_bits`, `1-3_Push_Cart_bits`); CTests `buildit-real-data`, `stud-callers-real-data`. evidence: [buildit.md §2026-10-26T23:06:15](../../re/buildit.md#ts-20261026-230615).

## 4. Stand-ins

- SCRIPT-S303 The pushable: the handle distance is the bounds face (the handle ray is not cast), the move is never cut by collision, the ground ray is taken as finding ground, pushables without `NoGravity` get neither the lift nor the fall, the landing and destroy messages and the room move are not ported, and the destination is its level placement. evidence: [pushables.md §2026-10-26T23:04:40](../../re/pushables.md#ts-20261026-230440).
- SCRIPT-S304 The player's handle states: the touch is the overlap of the player's box with the pushable's bounds box, the character's use test is taken as passed, record +0xc bit 2 is read as the action button, the grab is tested after the walk step, the height is left to itself, the enter functions' resets and the A-press grab of handles without `AutoPush` are not ported. evidence: [pushables.md §2026-10-26T23:04:40](../../re/pushables.md#ts-20261026-230440).

## 5. Open items

- [ ] SCRIPT-372 — The ledge platform `1-3_Ledge_Platform` (kind 64) cannot lift the player to the cart deck: its node rises when triggered, but SetPlatformSpeed / SetPlatformOrigin / SetPlatformTarget are not handled and the player does not ride on it; the CTest places the player on the track behind the cart instead; save states on the lift trigger and near its top exist. needs: SCRIPT-37, SCRIPT-378. uses: SetPlatformSpeed, SetPlatformTarget. evidence: [props-rocking.md §2026-10-26T13:39:27](../../re/props-rocking.md#ts-20261026-133927).
- [ ] SCRIPT-382 — Oracle check of the push cart (the grab, the 6 blend updates, the moves and the update of `1-3_Push_Done`) and of the end sequence of `1-3_Start_Rescue_Lawnmower_man` (the crane animation, the player's bridge jump, the camera shots); a save state in front of the built push cart, one with the cart crashed into the box and one at the crane trigger now exist. evidence: [pushables.md §2026-10-26T23:04:40](../../re/pushables.md#ts-20261026-230440).

## 6. Findings summary

- The pushable class, the handles, the player's handle states and the push cart of mission 1-3: [pushables.md §2026-10-26T23:04:40](../../re/pushables.md#ts-20261026-230440).
- Build-its of class `leprops/leprop_buildit` are kind 21: [buildit.md §2026-10-26T23:06:15](../../re/buildit.md#ts-20261026-230615).
- The chain to the end of mission 1-3 in the level data: [props-rocking.md §2026-10-26T13:39:27](../../re/props-rocking.md#ts-20261026-133927).

## 7. Code and tests

- `src/runtime/pushable.*` (`PushableSet`, `pushGrabTest`, `pushGripUpdate`), glue `src/runtime/game_push_cart_glue.cpp` (log lines `pushable NAME: grabbed at HANDLE, player state 84`, `player state N`, `state 1`, `state 2`), `BuilditSet::load` by kind 21; CTests `pushable`, `pushable-real-data`, `game-push-cart-1-3` (slow), `game-crane-1-3` (slow; from a called `1-3_Push_Done` to `1-4_MissionStart`).
