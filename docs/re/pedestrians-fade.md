# Pedestrians: the spawn fade-in and the mesh node alpha

Subtopic of [pedestrians.md](pedestrians.md): the start 0x275434 that a pedestrian spawn runs on its mesh node, the per-frame step 0x10eb70 that moves the node's alpha byte, the node set-up 0x10d2d4 and the run-time value of the node type *0x37674c. Items: [pedestrians-traffic-pedestrians.md](../plans/features/pedestrians-traffic-pedestrians.md).

## Current state

- A spawn on a node whose type is the mesh node type stores alpha +0xdd = 1 and step +0xdc = 0xff (a signed step of -1) on the node and its children (0x275434). The per-frame step 0x10eb70 then turns the step to +16 when the node is within the fade-in distance of its tier, and adds the step to the alpha each frame. A pedestrian spawned beyond the fade-in distance but inside the spawn window (tier 0: 36 .. 57 m) keeps the step -1 and drops to alpha 0. [§2026-10-20T14:12:57](#ts-20261020-141257)
- The mesh node type *0x37674c is 2, not 1: slot 0 of the node type table is reserved by 0x10332c before the first registration. Read from the code, not measured. [§2026-10-20T14:12:57](#ts-20261020-141257)
- Port: `src/runtime/node_fade.*` (0x275434, 0x10eb70) and `src/runtime/pedestrian_fade.*` (the mesh node of a pedestrian, its bounds and fades), glue `src/game_pedestrian_fade.*`; the instances are drawn with the node alpha. Tests `pedestrian-fade`, `pedestrian-fade-real-data`. [§2026-10-20T14:12:57](#ts-20261020-141257)
- Measured at run time: the fade point 0x52a1b4 is the camera look point, which is the player position plus (0, 2.2, 0) with the follow camera on a street; a live pedestrian has class byte +0x18 = 0 and room byte +0x3a = 128 (0x80), as the traffic cars do. [§2026-10-25T13:18:33](#ts-20261025-131833)
- From the code: 0x52a1b4 is written only by 0x2ae1ac, called by the camera manager update 0x23f174 with the look point of the filtered pose C+0x4c (the pose eye C+0x70 when the pose flag 0x40000000 is set); the same function sets the jump byte 0x52a1cc. The third layer table push 0x17e544 is the enter virtual of the cutscene camera 0x3768e8; no direct call reaches 0x258eb0 before 0x102174, so *0x37674c = 2 holds. [§2026-10-25T14:19:23](#ts-20261025-141923)
- Measured in a vehicle camera (two cars) and on foot at a detective spot: 0x52a1b4 equals C+0x4c (never the eye C+0x70), the pose flag is clear, *0x37674c = 2 in every state, the class byte is 0 everywhere, and the entity byte +0x3a is a place value (128 .. 254) that is not the room type of the layer test. [§2026-10-25T14:35:54](#ts-20261025-143554)
- Measured: the room objects of live pedestrians in a street have type byte +0x3a 0 and 1 (class 0 active in the layer table, whose rows match the set-up table), while the entity's own byte +0x3a (128) is a separate field that the removal test does not read. [§2026-10-25T14:40:22](#ts-20261025-144022)
- Open: the park of an alpha 0 node under room +0x7c (room pass 0x2e6ce4), the source of the node radius +0x9c (the pass and blend of the node draw for an alpha below 255: [rendering-draw-order.md §2026-10-27T18:49:04](rendering-draw-order.md#ts-20261027-184904)), the fade point in a cutscene or scanner camera and which cameras set the pose flag 0x40000000, the per-frame refresh of the previous point +0x390, the room type in other levels, and the writer of the entity byte +0x3a. [§2026-10-25T14:40:22](#ts-20261025-144022)

<a id="ts-20261020-141257"></a>

## 2026-10-20T14:12:57+02:00 — The spawn fade-in 0x275434, the node step 0x10eb70 and *0x37674c (llvm-objdump only, no REA session, no emulator)

[Corrects pedestrians-inputs.md §2026-10-20T13:20:02](pedestrians-inputs.md#ts-20261020-132002): *0x37674c is 2, not 1.

Method: `llvm-objdump -d --triple=armv6k-none-eabi --mattr=+vfp2` on `artifacts/game/game.arm.elf`, read instruction by instruction; throwaway greps of the full disassembly for the pool words of 0x37674c / 0x376744 and the callers of 0x258eb0. Replica: `src/runtime/node_fade.*`, `src/runtime/pedestrian_fade.*`, tests `pedestrian-fade` and `pedestrian-fade-real-data`.

**0x275434(node, a, b).** When `node word & 31` equals *0x37674c, it stores byte +0xdd = 1 and byte +0xdc = 0xff, then calls itself with (a, b) (both unused) on the first child (+8) and each next sibling (+0xc). Another node type is left alone. The manager's spawn calls it on the pedestrian's node and on its children ([pedestrians.md §2026-10-20T11:17:42](pedestrians.md#ts-20261020-111742)).

**The step 0x10eb70(node, matrix).** It runs for a node when bit 2 of the word 0x529dc8 is set, bit 9 of the node word is set and bit 4 of byte +0xde is set; otherwise it returns. Bit 4 of +0xde is set by 0x10d2d4 at the node's creation and cleared by EnableDistanceFade. The order is:
1. Tier: bit 5 of +0xde gives tier 1. Otherwise the node radius +0x9c is compared with the words 0x52a1d0 / 0x52a1d4 / 0x52a1d8 (3, 6, 12): below the first 0, below the second 1, below the third 2, else the function returns without a change.
2. When byte 0x52a1cd is non-zero the step byte +0xdc becomes 16 and nothing is compared.
3. Otherwise s0 = (0x250da8 of node +0xa0 with the matrix argument, then 0x24ef70 against the point 0x52a1b4) minus radius squared (vmls). A step below 0 (signed byte) turns to 16 when the squared fade-in distance (0x52a1dc + 4 tier) is at least s0. A step of 0 or more turns to 240 (-16) when the squared fade-out distance (0x52a1e8 + 4 tier) is at most s0.
4. When byte 0x52a1cc is non-zero the alpha +0xdd becomes 0 for a step below 0 and 255 otherwise. Else it becomes the old alpha plus the signed step, clamped to 255 at the top and 0 at the bottom.

So the start values 1 and -1 mean: a node that is within the fade-in distance at its first step turns to +16 and fades in (1, 17, 33 .. 255 after 16 steps). A node beyond it keeps -1 and has alpha 0 after the first step. Its alpha 0 is what the room pass 0x2e6ce4 acts on ([pedestrians-inputs.md §2026-10-20T13:20:02](pedestrians-inputs.md#ts-20261020-132002), "Room +0x7c"): the node moves under room +0x7c and the manager removes the pedestrian. The spawn window of tier 0 is 36 x 0.5 = 18 .. 38 x 1.5 = 57 m from the view, so a spawn between 36 and 57 m is parked at once, and one under 36 m fades in.

**The node set-up 0x10d2d4 (the mesh node loop, objdump only for these stores).** `+0xdd = 255`, bit 4 of `+0xde` set, bit 9 of the node word set. Node +0x9c is the word at +8 of a 40-byte record (its address is r10 + 40 x index, with two pointers at +0 and +4), and node +0xa0 .. +0xb8 the six words at +0xc .. +0x24 of that record. The source of the record (the file mesh record has centre at +8 and half extents at +0x14, so the record is not the file's mesh) was not read.

**The mesh node type *0x37674c.** 0x103290 stores the result of 0x258eb0 at 0x376744 +8 = 0x37674c. 0x258eb0 takes the first slot of the 16-byte table 0x5303c4 whose byte 0 is 0 among the count [0x3767dc +4], else appends at the count. The start-up 0x102100 first calls 0x10332c (at 0x102174). That function, when its reference count [0x3767dc] is 0, sets the count to 1 and marks slot 0 as used (byte 1). Then 0x105b78 (0x102180) registers and gets 1, and 0x103290 (0x10218c) gets 2. 0x10395c (0x10221c, the shadowcam set-up) calls 0x105b78 again but its reference count is non-zero by then. The earlier reading of 1 missed the reserved slot 0. The calls of 0x10302c and 0x1035bc before 0x102174 were not followed to the end (0x10302c calls 0x105d8c, whose callees were not read); the earlier scan found no registration before 0x102174.

**The check in lvl_01 (chase-game offscreen, `--level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes`, a throwaway print of each live slot).** Pedestrians spawn within a few updates (up to five). Each live slot is enabled, has its body rig and a current clip, and stands 17 .. 50 m from the eye inside the view cone. Slots within 36 m reach alpha 255 and are drawn (one pedestrian is visible in the screenshot at update 120); slots beyond 36 m drop to alpha 0 and stay invisible until they walk inside 36 m, because the park and removal of 0x2e6ce4 is not ported (PED-21). One visibility fault was in the pedestrian path: the manager's node of each slot was an empty node (word 0), so its fade tier was -1 and the spawn window was 6 .. 162 m instead of 18 .. 57 m. The slots now hang on a mesh node ([pedestrians-traffic-pedestrians.md](../plans/features/pedestrians-traffic-pedestrians.md) PED-S11).

**The class byte +0x18.** The code has 97 `strb [rN, #0x18]` sites; none was matched to the entity of kind 197 (the writers that take a pedestrian entity were not identified), so the byte stays 0 as the create leaves it.

**Port.** `nodeFadeStart` and `nodeFadeStep` are 0x275434 and 0x10eb70 after the gates. `PedestrianFades` keeps the alpha and step of each slot, the local bounds centre and the radius. `chase-game` starts the fade after a spawn whose node has the mesh node type, steps every live slot after the update with the node's world centre (bounds centre through the node rows) and the camera eye as fade point, draws the draw test 0x2930e0 from the same inputs, and sets the body, head and hat instances to alpha / 255 with the premultiplied blended pass (PED-S12).

Unknowns:
- The writer and value of the point 0x52a1b4, the source of the node radius +0x9c and centre +0xa0 (the 40-byte record), and whether the matrix argument of 0x10eb70 is the node's world matrix (taken so; the camera eye stands in for 0x52a1b4, PED-S11).
- The node draw 0x10bcc8: the pass taken for alpha below 255 and its blend (PED-S12).
- The room pass 0x2e6ce4's park of an alpha 0 node and its return (PED-21), which decides the removal of a faded-out pedestrian.
- The writers of a pedestrian's class byte +0x18 and of the room byte +0x3a; the third table push 0x17e544; whether anything calls 0x258eb0 before 0x102174.

<a id="ts-20261025-131833"></a>

## 2026-10-25T13:18:33+01:00 — The fade point 0x52a1b4 and the live pedestrians' class and room bytes at run time (emulator, RPC reads only)

Provenance: 20261025-traffic-servant, idle jobs PED-13 / PED-16, recorded from a save state on a street with traffic after mission 1-1, 2026-10-25, RPC reads. The fade point was read after 3 loads of the state and 8 s of running, the pedestrian manager after 2 loads and 8 s; the loads agree.

**Fade point 0x52a1b4 (PED-13).** Read: floats (900.5492, 18.7672, 1110.2799) (raw little-endian words 0x44612326, 0x41962344, 0x448ac8f5 as read); the copy at 0x52a1c0 is identical. The player's world position is (900.549, 16.567, 1110.280), so the fade point is the player plus (0, 2.2, 0). The follow camera (K + 0x34 at 0x589aa4 is the pose at C + 0x24, with C = *0x376884) has its eye at (910.5491, 20.4672, 1110.2267) and its look point (C + 0x10) at (900.5492, 18.7672, 1110.2799), which equals the fade point exactly (the tracker at K + 0x94 plus K + 0xf4 gives the same look point). So whatever writes 0x52a1b4 copies the camera look point, not the eye. This differs from the stand-in that takes the camera eye as fade point ([pedestrians-traffic-pedestrians-fade.md](../plans/features/pedestrians-traffic-pedestrians-fade.md), PED-S11).

**Pedestrian manager 0x51b33c (PED-16).** Slot count +0x90 = 14, slot pointers from +0x40, limit +0xa0 = 5, flags +0xa4 = 3, countdown +0xa8 = 35 (changes). All 14 slot entities have type byte +0x12 = 197. On all 14 the class byte +0x18 is 0 and the room byte +0x3a is 128 (0x80), for the live ones and the disabled ones alike. The five live entities (the limit) have owner flags +0x10 = 0x0000 and node flags 0x84202 or 0xc4202; the disabled ones have owner flags 0x0001 (one 0x0041) and node flags 0xc4262 (one 0x84262). Live node positions (node +0x8c): (877.47, 16.60, 1101.68), (874.01, 16.81, 1129.77), (865.66, 16.80, 1118.41), (882.65, 16.74, 1096.36), (868.65, 16.80, 1106.34), all on the street in front of the player. The traffic cars have room byte 128 there too.

Unknown: the code that writes 0x52a1b4 and its value in other camera modes (a vehicle, a cutscene, the scanner); the room byte in other rooms and levels (only this state was measured); the third table push 0x17e544; *0x37674c was not read.

<a id="ts-20261025-141923"></a>

## 2026-10-25T14:19:23+01:00 — The writer of the fade point 0x52a1b4, the third layer table push and *0x37674c from code (llvm-objdump and REA call-path traces)

Provenance: 20261025-recorded-B, 2026-10-25. Method: `llvm-objdump -d --triple=armv6k-none-eabi --mattr=+vfp2` on `artifacts/game/game.arm.elf`, read instruction by instruction; REA (Ghidra 12.1.2 provider) `open_binary`, `trace_call_path` forward from 0x10302c, 0x1035bc and 0x25cf74 to 0x258eb0, `procedure_callers` of 0x25a1a0 and 0x25add8, `procedure_pseudo_code` of 0x10332c. Refines [§2026-10-25T13:18:33](#ts-20261025-131833) and [§2026-10-20T14:12:57](#ts-20261020-141257).

**Writer of 0x52a1b4 (PED-13).** The point 0x52a1b4 is 0x529e30 +0x384. The only function that writes it is 0x2ae1ac(vec3 p): it stores p's words 0 and 1 at +900 / +904 (`stm`) and word 2 at +908, then computes d = length(+912 - +900) (0x2507dc: vector difference, square root 0x25b578) and stores the byte +0x39c = 1 when 0.25 x [0x376794 +0x1c] (0.25 x 36 = 9) <= d, else 0. That byte is 0x52a1cc, the "jump to target" input of 0x10eb70. The only caller is the camera manager update 0x23f174 (call at 0x23f2b0), inside the branch where the current camera's type byte (manager byte +1) is non-zero, after the pose filter 0x1cf0b8 and the matrix build 0x2ca814 and before the shake offset is applied. The argument is C+0x4c, the filtered pose (its first vec3 is the look point), or C+0x70 (the pose eye, C+0x4c +0x24) when the word C+0x5c (pose flags +0x10) has bit 0x40000000 ("no rotation") set. C is the camera object at manager +0x14 (*0x376884). So the fade point is the final pose's look point for every camera whose pose flag is clear, and the unshaken eye for a pose with the flag. This matches the follow camera reading of [§2026-10-25T13:18:33](#ts-20261025-131833) (fade point = look point = player + (0, 2.2, 0)). The port's camera rig output (`CameraRig::lookAt`) is that pose: [camera-vehicle.md §2026-10-15T16:58:09](camera-vehicle.md#ts-20261015-165809) compares it with C+0x4c / C+0x70. Port: `pedestrianFadePoint` (`src/runtime/pedestrian_fade.*`), test `pedestrian-fade`.

**The point +0x390 (912).** 0x1077dc copies +900 to +912 (0x2554c0) only while the word +0x3c8 is 0; it is called from 0x25addc and 0x25c00c (the function 0x25add8, which has 16 callers). No other store to +912 was found with the immediate forms (`#912`, `#0x390`). Whether 0x25add8 runs once per frame, so that the jump byte means "the point moved 9 m or more since the last frame", is not read. The port keeps the jump byte false (PED-S16).

**Third layer table push 0x17e544 (PED-16).** 0x17e544 is the word at offset 0 (the enter virtual) of the camera object 0x3768e8, the cutscene camera ([camera-vehicle.md §2026-10-15T16:58:09](camera-vehicle.md#ts-20261015-165809)); the camera manager calls it when that camera becomes current (0x23f1f4 .. 0x23f240, `blx [camera +0]`). It sets bits of the byte 0x56d550 +8 when the byte 0x3768bc is non-zero, then loads the table pointer at [[0x3768c0] +0x28] and, when non-zero, tail-calls 0x1b3ebc, the table push (stack link in t +0, current = t, byte +0x3a of the listed room entries set to 255). There is no direct `bl` to it; the only reference is the word at 0x3768e8.

**Registrations of 0x258eb0 before 0x102174 (PED-16).** The direct callers of 0x258eb0 are 0x103160, 0x103238, 0x103290, 0x105b78 and the function around 0x1ed54c. The REA forward call-path traces from 0x10302c (299 functions visited, not truncated), 0x1035bc and 0x25cf74 (one function each) do not reach 0x258eb0 (direct calls only; indirect calls are not covered). 0x10332c (pseudo code read) reserves slot 0 when its count 0x3767dc is 0. So the first registration (0x105b78 at 0x102180) gets 1 and 0x103290 (0x10218c) gets 2: *0x37674c = 2 holds for the direct calls.

**Room byte +0x3a (PED-16).** The 21 `strb [rN, #0x3a]` sites were listed; the stored value was checked at nine of them. The table pushes 0x1b3f18 and 0x1a4c04 store 255 into the rooms' entries; the others write other record types or single bits. No site stores 0x80 into a pedestrian entity, so the writer of the measured 128 is not found.

Unknowns:
- The pose flag 0x40000000 at C+0x5c: which cameras set it (the port's rig has no such flag, PED-27).
- Whether 0x25add8 runs every frame, and so the meaning of the jump byte 0x52a1cc (PED-28).
- The writer of the measured room byte 128 in a pedestrian and a traffic entity (PED-29).
- The fade point in a vehicle camera and in a cutscene (servant request B-1) and the room byte in a second place (B-2): pending when this entry was written.

<a id="ts-20261025-143554"></a>

## 2026-10-25T14:35:54+01:00 — The fade point in a vehicle camera, *0x37674c and the entity bytes in other places (emulator, RPC reads only)

Provenance: requests 20261025-recorded-B-1 and 20261025-recorded-B-2, measured by 20261025-recorded-servant on 2026-10-25 from saves of the user and of earlier agents, `tools/azahar_oracle.py --rpc` reads 2 s (B-1) or 8 s (B-2) after the load. Camera object C = *0x376884. Checks [§2026-10-25T14:19:23](#ts-20261025-141923).

**Fade point (PED-13).** In every case 0x52a1b4 and its copy 0x52a1c0 equal C+0x4c exactly (all three raw words) and never C+0x70; the pose flag word C+0x5c is 0 (bit 30 clear), so the look point branch of 0x2ae1ac holds. *0x37674c = 2, bytes 0x52a1cc and 0x52a1cd = 0 in all states.
- Player driving a van (vehicle camera, two loads agree): 0x52a1b4 = (613.3692, 20.2841, 1062.5164) = C+0x4c = C+0x10. The eye C+0x70 is (616.0266, 22.7841, 1047.9569) on load 1 and (616.0643, 22.7841, 1047.9636) on load 2, so the eye differs between loads and the fade point does not. Player (613.3702, 17.0719, 1062.5145): the fade point is the player plus (0, 3.2122, 0).
- Player seated in a police car (vehicle camera, two loads, camera not settled after 2 s): 0x52a1b4 = C+0x4c in both loads, (887.7117, 18.7847, 1117.8595) and (887.0251, 20.1932, 1116.963); the eye is elsewhere. Only the equality is confirmed.
- On foot at a detective spot with its marker (follow camera, one load, before and after holding the action button for 0.5 s, no scanner view started): 0x52a1b4 = C+0x4c = (719.8959, 39.0327, 656.9098), player (719.8958, 37.0327, 656.9097): the fade point is the player plus (0, 2.0, 0); eye C+0x70 = (715.8723, 45.0577, 644.2371).
- The lift over the player is 2.2 on the street (earlier reading), 3.2122 in the van and 2.0 at the detective spot: it is the camera's look point, not a fixed offset. A cutscene or scanner camera was not reached.

**Entity bytes +0x18 and +0x3a, *0x37674c (PED-16).** *0x37674c = 2 in every state, including a police-station lobby. Class byte +0x18 = 0 for every pedestrian, traffic car and the player; no state with another class existed. Byte +0x3a of the slot entities of the pedestrian manager 0x51b33c and the traffic manager 0x527410 (one load per place):
- Police station lobby (interior, on foot): 0 pedestrian slots, 0 cars; the player's byte is 254.
- Street at the construction site in front of the mission 1-3 key (player byte 165): 14 pedestrian slots (type 197), all disabled, byte 165 on all; 10 cars (type 183), byte 165 on all, 3 live.
- Street at the detective spot (player byte 166): 14 pedestrians, byte 166 on 12 and 165 on 2 (placed in the construction site area); 10 cars, byte 166 on all, 4 live.
- Street where the van was (player byte 128, 8 s): 14 pedestrians, byte 128 on 10 and 129 on 4, 4 live; 10 cars, byte 128 on 6 and 129 on 4, 5 live. Entities carry the value of where they were last placed.

Unknowns:
- The room object's byte +0x3a (entity +0x38) of a live pedestrian, and the run-time layer table in a street: measured in [§2026-10-25T14:40:22](#ts-20261025-144022).
- The fade point in a cutscene, scripted or scanner camera, and the cameras that set the pose flag 0x40000000.
- The writer of the entity byte +0x3a.

<a id="ts-20261025-144022"></a>

## 2026-10-25T14:40:22+01:00 — The room object and the layer table of live pedestrians in a street (emulator, RPC reads only)

Follows [§2026-10-25T14:35:54](#ts-20261025-143554): the removal test reads the room object at entity +0x38, not the entity byte +0x3a.

Provenance: request 20261025-recorded-B-3, measured by 20261025-recorded-servant on 2026-10-25, recorded from a save state on a street with the player in a van, `tools/azahar_oracle.py --rpc` reads 8 s after the load; two loads agree (pedestrian positions differ by 0.1 m).

**Layer table.** *0x376ad4 +4 holds 17 hwords per class (stride 34 bytes); 0x1ffbd8(class, type) reads the hword at class x 34 + min(type, 16) x 2 and returns bit 1. Class 0 row: 0x3 for types 0 .. 15, 0x0 for 16 (bit 1 clear for 16 and up). Class 1 row: 0x3 x 16, then 0x2 (bit 1 set). Class 2 row: 0x2 x 16, then 0x0. These are the values of the set-up table in `startupRoomLayers`.

**Room objects.** Four live pedestrians (owner flags 0, class byte +0x18 = 0, entity byte +0x3a = 128): three hang on the room object at entity +0x38 that has type hword +0x12 = 2176, byte +0x3a = 0 and byte +0x18 = 224; the fourth on a different room object of the same level (the index +0x14 differs, 0x25a0 and 0x2740) with byte +0x3a = 1 and byte +0x18 = 128. Both room objects have the same name hash (+8) and a +0x7c node of their own (node word 0x00080060). So the room type that 0x1ffbd8 gets is 0 or 1 (class 0: active), while the entity's own byte +0x3a (128, the place value of [§2026-10-25T14:35:54](#ts-20261025-143554)) is a separate field that the removal test does not use. Room +0x7c is a node other than the +0x14 index and than the pedestrian's parent, as the port supplies it.

Unknowns:
- The room type in the other levels and in the rooms not visible from this street (two rooms of one level measured).
- The writer of the entity byte +0x3a and what reads it.
