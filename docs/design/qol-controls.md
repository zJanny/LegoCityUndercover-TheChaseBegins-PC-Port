# QoL: PC controls

How the game is controlled on a PC: what Escape does everywhere, and the overhaul of the on-foot and driving controls so that the game plays like a typical PC game and drives like a typical PC driving game. Feature file: [qol-controls](../plans/features/qol-controls.md). The key bindings menu is in [qol-settings-drawing.md](qol-settings-drawing.md); the 3DS-to-keyboard mapping that stays the faithful default is in [input](../plans/features/input.md).

<a id="ts-20261101-135044"></a>

## 2026-11-01T13:50:44+01:00 — Escape means back, close, pause or skip, and never closes the game

The user's words, after playing the new settings menu ([playtests-qol.md §2026-11-01T13:50:42](playtests-qol.md#ts-20261101-135042)): "ESC should be consistent, and now close the game" (read together with the answer below as: Escape must not close the game).

The lead asked what Escape does on the game window. The user chose **"Never closes the game"** (2026-11-01), option text: "Esc always means back/close menu/pause, like in typical PC games; quitting goes through a menu entry."

Decided (the user, 2026-11-01):
- Escape never closes the game, in any state. In gameplay it opens the pause menu (it is also bound to Start, Keypad Enter second); in a menu or dialog it goes back or closes it (the settings menu already follows this: [qol-settings-drawing.md §2026-10-30T21:56:04](qol-settings-drawing.md#ts-20261030-215604)).
- Quit entry, "PC Settings + title (Rec.)": a "Quit game" row with Yes/No on the PC Settings Game tab, and the same on the title and save slot screens. The original menus stay unchanged. Closing the window still ends the program.
- Quit game in the front end (the user, 2026-11-01, "Only via PC Settings (Recommended)"): the front end has no title screen with entries, so Quit game is reached there only through the PC Settings menu opened from the slot page (Game tab, Quit game row, Yes/No, mouse and keyboard); no new button on the slot page art. The screens before the slot page (logos, movie, language list, prompt pages) can be left only by closing the window.
- Quit game in play (the user, 2026-11-01, "Game tab, Quit only (Recommended)"): the PC Settings menu opened from the pause wheel also has a Game tab, but it shows only the Quit game row, with no volume, Invert camera or Language rows and no Reset button, because the pause wheel's own Options page holds those; no save is touched.
- Movies, cutscenes, logos and the title, "Skips if skippable (Rec.)": Escape acts like the original skip where the original has one, and does nothing else.
- Original mode, "Everywhere (Rec.)": the Escape rules apply with the master button on Original too (so the development quit on Escape in `chase-game` goes away in every mode).
- Developer tools, "Game only": `chase-viewer`, the FMV player and `chase-input-lab` keep Escape = quit.

Open: none.

<a id="ts-20261101-135047"></a>

## 2026-11-01T13:50:47+01:00 — Controls overhauled to PC conventions

The user's words: "The controls should generally be rehauled as a QoL feature. So the game plays like a typical PC game, and drives like a typical driving game on PC. (for this search how modern PC games control, then let me approve the design)". A research agent looked up how modern PC games control; the user approved its proposal with the recommended defaults on 2026-11-01.

**Approved design** (the user, 2026-11-01). No new game behaviour beyond the listed choices.
- Switch: "Own toggle, on in Improved": "PC controls" is its own toggle on the Quality of life tab, on in Improved, off in Original (it is not a Custom-only extra; the Escape rules above apply in both modes).
- On foot: W/A/S/D and the arrows move; Left Ctrl held = walk (Left Alt second); Space = jump (B); E = interact and confirm (A, Return second); left mouse button = punch (Y, R second); F = enter or leave a vehicle (X); Z and C turn the camera; middle mouse button = re-centre the camera (Z and C together second); Esc = pause (Start, Keypad Enter second); Tab = Select. Lower-screen menus are used by clicking them.
- In a vehicle: W or Up accelerate (A); S or Down brake and reverse (B); A and D steer; Space = brake (B); H = horn (Y); F = leave (X). Hand brake and siren stay unbound because the game has no player input for them.
- Q1 mouse camera, "Mouse X turns (Recommended)": mouse X feeds the existing camera turn (yaw only, analog size, no pitch). The cursor is captured in play and released in menus and on the lower screen.
  - Sensitivity (the user, 2026-11-02, "Stand-in + setting"): a fixed default now, labelled stand-in QOLCTL-S5; a sensitivity slider in PC Settings comes later as its own item (QOLCTL-15), not built yet.
  - Reaching the lower screen while the cursor is captured (the user, 2026-11-02, "Virtual pointer"): a hidden pointer is tracked from the relative motion; moving it down into the lower-screen rectangle releases the capture and puts the real cursor there; moving the cursor back out of that rectangle captures again.
  - Details of the mouse turn (the user, 2026-11-02, "Accept all four"): mouse right turns the camera like R; the Invert camera option flips the mouse direction too, like the shoulder buttons; the turn speed has no cap; a held mouse button never starts the capture (a touch drag keeps working).
- Q2 driving, "Vehicle bindings (Recommended)": the bindings depend on whether the player sits in a vehicle.
- Q3 Space in a vehicle, "Space = brake (Recommended)".
- Q4 mouse buttons on foot, "LMB punch, MMB recentre": left button punches, right button is unused, middle button re-centres; a left click on the lower screen still counts as touch.
- Q8 prompts, "Keep 3DS icons (Rec.)": the game keeps its 3DS button icons; key names appear only in the Controls tab.
- Q9 walk key, "Hold Left Ctrl (Rec.)", Left Alt second.
- Q10 camera keys, "Keep Z / C (Rec.)".
- Q11 gamepad analog, "Later step (Rec.)": out of this feature ([INPUT-1](../plans/features/input.md)).
- Rebinding, "Default set, rebindable": with PC controls on the PC layout is the default set; the Controls tab rebinds on top of it and "Reset to defaults" restores it.
- Where rebinds live (the user, 2026-11-01, "Separate file (Recommended)"): with PC controls on, the keys are read from and written to `config/controls.pc.cfg`, created from the built-in PC set when absent; with it off `config/controls.cfg` is used exactly as before. "Reset to defaults" restores the PC set when on and `config/controls.default.cfg` when off. Switching the toggle never changes the other file.
- Help text and file key (the user, 2026-11-01, "Approve as is"): the help text of the "PC controls" row is "Keys and mouse like a PC game: WASD, Space, E, F." followed by "Default: Off", and the `config/qol.cfg` key is `pc_controls`.
- Return key (the user, 2026-11-01, "Return = confirm (A)"): Return is A's second key next to E; pause (Start) is Escape only, with Keypad Enter as Start's second.

Open: none.
