# Debug overlay

The debug overlay is a developer tool of the [development runtime](game-runtime.md) (`chase-game`). It shows how fast the program runs and a few values of the running level. It is not part of the original game and it does not change the game.

## Use it

- It is **off by default**. While it is off, nothing is drawn and the game update does not change.
- Press **F3** in the game window to show it. Press F3 again to hide it.
- Start with it shown: `build/chase-game --debug-overlay`. Then F3 hides it, and F3 again shows it.
- It appears as a text panel in the top-left corner of the window. In a screenshot (`--screenshot`) it is part of the picture.
- It works only in a level, not on the front-end screens (logos, language, save slots, movie).
- When the window loses focus, the key state is reset, as for every binding. A key that switched the overlay on is then released, so the overlay goes off.

## Change the key

The key is the action `DebugOverlay` in `config/controls.cfg`. The default line is:

```
DebugOverlay key F3 toggle
```

- Use another key: `DebugOverlay key F4 toggle`. Key names are SDL names, see [Controls](controls.md).
- Use `hold` instead of `toggle` to show the overlay only while you hold the key.
- You can bind a mouse button the same way, for example `DebugOverlay mouse X1 toggle`.
- Without a `DebugOverlay` line there is no key. The overlay can still be started with `--debug-overlay`.

## What it shows

The panel shows one line each:

| Line | Meaning | Unit | How it is measured |
|---|---|---|---|
| `FPS` | Frames drawn in the last second | frames per second | The number of main loop passes that ended within the last second (window of 1.0 s, steady clock). |
| `FRAME MS LAST` | The last frame | milliseconds | Time between the start of the previous and the start of this pass of the main loop. With the window's vertical sync this includes the wait for the display. |
| `AVG` | Average frame time | milliseconds | Mean of the frame times in the same 1.0 s window. |
| `MIN`, `MAX` | Shortest and longest frame time | milliseconds | Smallest and largest frame time in the same window. |
| `MEM MB` | Resident memory of the process now | mebibytes (1024 x 1024 bytes) | `VmRSS` in `/proc/self/status`, read once per second. |
| `PEAK` | Highest resident memory since the process started | mebibytes | `VmHWM` in `/proc/self/status`, read once per second. |
| `CPU` | CPU use of the process | percent | CPU time (user plus system, from `getrusage`) divided by wall time, over about one second. **100 % is one full core.** All threads of the process count, so the value can exceed 100 %, for example with audio or driver threads or a software renderer. Shows `...` for the first second after the overlay appears. |
| `UPDATES/S` | Game updates in the last full second | updates per second | The count of game logic updates (the fixed update rate of the recovered scheduler), the same number the program prints once a second. `0` for the first second. |
| `LEVEL` | Level folder name | text | The loaded level, for example `lvl_01_cherrytreehills`. |
| `POS` | Player position | game units, X Y Z | The player's current position. `NO PLAYER` if the level has no player start. |
| `HEADING` | Player heading | degrees | The player's facing angle, converted from the game's 16-bit heading. |
| `SCRIPT THREADS` | Running level script threads | count | Number of threads of the level script runner now. |
| `ROOMS LOADED` | Rooms of the level that are loaded now | count | The rooms the room streaming rule keeps loaded around the player (the current room, its neighbours and up to three preloads). Not shown in levels without room streaming. A room that is requested counts only once its loading has finished. |

## Limits

- The memory and CPU values come from Linux (`/proc`, `getrusage`). On another system they are not available. The overlay is only supported on Linux, like the rest of the port.
- The numbers are for the whole process, not for the GPU. No GPU time or draw call count is shown.
- Reading `/proc` and drawing the panel cost a little time, well under a millisecond per frame. The overlay is only read and drawn while it is visible. Frame times are recorded all the time (one clock read per frame).
- The text uses a small built-in font with capital letters, digits and a few signs.

## Tests

See [Commands](commands.md): `debug-metrics-core`, `debug-text-core`, `debug-overlay-off`, `debug-overlay-on`, `debug-overlay-area`, `game-option-debug-overlay-value`, and the binding checks in `input-core`.
