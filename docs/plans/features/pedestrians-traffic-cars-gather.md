# Traffic car contact gather

## 1. Scope

The grid gather 0x237f30 that the contact step 0x29cc38 of a traffic car runs with the descriptor mask 4, so that `slideTrafficCarContacts` gets the characters near a car in driver states 5 and 8. The driver is in [pedestrians-traffic-cars-driver.md](pedestrians-traffic-cars-driver.md); the push the gathered entries go to is in [pedestrians-traffic-cars-push.md](pedestrians-traffic-cars-push.md).

## 2. Status

IN_PROGRESS, updated 2026-11-12T11:15:16+01:00. The gather is ported (`gatherTrafficGrid`, hook `makeTrafficContactGather`) and the grid is built and kept as the original does: the layout 0x271280 (`buildTrafficGridLayout`), the cell lists of the add 0x1f5cd0 and the cell update 0x209cb8 (`trafficGridUpdateCells`), the registration pass 0x30e0d8 (a new entry holds the cell bytes 255 255 255 / 0 0 0, no entry is ever removed). chase-game registers the player and every pedestrian slot as characters (`TrafficCharacterFeed`, glue `src/game_traffic_characters.cpp`; a disabled one carries owner flag bit 0) and the traffic cars gather and push them (`TrafficPushSupply`, [pedestrians-traffic-cars-push.md](pedestrians-traffic-cars-push.md)). The level inputs of the build and the rest of the pass are stand-ins (TRAFFIC-S46).

## 3. Verified behaviour

- The walk, filters, box test, cap and visited mark are transcribed from 0x237f30 and its helpers; the test `traffic-character-gather` pins them on made-up data evaluated by hand from the disassembly (no oracle run).
- The registration pass 0x30e0d8 (the scene's list, the rooms of the current group with their items and the three entity lists, the entity filter, the new entry's bytes, no removal) is read in [traffic-character-gather.md §2026-11-12T11:13:41](../../re/traffic-character-gather.md#ts-20261112-111341); the test `traffic-character-feed` pins the new entry's cell bytes, that a character that is not listed stays, and that a disabled one is not gathered (made up data, no oracle run).
- The grid build (extent as the intersection of two bound sets, halving of the largest cell size until 2048 cells, counts, origin), the cell index (z x count z + y) x count x + x and the order of a cell's list (append, the last entry takes a leaver's place) are transcribed from 0x271280, 0x209cb8 and 0x1f5cd0; the test `traffic-character-feed` pins them on made-up data evaluated by hand (no oracle run), and a car beside a pedestrian gets its entry.

## 4. Stand-ins

- TRAFFIC-S46 What chase-game puts in the grid and the world of the push: the layout's first bound set is the root boxes of the level's static collision items and the second the vertex boxes of all rooms (not the scene's list, the rooms' items and node bounds of the current room group); the grid is built at the first drive step; only the player and the pedestrian slots are entries (no entity of the level), the player is flagged disabled while seated; the pair test of a car's ignore list ignores no pair; a pushed character's word +0xc bit 3 is clear and its state is idle. needs: TRAFFIC-65, TRAFFIC-67. evidence: [traffic-character-gather.md §2026-11-12T11:13:41](../../re/traffic-character-gather.md#ts-20261112-111341)

## 5. Open items

- [ ] TRAFFIC-63 — A chase-game scene test where a traffic car in driver state 5 or 8 beside a pedestrian gathers it (the log line `contact gather lists N characters`); the existing scenes reach no such car and no short route to one is known. evidence: [traffic-character-gather.md §2026-11-12T11:13:41](../../re/traffic-character-gather.md#ts-20261112-111341)
- [ ] TRAFFIC-65 — Read the caller and time of the registration pass (a rebuild at a room change?), its per-entity call, the second slot counter's growth and the second grid over the same scene. blocks: TRAFFIC-S46. evidence: [traffic-character-gather.md §2026-11-12T11:13:41](../../re/traffic-character-gather.md#ts-20261112-111341)

## 6. Findings summary

- [traffic-character-gather.md §2026-11-12T08:10:25](../../re/traffic-character-gather.md#ts-20261112-081025): the gather 0x237f30 (cell walk, filters, box test, cap, visited mark), the cell of a point 0x20ea60 and the shape box 0x22ec3c.
- [traffic-character-gather.md §2026-11-12T09:58:20](../../re/traffic-character-gather.md#ts-20261112-095820): the grid build 0x271280, the add 0x1f5cd0 and the cell update 0x209cb8, the corrected cell index, and the feed of chase-game's characters.
- [traffic-character-gather.md §2026-11-12T11:13:41](../../re/traffic-character-gather.md#ts-20261112-111341): the registration pass 0x30e0d8, the new entry's bytes, no removal, and the supply of the push world.

## 7. Code and tests

`src/runtime/traffic_character_gather.{hpp,cpp}`, `src/runtime/traffic_character_feed.{hpp,cpp}` and `src/runtime/traffic_push_supply.{hpp,cpp}` (no SDL), glue `src/game_traffic_characters.cpp`; CTest `traffic-character-gather` (`tests/traffic_character_gather_test.cpp`) and `traffic-character-feed` (`tests/traffic_character_feed_test.cpp`).
