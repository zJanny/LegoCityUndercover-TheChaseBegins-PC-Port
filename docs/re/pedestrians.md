# Pedestrians: the pedestrian manager and `props/pedestriansystem`

The manager that spawns and removes the `chars/pedestrian` characters of a level, the `props/pedestriansystem` object (kind 198) and the spawn data of `Pregen/Pedestrians/<level>_Pedestrians.bin`. The pedestrian character itself (kind 197): its create, state step, walk and avoidance. Script native: EnableTrafficAndPedestrians ([script-natives-audio.md](script-natives-audio.md)). Traffic has its own manager (0x527410). Items: [pedestrians-traffic-pedestrians.md](../plans/features/pedestrians-traffic-pedestrians.md).

## Current state

- The pedestrian manager is 0x51b33c, not 0x527410 (0x527410 is the traffic manager). Its slots are the level objects `Pedestrian01` .. `Pedestrian20`, and its limit is the level parameter `Pedestrians_Max`. [§2026-10-20T11:17:42](#ts-20261020-111742)
- Update 0x2ee5e8: with bit 1 of +0xa4 clear, every live pedestrian is disabled (0x2428c0, no message). With bit 0 clear (no spawn data), nothing more happens. Otherwise, while live < limit <= slots, a countdown runs. At 0, one random slot is tried, and a spawn sets the countdown to 7 x live. At most one pedestrian is removed per update, which sets the countdown to 7 x (live - 1). [§2026-10-20T11:17:42](#ts-20261020-111742)
- Spawn 0x2ee068: a random spawn point of the spawn tree leaf at the player's position, in 11 tries. The point must lie in the camera's forward cone (dot >= 0.8) and within the fade distances of the pedestrian's tier (x 0.5 to x 1.5). It must also be at least 8 units (xz) from another pedestrian on the same path. The point's direction is kept or reversed 50/50, the speed is 2.0 + 1.5 x random, and the lateral offset is in [-0.25, 0.25). [§2026-10-20T11:17:42](#ts-20261020-111742)
- Bit 0 is the result of the pregen file load 0x2ee868, run by 0x2edc94 only when the level has a `props/pedestriansystem` (create 0x260d9c registers it at +0x94) with `PedestrianPath%03u` objects. [§2026-10-20T11:17:42](#ts-20261020-111742)
- Port: `src/runtime/pedestrian_manager.*` (test `pedestrian-manager`), not wired into the game. [§2026-10-20T11:17:42](#ts-20261020-111742)
- The pedestrian (kind 197): its update runs the walk 0x2b9138 in state 2 and then the state step 0x1d7e88 (entries of states 1 and 3 .. 7, the hold counter +0x10). The walk turns toward the target at dt x 5, moves along row 2 at speed x dt, advances the path parameter by 0.3 near the target, turns back by 0.75, and sidesteps by 1.3 for the avoidance 0x2edf08 (player or pedestrian within 2.0 and 1.0 to the side). The walk clip starts in mode 1. [§2026-10-20T12:20:49](#ts-20261020-122049)
- Port: `src/runtime/pedestrian_walk.*` (test `pedestrian-walk`), not wired. [§2026-10-20T12:20:49](#ts-20261020-122049)
- Open: the blocked tests 0x26cd38 and 0x1ffcd4, the message slot 0x19efbc (who requests the dodge and splat states), the root motion step of states 3, 4 and 7, the meaning of the node tests of the removal, the source of `Pedestrians_Max`, and the caller of the update.

<a id="ts-20261020-111742"></a>

## 2026-10-20T11:17:42+02:00 — Pedestrian manager 0x51b33c: level load, update, spawn and the pedestriansystem (llvm-objdump only, no REA session, no emulator)
Corrected by [pedestrians.md §2026-10-20T12:20:49](#ts-20261020-122049): 0x1918e0 is not the pedestrian's message handler (that is slot 0x14 0x19efbc), and the walk clip starts in mode 1; mode 6 is the removal's stop.

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

<a id="ts-20261020-122049"></a>

## 2026-10-20T12:20:49+02:00 — The pedestrian character (kind 197): create, state step, walk and avoidance (llvm-objdump only, no REA session, no emulator)

[Corrects §2026-10-20T11:17:42](#ts-20261020-111742) in two points. 0x1918e0 is not the pedestrian's message handler: it reads component +0x176 / +0x178, and the pedestrian's component has 100 bytes. The registration of kind 197 at 0x1786dc puts 0x19efbc in slot 0x14, the message slot. And the walk clip is started in mode 1 (loop). Mode 6 is what the stop 0x172754 of the manager's removal sets, which fits the mode 6 of the 14 lvl_01 pedestrians in [npc-animation.md](npc-animation.md) (not checked in that state).

Method: `llvm-objdump -d --triple=armv6k-none-eabi --mattr=+vfp2` on `artifacts/game/game.arm.elf`. Port: `src/runtime/pedestrian_walk.*` (`PedestrianWalk`), test `pedestrian-walk`.

**Create 0x1913f8.** It copies the 0x88-byte entity and allocates a 100-byte component (+0x7c). +0xc = the model 0x2442b4. Entity +0x5c = the length of entity +0x6c (0x2536a8; 0x242df4 fills +0x60 / +0x6c, not read). The clips (0x223a68) come from `models/char_minifig/<AnimationClass>/`; the `dodge_*01` clips come from `CLASS_Generic/` when the class has no `.bfnanm` of that name (0x21d758):
- +0x14 `walk_minifig`
- +0x18 `dodge_right01_minifig`, +0x1c `dodge_left01_minifig`, +0x20 `dodge_right02_minifig`, +0x24 `dodge_left02_minifig`
- +0x28 `bump01_minifig`, +0x2c `bump02_minifig`
- +0x30 `splat_minifig`, +0x34 `splat_up_minifig`

The four dodge clips and `splat_up` get bits 0xf of their word +8, the root motion bits of [objects.md](objects.md). Byte +0x61 = 4 when the parameter `IsMale` (default 1) is set, else 8. The state hwords +2 and +4 are 0. Post-link 0x191810 scales the model by 2.0 when 0x2369a8(6) is set, else by 1.0.

**Update, slot 0xc 0x19188c.** 0x2424d0 calls 0x277c24 only while state (+2) == requested (+4). Then the state step 0x1d7e88 runs. 0x277c24 switches on the state:
- 2: the walk 0x2b9138.
- 3, 4, 7: 0x277afc, the root motion of the current clip, then the wall test 0x2059ec and the ground probe 0x2058cc.
- 5: 0x1fd8c8 with the vector at 0x56d400, then the node position + that vector on a touch.
- 6: nothing.

**State step 0x1d7e88.** When state != requested, the entry of the requested state runs and the state becomes the requested one. Clip starts are 0x23afc0(entity, clip, mode, first 0, end 0xffff, speed 1.0, blend 0.0). "Root motion" below means 0x239568 on the new slot.
- 1: the target +0x38 = the path sample at +0x4c + 0.1 (forward +0x5c) or - 0.1, plus the lateral offset. The parameter itself stays. +0x10 = random % 50 + 10. Byte +0x62 = 1. Walk clip in mode 1.
- 3 / 4: +0x5e = 1 / 0, or 3 / 2 when byte +0x5f is 1. That dodge clip (+0x18 + 4 x +0x5e) in mode 0 with root motion.
- 5: +0x60 = random % 2. That bump clip in mode 0. The node rotation becomes the heading to the player: uxth(int(atan2(dx, dz) x 10430.378)), then 0x23deb4 (identity rows turned about y by heading x 9.58738e-5).
- 6: the splat clip in mode 1, +0x58 = 3.0.
- 7: `splat_up` in mode 0 with root motion.

Then, by state:
- 1 requests 2.
- 3, 4, 5 and 7 request 1 once their clip's kind 0x245f84 is 6 (finished).
- 6: when 0x278b04(player vehicle, entity) is true, +0x58 = 3.0. While +0x58 > 0 it drops by dt (0x23602c), and at 0 or below state 7 is requested.

Last comes the hold counter +0x10, which the manager's removal reads. When 0x2930e0(node) is true, 0x23de70 is false and state or requested is not 0, +0x10 = max(+0x10, 10). Otherwise it counts down to 0. Byte +0x5d counts down to 0.

**Walk 0x2b9138 (state 2).**
1. Turn: M = 0x22859c(from the node position to the target): row 2 = unit(target - position); row 0 = (z, 0, -x) of row 2, or (0, -z, y) when |row 2 y| > 0.98, normalised; row 1 = row 2 x row 0. The node's row 2 y and row 0 y become M's. Then rows 0 and 2 move toward M's by t = dt x 5.0 (0x221e5c, a + (b - a) t).
2. velocity = row 2 x (speed +0x54 x dt).
3. Ground probe flag. Byte 0x375d5c (cleared by the manager update) is clear: the flag is set, and so is the byte. Otherwise, with +0x5d != 0: the first angle of 0x247660 (atan2(-r2y, r2z), or its pair with +pi when that has the smaller length) is reduced to |a - pi| when |a| > pi / 2; the flag is set above 0.34906587 (20 degrees).
4. 0x1fd8c8(entity, position, velocity, 0, flag). When the player has a vehicle and the flag is clear, it returns 0 and does nothing else. Otherwise the pedestrian touches the player when the xz distance squared is below 1.5 (0x20af40). With the player's state hword +0x6c 2 or 3, the velocity's x and z then get + unit(position - player) x 0.1. With the flag set, the ground probe 0x2058cc runs: a ray from the position up and down by the floats at 0x374494 / 0x374498 (0 in the image, written at run time) sets the node y and the room.
5. A touch with player state 2 or 3 requests state 5. The ray 0x21ab20 from the position to (x, -1.0, z) then sets the node y on a hit.
6. Blocked: the first non-zero 0x1ffcd4(entity, position, velocity, dt) over the list at 0x5863b4 + 0x305c (count +0x3058). Objects whose +0x84 has component bit 1 of +0xcc are skipped.
7. position += velocity. The rows are rebuilt with up (0, 1, 0) (0x56d424): row 0 = unit(up x row 2), row 2 = unit(row 0 x up), row 1 = up.
8. Result r (blocked result first). When r is 0 and +0x5d is 0, r = the avoidance 0x2edf08.
   - r = 4: the direction flips. The parameter moves by +0.75 (new direction forward) or -0.75, and the target is sampled there.
   - r = 0 or 3 (and r = 0 while +0x5d != 0): only when the 3D distance from the position to the target is below entity +0x5c. Then r = 0 moves the parameter by +0.3 / -0.3 and samples the target there; r = 3 sidesteps with side -1.
   - r = 1 or 2: sidestep with side +1 for 1, else -1.
   - A sidestep sets target = position + row 2 x 1.3 + row 0 x (1.3 x side), and +0x5d = 30.
   - Wrap of the parameter: below 0 it becomes count + p; at count (hword path +0xa) or above it becomes p - count, for open paths as well.
9. With requested 3 or 4 (set during the update by a source not read), the node rows become 0x22859c(position, player position).

The path sample with offset: the type callback slot 0 of table 0x357e24 (20 bytes per type: type 0 0x1c17c0, type 1 0x1b33fc) is called as (path +8, target, tangent, 1, p). The target then gets x + (-tangent z) x lateral +0x48 and z + tangent x x lateral. 0x1c17c0 is the spline of `sampleDirectorPath` with the unit tangent of `detectivePathTangent`. With its fourth argument set and path +0x18 non-zero, it transforms both by the path's node (0x254250).

**Avoidance 0x2edf08(manager, entity).** The player first, then every live slot of the manager except the pedestrian itself, in slot order. d = other - position. When the xz length (0x20e968) is below 2.0 and the side s = d . row 0 in xz (0x221920) has |s| < 1.0, the result is 2 for s > 0, else 1. Otherwise the next one is tested, and 0 when none matches. There is no test of in front or behind.

**Message slot 0x19efbc**, read only at its start:
- 0x80000003: +0x10 = 0; in state 1 or with requested 1, state = requested = 0.
- 50: a distance test against the record of the table at 0x19f3f8 indexed by +0x61, then 0x23ae84.
- 0x80000001 and 252: not read.

Port: `PedestrianWalk` (`update`, `walk`, `avoidance`) on a `PedestrianBody` (the component fields, the node rows and position). The caller supplies dt, the path (`PedestrianPath`; `pedestrianLevelPath` for type 0), entity +0x5c, the player's position, vehicle and state, the other live pedestrians, byte 0x375d5c, 0x2930e0 / 0x23de70, 0x278b04, the blocked result 0x1ffcd4, the probe 0x2058cc, the ray 0x21ab20 and the clip finished test. Animation starts are returned as requests. The steady updates of states 3, 4, 5 and 7 are not run. Not wired into `chase-game`.

Unknowns:
- 0x1ffcd4 (the blocked results 1 .. 4, with a random % 2 at 0x1fff24), the floats 0x374494 / 0x374498 of the probe, the vector 0x56d400, 0x278b04 and 0x2930e0 (a node draw test with the distance fade tiers).
- 0x242df4 (the bounds behind entity +0x5c), whether any pedestrian path has a node at path +0x18, and type 1 0x1b33fc.
- The message slot 0x19efbc past its start: who requests states 3, 4 and 6 (vehicle hits, dodges). The root motion step 0x277afc of states 3, 4 and 7.
- 0x24ef70(position, 0x52a1b4) in the state step, whose result is not used.
