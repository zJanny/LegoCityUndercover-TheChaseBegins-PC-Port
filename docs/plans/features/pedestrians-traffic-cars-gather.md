# Traffic car contact gather

## 1. Scope

The grid gather 0x237f30 that the contact step 0x29cc38 of a traffic car runs with the descriptor mask 4, so that `slideTrafficCarContacts` gets the characters near a car in driver states 5 and 8. The driver is in [pedestrians-traffic-cars-driver.md](pedestrians-traffic-cars-driver.md); the push the gathered entries go to is in [pedestrians-traffic-cars-push.md](pedestrians-traffic-cars-push.md).

## 2. Status

IN_PROGRESS, updated 2026-11-12T08:11:13+01:00. The gather is ported (`gatherTrafficGrid`, hook `makeTrafficContactGather` for `TrafficDriveStep::gatherContacts`, TRAFFIC-43 done): the cell walk, the filters of an entry, the box test and the cap, for the descriptor modes 0 and 1. chase-game does not set the hook, so no car sees a character yet (TRAFFIC-S39, TRAFFIC-53).

## 3. Verified behaviour

- The walk, filters, box test, cap and visited mark are transcribed from 0x237f30 and its helpers; the test `traffic-character-gather` pins them on made-up data evaluated by hand from the disassembly (no oracle run).

## 4. Stand-ins

- TRAFFIC-S39 The grid of the contact gather is not the original's: an entry's cells are those of its current world box and its place in a cell's list is its registration order; chase-game passes no characters (the hook is unset, the gather lists nothing); the grid layout is an input, not the level's. needs: TRAFFIC-52, TRAFFIC-53. evidence: [traffic-character-gather.md §2026-11-12T08:10:25](../../re/traffic-character-gather.md#ts-20261112-081025)

## 5. Open items

- [ ] TRAFFIC-52 — Read the level's grid build (extents, cell size adjustment, counts) and the insertion order of an entry in a cell's list, and give `TrafficGrid` the level's layout and order. blocks: TRAFFIC-S39. evidence: [traffic-character-gather.md §2026-11-12T08:10:25](../../re/traffic-character-gather.md#ts-20261112-081025)
- [ ] TRAFFIC-53 — Fill a `TrafficGrid` with the characters of chase-game (the player and the pedestrians: mask 4, owner flag 0x200, node and shape) and set `TrafficDriveStep::gatherContacts` and `slide` for the traffic cars; the descriptor mode 2 and entry byte +5 bit 0 are unread. blocks: TRAFFIC-S39, TRAFFIC-56. evidence: [traffic-character-gather.md §2026-11-12T08:10:25](../../re/traffic-character-gather.md#ts-20261112-081025)

## 6. Findings summary

- [traffic-character-gather.md §2026-11-12T08:10:25](../../re/traffic-character-gather.md#ts-20261112-081025): the gather 0x237f30 (cell walk, filters, box test, cap, visited mark), the cell of a point 0x20ea60, the shape box 0x22ec3c and the grid build.

## 7. Code and tests

`src/runtime/traffic_character_gather.{hpp,cpp}` (no SDL); CTest `traffic-character-gather` (`tests/traffic_character_gather_test.cpp`).
