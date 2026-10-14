# Script text: tip text, dialogue lines, dialogue background, mission title

Handlers of batch SCRIPT-30 and the objects they feed. Evidence: llvm-objdump (armv6k, vfp2) on `artifacts/game/game.arm.elf`, memory polled over RPC in Azahar (state `agent-doughnut-title`, Old 3DS) and oracle screenshots. No REA session was used.

<a id="ts-20261014-152500"></a>

## 2026-10-14T15:25:00+02:00 — Tip text and dialogue objects, handler arguments, measured timing and layout

**Argument arrays.** `handler(r0 = thread, r1 = array)`; a text or number argument is read at `[r1 + 4]`, `[r1 + 0xc]` for the second; an object argument is the slot at `[r1 + 0x10]` and the argument after it is read at `[r1 + 0x1c]` (ShowDialogueTextTimed: key +4, id +0xc, object +0x10, seconds +0x1c; the object slot pair is the gap described in script-natives.md). Text arguments are 32-bit loc keys (sdbm hashes) written as decimal numbers in the level file, for example `4094106649`; they do not fit a float. Tip and dialogue texts are in `localisation/hinttext_<lang>.loc` (all 5 keys of 1-1 checked; `ShowDialogueTextTimed` of the intro too).

| Native | Handler | Behaviour |
|---|---|---|
| ShowTipTextOnce(key) | 0x263850 | Returns at once when the byte 0x37751c is 1. Else `0x235fb4(table [0x37596c], key)` resolves the string and `0x235f30(string, colour 0xffffad4a, 0, 1, seconds 5.0)` starts the tip object. The 5.0 is the literal at 0x2638e0. Colour bytes 74, 173, 255, 255. |
| ShowTipTextTimed(key, seconds) | 0x26ffec | Same guard, same colour and `0x235f30`, with the script's seconds. |
| ShowDialogueText(key, id, object) | 0x26ff84 | `0x17cc58(object)` stores the speaking object at 0x517ec8+0x54 (the player when it is the player's reserved entity); `0x1ffb08(uxth(id), key, 0, 4.0)` starts the dialogue object (literal 4.0 at 0x26ffe4); `0x1ffaac(player row == id)` is called afterwards. |
| ShowDialogueTextTimed(key, id, object, seconds) | 0x2a6d90 | The same with the script's seconds. |
| ShowDialogBG(flag) | 0x1bfb64 | `0x1a21dc(flag != 0)`. The bottom screen shows the dialog background instead of the map while it is on (oracle). |
| SetMissionTitle(key) / SetMissionSummary(key) | 0x263788 / 0x27be98 | Store the key at 0x375f98 / 0x375f9c. No reader found by literal pool search (users of 0x375f98 / 0x375f9c: only the setters). |
| ShowIntroText(key), ShowLevelText() | 0x1cdc2c, 0x1cdc54 | Look a text up through 0x235fb4 and call 0x20a4a8 (level text: row `[0x5163e8 + 0x26]` of the 84-byte table behind 0x375914, field +8). Not followed further. |
| ShowCharacterIntro(id), ShowTally, ShowBossHearts, ShowRaceStart, InformationBox, ShowPortraitOverVehicle, showinfopointer | 0x2c2af8, 0x194f78, 0x1da788, 0x1cdc9c, 0x1d9d4c, 0x2b4c1c, 0x263848 | Not read beyond the first instructions (showinfopointer 0x263848 is `mov r0,#1; bx lr`). |

**Timed object (tip at 0x517ef8, dialogue at 0x5189cc, class with the fade state machine 0x23cc00).** Layout read over RPC: +0x10 state byte (0 idle, 1 in, 2 hold, 3 out), +0x14 fade in 0.25 (float), +0x18 fade out 0.25, +0x1c duration (5.0 for the tips of 1-1, 4.0 for the dialogue), +0x20 draw callback (tip: 0x15b518), +0x28 start time (clock seconds), +0x2c flags, +0x30 text pointer, +0x34 colour 0xffffad4a (tip), +0x38 1. The dialogue object keeps the portrait path at +0x2c as `sprites/portraits/<name>.png` (the files are `.bpng`; `<name>` is column 1 of `tablescharacters.bxls` sheet 0 for the character id, none for ids 0 and 147; code 0x1b1314 builds it with the format at 0x1b1460). `0x235f30` treats a duration of exactly 0 as 4.0.

**Measured (poll of the tip object at about 2800 reads per second, mission 1-1 after the intro, script `1-1_OnScreenText_Script`).** State 1 starts at the first call; state 2 follows after 0.27 s wall time; each later call restarts the time (+0x28 grows by 3.0, 3.0, 4.0 s) and replaces the text without a state change; the last text runs 5.0 s, then state 3 and the object returns to 0 after 0.27 s. The draw alpha (float at 0x51a224 + 0xb0, read by 0x1b1fe0 and used by 0x15b518 as `alpha * 255`) steps 0.2, 0.4, 0.6, 0.8, 1.0 on consecutive game updates in and 0.8 .. 0 out, five updates each.

**What the player sees (screenshots, 400x480 composite).** Tip: bottom screen, the `ui_dialog_a/_b` background with a faint hexagon pattern (ShowDialogBG), a box (`ui_dialog_box`, 9-slice) at x 21..300, y 72..167 with the `ui_dialog_infoicon` at its top right (centre 271, 73, 28 px) and `ui_dialog_badge` at its bottom right (centre 271, 168), text in the blue 74, 173, 255, centred at x 160, y about 123, 17 px line pitch, button tokens (`<east>`, `<l>`, `<r>`, `<stick>`) drawn as the button icons (16 px). The heading "Cherry Tree Hills" (a level text box above the panel, fading in with the first tip and out before the third) is not drawn by the port. Dialogue: top screen, box x 44..375, y 155..217 (same 9-slice), orange text (255, 169, 22), two centred lines at y 180 and 197, the speaker portrait at the left (the cutscene frame shows it inside a tan hexagon frame of unknown source).

Unknowns:
- The meaning of byte 0x37751c (skip guard of both tip natives; value 0 in the oracle runs).
- Drawing code of the panel (0x15f6cc, 0x166504, 0x1667b4, 0x187b8c, 0x1ee790 reference the tip object) and of the dialogue box (0x1cc550 callback not read): positions above are fitted.
- Why the alpha steps five times while +0x14 says 0.25 s; the port uses five updates.
- The tan hexagon frame of the dialogue portrait; the heading box; readers of the mission title and summary; the other natives of the table.
- How a dialogue line reacts when the speaker object is not in view (0x17cc58 only stores the object).

<a id="ts-20261014-155429"></a>

## 2026-10-14T15:54:29+02:00 — Draw code of the tip panel and the dialogue box: layout from code, fade of the dialogue

Evidence: llvm-objdump (armv6k, vfp2) on `artifacts/game/game.arm.elf`, the `.btga` textures through the C++ decoder, and the oracle comparison pictures of [§2026-10-14T15:25](#ts-20261014-152500). No REA session, no new emulator run. Neither panel is a `.bxaml` layout: no layout file names a `ui_dialog` texture; the textures come from the preload table at 0x3584e8 (nine names, handles stored from 0x518418: index 6 = `ui_dialog_box` at +0x18, 7 = `ui_dialog_badge` at +0x1c, 8 = `ui_dialog_infoicon` at +0x20).

**Generic text-in-a-box function 0x23c780** (text, flag top screen, colour, wrap flag, alpha s0, pad x s1, pad y s2, x offset s3), called by both callbacks.
- Measure 0x23c680 returns a centre and a half size. Centre x is the screen centre plus the x offset (top 200 from the field 0x529e30+0x44, bottom 160 from +0x5c; the field values are taken from the oracle screen sizes, not read). Centre y is 120 on the bottom screen; on the top screen it is 180 when the wrap flag is set, else 206 minus the half height. The half size is half of the laid-out text (width = widest line, height = sum of the line heights); half height at least 20; half width at least 120 when the wrap flag is 0.
- Wrap widths: the initializer at 0x3454d8 stores 280.0 at 0x3758a8 (top) and 300.0 at 0x3758ac (bottom); 0x1a0ff8 selects them.
- Box 0x229128(texture, centre, half size + pad, alpha): centre and half size are rounded and made even, then the 9-slice has 16 px edges: x lines at centre -half-16, -half, +half, +half+16 (same for y), 9 patches of the 64x64 texture (corners 16 px; the border is opaque from pixel 3).
- The tip passes pad x 0, pad y 16, offset 0, wrap flag 0 (0x15b5f8..0x15b62c); the dialogue passes pad x 24, pad y 0, offset 10, colour 255, 169, 22 (0x1cc6e4..0x1cc740). The wrap flag byte of the dialogue (0x518bcc+0xb1) was assumed 0: it reproduces the measured centre y 186.

**Tip callback 0x15b518.** After the box and text it draws two sprites at their natural size, with the half size (hx, hy) of the text: badge top-left (cx + hx - 52, cy + hy) (literal -52 at 0x15b7bc), info icon top-left (cx + hx - 36, cy - hy - 16 - 27) (literals -36, -27, 16.0 at 0x15b7c4, 0x15b7c8, 0x15b7b4). Natural sizes: badge 64x64 (opaque 12..51, 8..53), icon 32x32. The earlier port drew the badge at 62 px width, which is about half the glyph size of the original. Check: for "Press <L> or <R> to rotate the camera." the box border runs over columns 20..299 and rows 72..167 of the bottom screen in the oracle picture and in the port; mean abs difference of the panel region 0.50 against 6.09 before.

**Dialogue callback 0x1cc550, portrait.** Draws `[0x5189cc+0xac]` (the portrait) through 0x21d27c, which treats the position as the sprite centre when its flag is 1, at (centre x - half width - 24 - 7, centre y - 1) (literals -7.0, -1.0 at 0x1cc898, 0x1cc89c); before it, when the byte 0x518c7c is set and `[0x51920c+0x264]` is non-null, that object at the same place (probably the hexagon frame, not identified). With the measured half width this gives a centre x of about 50, but the oracle portrait and hexagon centre is at x 36: not resolved, the port keeps the fitted position.

**Fade.** Dialogue (0x1cc5b8..0x1cc6b4): the alpha float at `[0x5189cc+0x2b4]` rises by dt (0x23602c) * 5.0 (literal 0x1cc878) per update, clamped to 1.0; in state 3 it is replaced by 0x235e2c, which divides the remaining time by the fade-out time +0x18 (0.25 s): linear fade-out. States 1 and 2 of 0x235e2c use +0x14 (0.25 s) and 1.0. So the object's 0.25 s is the fade-out; the fade-in is a rate of 5 per second (0.2 s). At dt = 1/30 s that is 0.167 per update (six updates), not the 0.2 per update measured on the tip; the dt of the measuring run was not checked. The tip's draw alpha is read from the float at 0x51a2d4 by 0x1b1fe0; its writer was not found (literal pool users of 0x51a224: 0x1b1fec, 0x1be3ec, 0x1d9590, 0x26320c, 0x26321c, 0x26e9d4, 0x290fe4; the ones read stay on other bytes). The tip keeps the measured 0.2 per update.

**Port.** `src/frontend/script_text_pages.cpp` computes both panels from the above (wrap 300 and 280, centres, half sizes, even rounding, padding, icon places). Fitted stand-ins left: line pitch 17, glyph centre shifts (tip +3.5, dialogue +2.5), the 16 px button icon width, the portrait position. `src/runtime/script_text.*` fades the dialogue by the code above. Tests: `script-text-pages-real-data` (box extents 20..299 x 72..167 and 44..375 x 154..217 against the oracle pictures), `script-text` (dialogue fade).

Unknowns:
- The tip's alpha writer (0x51a2d4) and the dt of the measuring run (0.2 per update against 5.0 * dt).
- The portrait position formula against the measured centre (36, 186), the object `[0x51920c+0x264]` and the byte 0x518c7c.
- The level heading box, ShowCharacterIntro, ShowTally, ShowIntroText: not read (HUD-13).
- The text origin inside the box (the glyph centre shifts 3.5 and 2.5 are fitted), the font's line height and the width of inline button icons (0x21bc2c).
