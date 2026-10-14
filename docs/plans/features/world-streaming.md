# World streaming

## 1. Scope

How the original organises the city (master level plus world levels), loads and unloads rooms and decides what is visible. Switching between levels and buildings is in [level-transitions.md](level-transitions.md). The port today loads the whole `models/<level>/` folder of one level at once ([assets-formats.md](assets-formats.md) ASSET-S2).

## 2. Status

IN_PROGRESS, updated 2026-10-13T22:30:00+0200. Mapped from code and the oracle: the data model and the room-link table are decoded (`Level::roomLinks`); the rule that loads and unloads rooms while the player moves is not found. `chase-game` is unchanged.

## 3. Verified behaviour

- The city is `lvl_the_city.blvl` (master) plus eight world levels; at gameplay-start only the master and `lvl_01` are loaded (state 2), 12 of the 43 rooms of `lvl_01` are loaded (Room 1, 2, 3, 7, 8, 9, 14, 15, 16, 17, 22, 23), the player is in Room8. evidence: [streaming.md §2026-10-13T22:27](../../re/streaming.md#ts-20261013-222700).
- Section +0x34 of the master level holds room links (309 records); the links of Room8 in `WorldLevel_01_CherryTreeHills` equal nine of the twelve loaded rooms. Pinned by `streaming-real-data`.
- `pregen/*.bin` is traffic, pedestrian and collectable data, not room streaming.

## 4. Stand-ins

- WSTREAM-S1 `chase-game` loads one whole level folder and shows all rooms; no room is unloaded and no master level exists (same as ASSET-S2).

## 5. Open items

- [ ] WSTREAM-1 — Partly done: the loaded set follows the current room (sets for Room7 and Room8 and after the station switch are in the evidence). Open: the writer of the room state byte and the ranking key of the per-update function 0x279ab0. blocks: WSTREAM-3. evidence: [streaming.md §2026-10-14T11:13](../../re/streaming.md#ts-20261014-111312).
- [ ] WSTREAM-2 — Decide what the room link table is used for at run time (consumer 0x173f94 builds neighbour lists; check whether the unload uses the same lists) and whether the `root` record and the `lvl_05` records matter. needs: WSTREAM-1.
- [ ] WSTREAM-3 — Native room state per level in `chase-game`: load the rooms of the link set around the player's room, unload the rest, with the same start set as the oracle. needs: WSTREAM-1, ASSET-1 (room models). blocks: TRANS-3.
- [ ] WSTREAM-4 — Visibility inside the loaded rooms (hide or draw): objects carry their room (object record +8); find what hides objects of unloaded rooms and what the camera or portals add. needs: WSTREAM-1.
- [ ] WSTREAM-5 — Section +0x20 (0x28-byte records with room names and float triples, probably paths) and the +0x24 byte tuple at +8; decode in `chase_assets` with a real-data test. One session.
- [ ] WSTREAM-6 — The `RoomStreamingControl/Destination` classes and `StreamRoomIn` / `RoomStreamingWait` natives as scripted preloads. needs: SCRIPT-1.
- [x] WSTREAM-7 — Closed 2026-10-13: the reported grey area behind the police-station start is a view the original never shows; its cause is the missing camera lock, tracked as CAMERA-6.

## 6. Findings summary

- Everything is in [streaming.md §2026-10-13T22:27](../../re/streaming.md#ts-20261013-222700): world levels, scene array, room objects and states, room links, request list, natives.

## 7. Code and tests

- `Level::roomLinks`, `RoomLinks` (`src/assets/level.cpp`, `assets.hpp`). CTest `streaming-real-data`.
