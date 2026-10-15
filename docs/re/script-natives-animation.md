# Script natives: animation and attach (batch SCRIPT-32)

Handlers of the 13 natives of batch 32 and what the port does with them. Census and the other batches: [script-natives.md](script-natives.md). Items: [script-system-natives.md](../plans/features/script-system-natives.md) (SCRIPT-32, SCRIPT-112, SCRIPT-S110). Argument layout of the handlers: [script-natives-progress.md](script-natives-progress.md) (an object argument at `[r1 + 8 i]`, any other argument as a value pointer at `[r1 + 8 i + 4]`).

<a id="ts-20261014-213649"></a>

## 2026-10-14T21:36:49+02:00 — Batch 32 (SCRIPT-32): the 13 handlers read, all run as stand-ins (llvm-objdump on game.arm.elf, C++ runner tests; no REA session, no emulator for these handlers)

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
