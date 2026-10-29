# QoL: settings menu and toggles

The user's approved design of how quality-of-life features are switched on and off: the settings menu, the master toggle, the per-feature toggles and where the choices are saved. Plan: [qol.md](../plans/qol.md); rules: [qol.md](../rules/qol.md); feature file: [qol-settings-menu.md](../plans/features/qol-settings-menu.md).

## Current state

- Window mode, size and F11 in `chase-game` (implemented; answered in [§2026-10-29T11:53:10](#ts-20261029-115310)): [§2026-10-29T11:45:52](#ts-20261029-114552).
- Every QoL feature is switched in the settings menu, and the faithful version is priority one; the user's two rules: [§2026-10-28T09:21:31+01:00](#ts-20261028-092131).
- QoL is off by default; the master toggle has three states (Original, Custom, Improved), stored in `config/qol.cfg` (corrected by [§2026-10-28T09:53:33](#ts-20261028-095333), first answer: derived from the per-feature switches); choices live in `config/qol.cfg`, never in the game save: [§2026-10-28T09:21:31+01:00](#ts-20261028-092131).
- Corrected in part by [§2026-10-28T09:32:07](#ts-20261028-093207): the pause menu gets its own "PC Settings" entry instead of reusing Options, and the original option entries are mirrored in the Game tab. The first answer: the settings menu opens from the title screen and the pause menu, with no extra key; it holds the master toggle, one toggle per feature, window mode and resolution, and key bindings: [§2026-10-28T09:21:31+01:00](#ts-20261028-092131).
- QoL source code is separate from the port's code: `src/qol/`, port files get only listed hook points: [§2026-10-28T09:24:08](#ts-20261028-092408).
- The settings menu is reachable from the title screen and through an always shown "PC Settings" entry in the pause menu (the one approved exception to the faithful default; the original Options page stays unchanged); it has the tabs Game, Display, Controls and Quality of life; mouse, keyboard and gamepad all work: [§2026-10-28T09:32:07](#ts-20261028-093207).
- The Game tab mirrors the original Options entries (SFX volume, Music volume, Invert camera, Language), kept in sync with the original Options page, same values, same save: [§2026-10-28T09:32:07](#ts-20261028-093207).
- `config/qol.cfg` has the line format of `config/controls.cfg`, parsed strictly: [§2026-10-28T09:32:07](#ts-20261028-093207).
- The separation rule (`src/qol/`, library `chase_qol`, `tests/qol/` with `qol-*` tests, only the link line, a hook header include and listed hook calls in port files, CTest `qol-separation` in the first round) is confirmed: [§2026-10-28T09:32:07](#ts-20261028-093207).
- Master button, window modes, tabs, files and the pause entry position: the master button cycles Original, Improved, Custom; the Display tab has windowed and borderless full screen plus an F11 shortcut; the Game, Display and Controls tabs are always active, bindings stay in `config/controls.cfg` and the rest is in `config/qol.cfg`; "PC Settings" is the last entry of the round pause menu, chosen by mouse, D-pad or keyboard: [§2026-10-28T09:37:29](#ts-20261028-093729).
- All panels, the settings menu included, use the original game's art and fonts: [§2026-10-28T09:37:31](qol-screen-layout.md#ts-20261028-093731).
- Custom restores the last own mix kept in `config/qol.cfg` and is skipped while no mix has been set: [§2026-10-28T09:40:40](#ts-20261028-094040).
- The bottom screen and its panels have no toggle of their own; only the master button switches them: [qol-screen-layout.md §2026-10-28T09:48:15](qol-screen-layout.md#ts-20261028-094815).
- The minimap and the hiding of the bottom screen follow the master button only (Improved): [qol-screen-layout.md §2026-10-28T09:50:34](qol-screen-layout.md#ts-20261028-095034).
- The master state is stored in `config/qol.cfg`: Original = all QoL off, Improved = all on (including the minimap and the hidden bottom screen); the per-feature switches (so far only the widened view with the corner HUD) apply only in Custom: [§2026-10-28T09:53:33](#ts-20261028-095333).
- The key names of `config/qol.cfg` are approved: master key `master` with the values `Original`, `Improved`, `Custom` (case-sensitive); feature keys in snake_case with `on` or `off` (so far `widened_view`); `#` starts a comment: [§2026-10-28T21:43:23](#ts-20261028-214323).
- Window mode and resolution are stored in `config/qol.cfg` under the keys `window_mode` and `resolution`; their values are `windowed` or `fullscreen` and `WIDTHxHEIGHT`, both written in every mode ([§2026-10-28T22:38:57](#ts-20261028-223857)): [§2026-10-28T22:20:41](#ts-20261028-222041).
- Display defaults and list: absent `window_mode` and `resolution` default to fullscreen at the desktop size; old files stay valid; the resolution row offers the monitor's display modes of at least 400x240; a stored size outside the list is kept and shown as custom: [§2026-10-29T09:23:03](#ts-20261029-092303) (the desktop-size spelling and the upper bound: [§2026-10-29T10:31:01](#ts-20261029-103101)).
- Feature rows outside Custom copy the mode's switches into an own mix, apply the click and switch to Custom; menu rows and tabs wrap; Left and Right stay ignored: [§2026-10-28T22:38:57](#ts-20261028-223857), [§2026-10-28T23:13:22](#ts-20261028-231322).
- Answered 2026-10-29: the Game tab language row opens the port's Languages page; the file has no `resolution` line while the desktop size is in use; a stored resolution above the largest display mode is limited to it at startup: [§2026-10-29T10:31:01](#ts-20261029-103101).
- Rebind flow and scope: a taken key is refused with a message, one confirmed "Reset all" row, Escape cancels waiting; keys, mouse buttons and gamepad buttons can be rebound, `Walk` and `DebugOverlay` are listed with the 3DS actions, two slots per action with hold or toggle mode per slot and clearing; a stored resolution is above the limit when either side is larger than the largest mode's: [§2026-10-29T11:06:51](#ts-20261029-110651).
- Settings menu art and fonts: "Options-page art", the original Options-page art and font as the 3DS draws them, scaled up; which assets remain for a spec round. The Game tab's Language row stays read-only until the Languages page is ported. Controls tab model, adapter and writer implemented; open: pad format, comments, defaults and saving: [§2026-10-29T11:46:20](#ts-20261029-114620).
- Open: which exact assets the menu drawing uses (QOLSET-22, spec round); the rest follows the recorded decisions.

<a id="ts-20261028-092131"></a>

## 2026-10-28T09:21:31+01:00 — Settings menu, master toggle and rules
Corrected by [qol-settings.md §2026-10-28T09:53:33](#ts-20261028-095333): the master state is stored in `config/qol.cfg`, not derived from the per-feature switches.
Corrected by [qol-settings.md §2026-10-28T09:32:07](#ts-20261028-093207): how the menu is reached, the original Options entries, the layout, the `qol.cfg` format and the separation details are decided.

Asked of the user by the lead on 2026-10-28 (planner questions 1 to 4 and 8); answers quoted as selected.

- Gate: "I am opening the QOL of Life Gate." (the user, 2026-10-28), earlier than the old condition "after the game is finished for PC".
- Rule 1, the user's words: "Every QOL of Live Feature is switched in the Menu". Every QoL feature has a toggle in the settings menu; there is no other way to switch one (no command-line-only, file-only or key-only switch for the player).
- Rule 2, the user's words: "The faithful Version Stays no 1 Priority, everything is Tested First on the original Port without QOL and then with the Features enabled." Each feature is first tested in original mode (all QoL off, the port's tests unchanged and passing), then with the feature on.
- Toggle mechanism, from the user's description: "The mechanism to Switch bettween 'improved' and 'original' more should be integrated into the normal Game Settings Menu, a toggle there switches eitler All QOL Features on/off or you can select each by itself."
- Default (question 1): "OFF (Recommended)".
- Where the settings menu opens (question 2): "Pause menu Options (Rec.), Title screen (Rec.)". A dedicated key was not selected: no extra key.
- Menu content besides the master toggle and the per-feature toggles (question 3): "Window mode / resolution, Key bindings". "Music/sound volume" was not selected and "Nothing else" was not selected. Volumes are therefore not part of the menu; the original Options page does have SFX, Music, Invert camera and Language (UI-4 in [ui-menus.md](../plans/features/ui-menus.md)); whether any of these belongs into the settings menu is open.
- Master toggle (question 4): "3 states, qol.cfg (Rec.)": Original, Custom and Improved, derived from the per-feature switches (all off is Original, all on is Improved, anything else is Custom); choosing Original or Improved sets every switch. Saved in a new `config/qol.cfg`, never in the game save, so saves stay faithful.
- Order (question 8, "Yes to all (Rec.)"): S1 settings menu and toggles first, nothing else before it; S2 full screen; S3 minimap; S4 the rest of the bottom screen.

Open (not decided by the user): layout, look and navigation details of the menu (mouse and keyboard/pad); which of the original Options entries (SFX, Music, Invert camera, Language) are shown, and whether they are saved in the game save as in the original or in `qol.cfg`; the format of `qol.cfg`; whether window mode and key bindings are QoL toggles (off by default) or always available.

<a id="ts-20261028-092408"></a>

## 2026-10-28T09:24:08+01:00 — Separate source code

Rule 3 of the user (2026-10-28): "Another rule: Source Code for These Features is separated from other Code! No mixing".

The agent proposed: QoL code in `src/qol/` as its own CMake library `chase_qol`, tests in `tests/qol/`; port files get only the link line, the include and hook calls listed by name; a CTest `qol-separation` guard (QOLSET-7). The user confirmed it: [§2026-10-28T09:32:07](#ts-20261028-093207).

<a id="ts-20261028-093207"></a>

## 2026-10-28T09:32:07+01:00 — Reaching the menu, its content, layout, file format and separation

Asked of the user by the lead on 2026-10-28; the quoted labels are the user's selections. [Corrects §2026-10-28T09:21:31 in part](#ts-20261028-092131): the open points about reaching the menu, the original Options entries, the menu layout, the `qol.cfg` format and the separation details are decided here.

- Reaching the menu with QoL off (QOLSET-5): "Title + PC Settings entry": reachable from the title screen, plus an extra "PC Settings" entry clicked with the mouse in the pause menu, always shown. The original Options page stays unchanged.
- Approved exception to the faithful default: the "PC Settings" entry of the pause menu is visible with every QoL feature off. This is the user's explicit decision. The entry opens the PC settings menu only and changes no game behaviour; it is the one such exception.
- Original Options entries (QOLSET-6): "Yes, mirrored": SFX volume, Music volume, Invert camera and Language are shown in the PC settings menu and kept in sync with the original Options page; same values, same save.
- Layout (question 5a): "Tabs, mouse+keys+pad": tabs Game (the original options), Display (window mode, resolution), Controls (key bindings) and Quality of life (master toggle and per-feature toggles); mouse, keyboard and gamepad all work.
- `qol.cfg` format (5b): "Like controls.cfg": the same line format as `config/controls.cfg` (one "name value" per line), parsed strictly; unknown or malformed lines stop with an error naming the line.
- Separation (6): "Confirm as proposed": `src/qol/` as library `chase_qol`, tests in `tests/qol/` named `qol-*`, port files contain only the link line, a QoL hook header include and listed hook calls, the CTest `qol-separation` guard in the first round.

Open (not decided by the user): see the list in the report of the planner, recorded in the feature files as open items.

<a id="ts-20261028-093729"></a>

## 2026-10-28T09:37:29+01:00 — Master button, window modes, tabs, files and the pause entry

Asked of the user by the lead on 2026-10-28; the quoted labels are the user's selections. Decides QOLSET-10 to QOLSET-13.

- Master button (QOLSET-10): "Cycle all three": the master button runs through Original, Improved, Custom. This was not the recommended option (the recommendation was a click from Custom going to Improved).
- Window modes (QOLSET-11): "Win + borderless + F11 (Rec.)": windowed and borderless full screen, plus an F11 shortcut.
- Tabs and files (QOLSET-12): "Always, split files (Rec.)": the Game, Display and Controls tabs are always active; key bindings stay in `config/controls.cfg`, everything else in `config/qol.cfg`.
- "PC Settings" entry (QOLSET-13): "Last entry, mouse+keys (Rec.)": the last entry of the round pause menu, chosen by mouse click, D-pad or keyboard like the other entries.

Open (not decided by the user): what Custom restores when the master button cycles to it (QOLSET-14): the last Custom mix, a defined default mix, or skipping Custom while no mix exists.

<a id="ts-20261028-094040"></a>

## 2026-10-28T09:40:40+01:00 — What Custom restores

Asked of the user by the lead on 2026-10-28; the quoted label is the user's selection. Decides QOLSET-14.

- Custom (QOLSET-14): "Last own mix (Rec.)": when the master button cycles to Custom it restores the last Custom mix, kept in `config/qol.cfg`; Custom is skipped while no mix has been set.

Open: none from this answer.

<a id="ts-20261028-095333"></a>

## 2026-10-28T09:53:33+01:00 — The master state is stored

Asked of the user by the lead on 2026-10-28; the quoted label is the user's selection. Decides QOLSET-15. [Corrects §2026-10-28T09:21:31](#ts-20261028-092131): that entry said the master state is derived from the per-feature switches.

- Master state (QOLSET-15): "Stored value (Rec.)": "Master state stored in config/qol.cfg. Original = all QoL off, Improved = all on (incl. minimap + hidden bottom); per-feature switches (so far only widened view + corner HUD) apply only in Custom."

Open: none from this answer.

<a id="ts-20261028-214323"></a>

## 2026-10-28T21:43:23+01:00 — Key names of config/qol.cfg

Asked of the user by the lead on 2026-10-28, with the names chosen by the worker of round 20261028-qol (agent A) for `config/qol.cfg`; the user's answer, as given: "Keep these names". Refines the line format of [§2026-10-28T09:32:07](#ts-20261028-093207) ("name value" per line, like `config/controls.cfg`): this entry sets the key names and values.

- Master key: `master`, with the values `Original`, `Improved`, `Custom`. The values are case-sensitive.
- Feature keys: snake_case, with the values `on` or `off`. So far `widened_view`; later features (fullscreen, minimap) add their keys in the same style.
- `#` starts a comment line or a comment at the end of a line.
- Feature lines are written only once the user has set an own mix (Custom, see [§2026-10-28T09:40:40](#ts-20261028-094040)).
- Parsing stays strict: an unknown key or a malformed line stops with an error naming the line (CLAUDE.md "Strict input").

Open: none from this answer.

<a id="ts-20261028-222041"></a>

## 2026-10-28T22:20:41+01:00 — Where the Display tab stores window mode and resolution

Asked of the user by the lead on 2026-10-28: "Where should the Display tab store the window mode and the resolution?"; the quoted label is the user's selection. Decides the storage for QOLSET-3 and the saving part of QOLSET-4. Adds to the key names of [§2026-10-28T21:43:23](#ts-20261028-214323).

- Storage: "qol.cfg: window_mode, resolution": two new keys in `config/qol.cfg`, next to `master` and `widened_view`. The planner then puts QOLSET-3 into a later round.

Open (not decided by the user): the value formats of `window_mode` (for example the spelling of windowed and borderless) and of `resolution` (for example a width and height pair or a named size), and whether they are written only in Custom like the feature lines or always.

<a id="ts-20261028-223857"></a>

## 2026-10-28T22:38:57+01:00 — Feature rows outside Custom, row wrap, window mode and resolution values

Asked of the user by the lead on 2026-10-28; the quoted labels are the user's selections. Decides QOLSET-16 and QOLSET-17 and the open part of [§2026-10-28T22:20:41](#ts-20261028-222041).

- Feature row while the master is Original or Improved (QOLSET-16): "Switch to Custom": "Copy the current mode's switches into an own mix, apply the click, and set the master to Custom."
- Moving past the first or last row (QOLSET-17): "Wrap around": "From the last row to the first and back; the other keys as A built them." So PageUp/PageDown and pad L/R switch tabs, Enter or pad A activates, Escape or pad B goes back, hover selects, click selects and activates, and Left/Right stay ignored.
- Values in `config/qol.cfg`: "windowed|fullscreen, WxH, always": `window_mode` is `windowed` or `fullscreen`; `resolution` is `WIDTHxHEIGHT`, for example `1280x720`; both are written in every mode.

Open: none from this answer.

<a id="ts-20261028-231322"></a>

## 2026-10-28T23:13:22+01:00 — Tabs wrap

Asked of the user by the lead on 2026-10-28: "Should the tabs (PageUp/PageDown, pad L/R) also wrap from the last tab to the first and back? Right now they stop at the first and last tab." The user's answer, "Tabs wrap too": "From the last tab to the first and back, like the rows." Extends the row wrap of the entry before it.

Open: none from this answer.

<a id="ts-20261029-092303"></a>

## 2026-10-29T09:23:03+01:00 — Display defaults, old files and the resolution list
Corrected by [qol-settings.md §2026-10-29T10:31:01](#ts-20261029-103101): the open points (file spelling of the desktop size, upper bound) are answered; no `resolution` line is written while the desktop size is in use.

Asked of the user by the lead on 2026-10-29 (QOLSET-18); the quoted labels are the user's selections. Answers the open part of [§2026-10-28T22:38:57](#ts-20261028-223857).

- Defaults when the keys are absent: "Fullscreen, desktop size": fullscreen at the monitor's current resolution.
- Old `config/qol.cfg` files without the two keys: "Still valid, use defaults": a missing key takes its default; the next write adds both keys.
- Resolutions the Display row offers: "Display modes from SDL": the modes the monitor reports that are at least 400x240.
- A stored resolution that is not in the list, for example a hand-edited 1000x600: "Keep it, show as custom": the value is used and shown as an extra entry until the user picks another.

Open (not decided by the user): an upper bound for a stored resolution (the reader limits the digits to five per side as a parse safety, not an approved limit); how the file is written while the desktop size is still unresolved (the store holds "desktop" as no value and writes no `resolution` line until the window code calls the resolve step with the real size; the spelling for "desktop" in the file, if any, is not decided).

<a id="ts-20261029-103101"></a>

## 2026-10-29T10:31:01+01:00 — Languages page, desktop spelling, upper bound and the menu drawing spec round
Corrected by [qol-settings.md §2026-10-29T11:06:51](#ts-20261029-110651): a size is above the limit when either side is larger than the largest mode's side.
Corrected by [qol-settings.md §2026-10-29T11:46:20](#ts-20261029-114620): the Language row stays read-only until the Languages page is ported, instead of opening it.

Asked of the user by the lead on 2026-10-29; the quoted labels are the user's selections. [Corrects §2026-10-29T09:23:03 in part](#ts-20261029-092303) (open points answered, the old-files write rule changed for `resolution`) and answers the language question of [§2026-10-28T09:32:07](#ts-20261028-093207).

- Game tab language row (QOLSET-21): "Opens Languages page": activating the row opens the port's own Languages page, as the original does.
- File spelling while the resolution is still the desktop size (QOLSET-19): "Omit the line": `config/qol.cfg` has no `resolution` line until the player picks a size. This replaces "the next write adds both keys" of the old-files answer in [§2026-10-29T09:23:03](#ts-20261029-092303) for `resolution`; `window_mode` is still written in every mode.
- Upper bound for a stored resolution (QOLSET-19): "Largest display mode": a stored resolution above the monitor's largest mode is limited by it. Chosen behaviour: "Clamp to largest mode": at startup the largest mode is used instead and the file is corrected on the next write (no error).
- Next round (QOLSET-19 follow-up), the user chose "Yes, next round": a spec round for the menu drawing (QOLSET-2: which original art and fonts the menu reuses) happens in the next round.

Open (not decided by the user): which original art and fonts the settings menu uses (QOLSET-2, spec round next round). The meaning of "above the largest mode" for a size with one side above and one below is not stated; the model treats a size as above when either side exceeds the largest mode (largest = most pixels) and limits it to that mode.

<a id="ts-20261029-110651"></a>

## 2026-10-29T11:06:51+01:00 — Rebind flow, rebind scope and the meaning of "larger than the largest mode"

Asked of the user by the lead in question prompts on 2026-10-29; the quoted labels are the user's selections. Answers QOLSET-26 and QOLSET-27 and the "either side" point; [Corrects §2026-10-29T10:31:01 in part](#ts-20261029-103101).

Rebind flow (QOLSET-26):
- A key already bound to another action: "Refuse": nothing changes; a message says the key is taken; the tab keeps waiting for a key.
- Reset to defaults: "All, with confirm": one "Reset all" row that asks for confirmation.
- Cancel waiting for a key: "Escape": Escape cancels, so Escape itself cannot be bound from the tab.

Rebind scope (QOLSET-27):
- Inputs that can be rebound: "Keys + mouse + pad": keyboard keys, mouse buttons and gamepad buttons.
- Stand-in actions `Walk` and `DebugOverlay`: "Listed with the rest": in the same list as the 3DS actions.
- Several bindings per action and their hold or toggle mode: "Two slots, mode per slot": a primary and a secondary slot per action; each slot can switch between hold and toggle; a slot can be cleared.

Display (QOLSET-19):
- "Larger than the largest display mode": "Either side larger": a stored size is above the limit when its width or its height is above the largest mode's width or height. Settles the open point of [§2026-10-29T10:31:01](#ts-20261029-103101).

Open (not decided by the user): how the largest mode itself is picked (the model uses the most pixels). The model changes for these answers (refuse message, Reset all row with confirmation, Escape cancel, mouse and pad binding, two slots with a mode each, clearing) are not written yet.

<a id="ts-20261029-114552"></a>

## 2026-10-29T11:45:52+01:00 — Window mode, size and F11 at startup (implementation record)

Recorded by 20261029-qolview-B; no new user decision. Applies [§2026-10-29T09:23:03](#ts-20261029-092303) and [§2026-10-29T10:31:01](#ts-20261029-103101) in `chase-game` (QOLSET-19).

- At startup `config/qol.cfg` (next to the controls file) is read strictly, the stored size is clamped to the largest SDL display mode of display 0, and the window is created in `SDL_WINDOW_FULLSCREEN_DESKTOP` (borderless) or windowed at the effective size (stored size or desktop size). Display modes below 400x240 and duplicates are dropped.
- The F11 binding is the action `Fullscreen` in `config/controls.cfg` (`Fullscreen key F11 hold`). Each new press switches the mode and writes `window_mode` through the settings store; `resolution` stays unwritten while the desktop size is in use. The window has a 400x240 minimum size.
- What changes at defaults: `chase-game` without `config/qol.cfg` starts in borderless full screen at the desktop size instead of a 960x600 window. The test build writes `build/config/qol.cfg` with `window_mode windowed` and `resolution 960x600` so that the screenshot tests keep their size.
- Decided by the implementation, not the user: `--window WIDTHxHEIGHT` starts windowed at that size and wins over the file (QOLSET-29).

Open (not decided by the user): QOLSET-28 whether the resolution applies in borderless full screen (implemented: the window covers the desktop and the resolution is used for the windowed size only); QOLSET-29 the rule that `--window` wins over `window_mode` and `resolution`.

<a id="ts-20261029-115310"></a>

## 2026-10-29T11:53:10+01:00 — Answers on the window defaults, resolution in full screen and --window

Asked of the user by the lead on 2026-10-29; the quoted labels are the user's selections. Answers the open points of [§2026-10-29T11:45:52](#ts-20261029-114552).

- Default with no `config/qol.cfg` at all (QOLSET-19): "Fullscreen default": borderless full screen at the desktop size, also when the file is missing.
- Resolution in full screen (QOLSET-28): "Windowed only": the resolution sets only the windowed size; borderless full screen always uses the desktop size.
- `--window WIDTHxHEIGHT` (QOLSET-29): "Yes, option wins": the option overrides `config/qol.cfg` for that run.

Open (not decided by the user): nothing for these points.

<a id="ts-20261029-114620"></a>

## 2026-10-29T11:46:20+01:00 — Settings menu art, Language row, Controls tab implementation

Two user decisions of 2026-10-29 (quoted labels are the user's selections), then what the Controls tab work (20261029-qolview-C) implemented.

- Art and fonts of the settings menu drawing (QOLSET-22): "Options-page art": the menu uses the original Options-page art and font exactly as the 3DS draws them, scaled up. Which assets (tabs, rows, master button, window) remain for a spec round; the menu drawing stays open until then.
- Language row of the Game tab (QOLSET-25, UI-23): the row stays read-only until the Languages page is ported (the current plan). [Corrects §2026-10-29T10:31:01 in part](#ts-20261029-103101): the row opens no page for now.

Implemented for [§2026-10-29T11:06:51](#ts-20261029-110651) (`src/qol/controls_tab.*`, `src/qol/controls_file.*`, tests `qol-controls-tab`, `qol-controls-file`):
- Model: a taken input (same source and code, also on the same action) is refused with a message and the tab keeps waiting; "Reset all" asks for confirmation (`requestReset`, `confirmReset`, cancel declines); Escape cancels waiting and cannot be bound; keys and mouse buttons; at most two slots per action, a hold or toggle mode per slot (a new slot is `hold`, rebinding keeps the mode), clearing a slot.
- Adapter: `readBindingEntries` runs `chase::readBindings` (strict, no second parser) and names the codes back through a key-namer callback; `controlActions` lists all 14 actions in enum order, `Walk` and `DebugOverlay` last.
- Writer: `formatBindingEntries` / `writeBindingFile` validate every entry, write one `Action source Code mode` line each (spaces as `_`), check that `readBindings` accepts the text, and replace the file through a temporary file.

Open (not decided by the user):
- Gamepad buttons (QOLSET-30): `config/controls.cfg` has only the sources `key` and `mouse`; a `pad` source and the button names need a format decision. Until then the tab accepts `pad` only through a caller's validator and the writer rejects it.
- Comments and the wording of the refuse message (QOLSET-31): the writer writes the file fresh, so the comment lines of the shipped file are not kept; a key named `#` cannot be written (the reader treats `#` as a comment). Whether comments are preserved or regenerated is not decided. The message text ("key W is already used by Up") is a placeholder for the drawing.
- Defaults and saving (QOLSET-32): where "Reset all" takes its defaults from (the shipped file) and when the file is written (on leaving the tab, or at once) are not decided.
