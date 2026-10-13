# ARM executable, services, input and main loop

Reverse-engineering findings, moved verbatim from the progress log. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis.

## 2026-10-12T15:10:00+02:00 — S3 focused analysis (REA, Hopper provider, ELF sha256 5152ed14…0881)

- Opened artifacts/game/game.arm.elf; binary_overview ev_58c1dadc79e269e074c9c5fe98ce23a3b06199eb833f901f82be0d66b789bc54: 3 segments, 963 procedures, 0 strings defined (no sections, so Hopper defines no strings).
- Entry 0x100000 (ev_e86237bf46c633b755ad47fe4e25bff928d851aafa8760d5bd0e03b49ecbdbd1): `bl` chain sub_100024, sub_10022c, sub_1002c8, sub_1002a0, sub_1336cc, sub_1001f8, sub_100280, sub_100108, then `b` to svc 0x3 (ExitProcess). This matches typical CTR SDK crt0; roles of individual init calls not yet confirmed.
- sub_100108 (ev_1f33c6d8d139ac39e591c882c9d2c37299641b8d9d97dbd19ad5ff226101a5ea): 0x930-byte frame; loop `while (sub_100448()) sub_10057c();`, then sub_100324 and sub_100954. Inference (not confirmed): a framework/applet dispatcher, not game logic.
- sub_100448 (ev_cd6d2d3e6cabe488c953db4d96d4c28ea3c6034219fdcef6e4ddd52b8342d324) walks a message list with kinds 1/2/3/7 and calls a virtual method at +0x10. sub_10057c (ev_d3565622686694c11b9731296e21efd0c14fc9f675cc63839da706915d17d043) calls sub_25cb2c(*0x376310, 1).
- SVC census from local llvm-objdump (armv6k, text only; local evidence, not REA). Each SVC appears exactly once in its own stub: 0x01 0x03 0x08 0x09 0x0A 0x0C 0x14 0x17 0x18 0x19 0x1F 0x20 0x21 0x22 0x23 0x25 0x27 0x28 0x2D 0x32 0x35 0x38 0x3A 0x3C. There are 2 spurious `svc #0xed6b90` hits in data. 24 stubs named in REA using standard 3DS SVC numbering (ev_4f958eba9c138769e7849bda90005ec82a681b6cf30b4e97375b17a151f88f98), e.g. svcSendSyncRequest 0x25947c, svcConnectToPort 0x105b08, svcExitProcess 0x25d4d4.
- Service names in rodata: cecd:u/s, dsp::DSP, fs:LDR, fs:USER, ndm:u, srv:pm, cfg:u/i/s, hid:USER/SPVR, ptm:sysm/s/gets/u/play, err:f, gsp::Lcd, gsp::Gpu. No "srv:" or "APT:U" string was found by this simple search; not yet explained.
- Paths: rom:/ (literal users at 0x128608, 0x1287d8, 0x159020, 0x188fa4), data:/savegame.dat (pointer at 0x377964), levels/triggers.txt, levels/letriggers.txt, pregen/*.bin, Pregen/Traffic|Pedestrians/%s_*.bin, TouchGestures/%s.dat, Localisation/extended_chars_utf8.txt, scriptfns.txt. Assets live in the 769 MB RomFS (not yet extracted).
- Coverage caveat: resolve_containing_procedure returned not_in_procedure for the fs:USER user 0x10f988 (ev_f52ce74dbc497c0f3ce93454138b76efb3281e5ccdc80c18256246e4cac469b2) and the rom:/ literal 0x128718 (ev_d887d2367ebb3881e7d6eef78c3e7ad82dd29c9e652671dfa00ddabd9e3293fe). The local disassembly has ~6697 `push {...,lr}` prologues, so Hopper covers a minority of functions.
- Engine strings (ATTR_*, StudSpawn*, CUFO*) suggest TT Games' engine. This is an unconfirmed inference.

## 2026-10-12T15:25:00+02:00 — S3 DSP/service-handle follow-up

- The user added a permission rule for mcp__rea__resolve_containing_procedure, and the call was retried. For 0x10a080 it returned not_in_procedure (ev_c0e45080d9f6d52a852b58e0a5d3377f6c66d5d7a7303707d43d9b204b691064), the same Hopper coverage gap. The earlier blocker is resolved.
- Local disassembly: function 0x109fc0 loads "dsp::DSP" (literal 0x10a080 at 0x109fe4), calls 0x258fd8 (length) then 0x258fa4(out, name, len, 0), initializes state, and calls 0x110948.
- 0x258fa4 has 11 call sites. Each site with its enclosing function (nearest preceding push…lr) and service name:
  - Name loaded directly:
    - 0x10a004 in 0x109fc0: dsp::DSP
    - 0x10f91c in 0x10f8e4: fs:USER
    - 0x1100a8 in 0x11006c: ndm:u
  - Name not resolved by the direct-literal scan (likely loaded through rodata pointer tables such as 0x359c64 → hid:USER):
    - 0x102f0c in 0x102ec8
    - 0x110164 in 0x110134
    - 0x117320 in 0x1172b4
    - 0x118e70 in 0x118e2c
    - 0x1220f0 in 0x122090
    - 0x12af28 in 0x12aea0
    - 0x131310 in 0x1312d0
    - 0x25b700 in 0x25b67c
  - Inference: 0x258fa4 is srv:GetServiceHandle.

## 2026-10-13T10:45+02:00 — S3 HID input and main loop (REA, Ghidra 12.1.2; read-only, partial)

- Tooling: REA binds the Ghidra provider (full coverage). VA = file offset + 0xff000 in all segments. Local llvm-objdump needs --mattr=+vfp2 for the VFP code.
- 0x1172b4 opens hid:USER and maps 0x2b0 bytes of shared memory into the HID object at 0x5941f8: pad +0, touch +0xa8, accel +0x108, gyro +0x158 (ev_98ab09d2…). Readers: pad (constructor 0x110038) and touch (0x10fca0), built at 0x1075f0 (globals 0x3760c4).
- Pad read 0x10a4dc (ev_b029ae29…): hold &= ~0x2000; trigger = hold & ~prev; release = prev & ~hold; all zeroed when 0x25b8c4() && !0x259648() (meaning unknown).
- Stick: circle clamp 0x119724 with min 40 and max 145 (defaults at 0x117a10; ev_bb8429e5…). |v| ≤ 40 → 0; otherwise length becomes min(|v| − 40, 105), then divided by 105 (0x10fcb0). Touch 0x10a44c gives {u16 x, u16 y, u8 down}.
- Channel layer 0x103dc8 (ev_00990eb9…) writes 20-byte channels {f32 value, u8 type, f32 deadzone, f32 threshold, s16 press, s16 release}.
  - Pad: Up 0x40→12, Down 0x80→13, Left 0x20→14, Right 0x10→15, Start 0x08→6, A 0x01→16, B 0x02→17, X 0x400→18, Y 0x800→19, L 0x200→8, R 0x100→10, stick → ch0/ch1. Select (0x04) is not mapped.
  - Accel → ch24–26, gyro → ch30–41.
  - Touch: ch0 = x, ch1 = y, ch3 = 1 while down, rotated by the global at [0x529e30 + 0x28] (0/90/180/270).
- Per-frame poll 0x25cb2c (ev_ea8b8dd9…): copies previous channels to 0x52f0d0, applies a radial deadzone to (ch0, ch1) and (ch2, ch3) and an axial deadzone elsewhere ((v − dz) / (1 − dz), clamped), and expires a timed list (0x10497c; possibly rumble, unknown).
- Edges 0x25b4b4 (ev_1d25fc79…): press = ±1 when the value crosses ±threshold; release the same in reverse. No key-repeat logic was found.
- 0x168c9c (ev_70129f7a…) creates the pad device (*0x3769e4) and fills the slot table 0x3769e8..0x376a2c with channels 12, 13, 14, 15, 16, 17, 16, 17, 18, 19, 6, 7, 8, 10, then axes 0–3. It also records and plays back input (stream 0x379d7c; 0x25afcc, 0x25b310). Game code reads press edges through these slots (e.g. 0x2005e8 reads ch18). No action names were found; no .bxls table holds control mappings.
- Main loop: 0x100108 registers "LEGO_City3DS" and runs `while (0x100448()) 0x10057c();`.
  - 0x10057c polls the pad.
  - 0x100448 (ev_f5b009a8…) runs one frame: process list kinds 1/3 call Update(dt) (vtable +0x10), kind 2 calls 0x100f44, kind 7 removes.
  - 0x100f44 (ev_68bf373e…) is a fixed-step scheduler: step from config +0x10, catch-up cap +0x70, idle path 0x25c174(1/30).
- Unknown: the GSP vsync call and the step value (30 vs 60 fps), slot action names, bit 0x2000, channel 7.
- Effect on the input lab: the game has no key repeat and Select is unused, which is consistent with the lab's rules. The lab's action names stay 3DS buttons until slot meanings are recovered.


## 2026-10-13T08:55+02:00 — Fixed-step scheduler fields (REA Ghidra 12.1.2 ev_68bf373e…, llvm-objdump; read-only, partial)

- 0x100f44 takes a process object P. P+0xc points to a client object C with vtable +0x1c Update(dt) and +0x24 (called with 1 before and 0 after Update).
  - C+0x10, +0x14 and +0x18 are floats multiplied by the tick frequency 0x25c1f8: the step, the minimum step and the variable-step length.
  - C+0x1c is a float scale applied to the tick count. C+0x20 = run every call with dt = C+0x10. C+0x21 = variable step. C+0x22 = call 0x102340 after the step when P+0x10 == 2.
  - P+0x70 is the catch-up cap, P+0x74 the last tick consumed, P+0x78 the dt passed to Update, and *0x376b0c the current tick.
  - When no step is due, the scheduler sleeps through 0x25c174(1/30 s).
- 0x158640 builds a smoothing table with dt = 1/60 (0x1640a8). 0x1ab96c and 0x309514 convert a frame count to seconds with 1/30. Neither proves the scheduler step.
- Searched without success: the 1/30 and 1/60 literals, the app constructor 0x10087c, stores to +0x10..+0x1c with flag bytes +0x20..+0x22 (the candidates 0x156914 and 0x343e30 are unrelated). The object that sets C+0x10 is still unknown.
- Correction to the 2026-10-13T08:53 save-format entry: slot-table entries 12 and 13 (*0x376a18, *0x376a1c) hold channels 8 (L) and 10 (R), using the table order from the 10:45 entry. Invert camera therefore flips the direction of L/R camera rotation, not a stick axis. Not verified at runtime.
