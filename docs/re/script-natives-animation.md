# Script natives: animation and attach (batch SCRIPT-32)

Handlers of the 13 natives of batch 32 and what the port does with them. Census and the other batches: [script-natives.md](script-natives.md). Items: [script-system-natives.md](../plans/features/script-system-natives.md) (SCRIPT-32, SCRIPT-112, SCRIPT-S110). Argument layout of the handlers: [script-natives-progress.md](script-natives-progress.md) (an object argument at `[r1 + 8 i]`, any other argument as a value pointer at `[r1 + 8 i + 4]`).

## Current state

- The 13 natives of batch SCRIPT-32 are read: PlayCharAnim 0x1bf9cc (-> 0x29e5f0, never blocks), PlayAnimation 0x268240, LoadAnimation 0x2681a0, UnloadAnimation 0x27e9c4, StopCharAnim 0x1bfb84, StopAnim 0x1883d0, the waits 0x263ae4 / 0x2a06a8 and the Attach natives. [§2026-10-14T21:36:49](#ts-20261014-213649)
- PlayCharAnim on Player1 puts the player into the user-animation state 0x50 (update 0x2a47b0 runs no locomotion and requests state 1 when the slot kind is 6; StopCharAnim ends the slot at once); the port poses the script clip through `runtime::PlayerScriptAnimation`. [§2026-10-19T12:55:03](#ts-20261019-125503)
- The play itself is 0x295a8c (find or load the animation, start it with 0x23afc0 at blend 0.1, state request 80 or 81); 0x1b71e8 plays the cape animation, not the looked-up one (this corrects the first reading). [§2026-10-15T18:33:36](#ts-20261015-183336)
- The waits block while the slot kind is 0, 2 or 3 and finish for kind 6, or for kind 1 or 5 when the position is within one frame of the length. [§2026-10-15T18:33:36](#ts-20261015-183336)
- AttachObject, AttachToBone and AttachToBoneRot attach the second object to the first (this corrects the first reading); 0x245008 / 0x245048 link and unlink nodes keeping the world pose. [§2026-10-15T18:33:36](#ts-20261015-183336)
- The van driver stuck in the mailbox is the `postboxstuck_idle_minifig` clip (42 frames, loop bit) started by `1-1_VanBreakdown`, with no offset. [§2026-10-15T18:33:36](#ts-20261015-183336)
- Animated placed props are skinned (joint tracks, no object tracks) and play `models/<mesh folder>/<name>.bfnanm`; the port poses them with `skinMatrices`. [§2026-10-15T20:04:40](#ts-20261015-200440)
- The Attach natives are read but not ported for bone attaches (SCRIPT-272); AttachObject, AttachToWorldRoot and AttachToRoom change nothing visible yet. [§2026-10-15T20:04:40](#ts-20261015-200440)
- Open: state requests 80 / 81, class message 253, the loaded set of an entity (0x21e8b4), the bone-attach matrix (0x258858, 0x23d39c), 0x22716c.


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

<a id="ts-20261015-200440"></a>

## 2026-10-15T20:04:40+02:00 — PlayAnimation, StopAnim and WaitForCurrentAnim on placed props: skinned prop clips; what the Attach natives would change (C++ loaders over the archive, llvm-objdump; no oracle run)

Source: `chase::assets` loaders over `lego_city3ds.fib` (listing of the calls and clips on 2026-10-15), llvm-objdump of `game.arm.elf`. Completes the prop unknown of [§2026-10-15T18:33](#ts-20261015-183336).

**The animated props are skinned, not node-animated.** The scripts of `lvl_01_cherrytreehills_main.blvl` call PlayAnimation on `police_garage_door_01`, `FP_Police_Garage_FPDoor02`, `FP_LombardStreet_GarageDoor01` / `02` (mesh `Prop_Garage_shutter`, clip `open`: 60 frames at 15 fps, no loop), `1-2_Dog_Cage` (`open`: 44 frames, loop bit), `1-2_Dumpster_Open` (`dumpster_peak` 84 frames loop, `dumpster_exit` 29 frames), `1-2_Dog` (mesh `char_dog`, `dog_cage_open` 45 frames), `1-3_crane`, `1-3_crane_hook` and `1-3_Lawnmower_Final` (`cherry_crane_sequence`, 150 frames), `1-3_Lawnmower_01` / `02` (`use`, 15 frames, loop), `1-3_Gardner_Prop` / `_02` (`ride_lawnmower_minifig`, 21 frames at 30 fps, loop), `1-3_Crow` (`pecking` 41 frames, `fly` 6 frames, both loop). Every clip has joint tracks and no object tracks (`Animation::objects` is empty), and the model has a `.bfnskl` skeleton next to the `.bfnmdl`; the clip path is `models/<mesh folder lower case>/<name>.bfnanm`, the name lower-cased. So the pose of such a prop is the skin of `skinMatrices(skeleton, animation, frame)` (the per-node transforms of build-it bricks do not apply). The target of every call is a placed object of class `leprops/leprop_simpleindestructible`, `leprops/leprop_prop`, `props/prop_prop`, `props/prop_platform`, `leprops/leprop_platform` or `ledummy/ledummymesh`.

**Waits.** The garage shutter takes 59 frames / 15 fps = 3.93 s after PlayAnimation; `WaitForCurrentAnim` on the four garage doors therefore blocks that long (the test below pins the moment: not near the end after 100 updates of 1/30 s, near the end after 120).

**Attach natives (SCRIPT-272), read but not ported.** AttachToBone (0x1ddb90) calls 0x20f33c(parent, child, bone name, matrix): the child's node is unlinked from its parent (0x245048), the parent's bone is found by name (0x24f444 on the parent's animation player entity +0x44, 0x227788 gives its world matrix), the child's local matrix is built from the argument matrix (0x258858, 0x23d39c) and set on the node (0x24c118), then the node goes under the parent's bone node (0x22690c). In lvl_01 the only attaches with a visible effect are the bone attaches on characters (`GrappleGun` on `1-7_AI_GrappleGiver` bone `weaponRight`, the booms and cameras of `1-3_*_Man_*` on `weaponright` with a rotation) and attaches under a moving parent; the port moves no parent object (no code handles `props/prop_platform` or `leprops/leprop_platform` objects such as `1-3_Ledge_Platform` and the mine cart; only carried objects, doors and vehicles change their transform), so AttachObject, AttachToWorldRoot and AttachToRoom change nothing that can be seen yet. Whether a parent's root node moves while a skinned clip plays was not read.

**Port.** `src/runtime/prop_animation.*` (`PropAnimations`: the slot player of each prop, `animationTargets`, the skin per update), `src/runtime/animation_driver_chain.hpp` (characters and props behind the one driver of the script runner), glue in `src/game_level.cpp` (a prop that is an animation target is added under its own scene key `propSceneKey`, so `Scene::pose` re-skins only that instance) and `updateLevelCharacters`. Test `prop-animation-real-data`. A run of `chase-game` with `--call-script 1-2_Cage_Key_Target_Script` for 120 updates completes; no picture was compared with the original.

Unknowns:
- The frame sampling of the original for a prop at a fractional position: the port uses `skinMatrices` as for characters (blend of the neighbouring frames).
- The matrix of the bone attach (0x258858 from the argument, 0x23d39c) and the composition with the parent's bone: not transcribed, so the guns, booms and cameras stay at their placement (SCRIPT-272).
- Class message 253 and the calls after the start (0x2b9e2c) for props, and which animations a prop class loads by itself.

<a id="ts-20261019-125503"></a>

## 2026-10-19T12:55:03+02:00 — PlayCharAnim, StopCharAnim and the character wait on Player1: the user-animation state 0x50 hands the pose to the script clip and back (llvm-objdump of `game.arm.elf`, data words from `code.bin`; no oracle run)

Extends [§2026-10-15T18:33:36](#ts-20261015-183336) and the state description of [npc-animation.md §2026-10-18T14:28:08+02:00](npc-animation.md#ts-20261018-142808).

**State table.** The words at 0x3577f4..0x357828 (read from `code.bin`) are {id, update, enter, 0, 0, name pointer, 0}: state 0x50 = {update 0x2a47b0, enter 0x25fbec}, 0x51 and 0x52 the same pair. A state request goes 0x2414b4 -> 0x2eafc8, which only stores the state object at [machine+0xc], sets the byte [machine+0x14] = 1 and tail-calls 0x1eb868 (copies the state ids of the stack entries into +0x1c / +0x1e). The enter and update functions run from the state runner 0x2424d0 in the controller update, so a request made by a script native takes effect in a later controller step, not inside the native.

**PlayCharAnim 0x29e5f0, tail after the play.** 0x295a8c (play), 0x241304(entity) (sets ctrl+0x15 = 1), then 0x2414b4(entity, ctrl+0x50, 80, 0); with the caller flag r2 non-zero (the cutscene call) the request is 81. Then 0x1b71e8 (cape lookup) and, when it finds one, 0x23afc0 and 0x1fb894 on the cape. Nothing in this path touches the locomotion state except the request.

**Enter 0x25fbec** (VFP code that llvm-objdump prints as `<unknown>`, decoded by hand): `ctrl+0x2b8 = 0.0`, `ctrl+0x315 |= 8`, return. The function after it (0x25fc08: `ctrl+0x24c = 0.0`, halfword `[r0+0x10] &= ~0x200`) belongs to another state.

**Update 0x2a47b0(entity, ctrl)** (disassembly): kind = 0x22f560(0x233900(entity + 0x40)); when 6, 0x2414b4(entity, ctrl+0x50, 1, 0); then 0x215804(entity, ctrl, flag = ctrl+0x6c is 80 or 82, 0) and `ctrl+10 = ctrl+8`. 0x215804 runs the root-motion mover 0x20cd30 after a heading step (0x23df04 / 0x23deb4) in one case (machine byte +0x14 is 10, and the entity is not `*0x37719c` or ctrl+0xd8 bit 4 is clear). No locomotion function runs in this state, so the stick, jumps and attacks do nothing while the clip plays.

**StopCharAnim 0x1bfb84:** 0x22dacc(object), 0x233900(entity + 0x40), 0x225c5c(slots), returns 1. 0x225c5c on a slot with an animation whose mode is not 6 pauses it (0x24f8fc) and sets mode 6, so the next update of state 0x50 sees kind 6 and requests state 1; locomotion runs again one controller step later. It does not request a state itself. The wait 0x263ae4 reads the same slot, so it ends in the update that sees kind 6 and, for a play-once clip, in the update the clip ends.

**Port.** `runtime::PlayerScriptAnimation` (`src/runtime/player_script_animation.*`, an `AnimationDriver` for `player1`): `play` uses the 0x295a8c slot rules and queues state 0x50; `update(dt)`, called first in the player update, applies the queue and then the 0x2a47b0 rule; while it returns true `PlayerRig::poseClip` poses the clip frame (clamped to the last sample) and the stick is treated as taken like the other control-takers; `stop` is 0x225c5c; `currentNearEnd` is the slot test, or nothing once locomotion has taken over (the wait then completes at once, as before). Tests `player-script-animation` (synthetic idle clip: 19 frames, 15 fps, loop bit; the numbers come from [npc-animation.md §2026-10-15T09:29:00](npc-animation.md#ts-20261015-092900)) and `player-script-animation-real-data` (the same sequence on `idle_minifig.bfnanm`). `chase-game --call-script 1-2_Cage_Key_Target_Script` runs without error; the animation was not compared with the original.

Unknowns:
- Whether the update that applies the request already runs 0x2a47b0 (the port does) and whether the previous state's update runs in the update that switches.
- 0x215804 / 0x20cd30 with the user-animation flag: whether the clip's root motion moves the player (the port applies none), the heading step, and what ctrl+0x315 bit 3 and ctrl+0x2b8 = 0 from the enter do.
- The cape animation 0x1b71e8 (whether the police character has one), 0x1fb894 / 0x2b9e2c after the start, ctrl+700 / ctrl+0x310 and PlayAnimation's message 253 (all SCRIPT-273).
- Whether gravity and the vertical state keep running in state 0x50 (the port keeps its vertical step), the blend of 0.1 between the script clip and the next locomotion clip, and the folder rule 0x22716c for the player (the port uses `models/char_minifig/<class>/`).
