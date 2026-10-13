# Level scripts and the start state of placed objects

How the level's own scripts decide which placed objects are disabled when a level starts. Format of the level file: [levels.md](levels.md). Collision consumer: [collision.md](collision.md).

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

Stand-ins and approximations (open items in the progress file)
- Waits and every blocking native count as elapsed; SpawnScript runs the child to its end before the parent continues (the original runs it until it yields); only the "startup" script is run; no script runs after the start (no mission events, triggers, bounds).
- `IsLevelUnlocked`, `GetGameComplete`, `IsRedBrickExtraActive` return 0 (new game, no save). A loaded save would change the state (the code supports it; the port has no save-aware start yet).
- Revealer rule is a data fit; verified only in lvl_01. lvl_00's start state was not compared with the oracle.

Unknowns
- Which of the disabled-by-script objects with an enabled entity are re-enabled by another script (hydrants, tokens, `FP_Task_COP_CTH_*`) and why `1-3_BI_Seesaw` / `1-3_BI_Roundabout` (build-it bits) are enabled while `MissionHide` disables them.
- The code that hides the revealer targets; the role of the other type-4 parameters (they link objects: Destination, ObjectTo, TriggerObject, ...).
- Variables of type 5 and 7 in conditions (converted to floats here; the executor compares floats).
- Hydrant_Spray_01 appears twice in the startup script; the other mechanism that keeps it enabled is not known.
