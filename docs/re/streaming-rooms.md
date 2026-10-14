# Room streaming rule (which rooms are loaded)

The per-update rule that decides which rooms of a world level are requested, loaded and unloaded. Data model, room links and scene array: [streaming.md](streaming.md). Feature file: [world-streaming](../plans/features/world-streaming.md). Replica: `src/runtime/room_streaming.{hpp,cpp}`, test `room-streaming-real-data` (`tests/room_streaming_real_test.cpp`).

<a id="ts-20261014-140018"></a>

## 2026-10-14T14:00:18+02:00 — The request rule: neighbour list plus three view-ranked preloads (REA not used for this entry; llvm-objdump `--mcpu=arm1176jzf-s` on `artifacts/game/game.arm.elf`, Azahar oracle New 3DS, GDB reads and `memtrace`, no RPC)

Corrected by [§2026-10-14T15:07:48](#ts-20261014-150748): the room node position is the centre of the vertex box of meshes 1 and up (all 43 rooms), not mesh 0's centre plus an offset.

[Corrects streaming.md §2026-10-14T11:13](streaming.md#ts-20261014-111312) (the "rank" remark and the seven-room reading). All addresses are for the reference build.

**Per-update chain (scene room update 0x197794)**
- 0x197794 runs per update over the scenes of the list at 0x589a18 (count `[0x589a18+8]`, pointers `+0xc`). Each scene has a dirty-room list at scene+0x132c (count) / scene+0x1330 (array); 0x205b5c(room, request) appends a room there when its new request word differs from its applied word.
- Step 1 (0x1977ac..0x197808): every room in the dirty lists gets the default request word at 0x376c7c, read in the oracle as bytes `00 02 ff 00` (the `0002ff00` that every unrequested room shows at +0x38 and +0x3c).
- Step 2 (0x197810): `blx [0x376c84]`; the global holds 0x279834 (read in the oracle). 0x279834 first calls 0x2af6b0, then runs the ranking below.
- Step 3 (0x19781c..0x197860): 0x1dd5c8 per dirty room. Step 4 (0x1978a0..0x197924): 0x189c90 (room apply) per dirty room, then the room is dropped from the dirty list when its applied word at +0x38 equals the default word.
- Room request layout (room +0x38 applied, +0x3c requested, each 4 bytes): byte 0 load wanted (0 or 1), bytes 1 and 2 (neighbour requests carry 1 and 0 for the current room or 1 for the other linked rooms; the default is 2 and 0xff; a preloaded room keeps 2 and 0xff, read as `01 02 ff c8` for Room3), byte 3 priority. The loader state byte is the state struct byte +8 (`[room+4]+8`: 1 loading, 2 loaded, 3 not loaded).

**0x2af6b0, neighbours of the current room**
- The current room (global 0x376c78) holds its neighbour list: count u16 at +0x30, array pointer at +0x40, entries of 0x18 bytes, resolved to the room object by 0x21b51c. The list is built from the section +0x34 record of the room (decoded: `Level::roomLinks`) by consumer 0x173f94, with the room itself as entry 0. In the oracle Room7's list is Room7, 1, 2, 6, 8, 14, 15 and Room8's is Room8, 1, 2, 3, 7, 9, 14, 15, 16; both equal the level data.
- Each entry carries a request word at +0x14: `01 01 00 80` for the room itself, `01 01 01 80` for the others. 0x2af6b0 copies it into the linked room's +0x3c..0x3f and appends the room to the dirty list. The same function sets the request of each scene's root object (scene+0x1328) to `01 01 00` plus its old priority byte, so the root of every scene is always wanted.
- Unloading is therefore the absence of a request: a room that is not in the current room's list and not preloaded keeps the default request, and the apply step drops it. Measured with `memtrace` (walk1: stick 20 150 from `street-at-entrance`): at update 28 the current room changes from Room7 to Room8; in the same update Room6 gets request 0 and state 3 (unloaded at once), Room16 is requested (state 1 for 9 updates, loaded at update 37).

**0x279834, ranking and preloads** (skipped when byte `[0x5163e8+0x26]` is 1; 2 in the oracle states used)
- Inputs: A = player position, `[[0x3772b8]+0x3c]+0x5c+0x30` (3 floats); B = view direction, `[0x377544]+0x5c+0x20` (3 floats, the third row of that object's matrix, the camera's look direction: the camera sits 5.8, 3.9, 8.1 behind the player and looks along B in the oracle); P(room) = `[room+0x14]+0xa0` (the room node position).
- Key: `dot(normalize(P - A), B)`; a length whose float bits are not above 872415232 (0x34000000) gives the vector (0, 0, 1). 0x257f14 subtracts, 0x257ea0 normalises, 0x257e78 is the dot product.
- Loop 1: the key of every list entry (including the room itself) goes into a table. A list entry that is not loaded (0x2284bc: state 2 and resource set) gets request priority 255 (the current room) or 254 minus the number of entries with a greater key, written to the request word via 0x205b5c with the room's own flag bytes.
- Anchor: among the loaded entries that are not the current room, the one with the greatest key above 0 (start value 0.0, strictly greater replaces).
- Preloads: if there is an anchor, each entry of the anchor's own list (the anchor itself first) whose room has byte 0 of its request equal to 0 (not in the current room's list) is inserted into a descending table by key (an existing entry stays ahead when it is greater or equal). The first three are requested with byte 0 = 1, priority `200 - i` (0x279ba0..0x279bc8).
- 0x2b9cf8 then walks the global request list 0x375d70 (scripted `RoomStreamingControl` requests, 0x20f194) and requests each room with byte 0 = 1 and priority 0; bytes 1 and 2 become 1 when 0x2297b0(0) or 0x236104() returns non-zero (not evaluated).
- Applying a request (0x189c90, called by step 4): when byte 0 is not 0, the room is not loaded and not the root, a room in state 1 gets its resource priority updated (0x1c07f8) and a room in state 3 starts the loader through 0x223878 -> 0x24d808(state, async flag from 0x24d4cc, priority). 0x24d808 sets the state byte (+8) to 1 when the load is queued (0x24d9ec) and to 2 when it completes (0x24d9b0). The resource queue is an array at 0x52cf1c, sorted by priority.

**Replica and checks**
- `planRoomStreaming` returns the wanted set (neighbour list plus preloads plus scripted rooms), the load order (non-loaded neighbours with their priority, then preloads) and the anchor.
- Five readings of the oracle (positions, camera direction, current room, loaded set after 90 idle updates) all give wanted == loaded: `street-at-entrance` +1 update (Room7, 9 rooms), `wstream-room8-settled` (Room8, 11 rooms), `wstream-walk-b` (Room8, camera turned west, Room17 added), `wstream-walk-c` (Room8, 11 rooms), `wstream-walk-d` (Room7 again, Room16 and Room22 preloaded, 10 rooms). They are in `tests/room_streaming_real_test.cpp` with the numbers and the scenario of each state. The room positions are the 43 values read from the oracle (GDB, state `wstream-room8-settled`).
- Earlier remark corrected: the seven-room set of `street-at-entrance` in the 2026-10-14T11:13 entry was read before the two preloads (Room3, Room9) had finished loading; one update later the state holds nine rooms.
- Room node position vs. model data (parseModel, `models/lvl_01_cherrytreehills/roomN.bfnmdl`): for Room2, 3, 7, 8, 9 the node z equals the z of mesh 0's centre (1250.28, 1250.28, 1100.24, 1100.24, 1100.24) and the node x is the mesh-0 x plus 50.24 (849.76 vs 799.52); the node y (69.27, 70.10, 45.34, 27.41, 38.87) is not a mesh value. The source of the position is not recovered.

Unknowns:
- Where the room node position ([room+0x14]+0xa0) comes from; the x/z fit above is a data fit only, y is open.
- The meaning of `[0x5163e8+0x26]` (the whole ranking is skipped when it is 1) and of byte 1 and 2 of the request word (mode 2, 0xff): only the default and the neighbour values were seen.
- What the apply step does for flag byte 1 values 0 and 2 (hide and show: 0x245008, 0x245048, 0x23f124 calls) and which function releases the resource when a room goes to state 3; the state byte write for the unload was not located (watchpoints are unreliable).
- Whether the ranking uses a room's loaded state in the same update as its load completion (timing of the load: 9 updates for Room16, 7 to 13 for the preloads, async, depends on the host).
- The handling of the `lvl_05` records (54 records, world level hash 0, all lists empty) and of the `root` record in 0x173f94: the root of each scene is requested by 0x2af6b0, the effect of the records is not shown.
- The cases of a room with more than 32 neighbours (the tables hold 32 entries) were not checked.

<a id="ts-20261014-150748"></a>

## 2026-10-14T15:07:48+02:00 — Room node position from the room model, and room streaming in chase-game (llvm-objdump on `artifacts/game/game.arm.elf`, data probes with `chase_assets`; no emulator run)

Position of the room node ([room+0x14]+0xa0), the input of the ranking:
- Fit, checked for all 43 rooms of WorldLevel_01_CherryTreeHills against the 43 oracle values of the 2026-10-14T14:00 entry: the centre of the vertex bounding box (min and max over the vertex positions in model space) of every mesh of `models/lvl_01_cherrytreehills/roomN.bfnmdl` except mesh 0. All 129 values agree to under 0.01 (float rounding). Mesh 0 is the small marker mesh at the room's origin corner (half extents 1.25, y -48.76, a quad); including it, or using the mesh centres and half extents of the model header, does not fit (e.g. Room2 y 47.83 instead of 69.27). Replica: `roomBoxOf`, `roomBoxesOf` in `src/runtime/room_streaming.cpp`; the test `room-streaming-real-data` derives all 43 positions and compares them to the oracle table.
- [Corrects §2026-10-14T14:00:18](#ts-20261014-140018) (the room position remark: x/z from mesh 0, y open): the node is the box centre of mesh 1 and up, not mesh 0's centre plus an offset.
- The code that fills +0xa0 was not found; the fit is a stand-in (WSTREAM-S3).

Current room:
- Static writers of the current-room global 0x376c78 (literal pool scan of `code.bin` for the value and the stores through the loaded register): only 0x2e6354 (in the player-start loader 0x2e6264: the room of `Player1Start` from 0x213284 is stored) and 0x3035c0 (teardown, stores 0). What changes the room later (update 28 of walk1, Room7 to Room8) was not found; it may be written through a computed address. The port takes the room whose model box (x, z) contains the player (smallest area when several do, e.g. Room13 and Room43 are large); this reproduces the current room of all five oracle readings (WSTREAM-S4).

Port runtime (`src/runtime/room_streamer.{hpp,cpp}`, `src/game_level_rooms.cpp`):
- The level load settles the wanted set instantly around the start (the load screen; reproduces the 9 rooms of `street-at-entrance` +1 update and, from the default start in Room8, the 12 rooms read at gameplay-start in [streaming.md §2026-10-13T22:27](streaming.md#ts-20261013-222700)). Every update: the load flags finished 9 updates after their request become loaded, the room is chosen, `planRoomStreaming` runs on the loaded set, rooms that left the wanted set unload at once, new requests start loading. Replayed in `room-streaming-real-data`: the walk1 step (Room16 requested at update 1, loaded at update 10, the 28 to 37 of the trace) and the sets of `wstream-walk-b`, `-c`, `-d` reached in order from `wstream-room8-settled`.
- A not loaded room's model instance and the placed objects of that room are not drawn; the bound rule of the script runner (+0xb) counts only loaded rooms after the settle. Characters (`addLevelCharacters`) and studs of unloaded rooms are still drawn.
- The load time of 9 updates is the one measured for Room16 (7 to 13 for the preloads); all requested rooms load at the same time. What the original draws for a room in state 1 (nothing, so far as the scene list shows) was not measured.

Unknowns:
- The code that fills the room node position, and the writer of the current room after the level start.
- Load time per room and whether loads run in parallel or in priority order (the resource queue 0x52cf1c is sorted by priority).
- Whether characters and studs of unloaded rooms are hidden in the original.
