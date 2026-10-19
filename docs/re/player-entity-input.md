# Player entity and input

Where the player entity lives, how the stick becomes intent, where it gets its position and character, head attachment and appearance.

## Current state

- The player entity is "Player1.Player01" (looked up by 0x19f41c), stored in 0x3772c0 and 0x3772b8; its node gives the world matrix at node+0x5c, controller = `*(entity+0x7c)`. [§2026-10-13T10:42:27](#ts-20261013-104227)
- Stick to intent (0x200354): each axis becomes `sqrt(|v|) * v` unless the entity type is 0x14b or 0x189; heading is `atan2(slot 14, slot 15) * 10430.378` plus the camera yaw word, wrapped to 16 bits; the heading is 0 when the stick is inactive. [§2026-10-13T10:59:11](#ts-20261013-105911)
- The active test uses each channel's own dead zone; the walk flag (ctrl+0xc bit 3) is set only when the raw `x^2 + y^2` is strictly below 0.5. [§2026-10-13T10:59:11](#ts-20261013-105911)
- The atan2f at 0x2473ec is a custom routine, ported bit-exact as `gameAtan2` (needs `-ffp-contract=off`); the D-pad heading path is dead code. [§2026-10-13T10:59:11](#ts-20261013-105911)
- The yaw word 0x376894 is written by the camera manager update 0x23f174, and the player's entity type is 3. [§2026-10-13T14:41:13](#ts-20261013-144113)
- Spawn transform is `Player1Start` (not the `Player1` object); the starting character is `char_player` with CharacterEnum 9 ("police") as a guess. [§2026-10-13T11:08:12](#ts-20261013-110812)
- Head attachment: builder 0x2442b4 attaches `HeadModel` on "head" and `HatModel` on "hat" [§2026-10-13T12:56:22](#ts-20261013-125622); the head is drawn with its model's node matrix (0, 1.364, 0.0381) on the joint's skin matrix: head node matrix * skin(head) * player (the node+0x10 consumer is not read) [models.md §2026-10-19T19:24:18](models.md#ts-20261019-192418).
- The appearance path is shared with level characters (`addCharacter`, `poseBody`, `placeCharacter`). [§2026-10-13T16:34:30](#ts-20261013-163430)
- Open: gate byte 0x372d44 writers, the draw-time consumer of node+0x10, the real start character and hat assembly, and the AttachRelative parameters.

<a id="ts-20261013-104227"></a>

## 2026-10-13T10:42:27+02:00 — Player entity pointer, scene node, stick-to-intent (worker, REA Ghidra 12.1.2; key claims re-checked with objdump)

Corrected by [camera-follow.md §2026-10-13T14:39](camera-follow.md#ts-20261013-143958): the yaw writer is the camera manager update 0x23f174
Corrected by [player-entity-input.md §2026-10-13T10:59](#ts-20261013-105911): active test of the stick and the dead D-pad path
Corrected by [player-entity-input.md §2026-10-13T14:41](#ts-20261013-144113): the yaw writer is the camera manager update 0x23f174

Binary: artifacts/game/game.arm.elf (sha256 5152ed14…0881).

- **Player pointer (confirmed, re-checked).** 0x19f41c looks up the entity named "Player1.Player01" (string at 0x19f498) with 0x23b350(*(0x589a18+4), name). It stores the result to 0x3772c0 (literal 0x19f490, store at 0x19f440) and 0x3772b8 (literal 0x19f4ac, store at 0x19f44c), writes 0xff to entity+0x19 and calls 0x224f54(entity, 0). It is the only writer of 0x3772b8 found by a literal-pool scan. Both words are zero in the static image. The player controller compares against 0x3772b8 (0x26be30, 0x2003bc).
- **Scene node (strong inference).** entity+0x3c points to a node. 0x250810(node) refreshes the world matrix when node word 0 has bit 0x1000 and returns node+0x5c, a 64-byte row-major world matrix (translation in row 3). So the world position is node+0x8c/+0x90/+0x94 and row 2 (node+0x7c..+0x84) is the forward axis. Local translation: node+0x4c..+0x54; parent link node+4.
- **Controller block.** ctrl = *(entity+0x7c).
  - ctrl+6: s16 intended heading, 32768 = π (scale 10430.378 = 0x4622f983). Stores at 0x2007e0 (re-checked) and 0x200864.
  - ctrl+0xc: u16 held flags (bit 0 move requested, bit 3 = 8 "walk" below, …); ctrl+0xe: u16 press-edge flags; ctrl+0x14/+0x15: state / next state; ctrl+0x6c: u16 entity type; ctrl+0x50: event handler stack (dispatch 0x21e1d4).
- **Stick to intent (0x2007a0..0x200864).**
  - Stick x/y are the f32 values of input slots 14/15 (0x376a20/0x376a24); active when |v| > the channel deadzone.
  - Unless the entity type is 0x14b or 0x189, each axis becomes sqrt(|v|)·v (0x25b578 is a vsqrt; the multiply at 0x2007c4 re-checked).
  - Heading = (s16)(atan2(x', y') · 10430.378) via 0x24765c (x first, so 0 = stick up), then plus (s16)(*(f32*)0x376894 · 10430.378). 0x376894 behaves like the camera yaw in radians (22 readers, writer not found).
  - Walk flag: ctrl+0xc |= 8 when x² + y² < 0.5 on the raw values (compare with 0x3f000000 at 0x200820, re-checked), i.e. |stick| < 0.7071. That this flag selects a walk speed is a guess.
  - Events through ctrl+0x50: 9 for move, 6/7 on press, 0xc/0xd on A/B release, 200 for X.

Unknowns:
- The locomotion code (event 9 handlers, per-state table *0x3771f0 built at 0x2a3c18) was not found; walk/run speed, acceleration and turn rate are probably data-driven (reaction objects built from scripts or character data). Attribute tables 0x16ff98 (maxSpeed 0.5, maxAccel 0.05, Drag 0.9) and 0x16e224 (TurnSpeed 0.1) belong to other object classes as far as checked.
- Who writes the node position each frame. Next step: oracle traces of the world position under fixed stick input, and a write watchpoint on node+0x4c.

<a id="ts-20261013-105911"></a>

## 2026-10-13T10:59:11+02:00 — Stick-to-intent replica, library atan2f, dead D-pad path (worker; objdump of artifacts/game/game.arm.elf, qemu-arm for atan2)

Replica: src/runtime/player_intent.{hpp,cpp} (`stickIntent`, `gameAtan2`), checks in tests/runtime_test.cpp.

- **Frame order in 0x200354 (confirmed).** The prologue zeroes ctrl+6 (heading), ctrl+0xc (held) and ctrl+0xe (press) at 0x2003b0..0x2003b8. So the heading is 0 on every frame in which the stick is not active; nothing writes it in that case. The whole body is skipped when the entity is not *0x3772b8 (0x2003c8) or 0x235f08() (byte at 0x377508) == 3 (0x2003d8).
- **Slots (confirmed).** Literal pool 0x200744 = 0x376a20 (slot 14), 0x200748 = 0x376a24 (slot 15). Entry i of 0x3769e8 is a channel index; channel record = 20 bytes at *(*0x3769e4 + 0x14): +0 f32 value, +8 f32 dead zone, +0xc threshold, +0x10 s16 press, +0x12 s16 release (matches src/runtime/pad.hpp). The first axis passed to atan2 is slot 14 (s16 at 0x2007a0), the second is slot 15 (s17 at 0x2007a4).
- **Active test (0x200588..0x2005d0, [Corrects player-entity-input.md §2026-10-13T10:42:27](#ts-20261013-104227): "active when |v| > dead zone" only in detail).** Active iff |slot14| > dz(slot14) (vcmpe + bgt, strict, false for NaN) or dz(slot15) < |slot15| (vcmpe s1,s0 + bhs skips on <= and on unordered). Each axis uses its own channel dead zone (+8). On active r7 = 1 and r5 = 0xffffff81 (mvn r5,#126).
- **Gate 0x200654.** 0x1aa8ac returns the signed byte at 0x372d44. If nonzero: bits 0 and 1 of ctrl+0xc and ctrl+0xe are cleared and r5 = 0, which ends the stick path at 0x200698 (this happens even when the stick is inactive, as long as the earlier button code reached it). 0x339d30 is `mov r0,#0; bx lr` (0x339d30), so its "clear everything" branch (0x200688..0x200694) never runs.
- **Axis shaping (confirmed).** 0x25b578 is `vsqrt.f32 s1,s0`. Unless ctrl+0x6c (u16) is 0x14b or 0x189 (0x200778..0x2007a0: type - 256 - 75 == 0, else type - 331 == 62), each axis becomes `sqrt(|v|) * v`, slot 14 first (0x2007b0..0x2007c4).
- **Heading (confirmed).** 0x24765c is a branch to 0x2473ec, the library atan2f(s0, s1) with s0 = shaped slot 14 and s1 = shaped slot 15 (standard atan2(y, x) argument order, so 0 = slot 15 positive, i.e. stick up; slot 14 positive gives +16384). Result * 10430.378 (0x4622f983, float multiply at 0x2007d4) -> vcvt.s32.f32 (round toward zero, saturating, NaN -> 0) -> strh to ctrl+6 (low 16 bits). Then 0x200848: `ldrh` ctrl+6 (zero-extended), add (int)(f32 at *0x200bf8 * 10430.378) (the literal 0x200bf8 points at 0x376894; an earlier entry in this file named it the camera yaw), strh low 16 bits. So the final heading is `(u16)((int32)(yaw*k) + (u16)(int32)(atan2*k))` as an s16, wrapping modulo 2^16. Held flags: ctrl+0xc |= 1 at 0x200758, |= 8 at 0x20082c.
- **Flag 8 (confirmed).** s0 = |slot14|^2 then s0 += |slot15|^2 (vmul, vmla, both rounded), the float bits are compared as a signed integer with 0x3f000000 (cmp at 0x200820, bge skips): the flag is set only if the sum is strictly below 0.5. A sum of exactly 0.5 does not set it. It uses the unshaped channel values, re-read from the channel array at 0x2007f0..0x200808.
- **Library atan2f 0x2473ec..0x24765c (confirmed bit-exact).** It is not glibc/libm: it handles non-finite/zero/tiny/huge operands by bit tests on the exponent window [24, 252] (0x2473f4..0x247474), takes the quotient directly when the exponent difference is in [-26, 27] (reduction by atan(0.5) with the hi/lo constants 0x3eed6000/0x37ce0ac3 when the operands are within one binade, 0x247544..0x247588; |first| > |second| is mapped through +-pi/2 hi/lo 0x3fc90000/0x39fdaa22), then a 4-term polynomial in t^2 with VFP non-fused vmla chains (0x2475bc..0x2475f4). C++ port `gameAtan2` matches the extracted code run under qemu-arm (hand-written harness around the bytes 0x2473ec..0x247f00, outside the repo) on 300441 argument pairs (21x21 special values incl. +-0, +-inf, NaN, denormals, plus random in-range, near-ratio, huge/tiny and raw random bit patterns): 0 mismatches except NaN payloads. Differs from libm by up to an ulp (e.g. atan2f(1e30, 1e30) gives 0x3f490fda). The target needs -ffp-contract=off so vmla stays unfused.
- **Dead code (corrected).** The "D-pad fallback" with the 8-entry s16 table (0x200838: ldrh from sp + r5*2, strh to ctrl+6) and the scripted heading path (0x2006b4..0x200720: heading from the node's forward vector via atan2 and the byte 0x374513 compare) are unreachable. r5 only ever holds 0 or 0xffffff81 before 0x2006cc, and the path needs `ldrb [0x374513] == r5`, which a byte can never equal 0xffffff81; 0x200838 needs r7 == 0 together with r5 != 0 at 0x200698/0x200768, and r5 != 0 implies r7 == 1. The table copied from 0x357e58 to the stack (16 s16: 0, 16384, -16384, 0, 0, 8192, -8192, 0, -32768, 24576, -24576, 0, 0, 0, 0, 0) is therefore not consulted. The D-pad itself does not feed the heading; slots 0..3 (channels 12..15) are not read in this function.
- **After the step.** 0x200868 stores r5's low byte (0x81 when active) to 0x374513 and clears the byte at 0x3772bc; 0x20087c clears 0x3772bc when the stick path is skipped. Then flags decide events (0x200884..0x200984, see above).

Unknowns:
- Writers of 0x372d44 (the gate byte) and 0x3772bc, and whether anything ever writes 0x374513 other than 0x200868.
- Whether the real VFP11 on the 3DS flushes denormals (the replica assumes IEEE with denormals; irrelevant for stick values above 1e-38).
- The meaning of the heading beyond its scale (32768 = pi) and the reaction to flag 8 are still not recovered (no locomotion code found).

<a id="ts-20261013-110812"></a>

## 2026-10-13T11:08:12+02:00 — Where the player entity gets its position and character (worker; objdump of artifacts/game/game.arm.elf, .blvl dumps with parameter names from code.bin)

Corrected by [player-entity-input.md §2026-10-13T12:56](#ts-20261013-125622): the head attachment rule

Replica: `playerStart` and `characterAppearance` in src/assets/level.cpp, used by `placePlayer` in src/game.cpp; checks in tests/assets_test.cpp (synthetic) and tests/assets_real_test.cpp (pinned real values).

- **Entity source (confirmed from data).** `levels/players.blvl` holds 16 objects `Player01..Player16` of class `chars/char_player` (no parameters). Every `*_main.blvl` (8 of 8) has an object of class `players`, instance `Player1`, room `Objects`. In lvl_00_policestation_main it has initialposition (-2.22, 2.0901, 13.6475) and initialorientation (0, -90, 0). The name looked up at 0x19f41c, "Player1.Player01", is therefore `<instance of the players object>.<instance in players.blvl>`. That the dotted name is built this way is inferred from the names; the registry code that composes it was not read.
- **Spawn (confirmed in code).** The level-start function at 0x2e6264 picks the loaded level's object registry (r7), then at 0x2e6334 looks up the object named "Player1Start" (string 0x2e668c, `add r1, pc, #848`) with 0x23b350. 0x213284(entity, out) builds a matrix from entity ctrl +0 (position, copied to out+0x30 by 0x2554c0) and ctrl +0xc (Euler angles through 0x237c48, the same rotation as `placementMatrix`). 0x24c118(*0x3772b8 -> node +0x3c, matrix) stores it as the player node's world and local matrix (0x2e6340..0x2e6374). So the spawn transform is the Player1Start transform, not the `Player1` object's.
- **Data.** All 8 `*_main.blvl` have a `point` object `Player1Start`. lvl_00_policestation_main: room1, initialposition (-2.1713, 1.4203, 17.2821), initialorientation (0, 180.9585, 0). `xx_skipto` "SkipToLobby" has the identical transform. With `placementMatrix` the forward axis (row 2) is (-0.017, 0, -1), toward the lobby desk. A render with the character model at this transform stands on the lobby floor.
- **Character (partly inferred).** `chars/char_player.blvl` class defaults (section +0x10, now kept by `parseLevel` as `Level::defaults`): meshname `char_minifig`, HeadModel `Char_Head_TEST`, FaceTexture `head_TEST.tga`, LowerTexture `lower_TEST.tga`, UpperTexture `upper_TEST.tga`, CharacterEnum 0. The shared models carry those names (char_minifig parts `lower_Test.tga`, `upper_TEST.tga`; the head part `head_TEST.tga`), so a character class replaces them. `chars/char_police.blvl` has CharacterEnum 9, HeadModel `Char_Head_Chase`, head_ChaseMcCainDIFF64_c4x4, lower_ChaseMcCainDIFF64, upper_ChaseMcCainDIFF64_uk. CharacterEnum equals the row in tablescharacters sheet 0 (row 9 = "police"). Row 9 is the only row with class byte +0xc == 1 and is the one excluded from the completion count (docs/re/save-format.md). **Guess:** it is the character the player starts with. Nothing read so far selects the character at start (the save/character-select path was not traced).
- **Assembly (guess, matches the render).** The body model is skinned (21 joints, skeleton char_minifig.bfnskl). The head model has its vertices relative to the "head" joint (bbox y 0..0.66; joint at y 1.348). Drawing it with bind(head) · skin(head) · player matrix puts it on the neck. Head and body textures are replaced by name; the real texture-binding code was not read.
- **Idle pose.** models/char_minifig/class_police/idle_minifig.bfnanm frame 0, chosen by file name only. The game's animation selection (tablesanimations.bxls) was not traced; the playback rate is still unknown, so the pose is static.

Unknowns:
- Which code builds "Player1.Player01" (registry key composition) and how the `Player1` object's position (y 2.0901, yaw -90) relates to the Player1Start placement; whether the level start of a saved game uses Player1Start too (the function at 0x2e6264 was not read beyond 0x2e6460).
- The starting character, the hat/hair assembly (the `hat` joint, char_head_policehat) and the face texture binding.
- Which of Player01..Player16 is active for one player (the code at 0x19f41c uses Player01).

<a id="ts-20261013-125622"></a>

## 2026-10-13T12:56:22+02:00 — Head attachment: correction of the "Assembly (guess)" entry (worker; objdump of artifacts/game/game.arm.elf, model/skeleton dumps through chase_assets)

Corrected by [models.md §2026-10-19T19:24:18](models.md#ts-20261019-192418): the head model carries its offset (0, 1.364, 0.0381) as its node matrix, which the model draw applies; the head hangs on the joint's skin matrix without the bind translation.

Replica: `jointAttachment` in src/assets/anim.cpp, used by `placePlayer` in src/game.cpp; pinned in tests/assets_test.cpp (synthetic) and tests/assets_real_test.cpp (police idle frame 0).

- **[Corrects player-entity-input.md §2026-10-13T11:08:12](#ts-20261013-110812).** The earlier entry's rule "bind(head) · skin(head) · player" was wrong. bind · skin = bind · bind⁻¹ · animated = the animated head world, which still carries the head joint's bind orientation. In char_minifig.bfnskl joint 9 "head" has world rows (0,0,-1), (1,0,0), (0,-1,0) (a 90 degree twist; joint 10 "hat", its child, has an identity world rotation), so the upright head model (vertices y 0..0.57, x/z ±0.26) was turned sideways.
- **Who attaches the head (confirmed in code).** The character builder at 0x2442b4 (read from 0x2442b4 to 0x244880) reads the object parameters "meshname", "AttachModel%02d"/"AttachBone%02d"/"AttachRelative%02d" (4 slots), then "HeadModel" (string 0x24489c) and "HatModel" (0x2448a8). Each model name is passed to 0x2121e8(entity, modelName, boneName, 0, 0) with boneName = "head" (string 0x2448b4) for HeadModel and "hat" (0x2448bc) for HatModel. 0x2121e8 builds "models/<name>/<name>.fnmdl" (0x2123cc loads it), adds it as a child node of the entity's node (0x22690c on entity+0x3c), and binds it to the skeleton joint named by the string: 0x24f444(entity+0x44, name) -> 0x2023bc stores {skeleton, joint} in a 0x38-byte record (+0x30/+0x34), and 0x242814 stores that record at node+0x10. The code that consumes node+0x10 when drawing was not found.
- **Where the head model's origin sits (data).** The cutscene minifig models (csdata/models/<id>/<actor>/<actor>_minifig.bfnmdl) carry the head as a separate unskinned mesh whose node is named "head" with node offset (0, 1.3640, 0.0381) (node record +4). Its vertices equal the standalone Char_Head_Test vertices plus that offset (bbox y 0..0.568 -> 1.364..1.932, z -0.26..0.26 -> -0.222..0.298; mesh centre 0.2839 -> 1.6478). So a standalone head model's vertices are relative to a pure translation, with no rotation, from the head joint's bind position. The skeleton's head joint is at (0, 1.348, 0.0381).
- **Rule implemented.** head matrix = translate(head joint bind position) · skin(head) · player, i.e. the bind orientation is dropped and the joint's skin matrix (inverse bind · animated world, 0x252cb0) moves the head. For the police idle frame 0 the result is an upright head (rotation diagonal 0.9999, 0.9992, 0.9992) at (-0.036, 1.309, -0.008) in model space. chase-game now shows the hair centred on the back of the head, as in the oracle references gameplay-start.png and gameplay-stick-up-60.png (compared by eye; no pixel comparison).
- Char_Head_Chase is a single mesh holding hair (texture hair_ChaseDIFF64_c4x4, 161 vertices) and face (head_TEST.tga slot, 191 vertices), so there is no separate hat for the starting character; "hat" is only used when a HatModel parameter exists.
- Cutscene actors do not exercise this path: their heads are baked into the actor's own model, and the viewer leaves unskinned meshes (the head) static in the bind pose (src/render/scene.cpp poses only skinned parts), so the cutscene check does not validate the attachment.

Unknowns:
- The draw-time consumer of node+0x10 (whether the game applies skin(head), the animated head world with a different frame, or something else). The rule above is inferred from the baked cutscene data and the render, not read from the draw code.
- The baked head node offset y 1.3640 differs by 0.016 from the head joint's bind y 1.348; the runtime path may add an offset (AttachRelative%02d parameters were not traced). The implementation uses the joint position.
- Whether unskinned meshes of cutscene minifigs (the baked head) follow the head joint in the real game; the viewer does not move them.
- Orientation of the "hat" joint attach (identity bind world) and the models used with HatModel were not checked.

<a id="ts-20261013-144113"></a>

## 2026-10-13T14:41:13+02:00 — Camera yaw word and player entity type (see camera.md)

- **[Corrects player-entity-input.md §2026-10-13T10:42:27](#ts-20261013-104227):** the writer of the f32 at 0x376894 is the camera manager update 0x23f174: it stores atan2(row2.x, row2.z) of the camera scene node's world matrix (the yaw of the view direction). Its camera law is in [camera.md](camera.md); the earlier note that it was not recovered is obsolete.
- The player's ctrl+0x6c (entity type) is 3 (read from the locomotion traces), so the stick shaping applies and the stick x turns the camera (types 1..5 and 0x11c). chase-game uses 3.

<a id="ts-20261013-163430"></a>

## 2026-10-13T16:34:30+02:00 — Appearance rule shared with level characters (see objects.md)

The head/face/upper/lower replacement and the head attachment of the player are now one library path (`addCharacter`, `poseBody`, `placeCharacter` in src/render/character.cpp) used by chase-game for the player and for the level's characters. The character-select display 0x27a2bc replaces the `*_TEST.tga` placeholders on body (upper_/lower_) and head (head_) by the character class's names in the same way and attaches the head model on the `head` joint, which confirms the rule; details and the custom-character path (0x2372e0) in [objects.md](objects.md).
