# Traffic car push: push vector, squash and character events

## 1. Scope

The world of a traffic car's push of a character: the push vector and shape test 0x1f49d8 with the shape tests it calls (the world slide 0x23d434, the mesh test 0x287ac4, the oriented test 0x27f174), the squash 0x233594 and the listeners of the events 17 and 18 that message 45 sends to the characters' states. The push 0x2baef0 itself, message 45 of the characters and the cop whistle are in [pedestrians-traffic-cars-driver.md](pedestrians-traffic-cars-driver.md).

## 2. Status

IN_PROGRESS, updated 2026-11-12T10:11:31+01:00. `trafficPushVector` ports 0x1f49d8 and serves as the hook `pushVector`; `connectTrafficPushShapes` fills its shape hooks with the world slide, the mesh test and the oriented test over `CollisionWorld`. The squash and the listeners of the states 0x99 and 0x199 are ported, and `deliverCarPush` / `applyCarPushToCharacter` connect message 45 to a character's hit handler `receiveHit`. Nothing in `chase-game` supplies the world, so no car pushes a character in play (TRAFFIC-56).

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
- TRAFFIC-S45 The mesh test and the oriented test run on a copy of the move: their slide and push of the caller's move do not reach the contact slide, as `pushVector` takes the move const. needs: TRAFFIC-64. evidence: [traffic-car-push.md §2026-11-12T10:09:38](../../re/traffic-car-push.md#ts-20261112-100938)

## 5. Open items

- [ ] TRAFFIC-55 — Read the throw finisher's tests and combat hit, the reader of record byte +0x15 in the callees of the character's hit path, and what enters the states 0x99 and 0x199. blocks: TRAFFIC-S41. evidence: [traffic-car-push.md §2026-11-12T10:09:38](../../re/traffic-car-push.md#ts-20261112-100938)
- [ ] TRAFFIC-56 — Supply `TrafficPushWorld`, `TrafficPushShapes` and `TrafficPushShapeWorld` in `chase-game` from its entities and collision, and apply `deliverCarPush` to the pushed characters. needs: TRAFFIC-53, TRAFFIC-S44. blocks: TRAFFIC-S34, TRAFFIC-63. evidence: [traffic-car-push.md §2026-11-12T10:09:38](../../re/traffic-car-push.md#ts-20261112-100938)
- [ ] TRAFFIC-64 — Let `TrafficPushWorld::pushVector` and the shape hooks change the move, so the mesh and oriented tests slide the contact slide's move as the push vector does. blocks: TRAFFIC-S45. evidence: [traffic-car-push.md §2026-11-12T10:09:38](../../re/traffic-car-push.md#ts-20261112-100938)

## 6. Findings summary

- [traffic-car-push.md §2026-11-12T08:17:17](../../re/traffic-car-push.md#ts-20261112-081717): the push vector 0x1f49d8, the squash 0x233594, the listeners of the events 17 and 18 (0x3305fc, 0x334888 / 0x1e3040) and their states 0x99 and 0x199.
- [traffic-car-push.md §2026-11-12T10:09:38](../../re/traffic-car-push.md#ts-20261112-100938): the world slide 0x23d434 (0x208d84, 0x209a98, 0x2092d8), the mesh test 0x287ac4, the oriented test 0x27f174, the turn branch, and the missing writers of the excluded kinds and the damage word.

## 7. Code and tests

`src/runtime/traffic_push_world.{hpp,cpp}` (no SDL); CTest `traffic-push-world` (`tests/traffic_push_world_test.cpp`). `src/runtime/traffic_push_shapes.{hpp,cpp}` (no SDL); CTest `traffic-push-shapes` (`tests/traffic_push_shapes_test.cpp`).
