# Runtime oracle (Azahar)

`tools/azahar_oracle.py` runs the user's own game copy in the Azahar 3DS emulator (2126.2) and observes it through the emulator's GDB stub. It measures the original game; it is not part of the port. Scenario commands are listed in the tool's docstring.

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

## 2026-10-13T12:02:21+02:00 — RPC channel (non-halting memory access, breakpoint-free pad input)

Azahar 2126.2 has an RPC server (source: `src/core/rpc/` of the Azahar source, `/usr/src/debug/azahar/azahar-unified-source-2126.2/`). The tool uses it as a second channel next to the GDB stub: `--rpc` (reads, screenshots, `sample`, `perf`) and `--rpc-input` (pad buttons without per-frame breakpoints). Tests of the client: `tests/test_azahar_oracle_rpc.py` (fake UDP server).

**Enabling**
- Setting `enable_rpc_server`, section `[Debugging]` (`citra_qt/configuration/config.cpp` `ReadDebuggingValues`). The generated config carries `enable_rpc_server=true` and `enable_rpc_server\default=false`. `core/core.cpp` creates the server at the end of `System::Init` only if the setting is on (and `ENABLE_SCRIPTING` was compiled in; the Arch build was).
- The UDP port is hard-coded: `udp_server.cpp` binds `0.0.0.0:45987` ("TODO: Make configurable"). Not configurable, so two emulators cannot both have RPC. If the bind fails, `Server` catches the exception, logs "Error starting UDP server" and runs without RPC, with no other sign. The tool therefore refuses `--rpc` when UDP 45987 cannot be bound by the tool itself, and `--rpc` is off by default. A running RPC emulator also answers any client on that port: do not attach a second tool instance to the wrong emulator.
- The log line in `azahar_log.txt` says `Starting RPC server (synchronous)`: the build has `ENABLE_SCRIPTING_SYNC`. Every request except `ReadScreenshot` is queued and executed by the emulation thread (`ProcessCoreRequests`, `core/core.cpp`), so reads are consistent (no torn words) and it works while the GDB stub has the CPU halted (verified).

**Packet format** (`packet.h`, `udp_server.cpp`, `rpc_server.cpp`)
- One UDP datagram per request and per reply: header of four little-endian u32 `version (=2), id, type, size`, then `size` payload bytes. The datagram length must be exactly 16 + size, else it is dropped without reply (no reply at all). Request size is at most 16 + 32768. The reply echoes version, id and type with its own size.
- A request with size < 8, a wrong version or an unknown type gets an empty reply (size 0). Every request type has two u32 arguments first.
- Types: 1 ReadMemory (addr, size; size 1..32768; reply = the bytes), 2 WriteMemory (addr, size, then data; size 1..32760; reply is empty), 3 ProcessList (start, max; reply u32 count + count * 20-byte packed `{u32 pid, u64 title, char name[8]}`), 4 SetGetProcess (op 0 = get, reply u32 selected pid; otherwise set pid), 5 TakeScreenshot (res_scale, flags; flags bit0 secondary window, bit1 raw RGB; reply `{u32 result, width, height, data_size}`), 6 ReadScreenshot (offset, size; up to 32768), 7 GetPerfStats (mode 0 last / 1 get-and-reset; reply 10 f64 + u32 = 0x54 bytes).
- Reads and writes use the selected process (default `0xFFFFFFFF` = whatever `Memory()` currently maps, logged as an error). The tool selects the application process (title high word 0x00040000, here pid 11, `LEGOCITY`) after connecting.
- A reply shorter than requested happens only when the selected process does not exist (size 0). Reads of unmapped addresses are not reported as errors by the protocol (see unknowns).
- WriteMemory is accepted only when the start address is inside the process image (0x100000..), heap (0x08000000..0x10000000), linear heap (0x14000000..0x1c000000) or New 3DS extra RAM; otherwise it is silently ignored, and the reply is the same empty reply as for success. The tool refuses such writes locally and verifies every write by reading it back. Writes to shared memory (0x10000000) and VRAM cannot be done over RPC.
- A successful write calls `System::InvalidateCacheRange`, which invalidates the JIT (`ARM_Dynarmic::InvalidateCacheRange`). GDB memory writes (`M` packet) call `ClearAllInstructionCache` for the same reason.
- `TakeScreenshot` returns `Unsupported` with the software renderer (`rpc_server.cpp`). With OpenGL it asks the renderer for a frame, so the emulation must be running (a halted CPU renders no frame and the request times out after 10 s). The PNG is a combined top-over-bottom layout, 400x480 at scale 1.

**Pad input without breakpoints (`--rpc-input`, verified)**
- **Correction (earlier version of this entry, commit 319610e):** the first design used the zero word at 0x10acf4 as the mailbox. That was wrong: `10ac94: eddf1a16 vldr s3, [pc, #88] @ 0x10acf4` loads it as the float constant 0.0, so writing the mask there corrupted that function whenever a button was held. A scan for absolute addresses cannot see pc-relative loads.
- The pad reader loads the held mask with `ldr r0,[r4]` at 0x10a578 (then `bic r0,r0,#0x2000` at 0x10a57c). The tool patches 0x10a578 to `b 0x356580` (`ea093000`) and writes a code cave: `0x356580 ldr r0,[pc]` (`e59f0000`, reads the mailbox), `0x356584 b 0x10a57c` (`eaf6cffc`), `0x356588` the mailbox word (held mask). `hold` writes the mailbox over RPC. The RPC write invalidates the JIT, so the patch takes effect on the next execution of the block. Encodings checked with `llvm-objdump` on a patched copy of `game.arm.elf`: `10a578: b 0x356580`, `356580: ldr r0,[pc] @ 0x356588`, `356584: b 0x10a57c`.
- Cave choice: the text segment is 0x100000..0x357000 (599 pages); `.code` text size is 2450812 bytes, so the last used byte is 0x35657b (`bx lr` at 0x356578) and 0x35657c..0x356fff is zero padding (2692 bytes). Evidence that 0x356580..0x356fff is unreferenced (script run against `artifacts/game/game.arm.elf` and `code.bin`): (1) the whole `llvm-objdump` disassembly (612545 lines): 19152 pc-relative `@ 0xADDR` comment lines and 90291 branch lines, none targeting the range; (2) an independent decode of every text word for pc-relative `ldr/str`, `ldrh/ldrd`, `vldr`, `add/sub rX,pc,#` and `b/bl` targets: none in the range; (3) every 32-bit word of `code.bin` (any alignment) with a value in 0x356500..0x357000: only one unaligned match (0x22dd8f, `0x356ee5`), which straddles the bytes of `ldr r0,[r0,#4]` and `bl 0x23b350` at 0x22dd8c/0x22dd90, not a pointer. The cave is cleared (zeroed) again on exit.
- Verified at the language screen: `hold A` + free run gives pad channel 16 = 1.0 and nothing else; `hold UP+B` gives channels 12 and 17 = 1.0; `hold -` gives zeros. It also works stepped (`frames 5` after `loadstate`). A loaded save state brings back the memory of the state, so the patch is re-applied after `loadstate` (re-checked after the cave change: after `loadstate` the instruction is `ea093000`, the cave `e59f0000 eaf6cffc <mask>`, and `frames 5` shows ch16 = 1.0). A later `--attach` reads the original `e5940000` at 0x10a578 and zeros in the cave: the patch is removed on exit.
- The tool restores the original instruction before leaving a kept emulator (otherwise the last mask would stay pressed for later attaches); `--stop`/kill needs no restore. The patch replaces the held mask entirely, host input is not combined.
- The circle pad is not covered: the raw words at r4+12/+14 are consumed at 0x10a540 as halfword loads, which cannot load a full word from the cave without a longer stub. `stick` still installs the per-frame breakpoint at 0x10a540 (only then, and the held mask stays breakpoint-free).

**Measurements** (this machine, software renderer unless noted, old 3DS, `--frame-limit 100`, state `fast-lang` at the language screen)
| Item | Time or value |
|---|---|
| Screenshot via RPC memory reads (software renderer, stepped/halted) | 0.05 s (identical PNGs to the GDB route) |
| Screenshot via GDB stub (same two screens, `screenshot NAME gdb`) | 10.7 s |
| Screenshot via RPC `TakeScreenshot` (OpenGL renderer, game runs during capture) | 0.18-0.20 s |
| RPC read round trip, language screen | about 0.03-0.1 ms per request (5 requests per `sample` row: 0.13-0.55 ms) |
| RPC read round trip, in-game dialog state (`agent-intro-dialog`) | about 1.7 ms per request (8-26 ms per `sample` row) |
| `sample 20 0.1` free run, language screen | 50.14 clock ticks per vblank, 60.2 vblanks/s (both counters 0x593d14/0x593d18 advance together) |
| Same, OpenGL renderer, `sample 10` | 50.14 ticks per vblank, 60.6 vblanks/s |
| Same, `agent-intro-dialog` state | 51.13 ticks per vblank |
| Stepped `frames 300` (breakpoint per frame), language screen | 65.5 ticks per vblank (0x593d14), 68.0 (0x593d18); 107 s wall |

The free-run rate of 50.14 ticks per vblank is the undistorted figure; the earlier 51.3 (see above) probably included the stub stops that bracket a `wait` interval; not isolated. Stepping inflates the clock by about 31 % even when the memory reads go through RPC, because the inflation comes from the halts.

**Limitations**
- RPC uses fixed UDP port 45987: one RPC emulator per machine. Port clashes are silent on the emulator side.
- Software renderer: no RPC screenshot request (frames only exist in emulated VRAM, read them with RPC memory reads). OpenGL: faithful timing and `TakeScreenshot` works, but the capture is taken from a running game, not a stepped frame, and VRAM reads are stale.
- RPC cannot set breakpoints, read registers or stop the CPU: stepped frame counting (`frames`, `trace`) still needs the GDB stub.
- `sample` reads about 5 values per row non-atomically (separate requests, so the game may advance a frame between them); rows carry host-side `t` as the midpoint of the row.
- Writes are limited to the regions above.

Unknowns:
- What an RPC read of an unmapped guest address returns (the code path was not exercised).
- Whether `ENABLE_SCRIPTING_SYNC` ordering guarantees (a request executes at a defined point of the emulation loop, e.g. at the start of a timeslice) matter for sampling alignment against vblank; not measured.
- Why the RPC round trip is 50 times slower in the dialog state than at the language screen (likely the emulation loop services pending work less often under load); not investigated.
- Behaviour of RPC writes into a JIT block that is executing at that moment on the second core (old 3DS has only core 0, so none seen).
- Circle-pad injection without a breakpoint (needs a patch of 0x10a540 and 0x10a550 to load full words from spare RAM, or a stub in the code cave).

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

## 2026-10-13T12:43:33+02:00 — Route to gameplay and the gameplay-start checkpoint

- `tools/oracle/to-gameplay.txt` (worker, Haiku; run with `--renderer opengl --display xvfb`, about 340 s from boot) reaches player control: language English (A), StreetPass No (B), HOME notice (A), autosave notice (A), slot 1 New Game (A), intro cutscene runs by itself (~140 s free run), "Doughnut Disaster" card (A). It ends with `savestate gameplay-start`.
- Lead check: `--state gameplay-start --renderer opengl --rpc --rpc-input`: the RPC screenshot shows Chase in the city street with the HUD (portrait, hearts) and the minimap; the player entity starts at world (886.764, 16.589, 1111.166). Stick (0, 150) for 60 stepped frames moved it to (856.937, 16.587, 1098.050), intent heading -18569, held flags 1, controller state byte 255.
- The software renderer crashes in this area (Azahar SIGSEGV in Pica::Texture::LookupTexture, seen three times), so gameplay work uses OpenGL with RPC screenshots.
- Checkpoint files live in artifacts/oracle/states/ (local, not committed); recreate them with the scenario.

Unknowns: the meaning of controller state 255; whether the cutscene timing (free-run waits) is long enough on slower hosts.

## 2026-10-13T13:45:00+02:00 — Watchpoints, execution traps, fixed dt and memory traces (locomotion work)

- `watch ADDR LEN N NAME [KIND]` sets a GDB watchpoint (Z2 write, Z3 read, Z4 access; the stub supports them, `gdbstub.cpp` `CommitBreakpoint`) and advances N frames. `trap ADDR[,ADDR] N NAME` does the same with execution breakpoints. Both write registers r0-r15, 32 stack words and 64 bytes at each pointer-like r0..r3 of every stop to `NAME.txt`. The stub reports `watch:ADDR;` (or `awatch`) in the stop reply; the Azahar log warns that dynarmic has no accurate watchpoints: the reported pc is at a block boundary, not the accessing instruction. Use a trap on candidate functions to confirm.
- `dt MIN MAX` writes the scheduler's dt limits (`C+0x10`, `C+0x14`, `C` = `*(P+0xc)`); with MIN == MAX each update gets exactly that step (the step is quantised to the 1 ms game clock: 0.0165 became 0.0163). Stepped runs otherwise get dt = max (0.07) on every update because the stub stops take longer than that; free-running games use about 0.033.
- `trace N NAME mem` also writes `NAME.mem`: per frame u32 frame, entity (0x100 bytes), controller (0x340) and node (0xb0).
- Bug fixed: `loadstate` with `--attach` did nothing (the quick-load key reached no window because the main window had no keyboard focus in the new X connection) while printing "loaded". The tool now focuses the main window before every quick save and quick load. Check a load by reading `player` afterwards.
- Timings (OpenGL, RPC, stepped): `trace` about 0.5 s per frame with memory dumps, `loadstate` plus 3 frames about 5 s with `--attach`.
- Findings from these tools: [player.md](player.md) (2026-10-13T13:42:20+02:00).
