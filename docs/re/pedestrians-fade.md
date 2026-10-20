# Pedestrians: the spawn fade-in and the mesh node alpha

Subtopic of [pedestrians.md](pedestrians.md): the start 0x275434 that a pedestrian spawn runs on its mesh node, the per-frame step 0x10eb70 that moves the node's alpha byte, the node set-up 0x10d2d4 and the run-time value of the node type *0x37674c. Items: [pedestrians-traffic-pedestrians.md](../plans/features/pedestrians-traffic-pedestrians.md).

## Current state

- A spawn on a node whose type is the mesh node type stores alpha +0xdd = 1 and step +0xdc = 0xff (a signed step of -1) on the node and its children (0x275434). The per-frame step 0x10eb70 then turns the step to +16 when the node is within the fade-in distance of its tier, and adds the step to the alpha each frame. A pedestrian spawned beyond the fade-in distance but inside the spawn window (tier 0: 36 .. 57 m) keeps the step -1 and drops to alpha 0. [§2026-10-20T14:12:57](#ts-20261020-141257)
- The mesh node type *0x37674c is 2, not 1: slot 0 of the node type table is reserved by 0x10332c before the first registration. Read from the code, not measured. [§2026-10-20T14:12:57](#ts-20261020-141257)
- Port: `src/runtime/node_fade.*` (0x275434, 0x10eb70) and `src/runtime/pedestrian_fade.*` (the mesh node of a pedestrian, its bounds and fades), glue `src/game_pedestrian_fade.*`; the instances are drawn with the node alpha. Tests `pedestrian-fade`, `pedestrian-fade-real-data`. [§2026-10-20T14:12:57](#ts-20261020-141257)
- Open: the park of an alpha 0 node under room +0x7c (room pass 0x2e6ce4), the source of the node radius +0x9c, the draw 0x10bcc8, the class byte +0x18 and room byte +0x3a at run time. [§2026-10-20T14:12:57](#ts-20261020-141257)

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
