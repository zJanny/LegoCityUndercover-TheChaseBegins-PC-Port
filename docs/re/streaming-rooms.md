# Room streaming rule (which rooms are loaded)

The per-update rule that decides which rooms of a world level are requested, loaded and unloaded. Data model, room links and scene array: [streaming.md](streaming.md). Feature file: [world-streaming](../plans/features/world-streaming.md). Replica: `src/runtime/room_streaming.{hpp,cpp}`, test `room-streaming-real-data` (`tests/room_streaming_real_test.cpp`).

## Current state

- Room requests come from the scene room update 0x197794 each update: every dirty room gets the default request word (0x376c7c), 0x2af6b0 requests the current room's neighbour list (entries of 0x18 bytes, `01 01 00 80` for the room itself, `01 01 01 80` for the others), and 0x279834 ranks the rest. A room outside the list and not preloaded is unloaded at once. [§2026-10-14T14:00:18](#ts-20261014-140018)
- Ranking: key `dot(normalize(P - A), B)` (A player position, B camera look direction, P room node position); not-loaded neighbours get priority 255 or 254 minus rank; the anchor is the loaded non-current neighbour with the greatest key above 0, and the first three rooms of its list that are outside the current list are preloaded with priority `200 - i`. [§2026-10-14T14:00:18](#ts-20261014-140018)
- The room node position is the centre of the vertex box of meshes 1 and up (all 43 rooms of lvl_01); this corrects the mesh 0 reading. The code that fills it was not found (stand-in WSTREAM-S3). [§2026-10-14T15:07:48](#ts-20261014-150748)
- The current room (0x376c78) is written by 0x1f4244, called per update from 0x2e5aec with the room that owns the player's node (0x2234a8); the node moves between rooms in 0x2e6ce4 -> 0x237b04 -> 0x22d204 by the box rule (position lifted by 0.1, current list first, then scene array order, then the smallest vertical distance). This corrects the port's earlier smallest-containing-box rule; replica `roomAfterMove` matches all 160 recorded records. [§2026-10-14T20:51:26](#ts-20261014-205126)
- Loads take 6 to 12 updates per room in the oracle (loader is a separate thread); the port uses 9 updates for every room in parallel (stand-in WSTREAM-S30). [§2026-10-14T20:51:26](#ts-20261014-205126)
- The port runtime (`room_streamer`, `room_streaming`, `room_locator`) reproduces five oracle readings and the walk1 step. [§2026-10-14T15:07:48](#ts-20261014-150748)
- Objects of the player's room and of one round-robin entry of the current room's list change rooms by the same box rule. The port applies it to vehicles, which fixes the car that vanished when its placement room unloaded (VEH-56). [§2026-10-16T13:47:45](#ts-20261016-134745)
- Open: which object types the room lists hold, the writer of the room node position, the meaning of `[0x5163e8+0x26]` and `u16 [room+0x34]`, the unload and release path (0x234690), the loader's dequeue order, whether characters and studs of unloaded rooms are hidden.


<a id="ts-20261014-140018"></a>

## 2026-10-14T14:00:18+02:00 — The request rule: neighbour list plus three view-ranked preloads (REA not used for this entry; llvm-objdump `--mcpu=arm1176jzf-s` on `artifacts/game/game.arm.elf`, Azahar oracle New 3DS, GDB reads and `memtrace`, no RPC)

Corrected by [§2026-10-14T15:07:48](#ts-20261014-150748): the room node position is the centre of the vertex box of meshes 1 and up (all 43 rooms), not mesh 0's centre plus an offset.

[Corrects streaming-transitions.md §2026-10-14T11:13](streaming-transitions.md#ts-20261014-111312) (the "rank" remark and the seven-room reading). All addresses are for the reference build.

**Per-update chain (scene room update 0x197794)**
- 0x197794 runs per update over the scenes of the list at 0x589a18 (count `[0x589a18+8]`, pointers `+0xc`). Each scene has a dirty-room list at scene+0x132c (count) / scene+0x1330 (array); 0x205b5c(room, request) appends a room there when its new request word differs from its applied word.
- Step 1 (0x1977ac..0x197808): every room in the dirty lists gets the default request word at 0x376c7c, read in the oracle as bytes `00 02 ff 00` (the `0002ff00` that every unrequested room shows at +0x38 and +0x3c).
- Step 2 (0x197810): `blx [0x376c84]`; the global holds 0x279834 (read in the oracle). 0x279834 first calls 0x2af6b0, then runs the ranking below.
- Step 3 (0x19781c..0x197860): 0x1dd5c8 per dirty room. Step 4 (0x1978a0..0x197924): 0x189c90 (room apply) per dirty room, then the room is dropped from the dirty list when its applied word at +0x38 equals the default word.
- Room request layout (room +0x38 applied, +0x3c requested, each 4 bytes): byte 0 load wanted (0 or 1), bytes 1 and 2 (neighbour requests carry 1 and 0 for the current room or 1 for the other linked rooms; the default is 2 and 0xff; a preloaded room keeps 2 and 0xff, read as `01 02 ff c8` for Room3), byte 3 priority. The loader state byte is the state struct byte +8 (`[room+4]+8`: 1 loading, 2 loaded, 3 not loaded).

**0x2af6b0, neighbours of the current room**
- The current room (global 0x376c78) holds its neighbour list: count u16 at +0x30, array pointer at +0x40, entries of 0x18 bytes, resolved to the room object by 0x21b51c. The list is built from the section +0x34 record of the room (decoded: `Level::roomLinks`) by consumer 0x173f94, with the room itself as entry 0. In the oracle Room7's list is Room7, 1, 2, 6, 8, 14, 15 and Room8's is Room8, 1, 2, 3, 7, 9, 14, 15, 16; both equal the level data.
- Each entry carries a request word at +0x14: `01 01 00 80` for the room itself, `01 01 01 80` for the others. 0x2af6b0 copies it into the linked room's +0x3c..0x3f and appends the room to the dirty list. The same function sets the request of each scene's root object (scene+0x1328) to `01 01 00` plus its old priority byte, so the root of every scene is always wanted.
- Unloading is therefore the absence of a request: a room that is not in the current room's list and not preloaded keeps the default request, and the apply step drops it. Measured with `memtrace` (walk1: stick 20 150 from the save state at the police station entrance after mission 1-1): at update 28 the current room changes from Room7 to Room8; in the same update Room6 gets request 0 and state 3 (unloaded at once), Room16 is requested (state 1 for 9 updates, loaded at update 37).

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
- Five readings of the oracle (positions, camera direction, current room, loaded set after 90 idle updates) all give wanted == loaded: the save state at the police station entrance after mission 1-1 +1 update (Room7, 9 rooms), the save state in the street with room 8 settled (Room8, 11 rooms), the save state in the street after a first walk (Room8, camera turned west, Room17 added), the save state in the street after a second walk (Room8, 11 rooms), the save state in the street after a third walk (Room7 again, Room16 and Room22 preloaded, 10 rooms). They are in `tests/room_streaming_real_test.cpp` with the numbers and the scenario of each state. The room positions are the 43 values read from the oracle (GDB, save state in the street with room 8 settled).
- Earlier remark corrected: the seven-room set of the save state at the police station entrance after mission 1-1 in the 2026-10-14T11:13 entry was read before the two preloads (Room3, Room9) had finished loading; one update later the state holds nine rooms.
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
Corrected by [streaming-rooms.md §2026-10-14T20:51:26](#ts-20261014-205126): the current room is the room that owns the player's node; it changes with the player's position (0x1f4244, 0x22d204), and the port's room-containing-the-player rule is replaced by that rule.

Position of the room node ([room+0x14]+0xa0), the input of the ranking:
- Fit, checked for all 43 rooms of WorldLevel_01_CherryTreeHills against the 43 oracle values of the 2026-10-14T14:00 entry: the centre of the vertex bounding box (min and max over the vertex positions in model space) of every mesh of `models/lvl_01_cherrytreehills/roomN.bfnmdl` except mesh 0. All 129 values agree to under 0.01 (float rounding). Mesh 0 is the small marker mesh at the room's origin corner (half extents 1.25, y -48.76, a quad); including it, or using the mesh centres and half extents of the model header, does not fit (e.g. Room2 y 47.83 instead of 69.27). Replica: `roomBoxOf`, `roomBoxesOf` in `src/runtime/room_streaming.cpp`; the test `room-streaming-real-data` derives all 43 positions and compares them to the oracle table.
- [Corrects §2026-10-14T14:00:18](#ts-20261014-140018) (the room position remark: x/z from mesh 0, y open): the node is the box centre of mesh 1 and up, not mesh 0's centre plus an offset.
- The code that fills +0xa0 was not found; the fit is a stand-in (WSTREAM-S3).

Current room:
- Static writers of the current-room global 0x376c78 (literal pool scan of `code.bin` for the value and the stores through the loaded register): only 0x2e6354 (in the player-start loader 0x2e6264: the room of `Player1Start` from 0x213284 is stored) and 0x3035c0 (teardown, stores 0). What changes the room later (update 28 of walk1, Room7 to Room8) was not found; it may be written through a computed address. The port takes the room whose model box (x, z) contains the player (smallest area when several do, e.g. Room13 and Room43 are large); this reproduces the current room of all five oracle readings (WSTREAM-S4).

Port runtime (`src/runtime/room_streamer.{hpp,cpp}`, `src/game_level_rooms.cpp`):
- The level load settles the wanted set instantly around the start (the load screen; reproduces the 9 rooms of the save state at the police station entrance after mission 1-1 +1 update and, from the default start in Room8, the 12 rooms read at gameplay-start in [streaming.md §2026-10-13T22:27](streaming.md#ts-20261013-222700)). Every update: the load flags finished 9 updates after their request become loaded, the room is chosen, `planRoomStreaming` runs on the loaded set, rooms that left the wanted set unload at once, new requests start loading. Replayed in `room-streaming-real-data`: the walk1 step (Room16 requested at update 1, loaded at update 10, the 28 to 37 of the trace) and the sets of the save state in the street after a first walk, `-c`, `-d` reached in order from the save state in the street with room 8 settled.
- A not loaded room's model instance and the placed objects of that room are not drawn; the bound rule of the script runner (+0xb) counts only loaded rooms after the settle. Characters (`addLevelCharacters`) and studs of unloaded rooms are still drawn.
- The load time of 9 updates is the one measured for Room16 (7 to 13 for the preloads); all requested rooms load at the same time. What the original draws for a room in state 1 (nothing, so far as the scene list shows) was not measured.

Unknowns:
- The code that fills the room node position, and the writer of the current room after the level start.
- Load time per room and whether loads run in parallel or in priority order (the resource queue 0x52cf1c is sorted by priority).
- Whether characters and studs of unloaded rooms are hidden in the original.

<a id="ts-20261014-205126"></a>

## 2026-10-14T20:51:26+02:00 — The room change: who writes the current room, the rule, and measured load times (llvm-objdump `--triple=armv6k --mattr=+vfp2` on `artifacts/game/game.arm.elf`, Azahar oracle New 3DS with GDB watchpoints and stepped `memtrace`, no RPC, save state at the police station entrance after mission 1-1; REA was not used)

[Corrects §2026-10-14T15:07:48](#ts-20261014-150748) (current room: only 0x2e6354 and 0x3035c0 write 0x376c78 and the change after the level start was not found; the port used the room whose box contains the player, smallest area). The static scan missed the writer because it addresses the global through the base 0x376c74 with an offset of 4.

**The writer.** 0x1f4244(room, force) stores the current room at `[0x376c74 + 4]` (0x376c78): first the scene's default `[[scene+0x1320]]` (0x1f4280), then, when the given room is found in the scene's room array (count u16 `scene+0x131e`, array `[scene+0x1320]`; scene = `[room+0x28]`), the room itself (0x1f42b0). When the room equals the current one and `force` is 0 the store is skipped. Either way it then runs the scene room update 0x197794 at once (0x1f42c8, the request chain of [§2026-10-14T14:00:18](#ts-20261014-140018)), so the requests are applied in the update of the change; for a current room with a non-zero `u16 [+0x34]` it also logs a warning (0x168f10) when the room node's translation is longer than 10000. Callers: 0x2c36a8, 0x2e5aec, 0x2e663c, 0x2f68b4, 0x2faee4, 0x303630.
- Per update, at the end of the main update 0x2e5158 (0x2e5adc): 0x2234a8(`[[0x3772b8]+0x3c]`) returns the room that owns the player's node: for every scene of the list at 0x589a18 and every room of the scene, the room whose node (`room+0x14`, +0x78, +0x7c or +0x18) is the node or one of its ancestors (`node+4` is the parent). 0x2e5aec then calls 0x1f4244(that room, 0). The player's node is a child of the room node: in the save state at the police station entrance after mission 1-1 its parent is 0x088020c0, the node of Room7 (`[room+0x14]` of 0x0880cf40).
- So the current room changes when the player node is moved to another room's node. A watchpoint on the player node's parent field (node +4) stopped, in the update of the change, in 0x2e7070 (detach 0x245048, attach 0x245008 to `[newRoom+0x14]`, message 0x51 to the object through 0x244fc8, then 0x242b08), the final loop of 0x2e6ce4 (per update). The same update changed 0x376c78 (watchpoint stop in 0x1f4294, lr 0x2e5af0).

**The rule (0x2e6ce4 -> 0x237b04 -> 0x22d204).** 0x2e6ce4 handles per update two rooms: one entry of the current room's list in round-robin order (counter at `this+0x18c`, list count `[0x376c78]+0x30`, entry resolved by 0x21b51c) and the room of the player (`[[0x3772b8]+0x38]`). For an object of such a room (lists at `room+0x58+8*k`, k 0 to 3; types 90, 179, 183, 243 skipped) the position is the node's world translation plus the node's local box centre (`node+0xa0`, added by 0x23f50c; the player's is (0, 0.687523, 0.139909), read once). The object stays in its room while 0x1fa5d4(room, position) is true: the position lifted by 0.1 (`0x3dcccccd`, 0x1fa630) is inside the room's box (centre `node+0xa0`, half extents `node+0xac`, inclusive, in the room node's local space; the Room7 node matrix read in the oracle is the identity with translation 0, so the vertex box of the port is the same box). When it is not, 0x237b04(object) asks every scene in list order, 0x22d204(scene, position, first scene only):
1. (first scene only) the entries of the current room's list in list order, the room itself first: the first room whose box contains the lifted position.
2. When the current room's `u16 [+0x34]` is non-zero the result is none (the field is 0 in the 36 rooms read).
3. The rooms of the scene in array order: the first whose box contains the lifted position.
4. Otherwise, among the rooms whose x and z range strictly contains the unlifted position, the one with the smallest `|dy| - half y` (the first on a tie).

A result that is none or the object's own room changes nothing. The scene list has two scenes: 0x08501f30 without rooms (count 0) and 0x085abd20 with the 43 rooms of the street; the room array `[scene+0x1320]` = 0x0880a4a0 holds the rooms in ascending number order (Room7 is entry 6, the pointers ascend). Replica: `roomAfterMove` (`src/runtime/room_locator.cpp`); the player offset is the stand-in WSTREAM-S31.
- Check: three traces of the current room and the player node position per update (the save state at the police station entrance after mission 1-1, stick 20 150 for 28 updates, stick -150 0 afterwards; 80 + 35 + 45 records). `roomAfterMove`, fed the recorded positions in order, gives the recorded room in all 160 records, including the first change (Room7 to Room8 between x 899.7787 and 899.3618; Room7's box starts at x 899.53), the case where the position is inside both boxes and the room is kept (x 899.629: Room8 kept although Room7's box contains it too) and the change back (Room8 to Room7 at x 900.27, past Room8's high x 900.0). Pinned in `room-streaming-real-data`.

**Load times** (stepped `memtrace`, one record per update: the room's state byte `[room+0xa0+8]` and request word; 9 loads from three runs of the same walk; the loader is a separate thread, so the count differs between runs):

| Room | request to state 2 (updates) |
|---|---|
| 16 | 8, 9 (the earlier trace of agent O: 9) |
| 4 | 6, 6 |
| 10 | 12, 11 (requested together with Room4) |
| 6 | 11, 7 |

- Room4 and Room10 are requested in the same update and finish 6 and 11 to 12 updates later, which fits a loader that works one request at a time in priority order (about 6 each); Room16 and Room6 alone took 7 to 9. A load that is dropped from the wanted set while loading goes from state 1 to 3 at once (Room17 after 12 updates, Room22 after 11). No correlation with the model size was found (Room4 519 KB: 6, Room16 489 KB: 8 to 9, Room6 192 KB: 7 to 11). Every update added 5858 game clock ticks in the stepped run (the clock is clamped, so it does not measure the loader); how many guest milliseconds an update has in a free run was not measured. The port keeps 9 updates for every room, in parallel (stand-in WSTREAM-S30).
- The state byte is set to 2 by the worker (0x24d9b0 / 0x250538 after `blx [type+0x10]`). A function at 0x234690 with a halfword counter at `state+0x1a` (compared with `+0x18`), the queue 0x52cf1c and the type handler `[type+0x14]` looks like the release path that returns a resource to state 3 (WSTREAM-9); not verified.

Unknowns:
- The worker thread's dequeue and what its time per room depends on (queue 0x52cf1c sorted by priority, semaphore `[0x3765a4+0x10]`); the guest time of an update in a free run.
- What a non-zero `u16 [room+0x34]` is (all 36 rooms read have 0) and what the other object lists (k 1 to 3) and the table at 0x589410 (7 entries in the save state at the police station entrance after mission 1-1) move between rooms.
- Whether the player's box centre changes with the animation (one reading).
- The room order of other levels (only the street array was read).

<a id="ts-20261016-134745"></a>

## 2026-10-16T13:47:45+02:00 — Driven vehicles vanished when their placement room unloaded; vehicles now change rooms by the object rule (chase-game runs with temporary prints; no emulator, no REA, no new disassembly)

Cause of the invisible car (VEH-56, user report with the place from the F3 overlay: 895.82, 31.49, 958.11, heading -162.4, the car vanishes when driven forward and shows again when driven back; the police car and the donut van of mission 1-1):
- `chase-game` drew a placed object only while the room named in its placement record was loaded (`roomShown(roomNumber(placement.room))` in `LevelSession::draw`). `1-1_Police_Car` is placed in Room8, `1-1_DonutVan` in Room33. Run: `--call-script 1-1_VanBreakdown --call-script 1-1_Get_Car --start "886.0289 17.8243 1118.6516" --script "0 0 1500;0 0 3 X;0 0 330;at 895.82 31.49 958.11 -162.4;0 0 5;0 0 60 A"`. At the port's update 1879 the player switches from Room15 to Room21 and the loaded set becomes 14, 15, 16, 20, 21, 22, 26, 27, 28: Room8 is unloaded, so the car was hidden. Driving back into Room15 loads Room8 again, which matches the report.
- The original moves objects between rooms. Per [§2026-10-14T20:51:26](#ts-20261014-205126), 0x2e6ce4 tests every object of the player's room and of one round-robin entry of the current room's list. An object whose lifted test point (node translation plus node box centre) has left its own room's box goes to the room 0x237b04 / 0x22d204 picks. So a driven car follows the player from room to room.

Port change:
- `objectRoomAfterMove` (`src/runtime/room_locator.cpp`) is that rule for any object: the own room is kept while it contains the point; otherwise the current room and its list are searched, then the rooms in number order, then the smallest vertical gap; with no result the own room is kept. `roomAfterMove` (the player) is now this rule with own room = current room.
- `ObjectRoomMover` (`src/runtime/object_rooms.cpp`) runs the schedule: the player's room and one entry of the current room's list per update. The moves are applied after all tests, as the final loop of 0x2e6ce4 re-parents the nodes. It runs before the player's own room step in the same update.
- `chase-game` keeps a room per placed object (`placedRooms`). Only vehicles take part, and their test point is the vehicle matrix translation plus the centre of the model bounds (stand-in WSTREAM-S33). Vehicles whose placement room is not `roomN` (room -1, for example the traffic cars) are not moved and stay drawn as before.
- Result of the run above: `vehicle 1-1_Police_Car: room 8 -> 15 at update 1833` (the teleport) and `room 15 -> 21 at update 1879`, the same update as the player; the car stays drawn (screenshot at update 1928). Test points: (895.820007, 31.5676994, 958.029907) after the teleport, (892.770935, 32.3960686, 950.15509) at update 1878 (Room15) and (892.559814, 32.4092026, 949.58313) at update 1879 (Room21). Pinned in `room-streaming-real-data`, `object-rooms` (made-up rooms) and `game-vehicle-room`. In the mission 1-1 run the donut van moves from Room33 to Room15 at the teleport to the shop (update 2367).

Unknowns:
- Which object types are in the room lists (`room+0x58+8*k`; types 90, 179, 183, 243 are skipped) and which classes those types are; whether carried props, pushed props and characters change rooms the same way (WSTREAM-32).
- The vehicle node's box centre (`node+0xa0`) was not read; the model bounds centre is assumed (WSTREAM-S33).
- The start value and the step of the round-robin counter `this+0x18c` (WSTREAM-S32).
- Whether the player's room `[[0x3772b8]+0x38]` is the vehicle's room while the player is seated.
