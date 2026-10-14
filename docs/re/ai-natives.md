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
