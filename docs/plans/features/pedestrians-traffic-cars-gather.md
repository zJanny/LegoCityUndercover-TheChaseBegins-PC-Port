# Traffic car contact gather

## 1. Scope

The grid gather 0x237f30 that the contact step 0x29cc38 of a traffic car runs with the descriptor mask 4, so that `slideTrafficCarContacts` gets the characters near a car in driver states 5 and 8. The driver is in [pedestrians-traffic-cars-driver.md](pedestrians-traffic-cars-driver.md); the push the gathered entries go to is in [pedestrians-traffic-cars-push.md](pedestrians-traffic-cars-push.md).

## 2. Status

IN_PROGRESS, updated 2026-11-12T10:01:39+01:00. The gather is ported (`gatherTrafficGrid`, hook `makeTrafficContactGather`) and the grid is built and kept as the original does: the layout 0x271280 (`buildTrafficGridLayout`), the cell lists of the add 0x1f5cd0 and the cell update 0x209cb8 (`trafficGridUpdateCells`). chase-game feeds the player and the enabled pedestrians to it (`TrafficCharacterFeed`, glue `src/game_traffic_characters.cpp`) and sets `gatherContacts` and `slide` for the traffic cars. The slide's push is not applied (TRAFFIC-56) and the level inputs of the build are stand-ins (TRAFFIC-S44).

## 3. Verified behaviour

- The walk, filters, box test, cap and visited mark are transcribed from 0x237f30 and its helpers; the test `traffic-character-gather` pins them on made-up data evaluated by hand from the disassembly (no oracle run).
- The grid build (extent as the intersection of two bound sets, halving of the largest cell size until 2048 cells, counts, origin), the cell index (z x count z + y) x count x + x and the order of a cell's list (append, the last entry takes a leaver's place) are transcribed from 0x271280, 0x209cb8 and 0x1f5cd0; the test `traffic-character-feed` pins them on made-up data evaluated by hand (no oracle run), and a car beside a pedestrian gets its entry.

## 4. Stand-ins

- TRAFFIC-S44 What chase-game puts in the grid and how the layout is fed: the layout's bound sets are the vertex boxes of all rooms (not the scene's shape list, the rooms' items and node bounds); only the player (not when seated) and the enabled pedestrians are entries, with mask 4, kind 0, owner flag 0x200 and the entity box as shape; a new entry holds no cell range and a leaving one is taken out of its lists; the pair test ignores no pair; the slide's push is not applied. needs: TRAFFIC-61, TRAFFIC-62. blocks: TRAFFIC-56. evidence: [traffic-character-gather.md §2026-11-12T09:58:20](../../re/traffic-character-gather.md#ts-20261112-095820)

## 5. Open items

- [ ] TRAFFIC-61 — Find the port's data for the scene's shape list, the rooms' item lists and node bounds, the room set of the build and the points where the grid is rebuilt. blocks: TRAFFIC-S44. evidence: [traffic-character-gather.md §2026-11-12T09:58:20](../../re/traffic-character-gather.md#ts-20261112-095820)
- [ ] TRAFFIC-62 — Read the entry creation (the cell bytes of a new entry, the slot counter of owners with the table flag), the removal of an entry from the grid, the flags of the characters' entities (flag words, node word) and which entities are in the grid at a time; the descriptor mode 2 and an entry byte stay unread. blocks: TRAFFIC-S44. evidence: [traffic-character-gather.md §2026-11-12T09:58:20](../../re/traffic-character-gather.md#ts-20261112-095820)
- [ ] TRAFFIC-63 — A chase-game scene test where a traffic car in driver state 5 or 8 beside a pedestrian gathers it (the log line `contact gather lists N characters`); the existing scenes reach no such car. needs: TRAFFIC-56. evidence: [traffic-character-gather.md §2026-11-12T09:58:20](../../re/traffic-character-gather.md#ts-20261112-095820)

## 6. Findings summary

- [traffic-character-gather.md §2026-11-12T08:10:25](../../re/traffic-character-gather.md#ts-20261112-081025): the gather 0x237f30 (cell walk, filters, box test, cap, visited mark), the cell of a point 0x20ea60 and the shape box 0x22ec3c.
- [traffic-character-gather.md §2026-11-12T09:58:20](../../re/traffic-character-gather.md#ts-20261112-095820): the grid build 0x271280, the add 0x1f5cd0 and the cell update 0x209cb8, the corrected cell index, and the feed of chase-game's characters.

## 7. Code and tests

`src/runtime/traffic_character_gather.{hpp,cpp}` and `src/runtime/traffic_character_feed.{hpp,cpp}` (no SDL), glue `src/game_traffic_characters.cpp`; CTest `traffic-character-gather` (`tests/traffic_character_gather_test.cpp`) and `traffic-character-feed` (`tests/traffic_character_feed_test.cpp`).
