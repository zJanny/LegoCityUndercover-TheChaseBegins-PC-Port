# Pedestrians: the pedestrian manager and `props/pedestriansystem`

The manager that spawns and removes the `chars/pedestrian` characters of a level, the `props/pedestriansystem` object (kind 198) and the spawn data of `Pregen/Pedestrians/<level>_Pedestrians.bin`. The pedestrian character itself (kind 197) and its walk are only started here. Script native: EnableTrafficAndPedestrians ([script-natives-audio.md](script-natives-audio.md)). Traffic has its own manager (0x527410). Items: [pedestrians-traffic-pedestrians.md](../plans/features/pedestrians-traffic-pedestrians.md).

## Current state

- The pedestrian manager is 0x51b33c, not 0x527410 (0x527410 is the traffic manager). Its slots are the level objects `Pedestrian01` .. `Pedestrian20`, and its limit is the level parameter `Pedestrians_Max`. [§2026-10-20T11:17:42](#ts-20261020-111742)
- Update 0x2ee5e8: with bit 1 of +0xa4 clear, every live pedestrian is disabled (0x2428c0, no message). With bit 0 clear (no spawn data), nothing more happens. Otherwise, while live < limit <= slots, a countdown runs. At 0, one random slot is tried, and a spawn sets the countdown to 7 x live. At most one pedestrian is removed per update, which sets the countdown to 7 x (live - 1). [§2026-10-20T11:17:42](#ts-20261020-111742)
- Spawn 0x2ee068: a random spawn point of the spawn tree leaf at the player's position, in 11 tries. The point must lie in the camera's forward cone (dot >= 0.8) and within the fade distances of the pedestrian's tier (x 0.5 to x 1.5). It must also be at least 8 units (xz) from another pedestrian on the same path. The point's direction is kept or reversed 50/50, the speed is 2.0 + 1.5 x random, and the lateral offset is in [-0.25, 0.25). [§2026-10-20T11:17:42](#ts-20261020-111742)
- Bit 0 is the result of the pregen file load 0x2ee868, run by 0x2edc94 only when the level has a `props/pedestriansystem` (create 0x260d9c registers it at +0x94) with `PedestrianPath%03u` objects. [§2026-10-20T11:17:42](#ts-20261020-111742)
- Port: `src/runtime/pedestrian_manager.*` (test `pedestrian-manager`), not wired into the game. [§2026-10-20T11:17:42](#ts-20261020-111742)
- Open: the pedestrian's own states and walk (kind 197), the blocked test 0x26cd38, the meaning of the node tests of the removal, the source of `Pedestrians_Max`, and the caller of the update.

<a id="ts-20261020-111742"></a>

## 2026-10-20T11:17:42+02:00 — Pedestrian manager 0x51b33c: level load, update, spawn and the pedestriansystem (llvm-objdump only, no REA session, no emulator)

[Corrects script-natives-audio.md §2026-10-15T15:10:38](script-natives-audio.md#ts-20261015-151038). That entry has the two managers the wrong way round. 0x51b33c (flag byte 0x51b3e0, enable 0x2b476c, init 0x2ee4b4, update 0x2ee5e8) is the **pedestrian** manager. 0x527410 (flag byte 0x5277a0, enable 0x29f09c, init 0x2dfa48, update 0x2dfe88) is the **traffic** manager. Evidence: the level load of 0x51b33c (0x2ee4c8) looks up "Pedestrian%02u" (string 0x2ee558) and "Pedestrians_Max" (0x2ee568). Its pregen loader 0x2ee868 opens "Pregen/Pedestrians/%s_Pedestrians.bin" (0x2eeaa0). The level load of 0x527410 (0x2dfa78) looks up "Traffic%02u" (0x2dfc4c), "Traffic_MaxVehicles" (0x2dfc58) and "Traffic_FadeBound%02u" (0x2dfc6c), and its loader 0x2e02d4 opens "Pregen/Traffic/%s_Traffic.bin" (0x2e0560). In lvl_01 `Traffic01` .. `Traffic10` are `vehicles/*` objects and `Pedestrian02` .. `Pedestrian15` are `chars/pedestrian` (14 of them, with one `props/pedestriansystem` named `Pedestrians`, all in room7; read with `tools/chase_native.py level_objects`). The message 0x80000006 sent before 0x2428c0 belongs to the traffic update. The pedestrian update sends no message. The layout the old entry gives for the traffic manager (count +0x90, limit +0xa0, countdown +0xa8) is the pedestrian manager's.

Method: `llvm-objdump -d --triple=armv6k-none-eabi --mattr=+vfp2` on `artifacts/game/game.arm.elf`. Replica: `src/runtime/pedestrian_manager.*` (`PedestrianManager`), test `pedestrian-manager`.

**Manager object 0x51b33c.** Its static constructor is 0x34c14c, with the vtable 0x362850 and the name "PedestrianSystem" (0x34c194). It sets +0x1c and +0x20 to -1 and the floats 0x375d60 to {40.0, 6.0}, whose readers were not traced. Vtable slots: +0x18 init 0x2ee4b4, +0x1c level load 0x2ee4c8, +0x24 0x2ee5d8 (+0x94 = 0), +0x48 update 0x2ee5e8. Fields:
- +0x24: the root of the spawn tree, with hword size +0x24 and centre +0x28 / +0x2c (0x2edc94).
- +0x38 / +0x3c: the spawn records and their count (0x2ee868).
- +0x40: the slot list (20 entity pointers) and +0x90 its count.
- +0x94: the pedestriansystem entity. +0x98 / +0x9c: the path objects and their count.
- +0xa0: the limit. +0xa4: flags (bit 0 spawn data loaded, bit 1 enabled). +0xa8: the countdown.

**Init 0x2ee4b4.** Clears bits 0 and 1 of +0xa4 and sets bit 1.

**Enable 0x2b476c(flag).** Replaces bit 1 with the flag.

**Level load 0x2ee4c8(manager, level).** For i = 1 .. 20 it builds "Pedestrian%02u" and finds the object by name (0x23b350). A found object is appended to +0x40 (count +0x90) and disabled (0x2428c0). Then +0xa0 = 0x2451d0(0x23c188(level), "Pedestrians_Max", 5, 0). Which object 0x23c188 returns, and so where the parameter lives, was not read: the lvl_01 `Pedestrians` object has only two parameters.

**Kind 198 (`props/pedestriansystem`).** The registration at 0x1c8650 gives create 0x260d9c and slot 8 0x22f0a0. The create copies the 0x88-byte entity, allocates a 600-byte component (+0x7c) and stores the entity at manager +0x94. Step 0x2edc94 runs when +0x94 is set:
- For i = 1 .. 128 it finds "PedestrianPath%03u" (string 0x358a3c) in the scenes of 0x589a18 with 0x22a3b0, and appends it to the component (count +0x250). +0x98 / +0x9c point at that list.
- With at least one path, it bounds all path points (path +0xa hword count, +0xc 12-byte points) in x and z. It stores the half extent (u32 of the larger one) at +0x24 and the centre at +0x28.
- Then it calls 0x2ee868 and sets bit 0 of +0xa4 to its result.

**Pregen loader 0x2ee868.** It opens "Pregen/Pedestrians/%s_Pedestrians.bin" with the level name of 0x375914 (index 0x5163e8+0x26). A missing file returns 0. It reads the count (+0x3c) and that many 40-byte records (+0x38): +0 position (12 bytes), +0x18 (4), +0x1c (4), +0x20 (4). For each record:
- It resolves the path +0x18 from +0x1c in each scene (0x1b3d04).
- It computes +0xc from the path object's type callback (table 0x358a14, argument +0x20).
- It stores the room 0x239838(record) at +0x24.
- Then 0x1eb254 builds the spawn tree at +0x24 from the rest of the file.
The record decoder belongs to the pregen file format, not to this entry.

**Spawn tree lookup 0x1eb490(manager, node, position).** A node is 20 bytes: hword +0 size, hword +2 count, +4 x, +8 z, +0xc array of record pointers, +0x10 four children. A node with count != 0 is the leaf and is returned. Otherwise, with no children, the result is null. Otherwise the lookup goes into one of the four children by comparing `x + size` and `z + size` of children 0 and 1 (and 2) with the position.

**Update 0x2ee5e8(manager)** (byte 0x375d5c is cleared first):
1. Bit 1 clear: each listed entity whose flag hword +0x10 bit 0 is clear (live) is disabled with 0x2428c0; return.
2. live = the number of listed entities with bit 0 clear. Bit 0 clear: return (no removal pass either).
3. When limit > live and limit <= count: countdown = countdown - 1 if that is above 0, else 0 (signed). At 0: slot = 0x235498(count) (random word % count). If that entity is disabled and 0x2ee068(manager, slot) succeeds, live + 1, and the countdown becomes 7 x live when it is 0, or larger than 7 x live (signed).
4. Removal pass, over each slot in order. Skip it when it is disabled, when its component hword +4 (requested state) is 1, or when hword +2 (state) is 1. Let room = entity +0x38. The room counts as active when it is non-null, 0x2284bc(room) (room +4 -> byte +8 == 2 and +0x14 != 0) and 0x1ffbd8(entity byte +0x18, room byte +0x3a) (bit 1 of the activity table 0x376ad4 entry [class x 17 + min(type, 16)]).
   - In an active room: a component word +0x10 != 0 keeps it.
   - Otherwise it is removed when node +4 == room +0x7c, or when 0x23de70(entity) is true (no node, the parent node with bits 17 and 18 set, or the node with bit 18 set), or when both hwords +4 and +2 are 0.
   - In a room that is not active, it is always removed.
   - Removal: the entity's animation player (entity +0x44) gets all slots stopped (0x172754: 0x24f8fc and mode 6), then 0x2428c0. The countdown becomes 7 x (live - 1) when it is 0, or larger than 7 x (live - 1) (signed), and the update returns, so at most one removal per update.

**Spawn 0x2ee068(manager, slot).**
- The view is the matrix of camera 0x22e1f4(0) (0x2429d0: row 2 at +0x20, position at +0x30). When the player entity *0x3772b8 is valid (0x245b00) and has a vehicle (0x24255c) whose speed 0x34163c(vehicle +0x84) is above 0, the position moves forward by row 2 x speed x 0.5.
- leaf = 0x1eb490(manager, +0x24, player node position). Null: return 0, before any random draw.
- forward = (0x235498(20) >= 10). lateral = (0x2345dc() - 0.5) x 0.5, where 0x2345dc is the low 16 bits of the next random word / 65536.
- Up to 11 tries. Each draws k = 0x235498(leaf count), takes record = leaf +0xc [k], and retries (try + 1) when any test fails, in this order:
  - record +0x18 (the path) is 0.
  - The room record +0x24 is not active (the same three tests as the removal, with the pedestrian's class byte).
  - min > distance or distance > max. Here d = position - view, distance = 0x257ea0(d) (length, with d normalized; below 2^-23 d becomes (0, 0, 1)), tier = 0x28781c(pedestrian node), min = word(0x52a1dc + 4 tier) x 0.5 and max = word(0x52a1e8 + 4 tier) x 1.5. Those are the distance-fade tables of [script-natives-world.md](script-natives-world.md) (radius 3/6/12, fade-in 36/72/108, fade-out 38/76/114, from 0x1077dc: the record 0x376794 +0x24 / +0x1c / +0x20 times 1, 2, 4 or 3). Tier -1 reads the words before each table.
  - dot(d, row 2) is below 0.8 (compared as signed bit patterns).
  - 0x26cd38(pedestrian, position, zero vector, 0.0) is non-zero: a loop over the list at 0x5863b4 + 0x305c with 0x1ffcd4, not read.
  - Another live slot whose component +0x44 equals the record's path lies within xz distance squared 64.0 (0x20af40).
- Success:
  - When (node word & 31) == *0x37674c, node byte +0xdd = 1 and +0xdc = 0xff, and every child node gets 0x275434(child, 1, -1).
  - The component gets: hword +4 = 1 (state 1 requested), hword +2 = 0, +0x44 = path, byte +0x5c = forward, +0x10 = 0, float +0x54 = 2.0 + 0x2345dc() x 1.5, float +0x48 = lateral, byte +0x5d = 0, +0x4c = record +0x20.
  - The node matrix becomes identity, with position = record +0 and row 2 = record +0xc (times -1.0 when not forward). Then 0x237a0c sets row 0 from row 2 ((z, 0, -x) when |y| <= 0.98, else (0, -z, y)), normalized, and row 1 = row 2 x row 0 (0x23b960).
  - Then 0x24c118 sets the node, 0x245048 runs when the node has a parent, and 0x245008(room +0x14, node) attaches it to the room. Bit 6 of the entity flags is cleared, the entity is enabled (0x242cf0), and the spawn returns 1.

**Pedestrian character (kind 197), read only as far as the manager needs it.** The registration at 0x1786dc gives create 0x1913f8, slot 8 0x19ef90 (it frees the component), slot 0xc 0x19188c, slot 0x14 0x19efbc and post-link 0x191810. Slot 0xc calls 0x2424d0 with the callbacks 0x277c24, 0x281fc0 and 0x1c5448, then 0x1d7e88. The function after it, 0x1918e0 (component +0x7c, which slot it fills was not checked), compares the message with 3, 0x52, 0x57 and 0xfc. Message 3 in state 3 (hword +2), with speed +0x54 >= 2.0 and further tests on the argument, requests state 4 (hword +4 = 4). 0x277c24 switches on the state hword +2 (2 .. 7). The state functions, the walk along the path, and the walk animation (mode 6 in [npc-animation.md](npc-animation.md)) were not read.

**Port.** `PedestrianManager` keeps the slot fields the manager reads and writes (live flag, state, requested state, +0x10, path, direction, lateral offset, speed, path parameter, placement). It takes everything else from the caller: the leaf's spawn points with their room test, the camera rows and the vehicle speed, the pedestrian's fade tier, the blocked test, the room, node-on-room-root and hidden tests, and the fade-node test. `DistanceFadeTable` defaults are the oracle values. The removal reports whether the animations are stopped first. Nothing is wired into `chase-game`.

Unknowns:
- Which object 0x23c188(level) returns for `Pedestrians_Max` (the limit of lvl_01 is not known), the initial countdown +0xa8 (no store besides the update was found), the caller of vtable slot +0x48, and byte 0x375d5c.
- 0x26cd38 / 0x1ffcd4 (the blocked test), the meaning of node +4 == room +0x7c and of the node bits that 0x23de70 tests, the entity class byte +0x18 of a pedestrian, and the node bits that give a pedestrian its fade tier (0x28781c).
- The pedestrian's states and walk (0x277c24, 0x281fc0, 0x1c5448, the state update of state 1), what reads +0x48 / +0x4c / +0x54 / +0x5c, 0x2edf08 (reached from 0x2b3fbc with an entity; distance tests against the player and the other pedestrians), and the floats 0x375d60.
- The spawn tree builder 0x1eb254 and the record decoder belong to the pregen file format.
