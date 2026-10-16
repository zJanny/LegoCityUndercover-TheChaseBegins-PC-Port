# Runtime oracle: Launch, display and crash handling

Subtopic of the Azahar runtime oracle. Start with [oracle.md](oracle.md) for setup and the fast workflow. Entries are dated; new entries go at the end.

Launching Azahar for the oracle: Xvfb, the render window race, the launch retry, and the visible desktop mode.

## Current state

- Launch under Xvfb: the software renderer can abort on a state load (a paint race between the Qt thread and the state load, Azahar 2126.2); the tool runs with `singleWindowMode=false` and unmaps the render window, after which 8 of 8 runs and 6 x 8 loads succeeded. [§2026-10-13T12:41](#ts-20261013-124127)
- The tool relaunches at most 3 times when the emulator dies before the scenario starts, saves `crash<N>-*` logs, and kills only the process groups of the emulator and Xvfb it started. [§2026-10-13T12:41](#ts-20261013-124127)
- `--display desktop` runs on the current X display for a person to watch; agents keep using `xvfb`, and `--rpc-input` must not be combined with it. [§2026-10-13T22:45](#ts-20261013-224508)
- Default keys of the generated config: circle pad arrows, A = A, B = S, X = Z, Y = X, L = Q, R = W, D-pad T/F/G/H, Start = M, Select = N, F1 quick save, F12 quick load. [§2026-10-13T22:45](#ts-20261013-224508)
- Open: crashes on a plain boot, whether an unmapped window changes timing, the cause of one launch that ended after 4 loads, crashes of a visible OpenGL window.

<a id="ts-20261013-124127"></a>

## 2026-10-13T12:41:27+02:00 — Azahar abort on quick load under Xvfb, launch retry and cleanup

**Symptom:** about 1 in 4 launches with `--display xvfb` and the software renderer died with SIGABRT a few seconds after the quick load, which looked like a startup crash (`ConnectionRefusedError`/`ConnectionResetError`, defunct azahar, running Xvfb).

**Cause (log and source, Azahar 2126.2):** the stack in `azahar_log.txt` is `SoftwareRenderWidget::paintEvent` (bootmanager.cpp:379) -> `Present()` draw lambda -> `unique_ptr::operator*` on null. The `ArchiveManager` in the symbol is an LTO-merged `unique_ptr` dereference; the pointer is most likely the `System::gpu` reached through `system.GPU().Renderer()` (not confirmed which one). The log line before the abort is `Begin load of slot 0` (core.cpp:195), 35-75 ms earlier in the one crash log kept. The state load runs `Shutdown(true)` (core.cpp:766), which sets `is_powered_on=false`, then `gpu.reset()` and `archive_manager.reset()`, and `Init` rebuilds them on the emulation thread. The GUI thread paints the software widget in a loop (`paintEvent` calls `update()`); `Present` tests `IsPoweredOn()` once (a plain `bool`, no lock) and dereferences `GPU()` later, after the top screen was scaled. A paint that passed the check before the teardown crashes. So this is a data race between the Qt thread and a state load, not an initialisation-order problem at startup.

**Measurement** (one launch, repeated `loadstate` of `fast-lang`, `artifacts/oracle/fast-home`): default config, loads completed before the abort per launch: 7, 2, 1, 2, 1 (and 3, 2, 2, 1 with `QT_QPA_UPDATE_IDLE_TIME=60000`, which has no effect). Render window unmapped (below): launches of 8 loads each: 8, 4 (aborted after 4, log not kept, cause not confirmed), 8, 8 and then 6 x 8 without an abort.

**Adopted mitigation (`tools/azahar_oracle.py`, `--display xvfb` only):**
- Config `[UI] singleWindowMode=false` with `\default=false` (with `\default=true` Azahar ignores the value), so the render widget is its own top-level window titled `... | Primary Window`.
- After both windows exist the tool calls `XUnmapWindow` on it and `XSetInputFocus` on the main window. Qt then gets no expose events, so `paintEvent` does not run. The quick save/load shortcuts are application shortcuts (`Context=2`) and still fire from the main window. The oracle never reads the host window (frames come from emulated VRAM through GDB/RPC).
- Verification scenario (`hold A`, `frames 3`, `channels`, state `fast-lang`, `--rpc --rpc-input`): 8/8 and 8/8 runs succeeded with 0 retries; ch16 = 1.0 as before. Before the change, with only the retry code, 1 of 8 runs needed a relaunch.
- Not adopted: `QT_QPA_UPDATE_IDLE_TIME` (no effect); a smaller window (does not remove the race).

**Launch robustness:** the startup phase (launch, window silencing, GDB connect, RPC connect, `--state` load) watches the emulator process. If it exits before the scenario starts, the tool saves the logs as `crash<N>-azahar_log.txt` and `crash<N>-azahar-stdout.log` next to the originals, prints one line with the signal (naming the known crash when the log holds both the `SoftwareRenderWidget::Present` and `ArchiveManager` frames), kills and reaps its own Xvfb and emulator and relaunches, at most 3 launches. Nothing is retried once the scenario has begun. On any exit (exception, Ctrl-C, SIGTERM, SIGHUP) the tool kills the process groups of the emulator and Xvfb it started (both are session leaders, so `killpg` cannot touch other processes) and waits for them; nothing is matched by name, so another emulator on another port is never touched. A SIGTERM during a run left no azahar, Xvfb or defunct process. Tests: `tests/test_azahar_oracle_launch.py`.

Unknowns:
- Whether Azahar can crash the same way during a plain boot without any state load (not seen).
- Whether an unmapped render window changes emulation timing (frame pacing was not compared).
- The cause of the one launch that ended after 4 loads with the render window unmapped (its log was overwritten); it may be a different race.

<a id="ts-20261013-224508"></a>

## 2026-10-13T22:45:08+02:00 — Visible mode for the user (--display desktop)

- `--display desktop` runs Azahar on the current X display (`DISPLAY`, here XWayland :0) with the same isolated home, generated config and Quick Save/Load hotkeys as `--display xvfb`; the render window is not unmapped. Meant for a person to watch and play (user request); agents keep using `xvfb`. Do not combine it with `--rpc-input` (that patch replaces the pad reader, so the keyboard would stop working).
- Verified: `--display desktop --renderer opengl --state gameplay-start --keep --port 24690 --home artifacts/oracle/desktop-home` loaded gameplay-start (frame 30) and left the emulator running visibly; `tests/test_azahar_oracle_*.py` 23 OK.
- Default Azahar keys of the generated config: circle pad = arrow keys, A = A, B = S, X = Z, Y = X, L = Q, R = W, D-pad = T/F/G/H, Start = M, Select = N; F1 quick save, F12 quick load.

Unknowns:
- Whether the known software-renderer paint crash during state loads also hits a visible OpenGL window (not seen in the one run).
