# Cutscene actors

Which entities a cutscene's cast are, why the original draws no second Chase and gives the actors their faces and clothes. For the cutscene natives and fades see [cutscenes.md](cutscenes.md).

## Current state

- A cutscene's cast are the level's own characters: the manifest constructor 0x2d236c calls the actor builder 0x2d1ddc per model folder; slot setup 0x1f35f4 wraps an existing entity. [§2026-10-15T07:54:38](#ts-20261015-075438)
- Folder `player1` resolves to `Player1.Player01` (Chase himself); other folders match the entity whose `CSCharacter` parameter equals the folder name; `cape` models get no entity. [§2026-10-15T07:54:38](#ts-20261015-075438)
- Only when no entity is found does the cutscene build its own model from the folder; the script native DCamCutSceneFree 0x196144 restores entity byte +0x18 and destroys own models. [§2026-10-15T07:54:38](#ts-20261015-075438)
- The port draws a copy of the level character and hides the figure during the cutscene (stand-in MOVIE-S30); the double Chase after the van drive was a port bug, fixed so the figure stays hidden. [§2026-10-16T09:40:39](#ts-20261016-094039)
- Original and port frames of `1_1_b` were compared but show different moments, so no actor comparison exists. [§2026-10-15T10:50:31](#ts-20261015-105031)
- Each actor clip starts at the camera frame in its file name (`<actor>_NNNN`, 0x2ec30c) and is started by 0x2ec454 when the frame passes it; the clip started last drives the actor. Ported; this fixed the floating donut box of `1_1_d`. [§2026-10-16T14:05:18](#ts-20261016-140518)
- A cutscene model of its own is drawn per node: object track of the node x the actor node of the root tracks (oracle, `1_1_d`); before its first clip it stands at that clip's frame 0. Ported; this fixed the van under the street and the box not held by the man. [§2026-10-16T14:39:48](#ts-20261016-143948)
- User report: the magnifying glass floats in the port's mission 1-2 intro; the cause was the prop `prop_magnifyingglass` of `1_2_a` (clip start frame 478) drawn at its frame-0 pose from the start. The port now draws a model of its own only once its first clip starts (stand-in MOVIE-S33); the original's gate was not found. [§2026-10-18T20:38:55](#ts-20261018-203855), [§2026-10-18T21:06:57](#ts-20261018-210657)
- The binding loop tests no kind: a prop or platform with a matching `CSCharacter` is an actor too, and the cutscene poses that entity (no model of its own). `1_2_c`'s dog is `1-2_Dog`; the port poses the placed prop (props without a scene key of their own keep a model of their own, stand-in LEVOBJ-S86). [§2026-10-19T12:59:54](#ts-20261019-125954)
- The cutscene setup 0x2d1524 enables a disabled actor entity and shows its node, saves the node's 64-byte local matrix and swaps its animation layers for an empty array; the teardown 0x2d135c (the mode exit 0x2fac7c) disables it again, puts the matrix and the layers back. So a disabled actor is drawn during the cutscene and the dog of `1_2_c` ends at its place before the cutscene. Entity byte +0x18 is the room-activity class: 5 sets both table bits for every room type. Ported for props and characters (`CutsceneHold`). [§2026-10-19T13:47:29](#ts-20261019-134729)
- Open: the code that binds object tracks to nodes, the camera framing of `1_1_d`, the player's position after a cutscene, actors `Donut1` and `Donutbox`, a close-up oracle state.

<a id="ts-20261015-075438"></a>

## 2026-10-15T07:54:38+02:00 — Cutscene actors are the level's own characters (REA/Ghidra on `artifacts/game/game.arm.elf`, llvm-objdump, level data scan; oracle slot-table reads inconclusive)

Read: cutscene object constructor 0x2d236c, actor builder 0x2d1ddc (called per manifest model entry), slot setup 0x1f35f4, cutscene end 0x196144, helpers 0x24a154 (case-insensitive `strstr`, returns the hit or 0) and 0x25ad90 (case-insensitive `strcmp`, 0 when equal).

- **The constructor** (0x2d236c) walks the manifest lines (`CSData/models/%s/manifest.bstxt`). For every `.fnmdl` entry it cuts the path at the last `/` (directory = actor folder such as `DonutMan`, rest = model file) and calls 0x2d1ddc(object, "CSData/models/<id>/<Folder>", "<model file>") unless the folder is `CSCamera` or `textures`. The slot array of the object is at +0x70 (pointer), +0x74 capacity, +0x78 count, 0x22c bytes per slot.
- **Actor builder 0x2d1ddc** (normal case, the debug callback at 0x376d28 is not set): it looks for an existing entity first.
  - Model file name contains `cape` (case-insensitive): no entity.
  - Folder name contains `player`: when the model file contains `minifig` and the character after `player` is `1`..`9`, it tries the entity names `Player%s.Player%02d` (the `%s` is the folder name after the first six characters, so `player1` gives `Player1.Player01`), for 1..16, in the level object table (0x589a1c, lookup 0x23b350) and takes the first one that exists, is not flagged (+0x10 bit 0 clear) and has a node (+0x3c). Folder `player1` is therefore Chase himself.
  - Any other folder: the first entity of the level's entity list (scene object +0x24, count +0x1c) whose `CSCharacter` parameter (lookup 0x241624, name `CSCharacter`, type 0x1000010) equals the folder name case-insensitively.
  - Found: slot setup 0x1f35f4(slot, 1, entity, model file): slot +0 = 1, +2 = name, +0x200 = entity pointer, saves the entity's byte +0x18 into slot +0x1fa and writes 5 into entity +0x18. The cutscene then poses the entity's own node (the node pointer is `*(slot + 0x200 + 0x3c)` when slot +0 is set). So the actor **is** the level's figure with its own head, face, upper and lower textures; nothing new is created.
  - Not found: a model of the cutscene folder is built (`.bfnmdl` from the folder, textures from the folder's `textures/` directory, which carries the `*_test` placeholders) and owned by the slot (slot +1 = 1).
- **End** (0x196144, DCamCutSceneFree): for every wrapped slot it writes the saved byte back to entity +0x18; for own models it destroys them (0x2427a4, 0x250b8c). So a level character or the player is not hidden: it is the actor for the cutscene's duration and goes back afterwards.
- **Data** (scan of every `.blvl` of lvl_01 for the name hash 0xcabcdbd4 of `CSCharacter`): `1-1_AI_DonutShopMan` = `DonutMan`, `1-1_AI_DonutShopMan01` = `DonutEater`, `1-1_AI_DonutCop` = `trolley_guy1`, `1-1_AI_DonutLady` = `donut_driver`, and 17 more (`dog_owner`, `natalia`, `robber1`, `robber2`, `boss`, `carsalesman`, `police1`, `police2`, ...). Cutscene 1_1_a's cast is `player1`, `DonutEater`, `DonutMan` (minifigs) plus `Donut1` and `Donutbox` (props, no `CSCharacter` match among the characters).
- **Port** (`runtime/cutscene_cast.*`, `game_cutscene.*`): the same rules choose the stand-in. The port cannot drive the shared figure (level characters with the same look share one skinned body), so an actor that is a level character gets a copy of that character (same look, own skin) and the level's figure is hidden while the cutscene shows (stand-in MOVIE-S30, the visible result is the original's). The cast skeletons equal `char_minifig.bfnskl` joint for joint (test `cutscene-cast`). Result in `chase-game --call-script 1-1_VanBreakdown`: one Chase with his hair and clothes, the donut man with face and shirt.
- **Oracle**: the slot table 0x58976c of `gameplay-start` plus teleport onto the donut shop waypoint shows slot 0 named `1_1_b` with the object pointer set (0x58978c), count 3, capacity 7. The slot array at object +0x70 read as zeros through the GDB stub (heap above 0x09000000), so the flag byte, the entity pointer and the entity byte 5 were not seen. The word at 0x5898b4 that the earlier entry calls the active object points into an array of 0xd0-byte records, not into the 672-byte object; the earlier entry's name for it may be wrong.

Unknowns:
- What entity byte +0x18 = 5 means (the figure's control state while a cutscene drives it) and what else the original does to the driven figure (position after the end, AI state, collision).
- Where the player stands after a cutscene (the original keeps the figure's own position or the script moves it; MOVIE-S21).
- Whether `Player%s` entities other than player 1 exist in the story mode, and the actors `Donut1` and `Donutbox` (non-minifig props: the level has `Donut_Box` with an empty `CSCharacter`).
- Port-vs-original comparison pictures: not taken (software renderer crashes on street states, RPC not granted).

<a id="ts-20261015-105031"></a>

## 2026-10-15T10:50:31+02:00 — MOVIE-21 comparison screenshots: cutscene `1_1_b` in the original and the port's `1-1_VanBreakdown` run (oracle `cutscene-1-1-b` with OpenGL and RPC screenshots; port `chase-game`; no code change; REA was not used)

- **Original.** `tools/azahar_oracle.py` with a scenario of `screenshot shot-movie21-orig-0`, `wait 2`, `screenshot shot-movie21-orig-2s`, and the options of the COLL-21 entry, with `--state cutscene-1-1-b`. Frames: `artifacts/oracle/agentSHOT-out/shot-movie21-orig-0.png` and `shot-movie21-orig-2s.png` (400x480).
- **Port.** `build/chase-game --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --no-start-cutscenes --call-script 1-1_VanBreakdown --frames N --window 400x480 --screenshot <scratch>/port-movie21-N.bmp` with N = 45, 100, 200. Copies: `artifacts/oracle/agentSHOT-out/port-movie21-45.png`, `port-movie21-100.png`, `port-movie21-200.png`.
- **Side by side:** `artifacts/oracle/agentSHOT-out/sbs-movie21.png` (original 0 s, original +2 s, port 45, 100, 200 updates).
- **Matches.** The same subtitle box and dialogue style; the 400x480 layout; the donut shop and the police garage appear as the level's own rooms.
- **Differs.** The original state shows the police garage with the car and the line "Go to your patrol car. I'll send you the delivery van's location." The port run from the donut-shop start shows the donut-man lines ("I'm sorry. We're out of doughnuts...", then "Not good. Where's the van?..."). At +2 s the original has no subtitle and the camera is still on the garage, with no actor in close view. The original frames therefore show a different moment of the sequence than the port frames, and no actor can be compared in them. The port run starts at the level's start point, not at the state's position (see MOVIE-23). The original's bottom screen during the cutscene shows a hexagon panel with a police badge in the centre; the port's bottom screen shows the tip panel (45, 100 updates) or the empty dialogue background (200 updates).

Unknowns:
- Which part of the original sequence the state is in at 0 s (the state was saved before the cutscene's dialogue) and how long the cutscene lasts from it; not measured.
- Whether the port's 1-1 sequence (from the donut shop) matches the original's order of cutscenes 1_1_a and 1_1_b; not read.
- The actors in the original frames are too far to compare; a close-up state is needed for the actor check.
- Which code draws the original's bottom-screen hexagon panel during a cutscene, and its textures; not searched.

<a id="ts-20261016-094039"></a>

## 2026-10-16T09:40:39+02:00 — MOVIE-31: Chase drawn twice in the cutscene after the van drive was a port bug (port run of `game-van-escort`, oracle shots `m11-6b` of agent A, round 15; no new code read)

- **Original.** The oracle shots of `m11-6-van-to-shop` (hold A, then the cutscene to its end, `r15-b/oracle/m11-6b_NNN.png`) show Chase once: standing right of the parked van, seen from behind; the donut seller runs in with the red box. This matches [the actor entry](#ts-20261015-075438): the actor is the player's own figure.
- **Port before the fix.** `chase-game --call-script 1-1_SO_VanFixed --start "866.0 52.0 691.0"` with the script of CTest `game-van-escort` (jump into the van, short drive, `at 820.0 18.5 1047.0`, the waypoint) showed two Chases for the whole cutscene 1_1_d. Cause: `1-1_VanEscorted` calls `VehicleJumpOut` and `PositionObject`; the placement runs `LevelSession::cancelBoardingByPlacement`, which showed the player figure again after the cutscene host had hidden it (MOVIE-S30), so the hidden original and the cast copy were both drawn.
- **Fix.** `cancelBoardingByPlacement` leaves the figure hidden while a cutscene plays; the host shows it again when the cutscene ends. The cutscene host logs `cutscene NAME: N level figures drawn beside their cast copies` ten updates into every cutscene; `game-van-escort` expects 0 for 1_1_d. Port picture after the fix: one Chase beside the van and the seller's box (scratch only).

Unknowns:
- Where the player stands after the cutscene in the original (MOVIE-22); the port still restores the place of the figure.

<a id="ts-20261016-140518"></a>

## 2026-10-16T14:05:18+02:00 — Actor clips start at the frame in their file name; the van of 1_1_d is posed 5.8 below its bind height (REA with Ghidra on `artifacts/game/game.arm.elf`: 0x2d236c, 0x2ec6c8, 0x2ec30c, 0x2ec454; llvm-objdump; data probes with `chase_assets`; chase-game runs; no emulator)
Corrected by [cutscene-actors.md §2026-10-16T14:39:48](#ts-20261016-143948): the van is not posed below the street in the original; the object tracks of the clip place the model nodes on the actor node, and a model of its own stands at frame 0 of its first clip before the start.

User report (2026-10-16): in cutscene `1_1_d` (the van brought back to the shop) the van is invisible, the donut box hangs in the air and the man does not hold it.

Clip start frames (from the code):
- The manifest constructor 0x2d236c builds the actors (0x2d1ddc per model) and then calls 0x2ec6c8(actor slot, manifest text, folder) for every slot. 0x2ec6c8 walks the `.fnanm` lines twice: it counts the clips of the slot, then fills an array of 8-byte entries at slot+0x224 (count at +0x228). Each entry is {float start frame, animation handle}. A file missing from the archive leaves its entry empty, and that clip never plays.
- 0x2ec30c(slot, file name, &start) matches the file to the slot by the pattern `%s_????.*` (or `%s_????_%s.*` when the model has a suffix such as `minifig`), with the slot's actor name for `%s`. It parses the number after `<actor>_` with 0x203564. So `donutbox_0100.bfnanm` starts at frame 100.
- 0x2ec454(previous frame, current frame, slot, ...) is the per-update actor step. Every entry with `previous < start <= current` starts its clip on the actor (0x245b40 for a model of its own, 0x23afc0 for an entity), and the clip started last becomes the current clip (slot+0x1fc). While `current - start` is inside the clip, the clip is re-synced (0x22c2fc) when its time is off from `current - start` by more than 0x3d088889 (1/30 s). The cutscene object starts with the previous frame at -1.0 (0x2d236c stores 0xbf800000 at +8), so a clip with start 0 begins in the first update.
- Data: 330 actor clips are listed in the 69 manifests. All names match the pattern, and 61 have a start other than 0. For 37 of those 61, start plus clip length equals the camera length (`donutbox_0100`: 100 + 98 = 198). Two actors have two clips: `dunby` of 2_1_b (0000 and 0020; 0000 is listed but absent from the archive) and `plant` of 2_6_b (0000 and 0075).
- The port played every clip from frame 0. So the box ran 100 frames early, ended in mid-air and never met the man's hand. `chase_assets` now keeps every clip of an actor with its start frame (`cutsceneClipStart`, `cutsceneClipAt`); `chase-game` and `chase-viewer` pose an actor from its current clip. After the change the box is not drawn before frame 100 and then moves up by the man (screenshots of the port at camera frames about 70, 137 and 190).

The van (not fixed):
- `prop_donutvan_0000` has one set of root tracks: position (820.207, 12.7203, 1051.31), yaw 0, scale 1, constant over all 198 frames. The model `prop_donutvan.bfnmdl` has five nodes whose matrices put the root and the chassis at (820.207, 18.5204, 1051.31). Its mesh 0 spans y -1.52 to 4.01 locally. Posed by the track, the body spans y 11.2 to 16.7, below the pavement near the shop (about 17 to 18.5), so the street hides it.
- `prop_donutvanwheels` has four meshes, each centred on the origin, with one node each (wheel centres at y 17.24, x and z about 1.1 and 1.5 from the root). Its single track set sits at wheel 1's x and z with y 16.7431. The port draws all four meshes with the one actor transform, so the wheels overlap in one spot.
- So the original combines a prop's root track with its node matrices in a way the port does not have. The difference for the van body is 5.8 in y only; x and z equal the bind pose.

Unknowns:
- How the original poses a cutscene prop from its track and its node matrices (bind pose, base of the accumulating branch of 0x287ca4), and so where the van and its wheels stand.
- What an actor shows before its first clip starts (the node is not animated yet; the port uses the identity transform for its own models and the level figure for driven characters, stand-in MOVIE-S31).
- Whether a finished clip holds its last frame (the port holds it) and whether a later clip blends with the earlier one (0x245b40 / 0x23afc0 get 1.0 and 0 as their first arguments).

<a id="ts-20261016-143948"></a>

## 2026-10-16T14:39:48+02:00 — Cutscene props: object tracks place the model nodes on the actor node (Azahar oracle, New 3DS, OpenGL, `--rpc`, state `leadcut-1_1_d-early` from `m11-6-van-to-shop`; RPC reads and `shotloop`; REA with Ghidra on 0x287ca4, 0x245b40, 0x250810)

[Corrects §2026-10-16T14:05:18](#ts-20261016-140518) (the van of `1_1_d` was taken as posed 5.8 below the street with the rule unknown, and a model of its own was assumed to keep the identity transform before its first clip).

User report (2026-10-16, after the start frames): the donut box still does not look held by the man in `1_1_d`.

Measurement (RPC sampler over the whole cutscene, one record per changed frame of object +8; actor array object +0x70, slots of 0x22c bytes, +0x78 count 6; world matrix at node +0x5c, 0x250810 returns node + 0x5c after refreshing it when node word 0 has 0x1000):
- The actor nodes of the models of their own carry exactly the root tracks: van (820.2075, 12.7203, 1051.3093), wheels (821.2885, 16.7431, 1049.8235), both with identity rotation over the whole cutscene; the box (820.0394, 18.2891, 1053.9601) from camera frame 0, unchanged up to frame 100.97, moving from 101.97 (820.0273, 18.2809, 1053.9751) and held from about 180 (820.7750, 17.7669, 1057.9631). So the start frame 100 holds, and before it the box node already stands at frame 0 of its clip (identity only at frame -1, before the first update).
- The actor nodes are siblings under the cutscene root (node +4 parent, +8 first child 0, +0xc next sibling: van -> box); their bounds (+0x98 radius, +0x9c centre, then half extents) stay the model's header box.
- Pictures (`shotloop`, frame logged with each shot): the van stands on the road at its normal height and the man holds the box in front of him at frame 165.

The rule (data, consistent with the measurement):
- Each clip has an object track per model node (section +0x0c: name, block 0 a curve of the decoder 0x248910, block 1 one byte per frame). They are local to the actor node: the van's `Chassis` is (0, 5.8, 0) with a 180° turn, so object x actor node gives (820.2075, 18.5203, 1051.3093) with the turn, which is the model's bind matrix of `Chassis`; `Wheel_20x12` is (0, 0.4975, 0) on the wheels' actor node, the bind centre 17.2404, and the other three wheels have their own offsets (`Wheel_20x13` (-0.035, 0.4975, 3.04)). The box's `Box` track moves the box out of the van (frame 0 (-0.039, 0.033, 0.001) on the actor node) to the man (end (0.4275, 0.1675, 1.2031) with a 180° turn).
- The port drew every mesh of such a model at the actor matrix alone: the van 5.8 too low, the four wheels in one spot and the box upright and turned, not in the man's hands. `cutsceneNodeMatrices` now draws node n at object(n) x actor (object matched by name, case-insensitive; `cutsceneObjectMatrix` blends two frames and holds at both ends), `cutsceneRestingClip` gives frame 0 of the first clip before its start; the game and the viewer draw one instance per node (`Scene::addModelNodes`). Test `assets-real-data` pins the actor nodes above and checks that every mesh node of the van and the wheels lands on its bind matrix at frame 0.
- Census over all 69 cutscenes: 89 models of their own without skeleton with 104 mesh nodes; 89 have an object track of their name, 15 (nine models, for example `truck` of 4_2_a and `donutbox` of 2_3_d) belong to clips without any object track, and one object track has no node of its name.
- 0x287ca4 (root-motion event 0xb945fcd8): for a slot with flag 0x800 (set by 0x2ec454 for cutscene clips) it copies position x, y, z and yaw straight into the target's transform record (target +4, fields +0x10..+0x1c) and returns; otherwise it adds weighted deltas per axis (the walking path). 0x245b40 is the generic slot player (blend, speed, start and end frame, mode bits); neither binds object tracks to nodes, so the binding code itself was not found.

Unknowns:
- The code that binds an object track to a model node (by name or by index; the data matches by name) and what a mesh node without an object track does (the port keeps it at the actor matrix, stand-in MOVIE-S32).
- Whether the object tracks, like the root, also stand at frame 0 before the start (assumed; only the actor node was read) and what a driven figure shows before its start (MOVIE-S31).
- The camera of `1_1_d`: at camera frames 120 and 137 the original still shows the close shot of the van while the port shows a wider shot; whether the port's frame count or its camera differs is open (MOVIE-33).

<a id="ts-20261018-203855"></a>

## 2026-10-18T20:38:55+02:00 — User report: the magnifying glass floats in the opening cutscene

- The user played `chase-game` after round 23 (glass drawn from the hand item track, [abilities-detective.md §2026-10-18T19:30:00](abilities-detective.md#ts-20261018-193000)): in the beginning cutscene the magnifying glass is visible, standing still in the air; in the emulator it is not seen until Chase pulls it out.
- Not checked yet: which cutscene the user means, and which part of the port draws the glass there (the player's hand item glue, or the cutscene's per-node drawing of a model of its own, [§2026-10-16T14:39:48](#ts-20261016-143948)).

Unknowns:
- Which cutscene it is, which instance shows the glass, and how the original keeps it hidden before Chase pulls it out (the hand item track 0xba1e5b53 in the cutscene clip is one candidate).

<a id="ts-20261018-210657"></a>

## 2026-10-18T21:06:57+02:00 — The floating glass is the prop of cutscene `1_2_a`, drawn before its clip starts (`chase-game` offscreen runs, scan of all cutscene clips with `runtime::handItem`, REA with Ghidra on 0x2ec454, 0x2d19cc, 0x2d1ddc, 0x245b40)

Corrects the open question of [§2026-10-18T20:38:55](#ts-20261018-203855) (which instance drew the glass): it was not the player's glass glue.

- **Which cutscene:** `1_2_a`, the intro of mission 1-2 (`1-2_HouseArrival`, `chase-game --call-script 1-2_HouseArrival`). Its cast is `dog_owner`, `player1` and the model-of-its-own prop `prop_magnifyingglass` (nodes `polySurface7`, `glow1`) with one clip `prop_magnifyingglass_0478`, so it starts at camera frame 478 of 651 and ends at frame 650. Cutscene `1_1_a` (the beginning of a new game) has no glass: its cast is `player1`, the donut man, the eater, `Donut1`, `Donutbox`; runs with the front end and with `--level` showed no glass at any sampled frame (the player's glass glue never showed it either, instrumented run over 2700 updates of lvl_01 and 600 of the police station).
- **Cause (port):** the resting rule of [§2026-10-16T14:39:48](#ts-20261016-143948) posed a model of its own at frame 0 of its first clip before the start, and `show` made it visible for the whole cutscene. The glass therefore stood from the first update at (741.292, 31.061, 637.666), beside the dog owner at (740.9, 30.0, 636.7), and hung there still until frame 478 (`chase-game` at update 20: glass beside the owner; after the change it is gone, and at update 520 it is in Chase's raised hand).
- **Data:** none of the 330 cutscene clips has the hand item track 0xba1e5b53 (scan with `runtime::handItem`), so the glass in a cutscene is only this prop; the player's glass glue is not involved. No other cutscene folder has a glass prop except `1_4_c/glass` (a window).
- **Original:** the user observed that the glass is not seen in the emulator until Chase pulls it out (2026-10-18, relayed by the lead; no oracle measurement). The code read does not show the gate: 0x2ec454 (per actor step, caller 0x2d19cc) starts a clip when `previous < start <= current` and for a model of its own sets 0x80000000 in the skeleton resource's flags word (`slot+0x208 -> +4`, the same word gates a branch of 0x2d19cc that sets the node's orientation (0x292ca8, 0x20e850, 0x24c118), so it looks like a motion flag, not a visibility bit; not proven); 0x2d19cc attaches the model's node to the room found at its world position (0x239838) on every update; 0x245b40 only starts the animation slot. Nothing read hides the node before the clip. The donut box of `1_1_d` (clip start 100) would be hidden before frame 100 by the same rule; not measured.
- **Port now:** a model of its own is drawn from the update its first clip starts and hidden before (`InEngine::showOwnModel`), stand-in MOVIE-S33. `chase-game` prints how many models of their own are drawn at update 10 of a cutscene; CTest `game-cutscene-glass-hidden` expects 0 for `1_2_a`; `assets-real-data` pins the start frame 478 of 651.

Unknowns:
- The original's gate for a model of its own before its first clip (a flag, the room attach, or the node at frame 0 being outside the view), and whether it applies to every prop; the donut box of `1_1_d` before frame 100 and the glass at frames 0 to 477 of `1_2_a` as pictures from the oracle (needs RPC).
- `chase-viewer --cutscene` still draws such models at their resting pose.

<a id="ts-20261019-125954"></a>

## 2026-10-19T12:59:54+02:00 — A cutscene actor binds to any entity with a matching CSCharacter, props included: the dog of `1_2_c` is the prop `1-2_Dog` (llvm-objdump of 0x2d1ddc and 0x196144 on `artifacts/game/game.arm.elf`, level and cutscene data through the C++ loaders; no emulator, REA tools not used)

Corrected by [cutscene-actors.md §2026-10-19T13:47:29](#ts-20261019-134729): the "End" paragraph is wrong; the teardown 0x2d135c puts the entity node matrix back to the one saved at the setup 0x2d1524, so the entity does not stay at the last pose.

[Corrects objects.md §2026-10-19T09:59:14](objects.md#ts-20261019-095914): the cutscene does bind `1-2_Dog`; the port no longer draws a dog of its own for it. [Corrects script-natives-flags.md §2026-10-19T11:03:27](script-natives-flags.md#ts-20261019-110327): the open question whether `1_2_c` binds `1-2_Dog` is answered.

**Rule (0x2d1ddc, instruction level).** After the `cape` and `player` tests of [§2026-10-15T07:54:38](#ts-20261015-075438) the loop at 0x2d1f88..0x2d1fec walks the scene object's entity array (`[scene+0x24]`, count `[scene+0x1c]`, index from 0) and for every non-null entry reads `CSCharacter` (0x241624, type 0x1000010) and compares it with the folder through 0x25ad90 (case-insensitive); the first equal entry is taken (0x2d1fc0). Nothing in the loop tests the entity's kind, its class, whether it is a character, whether it has a skeleton, or whether it is enabled. So a prop or platform with a `CSCharacter` is an actor like a minifig is. A found entity goes to 0x1f35f4 (slot +0 = 1, entity pointer at slot +0x200, entity byte +0x18 saved to slot +0x1fa and set to 5), as for characters.

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
