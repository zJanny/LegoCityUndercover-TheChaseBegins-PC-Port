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
