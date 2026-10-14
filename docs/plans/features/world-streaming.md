# World streaming

## 1. Scope

How the original organises the city (master level plus world levels), loads and unloads rooms and decides what is visible. Switching between levels and buildings is in [level-transitions.md](level-transitions.md). The port today loads the whole `models/<level>/` folder of one level at once ([assets-formats.md](assets-formats.md) ASSET-S2).

## 2. Status

IN_PROGRESS, updated 2026-10-14T15:30:00+0200. The load rule is recovered (`planRoomStreaming`) and `chase-game` applies it every update in the street level: rooms load and unload while the player walks and unloaded rooms are not drawn. Room positions come from the room models (fit, WSTREAM-S3); the current room and the load time are stand-ins (WSTREAM-S4).

## 3. Verified behaviour

- The city is `lvl_the_city.blvl` (master) plus eight world levels; at gameplay-start only the master and `lvl_01` are resident, 12 of the 43 rooms of `lvl_01` loaded. evidence: [streaming.md §2026-10-13T22:27](../../re/streaming.md#ts-20261013-222700).
- Section +0x34 of the master level holds room links (309 records). Pinned by `streaming-real-data`.
- Wanted rooms each update = the current room's link list (room first) plus up to three preloads from the neighbour list of the loaded neighbour most in the camera's view. A room that falls out of the set unloads in the same update; a new one loads asynchronously (9 updates for Room16). Pinned by `room-streaming-real-data` (five readings in two rooms, plus the Room7 to Room8 step). evidence: [streaming-rooms.md §2026-10-14T14:00](../../re/streaming-rooms.md#ts-20261014-140018).
- Port runtime (street level): the settled start set equals the oracle's (9 rooms from Room7, 12 from the default start in Room8); the sets of the walks `wstream-walk-b`, `-c`, `-d` are reached in order and the walk1 step (Room6 out at once, Room16 in after 9 updates) is reproduced. In the police station only Room1 is loaded of 3 (the oracle's scene list also had Room3 and Room5 unloaded, [streaming.md §2026-10-14T11:13](../../re/streaming.md#ts-20261014-111312)). Pinned by `room-streaming-real-data` and `game-room-streaming`. evidence: [streaming-rooms.md §2026-10-14T15:07:48](../../re/streaming-rooms.md#ts-20261014-150748).
- `pregen/*.bin` is traffic, pedestrian and collectable data, not room streaming.

## 4. Stand-ins

- WSTREAM-S1 `chase-game` loads the whole level folder into memory (no master level, ASSET-S2) and only hides rooms that are not loaded; levels without room models for every room show all rooms. Characters and studs of unloaded rooms stay visible (WSTREAM-14).
- WSTREAM-S3 The room node position is the centre of the vertex box of meshes 1 and up of `roomN.bfnmdl`: a fit that matches all 43 oracle values, the code that fills +0xa0 is not read (WSTREAM-12). evidence: [streaming-rooms.md §2026-10-14T15:07:48](../../re/streaming-rooms.md#ts-20261014-150748).
- WSTREAM-S4 The current room is the room whose model box contains the player (smallest area wins), and a requested room loads in a fixed 9 updates, all in parallel; the original's room change trigger and load timing are not recovered (WSTREAM-13). evidence: same entry.

## 5. Open items

- [x] WSTREAM-1 — closed 2026-10-14: rule recovered (request chain 0x197794, 0x2af6b0, 0x279834, apply 0x189c90). evidence: [streaming-rooms.md §2026-10-14T14:00](../../re/streaming-rooms.md#ts-20261014-140018).
- [x] WSTREAM-2 — closed 2026-10-14: unloading is the absence of a request, driven by the same link lists (0x173f94); the `root` object of each scene is always requested; the `lvl_05` records' effect is open as WSTREAM-11. evidence: same entry.
- [x] WSTREAM-3 — closed 2026-10-14: `RoomStreamer` applies `planRoomStreaming` every update in `chase-game` (street level), room models and placed objects are shown only while loaded, the script bound rule uses the loaded rooms (SCRIPT-S7). Remaining: WSTREAM-13, WSTREAM-14. evidence: [streaming-rooms.md §2026-10-14T15:07:48](../../re/streaming-rooms.md#ts-20261014-150748).
- [ ] WSTREAM-4 — Visibility inside the loaded rooms (hide or draw): objects carry their room (object record +8); find what hides objects of unloaded rooms and what the camera or portals add. needs: WSTREAM-9.
- [ ] WSTREAM-5 — Section +0x20 (0x28-byte records with room names and float triples, probably paths) and the +0x24 byte tuple at +8; decode in `chase_assets` with a real-data test. One session.
- [ ] WSTREAM-6 — The `RoomStreamingControl/Destination` classes and `StreamRoomIn` / `RoomStreamingWait` natives as scripted preloads (the replica takes a `scripted` set). needs: SCRIPT-1.
- [x] WSTREAM-7 — Closed 2026-10-13: the reported grey area behind the police-station start is a view the original never shows; its cause is the missing camera lock, tracked as CAMERA-6.
- [x] WSTREAM-8 — closed 2026-10-14: the node position is the centre of the vertex box of meshes 1 and up (43 of 43 rooms); the code itself stays open as WSTREAM-12. evidence: [streaming-rooms.md §2026-10-14T15:07:48](../../re/streaming-rooms.md#ts-20261014-150748).
- [ ] WSTREAM-12 — Read the code that fills the room node position ([room+0x14]+0xa0) and replace stand-in WSTREAM-S3. evidence: same entry.
- [ ] WSTREAM-13 — Find what changes the current room after the level start (only 0x2e6354 and the teardown 0x3035c0 write 0x376c78 statically) and the real load time of a room; replaces WSTREAM-S4. Needs an oracle trap on the room change. evidence: same entry.
- [ ] WSTREAM-14 — Hide or keep the characters and studs of unloaded rooms as the original does. needs: WSTREAM-9. evidence: same entry.
- [ ] WSTREAM-9 — What the apply step 0x189c90 does for the request flag bytes 1 and 2 (hide, show, fade) and the function that releases a room to state 3. blocks: WSTREAM-4. evidence: same entry.
- [ ] WSTREAM-10 — Meaning of byte `[0x5163e8+0x26]` (skips the ranking when 1) and of the scripted request bytes (0x2297b0, 0x236104). Check in a state where it is 1.
- [ ] WSTREAM-11 — Effect of the `lvl_05` room link records (world level hash 0, empty lists) and of `root` records in consumer 0x173f94.

## 6. Findings summary

- Room node position from the model, current room, port runtime: [streaming-rooms.md §2026-10-14T15:07:48](../../re/streaming-rooms.md#ts-20261014-150748).
- World levels, scene array, room objects and states, room links, request list, natives: [streaming.md §2026-10-13T22:27](../../re/streaming.md#ts-20261013-222700).
- Request chain, ranking key, preloads, unload rule: [streaming-rooms.md §2026-10-14T14:00](../../re/streaming-rooms.md#ts-20261014-140018).

## 7. Code and tests

- `Level::roomLinks`, `RoomLinks` (`src/assets/level.cpp`, `assets.hpp`). CTest `streaming-real-data`.
- `streamRoomsOf`, `planRoomStreaming`, `roomBoxOf` (`src/runtime/room_streaming.{hpp,cpp}`), `RoomStreamer` (`src/runtime/room_streamer.{hpp,cpp}`), glue `src/game_level_rooms.cpp`. CTests `room-streaming-real-data`, `game-room-streaming`. Oracle states `wstream-room8-settled`, `wstream-walk-b`, `-c`, `-d`.
