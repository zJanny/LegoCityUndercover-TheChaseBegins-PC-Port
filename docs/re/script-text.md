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
