# QoL: settings menu and toggles

The user's approved design of how quality-of-life features are switched on and off: the settings menu, the master toggle, the per-feature toggles and where the choices are saved. Plan: [qol.md](../plans/qol.md); rules: [qol.md](../rules/qol.md); feature file: [qol-settings-menu.md](../plans/features/qol-settings-menu.md).

## Current state

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
- Window mode and resolution are stored in `config/qol.cfg` under the keys `window_mode` and `resolution`; their value formats are open: [§2026-10-28T22:20:41](#ts-20261028-222041).
- Open: the value formats of `window_mode` and `resolution`, and whether they are written only in Custom; implementation details (menu look) follow the recorded decisions.

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
