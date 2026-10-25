# Script system: missions, mission 1-3 at the garage

## 1. Scope

The garage part of mission 1-3 in `chase-game`: the key the detective dirt mound reveals, its pickup, the key use at the keyhole and the garage door, the parts crate carried to the base plate, the generator and the mission end. Split from [script-system-missions.md](script-system-missions.md); the roof, the super brick and the helipad of mission 1-3 are in [studs-super-bricks.md](studs-super-bricks.md) and [level-objects-buildit.md](level-objects-buildit.md), the carry classes in [level-objects-carry.md](level-objects-carry.md).

## 2. Status

IN_PROGRESS, updated 2026-10-25T16:20:22+01:00. The key `1-3_Garage_Key` can be picked up: it is a carry object whose zone comes from its class mesh, the green arrow above it is the carry arrow every free carry object has, and after the pickup the arrow moves to the keyhole, as in the oracle (CTest `game-key-pickup-1-3`). Where the user's save state stands (1.71 from the key) A does nothing in the original either. Placing the key into the keyhole and the generator are not run yet (SCRIPT-362, SCRIPT-364). The parts crate runs from its break to the build of `1-3_Pull_Switch_Bits` (started at the crate, not through the key: the crate breaks in two hits, `1-3_Got_parts` enables the big carry object, A picks it up, A at the base plate places it and the build-it, hidden until then, builds; CTest `game-parts-crate-1-3`).

## 3. Verified behaviour

- The key's use zone, the keyhole's zone and references, the carry arrow on the key before the pickup and on the keyhole after it, and the key in the hand match the oracle (RPC 20261025-recorded-C-1); CTest `carry-real-data`, `carry-arrows`, `game-key-pickup-1-3`. evidence: [carrying.md §2026-10-25T14:37:21](../../re/carrying.md#ts-20261025-143721).

- The parts crate chain: two hits break `1-3_Lift_Crate_01`, `1-3_Got_parts` enables `1-3_Generator_Parts_Carry`, placing it into `1-3_BasePlate` fires `1-3_Hidden_Exhaust_Carry_Target_Script` and enables the PlacedObject build-it, which is hidden until then (post-link 0x18e0f8); CTest `game-parts-crate-1-3`, `level-start-real-data`. Read from the code, not yet measured in the oracle (SCRIPT-368). evidence: [carrying.md §2026-10-25T16:20:07](../../re/carrying.md#ts-20261025-162007).

## 4. Stand-ins

- SCRIPT-S291 Placing a carried object into a target always plays the put-down clip; the target's `dropAnim` (role 710 for the keyhole of mission 1-3) is not played (the enter 0x2768bc reads it). needs: SCRIPT-362. evidence: [carrying.md §2026-10-25T14:37:21](../../re/carrying.md#ts-20261025-143721).

## 5. Open items

- [ ] SCRIPT-362 — Key use: placing the key fires `1-3_Garage_Key_Base_Script`, which enables `1-3_Garage_Key_Switch` (`props/prop_switchkey_vertical`, kind 58); the switch class is not ported, so its `TriggerObject` `1-3_Garage_Key_used` (garage door open, `1-3_Garage_Dummy` to the go-to group) never fires; not traced in the oracle. needs: SCRIPT-358. blocks: SCRIPT-363, SCRIPT-S291. evidence: [carrying.md §2026-10-25T14:37:21](../../re/carrying.md#ts-20261025-143721).
- [ ] SCRIPT-368 — Oracle check of the crate steps: the entity flag of `1-3_Pull_Switch_Bits` before and after the placing, and the update timings of the crate hits, the pickup and the placing; the port values are its own. blocks: SCRIPT-364. evidence: [carrying.md §2026-10-25T16:20:07](../../re/carrying.md#ts-20261025-162007).
- [ ] SCRIPT-364 — Generator and mission end: the build of `1-3_Pull_Switch_Bits`, the pull switch `1-3_Generator_Pull_Switch` (`props/prop_switchpull`, kind 58, `TriggerObject` `1-3_Generator_Script`) and the natives of that script (WaitForPlatform, SetPropRocking, AISuperJump, ...) up to the mission end; not run. needs: SCRIPT-363, SCRIPT-358. evidence: [carrying.md §2026-10-25T14:37:21](../../re/carrying.md#ts-20261025-143721).

## 6. Findings summary

- The key, the keyhole, the oracle reads of the pickup, the carry arrows and the held-object matrix: [carrying.md §2026-10-25T14:37:21](../../re/carrying.md#ts-20261025-143721).
- The parts crate, the base plate and the PlacedObject hide: [carrying.md §2026-10-25T16:20:07](../../re/carrying.md#ts-20261025-162007).
- The user's report of the blocked key: [playtests.md §2026-10-25T13:56:15](../../re/playtests.md#ts-20261025-135615).

## 7. Code and tests

- `src/runtime/carry.*` (load by ObjectType, mesh of the model loader, `PlacedObject`, `CarrySet::useAt`), `src/runtime/carry_arrows.*` (carry arrow list and group), `ScriptRunner::setCarryGroupTarget`; glue `src/game_key_pickup.*` (log lines `carry NAME: picked up`, `carry arrow over NAME added / removed`).
- CTests `carry-arrows`, `carry-real-data` (the key and the keyhole), `carry-animation-real-data` (small pickup timing), `game-parts-crate-1-3` (slow; from the crate to the build of `1-3_Pull_Switch_Bits`), glue-free: the hide is in `src/runtime/level_start.cpp`, `level-start-real-data` pins it. `game-key-pickup-1-3` (slow; the real path from `1-3_ConstructionYard` through the detective reveal to the pickup).
