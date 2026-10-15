# Script natives: animation and attach (batch SCRIPT-32)

Handlers of the 13 natives of batch 32 and what the port does with them. Census and the other batches: [script-natives.md](script-natives.md). Items: [script-system-natives.md](../plans/features/script-system-natives.md) (SCRIPT-32, SCRIPT-112, SCRIPT-S110). Argument layout of the handlers: [script-natives-progress.md](script-natives-progress.md) (an object argument at `[r1 + 8 i]`, any other argument as a value pointer at `[r1 + 8 i + 4]`).

<a id="ts-20261014-213649"></a>

## 2026-10-14T21:36:49+02:00 — Batch 32 (SCRIPT-32): the 13 handlers read, all run as stand-ins (llvm-objdump on game.arm.elf, C++ runner tests; no REA session, no emulator for these handlers)

Corrected by [script-natives-animation.md §2026-10-15T18:33](#ts-20261015-183336): 0x1b71e8 plays the cape animation, not the looked-up animation; the Attach natives attach the second object to the first.

[Corrects script-natives.md §2026-10-14T13:33](script-natives.md#ts-20261014-133339): PlayCharAnim is not blocking and does not share the body of PlayCutscene. The census row said "first call starts the animation; returns 1 when 0x2369c4 == 3 (shared with PlayCutscene)".

### Handlers

| Native | Address | What it does |
|---|---|---|
| PlayCharAnim | 0x1bf9cc | Two instructions: `r2 = 0`, branch to 0x29e5f0. (PlayCutscene 0x1bf9d4 sets `r2 = 1` and falls into its own body 0x1bf9dc, which returns 1 only when 0x2369c4 == 3.) 0x29e5f0(thread, args, r2): object `[r1]` through 0x22dacc; an entity of kind byte (+0x12) 159 whose component has a non-null word at +0x348 is first destroyed with 0x23cd80(entity, 0) and the player entity (`*0x3772b8`) is used. The animation name is `[r1+0xc]`, the class folder `[r1+0x14]`, the loop flag `[r1+0x1c]` (float, loop when above 0). A path string is built (`"%s/%s"` of the class name of component byte +0x2cc and the class argument when 0x22716c is true, else `"%s"` of the entity's +0x40), 0x295a8c(entity, name, path, loop, 0.1, 1.0, ...) prepares the entity, the object gets state request 80 (81 when `r2 != 0`) through 0x2414b4, the name is copied (63 characters), the text after its first `_` is upper-cased, a compare with a constant string at 0x29e808.. can cut it, and 0x1b71e8(entity, component, name, class, 0) returns the animation. If found, 0x23afc0(entity, animation, loop, r3 = 0, speed 1.0, blend 0.1, stack 0xffff) starts it and 0x1fb894(animation) follows. Returns 1 in every path: it never blocks. |
| PlayAnimation | 0x268240 | Looks the animation up with 0x21e8b4(entity + 0x40, name `[r1+0xc]`); not found: returns 1 without effect. Found: class message 253 with a 12-byte record {u16 0, animation, float 0.0} (0x244fc8), the object variable `"AnimationOffset"` (0x2451d0), then 0x23afc0(entity, animation, byte(arg 3), r3 = 0, speed = arg 2, blend = arg 4, stack 0xffff); a non-zero offset is applied with 0x227228(animation, float(offset)); 0x2b9e2c(animation, result) last. Returns 1. |
| LoadAnimation | 0x2681a0 | 0x21e8b4(entity + 0x40, name) finds a loaded animation: then it only tail-calls 0x17e588. Otherwise 0x23e6e4(entity, name, class or null for an empty class string, 1, context 0x24d4cc(), 1) loads it. Returns 1. |
| UnloadAnimation | 0x27e9c4 | Finds the animation as above and releases it with 0x23e518; returns 1. |
| StopCharAnim | 0x1bfb84 | `0x233900(0x22dacc(arg 0) + 0x40)` (the animation player), then 0x225c5c (stop); returns 1. StopAnim 0x1883d0 does the same with the object argument used directly (no 0x22dacc). |
| WaitForCurrentCharAnim, WaitForCurrentAnim | 0x263ae4, 0x2a06a8 | The animation player is 0x233900(entity + 0x40) (the character variant resolves the entity with 0x22dacc first). No player or no current animation: returns 1. Node kind (0x245f84) 6: returns 1. Kind 1 or 5: returns 1 when `abs(length - position) <= 1.0` (length 0x23caa4 as unsigned to float, position 0x213140(animation, 0)), otherwise 0 (blocked, called again every update). Any other kind: 0. |
| AttachObject | 0x1bf68c | Arguments are two objects (`[r1]` child, `[r1+8]` target). If the target's scene node already has a parent (node +4), 0x245048 detaches it; 0x245008(child node, target node) attaches; bit 0x400 of the target's flag halfword (+0x10) is cleared, class message 43 goes to the child with the target, bit 0x40 of the target's halfword is set or cleared by whether its parent equals the root node of the scene object list. Returns 1. |
| AttachToBone | 0x1ddb90 | Child `[r1]`, target `[r1+8]`, child bone name `[r1+0x14]`, target bone name `[r1+0x1c]`. When the target node's type (low 5 bits of its first word) equals the word at 0x37674c, the bone index of the target name (0x23645c) and the bone matrix (0x258858, 0x258b38) are looked up and 0x20f33c(child, target, name, matrix) attaches; otherwise a compare of the name with a constant string selects 0x241590 or 0x22acdc. If both entities have a pointer at +0x84, 0x2432bc(..., 0) follows. Returns 1. Partly read. |
| AttachToBoneRot | 0x1bf718 | As AttachToBone, with the child bone name `[r1+0x14]` and three rotation angles `[r1+0x1c]`, `[r1+0x24]`, `[r1+0x2c]` in degrees (times 0.0174533) built into a matrix (0x24d434, 0x237c48) for 0x20f33c; the halfword bit 0x40 is set as in AttachObject. Returns 1. |
| AttachToWorldRoot | 0x2951ec | Detaches the node (0x245048 when it has a parent), attaches it under the world root node (root node from 0x23c188 applied to the word at thread +0xc, +0x20; then 0x245008), clears bits 0x6000 of the node word and sets bit 0x40 of the entity's halfword (+0x10). Returns 1. |
| AttachToRoom | 0x1ddc88 | Finds the room node of the entity (0x237b04); if it is not the current parent, the node is detached, 0x2268f4(node, 0) runs, the node is attached to the room node (0x245008) and bits 0x6000 of the node word are set; bit 0x40 of the entity halfword is set. Returns 1. |

### Port

- `src/runtime/natives_animation.{hpp,cpp}`, dispatch `ScriptRunner::animationNative`. All 13 count as stand-ins (SCRIPT-S110): the request (animation name, class folder, loop, speed, blend), the loaded set and the attachment of each instance are recorded; nothing reads them, the waits complete at once and the node tree is not touched.
- Animation names resolve to files `models/<model>/<class>/<name>.bfnanm` for characters (`models/char_minifig/class_generic/crying_idle_minifig.bfnanm`) and `models/<model>/<name>.bfnanm` for objects (`models/prop_green_dumpster_xl_empty/dumpster_peak.bfnanm`), see `lego_city3ds.csv`; the port does not resolve them yet.
- Tests `natives-animation` (synthetic scripts) and `natives-animation-real-data` (scripts of lvl_01, `1-2_Cage_Key_Target_Script`, `1-2_MissionStart`, level startup; the expected values are the static call texts of the level file).

Unknowns:
- What 0x295a8c, 0x1b71e8 and 0x23afc0 do in detail (the animation player), the meaning of the state requests 80 / 81 and of the constant string that can cut the name in 0x29e5f0.
- The class of `0x22716c` and what the entity `+0x40` word is in the second path format.
- Whether animations are loaded by the object class itself before a script calls PlayAnimation (the original skips PlayAnimation for an animation that is not loaded; the port records the request anyway).
- The lengths for the WaitForCurrent natives need the animation player (ANIM-2).

<a id="ts-20261015-175022"></a>

## 2026-10-15T17:50:22+02:00 — User report: the van driver is not stuck in the mailbox (mission 1-1)

- User report: at the broken-down donut van (where the cutscene of mission 1-1 starts), the original shows the van driver stuck inside a mailbox; in `chase-game` the driver stands in front of it.
- The driver is `1-1_AI_DonutLady` (`CSCharacter` `donut_driver`, [cutscene-actors.md](cutscene-actors.md)). The scripts that set up the broken van scene, `1-1_VanBreakdown` and `1-1_Found_Van`, call PlayCharAnim ([objective-hud.md §2026-10-14T15:59](objective-hud.md#ts-20261014-155919)); in the port PlayCharAnim only records the request (SCRIPT-112), so no special animation plays.
- Likely cause: the stuck pose is an animation (and maybe an offset) started by PlayCharAnim. Not checked against the script arguments or the oracle.

Unknowns:
- The object, animation name and any position change of these PlayCharAnim calls.

<a id="ts-20261015-183336"></a>

## 2026-10-15T18:33:36+02:00 — Batch 32 played through the slot player: PlayCharAnim 0x295a8c, PlayAnimation, the waits, the attach direction, the van driver in the mailbox (REA/Ghidra pseudo-code and llvm-objdump of `game.arm.elf`; oracle `m11-3-repair-van-marker` memory reads; `chase-game` offscreen run)

[Corrects §2026-10-14T21:36](#ts-20261014-213649): 0x1b71e8 does not return the played animation, it plays the cape animation; AttachObject, AttachToBone and AttachToBoneRot attach the second object to the first, not the first to the second.

**PlayCharAnim 0x29e5f0.** The object resolves through 0x22dacc (an entity of kind 159 with a vehicle record is destroyed first and the player used). The folder is `"%s/%s"` of 0x2270f8(character byte ctrl+0x2cc) and the class argument when 0x22716c(entity) is true, else the entity's +0x40 text; 0x2270f8 returns `char_minifig`, `char_minifig_small` (table byte +0x1f bit 2) or `char_minifig_tall` (bit 0x40) from the character table 0x372e10 (stride 0x28). **0x295a8c(entity, name, folder, loop, ...)** is the play: 0x21e8b4(entity + 0x40, name) finds a loaded animation, else 0x23e6e4 loads it (none: nothing happens). When the found animation is the current one (0x23ad24) and ctrl+0x317 bit 0x40 is set, only the mode bits of the current slot change and 0x22c2fc(0) starts it again at frame 0. Otherwise a previous script animation is released (callback `*0x3771c8`, bit 0x40 cleared), 0x23afc0(entity, animation, mode = loop, first 0, end 0xffff, speed 1.0, blend 0.1) starts it, ctrl+700 = 0xffff, ctrl+0x310 = 1.0, and bit 0x40 is set when this call loaded it. Then state request 80 (81 when the caller passes r2 = 1) through 0x2414b4. The name copy with its text after the first `_` upper-cased and `_MINIFIG` cut is for **0x1b71e8**: for characters whose table byte +0x1f has bit 8 it looks for `models/<folder>/<class>/<name>_cape.bfnanm` and plays it on the cape entity (ctrl+0xe0 +0x28) with the same 0x23afc0 call.

**PlayAnimation 0x268240** (asm read): mode = the low byte of the raw argument word 3 (no float conversion; the port reads the argument as a number as for the other raw-word natives, [script-natives-progress.md](script-natives-progress.md)), speed argument 2, blend argument 4, then `AnimationOffset` through 0x2451d0 (the int/bool parameter lookup) applied by 0x22c2fc when not 0. Class message 253 with {u16 0, animation, 0.0} goes to the object first.

**Waits 0x2a06a8 / 0x263ae4**: slot 0x233900(entity + 0x40); none or no animation -> done; kind (0x245f84 = 0x22f560 of the animation's slot) 6 -> done; kind 1 or 5 -> done when `|length (0x23caa4) - position (0x213140)| <= 1.0` (float bits compared with 0x3f800001); every other kind (0 still playing, 2, 3) blocks. So a play-once animation blocks until it finishes, a looping one until its position is within one frame of the length.

**Attach.** 0x245008(parent, child) links the child under the parent (child +4 = parent, parent +8 = child, child +0xc = the old first child) and makes the child's local matrix from both world matrices (+0x5c), so the child keeps its world pose; 0x245048(parent, child) unlinks. AttachObject(A, B): B's node leaves its parent and goes under A's, B's halfword +0x10 loses 0x400, A gets class message 43 with B, B's bit 0x40 says whether its new parent is the scene's root node. AttachToBone(A, B, bone of A, bone of B or "relative"): 0x20f33c(A, B, bone, matrix) unlinks B, finds A's bone through A's animation player (0x24f444, 0x227788), attaches B's node to it (0x22690c) with the inverse of B's own named bone as offset when B is skinned (type `*0x37674c`), else 0x241590 for "relative" or 0x22acdc. AttachToBoneRot(A, B, bone of A, x, y, z degrees) uses the rotation matrix instead. In lvl_01: `AttachObject(1-3_Lawnmower_01, 1-3_Gardner_Prop)` (the gardener rides the mower), `AttachToBone(1-7_AI_GrappleGiver, GrappleGun, "weaponRight", "handle")`.

**The van driver (ANIM-7).** `1-1_VanBreakdown` ends with `EnableCollision(1-1_Mail_Box, 0)`, `EnableCollision(1-1_AI_DonutLady, 0)`, `EnableObject(1-1_AI_DonutLady, 1)`, `Wait(0.1)`, `PlayCharAnim(1-1_AI_DonutLady, "postboxstuck_idle_minifig", "Class_generic", 1)`; `1-1_Found_Van` only plays `idle_minifig` (loop 0) on Player1. In the original the driver's slot plays that clip in mode 1 with end 42 at her placement ([npc-animation.md §2026-10-15T18:33](npc-animation.md#ts-20261015-183306)). No offset and no position change: the stuck pose is the animation (42 frames, 15 fps, loop bit). `chase-game --level ...lvl_01... --no-start-cutscenes --call-script 1-1_VanBreakdown --start "875.2 52.79 704.6" --facing 240 --frames 760` now shows her legs out of the mailbox; the build before showed her standing in front of it (pictures not kept). An original picture was not taken (no RPC this round).

**Port.** `runtime::playCharacterAnimation` (0x295a8c rules), `runtime::playObjectAnimation` (0x268240 after its loaded check), `AnimationDriver` (play, stop, the wait test), `ScriptRunner::setAnimationDriver`, `loadAnimationOffsets`; `game_characters::LevelCharacters` drives the level characters. Tests `animation-slots`, `natives-animation` and `natives-animation-real-data` (the VanBreakdown play, the clip's length and loop bit, the attach direction of the mower startup).

Unknowns:
- State requests 80 / 81 and what that state does after a play-once animation; class message 253; 0x1fb894 and 0x2b9e2c after the start.
- The loaded set of an entity (0x21e8b4): which animations a class loads by itself. The port knows only LoadAnimation and the idle clips, so `PlayAnimation(1-4_Booster_Strange, "cower_minifig", ...)` (no LoadAnimation) does nothing in the port.
- What 0x22716c decides for level characters and the folder from entity +0x40; 0x23e6e4's path rules.
- Player1 (the player entity) and props have animation players the port does not drive.
