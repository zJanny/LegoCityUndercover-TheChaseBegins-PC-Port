# World streaming

## 1. Scope

How the original organises the city (master level plus world levels), loads and unloads rooms and decides what is visible. Switching between levels and buildings is in [level-transitions.md](level-transitions.md). The port today loads the whole `models/<level>/` folder of one level at once ([assets-formats.md](assets-formats.md) ASSET-S2).

## 2. Status

IN_PROGRESS, updated 2026-10-14T20:55:00+0200. The load rule is recovered (`planRoomStreaming`) and `chase-game` applies it every update in the street level: rooms load and unload while the player walks and unloaded rooms are not drawn. The current room follows the player by the original's rule (`roomAfterMove`, checked against 160 oracle updates). Room positions come from the room models (fit, WSTREAM-S3); the load time is a stand-in (WSTREAM-S30).

## 3. Verified behaviour

- The city is `lvl_the_city.blvl` (master) plus eight world levels; at gameplay-start only the master and `lvl_01` are resident, 12 of the 43 rooms of `lvl_01` loaded. evidence: [streaming.md §2026-10-13T22:27](../../re/streaming.md#ts-20261013-222700).
- Section +0x34 of the master level holds room links (309 records). Pinned by `streaming-real-data`.
- Wanted rooms each update = the current room's link list (room first) plus up to three preloads from the neighbour list of the loaded neighbour most in the camera's view. A room that falls out of the set unloads in the same update; a new one loads asynchronously (9 updates for Room16). Pinned by `room-streaming-real-data` (five readings in two rooms, plus the Room7 to Room8 step). evidence: [streaming-rooms.md §2026-10-14T14:00](../../re/streaming-rooms.md#ts-20261014-140018).
- Port runtime (street level): the settled start set equals the oracle's (9 rooms from Room7, 12 from the default start in Room8); the sets of the walks `wstream-walk-b`, `-c`, `-d` are reached in order and the walk1 step (Room6 out at once, Room16 in after 9 updates) is reproduced. In the police station only Room1 is loaded of 3 (the oracle's scene list also had Room3 and Room5 unloaded, [streaming-transitions.md §2026-10-14T11:13](../../re/streaming-transitions.md#ts-20261014-111312)). Pinned by `room-streaming-real-data` and `game-room-streaming`. evidence: [streaming-rooms.md §2026-10-14T15:07:48](../../re/streaming-rooms.md#ts-20261014-150748).
- The current room is the room that owns the player's node: the player is moved to another room's node when its lifted position leaves the current room's box, the first room of the current room's link list that contains it, else the first room in number order, else the nearest by vertical gap. The room is kept while it still contains the player, even when another box does too. Pinned by `room-streaming-real-data` (Room7 to Room8 at x 899.5, back at x 900.0, 160 oracle updates replayed once offline). evidence: [streaming-rooms.md §2026-10-14T20:51:26](../../re/streaming-rooms.md#ts-20261014-205126).
- Room loads take 6 to 12 updates in the stepped oracle (9 loads, four rooms, differing between runs); two rooms requested together finish about 6 updates apart. Same entry.
- `pregen/*.bin` is traffic, pedestrian and collectable data, not room streaming.

## 4. Stand-ins

- WSTREAM-S1 `chase-game` loads the whole level folder into memory (no master level, ASSET-S2) and only hides rooms that are not loaded; levels without room models for every room show all rooms. Characters and studs of unloaded rooms stay visible (WSTREAM-14).
- WSTREAM-S3 The room node position is the centre of the vertex box of meshes 1 and up of `roomN.bfnmdl`: a fit that matches all 43 oracle values, the code that fills +0xa0 is not read (WSTREAM-12). evidence: [streaming-rooms.md §2026-10-14T15:07:48](../../re/streaming-rooms.md#ts-20261014-150748).
- WSTREAM-S30 A requested room loads in a fixed 9 updates, all in parallel; the original loads one room at a time in priority order, 6 to 12 updates per room in the stepped oracle and 6 to 15 in a free run ([rendering-fog.md §2026-10-14T21:42](../../re/rendering-fog.md#ts-20261014-214220)), on a separate thread. needs: WSTREAM-30. evidence: [streaming-rooms.md §2026-10-14T20:51:26](../../re/streaming-rooms.md#ts-20261014-205126).
- WSTREAM-S31 The player's box centre (0, 0.6875, 0.1399) that is added to its position for the room test is one reading of the node's +0xa0, not derived from the model. The level start still picks the room by box area when the level gives none (`RoomStreamer::roomAt`). evidence: same entry.

## 5. Open items

- [ ] WSTREAM-4 — Visibility inside the loaded rooms (hide or draw): objects carry their room (object record +8); find what hides objects of unloaded rooms and what the camera or portals add. needs: WSTREAM-9.
- [ ] WSTREAM-5 — Section +0x20 (0x28-byte records with room names and float triples, probably paths) and the +0x24 byte tuple at +8; decode in `chase_assets` with a real-data test. One session.
- [ ] WSTREAM-6 — The `RoomStreamingControl/Destination` classes and `StreamRoomIn` / `RoomStreamingWait` natives as scripted preloads (the replica takes a `scripted` set). needs: SCRIPT-1.
- [ ] WSTREAM-12 — Read the code that fills the room node position ([room+0x14]+0xa0) and replace stand-in WSTREAM-S3. evidence: same entry.
- [ ] WSTREAM-30 — Read the loader worker thread (queue 0x52cf1c, semaphore `[0x3765a4+0x10]`, dequeue by priority) for the per-room load cost and the serial order; replace WSTREAM-S30. needs: WSTREAM-9.
- [ ] WSTREAM-31 — Meaning of `u16 [room+0x34]` (a non-zero value ends the room search), the other object lists (k 1 to 3) and the entity table at 0x589410 that 0x2e6ce4 also moves between rooms, and the player's box centre during animation (WSTREAM-S31). evidence: same entry.
- [ ] WSTREAM-14 — Hide or keep the characters and studs of unloaded rooms as the original does. needs: WSTREAM-9. evidence: same entry.
- [ ] WSTREAM-9 — What the apply step 0x189c90 does for the request flag bytes 1 and 2 (hide, show, fade) and the function that releases a room to state 3. blocks: WSTREAM-4. evidence: same entry.
- [ ] WSTREAM-10 — Meaning of byte `[0x5163e8+0x26]` (skips the ranking when 1) and of the scripted request bytes (0x2297b0, 0x236104). Check in a state where it is 1.
- [ ] WSTREAM-11 — Effect of the `lvl_05` room link records (world level hash 0, empty lists) and of `root` records in consumer 0x173f94.

## 6. Findings summary

- Current room rule and load times: [streaming-rooms.md §2026-10-14T20:51:26](../../re/streaming-rooms.md#ts-20261014-205126).
- Room node position from the model, port runtime: [streaming-rooms.md §2026-10-14T15:07:48](../../re/streaming-rooms.md#ts-20261014-150748).
- World levels, scene array, room objects and states, room links, request list, natives: [streaming.md §2026-10-13T22:27](../../re/streaming.md#ts-20261013-222700).
- Request chain, ranking key, preloads, unload rule: [streaming-rooms.md §2026-10-14T14:00](../../re/streaming-rooms.md#ts-20261014-140018).

## 7. Code and tests

- `Level::roomLinks`, `RoomLinks` (`src/assets/level.cpp`, `assets.hpp`). CTest `streaming-real-data`.
- `streamRoomsOf`, `planRoomStreaming`, `roomBoxOf` (`src/runtime/room_streaming.{hpp,cpp}`), `RoomStreamer` (`src/runtime/room_streamer.{hpp,cpp}`), `roomAfterMove`, `approachToBox` (`src/runtime/room_locator.{hpp,cpp}`), glue `src/game_level_rooms.cpp`. CTests `room-streaming-real-data`, `game-room-streaming`. Oracle states `wstream-room8-settled`, `wstream-walk-b`, `-c`, `-d`.
