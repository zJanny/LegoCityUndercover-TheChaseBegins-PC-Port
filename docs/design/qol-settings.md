# QoL: settings menu and toggles

The user's approved design of how quality-of-life features are switched on and off: the settings menu, the master toggle, the per-feature toggles and where the choices are saved. Plan: [qol.md](../plans/qol.md); rules: [qol.md](../rules/qol.md); feature file: [qol-settings-menu.md](../plans/features/qol-settings-menu.md).

## Current state

- Every QoL feature is switched in the settings menu, and the faithful version is priority one; the user's two rules: [§2026-10-28T09:21:31+01:00](#ts-20261028-092131).
- QoL is off by default; the master toggle has three states (Original, Custom, Improved) derived from the per-feature switches; choices live in `config/qol.cfg`, never in the game save: [§2026-10-28T09:21:31+01:00](#ts-20261028-092131).
- The settings menu opens from the Options entry of the pause menu and from the title screen, with no extra key; it holds the master toggle, one toggle per feature, window mode and resolution, and key bindings: [§2026-10-28T09:21:31+01:00](#ts-20261028-092131).
- QoL source code is separate from the port's code: `src/qol/`, port files get only listed hook points: [§2026-10-28T09:24:08](#ts-20261028-092408).
- Open: volumes (the original Options page has SFX, Music, Invert camera and Language, UI-4) were not selected by the user; layout and look of the menu: [§2026-10-28T09:21:31+01:00](#ts-20261028-092131).

<a id="ts-20261028-092131"></a>

## 2026-10-28T09:21:31+01:00 — Settings menu, master toggle and rules

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

The agent's proposal, which the user has not yet confirmed (the lead should confirm): QoL code in `src/qol/` as its own CMake library `chase_qol`, tests in `tests/qol/`; port files get only the link line, the include and hook calls listed by name; a CTest `qol-separation` guard (QOLSET-7). The user's words decide the principle; the directory name, the exact hook form and the guard are proposals.

Open: confirmation of `src/qol/`, `tests/qol/` and the hook form.
