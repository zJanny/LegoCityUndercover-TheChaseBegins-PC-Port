# Level scripts and the start state of placed objects

How the level's own scripts decide which placed objects are disabled when a level starts. Format of the level file: [levels.md](levels.md). Collision consumer: [collision.md](collision.md).

<a id="ts-20261013-204236"></a>

## 2026-10-13T20:42:36+02:00 — Script data, executor, EnableObject, the startup script and the revealer links (REA Ghidra 12.1.2, llvm-objdump, Azahar oracle port 24692 without RPC)

Replica: `Level::scripts` / `objectReferenceHash` (src/assets/level.cpp), `levelStartState` (src/runtime/level_start.cpp), `PlacedObjectState` and `objectCollisions` (src/assets/object_collision.*). Tests: `level-start-real-data` (tests/level_start_real_test.cpp), `collision-real-data`.

### Script data (level section +0x18)

- Loader 0x1cfc50 (ev_1eed109a…f773): per script a 0x1c-byte record: u32 name hash at +0 (`nameHash` of the lower-cased script name, for example "startup" = 0xe48e50f6), u16 call count at +6, u32 argument count at +0x14. Blocks per script: the call table (8 bytes per call: u16 function index, u16 argument count, u32 pointer), the argument pointer table, then one string block per argument, then (pool -16) one function name string per call. The function index in the file is ignored by the loader: it re-derives it from the name with 0x208a3c (ev_cd34ae96…a091), the hash lookup in the function table (see below). lvl_01 has 314 scripts, lvl_00 fewer (not counted).
- Argument strings keep their quotes when they are literal strings (`"=="`); numbers are plain decimal strings; everything else is a variable or object instance name.
- Calls are native functions or "script words" that the executor treats the same way (they sit at indices 0..7 of the same table): `?` if (index 0), `!?` else (1), `?!` else-if (2), `??` end if (3), `=` assign (4), `=+` arithmetic assign (5), `CallScript` (6), `SpawnScript` (7).

### Function table and executor

- The function table is a heap array of 16-byte entries {u32 name hash, u32 handler pointer, u32 info (parameter count and type bytes), u32 0}; pointer at 0x376ca4, count u16 at 0x376ca8 (377 at gameplay-start; table at 0x083599cc, read with `read`). The order is the order of `artifacts/fib/files/levels/scriptfns.txt`, which also documents every native with its argument types. EnableObject is entry 0x138 (handler 0x1ddd00), EnableCollision 0x13c (0x27e9f4), Wait 0x137 (0x174604), IsLevelUnlocked 0x105, GetGameComplete 0x130 (0x2635d0), CallScript and SpawnScript 6 and 7 (both 0x1c3730).
- Executor 0x234204 (disassembly): thread record r7: +0x10 the script, +0x14 flags (bit 0 stop, bit 1 else-if pending, bit 2 blocked), +0x16 the program counter. It loops while pc < call count and flag 0 is clear: it evaluates the call's arguments with 0x211e04 (ev_3af9fd1a…8a1c: type 0/10 number or variable, 1 string, 4 object, ...) and calls the handler; a handler result of 0 sets the blocked flag and keeps the pc (Wait and the other yielding natives), otherwise pc advances.
- If 0x16354c: operator strings at 0x3595a4.. are `<`, `<=`, `==`, `!=`, `>=`, `>`, comparing two floats. A false condition scans forward: an if raises the depth; at depth 0 an else sets pc to it (the executor then runs past it), an else-if sets pc to the instruction before it and the thread flag 2 (so the else-if is evaluated as a condition), an end if sets pc to it. Else 0x174428 and an else-if reached without the flag scan to the matching end if (they count ifs only). End if returns at once. Assign 0x17efc0 copies a float; arithmetic assign 0x17ef30 takes `+ - * /` from its third argument.
- CallScript / SpawnScript 0x1c3730: create the thread of the named script (0x22ce9c), evaluate its arguments, set the parent link (+0x4c) and block the parent for CallScript, and run the new thread at once with 0x234204. Scripts that wait therefore continue in later updates.
- Variables: a condition names a float of the level's default parameter table (section +0x10; `Level::defaults`, type 0 floats, also types 5 and 7). `StoryProgress` is a default of every `lvl_0N_*_main.blvl` (lvl_01: 1.0, lvl_02/03: 2.0, lvl_00: 0.0); the code reads it in 0x1a07d4 (ev_df87e714…8e) through the same lookup. A new game therefore starts lvl_01 at StoryProgress 1.

### EnableObject and EnableCollision

- EnableObject 0x1ddd00 (ev_21ab8ae2…): value 0 calls 0x2428c0 and sends message 0xfb (0) to the object (0x2041dc); any other value calls 0x242cf0 and sends 0xfb (1). 0x2428c0 returns when entity flag bit 0 (u16 at entity+0x10) is set, otherwise sets it, disables the sub-objects (flag 2) and the scene node, and removes the object from the scene queries. 0x242cf0 clears bit 0 again. The player and camera query layers reject owners with flag bits 0 or 1 ([collision.md](collision.md)), so a disabled object does not collide.
- EnableCollision 0x27e9f4: value 0 clears entity flag 0x200, otherwise sets it and registers the entity with the world grid (0x2ba3f0). 1-1_Mail_Box has BoundCollision 0 (flag 0x200 cleared by 0x23aff8) and the startup script sets it again: the grid read has flags 0x740 for it.

### The startup script

- Every level file with a script called "startup" runs it at level start (lvl_01: script 305, 339 calls). It calls `1-N_MissionHide` (disables the mission props of every chapter with EnableObject 0), then `1-<StoryProgress>_MissionStart` and `1-K_MissionSkip` for the finished chapters, `EnableObject` 0 for freeplay props that stay off (hydrant sprays, tokens, `FP_*` pieces) and tests such as `IsLevelUnlocked(6, ApolloIslandUnlocked) == 0` to disable the astronaut tasks. The state the oracle shows at gameplay-start is the result of this script run once in call order with CallScript and SpawnScript depth 2 (14 script runs), waits treated as elapsed and the unlock/complete natives returning 0.
- The unlock natives write their last argument ("Attrib gets set to 1 - unlocked or 0 - locked", scriptfns.txt). With 0 the sim reproduces the grid for the astronaut tasks (flag 0, `FP_Task_ASTRONAUT_CTH_01..04`) and `Fast_Travel_Ferry`; a different value would disagree.

### The revealer links

- Prop classes keep object references (parameter type 4, block {u32 0, u32 target name hash, u32 0, u16 2, ...}) to the objects they build or reveal: `BuilditFinalProp` (props/prop_buildit, leprops/leprop_buildit; set in 0x16459c, ev_03d09ec2…, which stores it at +0x1c of the class data), `PropToReveal` (props/prop_superbuildit), `RevealObject` (props/ability_detective_reveal_*). The name hash is the CRC-32 of the lower-cased instance name.
- Rule (data, 0 counterexamples): every target is disabled (entity flag bit 0) at gameplay-start and shown by the build-it state machine. Example: 1-3_BI_Seesaw → 1-3_Seesaw. The code that sets the flag for the targets was not located (the build-it state change 0x1650c8 acts on the +0x1c target in state 2 only); the rule is the data fit below, so it is evidence-backed but not traced.

### Oracle comparison (state gameplay-start, scratch scripts, nothing committed)

- Read: grid entry pool 0x56defc (1600 entries, owner pointer at +0x10), 1415 distinct owners, each entity's 0x100 bytes and scene node (world translation at node+0x8c). Entities were matched to level objects by world position (1292 owners, 1133 objects); 32 objects could not be separated from a co-located twin by class/type byte (+0x12). Entity flags u16 at +0x10: 1200 owners have bit 0 clear, 215 set.
- Over the 896 objects that would register collision, the state rule gives: 819 enabled in both, 48 flag-0 in both, 4 without any entity and disabled by the script, 12 flag-0 only through the revealer links, 8 co-located twins (for example 1-2_Dumpster_Open flags 0x741 and 1-2_Dumpster_Closed 0x740), and no object that the rule disables while its entity is enabled. The old 85-name list is covered except `1-2_Dtv_DogPath_XSpot` and `1-3_Dtv_BrickSpot_XSpot_02` (detective X-spots without grid entry, not collision candidates).
- Objects disabled by the script but with an enabled entity exist outside the collision candidates (the hydrant sprays, `Pickup_CharacterToken06/07/10`, `FP_Task_COP_CTH_02..05`, `FP_Archway_GrapplePad01`, `1-3_BI_Seesaw`, `1-3_BI_Roundabout`): another script or class handler enables them again later, or the class ignores message 0xfb; not investigated. None of them is among the 896 collision candidates, so the replica is not affected, but `levelStartState` lists them as disabled although the original has them enabled.

Stand-ins and approximations (stand-ins and open items: [features/script-system.md](../plans/features/script-system.md))
- Waits and every blocking native count as elapsed; SpawnScript runs the child to its end before the parent continues (the original runs it until it yields); only the "startup" script is run; no script runs after the start (no mission events, triggers, bounds).
- `IsLevelUnlocked`, `GetGameComplete`, `IsRedBrickExtraActive` return 0 (new game, no save). A loaded save would change the state (the code supports it; the port has no save-aware start yet).
- Revealer rule is a data fit; verified only in lvl_01. lvl_00's start state was not compared with the oracle.

Unknowns
- Which of the disabled-by-script objects with an enabled entity are re-enabled by another script (hydrants, tokens, `FP_Task_COP_CTH_*`) and why `1-3_BI_Seesaw` / `1-3_BI_Roundabout` (build-it bits) are enabled while `MissionHide` disables them.
- The code that hides the revealer targets; the role of the other type-4 parameters (they link objects: Destination, ObjectTo, TriggerObject, ...).
- Variables of type 5 and 7 in conditions (converted to floats here; the executor compares floats).
- Hydrant_Spray_01 appears twice in the startup script; the other mechanism that keeps it enabled is not known.

<a id="ts-20261014-095252"></a>

## 2026-10-14T09:52:52+02:00 — Per-update script runner, thread record, Wait, CallScript / SpawnScript, script starters and the trigger path (llvm-objdump, REA Ghidra 12.1.2, Azahar oracle port 24701 without RPC)

Replica: `ScriptRunner` (src/runtime/script_runner.cpp), `runLevelStartup` / `levelStartState` (src/runtime/level_start.cpp), hook in `chase-game` (src/game.cpp). Tests: `script-runner` (tests/script_runner_test.cpp, synthetic), `level-start-real-data`. REA evidence: ev_8cc8870b… (0x1746c0, 0x17acbc, 0x1ecc50, 0x2f67fc), ev_9d4fee32… (0x197c3c, 0x245258, 0x2b4a50, 0x1964dc), ev_f23162d2… (0x2388e4, 0x23c188, 0x234124, 0x224eb4), ev_b2f65ae3… (0x1cddf8), ev_27b1b3b4… (0x16babc, 0x15b888), ev_e8434d21… (0x23c41c, 0x23cee8, 0x28aef4), ev_d5b25508… (0x21a560, 0x201a54, 0x23cff4). The executor, Wait, CallScript and EnableTrigger were read with llvm-objdump (`--mattr=+vfp2` for the float code).

### Runner

- The gameplay mode update 0x2e5158 ([streaming.md](streaming.md)) loops over the scene array 0x589a18 (count +8, array +0xc) and calls 0x169254(scene) at 0x2e54b4 for each. 0x169254 walks the list whose head is scene+0x1310 (node {next +0, prev +4, data +8}); it reads `next` before it calls the executor 0x234204(thread), so a thread may end (unlink) during its own run. A second caller exists at 0x2d1d6c (not identified).
- Oracle (state gameplay-start, `trap 0x169254,0x234204,0x174604 3`): 0x169254 is entered twice per update (r0 0x08501f30 and 0x085abd20, the two resident scenes, lr 0x2e54b8); 0x234204 and Wait 0x174604 are never entered in 3 updates. The heads scene+0x1310 of both scenes are 0 (`read`), the free list 0x589734 is not empty. The same trap at the `police-station` state (4 updates) also gave no executor stop. So no script thread is alive at either checkpoint: the startup threads have ended and nothing else is waiting.

### Thread record (0x50 bytes, created by 0x22ce9c)

- +0 next, +4 prev, +8 self (list node), +0xc owner (a level data owner; owner+0x20 is its scene, owner+0x24 its level data), +0x10 script record, +0x14 u16 flags (bit 0 suspended, bit 1 else-if pending, bit 2 blocked), +0x16 u16 pc, +0x18.. argument slots (8 bytes each, value pointer at +0x1c + 8i pointing to +0x38 + 4i), +0x48 Wait timer (float), +0x4c parent thread.
- 0x22ce9c(owner, script): takes a record from the free list 0x589734; when the list is empty it ends the first thread of the owner's scene whose script has the same name hash or the name "EffectScript" (0x359144) and reuses it. The record is zeroed (0x2594d8, 0x50 bytes), +0xc owner, +0x10 script, pc 0, and inserted at the head of scene+0x1310 (0x25cf74 inserts at the head). An optional hook at *(0x376cb0) is called with it. Pool size not read.
- 0x224eb4(scene, name, strict): the first thread of the list with that script name hash counts as running while pc < call count (and, when strict, flag bit 0 is clear).

### Executor 0x234204 (exact)

- `while (pc < count && !(flags & 1))`: evaluate the declared arguments (count byte at function info +8) with 0x211e04, call the handler; result 0 → `flags |= 4` and leave the loop; otherwise `pc++` and, if `pc < count`, `flags &= ~4`.
- After the loop: if `pc >= count` and bit 0 is clear: parent (+0x4c) flags `^= 1`, +0x4c = 0, unlink (0x25cf98), push to the free list 0x589734 (0x25cf74).
- A blocked thread is not skipped by the runner: the same call is made again every update until its handler returns non-zero. Bit 2 tells the handler that it is a repeat call.

### Wait 0x174604 (exact)

- First call (bit 2 clear): +0x48 = argument 0 (seconds), return 0. Repeat calls: +0x48 -= dt (0x23602c, the update step), return 1 when +0x48 <= 0 (vcmpe, `ls`). So Wait 0 yields once and ends in the next update; Wait t ends in the first update where the sum of the dts after the first call reaches t.

### CallScript / SpawnScript 0x1c3730 (exact)

- 0x22ce9c creates the child (its arguments are copied into the slots); for the CallScript function index (`*(0x376cc0+0x1c)`): child+0x4c = caller and 0x217b50(caller, 1) sets caller flag bit 0. Then the child runs at once (0x234204) until it yields or ends; the handler returns 1 (caller pc advances). If 0x22ce9c returns 0 the handler returns 0 (the caller retries next update).
- Consequences: a CallScript caller stops after the call until the child ends; a child that ends without yielding clears bit 0 again inside the same executor loop, so the caller continues at once. A child that yields sits at the head of the list, ahead of its parent, so when it ends during a runner pass the parent continues later in the same pass. SpawnScript sets no parent: the caller continues in the same update.

### What starts threads

- Scene start 0x197c3c(scene) (from 0x2f67fc, the level enter): clears the list, then walks the owner array scene+0x24 (count +0x1c) from the last to the first owner; for each owner whose level data has a script named "startup" (0x2388e4 looks a name hash up in owner+0x24 → script table +0x18, count u16 +6) it creates a thread and runs it at once. Owner 0 (0x23c188(scene)) is the level itself. Class files have scripts too: 72 `.blvl` files carry scripts, 64 of them a "startup" (for example props/prop_sbsuperjump, vehicles/vehicle_fire_douser; census with the C++ parser), so placed objects of such classes run their class's startup at scene start. At gameplay-start scene 0 had 0xd7 owners and scene 1 0xa0d (`read` of +0x1c).
- Events 0x1ecc50(scene, name): formats "%s%d" with the scene's u16 +0x2c, starts that script of owner 0 at once, then the scripts with the plain name in the scene's rooms (list +0x40, 4 buckets each). Called with "OnSceneEnter" (0x2e6888) and by the scene switch 0x303480 (OnSceneExit / OnSceneEnter). The gameplay update starts "ScriptCheckExtras" (0x358248) at once when byte +0x35 of a mode object is set (0x2e5638..0x2e568c). Fade-in waits while "startup" or "OnSceneEnter%d" runs (0x2b4a50 and 0x245258 call 0x224eb4 strict).
- Triggers: the pending queue scene+0x12fc (count) / +0x1300 (0x24-byte entries {trigger record, owner entity, ?, instigator}) is filled by 0x21a560(trigger, instigator, ...): only when the trigger's enable byte +0 is set, its owner entity is not disabled (entity flag bit 0) and its script is not already running (0x18a89c), unless forced. 0x1746c0(scene), called by the gameplay update at 0x2e56c0 after the runner pass, first calls the check function of every enabled trigger of the scene array +0x12f4 / +0x12f8 whose owner is enabled, then creates the queued threads (from the last entry) with 0x22ce9c(trigger+0xc, trigger+0x10) without running them; they run first in the next update's runner pass.
- Trigger types: 0x16babc and 0x15b888 register the check functions by event name (0x23c41c sets type+0x28): BoundEntered 0x28aef4, ObjectBoundEntered 0x2bc67c, BoundLeft 0x26a20c, ObjectBoundLeft 0x2aae30, BoundInside 0x282028, BoundOutside 0x28af68, ObjectBoundInside 0x2b7ecc, BoundBothEntered 0x2b0cd0, Player1BoundEntered 0x2c185c, Player2BoundEntered 0x2c194c, CharacterCulled 0x2aadc8, PlayerVehicleBoundEntered 0x2cb578, PlayerOnFootBoundEntered 0x2ca6b8; the others (ObjectTriggered *0x3775d0, ObjectUntriggered *0x3775d4, Reset, AIMove, PlayerVehicleEntered, ...) are fired by code through 0x23cee8(type, object, ...) → 0x23cff4 (the trigger of that type and owner in the scene, also searched in the master scene) → 0x21a560.
- BoundEntered 0x28aef4(trigger): `who = 0x201a54(trigger+0xc, trigger+0x14 bound)` (the player 0x3772c0 when bound bytes +0xa and +0xb are both set and the player's position, moved into the owner's node space, is inside the bound by 0x22f650); fires 0x21a560(trigger, who, 0xffff, 0, 0, 0) when trigger+1 is 0, who is set and the previous state +0x1c was outside; then +0x1c = inside, +1 = 0.
- EnableTrigger 0x268164: argument 0 resolves to a trigger (value +8); its enable byte +0 = (argument 1 > 0).
- File trigger record (section +0x1c, loader 0x1cfc50): +4 type (0x18a9c0 of the event name), +8 object name, +0xc script name, +0x10 / +0x14 the two settings (a string, or resolved by 0x215a58 when the type's setting is not a plain string, 0x1a5530). Bytes +0..+3 not decoded.

### TriggerObject 0x1cddf8 (exact)

- Value 0: message 0xfe to the object (0x244fc8) and the ObjectUntriggered event (0x23cee8(*0x3775d4, object, object, 0xff, 0)); otherwise message 0xff and ObjectTriggered (*0x3775d0). Returns 1 (never blocks). Doors answer 0xff / 0xfe in 0x1749b8 ([doors.md](doors.md)).

### Native table (oracle `read` of the table at 0x083599cc, 377 entries; name hash = CRC-32 of the lower-cased name)

- TriggerObject 0x25 → 0x1cddf8 (info 0x802), AIMove 0x26 → 0x171f7c, SceneChange 0x2f → 0x1b262c, StartDCam 0x14 → 0x194fd8, WaitForDCam 0x19 → 0x1883ec, WarningDialogBox 0x10a → 0x2700d4, EnableTrigger 0x135 → 0x268164, EnableBound 0x136 → 0x1d0ce8, Wait 0x137 → 0x174604, EnableObject 0x138 → 0x1ddd00, DCamCutSceneWait 0x172 → 0x288a40.

### The police front doors (lvl_01)

- `Police_Door_01/02` are opened only by the script `Police_ExtToInt` (name hash 0x2a5fbbbf): WarningDialogBox → DIALOG_RETURNCODE; on 2 it moves the player (PositionObject, AIMove, StartDCam), `TriggerObject Police_Door_01/02 1`, `Wait 3`, `TriggerObject ... 0`, WaitForDCam, then `SceneChange Police_ExtToInt`. Its triggers: BoundEntered on bound `Police_ExtToInt` and PlayerOnFootBoundEntered on bound `Police_Station_Transition_Lower`; `Police_ExtToInt_SwitchOn` (BoundLeft) re-enables it, and `1-1_MissionStart` disables `Police_ExtToInt_SwitchOn`.
- Oracle: from gameplay-start, the player poked to (904.4, 16.58, 1114.4) and walked with stick (-131, 72) for 20 updates to (905.613, 16.568, 1116.406), inside `Police_Station_Transition_Lower` (centre 907.242 18.2292 1115.83, half 2.0466 2 2); a trap of 0x234204, 0x174604 and 0x1cddf8 for 3 updates after that had no stop. So the trigger did not fire there at that point of the story (trigger or bound disabled, or the bound test uses another point); not resolved.

### Port behaviour (chase-game)

- `levelStartState` runs "startup" through `ScriptRunner` and steps it with dt 0.034 until no thread is left: 326 updates for lvl_01 (Wait 1 in `1-1_MissionStart`, waits of `1-1_OnScreenText_Script`); the disabled set is the same 249 names as before. `chase-game` steps only until "startup" has ended (30 updates), then runs the remaining threads in its own updates, and sends TriggerObject 0xff / 0xfe to the doors of the same name.
- Correction of the 2026-10-13T20:42 entry: its first stand-in (waits count as elapsed, SpawnScript runs the child to its end, only "startup" runs) no longer describes the port; waits now take their time and SpawnScript returns when the child yields.

Stand-ins and approximations (IDs: [features/script-system.md](../plans/features/script-system.md))
- Start steps use dt 0.034 (the free-running dt read at gameplay-start), not the loading phase's own updates.
- Functions without a handler continue at once; the original may block in them (WaitForDCam, DCamCutSceneWait, WarningDialogBox, AIMove, ...).
- A CallScript or SpawnScript of a missing script continues (the original's behaviour with a null script is not known).
- Only owner 0 (the level) is modelled; class startups, events and triggers start nothing.

Unknowns
- Which natives can return 0 (block); only Wait and the words were read.
- The runtime trigger record builder (where the enable byte +0 comes from), bound bytes +0xa / +0xb and EnableBound 0x1d0ce8, the box test 0x22f650, and why the PlayerOnFootBoundEntered walk above did not start `Police_ExtToInt`.
- Order of the scene owners (+0x24) against `Level::objects`; thread pool size; the second runner caller 0x2d1d6c; the script argument slots (+0x18..) and how a script reads them.
- Where in 0x2e5158 the runner sits relative to the player and camera updates (`chase-game` runs scripts after the doors).

<a id="ts-20261014-100640"></a>

## 2026-10-14T10:06:40+02:00 — Trigger records, bound switches, the bound test and the police entrance explained (REA Ghidra 12.1.2, llvm-objdump, Azahar oracle port 24701 without RPC)

Replica: `ScriptRunner::checkTriggers`, `EnableTrigger`, `EnableBound`, `setRoomLoaded` (src/runtime/script_runner.cpp); `LevelTrigger::nameHash`, `LevelBound` node / shape / enabled / active / room, `Level::boundMatrices` (src/assets/level.cpp). Tests: `script-runner` (synthetic), `level-start-real-data` (oracle values below). REA evidence: ev_45c3f2fb… (0x22f650, 0x26a20c, 0x18a89c), ev_5f5e3c50… (0x2c185c, 0x24255c, 0x257f14), ev_05c9f4b1… (0x1dd380, 0x23e798, 0x21b3fc), ev_3549f20e… (0x250810), ev_ae423cb4… (0x229a40), ev_47524e83… (0x173944), ev_3af9fd1a… (0x211e04). Corrects the unknowns of the 09:52:52 entry about the trigger enable byte, bound bytes +0xa / +0xb, EnableBound, the box test and the police walk.

### Records (oracle `read`, gameplay-start)

- Level root owner 0x0870f6f0: +0x20 scene, +0x24 level data 0x08618720, +0x34 runtime trigger records 0x08eb22e0, +0x3c node 0x0870f790 with an identity world matrix (translation 0, 0, 0). Level data header: n18 314 scripts, n1c 241 triggers, n24 482 bounds; +0x18 scripts 0x08618a18, +0x24 bounds 0x08663354.
- Runtime trigger record (0x20 bytes, one per file trigger, same order; 241 of 241 matched the file's script by table index): +0 enabled, +1 skip, +2 u16 index, +4 type, +8 owner object, +0xc owner (level root), +0x10 script, +0x14 setting (the bound for bound events), +0x1c inside at the last check. ObjectTriggered records keep skip 1 (never checked); bound records have skip 0 after their first check.
- File trigger record +0 is the trigger's name hash (`Police_ExtToInt` = 0x2a5fbbbf); EnableTrigger's argument resolves through 0x1dd380: the first file trigger with that hash, then the runtime record at owner+0x34 + 0x20 * index. Not every trigger name equals its script name (`PlayerOnFootBoundEntered` → `Police_ExtToInt` has hash 0x713159aa).
- File bound record (0x28): +0 name, +4 room name (optional block), +8 node index (0xff none), +9 shape, +0xa enabled, +0xb active, +0x10 centre, +0x1c half extents (radius for shapes 2, 4, 6). In lvl_01 every file record has +0xa = +0xb = 1. EnableBound 0x1d0ce8 writes +0xa = (value > 0); the bound argument resolves through 0x23e798 (case-insensitive name compare 0x25ad90, first match). Section +0x28 holds 0x40-byte matrices (98 in lvl_01, highest bound node 97).
- Bound byte +0xb: the room setup 0x173944 sets it to 1 for every bound, then to 0 for each bound whose room name (+4) equals a room of the scene (room bound list +0x4c). Oracle: the 225 bounds with +0xb = 0 are exactly the bounds of rooms not loaded; the 257 with 1 are the bounds without a room plus those of Room 1, 2, 3, 7, 8, 9, 14, 15, 16, 17, 22, 23 (the loaded set of [streaming.md](streaming.md)).

### Checks

- 0x22f650(point, bound, level data): only when +0xa is set; p = point − centre; with a node index p is turned by the rows of the matrix at level data +0x28 + 0x40 * node (0x229a40, no translation); shape 2: |p| < r; shape 3: |x|, |y|, |z| below the half extents; shapes 4 and 6: sqrt(x² + z²) < r and |y| < half y.
- Player1BoundEntered 0x2c185c and BoundEntered 0x28aef4 (through 0x201a54): point = player node translation (node+0x8c) minus the owner's translation, bound +0xa and +0xb both required; fire 0x21a560 on outside → inside when skip is 0; store inside, clear skip. BoundLeft 0x26a20c fires on inside → outside. PlayerOnFootBoundEntered 0x2ca6b8 is Player1BoundEntered while 0x24255c(player) is 0 (not in a vehicle: controller +0x6c is neither 0x11c nor 0x19b).
- 0x21a560 also skips a trigger whose script already runs actively (0x18a89c: the first thread of that script with pc < count and flag bit 0 clear).

### Oracle comparison

- After the port's startup run with EnableTrigger and EnableBound (triggers start enabled), 236 of 241 trigger enable bytes and 479 of 482 bound +0xa bytes equal the oracle. Different: triggers 79 to 82 (`1-4_Run_Point_02..05`, AIMove / ObjectBoundEntered) and 141 (`DEBUG_Head_Robber_Chase_End`) are off in the original; bounds 266 (`CameraBound_Police_Helipad`), 271 and 272 (`CameraBound_VehicleDrop_01/02`) are off in the original. The port does not switch them off.
- The police entrance: bound 87 `Police_Station_Transition_Lower` has +0xa = 0 at gameplay-start (bytes ff 04 00 01; the startup path disables it, the port gets the same 0). This is why the walk in the 09:52:52 entry did not start `Police_ExtToInt`; the PlayerOnFootBoundEntered trigger 204 itself is enabled (skip 0, inside 0).

### Port behaviour

- `checkTriggers` runs after every runner pass in `chase-game`: BoundEntered, Player1BoundEntered, PlayerOnFootBoundEntered and BoundLeft with the player position; ObjectTriggered / ObjectUntriggered are queued by TriggerObject. Queued triggers become threads after the checks (last queued first) and run from the next update. Every room counts as loaded in `chase-game` (it shows all rooms). Other events are listed once on the console and never fire.

Unknowns
- Where the trigger enable byte starts (the builder of the runtime records was not read; "enabled" fits 236 of 241) and what switches off the five triggers and three camera bounds above.
- The object flag 0x100000 that forces an ObjectTriggered start (0x23cee8); the checks of ObjectBoundEntered, BoundInside / BoundOutside, AIMove, Character*, PlayerVehicle* and the other events.
- Whether the trigger pass runs for the master scene's triggers too (0x23cff4 searches it; the port has no master level).
