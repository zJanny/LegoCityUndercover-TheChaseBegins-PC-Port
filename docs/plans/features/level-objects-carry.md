# Level objects: carrying

## 1. Scope

Carry objects (`props/prop_carry`, object type 122) and carry targets (`props/prop_carrytargetnomesh`, type 123): the pickup with the action button, the carried state, the drop, the placing into a target and the script objects these fire. Mission 1-1 steps 9 and 10 (`1-1_DonutsDEMO`, `1-1_DonutTarget_DEMO`) are the first user.

## 2. Status

IN_PROGRESS, updated 2026-10-15T00:20:00+02:00. `chase-game` picks up `1-1_DonutsDEMO` with A inside its use zone, lets it float in front of the player, drops it with A outside a matching target zone, and places it with A inside `1-1_DonutTarget_DEMO`'s zone: `1-1_SO_Donuts_PickedUp`, `1-1_Deliver_Donuts` and `1-1_SO_MissionEnd` start as in the original's order (the target effect one update after the release). Animations, the hand attachment and the locomotion effects of carrying are stand-ins.

## 3. Verified behaviour

- Class values of the donut box and its target (references, `ATTR_SmallObject` 0, `ResetTime` 5 = 150 updates, target `UseBound`, `ATTR_SFX_DROP` 72); CTest `carry-real-data`.
- Use zone of the box: model bounds widened by (1, 1.2, 1), half (1.761, 1.606, 1.383) as read in `m11-8-donuts-in-hand`; axis-aligned, no yaw; the positions where the oracle's A presses worked are inside the zones, `m11-9-delivery-goal` is outside the target's zone; CTest `carry-real-data`.
- Press logic, timers (place: release 8 updates after the press, target effect 1 later; drop release 4), reset after 150 updates; CTest `carry`.
- Mission 1-1 steps 9 and 10 run end to end on the real level data; CTest `carry-real-data`.

## 4. Stand-ins

- LEVOBJ-S30 The durations of the pickup (attach at update 8, end at 17) are copies of the measured place timing; the drop end (17) is a copy as well. The original ends them on animation events. needs: ANIM-5, LEVOBJ-30.
- LEVOBJ-S31 The player does not turn towards the object or the target (the original sets the heading in the message 3 handlers) and stands still while a carry state runs (`CarrySet::busy` suppresses the stick).
- LEVOBJ-S32 A carried object floats 0.5 m in front of the player, 0.9 m up; the original hangs its `carryhandle` node on the player's hand node. A released box is put at the target position (place) or at the player (drop); the original lets it fall to the ground from the hand. needs: ANIM-5.
- LEVOBJ-S33 Character ability 6 (can lift) is taken as present, ability 7 (strength) as missing, so `ATTR_StrengthOnly` objects cannot be picked up.
- LEVOBJ-S34 The ray test of the pickup message (0x22f134, 3 m) and the interaction-flag gates of the action (ctrl+0x314 bit 6, ctrl+0x316) are not applied; the first matching zone in file order wins.

## 5. Open items

- [ ] LEVOBJ-30 — Measure the pickup in the oracle (update of the attachment, end of state 0xa7, release of a drop from a second point) once the step-9 waypoint can be reached in a scripted walk; replace LEVOBJ-S30. evidence: [carrying.md §2026-10-15T00:17](../../re/carrying.md#ts-20261015-001729).
- [ ] LEVOBJ-31 — Locomotion while carrying: walking speed and what the player cannot do (readers of the carry test 0x225754 in 0x203e14, 0x2061f8, 0x206540, 0x20bb7c, 0x219644; the vehicle search already refuses). evidence: [carrying.md §2026-10-15T00:17](../../re/carrying.md#ts-20261015-001729).
- [ ] LEVOBJ-32 — Effects of the carry system not played: sounds (`carryit_pickup_big_01`, `carryit_putdown_big_01`, `ATTR_SFX_DROP`, `ATTR_SFXReset`, `ATTR_SFXComplete`), the target's `SpawnDebris`, `DropParticle01/02`, `FadeAfterUse`, the blinking and reset effect of a dropped object, the HUD prompt "PICKUP CARRY IT" / "DROP CARRY IT". evidence: [carrying.md §2026-10-15T00:17](../../re/carrying.md#ts-20261015-001729).
- [ ] LEVOBJ-33 — Other carry classes and parameters: `TargetObject01..04`, `TargetBound01..04`, `TriggerOnRespawn`, `CarryTargetType` 1 and 2, `PlaceObject`, `ATTR_CarryType`, the variants in other levels (`prop_carry_walkie_talkie`, `prop_albatross_*`). evidence: [carrying.md §2026-10-15T00:17](../../re/carrying.md#ts-20261015-001729).
- [ ] LEVOBJ-34 — The carried and dropped state is not saved (save record type 122). needs: SAVE-8. evidence: [carrying.md §2026-10-15T00:17](../../re/carrying.md#ts-20261015-001729).

## 6. Findings summary

- Classes, records, zones, pickup, drop, placing, fired events and unknowns: [carrying.md §2026-10-15T00:17](../../re/carrying.md#ts-20261015-001729).

## 7. Code and tests

- `src/runtime/carry.*` (zones, press logic, timers, actions), `ScriptRunner::fireObjectTriggered`, `assets::modelBounds`; glue `updateCarry` in `src/game_level_update.cpp`, `carry.load` and the object position source in `src/game_level.cpp`.
- CTest: `carry`, `carry-real-data`.
