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

## 2026-10-13T09:20:40+02:00 — Input slot table 0x3769e8 (REA Ghidra 12.1.2; read-only, partial)

- The table is 18 words (0x3769e8..0x376a2c), each a channel index into the pad device's channel array. 0x168c9c fills it (ev_70129f7a…). Memory is zero until that init runs (ev_5ed4bcca…, static image).
- Readers index it as `*(s16*)(chanArray + slot*0x14 + 0x10)`, where `chanArray = *(DAT_003769e4 + 0x14)`: field +0x10 is the s16 press edge of the 20-byte channel record. So slots are lookups for press edges, not held state.
- Slot index → channel → pad meaning (channel meaning from the 10:45 entry):

| Slot | Address | Channel | Pad input | Evidence of use |
|---|---|---|---|---|
| 0 | 0x3769e8 | 12 | D-pad Up | menu 0x17d428: Up = cursor −4 (4-wide grid) |
| 1 | 0x3769ec | 13 | D-pad Down | same: Down = cursor +4 |
| 2 | 0x3769f0 | 14 | D-pad Left | same: Left = cursor −1 |
| 3 | 0x3769f4 | 15 | D-pad Right | same: Right = cursor +1 |
| 4 | 0x3769f8 | 16 | A | any-button check 0x1e7bac (below); 22 xrefs |
| 5 | 0x3769fc | 17 | B | same; xref'd by many functions |
| 6 | 0x376a00 | 16 | A (duplicate of slot 4) | 7 xrefs (0x2003e4, 0x200534, 0x29ce14 …); how it differs from slot 4 unknown |
| 7 | 0x376a04 | 17 | B (duplicate of slot 5) | not examined |
| 8 | 0x376a08 | 18 | X | any-button check; 0x2005dc, 0x200618 |
| 9 | 0x376a0c | 19 | Y | any-button check |
| 10 | 0x376a10 | 6 | Start | any-button check |
| 11 | 0x376a14 | 7 | channel 7, no pad bit feeds it | any-button check; xrefs 0x168cf0, 0x1e7c8c, 0x1e7cf0 |
| 12 | 0x376a18 | 8 | L | menu 0x17d428: L = previous page (0x1b283c(0)) |
| 13 | 0x376a1c | 10 | R | same: R = next page (0x1b283c(1)) |
| 14–17 | 0x376a20..0x376a2c | 0, 1, 2, 3 | analog axes ch0..ch3 | filled by 0x168c9c; readers not examined |

- 0x376a34 holds a copy of the device pointer (DAT_00376a34 = DAT_003769e4); 0x376a30 is not written by 0x168c9c.
- 0x1e7bac (xref 0x1e7c14, ev_a3accba0…) is an "any button pressed" test: true when slots 4, 9, 8, 5, 11 or 10 have a press edge (A, Y, X, B, ch7, Start), when the client at DAT_00372e80 is not in state 2 and DAT_00372e6c == 0. It also returns true after a timeout (DAT_00372e80+0xc + +4 against the clock at 0x2355c0). Probably a skip-screen check; the screen is not identified.
- Menu 0x17ceac (xref 0x17d428, ev_af92b1cd…) navigates a 12-per-page grid with the D-pad (slots 0–3) and pages with L/R (slots 12, 13). It also handles touch via 0x258850.
- Names: none are stored in the binary. Slot labels above are pad buttons, not recovered gameplay actions.

Unknown:
- Why A and B appear twice (slots 4/5 and 6/7), and which callers use which pair. Ghidra's body for the function containing 0x2004b8 is non-contiguous and was not analysed.
- What slot 11 (channel 7) reads, since no pad bit maps to channel 7. It may be Select (0x04) reserved but unmapped, or a non-pad source.
- Readers of the axis slots 14–17 and the meaning of channels 2 and 3.
- Only 3 of ~19 xref sites of slot 0 were examined.


## 2026-10-13T09:22:53+02:00 — Input slots: A/B pairs, channel 7, axis readers (REA Ghidra 12.1.2; read-only, partial)

- Binary: artifacts/game/game.arm.elf (sha256 5152ed14…0881), REA provider Ghidra 12.1.2.
- **A/B pairs (slots 4/5 vs 6/7).**
  - Both pairs are constants: 0x168c9c writes 0x10 to 0x3769f8 and 0x376a00, and 0x11 to 0x3769fc and 0x376a04 (ev_70129f7a…). Xrefs show 0x168d10 as the only writer of 0x376a00/0x376a04 (ev_aa50d128…, ev_7041245a…). So neither pair is rebound at run time; both always point at A and B.
  - The pairs differ only by reader. Slots 6/7 are read by the player controller and similar gameplay code; slots 4/5 have 22 readers (ev_d67b1398…) including the any-button check 0x1e7bac.
  - Slot 6/7 readers: 0x2003e4, 0x200534, 0x200418 (all inside the player controller FUN_0026be04, body 0x200354..0x200cbf; ev_c048f16d…), 0x29ce14 and 0x29ce38 (FUN_0029cd9c, ev_8aef62e0…), 0x18802c and 0x18818c (FUN_00187f78, not decompiled), 0x2e6c24 (FUN_002e6a60, not decompiled), 0x2eebec, 0x2ef350 (not examined).
  - In FUN_0026be04, slot 6 (A) sets move flags 0x84 while held (value > 0), a bit in +0xe on press edge, and calls action 0xc on release (+0x12). Slot 7 (B) sets flag 2 while held, a bit in +0xe on press edge, and calls action 0xd on release. Slot 8 (X) sets 0x100 and calls action 200 on press. Slot 9 (Y) held sets 0x60. L/R (slots 12/13) move camera fields DAT_00589bb0 and both pressed together call 0x2313c8.
  - FUN_0029cd9c reads slot 6/7 held values into the entity fields +0x28c/+0x290 and slot 9 (Y) press edge for an action. The D-pad slots 2/3 force the +0x288 axis to -1/+1.
  - Reading: the gameplay pair (slots 6/7) is used for held/press/release actions; the menu/skip pair (slots 4/5) is used by UI code. No code evidence shows the roles are fixed; the evidence is only the reader sets. Confidence: medium. In the lab both pairs correspond to the same 3DS A and B buttons.
- **Slot 11 (0x376a14, channel 7).** 0x103dc8 (ev_171a2e9621fe…) writes channel records at offset ch*0x14: ch6 (+0x78) from Start, ch8 (+0xa0) L, ch10 (+0xc8) R, ch12..15 (+0xf0,+0x104,+0x118,+0x12c) D-pad, ch16..19 (+0x140,+0x154,+0x168,+0x17c) A,B,X,Y. Offset +0x8c (ch7) and +0xb4 (ch9) are never written there, and Select (0x04) is never tested. So no 3DS pad bit feeds channel 7 in this layer. Other writers (record playback 0x1080b0, other devices) were not checked, so channel 7 is probably always zero on a 3DS but not proven. Only readers: the any-button check 0x1e7bac (0x1e7c8c, 0x1e7cf0). Confidence: medium.
- **Axis slots 14–17 (0x376a20..0x376a2c).**
  - Readers (ev_aaa1fd56…): 0x168d18, 0x168d30 (init), 0x1d9168 (FUN_001d9094), 0x200570/0x20077c/0x2007e8/0x200ab4 (player controller), 0x2289cc (FUN_002289b8), 0x2d79cc (FUN_002d7fd0, not decompiled).
  - They read the **f32 value at +0 (and the deadzone at +8)**, not +0x10. The player controller treats the stick as active when |value| > deadzone for slot 14 or 15, converts (x, y) with atan2 (scale 10430.378 = 32768/π) to a heading, and applies a power curve (0x25b578) except for entity types 0x14b and 0x189. Slot 14 value also drives DAT_00589bac for entity types 1–5 and 0x11c (dead zone ±0.25, gain 1.3333334).
  - **New:** FUN_002289b8 (ev_18417439…) synthesizes D-pad channels from the left stick: if the larger of |slot14| and |slot15| exceeds 0x3f19999a (about 0.6), it picks Right/Left (slots 3/2) when |x| > |y| and Up/Down (slots 0/1) otherwise (positive y gives slot 0 = Up), sets that channel value to 1.0, press edge +0x10 = 1 when the direction changed, release +0x12 = 0; when the stick returns it sets release +0x12 = 1 for the last direction. State lives in 0x376a30 (sentinel 0x2b = none).
  - **Correction to the 09:20:40 entry:** 0x376a30 is not unused; it holds the last synthesized stick direction written by FUN_002289b8. Slots 14–17 readers are no longer unexamined (slots 14/15 = left stick x/y; slots 16/17 = channels 2/3 had no reader in this pass).
  - Effect on the lab: the game's menu navigation sees a stick push past about 0.6 of the stick range as a D-pad press, with edge and release. The lab does not need to model this unless it wants to replicate the menu.
- Unknown:
  - Whether channel 7 is written elsewhere (record playback, other devices) and what the any-button check expects from it.
  - Callers that choose slots 4/5 versus 6/7 beyond the reader sets above; FUN_00187f78, FUN_002e6a60, FUN_002d7fd0 and the sites 0x2eebec/0x2ef350 were not decompiled.
  - Who reads slots 16/17 (channels 2/3, right stick / C-stick equivalents) and what produces channels 2/3 on a 3DS without a second stick (Circle Pad Pro is possible but unverified).
  - Callers of FUN_002289b8 (where in the frame it runs) were not traced.

## 2026-10-13T09:36:11+02:00 — GSP interrupt thread and vblank wait (REA Ghidra 12.1.2, llvm-objdump; read-only, partial)

- Svc stubs (objdump): 0x2594b4 = WaitSynchronizationN (svc 0x25; no svc 0x24 stub exists), 0x25947c = SendSyncRequest (svc 0x32), 0x259464 = CreateEvent (svc 0x17), 0x256c18 = ClearEvent (svc 0x19), 0x24e9d0 = GetSystemTick (svc 0x28), 0x105b20 = SleepThread (svc 0xa).
- GSP RegisterInterruptRelayQueue: header 0x130042 is stored by 0x1302f0 (literal at 0x13033c; only caller 0x12c224, inside GSP init 0x12c15c, which is called by 0x122090; ev_0b551343…). The event handle is created by 0x259464 (type 0) and stored at G+0x2c, where G = *0x386404 = 0x371c40. The same init creates the interrupt thread (CreateThread entry 0x1300c8, literal at 0x12c324).
- Interrupt thread 0x1300c8 (ev_30066315…): loops WaitSynchronizationN on the event G+0x2c with an infinite timeout, then ClearEvent. It pops the shared-memory interrupt queue (pointer G+0x34; queue byte 0 = read index, byte 1 = count, ids at +0xc, wrap modulo 0x34) and calls the callback in table G+0x10+id*4 (7 entries, zeroed in init, so ids 0..6). After each callback it sets G+0x76 = 2 and signals the object at G+0x64.
- Vblank wait: 0x12bf44 (reached through the thunk 0x24e06c; ev_8170e6aa…) is a one-interrupt wait. It sets G+0x76 = 1 and waits on the object at G+0x64 (0x251d88); if G+0x76 is already 2 it resets it to 0 and returns.
  - 0x257a68(mode) (ev_4c407a80…) latches two counters, 0x593d14 and 0x593d18, then repeats the one-interrupt wait until the selected counter changes. Mode 0x400: counter 0x593d14 changes. Mode 0x401: counter 0x593d18 changes. Mode 0x402: both change.
  - Callers: 0x10f370 (ev_371a0248…, a present/swap-end routine that selects the mode from the byte at 0x52a1f4: 1 gives 0x401, 2 gives 0x402, else 0x400), 0x105d8c and 0x25a6f0 (not examined).
  - 0x1132ac also calls the one-interrupt wait directly (inside FUN_00112630, not examined).
- Reading (inference): the game waits for a GSP interrupt counter once per present, which is a vsync wait. Frame pacing then depends on how often the counters at 0x593d14/0x593d18 advance, and that is not established.

Unknowns:
- Which callbacks fill the table G+0x10..0x28, and which ids increment 0x593d14 and 0x593d18 (PDC0/PDC1 ids 2 and 3 are not confirmed). Writers of 0x593d14/0x593d18 were not searched.
- Whether a counter advances on every vblank or only every second (a swap-interval multiple). Without this the answer 30 versus 60 fps is unknown.
- Whether 0x10f370 runs once per scheduler step (0x100f44) or per rendered frame, and the value of 0x52a1f4.
- The step value in the scheduler (C+0x10) is still unknown. The idle path 0x25c174 is not a sleep: it polls input (0x25cb2c, 0x25b4b4) and ends in 0x25afcc, so the earlier wording "sleeps through 0x25c174(1/30)" is unverified.

## 2026-10-13T09:38:42+02:00 — Vblank counters, wait modes and mode byte 0x52a1f4 (REA Ghidra 12.1.2, llvm-objdump; read-only, partial)

- Counter writers (ev_2916a84c…, ev_a2aee7af…): 0x11f4c8 increments 0x593d14 and 0x11f50c increments 0x593d18, each by 1 modulo 0x7fffffff. Each first calls an optional user hook (0x593d1c / 0x593d20) with 0x400 / 0x401. The counter block base is 0x593bc0 (counters at +0x154 / +0x158, hooks at +0x15c / +0x160).
- Registration: FUN_00112630 (GPU init, called from 0x105d8c; ev_868d354a…) calls 0x24e07c(callback, id) for ids 0..6 (0x1128a4 onward). 0x24e07c → 0x1222e8 stores the callback at G+0x10+id*4 (objdump of 0x1222e8..0x122320; id must be below 7). Id 2 gets 0x11f4c8 and id 3 gets 0x11f50c (literals at 0x112ffc / 0x113000). So 0x593d14 is the id 2 interrupt and 0x593d18 the id 3 interrupt. Mapping ids 2/3 to VBlank0 (top) / VBlank1 (bottom) is the usual 3DS GSP numbering (outside knowledge, not shown by this binary).
- Each GSP interrupt of id 2 or 3 advances exactly one counter by 1, so a counter advance is one vertical blank of one screen. No divider or skip logic appears in the two callbacks.
- 0x257a68(mode) (objdump 0x257a68..0x257ae8): latches both counters, then loops the one-interrupt wait (0x24e06c) until the condition holds. Mode 0x400: leave when 0x593d14 differs. Mode 0x401: leave when 0x593d18 differs. Mode 0x402: leave only after 0x593d14 has changed and then 0x593d18 has also changed. One call therefore waits for at least one interrupt and at most the next change of each counter. It never waits for two changes of the same counter.
- Mode byte 0x52a1f4 = 0x529e30+0x3c4 (literal 0x10f52c in 0x10f370). 0x10f370 maps 1 to 0x401, 2 to 0x402, anything else to 0x400. The only byte store found is 0x1030d0 (strb r1,[r2,#0x3c4], in the init path just before the call to 0x105d8c; r1 is set to 0 at 0x1030ac but several instructions between are undecoded by objdump, so the stored value 0 is likely, not proven). Readers: 0x1013fc, 0x101f5c (compare with 2, together with the word at +0x3c8) and 0x108804 (compare with 1). No other writer was found by literal-pool search (the base 0x529e30 has about 200 users, only the +0x3c4 offset was grepped). Value at run time: unknown.
- 0x10f370 has one caller, 0x109c98, which has one caller, 0x25acc0 (in the 0x25afcc area from the earlier entry). It starts with 0x251e4c(1) checks and skips the swap when its first flag is set, so not every call necessarily reaches the wait.
- 0x105d8c (init, ev_743b52ce…) ends with FUN_0010fa54(0x1e), which is a tail call 0x1193fc → FUN_00122908(1, 0x1e) between lock/unlock calls 0x25b678 / 0x25b5a0. Its meaning is not examined (0x1e is also 30).
- Reading (inference, not proven): in the default mode the present routine waits for one vblank of one screen (counter 0x593d14), so the present itself is paced to the vblank rate, not to every second vblank. Whether the game renders and presents every vblank or every second one depends on how often 0x25acc0 reaches 0x10f370 per vblank and on the scheduler step, neither of which was resolved.

Corrections to the earlier entry (09:36:11): the unknown "which ids increment 0x593d14 / 0x593d18" is resolved above (ids 2 and 3). The statement "Mode 0x402: both change" is refined to a sequential wait (0x593d14 first, then 0x593d18).

Unknowns:
- 30 or 60 fps: not decided. The frame count per present and the scheduler step C+0x10 writer were not examined (budget). Next step: decompile 0x25acc0 and its loop (0x25afcc), find what stores C+0x10, and test whether a frame skip or swap-interval counter gates 0x109c98.
- Whether FUN_00122908(1, 0x1e) sets a frame rate or swap interval.
- Value of 0x52a1f4 at run time and any writer other than 0x1030d0.

## 2026-10-13T09:41:32+02:00 — Present function 0x25a6f0, minimum frame interval 0x529de8, command 0x4f (REA Ghidra 12.1.2, llvm-objdump; read-only, partial)

- 0x25acc0 is not a function entry. It lies inside FUN_0025a6f0 (REA name; decompile ev_9f455fe2…), the display-list flush and buffer-swap routine. The call to 0x109c98 (which reaches 0x10f370) is its tail, after the swap, and runs only when byte 0x3760f0 is 0 and not (0x52a1f8 == 1 and 0x52a1f4 == 2). Correction: the earlier entry listed 0x25a6f0 as a separate caller of the vblank wait (not examined); it is the same routine.
- Waits per call (ev_9f455fe2…, objdump 0x25ab60..0x25abc8): the routine returns at once when byte 0x52a200 is 0. Otherwise it submits the frame and, if `0x52a1f8 == 1 || 0x52a1f4 != 2`, calls the mode wait 0x257a68(mode) once (mode from 0x52a1f4 as before) and then repeats it in a loop: `while (timer(0x529e30) < word(0x529de8) + last) wait(mode)`, where last = word 0x376120, refreshed after the loop. So one call waits for at least one counter change and then until the timer has advanced by word(0x529de8) since the previous present. This is a minimum frame interval, not an accumulator; no code path skips or repeats a game-update step here.
- 3D-style double call: when 0x52a1f4 == 2 the wait block runs only when 0x52a1f8 == 1, and 0x52a1f8 is toggled (xor 1) at the end of the call. So in that mode two calls of this routine make one swap and one wait (0x52a1f8 selects the eye/half). In mode 0 or 1 every call waits. Meaning of 0x52a1f8 is inferred from this use only.
- Interval word 0x529de8 = 0x529dc8 + 0x20. The 0x529dc8 block is zeroed at init (0x103034, size 0x6a8), so the value is 0 until set. Eleven stores of the constants 1 or 2 exist: value 2 at 0x2d5364, 0x2d84e0, 0x2d8978, 0x2f2170, 0x30003c and value 1 at 0x2d5a04, 0x2d874c, 0x2d8c2c, 0x2f2764, 0x300774, 0x313080 (objdump, each `str r0,[r1,#0x20]` with r1 from a literal pool word 0x529dc8 or 0x37472c / 0x375f90; the last two bases were not checked, so they may be a different structure). The function that holds each store was not decompiled, so what selects 1 versus 2 (a game state, 60 versus 30 fps cap) is unknown.
- Timer 0x529e30 (= 0x529dc8 + 0x68): set up at 0x1065d0 by FUN_0025a1a0(timer, 60, 1) (ev_642e8df4…, objdump 0x1065d0..0x1065dc). Type 1 = manual source: the raw value is the word at timer+8, supplied by some other code, and the scale is 60 numerator over 60 denominator. The getter FUN_00256af4(timer, 1) (ev_5678829b…, reached through the tail thunk 0x25cfc8) returns the scaled elapsed count. The type 0 source (GetSystemTick) and type 2 (another timer) also exist. The unit is nominally 60 per second, which suggests the interval 1 or 2 means 60 or 30 presents per second, but the writer of timer+8 was not found, so the unit is not proven.
- FUN_00122908(1, 0x1e) (ev_5862e813…): not a frame rate, swap interval or thread priority. It writes an IPC command header 0x004f0080 plus two parameters (1, 0x1e) to the thread-local buffer (TLS+0x80..0x88) and calls SendSyncRequest (0x25947c) on the session handle at 0x386578. Header 0x4f0080 means command 0x4f with 2 normal parameters. Outside knowledge, not shown by the binary: APT command 0x4f is SetApplicationCpuTimeLimit, so 0x1e would be a CPU time percentage (30). The service behind handle 0x386578 was not identified.
- Scheduler step C+0x10: not found. No store to a timer or counter that feeds timer+8 was located (a literal-pool scan of the 0x529e30 users found no `str [reg,#8]` within 14 instructions of the load).

Reading (inference): the present is gated to the vblank rate with an extra minimum interval of 1 or 2 timer units, so the game can run at 60 or at a 30 cap depending on state; no single fixed rate is established.

Unknowns:
- What writes word 0x529de8 = 1 versus 2 (functions around 0x2d5364..0x313080) and the value during normal gameplay.
- What writes timer+8 (0x529e38) and its unit.
- Whether 0x52a1f4 is 2 (3D double-call mode) at run time, and the meaning of 0x52a1f8.
- Writer of the scheduler step C+0x10 and the number of game-update steps per present.
- Which service handle 0x386578 is.

## 2026-10-13T10:35:31+02:00 — Runtime oracle (Azahar 2126.2 gdb stub) and scheduler 0x100f44 semantics

First runtime evidence. The game runs in Azahar 2126.2 (`azahar -g PORT`, Qt offscreen, isolated XDG home); `tools/azahar_oracle.py` drives it through the GDB remote protocol. The stub halts at entry 0x100000 until a client attaches.

**Scheduler 0x100f44(P) — confirmed** (objdump 0x100f44..0x10125c, live values below). Correction to the 08:55 entry: C+0x10/+0x14 are the minimum and maximum dt, not "step" and "minimum step", and the scheduler is a clamped variable step, not a fixed step.

- G = 0x376af8: G+0x10 = current P during the call, G+0x14 = "now" (the clock read by the scheduler; *0x376b0c in earlier entries).
- Timer object T = P+0x18 (type at +4, raw ticks +8, frequency +0xc read by 0x25c1f8; 0x25c1e8 adds to +8).
- minT = C+0x10 × freq, maxT = C+0x14 × freq, fixedT = C+0x18 × freq (float → int).
- If C+0x20 is set: loop {P+0x78 = C+0x10; Update(C+0x10); T += minT; P+0x74 = now; …} until P+0x12 is set. Flag observed 0.
- Otherwise, return if P+0x74 (last) ≥ now. n = ceil((now − last) / maxT) (0x25c1b8 unsigned divide).
  - n > cap (P+0x70): n = cap, per = C+0x21 ? fixedT : maxT.
  - else per = (now − last) / n, then per = C+0x21 ? fixedT : max(per, minT).
- ticks = (int)(C+0x1c × per), at least 1; dt = ticks / freq is stored at P+0x78.
- n == 0 (only possible with cap 0): 0x25c174(1, 1/30) (literal 0x101258 = 0x3d088889).
- For each of n steps: C.vt+0x24(1); first step only 0x102f34(0x5305c4, 2); C.vt+0x1c Update(dt); first step only 0x102f38(0x5305c4, 2); C.vt+0x24(0); T.raw += ticks; last += per; if P+0x10 == 2 and C+0x22: 0x102340(); stop if P+0x12.
- After the loop, when the cap was reached: if cap ≥ 5 or last < now, last = now (backlog dropped).

**Live values** (oracle, hardware renderer, frame limit 100, screen not captured; this first run was accidentally in New 3DS mode because Azahar ignores a setting whose `\default` flag is true):
- P = 0x585f18, C = 0x514378 (vtable 0x363cd0). C+0x10 = 0.03, C+0x14 = 0.07, C+0x18 = 0, C+0x1c = 1.0, flags +0x20..+0x22 = 0, P+0x70 cap = 1, P+0x10 = 2.
- T frequency 0xbb8 = 3000. Clock G+0x14 advanced 18050 over 360 vblanks (50 per vblank), so the clock unit is 1/3000 s.
- With cap 1 and backlog drop, every update gets dt = clamp(elapsed, 0.03, 0.07) s and the game never catches up with extra steps.
- 120 scheduler calls took 360 vblanks of each counter (3 per call, 20 updates/s), dt 0.0503. Which screen this was is unknown (about 110 s after boot, no input).
- Pacing globals: 0x52a1f4 = 2 (correction to the 09:38 entry: the run-time value is 2, not the "likely 0" from the init store), 0x529de8 = 1, 0x52a200 = 0 at the scheduler breakpoint.
- Timer 0x529e30 = {0, type 1, raw 0xb70, 60, …}: raw 2928 when the vblank counters were 2930. So timer+8 counts vblanks, and the unit of the minimum frame interval 0x529de8 is one vblank (1/60 s). The writer of timer+8 is still not located statically.

**Oracle mechanics (confirmed by use):**
- GSP shared memory is at 0x10000000 (G_gsp+0x34). Framebuffer info: top +0x200, bottom +0x240 (byte 0 = active entry, entries of 0x1c bytes from +4: active, left address, right address, stride, format, …). Observed: RGB8, stride 0x2d0, top 0x1f300000/0x1f346500, bottom 0x1f000000/0x1f038400 (VRAM virtual addresses).
- Input injection: the held mask is in r0 at 0x10a57c (just loaded from the pad state, before `bic #0x2000`), and the raw circle-pad s16 pair at r4+12/+14 is read at 0x10a540.
- With the hardware renderer, VRAM holds stale data (frames stay on the host GPU); the software renderer writes real frames. At frame 600 after boot the bottom screen shows the language selection list.
- With `frame_limit=0` (unthrottled), the game clock ran about 2000 per vblank instead of 50, so emulated timing is not faithful; oracle runs use the 100 % limit.
- Old 3DS mode (the oracle default), language screen, frames 600→720: 16 vblanks per update and 66.5 clock ticks per vblank, the same with the software and the hardware renderer. New 3DS mode gave 3 vblanks per update and 50.1 ticks per vblank (the nominal 3000 / 59.83). Reading (unverified): the vblank counters count serviced GSP interrupts, which fall behind the system-tick clock when the CPU is saturated. Correction (same day, free-run test): the per-frame breakpoint stops do distort it. Language screen, Old 3DS: a 20 s free run (`wait`, breakpoints removed) advanced the clock 62076 ticks over 1210 vblanks (51.3 per vblank), while 60 stepped frames right after advanced 63227 ticks over 960 vblanks (65.9). So rates and fps must be measured in free-run intervals; per-frame traces still record the dt the game itself used.
- Pad channels (oracle `channels`, language screen): 42 records, dead zone 0.1 and threshold 0.75 on every channel; types ch0/ch2 = 1, ch1/ch3 = 2, ch4–23 = 0, ch24–41 = 8..25. With raw stick (100, 50) injected, ch0/ch1 read 0.5779/0.2889, which the C++ replica (src/runtime/pad.cpp: circle clamp 0x119724, axis 0x10fcb0, radial dead zone in 0x25cb2c via 0x1035d0) reproduces.

Unknowns:
- Whether C+0x10/+0x14 and the cap change in other states (gameplay, cutscenes); only one state was measured.
- Who writes C+0x10..+0x1c (still not found statically).
- Which screen produced 3 vblanks per update, and the update rate during gameplay.
