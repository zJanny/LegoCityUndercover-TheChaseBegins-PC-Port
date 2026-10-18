# Controls and the input laboratory

`chase-input-lab` is a small program for testing keyboard and mouse controls. It shows every 3DS button, the D-pad and the lower touch screen. It is not the game.

```sh
cd build && ./chase-input-lab
```

- Press Escape to quit.
- `--config PATH` loads a different controls file.
- Run it from the `build` folder, because it looks for `config/controls.cfg` there.

The [model viewer](model-viewer.md) and the [development runtime](commands.md#development-runtime) read the same controls file.

## Change the controls

Edit `config/controls.cfg`. Each line looks like this:

```
Action key|mouse Code hold|toggle
```

For example, `A key Space hold` makes Space press the A button.

- **Actions:** Up, Down, Left, Right, A, B, X, Y, L, R, Start, Select, Walk, DebugOverlay.
- **Walk** is not a 3DS button. The [development runtime](commands.md#development-runtime) uses it to tilt the stick halfway, because a keyboard cannot. **DebugOverlay** is also not a 3DS button: it shows or hides the [debug overlay](debug-overlay.md) of the development runtime. The other actions are 3DS buttons.
- **Key names:** SDL key names, with `_` instead of spaces, for example `Left_Shift`.
- **Mouse buttons:** `Left`, `Middle`, `Right`, `X1`, `X2`.
- Lines starting with `#` are comments.

## How the controls behave

- You can give one action several bindings. Any of them works.
- **`hold`:** the button is pressed while you hold the key.
- **`toggle`:** each new press switches the button on or off.
- Pressing two opposite directions at once cancels them out.
- When the window loses focus, every button is released.
- The left mouse button acts as the touch screen inside the lower-screen area.

## Driving a vehicle

In the [development runtime](commands.md#development-runtime), once the character sits in a vehicle:

- **A** accelerates. **B** brakes, and reverses when the vehicle stands still.
- The **stick** steers. The **left and right D-pad** (A and D, or the arrow keys by default) steer fully to the left or right.
- **X** jumps out again. **Y** (the horn) does nothing yet.
- Pressing A and B together brakes.

## Detective mode

In the [development runtime](commands.md#development-runtime), on a detective point of a mission:

- **A** takes out the detective tool. The character walks slowly and cannot jump; footprints appear ahead.
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
