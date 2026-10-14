# AI natives and the AI component (batch 34)

The 17 AI natives of the level scripts, the AI component fields they write, the requested-state numbers and the AIMove trigger event. Evidence: llvm-objdump (`--triple=armv6k --mattr=+vfp2`) of `artifacts/game/game.arm.elf`, level data (`parseLevel`); no emulator run, no REA session. Census of all natives: [script-natives.md](script-natives.md). Items: [script-system-natives.md](../plans/features/script-system-natives.md) (SCRIPT-34, SCRIPT-42, SCRIPT-75, SCRIPT-79 to SCRIPT-84).

<a id="ts-20261014-163311"></a>

## 2026-10-14T16:33:11+02:00 — Handlers of the 17 AI natives, AI component fields, requested states and the AIMove event

[Corrects script-natives.md §2026-10-14T13:33](script-natives.md#ts-20261014-133339): none of the 17 batch 34 natives blocks (all return 1), the class B? of the census is wrong for them.

**Calling convention and objects.** Every handler returns 1. The first argument is resolved with 0x22dacc: an object named `Player1` (hash of the string at 0x358b70) becomes the entity in the global 0x3772c0 (the player; 0x3772b8 is a second global, equal to it in a one-player game, [script-natives-progress.md §2026-10-14T14:41](script-natives-progress.md#ts-20261014-144155)). The AI component is `entity + 0x7c`.

**Component fields written** (byte offsets from the component):
- `+0x14` current state, `+0x15` requested state. Requested numbers seen: 0 idle (0x2246c0, also clears `+0xa8`/`+0xc4`), 1 no state (0x241304), 5 path state (0x2c62fc), 9 mill about (0x22471c, only when bit 7 of `+0x315` is set; also `+0x84` = 3), 10 face target (0x211110, target at `+0xc4`, bit 3 of `+0xdb` = target is the player, own position copied to `+0xb8`), 35 / 36 (AIPursuit through 0x1d4688 -> 0x218b38: 35 when the takedown argument is non-zero, else 36).
- `+0xd8` bit 4: AI enabled (AIEnable); bit 6: invulnerable (CharacterInvulnerable). `+0xd9` bit 0: ignore player (AIIgnorePlayer). `+0xdb` bit 7: set when the AICantAttack argument is 0. `+0x315` bit 7: mill about (AIMillAbout sets it for an argument greater than 0).
- `+10` u16 heading (AIFaceTarget direct form), `+0xac` move target (AIMove), `+0xb8` home position (AIEnable, face target), `+0x8c` MinDistance (class parameter, default -1.0), `+0x88` TurnSpeed (default 0.1) read by the component constructor at 0x16e098 and following.

**Handlers** (name, address, arguments, effect):

| Native | Handler | Effect |
|---|---|---|
| AIEnable | 0x1881d4 | object, flag: bit 4 of `+0xd8` = flag != 0, home position `+0xb8` = node position, then 0x241304 (state 1). An object with flag bit 0x10 at entity+0xc is skipped unless it is 0x3772b8 and the flag is 0. For the player the same is applied to the party members named `Player1.Player%02u` (up to 10) and to the partner at `+0x130` when its type is 0x9f. |
| AIActive | 0x1881a4 | object: bit 7 of `+0x315` set -> 0x22471c (state 9), else 0x2246c0 (state 0). |
| AIMillAbout | 0x1b22e4 | object, number: bit 7 of `+0x315` = number > 0, then the same choice. |
| AIMove | 0x171f7c | object, target object, run flag (0 walk, 1 run): skipped for objects with flag bit 0x10; target position (a type-11 object: path point via 0x22faf8, else node position +0x30); 0x22d85c(object, position, 1, walk = (flag == 0), 0) asks the path finder 0x1f9834; `+0xac` = position; 0x2414b4(object, `+0x50`, 1, 0) installs the state data. |
| AIMoveNoAvoid | 0x1cd798 | same, with the avoid argument of 0x22d85c set to 1 (bit 0 of the path record `+0x7c`). |
| AIFaceTarget | 0x1bf570 | object, target: when the object is not 0x3772c0 or its bit 4 of `+0xd8` is set -> 0x211110 (state 10); else `+10` = int(atan2(dx, dz) * 10430.378) (literal 0x4622f983 = 65536 / 2 pi, vector from 0x257f14, atan 0x24765c / 0x2473ec). |
| AIIgnorePlayer | 0x1d9c38 | bit 0 of `+0xd9` = number != 0. |
| ainostate (AINoState) | 0x194e0c | 0x241304: requested state 1. |
| CharacterInvulnerable | 0x2a6cfc | bit 6 of `+0xd8` = int(number) & 1. |
| AICantAttack | 0x1bf528 | bit 7 of `+0xdb` set when number == 0. |
| AISuperJump | 0x1b2344 | character, target, height: 0x213300(character, target position, 28, height). |
| AIResetState | 0x1bf650 | 0x22517c(`+0x50`, entity) and 0x2414b4(entity, `+0x50`, 1, 0). |
| AIFollowSimplePath | 0x285d18 | character, path, start node, reverse, run: a negative start node gets count - 1 added (path count u16 at +10); 0x1fbde4(character, path, node, reverse != 0, walk = (run == 0)). |
| AIPursuit | 0x17cbf0 | character, first node, takedown: 0x1d4688(character, node, takedown != 0) (clears bit 4 of byte `+0xde` of the entity's node and of the object at `+0x16c`, requests 35 / 36, copies bit 0 of `+0xdb` into bit 5 of `+0xd9`, node into record `+0xe0` + 0xb4) then 0x2c2604(character, takedown) (pursuit manager record at 0x3746d4+0x20). |
| AIStalkPlayer | 0x1cd88c | character, distance: 0x282b88(character, distance, 0x3772b8 entity). |
| AIRunAway | 0x17cc30 | character: 0x228c70(character, -1.0, 0x3772b8 entity). |
| LockCharacter | 0x1cda60 | character enum; only ids 9, 19, 29, 39, 49, 59, 69: 0x23b388 (remove from the party), then clears bit n (0..6) of the byte at 0x52086b, the inverse of UnlockCharacter 0x263918 ([script-natives-progress.md §2026-10-14T14:41](script-natives-progress.md#ts-20261014-144155)). |

**The AIMove trigger event.** The event type pointer is `*0x3775e8` (slot 0x40 of the type table at 0x3775a8, registered at 0x16bbc8 with the name `AIMove`). It is raised by 0x23cee8(type, entity, entity, component byte 0, 0) from three places: the "go to target" state update 0x2c3bd8 (when MinDistance > the distance to `+0xb8`/`+0xc0` in x and z, or when the node's y is below `+0xbc` and bit 6 of `+0x314` is set), the path state at 0x2c630c (no next path node: end of an open path; a path with the closed flag at +9 wraps), and 0x2cbf10 (not read). 0x23cee8 -> 0x23cff4 searches the scene's trigger array for the first record that is enabled (byte 0), has the type and the entity as owner, and starts it through 0x21a560; if none is found for a `Player1` object it repeats with `Player1.Player%02u`. After the event the state update asks 0x1f8d5c(object, position, 0, `+0x7a`, ...) for a follow-up state: 0x21102c with it, else 0x2246c0 (idle).
- lvl_01 triggers of the type: `1-4_Booster_Strange` (script 1-4_Run_Point_01), `1-4_AI_HeadRobber` (1-4_Run_Point_02, 03, 04); several triggers of one owner fire one after another because the scripts switch the next one on.
- `chase-game` ends a move when the character stands on the target (after a straight walk) and starts the first enabled AIMove trigger of the character ([ai.hpp](../../src/runtime/ai.hpp), SCRIPT-S31).

**Measurement used for the walk stand-in.** Police entry, oracle: the player moves 0.11 per update of 1/30 s towards `Enter_Police_Station_Position_End` ([scripts-runtime.md §2026-10-14T10:54](scripts-runtime.md#ts-20261014-105432), WarningDialogBox section). `AiCore` uses 0.11 * 30 for walk and the player's run speed (0.23 per step) for run; `ai_real_test` pins the step to 0.11 +- 0.002 on the real script `Police_ExtToInt`.

Unknowns:
- Number of the state AIMove installs through 0x2414b4 (table at 0x3742c8, entry stride 24), the path finder 0x1f9834 and the state update that moves along its path; the real walk speed of non-player characters (class parameters) and the walk / run decision.
- Meaning of entity flag bit 0x10 at entity+0xc (AIMove, AIEnable skip such objects).
- What 0x241304 / 0x2414b4 do after the request byte (state change in the AI update), the follow-up lookup 0x1f8d5c, and the third raise site 0x2cbf10.
- AISuperJump 0x213300, AIPursuit 0x1d4688 / 0x2c2604 / 0x218b38, AIStalkPlayer 0x282b88, AIRunAway 0x228c70 and the face-target state are only read as far as their arguments; their effects are not ported.

<a id="ts-20261014-184240"></a>

## 2026-10-14T18:42:40+02:00 — AI movement: state table, simple-path state 22, go-to-target state 11, path finder front

Read with llvm-objdump on `artifacts/game/game.arm.elf` (`--triple=armv6k --mattr=+vfp2`); no REA session, no emulator trace. Adds to [the AI native handlers](#ts-20261014-163311).

**AI state table.** Built at 0x2a3c24 into the global 0x3771f0 as 16-byte records, one per state number; word 0 is the update function, word 3 (where set) a second hook. Update functions seen: states 4 and 5: 0x2c5d3c (path follower), state 11: 0x2c3bd8, state 12: 0x2c3d1c, state 22: 0x2cbe14; 0x21102c requests state 2 (pursuit of `+0xc4`). Requests go through 0x218b38(entity, component, state), which stores the number at `+0x15`.

**AIFollowSimplePath is state 22, not the path finder.** The helper 0x1fbde4 stores path `+0xa8`, start node `+0xa6`, reverse bit 7 of `+0xda`, walk bit 5 of `+0xd9` (argument `run == 0`) and bit 3 of `+0xd9`, then requests state 22. The update 0x2cbe14, each tick:
- node = path positions (`path+0xc`, 12 bytes each)[`+0xa6`]; `+0xe` bit 0 set (move wanted), bit 3 set when the walk bit is set; heading u16 `+6` = int(angle(position, node) * 10430.378) (0x23df44: atan2(dx, dz) of node - position through 0x24765c).
- Distance 0x22b62c: sqrt(dx^2 + dz^2), y ignored. The raw float bits are compared with 0x3f000000 (0.5): below 0.5 the node counts as reached, otherwise the tick ends. The character is not snapped to the node.
- Next index: reverse: index - 1; below 0 a closed path (byte `path+9`) continues with count - 2 (sub 1, sub 1, add count at 0x2cbea4..0x2cbec4), an open path ends. Forward: index + 1; when count is not greater, a closed path wraps to index + 1 - count, an open path ends. Count is u16 `path+10`.
- End of an open path: 0x218b38(..., 0) (idle), then the AIMove event 0x23cee8(*0x3775e8, entity, entity, **255**, 0) at **0x2cbf10**. A closed path never ends.
- The fourth argument of 0x23cee8 reaches 0x21a560 as the start argument (uxth) of the trigger found; the other two sites pass the component byte 0.

**State 11, go to target (0x2c3bd8).** Vector to `+0xb8` / `+0xc0` (x and z); distance via 0x229708. Arrived when MinDistance (`+0x8c`) is greater than the distance, or the node's y (transform `+0x34`) is below `+0xbc` while bit 6 of `+0x314` is set. Arrival raises the AIMove event with component byte 0 (second site), then asks 0x1f8d5c for a follow-up (a target to attack: non-zero -> 0x21102c, state 2; zero -> idle). Otherwise heading `+6` as above, `+0xc` bit 0 set, and for `+0x6c` == 1 the locomotion state 72 through 0x2414b4(entity, `+0x50`, 72, 0). MinDistance defaults to -1.0, so without a class value only the y test can end it. Who requests state 11 is not read.

**AIMove front end (0x22d85c).** It makes sure the path record (`+0xc8`, created by 0x206d18) exists, sets bit 0 of the record's u16 `+0x7c` (avoid) and runs the path finder; on result 1 it logs "I can't get there" and returns 0 (nothing changes). Otherwise the target is copied to `+0xb8`, the requested state is **4**, bit 5 of `+0xd9` = walk argument, bit 3 = avoid argument, `+0x84` = 5. The finder entry 0x1f9834 is a gate (budget byte at 0x3771d8 or the argument) that clears the low nibble of `+0xd8` and calls 0x204284.

**Path finder (0x204284, not ported).** It works on the loaded nav grids (global 0x376bec: 0x3c-byte grid records per room and 0x18 per room descriptor, the file records of [levels.md](levels.md#ts-20261012-2355) plus runtime fields). 0x1c2ccc maps a world position to (grid, cell, room) with cell size and origin from the level; both endpoints are looked up, equal cells give a direct leg, otherwise an incremental search (0x1d0700 resets node flags, 0x294768 steps the search until a node of type 3, 0x208338 builds the leg list). The record byte `+0x7e` holds the follow phase: 0 none, 1 failed, 2 direct, 3 within a grid, 4 across grids. The follower 0x1faf04 / 0x1fae48 returns a code per tick; the state 4/5 update 0x2c5d3c ends a plain move when it returns 0 and `+0xa8` is 0: AIMove event with component byte 0 at **0x2c630c**, follow-up 0x1f8d5c, idle. With `+0xa8` set that function advances the node itself (same index rules, then 0x22d85c to the next node, requested state 5) and raises the event only when no node is left.

Unknowns:
- The path finder and follower in full (0x204284, 0x1d0700, 0x208338, 0x294768, 0x1faf04, 0x1fae48, 0x20d8a8, 0x2397a0): the arrival test of a plain AIMove (the follower's last-leg tolerance) and the waypoint list.
- Actual movement: these states only set heading and the `+0xe` bits and play locomotion state 72 (0x2414b4 -> animation 0x2eafc8); speeds of non-player characters come from their locomotion and animation, not from this code.
- Value of component byte 0 (start argument of the event at the 0x2c3bd8 and 0x2c630c sites).
- The follow-up lookup 0x1f8d5c in full; who requests state 11; what the original does with an index outside the path (it reads past the node array).
