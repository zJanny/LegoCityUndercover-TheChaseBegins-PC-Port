# Scene start: class startups, camera bounds, OnSceneEnter, ScriptCheckExtras

What starts script threads and switches triggers and bounds when a level or a scene is entered, besides the level's own `startup` script. The script engine is in [scripts.md](scripts.md), the load sequence in [streaming.md](streaming.md). Items: [script-system.md](../plans/features/script-system.md).

## Current state

- Scene start 0x197c3c walks the owner array last to first and runs each owner's `startup` script at once, so the level's own `startup` (element 0) runs last. [§2026-10-14T14:27:39](#ts-20261014-142739)
- Class startups of lvl_01 objects (24 objects) only enable sub-objects, load animations or hide items; they change nothing in the level's own objects, bounds or triggers and do not explain SCRIPT-2. [§2026-10-14T14:27:39](#ts-20261014-142739)
- The camera bound system (init 0x2ff35c) switches a bound off at start when an entity has the `CameraBound` / `UseableCameraBound` parameter; in lvl_01 this explains bounds 266, 271 and 272. [§2026-10-14T14:27:39](#ts-20261014-142739)
- Triggers: data fit only: a trigger whose object name names no object of the level is disabled (lvl_01 triggers 79 to 82 and 141). [§2026-10-14T14:27:39](#ts-20261014-142739)
- Event 0x1ecc50 formats `"%s%d"` with the start room number, then starts the plain name in all room entities; `OnSceneEnter<n>` exists in several levels, no `OnSceneExit` anywhere. [§2026-10-14T14:27:39](#ts-20261014-142739)
- ScriptCheckExtras 0x2e5638 starts when byte +0x35 of the struct at 0x5163e8 is set, which happens on leaving the Extras screen. [§2026-10-14T14:27:39](#ts-20261014-142739)
- Open: runtime trigger record builder, startup versus camera bound init order at the initial load, empty object name resolution, camera bound update 0x2ff7c8, and the second script at 0x2f6870.

<a id="ts-20261014-142739"></a>

## 2026-10-14T14:27:39+02:00 — Class startups, camera bound switches, OnSceneEnter and ScriptCheckExtras (llvm-objdump on game.arm.elf, REA Ghidra 12.1.2 strings and xrefs, C++ level census, Azahar oracle `police-station` New 3DS, port 24780, `read` only, no RPC)

Replica: `SceneStart`, `cameraBoundsSwitchedOffAtStart`, `triggersWithoutObject`, `roomNumber` (src/runtime/scene_start.*), `levelStartState` and the session start in src/game_level.cpp. Tests: `scene-start` (synthetic), `level-start-real-data` (real data and the oracle values below).

### Class startups (scene start 0x197c3c)

- 0x197c3c(scene) walks the owner array scene+0x24 (count +0x1c) from the last to the first element; for each owner it looks up the script "startup" in owner+0x24 (0x2388e4), creates the thread (0x22ce9c) and runs it at once (0x234204). Its only caller is 0x2f6848 inside 0x2f67fc (the world's enter, vtable 0x362f10), which also starts one more script by the name in the byte string at [0x2f6c04]+0x3a when that byte is set.
- Owner array (oracle `police-station`, scene 0x085aa950: +0x1c = 0x121 owners, +0x24 = 0x08e5a590): element 0 is the level root (0x08fbe410: +0x20 scene, +0x24 level data 0x08f9cba0, +0x34 trigger records 0x08e65ac0, +0x3c node), the other elements are entities (+0x20 scene, +0x24 their class data). 0x23c188(scene) returns element 0. So the level's own `startup` runs last. lvl_00 has 350 objects and 288 entity owners (the others are in rooms that are not loaded); lvl_01 had 2573 owners of 2967 objects at gameplay-start.
- Class files with a `startup` script that placed objects of lvl_01 use (census with the C++ parser, `levels/props/*.blvl`): 24 objects: `CharacterLocker` (ChangeCharTokenTexture), `Cherry_VehicleDrop_01/02` (EnableObject of the sub-objects SBVehicleDropConsole and Vehicle_Collision 0), `FP_GrowSpot_Climb01` and `FP_Vine_Plant_01` (EnableObject Climb_Me 0), 4 construction, 4 miner and 8 robber freeplay tasks (animation loads; when `TaskComplete.TASK_COMPLETE == 1` they disable their trigger and themselves), `FP_Water_Plant_01` (no calls), `SuperJump_01/02` (CallScript SBHideItems: sub-objects Barrier01/02 and class bounds off). Their object names are names inside the class file (sub-objects, class bounds and triggers belong to the owner), an empty object name is read as the owner itself.
- No class file sets or clears anything in the level's own objects, bounds or triggers for a new game, so the level state after the startup is unchanged by them. The classes of the SCRIPT-2 objects (hydrant sprays, tokens, `FP_Task_COP_CTH_*`, grapple pad, `prop_buildit`) have no scripts, so class startups do not explain SCRIPT-2.

### Camera bound system (0x198014 -> vtable slot +0x40 = 0x2ff35c)

- 0x198014(scene) calls slot +0x40 of every registered scene system (list at 0x589a00), 0x198058 slot +0x44 (leave). 0x3635bc is the vtable of the camera bound system: slot +0x40 init 0x2ff35c, +0x44 leave 0x2ff758, +0x48 update 0x2ff7c8. It runs at the end of the level load 0x2e6264 (0x2e6870, before 0x1ecd44(6, 0) and the "OnSceneEnter" event at 0x2e6888) and in the scene switch 0x303480 (0x3036a0, before the event at 0x3036b8).
- 0x2ff35c walks the entities of the scene's rooms (lists +0x58, 4 per room) twice per string. The strings are "CameraBound" (0x2ff738) and "UseableCameraBound" (0x2ff744; "CameraBoundArrive" 0x3345f8 is used elsewhere). For an entity that has the parameter (0x241624, hash of "CameraBound" is 0x72c071e1) it finds the bound of that name in the level data of the entity's scene (0x23e798) and, when found, stores {entity, bound} in an array (+0x24 / +0x2c) and writes `strb 0, [bound, #0xa]` (0x2ff4fc, 0x2ff6e0): the bound starts switched off. The update 0x2ff7c8 later switches it on when the player uses the object (not read).
- lvl_01: only `1-3_Helipad` (`CameraBound_Police_Helipad`, bound 266), `Cherry_VehicleDrop_01/02` (`CameraBound_VehicleDrop_01/02`, bounds 271 and 272) and `FP_SuperBuildit_Statue_BP` (`CameraBound_SB_Statue`, no such bound) have the parameter. 266, 271 and 272 are exactly the three bounds that were off in the oracle although no script switches them. [Corrects scripts-runtime.md §2026-10-14T10:06](scripts-runtime.md#ts-20261014-100640): the open question about the three bounds.

### Trigger records

- Runtime record builder not found (searches for the allocation 0x20 bytes per trigger, the stores at +1, +2 and +4 and the callers of 0x1dd380 found nothing). Data fit: a trigger whose object name (file record +8) is set and names no object of the level is disabled. lvl_01: the object `1-4_AI_HeadRobber` (triggers 79, 80, 81, 82) and `1-4_AI_HeadRobber01` (141) do not exist: exactly the five triggers that were off in the oracle and that the level scripts do not switch; no other trigger of the 241 has a missing object. lvl_00: no trigger has a missing object and all 53 enable bytes are 1 in the oracle (`police-station`, 0x08e65ac0, 0x20 each). [Corrects scripts-runtime.md §2026-10-14T10:06](scripts-runtime.md#ts-20261014-100640): the open question about triggers 79 to 82 and 141 (the bound-event record builder is still unread).

### Events (0x1ecc50, gameplay load and scene switch)

- 0x1ecc50(room, name): `room` is the start room, the result of 0x213284(Player1Start or the destination entity) (0x2e6354 stores it); +0x28 its scene, +0x2c its number (the "%d"), +0x30 / +0x40 its linked rooms. It formats "%s%d", starts that script of the scene's level root at once, then starts the script with the plain name for every entity in the rooms. Callers: 0x2e6888 ("OnSceneEnter", level load), 0x303564 (OnSceneExit, scene switch at 0%) and 0x3036b8 (OnSceneEnter, at 100%).
- Data: no level file or class file has a script named `OnSceneExit`, `OnSceneExit<n>` or a plain `OnSceneEnter` (census of every .blvl). `OnSceneEnter<n>` exists in lvl_00 (1, 2, 3), lvl_02 (1), lvl_03 (1, no calls), lvl_04 (1), lvl_05 (3), lvl_07 (1, 2, 3); lvl_01 and lvl_06 have none.
- Oracle `police-station`: the lobby (Room1) was entered. After the port's startup of lvl_00 plus `OnSceneEnter1`, bound byte +0xa is 0 for bounds 6 (`Lobby2Basement_SceneChange`, startup), 13 (`HINT_Basement`) and 14 (`HINT_Roof_Lift`, both `OnSceneEnter1`) and 1 for the other 134: equal to the oracle's 137 bytes (without `OnSceneEnter1`: only bound 6). Bound byte +0xb off for 3, 7-10, 15, 16, 18, 21-23, 31-37 and 98-136 (rooms not loaded).

### ScriptCheckExtras (0x2e5638)

- The gameplay update 0x2e5158 reads byte +0x35 of the global struct at 0x5163e8; when set it starts the script "ScriptCheckExtras" of the level root at once (0x2e5654..0x2e5688), clears the byte (0x2e568c) and goes on with the runner pass (0x2e56a4 and 0x1746c0). The byte is set by the 8-byte function 0x290310, which the last instruction of a tear-down function (tail jump at 0x2feac4, after freeing the page's objects 0x1e8278 / 0x1e82a4) calls: leaving the Extras screen. The script exists in lvl_01, lvl_03 to lvl_07 and a test level (race mode switches, `IsRedBrickExtraActive 9 RaceUnlocked`, `PrintDebugString "Extras Exited!"`).

Stand-ins and open items (IDs in [script-system.md](../plans/features/script-system.md))
- The class startups run for all placed objects (the original: entities of loaded rooms) in private runners; sub-objects are not instantiated; the empty-name owner is not merged. The trigger rule is a data fit. The order class startups, level startup, camera bounds, OnSceneEnter is the call order of the load code, not traced in one run.

Unknowns:
- The runtime trigger record builder and the real rule behind the five triggers; the other bound-event types' builder.
- Whether 0x2f67fc (startup) runs before 0x2e6264 (camera bound init, OnSceneEnter) at the initial load; the scene switch order was read only from the call sites.
- How an empty object name resolves in 0x21b3fc / 0x2450f0 (owner or nothing), and how `TaskComplete.TASK_COMPLETE` is stored (saved variables of the class objects).
- The camera bound update 0x2ff7c8 (when bounds are switched on again) and `CameraBoundArrive` (0x3345f8).
- What the second script started in 0x2f6870 (name at [0x2f6c04]+0x3a) is.
- Whether the extras flag is also set elsewhere; the Extras screen is not in the port.
