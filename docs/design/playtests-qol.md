# QoL: playtests

What the user reported from playing the PC settings menu, with what an agent verified against the code and a measurement. Observations only; the specs the user approved are in [qol-settings.md](qol-settings.md) and [qol-settings-drawing.md](qol-settings-drawing.md). Feature file: [qol-settings-drawing](../plans/features/qol-settings-drawing.md).

<a id="ts-20261029-193101"></a>

## 2026-10-29T19:31:01+01:00 — PC settings menu opened from the slot page

**Played:** the build after round 20261029-qolhook (settings menu opened from the "PC Settings" bar of the save slot page). Lead's question to the user: "The entire menu is very bugged, is that normal?" The user's words: "Navigating to it feels weird and not dynamic, the button is very small below the save game buttons, it is not clear to me how i toggle and open it. Inside everything is also quite laggy and very low res and hard to see."

**Verified by the recorder** (code reading of `src/qol/menu_window_sdl.cpp`, `src/qol/menu_page.cpp`, `src/game_front.hpp`, and a timing and a picture of `drawMenuPage` on the menu model, 2026-10-29, build RelWithDebInfo; no emulator):

- **Button small and hard to find: confirmed.** The button is the slot page's unused third list bar, drawn at scale 0.5 with only the text "PC Settings": 114 by 25 pixels of the 320x240 bottom screen (about 36 % by 10 % of its size, 143 by 31 pixels in a 960x600 window), under the slot bars. It opens only by a left mouse click; no key and no pad button opens it (the user's decision, [qol-settings.md §2026-10-29T18:06:35](qol-settings.md#ts-20261029-180635)), and nothing on the page says so.
- **How to toggle is not clear: confirmed.** The page shows only the shortcut hints "Back (B)" and "Select (A)" with the pad letters; keyboard Enter, Escape, the arrows, the mouse tap circle and the wheel have no hint. On the Game tab the rows show only their label ("SFX", no value), because the menu opened from the slot page has no live game options (QOLSET-49, QOLSET-24); a volume or Invert camera row therefore does not show what it is set to.
- **Laggy: confirmed, cause found.** While the menu is open the loop presents every frame (vsync), but each change (every key, wheel step, click and each time the pointer enters a new hit area) sets `dirty`, and `FrontMenu::present` then redraws the whole page on the CPU: a new page canvas, the round scene, the shortcuts, the overlay, then a full-window blit and an upload of the whole window canvas to the GPU. Measured time of one such redraw (`drawMenuPage`, 10 runs averaged): 186 ms at 960x600, 268 ms at 1280x720, 595 ms at 1920x1080, 1059 ms at 2560x1440. That is 5 frames per second at best at the test window and about 1.7 at 1080p; at the default full screen on a normal monitor each keypress or hover freezes the menu for about half a second or more.
- **Not dynamic: confirmed, cause found.** The page is a still scene: the round is placed with its storyboards at their end, and while the menu is open the front end draws and updates nothing (stand-in QOLSET-S4), so there is no slide-in, no turning round, no highlight animation and no transition from the slot page; the menu replaces the whole window at once.
- **Low resolution: partly confirmed, different cause.** The page is not drawn at the 3DS size and stretched: the canvas has the window size and the page is laid out at the scale min(width / 320, height / 240) (3 at 1280x720, a 960x720 page with black bars left and right). The 3DS art (hexagon pictures, bars, panels and the bitmap font) is itself low resolution and is enlarged, so edges are soft and the glyphs blocky; this is how the original page is built, enlarged by 3 or more. The page uses only a 4:3 part of a wide window.

**Open:** none; the user decided the button, sharpness and motion on 2026-10-29: [qol-settings.md §2026-10-29T19:34:22](qol-settings.md#ts-20261029-193422).

<a id="ts-20261101-135042"></a>

## 2026-11-01T13:50:42+01:00 — New PC settings menu played

**Played:** the build after round 20261101-qolui3 (the new full-window settings menu wired in), reached during play with the master button on Improved through the hidden pause wheel.

The user's words: "The menu looks great. Just record new QoL design choices/open points: ESC should be consistent, and now close the game. The controls should generally be rehauled as a QoL feature. So the game plays like a typical PC game, and drives like a typical driving game on PC. (for this search how modern PC games control, then let me approve the design)".

- **Menu look: confirmed good** ("The menu looks great"); no bug reported.
- **Escape:** the user asks for consistent Escape behaviour; decided that it never closes the game: [qol-controls.md §2026-11-01T13:50:44](qol-controls.md#ts-20261101-135044).
- **Controls:** the user wants them overhauled as a QoL feature to PC conventions, on foot and in vehicles; the researched design was approved by the user the same day: [qol-controls.md §2026-11-01T13:50:47](qol-controls.md#ts-20261101-135047).

**Open:** none. Feature file: [qol-controls](../plans/features/qol-controls.md).
