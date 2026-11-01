# Controls and the input laboratory

`chase-input-lab` is a small program for testing keyboard and mouse controls. It shows every 3DS button, the D-pad and the lower touch screen. It is not the game.

```sh
cd build && ./chase-input-lab
```

- Press Escape to quit.
- `--config PATH` loads a different controls file.
- It reads `config/controls.cfg` in the current folder. CMake copies the file into `build/config/`, so run it from the `build` folder.

The [model viewer](model-viewer.md) and the [development runtime](commands.md#development-runtime) read the same controls file.

## Change the controls

Edit `config/controls.cfg` in the project folder. `chase-game` and `chase-viewer`, started from the project folder, read it directly. The input laboratory, started in `build`, reads the copy that CMake makes in `build/config/` when you build. `config/controls.default.cfg` next to it is the shipped copy; the settings menu reads it for "Reset all" and never writes it. Each line looks like this:

```
Action key|mouse|pad Code hold|toggle
```

For example, `A key Space hold` makes Space press the A button.

- **Actions:** Up, Down, Left, Right, A, B, X, Y, L, R, Start, Select, Walk, DebugOverlay, Fullscreen.
- **Walk** is not a 3DS button. The [development runtime](commands.md#development-runtime) uses it to tilt the stick halfway, because a keyboard cannot. **DebugOverlay** is also not a 3DS button: it shows or hides the [debug overlay](debug-overlay.md) of the development runtime. **Fullscreen** is also not a 3DS button: F11 (`Fullscreen key F11 hold`) switches `chase-game` between windowed and borderless full screen (see [Window mode](game-runtime.md#window-mode-and-size)). Keep it on `hold`. The other actions are 3DS buttons.
- **Key names:** SDL key names, with `_` instead of spaces, for example `Left_Shift`.
- **Mouse buttons:** `Left`, `Middle`, `Right`, `X1`, `X2`.
- **Gamepad buttons (`pad`):** SDL's game-controller button names in lower case: `a`, `b`, `x`, `y`, `back`, `guide`, `start`, `leftstick`, `rightstick`, `leftshoulder`, `rightshoulder`, `dpup`, `dpdown`, `dpleft`, `dpright`, `misc1`, `paddle1` to `paddle4`, `touchpad`. For example `A pad leftshoulder hold`. Any other name stops the program with an error. The default file has no `pad` line. `chase-game`, `chase-viewer` and `chase-input-lab` open the first connected game controller (also when it is plugged in later) and read its buttons; without a controller nothing changes. Analog sticks and triggers are not read.
- Lines starting with `#` are comments.

## How the controls behave

- You can give one action several bindings. Any of them works.
- **`hold`:** the button is pressed while you hold the key.
- **`toggle`:** each new press switches the button on or off.
- Pressing two opposite directions at once cancels them out.
- When the window loses focus, every button is released.
- The left mouse button acts as the touch screen inside the lower-screen area.
- The menus and buttons on the lower screen (map buttons, menus) are used by clicking them. There are no extra keys for them.
- In the [development runtime](commands.md#development-runtime) a left click that starts on the lower screen is a touch, not A. A left click on the upper screen is still A.

## Fast travel pad

In the [development runtime](commands.md#development-runtime), after you pressed A on the built helipad pad and the helicopter idles:

- Click the exit button on the lower screen (the hexagon with an arrow at the top centre of the lower screen) to leave the console. The six level buttons are locked.

## Map buttons

In the development runtime, on the lower screen during play:

- Click the characters button (top left) briefly: nothing happens yet.
- Hold the left mouse button on the characters button for more than half a second: the characters panel opens (dark, without the portraits yet). Click the characters button again to close it.
- The scan button (bottom left) is shown from mission 1-3 on; clicking it does nothing yet.

## Sound scanner

In the [development runtime](commands.md#development-runtime), while the scanner view is open:

- Click the back-arrow button at the lower left of the lower screen to leave the scanner. It works once the view has fully opened, like **B** (Q or right mouse). A press that starts elsewhere and slides onto it does nothing.
- The headphone button above it is greyed and does nothing.

## Pause menu

In the [development runtime](commands.md#development-runtime), while you play:

- **Start** (Return) opens the pause menu. The game stops.
- The lower screen shows Resume, Options, Credits, Exit and PC Settings on a wheel. Turn it with **left and right** (A and D, or the arrow keys), or click the button left or right of the middle one, or swipe sideways with the mouse.
- **A** (Space), or a click on the middle button, chooses the entry in the middle. Resume goes back to the game.
- **B** (Q or right mouse) or **Start** go back to the game.
- Options opens the options wheel: SFX, Music, Invert Camera and Language. Turn it like the pause wheel. On SFX and Music, **up and down** (W and S, or the arrow keys) change the volume from 0 to 10. On Invert Camera, up, down or **A** switch it on or off. **B** goes back to the pause wheel; after a change it first saves the options.
- PC Settings is the last entry (it shows the picture of Options). Turn the wheel left from Resume to reach it, then choose it with **A** or a click on the middle button. It opens the [PC settings menu](#pc-settings-menu) over the whole window; the game and the pause menu wait behind it. **Escape** (or the other ways to close it) returns to the pause menu.
- Language on the options wheel opens the language grid: nine flag tiles, none chosen at first. **Click** a flag to choose it, then click it again or press **A** to switch to that language; it is stored in the save file and you are back on the options wheel. **B** goes back without a change. The texts and pictures stay English.
- Credits and Exit do nothing yet. The numbers on the upper screen are not shown yet.

## PC settings menu

In the [development runtime](commands.md#development-runtime), on the slot list of the start screens:

- Click the small **PC Settings** bar under the two slots with the left mouse button. It slides in with the slots. No key or gamepad button opens it. During play the [pause menu](#pause-menu) has a PC Settings entry that opens the same menu, there without the Game tab.
- The menu covers the whole window with tabs across the top (Game, Display, Controls, Quality of life), the rows of the tab in the middle, a help line under them and a key-hint bar with a **Back** button at the bottom. It is drawn in its own font, sharp at the window's size. The start screens stop behind it; the game picture behind it is not shown yet (a black background).
- Keyboard: **Up and Down** move between rows, **Left and Right** change the value of the row. **Q and E** or **Page Up and Page Down** switch tabs; the last tab wraps to the first. **Up** on the first row moves onto the tab bar, where Left and Right switch tabs and Down goes back to the rows. **Enter** switches an On/Off row, opens the resolution list or presses a button. **Escape** closes the menu, or first closes an open list, prompt or dialog. Greyed-out rows are skipped.
- The menu opens again on the tab it was closed on.
- Mouse: moving onto a row or tab selects it; a click on a tab switches to it; click the `<` and `>` arrows or the value to change a choice; click or drag a slider; the wheel scrolls the rows; a **right-click** or a click on **Back** goes back.
- Gamepad: **leftshoulder / rightshoulder** switch tabs, the **D-pad** moves, **a** selects, **b** goes back.
- These keys are fixed. They do not follow `config/controls.cfg`.
- Game tab: SFX volume and Music volume (0 to 100 in steps of 10; the sound changes at once), Invert camera (On or Off), Language (`< English >`), and Reset to defaults. The options are stored in the save file when you close the menu, if you changed one. A new language is passed on when the menu closes, but the game does not switch to it yet (a line in the terminal says so).
- Display tab: Window mode (Windowed or Fullscreen) and Resolution (a list of the monitor's sizes; greyed out as "Desktop resolution" in full screen). A change applies at once and asks "Keep these settings?": **Enter** or Keep keeps it, **Escape**, Revert or 15 seconds without an answer go back to the old setting.
- Controls tab: one row per action with the columns Action, Slot 1, Slot 2 and Mode. **Left and Right** move between Slot 1, Slot 2 and Mode, **Enter** or a click chooses it. A slot shows "Press a key... (Esc cancel, Del clear)": press the new key, mouse button or gamepad button (a gamepad button only while a gamepad is connected), **Delete** clears the slot, **Escape** cancels. The menu keys cannot be bound. A key that another action uses asks "Swap" or "Cancel": **Enter** cancels, **Left and Right** move between Swap and Cancel and Enter presses the chosen one; a click on Swap swaps the two bindings. Mode switches the action between Hold and Toggle. `config/controls.cfg` is written after each change and the running game uses it at once.
- Quality of life tab: the preset (Original, Improved, Custom) and one On/Off row per feature. Changing a feature by hand switches the preset to Custom.
- The help line under the rows says what the focused row does and its default. Every tab ends with a **Reset to defaults** button, which asks first (Yes or No). The question opens on No: **Enter** declines, **Left** moves to Yes.
- The Display and Quality of life tabs store their settings in `config/qol.cfg` at once (see [window mode and size](game-runtime.md#window-mode-and-size)).

## Award viewer

In the [development runtime](commands.md#development-runtime), in the police station:

- Stand on the award viewer's use marker and press **A** (Space). The awards screen opens and the game stops.
- The lower screen shows Overview and the seven areas on a wheel. Turn it like the pause menu: **left and right** (A and D, or the arrow keys), a click on the button left or right of the middle one, or a swipe sideways with the mouse.
- The upper screen shows how many bronze, silver and gold awards you have of the area in the middle.
- **B** (Q or right mouse) goes back to the game. Start does nothing here. Clicking the Back button on the lower screen does not work yet.

## Attacking

In the [development runtime](commands.md#development-runtime), on foot:

- A short press of **Y** punches. Chase attacks when you let go of the button. A press held longer than about a third of a second does nothing.
- Chase turns toward the nearest smashable prop in front of him, such as a table, and hits it.
- Press **Y** again near the end of a punch to chain up to four punches.

## Driving a vehicle

In the [development runtime](commands.md#development-runtime), once the character sits in a vehicle:

- **A** accelerates. **B** brakes, and reverses when the vehicle stands still.
- The **stick** steers. The **left and right D-pad** (A and D, or the arrow keys by default) steer fully to the left or right.
- **X** jumps out again. **Y** (the horn) does nothing yet.
- Pressing A and B together brakes.

## Detective mode

In the [development runtime](commands.md#development-runtime), on a detective point of a mission:

- **A** takes out the detective tool: the character holds up a magnifying glass. He walks slowly and cannot jump; footprints appear ahead.
- Walk along the footprints. Facing the X at their end puts the tool away; **A** on the X reveals what is hidden.
- **A** away from any usable object puts the tool away. Moving far from the footprints does the same.

## Default controls

| Button | Default |
|---|---|
| D-pad | WASD or the arrow keys |
| A | Space or left mouse |
| B | Q or right mouse |
| X | E |
| Y | R |
| L | Z |
| R | C |
| Start | Return |
| Select | Tab |
| Walk (not a 3DS button) | Left Alt |
| Debug overlay (not a 3DS button) | F3 (toggle) |
