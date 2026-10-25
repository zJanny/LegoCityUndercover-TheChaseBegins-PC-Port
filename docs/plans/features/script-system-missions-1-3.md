# Script system: missions, mission 1-3 at the garage

## 1. Scope

The garage part of mission 1-3 in `chase-game`: the key the detective dirt mound reveals, its pickup, the key use at the keyhole and the garage door, the parts crate carried to the base plate, the generator and the mission end. Split from [script-system-missions.md](script-system-missions.md); the roof, the super brick and the helipad of mission 1-3 are in [studs-super-bricks.md](studs-super-bricks.md) and [level-objects-buildit.md](level-objects-buildit.md), the carry classes in [level-objects-carry.md](level-objects-carry.md).

## 2. Status

IN_PROGRESS, updated 2026-10-25T16:43:34+01:00. The key `1-3_Garage_Key` can be picked up: it is a carry object whose zone comes from its class mesh, the green arrow above it is the carry arrow every free carry object has, and after the pickup the arrow moves to the keyhole, as in the oracle (CTest `game-key-pickup-1-3`). Where the user's save state stands (1.71 from the key) A does nothing in the original either. Placing the key starts `1-3_Garage_Key_Base_Script`, which enables the key switch; A on the switch fires `1-3_Garage_Key_used` through the switch base and the garage door opens (CTests `game-garage-key-1-3`, `game-key-use-1-3`); the switch's states follow the oracle's update counts, the player's key-turn animation is a stand-in (SCRIPT-367). The parts crate runs from its break to the build of `1-3_Pull_Switch_Bits` (the crate breaks in two hits, `1-3_Got_parts` enables the big carry object, A picks it up, A at the base plate places it and the build-it, hidden until then, builds; CTest `game-parts-crate-1-3`). The generator is not run yet (SCRIPT-364). Its natives are ported: WaitForPlatform with the platform's travel (the crane platform's wait lasts 103 updates, as in the oracle), SetPropRocking with the girder's rocking, and AISuperJump; started by name the script runs past the jump (CTest `prop-rocking-real-data`).

## 3. Verified behaviour

- The key's use zone, the keyhole's zone and references, the carry arrow on the key before the pickup and on the keyhole after it, and the key in the hand match the oracle (RPC 20261025-recorded-C-1); CTest `carry-real-data`, `carry-arrows`, `game-key-pickup-1-3`. evidence: [carrying.md §2026-10-25T14:37:21](../../re/carrying.md#ts-20261025-143721).
- Placing the key only enables the key switch; the second A uses it and the shutter rises, with the oracle's update counts (RPC 20261025-garage-A-1); CTests `game-garage-key-1-3`, `game-key-use-1-3`. evidence: [switches.md §2026-10-25T16:34:17](../../re/switches.md#ts-20261025-163417).
- The parts crate chain: two hits break `1-3_Lift_Crate_01`, `1-3_Got_parts` enables `1-3_Generator_Parts_Carry`, placing it into `1-3_BasePlate` fires `1-3_Hidden_Exhaust_Carry_Target_Script` and enables the PlacedObject build-it, which is hidden until then (post-link 0x18e0f8); CTest `game-parts-crate-1-3`, `level-start-real-data`. Read from the code, not yet measured in the oracle (SCRIPT-368). evidence: [carrying.md §2026-10-25T16:20:07](../../re/carrying.md#ts-20261025-162007).

## 4. Stand-ins

- SCRIPT-S296 The platform record (kind 64) moves by the code's step (dt x 30 x `speed` in state 2), but a trigger message requests state 2, the arrival sets 1.0 and state 5, and the node stays where it is placed as measured for the crane platform, not from the class code (other types, `ObjectTo`, the way back are not ported). needs: SCRIPT-44. uses: WaitForPlatform. evidence: [props-rocking.md §2026-10-25T16:46:40](../../re/props-rocking.md#ts-20261025-164640).
- SCRIPT-S297 Rocking: the clock 0x241098 advances 2 units per update (60 per second, the rate measured for the use markers) and starts at 0 with the level session (multiplier and start value not read, so the speed and phase of the sway are not checked); and only kind 67 objects whose `Rocking` has bit 0 set get a rocking record, while the create keeps one for more of them (rule 0x16a334..0x16a3a0 not read), so SetPropRocking(x, 1) on another kind 67 prop does nothing. uses: SetPropRocking. evidence: [props-rocking.md §2026-10-25T16:29:44](../../re/props-rocking.md#ts-20261025-162944).
- SCRIPT-S291 Placing a carried object into a target always plays the put-down clip; the target's `dropAnim` (role 710 for the keyhole of mission 1-3) is not played (the enter 0x2768bc reads it). evidence: [carrying.md §2026-10-25T14:37:21](../../re/carrying.md#ts-20261025-143721).

## 5. Open items

- [ ] SCRIPT-368 — Oracle check of the crate steps: the entity flag of `1-3_Pull_Switch_Bits` before and after the placing, and the update timings of the crate hits, the pickup and the placing; the port values are its own. blocks: SCRIPT-364. evidence: [carrying.md §2026-10-25T16:20:07](../../re/carrying.md#ts-20261025-162007).
- [ ] SCRIPT-364 — Generator and mission end: the build of `1-3_Pull_Switch_Bits`, the pull switch `1-3_Generator_Pull_Switch` (`props/prop_switchpull`, kind 58, `TriggerObject` `1-3_Generator_Script`) and the script up to the mission end; its natives WaitForPlatform, SetPropRocking and AISuperJump are ported (platform travel SCRIPT-44, the flight SCRIPT-103), the real path is not run. needs: SCRIPT-368, SCRIPT-366. evidence: [carrying.md §2026-10-25T14:37:21](../../re/carrying.md#ts-20261025-143721).
- [ ] SCRIPT-367 — The player's switch-use state (its animation, the turn to the switch, the writer of the switch's use bit) is not ported. blocks: SCRIPT-S292. evidence: [switches.md §2026-10-25T16:34:17](../../re/switches.md#ts-20261025-163417).

## 6. Findings summary

- The order of `1-3_Generator_Script`, SetPropRocking and the rocking 0x28a758, WaitForPlatform: [props-rocking.md §2026-10-25T16:29:44](../../re/props-rocking.md#ts-20261025-162944).
- The oracle trace of the generator scene (platform travel 103 updates, the girder record, the jump of the girder guy): [props-rocking.md §2026-10-25T16:46:40](../../re/props-rocking.md#ts-20261025-164640), [ai-states.md §2026-10-25T16:46:40](../../re/ai-states.md#ts-20261025-164640).
- The key, the keyhole, the oracle reads of the pickup, the carry arrows and the held-object matrix: [carrying.md §2026-10-25T14:37:21](../../re/carrying.md#ts-20261025-143721).
- The switch base, the lever and the key switch: [switches.md §2026-10-25T16:34:17](../../re/switches.md#ts-20261025-163417), with the oracle trace of placing the key and using the switch (also the keyhole's `dropAnim` and release time).
- The parts crate, the base plate and the PlacedObject hide: [carrying.md §2026-10-25T16:20:07](../../re/carrying.md#ts-20261025-162007).
- The user's report of the blocked key: [playtests.md §2026-10-25T13:56:15](../../re/playtests.md#ts-20261025-135615).

## 7. Code and tests

- `src/runtime/carry.*` (load by ObjectType, mesh of the model loader, `PlacedObject`, `CarrySet::useAt`), `src/runtime/carry_arrows.*` (carry arrow list and group), `ScriptRunner::setCarryGroupTarget`; glue `src/game_key_pickup.*` (log lines `carry NAME: picked up`, `carry arrow over NAME added / removed`).
- `src/runtime/prop_rocking.*`, `src/runtime/platform_wait.*`, glue `src/runtime/game_generator_natives_glue.cpp` and `src/game_prop_rocking.cpp` (log line `prop NAME: rocking on|off at update N`); CTests `prop-rocking`, `prop-rocking-real-data`.
- `src/runtime/switch_base.*`, `src/runtime/switch_set.*`, glue `src/game_switch_glue.cpp`; CTests `game-garage-key-1-3` (A on the switch after its enabling script) and `game-key-use-1-3` (slow; the real path through the pickup, the placing and the use).
- CTests `carry-arrows`, `carry-real-data` (the key and the keyhole), `carry-animation-real-data` (small pickup timing), `game-parts-crate-1-3` (slow; from the crate to the build of `1-3_Pull_Switch_Bits`), glue-free: the hide is in `src/runtime/level_start.cpp`, `level-start-real-data` pins it. `game-key-pickup-1-3` (slow; the real path from `1-3_ConstructionYard` through the detective reveal to the pickup).
