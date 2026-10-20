# Script text: the bottom screen panel, dialogue background and the screens of FadeScreen

Subtopic of [script-text.md](script-text.md): what the bottom screen shows while tips and dialogue backgrounds are active, the panel alpha that fades the dialogue background, the ShowDialogBG byte and BlackoutMap.

## Current state

- The dialogue background (0x2d9410, UI_Dialog_A / _B over the map) draws with the panel alpha 0x51a2d4 shared with the tip panel: +0.2 per update while a tip is shown or BlackoutMap is set, else -0.2; source-alpha blend. BlackoutMap also turns the map alpha +0x4a0 to fading out (0x2de3b0). [§2026-10-19T23:15:01](#ts-20261019-231501)
- The ShowDialogBG byte 0x51a2bd is read by FadeScreen (set: top screen only, clear: both) and by 0x2d9d0c (hides the object 0x5190bc), not by the background. [§2026-10-19T23:15:01](#ts-20261019-231501)
- HUD-31: the port's InformationBox picture differed only by test set-up. [§2026-10-14T21:41:51](#ts-20261014-214151)
- After the release camera of mission 1-1 the original shows the hexagon panel, then the map; the port shows the driving tip. [§2026-10-15T17:19:16](#ts-20261015-171916)

<a id="ts-20261014-214151"></a>

## 2026-10-14T21:41:51+02:00 — HUD-31: why the port's picture of the InformationBox shows no HUD and a dialogue background (port runs and the oracle picture `ib3.png`; no new disassembly, REA was not used)

Evidence: `chase-game ... --window 400x480` runs, the oracle picture (box open, not kept) of [script-text.md §2026-10-14T20:46](script-text.md#ts-20261014-204600), the port's draw and update code (`src/game_level.cpp`, `src/game_level_update.cpp`, `src/frontend/hud.cpp`). Own pictures (not kept): `--call-script No_Entry_Auburn_On_Foot` after 20 updates, no script, and walking into the barrier.

**Cause: two artefacts of the port's test set-up, not a difference in what the barrier script switches.**
- Top screen: the HUD layout is drawn from the storyboard `TransitionOn` at the HUD's own time, and that time only advances in `LevelSession::simulate` after the dialog check; while a script dialog is open the update returns before `hud->update(dt)`. `--call-script` opens the box in update 0, so the HUD stays at time 0 of its fade-in (nothing drawn) while the box is open. Without `--call-script` the HUD is drawn at update 20 (`port_p0.png`).
- Bottom screen: the start of `lvl_01` with the start progress runs the mission 1-1 start script (heading and first tip with `ShowDialogBG`); background and tip stay until the player moves on. The oracle state is past that (mission 1-1 finished, no tip), so it shows the map. The barrier script does not call `ShowDialogBG`, a HUD native or the map: with the player walked into the barrier (`--start "755.09 26.5 598" --facing 180 --script "0 100 150"`, the box opens at update 104) the port's top screen shows portrait and hearts as in `ib3.png`, and the bottom screen shows the pending mission 1-1 tip.

**Result.** The port matches the oracle picture once the set-up matches (HUD above the box, map below when no tip or background is active). Nothing was changed in the HUD or map code.

Unknowns:
- Whether the original's HUD clock (`TransitionOn`, heart and stud bar animations) keeps running while a dialog box is open; the port freezes `hud->update` during a dialog (HUD-32).
- The reader of the `ShowDialogBG` byte (HUD-S9) is still not found.

<a id="ts-20261015-171916"></a>

## 2026-10-15T17:19:16+02:00 — Bottom screen after the release director camera of mission 1-1 (observation)

- Source: the oracle RPC screenshots that agent Q took 3, 10 and 30 updates after the hand-back of the release director camera ([camera-vehicle.md §2026-10-15T16:58](camera-vehicle.md#ts-20261015-165809)), next to `chase-game` pictures of the same moments.
- Original: at +3 the bottom screen shows the hexagon panel with the police badge (the panel of MOVIE-24), at +10 the panel is in a closing animation, at +30 it shows the map. No tip box is visible in the three pictures.
- Port: from the hand-back on, the bottom screen shows the tip "Press and hold A to accelerate and B to brake and reverse." over the map.
- Only three moments were looked at; whether the original shows this tip earlier, later or not at all is not measured. The top screen also differs in the vehicle portrait and blue hearts, which is VEH-18.

Unknowns:
- Which script line or code shows the driving tip in the original, and when.
- The code and textures of the hexagon panel (MOVIE-24) and its closing animation.

<a id="ts-20261019-231501"></a>

## 2026-10-19T23:15:01+02:00 — The dialogue background draw 0x2d9410 and its panel alpha, the readers of the ShowDialogBG byte, BlackoutMap, the blend of the screen fade (llvm-objdump with `--mattr=+vfp3,+neon` on `artifacts/game/game.arm.elf`; REA with the Ghidra provider for `xrefs`; no emulator run)

[Corrects script-text.md §2026-10-14T15:25:00](script-text.md#ts-20261014-152500) (ShowDialogBG does not switch the bottom screen to the dialogue background) and [Corrects script-text.md §2026-10-14T18:57:39](script-text.md#ts-20261014-185739) (the byte has two readers; the background fade is found and does not read it).

**Readers and writers of the byte 0x51a2bd (= 0x519e24 + 0x499 = 0x51a224 + 0x99).** REA `xrefs 0x51a2bd` (Ghidra 12.1.2, evidence `ev_c291d8e4…`) gives the writer 0x1a21e0 (`strb` in 0x1a21dc, called by ShowDialogBG 0x1bfb64 with flag != 0), the reader 0x1d9588 (`ldrsb` in 0x1d9584, whose only caller is FadeScreen at 0x1a2ea4) and the writer 0x2dd3e8 (the HUD object's init, value 0). Ghidra resolves only constant bases; an llvm-objdump search for byte accesses at offset 0x99 / 0x499 finds one more reader through `this`: 0x2d9d3c in 0x2d9d0c (`add r1, r4, #1024; ldrb r2, [r1, #0x99]`), called once per update from the HUD update 0x2ddd64 at 0x2dde40 with r4 = 0x519e24. 0x2d9d0c looks at up to ten entries of the list at [this + 0x24] + 0x164 for the nearest one inside a radius and shows or hides the timed object 0x5190bc; when byte +0x499 or +0x485 is set (or byte +0x476 is 0) it hides that object (0x2459d8) instead. The other offset-0x99 accesses (0x2f033c, 0x2f0690, 0x352064) are on other objects.

**FadeScreen's screen byte.** FadeScreen 0x1a2e8c passes `0x1d9584() != 0` as r1 of the fade start, so the ShowDialogBG flag decides the screens of the screen gate 0x1087ec: flag set = byte 1 = top screen only, flag clear = byte 0 = both screens. The value 1 read at `gameplay-start` ([cutscenes.md §2026-10-19T10:04:29](cutscenes.md#ts-20261019-100429)) is the flag of the mission 1-1 tips. With the value 4 (bit 2) of the halfword [thread + 0x14] set (`ldrh` at 0x1a2e94, `tst #4` at 0x1a2e98) the start is skipped (a resumed FadeScreen only tests 0x25207c).

**The dialogue background draw 0x2d9410 (this = 0x519e24).** The init 0x2dd538..0x2dd558 loads `sprites/UI_Dialog/UI_Dialog_A.tga` and `_B.tga` (strings at 0x2dd5a4 / 0x2dd5c8) through 0x250244 into +0x47c / +0x480. The bottom-screen pass of the HUD draw 0x2dda48 (screen index [0x529e30 + 0x3c8] = 1, layer argument 3) draws the map parts (0x2dbb04, 0x2db0dc, 0x2dc284, 0x2d9658) and ends with a tail call to 0x2d9410, so the background lies over the map. 0x2d9410 returns when the float [this + 0x4b0] (0x51a2d4) is 0.0; otherwise it packs the colour white with alpha byte `(int)(alpha * 255)` (literal 255.0 at 0x2d9640, truncating `vcvt.s32` at 0x2d94a4), builds a render record on the stack (0x252254 with texture A, then bytes [5] = 2, [8] = 4, [9] = 5, [0xb] with bit 3 cleared and bit 6 set), applies it (0x25511c), draws the first quad, switches the record's texture to B (0x2d954c) and draws the second. The quad geometry (floats at 0x519d48 and 0x529e8c) was not transcribed; the port keeps its two halves.

**The panel alpha 0x51a2d4.** Its only writer is the HUD update ([script-text.md §2026-10-14T16:51:43](script-text.md#ts-20261014-165143)): +0.2 per update while the tip state byte (0x1ee790) is not 0 or the byte 0x519e24 + 0x484 is set, else -0.2, clamped to 0..1. REA `xrefs 0x51a2d4` gives only 0x1b1fe4 (the tip draw's getter 0x1b1fe0), so the tip panel and the dialogue background share this one alpha. The tip draw 0x15b518 draws when the byte 0x375718 is 0, the tip's text ([0x517ef8 + 0x30]) is not empty and `(int)(alpha * 255)` (0x15b564) is not 0; it does not test the tip state. The byte +0x484 is written only by 0x19464c (REA `xrefs 0x51a2a8`: 0x194650 and the init 0x2dd3f4), called from 0x1b23f4, the handler of **BlackoutMap** (registration at 0x1629a4): `0x19464c([args + 4] word != 0)`, return 1. So BlackoutMap 1 fades the dialogue background in over the map without a tip, BlackoutMap 0 lets it fade out. The byte has a second reader in the same update: at 0x2de3b0 `ldrb r0, [r5, #0x84]; strbne r8, [r4, #0x4a4]` (r8 = -1 from 0x2ddf70) turns the direction byte +0x4a4 of the map alpha +0x4a0 (0x51a2c4) to fading out. The map alpha moves by direction × 5.0 (literal 0x2de440, loaded at 0x2de170) × dt per update, clamped to 0..1 (0x2de3c0..0x2de3f4). Its direction is set before (0x2de1f8..0x2de238, skipped while byte +0x49b is set): -1 (and the float +0x4ac = -1.0) while the tip state is not 0 or while the counter +0x498 (set to 10 when 0x236104 returns non-zero, else counted down by 1 per update) and the byte +0x49a are both not 0; otherwise +1 (+0x4ac = 1.0). The readers of +0x4a0 are the map draws ([objective-hud.md](objective-hud.md) takes the map alpha as 1).

**The blend of the fade quad (SCRIPT-310).** The record builder 0x254350 zeroes 96 bytes (0x2594d8) and sets defaults ([8] = 1, [9] = 0, [6] = 127, colour words +0x10 / +0x14 = 0xffffffff). The fade draw 0x24bda4 sets [8] = 4, [9] = 5 and [0x13] = (int)((1 - value) * 255) (value <= 1) or (int)((value - 1) * 255), truncated by `vcvt.s32` at 0x24be58 / 0x24be70. The apply 0x25511c calls 0x1292fc, which writes the GPU blend register (header word 0x000f0101 at 0x129538: register 0x101, mask 0xf) as `eq | eq << 8 | src << 16 | dst << 20 | src << 24 | dst << 28`, with eq = table 0x376278[record [7]] and src / dst = table 0x37628c[record [8] / [9]]. The factor table is {0, 1, 2, 3, 6, 7, 4, 5, 8, 9}: record 4 = 6 (source alpha), 5 = 7 (one minus source alpha) in the GPU's numbering; record [7] = 0 gives equation 0 (add). The record byte [0x13] is put into the top byte of a colour word in 0x128dc8 (0x1290e4..0x129150). So the quad is black with alpha [0x13] / 255 blended `src * a + dst * (1 - a)`: the screen darkens linearly with |1 - value|, as measured. The dialogue background uses the same factors with its alpha byte.

**Tween and end test (SCRIPT-S10).** Read again: the value 0x252880 is position / duration (0 for duration 0), the position 0x1202f8 is (clock - start tick) × rate clamped to [0, duration], and 0x25207c returns 0 for the kind 0x1166c0, else value < 1 for a rate >= 0 and value > 0 for a rate < 0. FadeScreen's 0.5 s is the literal 0x3f000000 at 0x1a2efc. No curve: the fade is linear, as [cutscenes.md §2026-10-19T09:12:06](cutscenes.md#ts-20261019-091206) found.

**Port.** `DialogBackgroundFade` (`src/runtime/dialog_bg_fade.{hpp,cpp}`) holds the panel alpha; `ScriptTextState::update` steps it with "tip shown" (the port's tip state not 0) and `ObjectiveState::mapBlackout()` (BlackoutMap; its map alpha reader is not ported, stand-in HUD-S77); chase-game draws the background with the truncated alpha byte whenever the alpha is not 0. ShowDialogBG only sets `dialogBgFlag`, and FadeScreen fades `Top` while it is set and `All` otherwise. The tip is drawn with the same panel alpha (`ScriptTextState::stepTip`), so a tip that starts while BlackoutMap holds the alpha at 1 is drawn at 1 at once. The screen fade's opacity is the truncated alpha byte `(int)(|1 - value| * 255)` / 255 (0x24be58 / 0x24be70). Tests `dialog-bg-fade`, `screen-fade`.

Unknowns:
- Whether the tip's text pointer +0x30 is cleared when the tip ends (then a released tip is not drawn while BlackoutMap holds the alpha; the port keeps it drawn until the alpha reaches 0).
- What 0x236104 tests for the map alpha counter +0x498, and the bytes +0x49a / +0x49b.
- The timed object 0x5190bc that 0x2d9d0c shows and hides, the list at +0x164 it searches, and the bytes +0x476 / +0x485; the port has no such object.
- The GPU register written with the record byte [0x13] in 0x128dc8 (a texture-combiner colour by its position, not decoded) and how it combines with the vertex colour.
- The quad geometry of 0x2d9410 (floats at 0x519d48, 0x529e8c and the literals 0.00390625 / 256.0).
