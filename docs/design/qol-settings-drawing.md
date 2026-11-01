# QoL: settings menu look and behaviour

How the PC settings menu looks and works: a crisp, modern full-window settings screen without game assets. Subtopic of [qol-settings.md](qol-settings.md); feature files: [qol-settings-ui](../plans/features/qol-settings-ui.md), [qol-settings-menu](../plans/features/qol-settings-menu.md).

<a id="ts-20261030-215604"></a>

## 2026-10-30T21:56:04+01:00 — Settings menu redesigned as a modern PC menu

The user redesigned the whole PC settings menu on 2026-10-30. This replaces the earlier drawing with the original Options-page art, the bars and `Menu_List`, the round of pictures, the two-line Controls bars, the 4:3 page scaled into the window and the original UI animations. It also replaces the key layout that Left and Right turn from row to row and Up and Down change values. The game's art and font are no longer used inside the menu.

The user's words, after hearing how the menu worked: "Okay, this is REALLY unintuitive to navigate. Re design the entire PC settings to just NOT have game assets, to look really crisp on a large screen. Then the entire settings menu should just function like any other modern PC game"

Answers of the user to the lead's questions (2026-10-30; the quoted labels are the user's selections):
- Keys, in the user's words: "Which modern game uses UP Down to change tabs? Any modern game, has a row of categories you can select by mouse or by navigating to them with the arrow keys, these navigation row is on the left for example, then on the right the screen is filled with the actual options the inside the category."
- When settings take effect: "Instant + display confirm": volumes, toggles and controls apply at once; a window mode or resolution change asks "Keep these settings?" and reverts after 15 seconds without an answer.
- Layout: "Full-window overlay, top tabs": covers the whole window over a dimmed game picture; tabs in a bar across the top, rows in a centred column with labels left and values right, key hints along the bottom. Asked to confirm a left category column or top tabs: "Top tabs".
- Font: "Bundle an open font": one freely licensed font (for example Noto Sans or Inter under the SIL Open Font License), rendered sharp at the window's real resolution.
- Navigation model, asked: "Look up how the settings menus of modern PC games look, then report back with your suggestion". The lead reported the suggestion below (sources: [Xbox Accessibility Guidelines 107](https://learn.microsoft.com/en-us/gaming/accessibility/xbox-accessibility-guidelines/107) and [102](https://learn.microsoft.com/en-us/gaming/accessibility/xbox-accessibility-guidelines/102); per-game conventions mostly from general knowledge). The user answered its four open points, then approved it explicitly (2026-10-30), asked "Do you approve it explicitly as written?" (text and row sizes, 4.5:1 contrast, Q/E or PageUp/PageDown, right-click as Back, LB/RB/A/B, 15-second countdown, Back button bottom-left, help line, hint bar): "Yes, approved as written".

**Approved suggestion**
- Layout: a full-window overlay over the dimmed game; tabs across the top: Game, Display, Controls, Quality of life; below, one row per setting with the label left and its control right, the value always visible; one help line for the focused row with its default (for example "Default: On"); a key-hint bar at the bottom; a Back button bottom-left. Everything is sized relative to the window height: body text about 2.2 to 2.8 percent of it, rows about 5 to 6 percent, hit areas at least about 40 px at 1080p; crisp at 1080p and 4K; contrast at least 4.5:1; focus is not shown by colour alone (a filled bar plus a border or marker, the selected tab underlined). Long tabs scroll with the focus; the tab bar and the hint bar stay fixed.
- Keyboard: Up and Down move between rows, Left and Right change the value; Q and E or PageUp and PageDown switch tabs; Enter activates a toggle or button; Esc closes the menu, or first cancels an open prompt, dropdown or dialog.
- Arrow keys reach the tabs ("Yes, arrows reach tabs"): Up on the first row moves the focus onto the tab bar, Left and Right switch tabs there, Down returns to the rows; Q and E stay as the fast way.
- Mouse: hovering a row focuses it (mouse and keyboard share one focus); a click on a tab switches; sliders can be dragged or clicked; short choices are `< Fullscreen >` with clickable arrows and a click on the value also cycles it; the wheel scrolls; right-click works as Back.
- Controller: LB and RB switch tabs, the D-pad moves, A selects, B goes back.
- Row types: volumes are a slider with a number 0 to 100; on/off settings `< On >` and `< Off >`; Window mode and Language are arrow selectors; the resolution is a dropdown list ("Dropdown list": click or Enter opens the list of detected resolutions); each tab ends with a "Reset to defaults" button that asks first.
- Controls tab: columns Action, Slot 1, Slot 2; choosing a slot shows "Press a key… (Esc cancel, Del clear)"; Esc and the menu keys themselves cannot be rebound. A key already bound to another action ("Ask: Swap / Cancel"): a small dialog names the other action and offers to swap the two bindings or cancel.
- Display changes: a window mode or resolution change asks "Keep these settings?" with a 15-second countdown, reverts on Esc or at the timeout, Enter confirms.
- Master preset ("Switch to Custom, keep it"): changing a feature toggle by hand switches the preset to Custom at once; the custom set is remembered, so picking Original or Improved and later Custom restores it.

**What stays**
- The entry points: the "PC Settings" bar of the save slot page ([qol-settings.md §2026-10-29T18:06:35](qol-settings.md#ts-20261029-180635)) and the "PC Settings" entry of the pause wheel ([qol-settings.md §2026-10-28T09:37:29](qol-settings.md#ts-20261028-093729)); the user did not change them. Both are drawn by the original pages; whether they keep game art is open.
- The stores, values and files (`config/qol.cfg`, `config/controls.cfg`, Custom restore, Display storage), the faithful game at the defaults, English texts for every language of the menu's own texts, the code rules (OpenGL only in `src/render/`, shaders only in `.glsl` files, QoL code in `src/qol/`).
- A bundled font needs its licence recorded in `REUSE.toml` and must be GPL-compatible (the OFL is).

**Answers to the open points** (the user, 2026-10-30; the quoted labels are the user's selections):
1. Font: "Noto Sans" (SIL OFL; its coverage of Latin-1 and Cyrillic is needed for the language names).
2. UI scale option: "No, scale with window": everything is sized from the window height; a scale option can come later if missed.
3. Volume sliders (the original has 11 steps, 0 to 10): "0–100 in steps of 10": each step is exactly one original step.
4. Language row: "Selector, applies on close": `< English >` cycles the game's languages; the language takes effect when the menu closes.
5. The two entry points (slot-screen PC Settings bar, pause-wheel entry): "Keep game art": only the menu that opens is in the new style.
6. Ends of lists: "Tabs wrap, rows don't": E on the last tab goes to the first; Down on the last row stays; Up on the first row goes to the tab bar.
7. Hold or toggle on the Controls tab: "Mode column per action": columns Action, Slot 1, Slot 2, Mode, with `< Hold >` or `< Toggle >` for the actions that have it; both slots follow it.
7b. Pad binding: "Yes, same two slots": while a pad is connected the "Press a key…" prompt also accepts pad buttons; a slot holds a key, a mouse button or a pad button.
8. Scope of "Reset to defaults": "Everything on that tab": Game: both volumes, invert camera, language; Display: window mode and resolution, with the keep-settings countdown; Controls: all bindings and modes; Quality of life: the preset back to Original; it always asks first.
9. Resolution row in full screen: "Greyed out with a note": it stays in place, cannot be changed and shows "Desktop resolution".

10. Proportions (the user, 2026-10-30T22:32:10+01:00, "Approve these values"): body text 2.5 percent and rows 5.5 percent of the window height; the row column 1.3 times the window height wide, at most 90 percent of the window width; controls 45 percent of the column; tab bar, help line and hint bar one row high.

11. Enter on an arrow selector row such as Window mode or Language (the user, 2026-10-30, "Do nothing"): Enter does nothing there; Left and Right and clicks change the value.
12. Right-click while the "Press a key..." prompt is open (the user, 2026-10-30, "Bind right button"): the right mouse button becomes the binding; only Esc cancels the prompt.
13. Enter in the "key already bound, Swap / Cancel" dialog (the user, 2026-10-30, "Enter = Cancel"): Enter cancels, Swap needs a click or moving the focus to it first; Esc, pad B and right-click also cancel. The "Keep these settings?" dialog keeps Enter = confirm.
14. Greyed-out rows such as Resolution in full screen (the user, 2026-10-30, "Skipped"): Up and Down, on keyboard and pad, jump over disabled rows; the note is seen only when the mouse hovers the row.

15. Keyboard and pad on a Controls row (the user, 2026-11-01, "Keep A's stand-in"): Left and Right move the focus between Slot 1, Slot 2 and Mode, without wrapping; Up and Down keep the column; Enter or A activates the focused cell (a slot opens the prompt, Mode toggles Hold and Toggle); a mouse click on a cell activates it.
16. Empty Slot 2 while Slot 1 is empty (the user, 2026-11-01, "Refuse it"): a click on it is refused; slots stay packed, so clearing Slot 1 moves Slot 2 up.

22. Language restored by "Reset to defaults" on the Game tab (the user, 2026-11-01, "Approve"): the language the game started with, the "Language at game start".
23. Window mode selector (the user, 2026-11-01, "Approve"): only Windowed and Fullscreen (no borderless), and the two values wrap with Left and Right.
24. Choosing the resolution that is already in use (the user, 2026-11-01, "Approve"): nothing changes and no "Keep these settings?" dialog opens.

25. Toggle rows of the Quality of life tab (the user, 2026-11-01, "Approve all"): any value change on a feature toggle (Enter, a click, Left or Right) flips it, because the navigation reports them all as one direction.

26. Preset selector (the user, 2026-11-01, "Approve all"): its value wraps at both ends (a value wrap, not a wrap of the row focus, so answer 6 stays) and skips Custom while no own mix is set.

27. "Reset to defaults" on the Quality of life tab (the user, 2026-11-01, "Approve all"): the confirmed reset sets the preset to Original and keeps the own mix, so Custom still restores it.

28. Help texts (the user, 2026-11-01, "Approve all"): the texts written for the Widened view toggle, the preset row and the reset row in `ui_page_qol.cpp`, each feature text ending with its default.

29. Painter values (the user, 2026-11-01, "Approve all"): mouse hint "Click select  Wheel scroll  Right-click back"; picture dim 0.72 and dialog dim 0.5; dialog width 0.8 of the row column; body line spacing 1.25; control inset 0.1 and corner radius 0.12 of the row height. Tab bar, hint bar and row column are opaque so the text keeps the 4.5:1 contrast of the Layout rule.

Open: none.
