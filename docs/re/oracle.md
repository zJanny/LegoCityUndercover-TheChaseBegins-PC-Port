# Runtime oracle (Azahar)

`tools/azahar_oracle.py` runs the user's own game copy in the Azahar 3DS emulator (2126.2) and observes it through the emulator's GDB stub. It measures the original game; it is not part of the port. Scenario commands are listed in the tool's docstring.

This file is the entry point: setup and the fast workflow are below. New dated entries go to the subtopic file that matches them (list below), not here.

Subtopics:
- [oracle-setup.md](oracle-setup.md): launch with Xvfb, the render-window crash and launch retry, `--display desktop`.
- [oracle-rpc.md](oracle-rpc.md): the RPC channel (`--rpc`, `--rpc-input`), memory access, screenshot and timing measurements.
- [oracle-routes.md](oracle-routes.md): the route to gameplay, scenario pitfalls (`teleport`, input that survives `loadstate`).
- [oracle-measuring.md](oracle-measuring.md): watchpoints, execution traps, fixed dt, memory traces, `poke`.

## Current state

- Run the emulator only through `tools/azahar_oracle.py` (Azahar 2126.2, GDB stub, isolated XDG home under `artifacts/oracle/<home>/`, one port per emulator); Old 3DS mode is the default. [§2026-10-13T11:50:30](#ts-20261013-115030)
- Start from save states (`--display xvfb`, `--state NAME`, `savestate NAME`, `loadstate NAME`): quick save is F1 and quick load F12, files in `artifacts/oracle/states/`; a state saved or loaded in the first 15 s after boot is refused. [§2026-10-13T11:50:30](#ts-20261013-115030)
- Per-frame stepping (`frames`, `trace`) inflates the game clock by about 30 percent; measure rates in free-run intervals (`wait`) and use `--rpc` for timing. `--frame-limit 0` is for navigation only. [§2026-10-13T11:50:30](#ts-20261013-115030)
- Screenshots need the software renderer and take about 11 s through the stub. [§2026-10-13T11:50:30](#ts-20261013-115030)
- `--keep` leaves the emulator running, `--attach --port N` reconnects, `--stop` ends it. [§2026-10-13T11:50:30](#ts-20261013-115030)
- Every session gets a virtual motion source (Cemuhook DSU, level pose); without it the scanner yaw is NaN. Details: [oracle-setup.md §2026-10-18T22:03:32](oracle-setup.md#ts-20261018-220332).
- Setup (including the console mode of save states), RPC, routes and measuring aids are in the subtopic files listed above; new dated entries go there.
- Open: whether a state saved during async loading is accepted, the exact boot time of the "pending operations" refusal, and slot names for other titles.

<a id="ts-20261013-115030"></a>

## 2026-10-13T11:50:30+02:00 — Setup, fidelity rules and the fast workflow

**Setup**
- `azahar` in `PATH`. Azahar is started as `azahar -g PORT <image>` with an isolated XDG home under `artifacts/oracle/<home>/`. The tool writes `config/azahar-emu/qt-config.ini` itself; the user's own config is never read or changed.
- Platform: `QT_QPA_PLATFORM=offscreen` by default. `--display xvfb` starts a private `Xvfb` and runs Azahar on the xcb platform (needed for save states, see below).
- With `-g` the emulator halts at entry 0x100000 until a gdb client connects. The client breaks on the scheduler 0x100f44 (one stop per game update) and on the pad reader (input injection).
- Ports used by agents: one port and one home per concurrent emulator. `--home` and `--port` select them.

**Fidelity rules**
- Azahar ignores a config value whose `\default` flag is `true`. Every generated key therefore carries `key\default=false`. (A first run was accidentally in New 3DS mode because of this.)
- Old 3DS mode (`is_new_3ds=false`) is the oracle default.
- Per-frame breakpoint stops (`frames`, `trace`) inflate the game clock about 30 % against vblanks (free run 51.3 ticks per vblank, stepped 65.9). Measure rates in free-run intervals (`wait`).
- Screenshots need the software renderer (it writes frames to emulated VRAM). With the OpenGL renderer VRAM is stale. The software renderer delivers vblanks late.
- `frame_limit=0` (`--frame-limit 0`) makes the game clock run about 2000 per vblank instead of 50. Use it for navigation only, never for timing measurements.
- A screenshot reads about 576 KB through the stub (2 KB per packet) and takes about 11 s.

**Fast workflow**
- Boot once, save a state at each checkpoint, load a state per experiment. Use `--keep` and `--attach` for interactive exploration.
- Reaching the language screen: free-running is far faster than stepping. `wait 25` reaches it in about 35 s; stepping `frames 600` takes about 4 minutes, because every game update costs several stub round trips. The frame counter only counts stepped frames, so after a `wait` it starts at 0.
- `--keep` leaves the emulator running. The gdb client detaches with `D`; the tool waits for the stub's reply before closing the socket (closing at once kills Azahar). The PID and port are printed and recorded in `<home>/session.json` together with the Xvfb PID and display.
- `--attach --port N` connects to a kept emulator (no image needed) and detaches again afterwards. The frame counter of every attach starts at 0. `--stop` kills the kept emulator and its Xvfb.
- `--display xvfb` plus scenario commands `savestate NAME` and `loadstate NAME`, and `--state NAME` (load right after boot).

**Save states**
- Azahar has no CLI flag for them. The tool binds the hotkeys `Quick Save` = F1 and `Quick Load` = F12 in the generated config (`[UI]` keys `Shortcuts\Main%20Window\Quick%20Save\KeySeq=F1` and `...\KeySeq\default=false`; the shortcut names in this version are "Quick Save", "Quick Load", "Save to Oldest Non-Quicksave Slot", "Load from Newest Non-Quicksave Slot") and sends the key with XTEST (`libXtst.so.6` through ctypes, no xdotool).
- Avoid F10 (Toggle Screen Layout) and F13/F14 (not mapped in the Xvfb keymap): a conflicting or unknown key does nothing.
- The quick-save file is `<home>/data/azahar-emu/states/<TITLEID>.00.cst` (title 00040000000AD600). `savestate NAME` copies it to `artifacts/oracle/states/NAME.cst` and writes `NAME.json` (file name, frame counter), so states survive `--fresh`. `loadstate` copies it back and presses the load key. A state is 3.5 MB early in the boot and 4.4 MB at the language screen.
- A load or save while the game is still starting fails: Azahar opens a modal "Savestates" dialog (X window title) and the key does nothing. The tool detects that window, dismisses it with Return and retries every 5 s for up to 2 minutes. About 15 s after boot loads succeed.
- The gdb client is detached while the key is pressed and reconnected afterwards. After a load the breakpoints work again, `report` shows the saved scheduler and clock values (language screen: C=0x514378, vtable 0x363cd0, clock 703618 right after the load), and the software-renderer screenshot shows the restored screen.
- Pitfall found on the way: a late free-running boot reaches the language screen by itself, so a "loaded" check must compare against the state's own values, not just the screen.

**Measured wall times** (this machine, software renderer, Old 3DS)
| Step | Time |
|---|---|
| Cold boot, `frames 600` to the language screen (plus savestate and screenshot) | 229 s |
| Cold boot, OpenGL, `--frame-limit 0`, `frames 600` | 194 s (clock distorted: 158 million ticks) |
| Cold boot, free-run `wait 25`, `frames 2`, report, screenshot | 37.5 s (screenshot 11 s) |
| `--state` at the language screen until `report` works (whole run incl. launch) | 13.5 s |
| `--attach` to a kept emulator, `report` | 0.4 s |
| `--attach` to a kept emulator, screenshot | 11 s |

Unthrottled speed barely helps stepped navigation: the stub round trips dominate, not the emulation speed.

Unknowns:
- Whether a state saved while the game is in a loading phase (async file reads) is accepted; the dialog text was not captured.
- Which exact boot time the "pending operations" refusal ends at (observed: refused at about 10 s, accepted at about 15 s after launch).
- Whether the quick-save slot file name differs for other titles; only title 00040000000AD600 was tried.
