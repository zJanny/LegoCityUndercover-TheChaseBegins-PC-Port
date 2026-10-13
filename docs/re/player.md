# Player entity and controller

Findings about the player character at run time: where it lives, how input becomes intent, and (later) how it moves.

## 2026-10-13T10:42:27+02:00 — Player entity pointer, scene node, stick-to-intent (worker, REA Ghidra 12.1.2; key claims re-checked with objdump)

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

## 2026-10-13T10:59:11+02:00 — Stick-to-intent replica, library atan2f, dead D-pad path (worker; objdump of artifacts/game/game.arm.elf, qemu-arm for atan2)

Replica: src/runtime/player_intent.{hpp,cpp} (`stickIntent`, `gameAtan2`), checks in tests/runtime_test.cpp.

- **Frame order in 0x200354 (confirmed).** The prologue zeroes ctrl+6 (heading), ctrl+0xc (held) and ctrl+0xe (press) at 0x2003b0..0x2003b8. So the heading is 0 on every frame in which the stick is not active; nothing writes it in that case. The whole body is skipped when the entity is not *0x3772b8 (0x2003c8) or 0x235f08() (byte at 0x377508) == 3 (0x2003d8).
- **Slots (confirmed).** Literal pool 0x200744 = 0x376a20 (slot 14), 0x200748 = 0x376a24 (slot 15). Entry i of 0x3769e8 is a channel index; channel record = 20 bytes at *(*0x3769e4 + 0x14): +0 f32 value, +8 f32 dead zone, +0xc threshold, +0x10 s16 press, +0x12 s16 release (matches src/runtime/pad.hpp). The first axis passed to atan2 is slot 14 (s16 at 0x2007a0), the second is slot 15 (s17 at 0x2007a4).
- **Active test (0x200588..0x2005d0, corrects "active when |v| > dead zone" only in detail).** Active iff |slot14| > dz(slot14) (vcmpe + bgt, strict, false for NaN) or dz(slot15) < |slot15| (vcmpe s1,s0 + bhs skips on <= and on unordered). Each axis uses its own channel dead zone (+8). On active r7 = 1 and r5 = 0xffffff81 (mvn r5,#126).
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
