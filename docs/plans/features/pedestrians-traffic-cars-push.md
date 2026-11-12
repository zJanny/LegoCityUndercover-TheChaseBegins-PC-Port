# Traffic car push: push vector, squash and character events

## 1. Scope

The world of a traffic car's push of a character: the push vector and shape test 0x1f49d8 with the shape tests it calls (the world slide 0x23d434, the mesh test 0x287ac4, the oriented test 0x27f174), the squash 0x233594 and the listeners of the events 17 and 18 that message 45 sends to the characters' states. The push 0x2baef0 itself, message 45 of the characters and the cop whistle are in [pedestrians-traffic-cars-driver.md](pedestrians-traffic-cars-driver.md).

## 2. Status

IN_PROGRESS, updated 2026-11-12T11:15:16+01:00. `trafficPushVector` ports 0x1f49d8 and serves as the hook `pushVector`; `connectTrafficPushShapes` fills its shape hooks with the world slide, the mesh test and the oriented test over `CollisionWorld`, and they change the caller's move in place. The squash and the listeners of the states 0x99 and 0x199 are ported, and `deliverCarPush` / `applyCarPushToCharacter` connect message 45 to a character's hit handler `receiveHit`. `TrafficPushSupply` supplies the world over the grid's characters and the traffic cars in `chase-game`: a traffic car (owner flags 0x1300, so a pusher with no slide) that touches a character sends message 45, the hit of a moving car in front goes to a hit target per pedestrian and is reported in the log; what a pedestrian or the player does with the hit is not ported (TRAFFIC-S47).

## 3. Verified behaviour

- Read from the code (test `traffic-push-world`), from [traffic-car-push.md §2026-11-12T08:17:17](../../re/traffic-car-push.md#ts-20261112-081717):
  - 0x1f49d8: the open box test of the moved mover box in the pushed box's frame, the world box of a tilted entry (|row 1 y| < 0.99), the slide clear for two entities with 0x100, and with slide the half push of each side (the pushed one by its slide 88, the mover back by its share plus the rest, slide 92);
  - the squash record {damage 32000, kind 0} to the pushing character;
  - only the states 0x99 (super knockback) and 0x199 (throw finisher) take the events 17 and 18; the order of 0x283d0c: event 17, then event 18 or the hit.
- Read from the code (test `traffic-push-shapes`), from [traffic-car-push.md §2026-11-12T10:09:38](../../re/traffic-car-push.md#ts-20261112-100938):
  - 0x23d434 / 0x208d84: the unrotated box at the move, the filter flags of 0x209a98 (repeated records, normal y, facing, side test), the slide of 0x2092d8 with and without the flattened normal, the corner pass of mode 0x10; box entries skip the filter and count once;
  - 0x287ac4: the mover's world box against one mesh entry with 93 / 3, and the pure turn call; 0x27f174: the open test, the push out from inside (smallest depth, no mostly vertical push) or outside (0.007 margin), the slide 92 with mode 1 or 3, and the part loop of an owner with 0x800;
  - the excluded-kinds list and the super knockback's damage word have no writer in the image as read.

## 4. Stand-ins

- TRAFFIC-S41 The listeners' unread parts: the throw finisher's tests of the other entity are inputs and its combat hit is an outcome only, and the squash record's byte +0x15 is not carried. needs: TRAFFIC-55. evidence: [traffic-car-push.md §2026-11-12T10:09:38](../../re/traffic-car-push.md#ts-20261112-100938)
- TRAFFIC-S47 The supply of the push: the traffic cars' owner flags and kind are measured on the traffic van only; a pedestrian's hit target is the class default (kind 197, 4 health) and the outcome of its hit is only logged, the player's hit is not applied at all, and no pedestrian state listens to the events 17 and 18. needs: TRAFFIC-66, TRAFFIC-67. evidence: [traffic-car-push.md §2026-11-12T10:09:38](../../re/traffic-car-push.md#ts-20261112-100938)

## 5. Open items

- [ ] TRAFFIC-55 — Read the throw finisher's tests and combat hit, the reader of record byte +0x15 in the callees of the character's hit path, and what enters the states 0x99 and 0x199. blocks: TRAFFIC-S41. evidence: [traffic-car-push.md §2026-11-12T10:09:38](../../re/traffic-car-push.md#ts-20261112-100938)
- [ ] TRAFFIC-66 — Read what a pedestrian and the player do with the hit of message 45 (the pedestrians' states and their listeners of the events 17 and 18, the player's hit) and apply it. blocks: TRAFFIC-S47. evidence: [traffic-character-gather.md §2026-11-12T11:13:41](../../re/traffic-character-gather.md#ts-20261112-111341)
- [ ] TRAFFIC-67 — Read the pair test (a car's ignore list), the pushed character's word +0xc bit 3, the entity kind and flags of the traffic classes other than the van, and which port data stand for the rooms' item lists and node bounds. blocks: TRAFFIC-S46, TRAFFIC-S47. evidence: [traffic-character-gather.md §2026-11-12T11:13:41](../../re/traffic-character-gather.md#ts-20261112-111341)

## 6. Findings summary

- [traffic-car-push.md §2026-11-12T08:17:17](../../re/traffic-car-push.md#ts-20261112-081717): the push vector 0x1f49d8, the squash 0x233594, the listeners of the events 17 and 18 (0x3305fc, 0x334888 / 0x1e3040) and their states 0x99 and 0x199.
- [traffic-character-gather.md §2026-11-12T11:13:41](../../re/traffic-character-gather.md#ts-20261112-111341): the registration pass 0x30e0d8 and the supply of the push world in `chase-game`.
- [traffic-car-push.md §2026-11-12T10:09:38](../../re/traffic-car-push.md#ts-20261112-100938): the world slide 0x23d434 (0x208d84, 0x209a98, 0x2092d8), the mesh test 0x287ac4, the oriented test 0x27f174, the turn branch, and the missing writers of the excluded kinds and the damage word.

## 7. Code and tests

`src/runtime/traffic_push_supply.{hpp,cpp}` (no SDL), CTest `traffic-push-supply` (`tests/traffic_push_supply_test.cpp`); `src/runtime/traffic_push_world.{hpp,cpp}` (no SDL); CTest `traffic-push-world` (`tests/traffic_push_world_test.cpp`). `src/runtime/traffic_push_shapes.{hpp,cpp}` (no SDL); CTest `traffic-push-shapes` (`tests/traffic_push_shapes_test.cpp`).
