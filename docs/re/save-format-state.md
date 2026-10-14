# Save format: level state, objects, Continue and the save request

Reverse-engineering findings on the level state blocks, unlock bits, object records, where Continue starts and the save request chain. Split from [save-format.md](save-format.md) (file layout, CRC, options, slot progress, level table); entries are chronological and later entries may correct earlier ones.

<a id="ts-20261014-115300"></a>

## 2026-10-14T11:53:00+02:00 — Level state blocks, unlock bits and the progress natives (llvm-objdump, REA-free static reading of artifacts/game/game.arm.elf; real save copy; oracle `read` on `street-after-1-1`, OpenGL renderer, no RPC)

Replica: `src/save/slot_progress.{hpp,cpp}` (`chase_save_progress`), `ScriptProgress` in `ScriptRunner`, `chase-game --save FILE --slot N` (`src/game_save.hpp`). Tests: `slot-progress` (synthetic), `slot-progress-real-data` (copy of the user's save, skips without it).

### Level block (0x820 bytes, 0x203464; completes the 09:13 entry)

- +0x00 u16[13] category counters, **+0x1a u16 length of the state data, +0x1c u16 StoryProgress at save time, +0x1e u8 "state saved", +0x1f.. 2048 bytes of state data**, +0x81f collected (read by 0x1cd3dc and GetGameComplete). The block of the current scene is found by 0x21d4b8(scene), which matches the scene file name against the TablesLevels scene-name column (+0x30 of rows 1..8; row 1 = lvl_00_policestation, row 2 = lvl_01_cherrytreehills).
- Writer: 0x2e80fc (scene unload) calls the serializer 0x1c26dc(scene, block+0x1f, 0x800), stores the length at +0x1a, sets +0x1e = 1 and stores the scene variable "StoryProgress" (string 0x358354, lookup 0x241624 / 0x2451d0) at +0x1c. The same pair is at 0x192f60 (called from 0x21cf58).
- Reader: 0x2e8054 (scene start) calls 0x192e14(scene) after the scene setup (0x1d9594, 0x1bcf94, 0x197948, 0x29d6e8) and before it starts the scene's threads. 0x192e14: when +0x1e is set it calls the restore 0x1c2400(scene, block+0x1f, length). Then, only for a scene that is not the main scene (compare with *(0x589a18+4)) and whose level has the second unlock bit (below) set, it overwrites StoryProgress with byte +0x41 of the level row (level table `*0x375914`, stride 0x54).
- State data (restore 0x1c2400, save 0x1c26dc): records {u32 name hash of an object, u8 size, 3 bytes, `size` bytes of payload}; the record of hash 0 is the level data; the object records (hash found by binary search in the scene's object array, `scene+0x28`) go to the class's restore callback (class table 0x583124 stride 0x28, callback at +0x24); a final marker 0xffffffff is followed by the bit set of `scene+0x12f4` entries (flag 4 of the mode word 0x376aac).
- Level-data record (hash 0): the default parameters of the level file (section +0x10) with record flag bit 1 (`LevelParam::flags & 2`), in file order, written by value: types 0, 5, 7 four bytes (type 0 float, 5 and 7 integers, the port converts them to float as for the defaults), type 1 a NUL-terminated string, type 2 twelve bytes, types 3, 4, 6 nothing. The mode word has the parameter bit (2) only; the bit sets of bounds (bit 1) are not in the saved data. Checked on the real save: for lvl_01 the record is 219 bytes = 43 floats and integers (172) + the string "lvl_01_CherryTreeHills" (23) + two vectors (24), consumed exactly.

### Unlock bits and the natives (all read the live slot, base 0x51b6b0)

- 0x21d450(level, which): level >= 9 gives 0; otherwise bit `2*level + which` of the bytes at 0x520890 = slot +0x51e0 (byte `(2*level+which)>>3`). which 0 = unlocked, 1 = second state ("completed / visited"). With the cheat flag 0x375961 bit 2 it returns true. The slot defaults 0x1a25cc OR byte +0x51e0 with 0x10, so a new game has level 2 unlocked. `IsLevelUnlocked` (handler 0x2636e4, native table entry name at 0x162338) calls 0x21d450(level, 0) and writes 1.0 / 0.0 to its second argument.
- `GetGameComplete` (0x2635d0): the collected byte +0x81f of level block 8 (0x203464(8)) >= byte +0x40 of TablesLevels row 8 (`*0x375914 + 0x2e0`, value 8): 1.0 else 0.0.
- `IsRedBrickExtraActive` (handler 0x2a6d34, table entry at 0x162304) calls 0x2369a8(index) = bit `index` of the word `*0x374724`. That word is a runtime mask: its only writers are the setter 0x1e8cb4(bit, on) (callers 0x2fe2d4, 0x316ddc, 0x316e24: the menu toggles) and the reset 0x19c4c0 (called at 0x2d544c). No loader copies the slot into it (literal search of 0x374724: 0x19c4d8, 0x1e8cd8, 0x2369c0, 0x2f67b8 only), so after a load it is 0.

### The user's save (copy at artifacts/oracle/agentJ-saves/savegame.dat; the originals under ~/.local/share/azahar-emu/sdmc/Nintendo 3DS/<id0>/<id1>/title/00040000/000ad600/data/00000001/savegame.dat were only read)

- File 42644 bytes, valid (`chase-savecheck`: slot 0 2/711, slot 1 empty), stored CRC 0x937dacf8, options +2 = 0 (last slot 0), slot 0 byte +7 = 4 (in use), slot 1 all zero. The Azahar file layout is `data/00000001/savegame.dat` (plus a 16-byte `00000001.metadata`); the file is the same format as the game's.
- Slot 0: unlock byte +0x51e0 = 0x10 (level 2 unlocked only, no second-state bits); level 1 (police station) block has state data 201 bytes, StoryProgress 0; level 2 block has state data 823 bytes, StoryProgress 2, collected 1. Decoded level-data record of lvl_01: StoryProgress 2, 1-1_IntroPlayed 1, Entered_Police_Station 1, Enter_Police_Car 1, DIALOG_RETURNCODE 2, Music_NumberGen 1, Language_Spanish 2 (type 5; meaning unknown), other saved values 260, 1, 160, 10 and two type-5 values of 1. This is the progress "after mission 1-1".

### Oracle comparison (state `street-after-1-1`, `read 0x08e02284 19280` = the 482 bound records of the level data, GDB, `--renderer opengl`; the software renderer crashed three times at the state load in `LookupTexture`)

- The `chase-game` start with this slot (saved variables + `startup`) gives the same bound enable bytes (+0xa) as the oracle for 479 of 482 bounds; the three that differ (`CameraBound_Police_Helipad`, `CameraBound_VehicleDrop_01/02`) are the known SCRIPT-10 differences. The earlier route (`--call-script 1-1_MissionSkip` after a new-game startup) differs in 5 bounds (1-1_Get_Back_To_Donuts is enabled, `Police_Station_Transition_Upper` is off). Files: artifacts/oracle/agentJ-out/compare/bounds-street-after-1-1-vs-save-route.txt and ...-oracle-read.txt. Object enable flags (entities) were not compared.

Unknowns:
- The object records of the state data (class restore callbacks: doors, pickups, build-its, ...), the end-marker bit set (`scene+0x12f4`, 34 bytes in lvl_01) and what the hub block (level 1) holds besides its level-data record.
- Which level (and position) a Continue starts: the slot's own fields for it were not found (options +0x1ec records are floats; see 08:56). `chase-game` needs `--level`.
- Whether the override of StoryProgress by byte +0x41 (0x192e14) matters for the main scene (it is skipped for it) and what the second unlock bit (which = 1) is set by.
- The mode word 0x376aac: its value is inferred from the saved data (parameter bit only), not read.
- `Language_Spanish` is a saved parameter (type 5) of unknown meaning.

<a id="ts-20261014-134146"></a>
<a id="ts-20261014-134146"></a>

## 2026-10-14T13:41:46+02:00 — Object records, end bit set, current level and the second unlock bit (llvm-objdump on artifacts/game/game.arm.elf; oracle `read` on `street-after-1-1`, OpenGL, no RPC; real save copy)

Code: `objectRecords`, `slotCurrentLevel`, `tasksCompleteVariables` in `src/save/slot_progress.*`; tests `slot-progress`, `slot-progress-real-data`.

- Class table 0x583124 (stride 0x28, index = object type byte +0x12, save callback +0x20, restore callback +0x24) is filled at run time; read at `street-after-1-1`: exactly four classes have callbacks. Callback is called only when the object's halfword +0x82 has bit 0.
  - Type 0x15 (21, `props/prop_buildit`, string "BUILD IT "): save 0x15d380, restore 0x15d348. 1 byte: bit 0 = bit 4 of byte +0x37 of the object's data (+0x7c), bit 1 = inverted bit 0 of the halfword +0x10 of the node at data +0x1c. Restore: bit 0 calls 0x2428c0 (enable), bit 1 with a node calls 0x242cf0 (disable).
  - Type 0x43 (67, `props/prop_prop`): save 0x156550 stores `*(u16*)(obj+0x10) & 1`; restore 0x15654c: bit 0 calls 0x2428c0 (enable). Never disables.
  - Type 0x62 (98, `props/prop_superbuildit`): save 0x192608 stores `!(data+0x92 & 8) & (data+0x92 >> 1)`; restore 0x192548: only when bit 0 is set it sets bit 1 of data+0x92 and runs the enable and disable calls (0x2428c0, 0x242cf0, 0x244f40) and, by data+0x8c, 0x2258bc with 6 or 8.
  - Type 0xfc (252): 4 bytes. Save 0x192804 stores the scene variable "TASKS_COMPLETE" (lookup 0x241624); restore 0x1927a8 sets it back (0x236968) and, if 0x239678(obj) is true, calls 0x244fc8(obj, 0x58, 0). The 4-byte records of a state are all this class.
- Hash of a record = object field +8. For instances placed in a level file it is `nameHash(instance)` (checked: `FP_SuperBuildit_Statue_BP` = 0x91737ed7, type 98). The other records (47 of 48 in level 2, 1 in level 1) belong to objects that no .blvl of the archive lists; at `street-after-1-1` 66 live objects of the level scene (0x085abd20, +0x1c = 3338 objects, array at +0x24; object headers read with `read` for 3333 pointers) carry these hashes: all type 0xfc for the 4-byte records and type 0x62 for the 1-byte ones. Their names and creator are unknown.
- Records are written in the order of the scene object array (+0x24). The first object (index 0) is the level object: its class callback result is the offset before the level-data record, the bounds bit set (mode bit 1) and the parameters (mode bit 2) follow.
- **Mode word 0x376aac is always 2** (read 2 at `street-after-1-1`; its only writer is the setter 0x29444c, called once at 0x2f5d1c with 2). So the bounds bit set (bit 1) and the end bit set (bit 4) are never filled. The serializer still adds `(scene+0x12f4 + 7) >> 3` bytes after the end marker (0x1c2984 is reached by both paths), so the 34 bytes (level 2) and 7 bytes (level 1) are zero padding (the buffer is cleared by 0x24d234 first); the restore skips them with the same count only when mode bit 4 is set and does not read them. Nothing to apply.
- User's save, slot 0: level 2 holds 48 object records (6 of 1 byte, 42 of 4 bytes), level 1 holds 1 (1 byte); every payload is 0. Applying them changes nothing: a 4-byte record sets TASKS_COMPLETE to 0.0, a 1-byte record with bit 0 clear is a no-op for types 0x43 and 0x62 and for type 0x15 sets nothing visible.
- **Slot byte +6 = current level id** (signed byte 0x5163e8+0x26, the level of the running game). Writers: 0x1947e0 (called at 0x20a520 when a level is entered, together with `bit 2·level` of the unlock set for levels 2..8), 0x2f68ec, 0x2e14c0, default 0x1a2608. Readers 0x2b1100 and 0x2e7d50 copy it into a front-end structure, then call 0x217f88(0). The user's save holds 1 (police station), matching its level-1 block with saved state. Slot +4 (u16) is the completion per-mille written by 0x17cb3c / 0x23cbe8 (0x1cd3dc); +7 bit 2 = in use. No position or vehicle field exists in the slot.
- **Second unlock bit (`which` 1)**: the only code that sets a bit of the set at 0x520890 is the setter 0x1b2098(level, which) (bit `2·level + which`) and 0x1947a0 (`which` 0). Both callers of 0x1b2098 (0x1b2780, a script native taking one level argument, and 0x2f68e0) pass which = 0. No other literal reference to 0x520890 exists (0x1947f0, 0x1b20b8, 0x21d4b0 only), so no code writes `which` 1 and the 0x192e14 override of StoryProgress by level-row byte +0x41 never fires in a game played from a new save. (Slot data copied from a file could in theory carry the bit: not seen in the real save.)

Unknowns:
- Names, creator and meaning of the type 0xfc and 0x62 objects behind the unmatched hashes (`TASKS_COMPLETE` with 42 objects in level 2).
- Behaviour of type 0x15 and 0x43 records with bit 0 set (never seen set: their objects had flag +0x82 bit 0 clear in all 3333 headers read).
- Whether the front end starts the level in slot +6 on Continue and at which position (the copy at 0x2b1100 and 0x2e7d50 was read, its consumer not followed).
- What `scene+0x12f4` counts (0 at `street-after-1-1`, 265..272 when the user's save was written).

<a id="ts-20261014-151903"></a>

## 2026-10-14T15:19:03+02:00 — Where Continue starts: level of slot byte +6, entered at Player1Start (REA/Ghidra on artifacts/game/game.arm.elf; oracle with the user's save copy, Old 3DS, OpenGL, no RPC)

Resolves the last unknown of [save-format.md §2026-10-14T13:41](#ts-20261014-134146). Code: `src/save/continue_start.*` (`continueStart`), glue `game_save::chooseContinueLevel`; tests `slot-progress`, `slot-progress-real-data`, `game-save-continue`, `game-save-continue-empty-slot`, `game-options-core`.

- **Chain (REA pseudo code and assembly).** The slot-list handler 0x2e7ce8 (the reader at 0x2e7d50 is inside it; 0x2b1100 is the same code in a second handler) reads row byte +0x2d of the list. Value 0 (an existing slot): copies the 8 bytes at 0x5207f8 to 0x593418, stores slot byte +6 (0x51b6b6) at 0x3741d5 (front-end structure 0x3741d4, +1), calls 0x217f88(0), sets byte +0x2f of the structure at 0x3741dc + 8 and calls 0x20eec4(1, 0). Value 1 (New Game) calls 0x217f88(0x514fc0) instead. The frame update 0x2d5874 sees byte +0x2f, clears it and calls 0x20a4d4(slot 0x5163e8, level 0x3741d5). 0x20a4d4: level 0 becomes 2; byte 0x51640e = level (0x51640d = level when it is above 8, else 0); copies the unlock words 0x5207f8 and 0x520840 to 0x593418 and 0x593430; calls 0x1947a0 (unlock bit which 0) and 0x1b1c90 (character group from slot +0x51e1.. or TablesLevels row +0x43..0x4a); starts the level load 0x25d26c(0.5, 0.5, slot, 2).
- **No position is saved or read.** The level start function 0x2e6264 puts the player at the level's own object "Player1Start" ([player-entity-input.md §2026-10-13](player-entity-input.md)); Continue only chooses the level.
- **Oracle.** The user's savegame.dat (slot 0 used, byte +6 = 1, per mille 2, after mission 1-1) was copied into the sdmc of the agent's own Azahar home (`artifacts/oracle/agentC-home`; the user's Azahar was not touched). Booted, A on the HOME notice and the autosave notice, state `savecont-slotlist-user` at the slot list (mode row pointer 0x372f2c, slot bytes at 0x51b6b0 read +4 = 2, +6 = 1, +7 = 4). A on the slot: 0x51640e = 1 (level 1), after 40 s free run the player entity is at world (-2.171, 1.432, 17.282), forward (-0.017, 0, -1), camera yaw 3.1416 = Player1Start of lvl_00_policestation (-2.1713, 1.4203, 17.2821, yaw 180.9585) with the ground snap (+0.012 in y); run twice, same values. chase-game `--save` without `--level` prints the same position (`player at -2.1713 1.4203 17.2821, yaw 180.958`).
- **Poke attempt.** Writing byte 02 into 0x51b6b6 at the slot list changed nothing: the live slot is overwritten from the loaded save when the slot is chosen (read back 1 after A, level 1 again). A level-2 Continue was therefore not measured.
- Boot with this save: after 20 s with no input the boot sequencer was on row 4 (row pointer 0x372f04), A moved to row 5 (0x372f18), A to the slot list (0x372f2c, row 6); the first-run byte [0x372e6c] read 1.

Unknowns:
- The start of a Continue at level 2 or later (heading and position, whether level scripts move the player by StoryProgress): only level 1 was measured, the pinned save has no other level in byte +6. Needs a save or state of the user with byte +6 above 1.
- What the slot list shows for a used slot (time, percent, location rows, delete) and how the cursor moves; no screenshot was taken (the RPC belongs to another agent).
- What 0x217f88 does with its argument (stores it at [0x5163e8-like object]+4 and calls a virtual); 0x514fc0 as the New Game argument.
- Why the first-run byte read 1 with a save whose options last-slot byte is 0.

<a id="ts-20261014-163403"></a>

## 2026-10-14T16:34:03+02:00 — Save request chain, slot commit and the slot row fields (llvm-objdump on artifacts/game/game.arm.elf; no emulator)

Resolves the writer part of [script-natives-progress.md §2026-10-14T14:41](script-natives-progress.md#ts-20261014-144155) (SAVE-10) and the data side of SAVE-2. Code: `src/save/save_writer.*`, tests `save-writer`, `save-writer-real-data`, `game-options-core`.

- **0x21cf38(0)** (QuickSave, SetRaceWon, AwardRacingCar, GiveReward): sets byte 0x5163e8+0x34 = arg, calls 0x17caf8 (summary), for a level id (signed byte 0x5163e8+0x26) of at most 8 (unsigned compare) calls 0x229740 (formats "levels/%s/main.lvl.world" from the level row +0x30 and hands it to 0x2baddc; purpose unknown) and 0x192f60 (level state, below), then queues 0x2400a0(0x1a5248, 0x261a1c, 0). 0x261a1c starts a level transition only when byte +0x34 is set, so the request with 0 starts none.
- **0x17caf8 (summary).** Copies the 8 bytes at 0x593418 to slot +0x5148, bits 2..4 of slot +0x520b from (byte 0x519760+0x40) << 2, the u32 at 0x593418+0x18 to slot +0x5190, and stores the completion per mille (0x1cd3dc) at slot +4. 0x23cbdc does only the last step.
- **0x192f60 (level state).** Finds the level block (0x21d4b8), serialises the live objects with 0x1c26dc into block +0x1f (2048 bytes max), stores the length at +0x1a, sets +0x1e = 1 and StoryProgress at +0x1c (0x2451d0). It needs the live object system and is not ported.
- **Request handler 0x1a5248, case 0.** Checks 0x2a0568: the signed byte at +0x3c of the object at 0x376844 (active slot) is >= 0 and below the u16 slot count; if not it does nothing. Otherwise it allocates a task (0x2ccf4c, template 0x2c8e2c) with options flag 1 and the slot index. Task state 0 (0x2c8e5c): copy the live slot (0x228008) into the buffer at the slot's file offset (0x1d0ba0), store the slot index at live options +2 (0x22801c), copy the live options over the buffer's options (0x1d0b80, when the flag is set), write the header and CRC (0x1fc604), turn the save indicator on (0x24004c) and start the I/O (0x2281c8(3), mode 3 of the save thread). State 1 polls 0x228058 and reads the result 0x228044; state 2 turns the indicator off. The front-end save page runs the same sequence at 0x2b63a0.
- **Slot row builder 0x2cc298** (per slot, label "Slot N"): row fields are u32 [slot+0], u16 [slot+4], byte [slot+6]; +7 bit 2 set = used. Text_Percent from the per mille p (p = 1 shows as 0; 1000 gives "100%" by format 0x358a58, else "%u%s%u%%" with p/10, the separator "." for language 1 else "," (0x358a50, 0x358a54), p%10). Text_Time from [slot+0] as 1/30 s ticks: hours = (t+1)*0x136b06e7 >> 45, minutes by 0x91a2b3c5, seconds by 0x88888889, format "%u:%02u:%02u" from one hour, "%02u:%02u" below, "999:59:59" from 1000 hours. Text_Location is the loc text of the hash at TablesLevels row [level] column 11 (runtime row stride 84, +8), level = byte [slot+6], 0 shows row 1. The ticks counter is incremented by one per update by the loops at 0x2d8a30, 0x2e5338, 0x2f26e8 and 0x300444.
- **Verified on the user's save copy** (`save-writer-real-data`): slot 0 row = 10462 ticks (05:48), per mille 2 (0.2%), level byte 1 (location hash 0x99a49618 = row 1 column 11); recomputing the per mille from the slot gives 2; committing slot 0's own bytes through `commitSlot` reproduces the file byte for byte (header, CRC, options +2). No screenshot of the original's row was taken, so the texts are the code's, not compared with a picture.

Unknowns:
- What 0x2baddc does with the world path, and whether the queue 0x2400a0 coalesces several pending requests (the port writes once per update after any new request).
- The values at 0x593418 (+0, +0x18) and 0x519760+0x40 in the running game (studs, stud total, a display setting?).
- The level state serialiser 0x1c26dc and the StoryProgress source 0x2451d0 (SAVE-14).
- Whether the row builder reads other TablesLevels columns than 11 (location) for the row; none seen in 0x2cc298.
