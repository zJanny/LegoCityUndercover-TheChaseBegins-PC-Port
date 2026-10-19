# Cutscene actors: props and level entities

Entries about cutscene actors that are placed level entities (props, vehicles) and what the cutscene setup and teardown do to them. Main topic: [cutscene-actors.md](cutscene-actors.md).

## Current state

- The binding loop tests no kind: a prop or platform with a matching `CSCharacter` is an actor too, and the cutscene poses that entity (no model of its own). `1_2_c`'s dog is `1-2_Dog`; the port poses the placed prop (props without a scene key of their own keep a model of their own, stand-in LEVOBJ-S86). [§2026-10-19T12:59:54](#ts-20261019-125954)
- The cutscene setup 0x2d1524 enables a disabled actor entity and shows its node, saves the node's 64-byte local matrix and swaps its animation layers for an empty array; the teardown 0x2d135c (the mode exit 0x2fac7c) disables it again, puts the matrix and the layers back. So a disabled actor is drawn during the cutscene and the dog of `1_2_c` ends at its place before the cutscene. Entity byte +0x18 is the room-activity class: 5 sets both table bits for every room type. Ported for props and characters (`CutsceneHold`). [§2026-10-19T13:47:29](#ts-20261019-134729)
- The dog of `1_2_c` was invisible because the port's prop animation pass ran after the cutscene pose and put the dog back at its End node; the held prop is no longer posed by its own clip. The dog truck (`policebarker`) stood 1.59 m low because the port skinned vertices without the node matrix, which the skeleton's bind joints need (oracle pictures: wheels on the pavement) [§2026-10-19T18:30:11](#ts-20261019-183011); the model draw applies it after the skinning, as for every model ([models.md §2026-10-19T19:24:18](models.md#ts-20261019-192418)).

<a id="ts-20261019-125954"></a>

## 2026-10-19T12:59:54+02:00 — A cutscene actor binds to any entity with a matching CSCharacter, props included: the dog of `1_2_c` is the prop `1-2_Dog` (llvm-objdump of 0x2d1ddc and 0x196144 on `artifacts/game/game.arm.elf`, level and cutscene data through the C++ loaders; no emulator, REA tools not used)

Corrected by [cutscene-actors.md §2026-10-19T13:47:29](#ts-20261019-134729): the "End" paragraph is wrong; the teardown 0x2d135c puts the entity node matrix back to the one saved at the setup 0x2d1524, so the entity does not stay at the last pose.

[Corrects objects.md §2026-10-19T09:59:14](objects.md#ts-20261019-095914): the cutscene does bind `1-2_Dog`; the port no longer draws a dog of its own for it. [Corrects script-natives-flags.md §2026-10-19T11:03:27](script-natives-flags.md#ts-20261019-110327): the open question whether `1_2_c` binds `1-2_Dog` is answered.

**Rule (0x2d1ddc, instruction level).** After the `cape` and `player` tests of [§2026-10-15T07:54:38](cutscene-actors.md#ts-20261015-075438) the loop at 0x2d1f88..0x2d1fec walks the scene object's entity array (`[scene+0x24]`, count `[scene+0x1c]`, index from 0) and for every non-null entry reads `CSCharacter` (0x241624, type 0x1000010) and compares it with the folder through 0x25ad90 (case-insensitive); the first equal entry is taken (0x2d1fc0). Nothing in the loop tests the entity's kind, its class, whether it is a character, whether it has a skeleton, or whether it is enabled. So a prop or platform with a `CSCharacter` is an actor like a minifig is. A found entity goes to 0x1f35f4 (slot +0 = 1, entity pointer at slot +0x200, entity byte +0x18 saved to slot +0x1fa and set to 5), as for characters.

**Hidden or reused.** Reused: the actor is the entity itself; the cutscene poses the node `*(entity + 0x3c)`, no model is created (slot +1 stays 0). The loop does not test or change the entity's disabled state; the script `1-2_Cage_Key_Target_Script` has `EnableObject 1-2_Dog 1` before the cutscene and `EnableObject 1-2_Dog 0` after it, so the dog is drawn by the cutscene while it is enabled.

**End (0x196144).** For every slot from the last to the first it frees the animation array and writes the saved byte back to entity +0x18 (0x196264..0x196268); a slot with its own model is destroyed (0x2427a4, 0x250b8c). No entity position or matrix is restored there (the call 0x24ce78 on a node field at the loop start was not read), so the entity stays where the cutscene's last pose left it; the script then disables it.

**Data.** lvl_01 has exactly one object with `CSCharacter` `dog` (`1-2_Dog`, class `props/prop_prop`, kind 67); the cast of `1_2_c` is `dog` (19 joints, the same joint names as `models/char_dog/char_dog.bfnskl`; one clip starting at camera frame 180 of 521), `dog_owner`, `player1`, `policebarker` and `prop_heart` (start frame 419). Census of all main levels (scratch tool over `loadCutscene` and the class files, folders that some cutscene casts): objects with a `CSCharacter` that is not a character of the level: lvl_01 the dog; lvl_02 `2-3_Parrot01`, `2-4_Hoop01`, `2-4_KeyLocker`, `2-4_MiddleGateKey`, `2-5_Safecrack01`, `2-7_CellsKeypad`, `FP_GymEquipment07/08`; lvl_03 `3-0_CraneLift`, `3-4_DcutPallet`; lvl_05 `6-3_Squirrel01`, `6-3_SquirrelCage01`, five `6-6_ChalkBoard*`; lvl_06 `5-4_Shuttle`.

**Port.** `resolveCastTarget` (src/runtime/cutscene_cast.cpp) takes the entities in level order and returns the first match, character or prop (`CastTargetKind::LevelProp`); `levelProps` / `InEngine::pose` (src/game_cutscene.cpp) pose the placed prop's own scene instance with the cutscene clip (transform of the actor tracks, skin from the cast skeleton under the prop's scene key), so there is one dog. `chase-game` from the cage (CTest `game-cutscene-dog-prop`) prints `1 level props posed` at update 10 of `1_2_c`; a debug print of the actor matrix gave the dog at 744.5 28.7 647.3 at its first frame and 745.4 30.9 642.7 in the middle of the clip. The prop is posed only when it has a scene key of its own (an animation target of the level's scripts) and the cutscene model has a skeleton; every other matching prop still gets a model of its own and keeps its placed prop (stand-in LEVOBJ-S86, LEVOBJ-106). CTest `cutscene-cast` pins the dog rule on lvl_01.

Unknowns:
- Whether the entity array of the cutscene object is in level order (the port takes the order of `Level::objects`); it matters only for equal `CSCharacter` values.
- What the entity does with byte +0x18 = 5 and whether the node is drawn while the cutscene shows an entity that is disabled (LEVOBJ-105).
- Where the dog stands after the cutscene: the port's prop update moves it back to its root-motion node; the original leaves it at the last cutscene pose and the script disables it (LEVOBJ-107).

<a id="ts-20261019-134729"></a>

## 2026-10-19T13:47:29+02:00 — A cutscene enables its actor entities and restores their node matrix at the end; byte +0x18 = 5 is a room-activity class (llvm-objdump of 0x2d1524, 0x2d135c, 0x2d19cc, 0x1f35f4, 0x2ec454, 0x196144, 0x24ce78, 0x23c1a8, 0x1590b0, 0x2405a0 on `artifacts/game/game.arm.elf`, level data through the C++ loaders; no emulator, REA tools not used)

[Corrects cutscene-actors.md §2026-10-19T12:59:54](#ts-20261019-125954): the entity does not stay at the last cutscene pose. Answers LEVOBJ-105 and LEVOBJ-107.

**Setup 0x2d1524** (called once by the cutscene mode enter right after the object is built, lr 0x2faddc), for every slot of the object (0x22c bytes, array at +0x70) whose entity E = `[slot + 0x200]` exists and whose name is not `CSCamera` (compared with the string at 0x35964c):
- the controller word `[[E+0x44]+0xc]` and the room pointer `[E+0x20]` are saved (slot +0x44, +0x48);
- **E is enabled for the cutscene:** when E's flags word `[E+0x10]` has bit 0 (disabled), slot byte +0x1b0 = 1, the node `[E+0x3c]` is shown with 0x23f734(node, 1) (clears 0x60 of the node's first word) and flags bit 0 is cleared (0x2d15d8..0x2d1600); otherwise slot +0x1b0 = 0;
- 0x22c2ec(node, slot + 0x1b4) copies 64 bytes from node + 0x1c (the node's local matrix, copy routine 0x258858) into the slot;
- for a name different from `CSCamera` it also saves the node byte +0xde, clears its bits 4..5 and sets +0xdd = 0xff, +0xdc = 0x10, saves the controller's layer array pointer (+0x2c), layer count (+2, low 5 bits) and flags (+4 without bit 31), clears 352 bytes at slot +0x50 and makes that the controller's layer array with count 0 (0x2d1658..0x2d16d8). So the entity's own animation layers do not run during the cutscene; the cutscene's clips use the empty array (0x24ce78 clears every layer: 88 bytes each).

**Step 0x2d19cc** (per update, after the clips): for E it sets the node orientation from the controller when its flag bit 31 is set (0x2327b4, 0x20e850, 0x24c118), re-attaches the node to the room found at its world position (0x22d204 on `[E+0x20]`, 0x245048 / 0x245008, 0x23c1a8, 0x229bf0) and calls 0x2ec454, which writes 5 into E +0x18 again when something changed it (0x2ec480..0x2ec4a4; slot +0x1fa keeps the first value). It never calls 0x23f734, so the node stays shown for as long as the setup left it.

**Teardown 0x2d135c** (called by the cutscene mode exit 0x2fac7c, lr 0x2fad70, which frees the object; [cutscenes.md §2026-10-19T10:04:29](cutscenes.md#ts-20261019-100429)), for every slot with E, name not `CSCamera`:
- slot +0x1b0 set: node hidden again with 0x23f734(node, 0) and flags bit 0 set (0x2d13dc..0x2d13fc), so a disabled entity is disabled after the cutscene exactly as before;
- 0x24ce78 clears the cutscene's layers; **0x241540(node, slot + 0x1b4) copies the 64 bytes back to node + 0x1c** (and sets the dirty bit 0x1000 on the node and its children), so the node's local matrix is the one saved at the setup, not the last cutscene pose; the saved layer array, count and flags and the node byte +0xde are put back; 0x271ccc(E) walks the layers like 0x1dd3f8 at the setup (waits for loading layers).
- The room pointer `[E+0x20]` is put back too (0x2d13c0).

**DCamCutSceneFree 0x196144** (script native, free by name; also 0x1ed614 at the level end) is a different path: per slot from the last it calls 0x24ce78 on the entity's controller (no matrix restore), frees the slot's array, writes the saved byte back to E +0x18 (0x196264) and destroys own models.

**Byte +0x18 = 5.** Creates write 2 into it (e.g. 0x1733fc, 0x1c475c); 0x1c3f44 turns 3 into 4 and re-places the entity (0x23c1a8). The consumer is 0x23c1a8(E, room) (and 0x1dd590 / 0x1ffbd8 through the same table): it indexes the table of u16 flags `*0x376ad4` with row `[E+0x18]` (7 rows, 34 bytes each) and column `min([room+0x3a], 16)`. Entry bit 0 selects the active room node lists (`room+0x14` / `+0x18`) over the inactive one (`room+0x78`) for an enabled entity (a disabled entity always goes under `room+0x78`); bit 1 of an enabled entity whose `[E+0x13]` is 0 leads to 0x173534(E) (not read), every other case to 0x22d480(E) (a routine under the lock 0x25c828, not read to the end; as read from the disassembly, not measured). The table builder 0x1590b0 (via the setter 0x2405a0(row, first column, bit 0, bit 1)) gives row 5 both bits for all 17 room types (row 0: both except column 16 zero; row 1: both, column 16 bit 1 only; row 2: bit 1 only, column 16 none; row 3: none; row 4: bit 0 only, column 16 none; row 6: both except column 16 bit 0 only, rewritten by 0x16c09c and 0x2ed144). So class 5 means "always in the active lists of its room, whatever the room type".

**Data.** The dog of `1_2_c`: `EnableObject 1-2_Dog 1` and `PositionObject 1-2_Dog 1-2_Dog_Position_End` (730.328 31.599 645.549, lvl_01) run before the cutscene, so the setup saves that matrix and the teardown restores it; then `EnableObject 1-2_Dog 0`. The earlier statement that it stays at the last cutscene pose was wrong.

**Port.** `CutsceneHold` (src/runtime/cutscene_hold.*) records the held entities and the saved node matrices; `InEngine::hold` / `release` (src/game_cutscene.cpp) run when the cutscene is shown and when it ends: a held entity counts as drawn (`drawnWhileHeld`, used for props in `LevelSession::draw` and for characters in `CharacterVisibility::apply`), and a posed prop gets its saved matrix back. The level characters' figures are never moved by the port (the cutscene draws a copy), so their place after the cutscene already equals the original's. CTest `cutscene-hold` (rules), `animal-props-real-data` (the dog's node, lvl_01 level data) and slow `game-cutscene-dog-end` (chase-game prints `cutscene 1_2_c ended: level prop at 730.328 31.5987 645.549` at update 745).

Unknowns:
- Whether the cutscene enter calls the setup before or after the cutscene's first update (the port holds when the cutscene is shown).
- The player's own entity (`player1`) goes through the same setup and teardown; where the player stands after a cutscene in the port is unchanged (MOVIE-22).
- Whether a script `DCamCutSceneFree` after the cutscene clears the entity's restored layers again (MOVIE-38).
- The layer swap: the port's prop clips keep running during a cutscene (LEVOBJ-110); the room classes are not modelled (LEVOBJ-111).

<a id="ts-20261019-183011"></a>

## 2026-10-19T18:30:11+02:00 — The dog of `1_2_c` was overwritten by its own clip and the dog truck lacked its node matrix (user playtest; port screenshots, level and model data through the C++ loaders, Azahar OpenGL RPC shots from `m12-13-cage-trigger` port 45683; no code read, REA not used)

Corrected by [models.md §2026-10-19T19:24:18](models.md#ts-20261019-192418): the original skins the vertices first and applies the node matrix after (model draw 0x205160, vertex program 0); the port now does so for every model.

Answers [playtests.md §2026-10-19T17:56:41](playtests.md#ts-20261019-175641) (dog invisible, dog truck in the ground).

**Reference (oracle).** From `m12-13-cage-trigger` with A held, `shotloop` took 904 pictures of the release and `1_2_c`: the dog truck stands on the pavement with all four wheels visible, the brown dog jumps out of its rear door and stands beside Chase, then the heart frame closes the scene. (Pictures are not committed.)

**Dog.** The cutscene bound and posed the placed prop `1-2_Dog` correctly (matrix and 19 skin matrices per update), but `LevelSession::updatePropAnimations` runs after `InEngine::update` in the same update and posed the prop's own animation (and its moved node, `1-2_Dog_Position_End` at 730.328 31.599 645.549, 15 m away) over it; at the draw the instance stood at 730.328 31.5987 645.549 for the whole cutscene. The port now skips the pose and node of a prop that a cutscene holds (`src/game_animals.cpp`, log line `prop 1-2_dog: its own animation is not shown while a cutscene holds it`); the clip clock still runs (LEVOBJ-110). The cutscene's own `dog.bfnmdl` is a 49-byte stub without a mesh, so the level prop is the only dog model; its posed bounds (skin of the clip at frames 0, 100, 300) are a normal 1.5 m dog, no NaN.

**Truck.** The model `csdata/models/1_2_c/policebarker/policebarker.bfnmdl` has one node (`Chassis`, matrix translation 0 1.59087 0) and five joints (`Base`, `Wheels_Front`, `Wheels_Rear`, `Chassis`, `Door`). Its skinned vertices are stored in node space: the 256 rigid wheel vertices of `Wheels_Front` average y -1.09363, the joint's bind height is 0.497598; with the node matrix applied the average is 0.4973. `prop_furnacevent` (node matrix translation 0.1905 0 0) fits the same way (slat vertices x -0.19055, joints at 0). The port skinned the raw vertices, so the truck stood 1.59 m too low, wheels in the ground. Of the 380 skinned `.bfnmdl` models only these two have a non-identity matrix on a skinned node. The static pose of the truck (actor root 743.905 31.8088 646.216, constant over the 521 frames; Base joint skin translation -0.3006, pitch about -9 degrees, yaw about -40 degrees, like the level's `1-2_Dogtruck` at 742.878 32.958 644.793 pitch -10.575 yaw -46.85) then puts the wheels at y 31.5, the pavement height beside Chase (feet at 31.08 two metres away, slope about 0.15).

**Port.** `skinnedPartInModelSpace` (src/assets/model.cpp) applies the node matrix to positions and normals of a skinned part when the scene loads it (src/render/scene.cpp). CTest `skinned-node-matrix` pins the centroids above (wheels, slats, an unchanged dog); `game-cutscene-dog-prop` also requires the held-prop log line. The rule is a data fit: the original's skinned draw was not read (MOVIE-S35, MOVIE-39).

Unknowns:
- How the original's skinned draw uses the node matrix (this fit applies it before the skin palette); only two models test it.
- Whether the dog's own layer swap (0x2d1524) also stops its clock; the port still advances it (LEVOBJ-110).
- The original shows the dog jumping out of the truck's rear door; the port poses the clip (start frame 180) but the jump was not compared frame by frame with the truck.
