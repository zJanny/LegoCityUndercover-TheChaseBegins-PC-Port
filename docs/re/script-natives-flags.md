# Script natives: batch 26 (object flags and script bookkeeping)

Handlers of batch 26 (SCRIPT-26). Census and the other batches: [script-natives.md](script-natives.md).

<a id="ts-20261014-141621"></a>

## 2026-10-14T14:16:21+02:00 — Batch 26 (SCRIPT-26): the 22 handlers read, 11 transcribed (llvm-objdump on game.arm.elf, C++ runner tests; no emulator, no REA session)

Corrected by [script-natives-flags.md §2026-10-14T18:39](#ts-20261014-183900): ToggleUpdateFlag changes mask 0x4 (not 0x10) of the halfword at +0x10; it is read by the entity update loop 0x1a4094.

[Corrects script-natives.md §2026-10-14T13:33](script-natives.md#ts-20261014-133339) where noted. Every handler address was checked against its registration (the handler word sits directly before the name string at 0x15fd1c, 0x161408, 0x15fdec, 0x1620a0, 0x160b44, 0x1604a8, 0x160cf8, 0x161a68, 0x1603d4, 0x160b30, 0x15fef4, 0x15ff5c, 0x160b08, 0x160b1c; KillScript is in the static table at 0x359154; `DestroyProp` and `KillObject` share 0x1a2fe4).

### Argument array

An object argument is followed by a gap: in the two-argument natives whose first argument is an object the second value is at `[r1 + 0xc]` (ToggleUseable, EnableTargetable, EnableDistanceFade, SetIndestructible, ObjectUnshovable, ToggleUpdateFlag, EnableShadow, EnableSwitch, ResetCarriedThing); in ActiveDuringDCam and PrintDebugString it is at `[r1 + 4]`; PositionBound reads its two arguments at `[r1 + 4]` and `[r1 + 8]`. The call text always has the arguments in order, so the port reads them by index.

### Transcribed (handler, effect, return)

All return 1 (never block). The float argument tests compare with the literal 0.0 (pool words checked); the stores are to the object entity (`obj`) or its component `obj + 0x7c` (`comp`).

| Native | Handler | Effect |
|---|---|---|
| PrintDebugString | 0x26fde8 | `mov r0,#1; bx lr`. Corrects the census note (0x2665d0 is called by the next function 0x26fdf0). |
| revealcollectable | 0x27bdb0 | `mov r0,#1; bx lr`. |
| KillScript | 0x1c3834 | Calls 0x223a14(thread, script owner, script record). It walks the thread list at scene +0x1310 and, for every thread other than the caller with that owner and script record, stores the script's call count (record +6) into the thread's pc (+0x16). The thread ends at its next runner pass. A script the level does not have matches nothing. |
| ResetObject | 0x1b25f8 | Resolves the object with 0x22dacc, then 0x244fc8(obj, 8, 0): the class message handler of the object's kind (table 0x245004 + kind * 40, +0x14). |
| SkipToFinalState | 0x270078 | Same with message 54. |
| ToggleUseable | 0x1cdcf8 | v = argument > 0. For an object of kind 0x47 the children of kinds 0x48 in its child range get 0x236624(child, v) (bit 4 of the byte at +4 of the child's record in the scene's usable registry, when it is registered) and 0x2219bc(child, v) (byte +0xa of its 12-byte record in the second registry); then the same two calls for the object itself. |
| EnableTargetable | 0x26fb2c | Null object: nothing. Else the argument is converted to an integer (truncation); `comp[0xda]` bits 0 and 3 become 1 when its low byte is 0, else 0 (bits 6 and 7 unchanged). |
| SetIndestructible | 0x27be40 | `comp[0xde]` bit 0 = argument != 0. |
| ActiveDuringDCam | 0x26f978 | `comp[0x312]` bit 1 = argument != 0. |
| ObjectUnshovable | 0x26fd68 | Argument != 0: `obj+0x10` (halfword) |= 0x100 and `obj+0xc` |= 0x20000; 0 clears both. The object is used as is (no 0x22dacc). |
| ToggleUpdateFlag | 0x270094 | `obj+0x10` (halfword) bit 4: set when the argument is 0, cleared otherwise (cleared for NaN). |

### Read and not transcribed

- **EnableDistanceFade 0x2a05bc, DistanceFadeMid 0x27e798.** On the object's node (`obj+0x3c`), only when `node[0] & 31` equals a global node kind: EnableDistanceFade sets or clears bit 4 of `node[0xde]` (clearing also stores 16 / 255 in `node[0xdc]` / `node[0xdd]`) and repeats the same through all child nodes (0x2b65dc, links at +8 and +0xc); DistanceFadeMid sets bit 5 of `node[0xde]`. The node kinds and the fade consumer are not recovered.
- **enableobjectfade 0x26fac4.** 0x244fc8(obj, 24, argument); when the class handler returns 0 it calls 0x2428c0 (argument 0) or 0x242cf0 (else), the node calls EnableObject 0x1ddd00 also uses. Which kinds handle message 24 is not known.
- **EnableShadow 0x1bf84c.** Only when 0x21aae0(obj) is true (kind 27 or 149, or 0x236b6c(obj) non-zero, or flag bit 14 of `obj+0xc`): `comp[0x316]` bit 2 = argument > 0.
- **EnableSwitch 0x1bf8a4.** Argument > 0: message 8, then 0x236624(obj, 1). Else: for kinds 94, 139, 100 and 58 `comp[4]` (halfword) = 3; then 0x236624(obj, 0). Needs the kind of the object.
- **DestroyProp / KillObject 0x1a2fe4.** 0x23cd80(0x22dacc(obj), 0): returns when `obj+0x10` bit 0 or `obj+0xc` bit 4 is set; otherwise unlinks the entity from the scene lists (0x23211c, 0x23cee8 several times), sends message 1 and, if it is not handled, sets `obj+0xc` bit 4 and calls 0x2320f8. This is the entity destruction.
- **VehicleSetUnshovable 0x29eaa8.** Returns 0 (blocks, runs again every update) while `comp[2]` is 6 or 7; otherwise `comp[4]` = 10 and returns 1. The meaning of the states is part of the vehicle system.
- **ResetCarriedThing 0x27bd64.** Takes the global player entity (pool word 0x27bdac), and calls 0x2253b8(player..., argument > 0, ...) on the thing carried at `player.comp[0x13c]` (kind 122); needs the carry system.
- **PositionBound 0x268390.** Bound record centre (+0x10..+0x18) = world position of the object's node (matrix 0x250810, translation +48), and the byte +0xb of the bound = low byte of the halfword +0x2c of the room record found by 0x2234a8 for the node. Only call: `PositionBound(CameraBound_Test, Player1)` in lvl_test_playground_mike, not a game level.
- **PositionPlayer 0x1da0dc.** Same steps as PositionObject 0x1d9e6c (node copy 0x24c118, room link 0x245008, heading from the target matrix times 0x10000 / 360 into `comp[8]` and `comp[10]`, controller fields cleared: `comp[0x315]` bits 2 and 4, floats at `comp[692]` and `comp[696]`, message 8 for the player) and, for the character kinds 179, 183 and 243, a ground probe: a grid gather 0x237f30 around the target, a sweep 0x21ab20 downward through the hit entities, and the position replaced by the hit point (0x2554c0). When the target is a vehicle (kind 11 branch) 0x213284 gives the transform. Vehicles (0x29f014) are repositioned too (state 5, 0x22b224, 0x204e98).

### Port

`src/runtime/natives_objects.{hpp,cpp}` stores the ten transcribed values by instance name (lower case) and queues the class messages 8 and 54; `ScriptRunner::killScript` ends the threads. Tests: `natives-objects` (synthetic scripts, values from the disassembly above) and `natives-objects-real-data` (scripts of lvl_01: `1-3_parkour_working` four usable flags, `1-3_Platform_Down`, the three ResetObject messages of `1-3_Ledge_Platform_Switch_Bottom_Script`, and `1-2_Scanned_Pringles` ending `1-2_Scanner_Enter_Script` before its first tip; call texts dumped with the C++ level loader).

Unknowns:
- Who reads each flag in the original (targeting, interaction prompt, shove, destruction, door and object update) and the meaning of the class messages 8 and 54 per kind.
- The kind byte (`entity+0x12`) of each placed class; it gates EnableShadow, EnableSwitch, the group case of ToggleUseable, the fade natives and message 24.
- Whether the registries of 0x236624 and 0x2219bc hold every object; an unregistered object ignores ToggleUseable.
- The runner's treatment of a thread whose pc was set past the end while it waits for a child (the port ends it when the child returns).

<a id="ts-20261014-183900"></a>

## 2026-10-14T18:39:00+02:00 — ToggleUpdateFlag has a reader: the entity update loop 0x1a4094; PositionPlayer read again (llvm-objdump on game.arm.elf, `--mattr=+vfp2`; no REA, no emulator)

[Corrects script-natives-flags.md §2026-10-14T14:16](#ts-20261014-141621) ("`obj+0x10` (halfword) bit 4"): ToggleUpdateFlag 0x270094 sets or clears the **mask 0x4** (bit 2) of the halfword at `obj+0x10` (`orr r1, r1, #4` / `bic r1, r1, #4`), not mask 0x10. An argument of 0 sets it.

### The reader

- The scene's entity update loop (function around 0x1a3ffc..0x1a4118) takes each entity of its list, reads the halfword `[entity + 0x10]` and skips the entity when `halfword & 0x24` is non-zero (`tst r0, #36` at 0x1a4098), then applies an optional filter callback (`[r9 + 8]`), then calls the class update `table[kind * 40 + 0xc]` with `kind = entity+0x12` (table at 0x583124, loaded at 0x1a40cc..0x1a40e0). For a door that update is the shell 0x169ba0 (update 0x1d16b8 and state change 0x1a6b6c). So an entity with the ToggleUpdateFlag mask set gets no class update at all: a door neither opens, closes nor plays sounds, a request still changes its target through the message handler 0x1749b8 but is not carried out. Mask 0x20 is set by code not read here.
- The same mask 0x4 is tested at gather and target-search sites that skip the entity: 0x15712c, 0x1580a8, 0x17683c, 0x18c210, 0x1fefac, 0x269e94, 0x2a59bc, 0x2a5b1c, 0x2a5c98, 0x2e6ec8, 0x2e6f9c (all `ldrh r, [entity, #16]; tst r, #4`). The semantics of each caller (target selection, camera, collision gathers) are not read.

### PositionPlayer 0x1da0dc and PositionObject 0x1d9e6c (read in more detail)

- Pool of PositionPlayer: 0x1da550 = 5.0f, 0x1da554 -> a global float3 at 0x56d424 (zero in the file, filled at run time), 0x1da558 = 0x362298 (a table of function pointers in the file, used as the query descriptor of 0x21ab20 as in the camera note), 0x1da560 = 0.0f. The probe for kinds 179, 183 and 243 gathers entities around the target with 0x237f30 (box half extents from the float at node+0x9c), builds a segment of length 5.0 along the global direction with 0x23b92c and casts it with 0x21ab20; a hit replaces the position by the hit point (0x2554c0). Without the run-time value of the direction, the descriptor words on the stack and the semantics of 0x21ab20 this cannot be transcribed.
- The controller reset (both functions): inside the branch taken when `0x21aae0(entity)` is true (kind 27 or 149, or kind in the run-time table filled at 0x3743ec, or entity flag 0x4000 at `+0xc`) the controller component (`entity+0x7c`) gets: heading `strh [comp+8]` and `[comp+10]` from atan2 (0x24765c) of the target matrix, `comp[0x315] &= ~0x14`, floats at `comp+692` (0x2b4, fall distance) and `comp+696` (0x2b8, fall speed) = 0.0, and when `comp+0xc8` is set 0x215988(comp). Then message 8 for the player (0x244fc8(player, 8, 0)) when the global player entity (0x3772c0) is the object. The port's `VerticalState` has `fallDistance` and `fallSpeed` for those two floats.
- Whether `0x21aae0(player)` is true is not decidable statically: the kind table at 0x3743ec has count 0 in the file and the kind byte of the player entity is set at run time.

Unknowns:
- Kind byte (`entity+0x12`) of the player and the contents of the table at 0x3743ec at run time (decides whether PositionObject/PositionPlayer reset fall distance and speed of the player).
- The global float3 at 0x56d424, the stack descriptor passed to 0x21ab20 and the gather box of the ground probe.
- The meaning of `comp[0x315]` masks 0x4 and 0x10, of mask 0x20 in the update loop test, and the callers of the other mask 0x4 tests.
