# QoL: settings menu drawing

How the settings menu is drawn: which original art and font it reuses, how it scales into the window, its text and the navigation onto the menu model's events; also the user's answer on the Reset all source. Subtopic of [qol-settings.md](qol-settings.md); feature file: [qol-settings-menu.md](../plans/features/qol-settings-menu.md).

## Current state

- Menu drawing, decided: Options-page art; the whole window at a free scale drawn at window resolution with black bars; tab name in the subtitle bar between arrows; round rows for Game, Display and Quality of life, list bars for Controls; `UI_StatusScreen_Options` for new rows and the "PC Settings" entry; "On" / "Off"; alert notices; English new texts; Left and Right step value rows; the wheel moves rows; no drawing before HUD-75: [§2026-10-29T12:11:55](#ts-20261029-121155) (proposal: [§2026-10-29T11:51:31](#ts-20261029-115131)).
- Reset all restores the shipped `config/controls.cfg` bindings, and the file is written after each change or reset: [§2026-10-29T12:11:55](#ts-20261029-121155).
- Left and Right step the volume rows, the Display rows, the master row and the toggle rows: [§2026-10-29T12:18:34](#ts-20261029-121834).
- Left steps backwards and Right forwards through the master states, the window mode and the resolution list, stopping at the ends without wrap; Invert camera flips like a toggle row; the Language and Controls rows ignore Left and Right: [§2026-10-29T12:20:38](#ts-20261029-122038).
- Open: none of the menu drawing questions; the drawing waits for HUD-75.

<a id="ts-20261029-115131"></a>

## 2026-10-29T11:51:31+01:00 — Settings menu drawing: art, fonts and layout (proposal)
Corrected by [§2026-10-29T12:11:55](#ts-20261029-121155): the questions are answered; toggles show "On" / "Off", and nothing is drawn before HUD-75 is fixed.

Spec round of QOLSET-22 by 20261029-qolview-D (docs only, no code, no emulator).

**Basis, the user's decision** (2026-10-29, question 1, label "Options-page art", relayed by the lead in the round's brief): the settings menu is drawn with the original Options-page art and font exactly as the 3DS draws them, scaled up. Everything else below is a **proposal, awaiting approval**; nothing in it is decided. Findings behind it: the Options page [ui-screens.md §2026-10-28T22:51:42](../re/ui-screens.md#ts-20261028-225142), the layout pieces [ui-layouts.md §2026-10-29T11:50:58](../re/ui-layouts.md#ts-20261029-115058), the font [text-localisation-fonts.md §2026-10-29T11:50:26](../re/text-localisation-fonts.md#ts-20261029-115026).

**The Options page as the 3DS draws it:** a bottom-screen page (320x240) of the round menu layout `sprites/ui_roundaboutmenu/main`: the background, the top bar with the title, the subtitle bar ("Options"), the round of five hexagon pictures with the selected one in the centre, the name plate with "Label: value" ("Music: 5", "Invert Camera: Yes"), and the Back (B) and Select (A) shortcuts in the bottom corners. Left and Right turn the round, Up and Down change a value, A toggles Invert Camera or opens the Languages page, B leaves.

**1. Parts and the art they reuse** (proposal). "Exists" means an original piece does this job; "composed" means original pieces put to a new job; no new art.

| Part | Original piece | State |
|---|---|---|
| Window and background | the whole round menu layout page: `BG_FlipA`, `Top_Bar`, `Subtitle`, corner plates | exists |
| Title | `TEXT_Title` with "PC Settings" | exists, new text |
| Tab headers (Game, Display, Controls, Quality of life) | none | question Q2 |
| Rows: label and value | the name plate text "Label: value", the Options page's pattern | exists |
| Row pictures | `UI_Options_SFX`, `_Music`, `_InvertCamera`, `_Language` for the Game tab; none for the other tabs | Game tab exists; others question Q4 |
| Value stepper (volumes, resolution) | none drawn: the value is part of the text and changes in place | exists as a pattern; keys question Q8 |
| Toggle on/off | the Invert Camera pattern "Label: Yes/No" (gametext `YES`, `NO`; `ON`, `OFF` also exist) | exists, question Q5 |
| Master button, three states | none; a value row "Quality of life: Original / Improved / Custom" in the toggle pattern | composed |
| Selection highlight | round form: the centre slot (neighbours drawn at the layout's lower opacity); list form: `Menu_Bar_HI` | exists |
| Scroll for long lists (Controls) | round form: none needed, the rows wrap; list form: five bars with the selection kept in view, `PageNumber` "n/m" as position | composed, question Q3 |
| "Waiting for key" and "key taken" messages | the notice form of `page_alerttext` (as "Checking save data") | composed, question Q6 |
| Confirm question of Reset all | the question form of `page_alerttext` with Yes (A) and No (B), as the quit question | exists |
| Back and Select icons | the shortcut bar of the Options page | exists |
| Picture of the "PC Settings" pause entry (QOLSET-8) | none of its own | question Q11 |

**2. Scaling into the window** (proposal). The menu is one 320x240 page. Scale s = min(window width / 320, window height / 240), not rounded to an integer; the page is centred and covers the whole window; the rest of the window is black, as the port's bottom-screen bars are today. The page is drawn at window resolution (every element box, picture and glyph scaled by s, sampled with linear filtering as the port's screen presenter does), not as a 320x240 picture enlarged. Examples: 400x240 gives s = 1 with 40-pixel bars left and right; 1920x1080 gives s = 4.5, a 1440x1080 page and 240-pixel bars; 3440x1440 gives s = 6, a 1920x1440 page. Alternatives: integer s with nearest filtering (sharp pixels, larger bars: s = 4 at 1920x1080); the 320x240 picture scaled as a whole (softer text); the page only in the bottom-screen area of the window as on the 3DS, with the top screen kept (needs another form when Improved hides the bottom screen); the bars filled with the 512x256 background instead of black (a placement the original does not have).

**3. Text sizes and glyphs.** The three front-end font files are one glyph set, line height 23, glyphs up to 22 pixels high. The layout texts ask for sizes 12 (round texts), 15 (title, subtitle), 16 (page number), 18 (name plate), 14 and 10 (notice page); whether the size changes the drawn glyphs is not known, and the port draws every layout text at the atlas size (HUD-75, UI-16). If the sizes do shrink the glyphs, the atlas holds 23/size times the 3DS resolution, so text stays sharp up to about s = 1.3 (size 18) to s = 1.9 (size 12) and is enlarged above that. Coverage: all of ASCII, the Latin-1 letters and × have glyphs, so SDL key, mouse and pad names, resolutions such as `1280x720`, English labels and the language names Deutsch, Français, Nederlands, English, Italiano, Português, Dansk, Español, Polski and Русский can all be drawn. Missing: control codes, ¢ £ ¤ ¥ § ± ² ³ µ ¼ ½ ¾ and similar signs, and Cyrillic letters the game's texts do not use. Open: whether every label fits the name plate (182 pixels) or a list bar (168 pixels) at the atlas size (QOLSET-34); the new texts ("PC Settings", the tab names, "Window mode", "Resolution", "Original", "Improved", "Custom", the action names, "Reset all", the messages) have no original translation (question Q7).

**4. Navigation onto the model's events** (`src/qol/menu_model.hpp`; no new rules, only which event drives which action):
- Up / Down, pad Up / Down: `Up` / `Down`, previous or next row with wrap.
- Enter, pad A, click on the Select icon: `Activate`: the master row cycles (`MasterCycled`), a feature row toggles (`FeatureToggled`, or `SwitchedToCustom` outside Custom), a Display row cycles window mode or resolution (`DisplayChanged`); Game and Controls rows are still placeholders (`Ignored`).
- Escape, pad B, click on the Back icon: `Back`, the menu closes.
- PageUp / PageDown, pad L / R: `TabPrevious` / `TabNext`, with wrap. A click on a tab header: `tabClick(i)` (Q2 b) or, with the arrows of Q2 a, `TabPrevious` / `TabNext`; hover over a header: `tabMove(i)` (ignored).
- Mouse over a drawn row: `rowMove(i)`; click on a row: `rowClick(i)` (select, then activate). In the round form a click on a neighbour slot thus selects and activates it, where the original only turns the round.
- Left / Right, pad Left / Right: `Left` / `Right`, ignored by the user's decision of [§2026-10-28T22:38:57](qol-settings.md#ts-20261028-223857).
- Not in the model: a volume step (`GameTab::step`; the original uses Up / Down, which the model uses for rows: Q8), the mouse wheel (Q9), routing keys to the Controls tab while it waits for a key (Escape cancels, [§2026-10-29T11:06:51](qol-settings.md#ts-20261029-110651)).

**5. Questions for the user** (recommendation first):
- Q1 Placement and scale: (a) whole window, free scale, drawn at window resolution, linear filtering, black bars (recommended); (b) integer scale, nearest filtering; (c) the 320x240 picture scaled; (d) only the bottom-screen area, as on the 3DS.
- Q2 Tab headers: (a) the subtitle bar shows the current tab name between the `Pageslots` arrows, `PageNumber` shows "1/4" (recommended, all pieces of the same layout); (b) four tab names as plain text in the top bar, the current one under the highlight bar; (c) the tabs as a first round, their rows as a second.
- Q3 Row form: (a) the round for all tabs, as the Options page; (b) the list form (five bars) for all tabs; (c) the round for Game, Display and Quality of life, the list form for Controls, whose 14 actions with two slots each do not fit one name plate (recommended); (d) plain text rows on the background.
- Q4 Pictures for rows without their own (round form): (a) `UI_StatusScreen_Options` for every new row (recommended); (b) no picture, an empty slot.
- Q5 Toggle words: (a) "Yes" / "No", the page's own words (recommended); (b) "On" / "Off" from gametext.
- Q6 "Waiting for key" and "key taken": (a) the notice form of `page_alerttext` (recommended); (b) the text of the name plate or bar.
- Q7 New texts without an original translation: (a) English in every language (recommended); (b) the menu waits until translations are approved.
- Q8 Changing a volume: (a) Left / Right step value rows (recommended; changes "Left and Right stay ignored" for these rows); (b) `Activate` steps up and wraps 10 to 0; (c) Up / Down step and Left / Right move rows, as the original.
- Q9 Mouse wheel: (a) moves rows like Up / Down (recommended); (b) no wheel.
- Q10 Text size until HUD-75 is solved: (a) the menu uses the port's current layout text drawing and follows HUD-75 when the port fixes it (recommended); (b) no drawing before HUD-75.
- Q11 Picture of the "PC Settings" pause entry: (a) `UI_StatusScreen_Options`, the wrench of Options, so the round shows it twice (recommended); (b) `UI_StatusScreen_Extras`, the yellow brick of the red brick extras; (c) no picture.

Open (not decided by the user): all of points 1 to 5 above; QOLSET-22 stays open until the user answers. Work items: drawing QOLSET-33, text and glyph check QOLSET-34.

<a id="ts-20261029-121155"></a>

## 2026-10-29T12:11:55+01:00 — Menu drawing answers and the Reset all source
Corrected by [§2026-10-29T12:18:34](#ts-20261029-121834): Left and Right also step the Display rows, the master row and the toggle rows.

Asked of the user by the lead on 2026-10-29; the quoted labels are the user's selections, relayed by the lead to 20261029-qolview-D. Answers the eleven questions of the drawing proposal, [Corrects §2026-10-29T11:51:31 in part](#ts-20261029-115131) (toggle words and text size differ from its recommendations), and [Corrects §2026-10-28T22:38:57 in part](qol-settings.md#ts-20261028-223857) (Left and Right are no longer ignored on value rows). With the basis of that proposal (the user's "Options-page art" decision) these settle QOLSET-22.

Menu drawing:
- Q1 placement and scale: "Whole window, free (Rec.)": the menu page covers the whole window at the scale min(width / 320, height / 240), not rounded to an integer, drawn at window resolution with linear filtering; the rest of the window is black.
- Q2 tab headers: "Subtitle bar + arrows (Rec.)": the subtitle bar shows the current tab name between the `Pageslots` arrows, and `PageNumber` shows the tab's place ("1/4").
- Q3 row form: "Round + list (Rec.)": the round for Game, Display and Quality of life; the list bars for Controls.
- Q4 pictures of rows without their own: "Options picture (Rec.)": `UI_StatusScreen_Options`.
- Q5 toggle words: "On/Off": toggle rows show "On" or "Off" (gametext `ON`, `OFF`), not the proposal's recommended "Yes" / "No".
- Q6 "waiting for key" and "key taken": "Alert notice (Rec.)": the notice form of `page_alerttext`.
- Q7 new texts without an original translation: "English everywhere (Rec.)": English in every language.
- Q8 changing a value: "Left/Right (Rec.)": Left and Right step value rows; this replaces "Left and Right stay ignored" of [§2026-10-28T22:38:57](qol-settings.md#ts-20261028-223857) for those rows.
- Q9 mouse wheel: "Moves rows (Rec.)": the wheel moves rows like Up and Down.
- Q10 text size, the user's words: "Wait, and make HUD-75 the Next a Action": no menu drawing before the port's HUD-75 (layout text font size) is fixed; HUD-75 is to be the next action of the `linux-port` plan (recorded there by the lead).
- Q11 picture of the "PC Settings" pause entry: "Options picture (Rec.)": `UI_StatusScreen_Options`.

Controls tab (QOLSET-32, a question of 20261029-qolview-C; [Corrects §2026-10-29T11:46:20 in part](qol-settings.md#ts-20261029-114620): its open point on defaults and saving is answered):
- Reset all: "Shipped file, save now": Reset all restores the bindings of the shipped `config/controls.cfg`, and the file is written right after each change or reset.

Open (not decided by the user): whether the Display rows, the master row and the toggle rows count as value rows for Left and Right (Q8 was asked about value rows such as the volumes); whether every text fits its box at the font size HUD-75 brings (QOLSET-34).

<a id="ts-20261029-121834"></a>

## 2026-10-29T12:18:34+01:00 — Rows that Left and Right step
Corrected by [§2026-10-29T12:20:38](#ts-20261029-122038): the direction and the remaining Game rows are answered.

Asked of the user by the lead on 2026-10-29; the quoted labels are the user's selections. Answers the open point of [§2026-10-29T12:11:55](#ts-20261029-121155) ([Corrects §2026-10-29T12:11:55 in part](#ts-20261029-121155)).

- Rows that Left and Right step besides the volume rows: "Display rows, Master row, Toggle rows" (all three selected). Left and Right thus step the volume rows, the Display rows, the master row and the feature toggle rows.

Open (not decided by the user): which way Left and Right step through a cycle (the master's Original, Improved, Custom; the window mode; the resolution list), for example Left backwards and Right forwards; whether Left and Right do anything on the Invert camera, Language and Controls rows.

<a id="ts-20261029-122038"></a>

## 2026-10-29T12:20:38+01:00 — Left and Right: direction and the remaining Game rows

Asked of the user by the lead on 2026-10-29; the quoted labels are the user's selections. Answers the open points of [§2026-10-29T12:18:34](#ts-20261029-121834) ([Corrects §2026-10-29T12:18:34 in part](#ts-20261029-121834)).

- Direction on the cycling rows (the master states, the window mode, the resolution list): "Left back, Right fwd, stop": Left steps backwards and Right forwards; they stop at the first and the last value, with no wrap.
- Invert camera, Language and Controls rows: "Invert only": Invert camera flips like a toggle row; the Language row (read-only) and the Controls rows (they open a page) ignore Left and Right.

Open (not decided by the user): none from these answers.
