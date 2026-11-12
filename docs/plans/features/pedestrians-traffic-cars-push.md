# Traffic car push: push vector, squash and character events

## 1. Scope

The world of a traffic car's push of a character: the push vector and shape test 0x1f49d8, the squash 0x233594 and the listeners of the events 17 and 18 that message 45 sends to the characters' states. The push 0x2baef0 itself, message 45 of the characters and the cop whistle are in [pedestrians-traffic-cars-driver.md](pedestrians-traffic-cars-driver.md).

## 2. Status

IN_PROGRESS, updated 2026-11-12T08:18:22+01:00. `trafficPushVector` ports 0x1f49d8 for box entries (characters) and serves as the hook `pushVector`; the squash and the listeners of the states 0x99 and 0x199 are ported, and `deliverCarPush` / `applyCarPushToCharacter` connect message 45 to a character's hit handler `receiveHit`. Nothing in `chase-game` supplies the world, so no car pushes a character in play (TRAFFIC-56).

## 3. Verified behaviour

- Read from the code (test `traffic-push-world`), from [traffic-car-push.md §2026-11-12T08:17:17](../../re/traffic-car-push.md#ts-20261112-081717):
  - 0x1f49d8: the open box test of the moved mover box in the pushed box's frame, the world box of a tilted entry (|row 1 y| < 0.99), the slide clear for two entities with 0x100, and with slide the half push of each side (the pushed one by its slide 88, the mover back by its share plus the rest, slide 92);
  - the squash record {damage 32000, kind 0} to the pushing character;
  - only the states 0x99 (super knockback) and 0x199 (throw finisher) take the events 17 and 18; the order of 0x283d0c: event 17, then event 18 or the hit.

## 4. Stand-ins

- TRAFFIC-S40 The parts of the push vector that are hooks: the mesh test (with its turn branch), the oriented test of blockers and part records, the world slide of both sides, and the list of excluded kinds taken as given (empty in the image's data). needs: TRAFFIC-54. evidence: [traffic-car-push.md §2026-11-12T08:17:17](../../re/traffic-car-push.md#ts-20261112-081717)
- TRAFFIC-S41 The listeners' unread parts: the super knockback's damage word is taken as 1 (its value in the image's data), the throw finisher's tests of the other entity are inputs and its combat hit is an outcome only, and the squash record's byte +0x15 is not carried. needs: TRAFFIC-55. evidence: [traffic-car-push.md §2026-11-12T08:17:17](../../re/traffic-car-push.md#ts-20261112-081717)

## 5. Open items

- [ ] TRAFFIC-54 — Read the mesh test, the oriented test and the world slide that the push vector calls, the turn branch of the mesh path, and the writers of the list of excluded kinds. blocks: TRAFFIC-S40, TRAFFIC-56. evidence: [traffic-car-push.md §2026-11-12T08:17:17](../../re/traffic-car-push.md#ts-20261112-081717)
- [ ] TRAFFIC-55 — Read the writers of the super knockback's damage word, the throw finisher's tests and combat hit, the reader of record byte +0x15, and what enters the states 0x99 and 0x199. blocks: TRAFFIC-S41. evidence: [traffic-car-push.md §2026-11-12T08:17:17](../../re/traffic-car-push.md#ts-20261112-081717)
- [ ] TRAFFIC-56 — Supply `TrafficPushWorld` and `TrafficPushShapes` in `chase-game` from its entities and collision, and apply `deliverCarPush` to the pushed characters. needs: TRAFFIC-53, TRAFFIC-54, TRAFFIC-S44. blocks: TRAFFIC-S34, TRAFFIC-63. evidence: [traffic-car-push.md §2026-11-12T08:17:17](../../re/traffic-car-push.md#ts-20261112-081717)

## 6. Findings summary

- [traffic-car-push.md §2026-11-12T08:17:17](../../re/traffic-car-push.md#ts-20261112-081717): the push vector 0x1f49d8, the squash 0x233594, the listeners of the events 17 and 18 (0x3305fc, 0x334888 / 0x1e3040) and their states 0x99 and 0x199.

## 7. Code and tests

`src/runtime/traffic_push_world.{hpp,cpp}` (no SDL); CTest `traffic-push-world` (`tests/traffic_push_world_test.cpp`).
