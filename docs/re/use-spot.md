# The use spot (`props/prop_simpleuse`)

A zone object that fires one `TriggerObject` when the player presses the action button inside it (the dog cage trigger of mission 1-2), and what is known about the other mission 1-2 trigger that nothing fires. Use zones in general: [carrying.md](carrying.md), [buildit.md](buildit.md). Items: [script-system-missions.md](../plans/features/script-system-missions.md) (SCRIPT-111).

## Current state

- The class (`props/prop_simpleuse`, 32-byte component) has the parameters `TriggerObject` (record +0x18) and `SendPlayerObject` (+0x1c); its zone is the class bound `UseBound` (centre 0, half 2 2 2) registered by 0x23e130 with `UseStartActive` default 1. [§2026-10-18T14:28:33+02:00](#ts-20261018-142833)
- Message 3 with the action flag fires 0x244f40 on the target, asks the player for state 1 and switches the zone off (0x236624); message 8 switches it on again. Confirmed in the oracle from `m12-13-cage-trigger`. [§2026-10-18T14:28:33+02:00](#ts-20261018-142833)
- A use marker (the spinning `useGeneric` disc, 2 m across, 0.1 above the ground) shows at each usable zone of 36 entity kinds within 25 m of the camera or player, fading in and out over 0.15 s; ported for use spots. [§2026-10-19T09:25:34](#ts-20261019-092534)
- After the release the original goes black (FadeScreen 0) and the cutscene mode of `1_2_c` fades the screen in again; `chase-game` now does the same and continues to `1-2_MissionEnd` and `1-3_MissionStart`. [§2026-10-19T09:12:53](#ts-20261019-091253)
- `1-2_Door_Lock_Hint_Script` (the scriptobject) is referenced by no object and no script call; the hint dialogue runs from the bound `1-2_Door_Lock_Hint` (BoundEntered), which the port already delivers. [§2026-10-18T14:28:33+02:00](#ts-20261018-142833)
- The cat rescue spot (kind 124, 4 in lvl_01) keeps its cat sitting: state 1 plays `idle_sat` in a loop; states 4, 6, 7 (`rescuecat_jump`, `rescuecat_jump_idle`, `rescuecat_rescue`) and 8 (cat and spot disabled) follow requests written by the player's rescue code, which needs the fireman ability 30 and is not ported. [§2026-10-19T12:58:16+02:00](#ts-20261019-125816)
- No name source for the scriptobject exists besides the level's own trigger records, the detective spot next to it has no `TriggerObject`, and a trap from `m12-7` saw no object trigger; code that finds the entity without a name is not excluded. [scripts-missions.md §2026-10-18T19:58:54](scripts-missions.md#ts-20261018-195854)

<a id="ts-20261018-142833"></a>

## 2026-10-18T14:28:33+02:00 — prop_simpleuse handler 0x1922e8 and the Door_Lock_Hint_Script trigger (REA on game.arm.elf, Azahar GDB trace without RPC, level data through the C++ loader)

**Class.** The descriptor (class id 0xa5) with create 0x1863fc, parameter reader 0x179470, destroy 0x22f0a0 and message handler 0x1922e8 is registered in the function that stores the pool words at 0x170348..0x170354. Create: record of 0x20 bytes at entity+0x7c, then 0x23e130(entity, record+0xc, 0, 0x56d40c, 0): without a class bound it would build the zone from the node bounds (see [carrying.md](carrying.md#ts-20261015-001729)); with `UseBound` it takes the class bound as it is. The class file `levels/props/prop_simpleuse.blvl` has the bound (centre 0, half 2, 2, 2). 0x23e130 also reads `UseStartActive` (default 1, zone bit 4) and `FaceTowards`. Parameter reader 0x179470: `TriggerObject` (object reference, record +0x18) and `SendPlayerObject` (byte, record +0x1c).

**Handler 0x1922e8(entity, message, parameter block).** Message 3 (the use query; the block holds the player at +0 and the action flag at +5): returns 0 when `TriggerObject` is unset. Otherwise, when the action flag is set and the player pointer is not null: `SendPlayerObject` 0 → 0x244f40(target, spot), else 0x244f40(target) (second argument whatever the register holds; the player is assumed); then 0x2414b4(player, player ctrl+0x50, 1, 0) (player state request 1) and 0x236624(spot, 0). Returns 1 whenever `TriggerObject` is set (the registry walk stops at this entry). Message 8: 0x236624(spot, 1), returns 0. 0x236624(entity, flag) finds the entity in the use registry and sets the zone record's byte +4: bit 4 = flag, bit 6 = not flag. 0x244f40 is the enabling trigger already used for carry events (enable the target, message 0xff, ObjectTriggered; [carrying.md](carrying.md#ts-20261015-111115)).

**Level data (lvl_01).** One spot: `1-2_Dog_Cage_Use_Spot` (room32, position 916.459 64.432 664.101), `TriggerObject` → `1-2_Cage_Key_Target_Script` (a scriptobject with an ObjectTriggered trigger of the same name). The script `1-2_Pringles_Cage_Locked` (the BoundEntered script of the bound of that name) enables the spot (`EnableObject 1-2_Dog_Cage_Use_Spot 1`); the level start script disables it.

**Oracle (port 24720, home `artifacts/oracle/r20-B`, OpenGL renderer, no RPC).** State `m12-13-cage-trigger`, scenario `hold A`, `ctrace 0x1922e8,0x244f40,0x236624 40`. In update 0 (the update of the A press edge): the handler is entered with r1 = 3 from the registry walk (lr 0x1fefd8); 0x244f40 follows with r0 = the scriptobject entity and r1 = the spot (so the instigator of the event is the spot for `SendPlayerObject` 0), called from 0x192358; then 0x236624 with (spot, 0) from 0x19237c. No further call of the three addresses in the next 39 updates with A still held: the zone is off after the use and the hint query (action flag 0) did not reach this class. Message 8 was not seen.

**Port.** `src/runtime/use_spot.*` (`UseSpotSet`), glue `LevelSession::updateUseSpots` in `src/game_detective.cpp`, tests `use-spot-real-data` (level data, class bound, one-shot rule, disabled object) and `game-use-spot-1-2` (A at the oracle position fires `1-2_Cage_Key_Target_Script`). Not modelled: the event's instigator (SCRIPT-190), player state request 1 (the player is already free), message 8, the hint prompt, the priority against other registry entries (the detective spots use DETECT-S6's order; this spot is far from them).

**Door_Lock_Hint_Script.** The scriptobject `1-2_Door_Lock_Hint_Script` (room34, 722.966 37.195 651.088) has the ObjectTriggered trigger (script: EnableBound `1-2_Door_Lock_Hint` 0, two dialogue lines of Doorlock Holmes, waits) and is referenced by no object parameter and by no argument of any script of lvl_01 (C++ loader scan of every object reference and script argument). The same script name is also bound to the bound `1-2_Door_Lock_Hint` (centre 720.343 38.840 655.627, half 3.787 2 2, shape 4) as BoundEntered; `1-2_SO_IntroCamOff` enables that bound (`EnableBound 1-2_Door_Lock_Hint 1`). So the hint is delivered by the bound, the way the port already runs it (`script 1-2_Door_Lock_Hint_Script started` after IntroCamOff and a step into the bound); the object trigger has no source found, like `Dtv_DogSpot_01_Script` and `Reveal_Bone_Script`.

Unknowns:
- Message 8 senders for the use spot; the hint-query behaviour of the class (the hint prompt text and when 0x1cba8c asks it); whether code builds the name of `1-2_Door_Lock_Hint_Script` at run time (callers of 0x2388e4 not read, SCRIPT-113).

<a id="ts-20261019-091253"></a>

## 2026-10-19T09:12:53+02:00 — What follows the release of the dog, and why `chase-game` stayed black (level data through the C++ loader, Azahar GDB `freetrap` from `m12-13-cage-trigger`, `chase-game` runs)

**Script.** `1-2_Cage_Key_Target_Script` (the ObjectTriggered script the spot fires): AIEnable Player1 1, cage collision off, the dog prop swapped for `1-2_Dog`, the player to `1-2_Chase_Position_Dog_Cage`, StartDCam `Dcam_Dog_Cage_Open`, the cage and dog animations and `dog_cage_open_minifig`, Wait 3, DCamCutSceneLoad `1_2_c`, **FadeScreen 0**, objects moved and switched off, the player to `1-2_Chase_Position_End`, StreamRoomIn `Room40`, `TriggerObject 1-2_Dcam_Exclusion_DogHouse`, wait 1.5, DCamCutScenePlay `1_2_c` 0 1 1 1, ShowDialogBG 1, DCamCutSceneWait, DCamCutSceneFree, ShowDialogBG 0, the dog truck on, the dog house room objects off, SpawnScript `1-2_MissionEnd`, AIEnable Player1 0. No FadeScreen 1 follows; `1-2_MissionEnd` (StoryProgress 3, reward, tip texts, dialogue, SpawnScript `1-3_MissionStart`) has none either, and `1-4_RobberCaught` has the same pattern.

**Original.** The cutscene mode brings the screen back: the trace of [cutscenes.md §2026-10-19T09:12:06](cutscenes.md#ts-20261019-091206) shows FadeScreen's fade to black, the DCamCutScenePlay handler's fade, the mode enter's 0.5 s fade in, the end fades and SpawnScript `1-2_MissionEnd` about 26 s after the press, then `1-3_MissionStart` and `1-3_Animations` (as in [scripts-missions.md §2026-10-18T19:58:54](scripts-missions.md#ts-20261018-195854)).

**Port before.** `chase-game` from the spot (`--start "916.702 64.431 663.724" --call-script 1-2_Pringles_Cage_Locked`, A at update 5) ran the whole chain (cutscene `1_2_c` from update 169 to 726, `1-2_MissionEnd` at 726, `1-3_MissionStart` at 1304, player at 745.951 29.973 636.281, StoryProgress 3), but the brightness of FadeScreen stayed 0 because only FadeScreen changed it: every picture after update 5 plus the fade was black (screenshot at update 906: all colour bytes 0).

**Port now.** The screen fade is shared (`ScreenFade`), so the cutscene mode fades in: black from the FadeScreen to the cutscene start (update 150 still black, as in the original), the cutscene from update 169, the game picture with the scanner tip after the end (screenshot at update 906). CTest `game-cage-release-1-2`.

Unknowns:
- The update numbers of the original between the press and the cutscene start were not measured (the trap gives wall-clock seconds with stop overhead).

<a id="ts-20261019-092534"></a>

## 2026-10-19T09:25:34+02:00 — The use marker on the ground (llvm-objdump on `artifacts/game/game.arm.elf`; RPC screenshots A-1 by the emulator servant and Azahar GDB `freetrap`, port 24771, state `m12-13-cage-trigger`; REA was not used)

**What it is.** The A-1 screenshots of `m12-13-cage-trigger` (on the spot, and 2.2 m back at 914.601 64.431 663.204, still inside the zone) show a flat disc about 2 m across on the ground at the spot: the texture `sprites/game/usemarkers/usegeneric.btga` (a dark disc with a ring and coloured dots). The use spot class has no mesh (`levels/props/prop_simpleuse.blvl` holds only the class id 165, two vectors and the bound), so the disc is drawn by a separate system.

**Textures and kinds (0x162adc, called from 0x2e43e4).** Loads `Sprites/Game/UseMarkers/use{Generic, Police, Robber, Miner, Astronaut, Farmer, Chicken, Pig, Fireman, Construction, Locked}.tga` into the table 0x5295ec (index 0 to 10) and `Sprites/Buttons/DSButtonA.tga` into 0x37603c + 8, sets a bit in the kind bitmap 0x5289cc for each entity kind of the zero-ended list 0x374640 (13, 36, 58, 73, 77, 110, 15, 144, 146, 145, 130, 153, 165, 94, 169, 171, 173, 181, 182, 185, 186, 190, 191, 195, 201, 209, 210, 231, 233, 150, 127, 200, 133, 124, 232, 227; 165 is `props/prop_simpleuse`), and makes 32 fader slots of 0x30 bytes at 0x529618 with 0x23cb90(owner 0x518218, slot, callback 0x17218c, 1, fade in 0.15 s, fade out 0.15 s).

**List (0x1c015c).** From the use registry of the scene (the list 0x236624 searches), every entry whose entity kind is in the bitmap and that passes 0x2397a0 for a player position lifted by 1.0 goes into the list 0x5289ec (0x18 bytes: entity, zone record, marker position at +8, new flag +0x14), and the zone record's +8 is set to 25.0. Rebuilt by the marker update when *0x376c78 changes and from 0x1b227c.

**Update (0x172254, per update).** Nothing while the player is in a vehicle (0x24255c). For each list entry it skips: the entity the player is using (ctrl+0x130), entities 0x23de70 reports, entity flag +0x10 bit 0, a zone without the usable bit 4, a node with flag bit 5, a parent that is off, and special cases of kinds 58, 150, 183 and 210. It then sends message 3 with no player and the action flag 0 (0x1922e8 returns 1 when `TriggerObject` is set) and needs a non-zero answer, and the camera (0x22e1f4) or the player within the zone record's +8 (25.0) of the entity. It takes the slot that has the entity, else a free one, else one whose entity 0x23de70 reports; a slot in state 0 marks the entry new, which places it with 0x1db800. With the entity's int parameter `HeadMarker` at its default -1 it calls 0x23cc00(slot, 0, hold 0.1); other values ask the player's character for ability `HeadMarker + 24` and call 0x22d7c0 instead.

**Placement (0x1db800).** From the zone centre (zone record +0x10) in the entity frame, a line 5 m down the node's up axis, objects in the box gathered (0x237f30, up to 50), a ray (0x204764, or 0x22a7a4 stepping up 0.5); the hit, in the entity frame, is the marker position; without a hit the zone centre.

**Fader (0x23cc00 show, 0x235e2c value).** State +0x10: 0 hidden, 1 fading in (value (now - start) / 0.15), 2 shown (1), 3 fading out (1 - (now - start) / 0.15), clamped to [0, 1]. Show: 0 → 1 with start = now; 2 restarts the start time; 3 → 1 with the start moved so the value goes on from where it was; all store the hold (+0x1c) of 0.1 s. What counts the hold down and starts state 3 was not found.

**Draw (callback 0x17218c → 0x236080 → 0x1b29f0).** 0x17218c frees the slot when it is fading out and its value x 255 is below 4; otherwise it transforms the marker position by the node matrix and submits 0x1b29f0. 0x1b29f0: alpha = value x 255; matrix = the node rotation with the marker position as translation, turned about its up axis by clock / 240 radians (0x241098, 0x237bc4; 0x247ac8 reduces by 2/pi, so radians); texture 0x292388 (index from the kind; 0 for kinds it does not list, such as 165); four vertices (-1, 0.1, -1) uv (1, 0), (1, 0.1, -1) uv (0, 0), (1, 0.1, 1) uv (0, 1), (-1, 0.1, 1) uv (1, 1), colour white with that alpha.

**Oracle (`artifacts/scratch/r27-A/marker_draw.txt` → `artifacts/oracle/r27A-out/markerdraw.txt`).** `frames 2`, `freetrap 4` at 0x1b29f0, 0x1b2a40 and 0x1db800: the draw runs every frame for slot 0x529678 (state 2, fade times 0.15, hold 0.1, callback 0x17218c, entry 0x5289ec); the clock after 0x241098 reads 0x7fed, 0x7fef, 0x7ff1, 0x7ff3 in successive draws (2 per frame, so 60 per second at 30 updates per second); 0x1db800 did not run (the marker was placed before the state was saved).

**Port.** `src/runtime/use_marker.{hpp,cpp}` (`UseMarkers`: 32 slots, the fader, range, quad and spin), glue `src/game_use_spot.cpp` (`LevelSession::updateUseMarkers` / `drawUseMarkers`, the quads go through the particle quad renderer with the use marker texture directory); only use spots feed it. `chase-game` from the spot: `use marker: 1-2_dog_cage_use_spot shown at update 0`, gone at update 13 after the A press at update 5 (hold and fade out); the screenshot shows the disc where the oracle has it. Tests `use-marker` (synthetic), `use-spot-real-data`, `game-use-spot-1-2`.

Unknowns:
- The fader manager that counts the hold down; the multiplier of the clock 0x241098; the render state that 0x1b29f0 passes to 0x25511c; the test 0x2397a0 of the list builder and the rebuild trigger *0x376c78; what 0x23de70 reports.
- The kinds other than 165 (carry objects, build-its, detective spots and the rest of the list) and the `HeadMarker` path.

<a id="ts-20261019-125816"></a>

## 2026-10-19T12:58:16+02:00 — The cat rescue use spot: resting clip and state machine (REA Ghidra 12.1.2 pseudo-code of 0x16d6a8, 0x1829ac, 0x176e04, 0x182a5c, 0x182b38, 0x1c6aa8, 0x18e1b8, 0x23afc0, 0x23e6e4; level data through the C++ loader; no emulator)

Closes the first part of the question left in [objects.md §2026-10-19T09:59:14](objects.md#ts-20261019-095914).

**Descriptor 0x16d6a8** (kind 124, class `props/ability_freeplay_fireman_cat_usespot`, 28 objects in all levels, 4 in lvl_01: `FP_Task_FIREMAN_CTH_01..04_UseSpot` in rooms 7, 10, 16, 26): create 0x1829ac, parameter reader 0x176e04, post-link 0x182a5c, update 0x182b38, message handler 0x18e1b8, component size 0x7c.

**Create 0x1829ac.** Entity of 0x88 bytes, `catrescue` string object at +0x3c, flags byte +0x18 = 2, record of 0x70 bytes at +0x7c with the state word +4 = 1 (requested state) and +2 not written (current state, taken as 0 from the zeroed allocation 0x25c030, not checked); registers a use zone (0x23e130, record +0xc) and stores 0x3f266666 (0.65) at record +0x64 (unread).

**Record.** +2 current state, +4 requested state, +0x10 flags (bit 4 cleared by state 8), +0x18 `TriggerObject` (0x176e04), +0x1c `Cat` (the second parameter read by 0x176e04; its name is the string "Cat" at 0x176e50, type 4 object reference), +0x20 the instigator object (set by the message handler), +0x24 / +0x28 / +0x2c / +0x30 the clips, +0x40 / +0x4c start and target points, +0x58 launch velocity, +0x6c state timer.

**Post-link 0x182a5c (indirect call, no direct caller).** When the `Cat` link resolved it loads four clips on the cat with 0x23e6e4(cat, name, 0, 0, 0, 1), which builds `models/<cat model folder>/<name>`: +0x24 `idle_sat`, +0x28 `rescuecat_jump`, +0x2c `rescuecat_jump_idle`, +0x30 `rescuecat_rescue`. All four exist as `models/char_cat/*.bfnanm` in the archive, and the four cats of lvl_01 (`FP_Task_FIREMAN_CTH_01..04`, class `props/ability_freeplay_fireman_cat`, `char_cat`) are the `Cat` targets.

**Update 0x182b38.** The generic object update 0x2424d0, then the state change 0x1c6aa8.

**State change 0x1c6aa8.** Equal states: the timer +0x6c grows by the frame time (0x23602c). Otherwise, by the requested state:
- 1: `idle_sat` on the cat with 0x23afc0(1.0, 0, cat, clip, 1, 0), i.e. 0x245b40 with blend 0, mode 1 (loop), speed 1.0, when the clip exists. This is the clip the cat plays at rest, from the first update of the spot on (the state starts at 0).
- 4: start point = the cat node translation, end point = the instigator's node translation plus 0.5 x its row 2 and 1.5 x its row 1 (0x2408c4 read as a scaled add, not checked); the launch velocity at +0x58 is (end - start) / T with T = 5 x |end - start|, its y component (dy + g/2 T^2) / T with g the float at 0x374290 (zero in the file: set at run time); then `rescuecat_jump` once (mode 0).
- 6: `rescuecat_jump_idle` looping.
- 7: `rescuecat_rescue` once, then 0x24c118(cat node, instigator node): the cat is hung on the instigator's node.
- 8: clears bit 4 of the flags and disables the cat and the spot (0x2428c0).
Every change then stores the requested state as current and zeroes the timer. The update itself never moves the cat along the launch arc; whatever applies +0x58 is outside this class.

**Message handler 0x18e1b8.** Message 3 (the action press, as for the other use spots): only in state 1; it needs the ability test 0x23a87c(character byte, 0x1e) (ability 30; a character without it gets code 0xff); with the pressed flag it sends message 0x1a6 from the spot to the instigator (0x2366bc), writes the angle toward the cat into the instigator's record (+0x296 = atan2 x 10430.378 from the instigator's +0x280 point and the cat node) and stores the instigator at +0x20; returns 1. Message 0x58: when 0x225cfc() is nonzero and 0x225ca8(*0x51640e) is zero, the cat and the spot are disabled. Who writes the requested state 4, 6, 7 and 8 was not found: the handler does not; the player's rescue state (reacting to message 0x1a6) is the likely writer.

**Port.** `src/runtime/cat_rescue.{hpp,cpp}` (`CatRescueSet`: `Cat` link, states 0/1/4/6/7/8, clip choice, timer, disabled list; `request` stands for the writer of record +4); glue in `src/game_level.cpp` (cats become skinned props) and `src/game_animals.cpp` (the clip starts go to `PropAnimations`). Test: CTest `cat-rescue-real-data` (four spots, links, clips in the archive, the state to clip table, the sitting cat's pose changes).

Unknowns:
- Who writes the requested state (4, 6, 7, 8) and applies the launch velocity; the fireman ability (30) and the player's reaction to message 0x1a6 are not in the port.
- Whether the idle clip starts only when the cat's room is loaded (the port starts it at level start); the unread bytes at record +0x64 and the message 0x58 condition (0x225cfc, 0x225ca8).
