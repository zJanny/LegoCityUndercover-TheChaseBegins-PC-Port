# QoL: settings menu drawing

How the settings menu is drawn: which original art and font it reuses, how it scales into the window, its text and the navigation onto the menu model's events. Subtopic of [qol-settings.md](qol-settings.md); feature file: [qol-settings-menu.md](../plans/features/qol-settings-menu.md).

<a id="ts-20261029-121155"></a>

## 2026-10-29T12:11:55+01:00 — Settings menu drawing

The user's decisions of 2026-10-29, asked by the lead; the quoted labels are the user's selections. They settle QOLSET-22. The questions came from a spec round (20261029-qolview-D, docs only, no code, no emulator) that the user had chosen for the menu drawing: "Yes, next round". Findings behind it: the Options page [ui-screens-pause.md §2026-10-28T22:51:42](../re/ui-screens-pause.md#ts-20261028-225142), the layout pieces [ui-layouts.md §2026-10-29T11:50:58](../re/ui-layouts.md#ts-20261029-115058), the font [text-localisation-fonts.md §2026-10-29T11:50:26](../re/text-localisation-fonts.md#ts-20261029-115026).

**Basis** (QOLSET-22): "Options-page art": the settings menu is drawn with the original Options-page art and font exactly as the 3DS draws them, scaled up.

**The Options page as the 3DS draws it:** a bottom-screen page (320x240) of the round menu layout `sprites/ui_roundaboutmenu/main`: the background, the top bar with the title, the subtitle bar ("Options"), the round of five hexagon pictures with the selected one in the centre, the name plate with "Label: value" ("Music: 5", "Invert Camera: Yes"), and the Back (B) and Select (A) shortcuts in the bottom corners. Left and Right turn the round, Up and Down change a value, A toggles Invert Camera or opens the Languages page, B leaves.

**Decisions** (question numbers of the spec round):
- Q1 placement and scale: "Whole window, free (Rec.)": the menu page covers the whole window at the scale min(width / 320, height / 240), not rounded to an integer, drawn at window resolution with linear filtering; the rest of the window is black.
- Q2 tab headers: "Subtitle bar + arrows (Rec.)": the subtitle bar shows the current tab name between the `Pageslots` arrows, and `PageNumber` shows the tab's place ("1/4"). On the Controls tab too (asked 2026-10-29): "Tab place \"3/4\"": the tab's place like on the other tabs, not the list position.
- Q3 row form: "Round + list (Rec.)": the round for Game, Display and Quality of life; the list bars for Controls, whose 14 actions with two slots each do not fit one name plate.
- Q4 pictures of rows without their own: "Options picture (Rec.)": `UI_StatusScreen_Options`.
- Q5 toggle words: "On/Off": toggle rows show "On" or "Off" (gametext `ON`, `OFF`). This was not the recommended option ("Yes" / "No", the page's own words). The mirrored Invert camera row too (asked 2026-10-29): "On/Off", the same as the other toggle rows.
- Q6 "waiting for key" and "key taken": "Alert notice (Rec.)": the notice form of `page_alerttext`.
- Q7 new texts without an original translation: "English everywhere (Rec.)": English in every language.
- Q8 changing a value: "Left/Right (Rec.)", changed 2026-10-30 to Up and Down (see [qol-settings.md §2026-10-30T17:03:17](qol-settings.md#ts-20261030-170317); Left and Right now turn from row to row): Up and Down step value rows. The rows, asked next: "Display rows, Master row, Toggle rows" (all three selected), so Up and Down step the volume rows, the Display rows, the master row and the feature toggle rows. The direction on the cycling rows (the master states, the window mode, the resolution list): "Left back, Right fwd, stop", now Up backwards and Down forwards: Up steps backwards and Down forwards; they stop at the first and the last value, with no wrap. The other Game rows: "Invert only": Invert camera flips like a toggle row; the Language row (read-only) ignores them; on the Controls rows Up and Down pick the slot (slot 1 or 2), answered 2026-10-29 and changed 2026-10-30 in [qol-settings.md §2026-10-29T11:06:51](qol-settings.md#ts-20261029-110651).
- Mouse on a volume row (QOLSET-60), answered 2026-10-30, "Click left/right half (Recommended)": a click on the left half of the value lowers it one step, on the right half raises it, no new art; the value is the round menu's name plate `Item_Name` (the user's answer of 2026-10-30T20:13:28+01:00, round 20261030-qolpanels2: "Yes, the name plate": left half lowers, right half raises) (not chosen: "Drag the bar", "Mouse wheel", "Agent proposes mock-up").
- Q9 mouse wheel: "Moves rows (Rec.)": the wheel moves rows like the row-turning keys (Left and Right since 2026-10-30).
- Q10 text size, the user's words: "Wait, and make HUD-75 the Next a Action": no menu drawing before the port's HUD-75 (layout text font size) is fixed; HUD-75 is to be the next action of the `linux-port` plan (recorded there by the lead).
- Q11 picture of the "PC Settings" pause entry: "Options picture (Rec.)": `UI_StatusScreen_Options`, the wrench of Options, so the round shows it twice.

**1. Parts and the art they reuse.** "Exists" means an original piece does this job; "composed" means original pieces put to a new job; no new art.

| Part | Original piece | State |
|---|---|---|
| Window and background | the whole round menu layout page: `BG_FlipA`, `Top_Bar`, `Subtitle`, corner plates | exists |
| Title | `TEXT_Title` with "PC Settings" | exists, new text |
| Tab headers (Game, Display, Controls, Quality of life) | the subtitle bar with the current tab name between the `Pageslots` arrows, `PageNumber` "1/4" | composed (Q2) |
| Rows: label and value | the name plate text "Label: value", the Options page's pattern | exists |
| Row pictures | `UI_Options_SFX`, `_Music`, `_InvertCamera`, `_Language` for the Game tab; `UI_StatusScreen_Options` for the rows of the other round tabs | Game tab exists; others composed (Q4) |
| Value stepper (volumes, resolution) | none drawn: the value is part of the text and changes in place, stepped by Left and Right | exists as a pattern (Q8) |
| Toggle on/off | the Invert Camera pattern "Label: value" with "On" / "Off" (gametext `ON`, `OFF`) | exists (Q5) |
| Master button, three states | a value row "Quality of life: Original / Improved / Custom" in the toggle pattern | composed |
| Selection highlight | round form: the centre slot (neighbours drawn at the layout's lower opacity); list form: `Menu_Bar_HI` | exists |
| Scroll for long lists (Controls) | list form: five bars with the selection kept in view; `PageNumber` keeps the tab's place ("3/4") | composed (Q2, Q3) |
| "Waiting for key" and "key taken" messages | the notice form of `page_alerttext` (as "Checking save data") | composed (Q6) |
| Confirm question of Reset all | the question form of `page_alerttext` with Yes (A) and No (B), as the quit question | exists |
| Back and Select icons | the shortcut bar of the Options page | exists |
| Picture of the "PC Settings" pause entry (QOLSET-8) | `UI_StatusScreen_Options` | composed (Q11) |

**2. Scaling into the window** (Q1). The menu is one 320x240 page. Scale s = min(window width / 320, window height / 240), not rounded to an integer; the page is centred and covers the whole window; the rest of the window is black, as the port's bottom-screen bars are today. The page is drawn at window resolution (every element box, picture and glyph scaled by s, sampled with linear filtering as the port's screen presenter does), not as a 320x240 picture enlarged. Examples: 400x240 gives s = 1 with 40-pixel bars left and right; 1920x1080 gives s = 4.5, a 1440x1080 page and 240-pixel bars; 3440x1440 gives s = 6, a 1920x1440 page.

**3. Text sizes and glyphs.** The three front-end font files are one glyph set, line height 23, glyphs up to 22 pixels high. The layout texts ask for sizes 12 (round texts), 15 (title, subtitle), 16 (page number), 18 (name plate), 14 and 10 (notice page); whether the size changes the drawn glyphs is not known, and the port draws every layout text at the atlas size (HUD-75, UI-16), so nothing is drawn before HUD-75 is fixed (Q10). If the sizes do shrink the glyphs, the atlas holds 23/size times the 3DS resolution, so text stays sharp up to about s = 1.3 (size 18) to s = 1.9 (size 12) and is enlarged above that. Coverage: all of ASCII, the Latin-1 letters and × have glyphs, so SDL key, mouse and pad names, resolutions such as `1280x720`, English labels and the language names Deutsch, Français, Nederlands, English, Italiano, Português, Dansk, Español, Polski and Русский can all be drawn. Missing: control codes, ¢ £ ¤ ¥ § ± ² ³ µ ¼ ½ ¾ and similar signs, and Cyrillic letters the game's texts do not use. The new texts ("PC Settings", the tab names, "Window mode", "Resolution", "Original", "Improved", "Custom", the action names, "Reset all", the messages) are English in every language (Q7).

**4. Navigation onto the model's events** (`src/qol/menu_model.hpp`):
- Up / Down, pad Up / Down, the mouse wheel (Q9): `Up` / `Down`, previous or next row with wrap.
- Enter, pad A, click on the Select icon: `Activate`: the master row cycles (`MasterCycled`), a feature row toggles (`FeatureToggled`, or `SwitchedToCustom` outside Custom), a Display row cycles window mode or resolution (`DisplayChanged`); in the model of 2026-10-29 the Game and Controls rows are placeholders (`Ignored`).
- Escape, pad B, click on the Back icon: `Back`, the menu closes.
- PageUp / PageDown, pad L / R, click on the `Pageslots` arrows: `TabPrevious` / `TabNext`, with wrap; hover over a header: `tabMove(i)` (ignored).
- Mouse over a drawn row: `rowMove(i)`; click on a row: `rowClick(i)` (select, then activate). In the round form a click on a neighbour slot thus selects and activates it, where the original only turns the round. Asked 2026-10-29: "Only on entering a slot": `rowMove` is sent only when the pointer enters a different slot, so a pointer resting on a slot does not spin the round. The area of a round slot (asked 2026-10-29): "Original tap circle": the original's touch rule, a circle of radius 32 pixels around the centre of each of the three visible slots.
- Left / Right, pad Left / Right: `Left` / `Right`, stepping the rows that Q8 names.
- While the Controls tab waits for a key, the keys go to it; Escape cancels ([qol-settings.md §2026-10-29T11:06:51](qol-settings.md#ts-20261029-110651)).

**5. Name plate texts** (asked 2026-10-29 by 20261029-qolmenu2-A):
- Game tab labels: "Original gametext labels": the labels of the original Options page from the game text, not the model's English labels.
- Quality of life tab: "Readable names", offered as, for example, "Quality of life: Original" for the master row and "Widened view: Off" for the feature row; neither the config key nor "Master" is shown.

- Controls bar form (asked 2026-10-29): "Two lines per bar": each Controls bar shows the action name on top and its two slots smaller underneath; fewer rows fit on the page. Answer 2026-10-30, "How should this go ahead while the port cannot draw smaller layout text (HUD-96)?": "Wait for HUD-96": build smaller text in the port first, then the two-line bars as decided.

Open (not decided by the user): whether every text fits its box (name plate 182 pixels, list bar 168 pixels) at the font size HUD-75 brings (QOLSET-34).
