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
