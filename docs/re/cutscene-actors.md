# Cutscene actors

Which entities a cutscene's cast are, why the original draws no second Chase and gives the actors their faces and clothes. For the cutscene natives and fades see [cutscenes.md](cutscenes.md).

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
