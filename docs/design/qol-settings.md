# QoL: settings menu and toggles

The user's approved design of how quality-of-life features are switched on and off: the settings menu, the master toggle, the per-feature toggles and where the choices are saved. Plan: [qol.md](../plans/qol.md); rules: [qol.md](../rules/qol.md); feature file: [qol-settings-menu.md](../plans/features/qol-settings-menu.md).

Subtopics: [qol-settings-drawing.md](qol-settings-drawing.md)

<a id="ts-20261028-092131"></a>

## 2026-10-28T09:21:31+01:00 — Settings menu, master toggle and rules

Asked of the user by the lead on 2026-10-28 (planner questions 1 to 4 and 8); answers quoted as selected.

- Gate: "I am opening the QOL of Life Gate." (the user, 2026-10-28), earlier than the old condition "after the game is finished for PC".
- Rule 1, the user's words: "Every QOL of Live Feature is switched in the Menu". Every QoL feature has a toggle in the settings menu; there is no other way to switch one (no command-line-only, file-only or key-only switch for the player).
- Rule 2, the user's words: "The faithful Version Stays no 1 Priority, everything is Tested First on the original Port without QOL and then with the Features enabled." Each feature is first tested in original mode (all QoL off, the port's tests unchanged and passing), then with the feature on.
- Toggle mechanism, from the user's description: "The mechanism to Switch bettween 'improved' and 'original' more should be integrated into the normal Game Settings Menu, a toggle there switches eitler All QOL Features on/off or you can select each by itself."
- Default (question 1): "OFF (Recommended)".
- Where the settings menu opens (question 2): from the title screen ("Title screen (Rec.)") and from the pause menu, through its own "PC Settings" entry ([§2026-10-28T09:32:07](#ts-20261028-093207)). A dedicated key was not selected: no extra key.
- Menu content besides the master toggle and the per-feature toggles (question 3): "Window mode / resolution, Key bindings". "Music/sound volume" and "Nothing else" were not selected; the original Options entries (SFX, Music, Invert camera, Language) are mirrored in the Game tab ([§2026-10-28T09:32:07](#ts-20261028-093207)).
- Master toggle (question 4): "3 states, qol.cfg (Rec.)": Original, Custom and Improved, saved in a new `config/qol.cfg`, never in the game save, so saves stay faithful. The master state is a stored value: [§2026-10-28T09:53:33](#ts-20261028-095333).
- Order (question 8, "Yes to all (Rec.)"): S1 settings menu and toggles first, nothing else before it; S2 full screen; S3 minimap; S4 the rest of the bottom screen.

<a id="ts-20261028-092408"></a>

## 2026-10-28T09:24:08+01:00 — Separate source code

Rule 3 of the user (2026-10-28): "Another rule: Source Code for These Features is separated from other Code! No mixing".

The agent proposed: QoL code in `src/qol/` as its own CMake library `chase_qol`, tests in `tests/qol/`; port files get only the link line, the include and hook calls listed by name; a CTest `qol-separation` guard (QOLSET-7). The user confirmed it: [§2026-10-28T09:32:07](#ts-20261028-093207).

<a id="ts-20261028-093207"></a>

## 2026-10-28T09:32:07+01:00 — Reaching the menu, its content, layout, file format and separation

Asked of the user by the lead on 2026-10-28; the quoted labels are the user's selections.

- Reaching the menu with QoL off (QOLSET-5): "Title + PC Settings entry": reachable from the title screen, plus an extra "PC Settings" entry clicked with the mouse in the pause menu, always shown. The original Options page stays unchanged.
- Approved exception to the faithful default: the "PC Settings" entry of the pause menu is visible with every QoL feature off. This is the user's explicit decision. The entry opens the PC settings menu only and changes no game behaviour. The only other such exception is the "PC Settings" button of the save slot page ([§2026-10-29T18:06:35](#ts-20261029-180635)).
- Original Options entries (QOLSET-6): "Yes, mirrored": SFX volume, Music volume, Invert camera and Language are shown in the PC settings menu and kept in sync with the original Options page; same values, same save.
- Language row of the Game tab (QOLSET-21, 2026-10-29): "Opens Languages page": activating the row opens the port's own Languages page, as the original does. Until that page is ported the row stays read-only (QOLSET-25, the user's decision of 2026-10-29, the current plan).
- Layout (question 5a): "Tabs, mouse+keys+pad": tabs Game (the original options), Display (window mode, resolution), Controls (key bindings) and Quality of life (master toggle and per-feature toggles); mouse, keyboard and gamepad all work.
- `qol.cfg` format (5b): "Like controls.cfg": the same line format as `config/controls.cfg` (one "name value" per line), parsed strictly; unknown or malformed lines stop with an error naming the line. Key names: [§2026-10-28T21:43:23](#ts-20261028-214323).
- Separation (6): "Confirm as proposed": `src/qol/` as library `chase_qol`, tests in `tests/qol/` named `qol-*`, port files contain only the link line, a QoL hook header include and listed hook calls, the CTest `qol-separation` guard in the first round.

<a id="ts-20261028-093729"></a>

## 2026-10-28T09:37:29+01:00 — Master button, window modes, tabs, files and the pause entry

Asked of the user by the lead on 2026-10-28; the quoted labels are the user's selections. Decides QOLSET-10 to QOLSET-13.

- Master button (QOLSET-10): "Cycle all three": the master button runs through Original, Improved, Custom. This was not the recommended option (the recommendation was a click from Custom going to Improved).
- Window modes (QOLSET-11): "Win + borderless + F11 (Rec.)": windowed and borderless full screen, plus an F11 shortcut.
- Tabs and files (QOLSET-12): "Always, split files (Rec.)": the Game, Display and Controls tabs are always active; key bindings stay in `config/controls.cfg`, everything else in `config/qol.cfg`.
- "PC Settings" entry (QOLSET-13): "Last entry, mouse+keys (Rec.)": the last entry of the round pause menu, chosen by mouse click, D-pad or keyboard like the other entries.

<a id="ts-20261028-094040"></a>

## 2026-10-28T09:40:40+01:00 — What Custom restores

Asked of the user by the lead on 2026-10-28; the quoted label is the user's selection. Decides QOLSET-14.

- Custom (QOLSET-14): "Last own mix (Rec.)": when the master button cycles to Custom it restores the last Custom mix, kept in `config/qol.cfg`; Custom is skipped while no mix has been set.

<a id="ts-20261028-095333"></a>

## 2026-10-28T09:53:33+01:00 — The master state is stored

Asked of the user by the lead on 2026-10-28; the quoted label is the user's selection. Decides QOLSET-15.

- Master state (QOLSET-15): "Stored value (Rec.)": "Master state stored in config/qol.cfg. Original = all QoL off, Improved = all on (incl. minimap + hidden bottom); per-feature switches (so far only widened view + corner HUD) apply only in Custom."

<a id="ts-20261028-214323"></a>

## 2026-10-28T21:43:23+01:00 — Key names of config/qol.cfg

Asked of the user by the lead on 2026-10-28, with the names chosen by the worker of round 20261028-qol (agent A) for `config/qol.cfg`; the user's answer, as given: "Keep these names". Sets the key names and values in the line format of [§2026-10-28T09:32:07](#ts-20261028-093207) ("name value" per line, like `config/controls.cfg`).

- Master key: `master`, with the values `Original`, `Improved`, `Custom`. The values are case-sensitive.
- Feature keys: snake_case, with the values `on` or `off`. So far `widened_view`; later features (fullscreen, minimap) add their keys in the same style.
- `#` starts a comment line or a comment at the end of a line.
- Feature lines are written only once the user has set an own mix (Custom, see [§2026-10-28T09:40:40](#ts-20261028-094040)).
- Parsing stays strict: an unknown key or a malformed line stops with an error naming the line (CLAUDE.md "Strict input").
- The Display keys `window_mode` and `resolution`: [§2026-10-28T22:20:41](#ts-20261028-222041).

<a id="ts-20261028-222041"></a>

## 2026-10-28T22:20:41+01:00 — Display tab: window mode and resolution

Asked of the user by the lead on 2026-10-28 and 2026-10-29; the quoted labels are the user's selections. Decides the storage of QOLSET-3 and the saving part of QOLSET-4, QOLSET-18, QOLSET-19, QOLSET-28 and QOLSET-29.

Storage:
- Where (2026-10-28, "Where should the Display tab store the window mode and the resolution?"): "qol.cfg: window_mode, resolution": two keys in `config/qol.cfg`, next to `master` and `widened_view`.
- Values (2026-10-28): "windowed|fullscreen, WxH, always": `window_mode` is `windowed` or `fullscreen`; `resolution` is `WIDTHxHEIGHT`, for example `1280x720`; both are written in every master mode.
- Desktop size (QOLSET-19, 2026-10-29): "Omit the line": `config/qol.cfg` has no `resolution` line while the resolution is the desktop size, until the player picks a size; `window_mode` is still written in every mode.

- `--window` and the menu (QOLSET-56, 2026-10-30, "With --window <size>, when the resolution row in the menu changes, what should the window do?"): "Menu wins": the window takes the size chosen in the menu, which is also saved.

Defaults (QOLSET-18 and QOLSET-19, 2026-10-29):
- Keys absent: "Fullscreen, desktop size": borderless full screen at the monitor's current resolution.
- No `config/qol.cfg` at all: "Fullscreen default": the same, also when the file is missing.
- Old files without the two keys: "Still valid, use defaults": a missing key takes its default; the next write adds `window_mode`, and `resolution` once the player picks a size.

Resolutions (QOLSET-18, QOLSET-19, QOLSET-28 and QOLSET-29, 2026-10-29):
- The list: "Display modes from SDL": the modes the monitor reports that are at least 400x240.
- A stored size not in the list, for example a hand-edited 1000x600: "Keep it, show as custom": the value is used and shown as an extra entry until the user picks another.
- Upper bound: "Largest display mode", with the behaviour "Clamp to largest mode": a stored resolution above the monitor's largest mode is replaced by that mode at startup, and the file is corrected on the next write (no error). "Either side larger": a size is above the limit when its width or its height is above the largest mode's width or height.
- Resolution in full screen (QOLSET-28): "Windowed only": the resolution sets only the windowed size; borderless full screen always uses the desktop size.
- `--window WIDTHxHEIGHT` (QOLSET-29): "Yes, option wins": the option overrides `config/qol.cfg` for that run.

Implemented in `chase-game` by 20261029-qolview-B (QOLSET-19):
- At startup `config/qol.cfg` (next to the controls file) is read strictly, the stored size is clamped to the largest SDL display mode of display 0, and the window is created in `SDL_WINDOW_FULLSCREEN_DESKTOP` (borderless) or windowed at the effective size (stored size or desktop size). Display modes below 400x240 and duplicates are dropped.
- The F11 binding is the action `Fullscreen` in `config/controls.cfg` (`Fullscreen key F11 hold`). Each new press switches the mode and writes `window_mode` through the settings store; `resolution` stays unwritten while the desktop size is in use. The window has a 400x240 minimum size.
- What changes at defaults: `chase-game` without `config/qol.cfg` starts in borderless full screen at the desktop size instead of a 960x600 window. The test build writes `build/config/qol.cfg` with `window_mode windowed` and `resolution 960x600` so that the screenshot tests keep their size.

Open (not decided by the user): how the largest mode itself is picked (the model uses the one with the most pixels).

<a id="ts-20261028-223857"></a>

## 2026-10-28T22:38:57+01:00 — Feature rows outside Custom, wrapping rows and tabs

Asked of the user by the lead on 2026-10-28; the quoted labels are the user's selections. Decides QOLSET-16 and QOLSET-17.

- Feature row while the master is Original or Improved (QOLSET-16): "Switch to Custom": "Copy the current mode's switches into an own mix, apply the click, and set the master to Custom."
- Moving past the first or last row (QOLSET-17): "Wrap around" held for the old menu; in the redesigned menu tabs wrap and rows do not (answer 6): [qol-settings-drawing.md §2026-10-30T21:56:04](qol-settings-drawing.md#ts-20261030-215604). Keys: see there.

<a id="ts-20261029-110651"></a>

## 2026-10-29T11:06:51+01:00 — Rebind flow and rebind scope

Asked of the user by the lead in question prompts on 2026-10-29; the quoted labels are the user's selections. Decides QOLSET-26, QOLSET-27 and QOLSET-32.

Rebind flow (QOLSET-26):
- A key already bound to another action: a dialog names the other action and offers Swap or Cancel; the tab keeps waiting after Cancel ([qol-settings-drawing.md §2026-10-30T21:56:04](qol-settings-drawing.md#ts-20261030-215604)).
- Reset to defaults: each tab ends with a "Reset to defaults" button that asks first and resets everything on its tab (answer 8) ([qol-settings-drawing.md §2026-10-30T21:56:04](qol-settings-drawing.md#ts-20261030-215604)).
- Reset all source and saving (QOLSET-32, asked later the same day): "Shipped file, save now": Reset all restores the bindings of the shipped `config/controls.cfg`, and the file is written right after each change or reset.
- Cancel waiting for a key: "Escape": Escape cancels, so Escape itself and the menu keys cannot be bound from the tab.

Applying a rebind (QOLSET-55, 2026-10-30, "Controls tab: should a rebind also take effect in the running game at once?"): "Apply at once": a rebind takes effect in the running game as soon as it is written.

Rebind scope (QOLSET-27):
- Inputs that can be rebound: "Keys + mouse + pad": keyboard keys, mouse buttons and gamepad buttons.
- Stand-in actions `Walk` and `DebugOverlay`: "Listed with the rest": in the same list as the 3DS actions.
- Several bindings per action and their hold or toggle mode: "Two slots, mode per slot": a primary and a secondary slot per action; each slot can switch between hold and toggle; a slot can be cleared.

Implemented by 20261029-qolview-C (`src/qol/controls_tab.*`, `src/qol/controls_file.*`, tests `qol-controls-tab`, `qol-controls-file`):
- Model: a taken input (same source and code, also on the same action) is refused with a message and the tab keeps waiting; "Reset all" asks for confirmation (`requestReset`, `confirmReset`, cancel declines); Escape cancels waiting and cannot be bound; keys and mouse buttons; at most two slots per action, a hold or toggle mode per slot (a new slot is `hold`, rebinding keeps the mode), clearing a slot.
- Adapter: `readBindingEntries` runs `chase::readBindings` (strict, no second parser) and names the codes back through a key-namer callback; `controlActions` lists all 14 actions in enum order, `Walk` and `DebugOverlay` last.
- Writer: `formatBindingEntries` / `writeBindingFile` validate every entry, write one `Action source Code mode` line each (spaces as `_`), check that `readBindings` accepts the text, and replace the file through a temporary file.
- Join (20261029-qolmenu-C, `src/runtime/qol_controls_bindings.*`, test `qol-controls-bindings`): `QolControlsBindings` loads the controls file and the shipped file through `readBindingEntries` with the SDL key resolver (`SDL_GetKeyFromName`) and namer (`SDL_GetKeyName`), owns the `ControlsTab` and writes the file after each change and after a confirmed reset; a failed write throws.

Gamepad buttons (QOLSET-30), answered 2026-10-29: "New `pad` source": `config/controls.cfg` gets a `pad` source next to `key` and `mouse`, with SDL game-controller button names such as `a`, `b`, `x`, `y`, `leftshoulder`, `dpup`.
- Implemented by 20261029-qolmenu-C: a line is `Action pad <name> hold|toggle`. The names are SDL's lower-case names (`a b x y back guide start leftstick rightstick leftshoulder rightshoulder dpup dpdown dpleft dpright misc1 paddle1 paddle2 paddle3 paddle4 touchpad`), codes are the SDL button numbers (`a` is 0, `touchpad` is 20), `Source::Pad` in `src/input.hpp`. `readBindings` rejects any other name (strict); a file with no `pad` line behaves as before. The reader, the tab validator `fileInputValidator` and the writer accept the same names.

Controls tab keyboard and wording, answered 2026-10-29:
- Slots: the Controls tab has the columns Action, Slot 1 and Slot 2; choosing a slot waits for the new input; Del clears it; a Mode column switches hold and toggle ([qol-settings-drawing.md §2026-10-30T21:56:04](qol-settings-drawing.md#ts-20261030-215604), answer 7).
- Reset confirm question: "Reset all controls to their defaults?" with Yes / No.
- Waiting for a key and the dialogs are modal: every other input is ignored; only Enter (confirm) and Esc (cancel) act.

Comments (QOLSET-31), answered 2026-10-29: "Drop the comments": the writer writes the file fresh and the player's comments are lost. A key named `#` cannot be written (the reader treats `#` as a comment).

Shipped bindings for Reset all (QOLSET-38), answered 2026-10-29: "config/controls.default.cfg", a shipped file next to `controls.cfg`, never written by the menu.


Open (not decided by the user): none for the Controls tab flow.
<a id="ts-20261029-180635"></a>

## 2026-10-29T18:06:35+01:00 — Opening the menu from the front end

Asked of the user by the lead on 2026-10-29 for 20261029-qolhook-B (QOLSET-41), because the port's front end has no title screen with entries; the quoted labels are the user's selections. They make "from the title screen" of [§2026-10-28T09:21:31](#ts-20261028-092131) concrete.
- Where: "SlotSelect page only": the settings menu opens from the save slot page of the front end, not from the other front-end screens.
- Trigger: "Bottom-screen button": a "PC Settings" button drawn on the bottom screen of that page, clicked with the mouse only; there is no pad or keyboard way to it.
- While the menu is open: "Front end stops": nothing behind the menu updates; movies and sounds hold.
- Size: "Whole window": the menu covers the whole window ([qol-settings-drawing.md §2026-10-30T21:56:04](qol-settings-drawing.md#ts-20261030-215604)).
- The button (answered 2026-10-29, after the code review found that the bottom-left shortcut place holds the page's Delete shortcut on a used slot): "Third list bar": the unused third list bar (`ListItem3` of the slot page layout) under the two slots, showing "PC Settings"; the Delete shortcut stays untouched.
- Place of the bar: see the playtest decision [§2026-10-29T19:34:22](#ts-20261029-193422) (full-size bar with a hint).
- Slide-in (answered 2026-10-29): "Yes, slot 2's timing": the bar slides in with the slots, like `ListItem2`.
- With every QoL option at its default (2026-10-29): "Allow, update rule": the button is shown with QoL off, a second approved exception next to the pause entry ([docs/rules/qol.md](../rules/qol.md)). A left click inside the button's area opens the menu instead of acting as A: "Yes, acceptable", part of that exception.
- Choices of 20261029-qolhook-B that the user accepted as they are (2026-10-29): the button shows and takes clicks only until a slot is chosen; its click area follows the button; the menu keys are fixed (Return, Escape, PageUp/PageDown and the arrows; pad a, b, the D-pad and the shoulders); key repeats are swallowed while releases, quitting and focus events pass on to the front end.

<a id="ts-20261029-193422"></a>

## 2026-10-29T19:34:22+01:00 — After the playtest of the menu: button, sharpness, motion

Asked of the user by the lead on 2026-10-29 after the user's playtest ([playtests-qol.md §2026-10-29T19:31:01](playtests-qol.md#ts-20261029-193101)); the quoted labels are the user's selections.

- Opening the menu from the save slot page (QOLSET-53), "How should the PC settings menu be opened from the save slot page?": "Full-size bar + hint": "A full-size, clearly labelled bar (moving the slots is not allowed with QoL off, so it covers part of the page)." Answer 2026-10-30, "Where should it sit?": "Agent proposes mock-up": an agent proposes place, coverage and hint text, and the user approves before it is built. Approved 2026-10-30 (proposal of an agent, answers of the user): the full-size bar of the save slot page (bottom screen 320x240) at x=49 y=184, 256x32 (same size and x as a slot bar), under slot 2, ending one pixel above the Select/Delete shortcut row (y>=217); the slots do not move; it uses the hidden `ListItem3` element instead of the half-size bar; first line "PC Settings", second line a hint.
  - "Where should the bar go?": "Under the slots (Recommended)" (not chosen: "Above the slots").
  - "What hint text?": "Click with the mouse" (not chosen: "Click to change settings", "Mouse: game settings", "No hint line").
  - "May the hint use the slot bar's small second line, and is covering the top 6 px of the corner arts OK?": "Wait for HUD-96": the hint is built with the smaller-text capability (`LayoutScene::scales`, HUD-96, merged 2026-10-30).
  - Pixel values come from layout data, not a measured render; before building, measure from a render the visible art height of a slot bar and the left edge of the "Select" label.
  - "Is covering the top ~6 px of the two corner button arts OK?": "OK, keep 32 px bar (Recommended)": same height as a slot bar, only decoration is covered (not chosen: "Shorten the bar", about 26 px). No open points left.

<a id="ts-20261030-170317"></a>

## 2026-10-30T17:03:17+01:00 — Arrow keys mean what they say in the settings menu

The user asked on 2026-10-30 for a consistent rule: Left and Right are Left and Right on the arrow keys, Up and Down are Up and Down. The redesign of the same day sets it: Up and Down move between rows, Left and Right change the value or, on the tab bar, switch tabs ([qol-settings-drawing.md §2026-10-30T21:56:04](qol-settings-drawing.md#ts-20261030-215604)).
- Gamepad: "Yes, same rule": the D-pad follows the arrow rule; the shoulder buttons switch the tabs.
- Scope: "All menus": the same rule everywhere. The faithful menus (pause menu, Options page, other wheels) already turn with Left and Right, so nothing changes in them and the faithful game stays the default; the change is in the PC Settings menu.
