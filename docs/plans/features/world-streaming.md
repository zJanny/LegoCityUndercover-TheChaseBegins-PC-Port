# World streaming

## 1. Scope

How the original organises the city (master level plus world levels), loads and unloads rooms and decides what is visible. Switching between levels and buildings is in [level-transitions.md](level-transitions.md). The port today loads the whole `models/<level>/` folder of one level at once ([assets-formats.md](assets-formats.md) ASSET-S2).

## 2. Status

IN_PROGRESS, updated 2026-10-14T14:01:23+0200. The rule that selects the loaded rooms is recovered and pinned by five oracle readings (`planRoomStreaming`, `src/runtime/room_streaming.cpp`): the current room's link list plus up to three view-ranked preloads. Not used by `chase-game` yet (WSTREAM-3).

## 3. Verified behaviour

- The city is `lvl_the_city.blvl` (master) plus eight world levels; at gameplay-start only the master and `lvl_01` are resident, 12 of the 43 rooms of `lvl_01` loaded. evidence: [streaming.md §2026-10-13T22:27](../../re/streaming.md#ts-20261013-222700).
- Section +0x34 of the master level holds room links (309 records). Pinned by `streaming-real-data`.
- Wanted rooms each update = the current room's link list (room first) plus up to three preloads from the neighbour list of the loaded neighbour most in the camera's view. A room that falls out of the set unloads in the same update; a new one loads asynchronously (9 updates for Room16). Pinned by `room-streaming-real-data` (five readings in two rooms, plus the Room7 to Room8 step). evidence: [streaming-rooms.md §2026-10-14T14:00](../../re/streaming-rooms.md#ts-20261014-140018).
- `pregen/*.bin` is traffic, pedestrian and collectable data, not room streaming.

## 4. Stand-ins

- WSTREAM-S1 `chase-game` loads one whole level folder and shows all rooms; no room is unloaded and no master level exists (same as ASSET-S2).
- WSTREAM-S2 The room positions in `room-streaming-real-data` are the 43 values read from the oracle, not derived from game data (see WSTREAM-8).

## 5. Open items

- [x] WSTREAM-1 — closed 2026-10-14: rule recovered (request chain 0x197794, 0x2af6b0, 0x279834, apply 0x189c90). evidence: [streaming-rooms.md §2026-10-14T14:00](../../re/streaming-rooms.md#ts-20261014-140018).
- [x] WSTREAM-2 — closed 2026-10-14: unloading is the absence of a request, driven by the same link lists (0x173f94); the `root` object of each scene is always requested; the `lvl_05` records' effect is open as WSTREAM-11. evidence: same entry.
- [ ] WSTREAM-3 — Native room state per level in `chase-game`: apply `planRoomStreaming` every update, load and unload room models and objects, with the oracle's start set. needs: WSTREAM-8, ASSET-1 (room models). blocks: TRANS-3.
- [ ] WSTREAM-4 — Visibility inside the loaded rooms (hide or draw): objects carry their room (object record +8); find what hides objects of unloaded rooms and what the camera or portals add. needs: WSTREAM-9.
- [ ] WSTREAM-5 — Section +0x20 (0x28-byte records with room names and float triples, probably paths) and the +0x24 byte tuple at +8; decode in `chase_assets` with a real-data test. One session.
- [ ] WSTREAM-6 — The `RoomStreamingControl/Destination` classes and `StreamRoomIn` / `RoomStreamingWait` natives as scripted preloads (the replica takes a `scripted` set). needs: SCRIPT-1.
- [x] WSTREAM-7 — Closed 2026-10-13: the reported grey area behind the police-station start is a view the original never shows; its cause is the missing camera lock, tracked as CAMERA-6.
- [ ] WSTREAM-8 — Source of the room node position ([room+0x14]+0xa0) used by the ranking; fit so far: x/z from mesh 0 of `roomN.bfnmdl`, y open. blocks: WSTREAM-3. evidence: [streaming-rooms.md §2026-10-14T14:00](../../re/streaming-rooms.md#ts-20261014-140018).
- [ ] WSTREAM-9 — What the apply step 0x189c90 does for the request flag bytes 1 and 2 (hide, show, fade) and the function that releases a room to state 3. blocks: WSTREAM-4. evidence: same entry.
- [ ] WSTREAM-10 — Meaning of byte `[0x5163e8+0x26]` (skips the ranking when 1) and of the scripted request bytes (0x2297b0, 0x236104). Check in a state where it is 1.
- [ ] WSTREAM-11 — Effect of the `lvl_05` room link records (world level hash 0, empty lists) and of `root` records in consumer 0x173f94.

## 6. Findings summary

- World levels, scene array, room objects and states, room links, request list, natives: [streaming.md §2026-10-13T22:27](../../re/streaming.md#ts-20261013-222700).
- Request chain, ranking key, preloads, unload rule: [streaming-rooms.md §2026-10-14T14:00](../../re/streaming-rooms.md#ts-20261014-140018).

## 7. Code and tests

- `Level::roomLinks`, `RoomLinks` (`src/assets/level.cpp`, `assets.hpp`). CTest `streaming-real-data`.
- `streamRoomsOf`, `planRoomStreaming` (`src/runtime/room_streaming.{hpp,cpp}`). CTest `room-streaming-real-data`. Oracle states `wstream-room8-settled`, `wstream-walk-b`, `-c`, `-d`.
