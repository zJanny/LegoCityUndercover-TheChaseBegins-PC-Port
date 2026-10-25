# Runtime oracle: Launch, display and crash handling

Subtopic of the Azahar runtime oracle. Start with [oracle.md](oracle.md) for setup and the fast workflow. Entries are dated; new entries go at the end.

Launching Azahar for the oracle: Xvfb, the render window race, the launch retry, and the visible desktop mode.

## Current state

- Launch under Xvfb: the software renderer can abort on a state load (a paint race between the Qt thread and the state load, Azahar 2126.2); the tool runs with `singleWindowMode=false` and unmaps the render window, after which 8 of 8 runs and 6 x 8 loads succeeded. [§2026-10-13T12:41](#ts-20261013-124127)
- The tool relaunches at most 3 times when the emulator dies before the scenario starts, saves `crash<N>-*` logs, and kills only the process groups of the emulator and Xvfb it started. [§2026-10-13T12:41](#ts-20261013-124127)
- `--display desktop` runs on the current X display for a person to watch; agents keep using `xvfb`, and `--rpc-input` must not be combined with it. [§2026-10-13T22:45](#ts-20261013-224508)
- Default keys of the generated config: circle pad arrows, A = A, B = S, X = Z, Y = X, L = Q, R = W, D-pad T/F/G/H, Start = M, Select = N, F1 quick save, F12 quick load. [§2026-10-13T22:45](#ts-20261013-224508)
- The tool runs a Cemuhook DSU motion server for every session (default `--port` + 1000, `--motion-port`) and writes `motion_device` into the isolated config; "level" is DSU accel (0, 0, -1) g, `tilt` changes it. With gravity on DSU y the scanner yaw is NaN, with gravity on z the scanner view runs. [§2026-10-18T22:03:32](#ts-20261018-220332)
- The script steps `touch X Y [UPDATES]`, `touch release` and `drag X1 Y1 X2 Y2 [UPDATES]` touch the bottom screen through the same DSU server (first touch of the pad data, `touch_device` written next to `motion_device` at launch); the game's HID touch entry holds exactly the pixel (all 320 x values and all 240 y values checked), no RPC, no memory writes. [§2026-10-25T11:35:28](#ts-20261025-113528)
- Save states: a state made in New 3DS mode must be loaded in New 3DS mode (loading one in Old 3DS mode ended with SIGILL), so `NAME.json` carries `new_3ds`, `savestate` writes it and `loadstate` refuses a mode mismatch; New 3DS runs the CPU faster, so timing is repeated in Old 3DS mode; use `--renderer opengl` for states (the software renderer crashed loading states made in a vehicle). [§2026-10-20T23:27:26](#ts-20261020-232726)
- Committed oracle scripts load the states a developer records by a role: `loadstate @ROLE` and `--state @ROLE` with `--state-param ROLE=NAME`; everything is checked before the emulator starts, and `tools/check_local_data.py` reads `tools/oracle/*.txt`. The converted scripts are verified by parser tests only; none was run in the emulator yet (TOOLS-21). [§2026-10-25T14:14:55](#ts-20261025-141455)
- Open: crashes on a plain boot, whether an unmapped window changes timing, the cause of one launch that ended after 4 loads, crashes of a visible OpenGL window.

<a id="ts-20261013-124127"></a>

## 2026-10-13T12:41:27+02:00 — Azahar abort on quick load under Xvfb, launch retry and cleanup

**Symptom:** about 1 in 4 launches with `--display xvfb` and the software renderer died with SIGABRT a few seconds after the quick load, which looked like a startup crash (`ConnectionRefusedError`/`ConnectionResetError`, defunct azahar, running Xvfb).

**Cause (log and source, Azahar 2126.2):** the stack in `azahar_log.txt` is `SoftwareRenderWidget::paintEvent` (bootmanager.cpp:379) -> `Present()` draw lambda -> `unique_ptr::operator*` on null. The `ArchiveManager` in the symbol is an LTO-merged `unique_ptr` dereference; the pointer is most likely the `System::gpu` reached through `system.GPU().Renderer()` (not confirmed which one). The log line before the abort is `Begin load of slot 0` (core.cpp:195), 35-75 ms earlier in the one crash log kept. The state load runs `Shutdown(true)` (core.cpp:766), which sets `is_powered_on=false`, then `gpu.reset()` and `archive_manager.reset()`, and `Init` rebuilds them on the emulation thread. The GUI thread paints the software widget in a loop (`paintEvent` calls `update()`); `Present` tests `IsPoweredOn()` once (a plain `bool`, no lock) and dereferences `GPU()` later, after the top screen was scaled. A paint that passed the check before the teardown crashes. So this is a data race between the Qt thread and a state load, not an initialisation-order problem at startup.

**Measurement** (one launch, repeated `loadstate` of the save state at the language list): default config, loads completed before the abort per launch: 7, 2, 1, 2, 1 (and 3, 2, 2, 1 with `QT_QPA_UPDATE_IDLE_TIME=60000`, which has no effect). Render window unmapped (below): launches of 8 loads each: 8, 4 (aborted after 4, log not kept, cause not confirmed), 8, 8 and then 6 x 8 without an abort.

**Adopted mitigation (`tools/azahar_oracle.py`, `--display xvfb` only):**
- Config `[UI] singleWindowMode=false` with `\default=false` (with `\default=true` Azahar ignores the value), so the render widget is its own top-level window titled `... | Primary Window`.
- After both windows exist the tool calls `XUnmapWindow` on it and `XSetInputFocus` on the main window. Qt then gets no expose events, so `paintEvent` does not run. The quick save/load shortcuts are application shortcuts (`Context=2`) and still fire from the main window. The oracle never reads the host window (frames come from emulated VRAM through GDB/RPC).
- Verification scenario (`hold A`, `frames 3`, `channels`, save state at the language list, `--rpc --rpc-input`): 8/8 and 8/8 runs succeeded with 0 retries; ch16 = 1.0 as before. Before the change, with only the retry code, 1 of 8 runs needed a relaunch.
- Not adopted: `QT_QPA_UPDATE_IDLE_TIME` (no effect); a smaller window (does not remove the race).

**Launch robustness:** the startup phase (launch, window silencing, GDB connect, RPC connect, `--state` load) watches the emulator process. If it exits before the scenario starts, the tool saves the logs as `crash<N>-azahar_log.txt` and `crash<N>-azahar-stdout.log` next to the originals, prints one line with the signal (naming the known crash when the log holds both the `SoftwareRenderWidget::Present` and `ArchiveManager` frames), kills and reaps its own Xvfb and emulator and relaunches, at most 3 launches. Nothing is retried once the scenario has begun. On any exit (exception, Ctrl-C, SIGTERM, SIGHUP) the tool kills the process groups of the emulator and Xvfb it started (both are session leaders, so `killpg` cannot touch other processes) and waits for them; nothing is matched by name, so another emulator on another port is never touched. A SIGTERM during a run left no azahar, Xvfb or defunct process. Tests: `tests/test_azahar_oracle_launch.py`.

Unknowns:
- Whether Azahar can crash the same way during a plain boot without any state load (not seen).
- Whether an unmapped render window changes emulation timing (frame pacing was not compared).
- The cause of the one launch that ended after 4 loads with the render window unmapped (its log was overwritten); it may be a different race.

<a id="ts-20261013-224508"></a>

## 2026-10-13T22:45:08+02:00 — Visible mode for the user (--display desktop)

- `--display desktop` runs Azahar on the current X display (`DISPLAY`, here XWayland :0) with the same isolated home, generated config and Quick Save/Load hotkeys as `--display xvfb`; the render window is not unmapped. Meant for a person to watch and play (user request); agents keep using `xvfb`. Do not combine it with `--rpc-input` (that patch replaces the pad reader, so the keyboard would stop working).
- Verified: `--display desktop --renderer opengl --state gameplay-start --keep --port 24690 --home DIR` loaded gameplay-start (frame 30) and left the emulator running visibly; `tests/test_azahar_oracle_*.py` 23 OK.
- Default Azahar keys of the generated config: circle pad = arrow keys, A = A, B = S, X = Z, Y = X, L = Q, R = W, D-pad = T/F/G/H, Start = M, Select = N; F1 quick save, F12 quick load.

Unknowns:
- Whether the known software-renderer paint crash during state loads also hits a visible OpenGL window (not seen in the one run).

<a id="ts-20261018-220332"></a>

## 2026-10-18T22:03:32+02:00 — Virtual motion source (Cemuhook DSU) for the oracle emulator (Azahar 2126.2 strings and config, `tools/oracle_motion.py`, oracle port 24753 with RPC, save state on the roof at the sound scanner trigger of mission 1-2)

**Why:** the camera yaw word 0x376894 turned NaN in every oracle run of the sound scanner ([abilities-scanner.md §2026-10-18T11:27:21](abilities-scanner.md#ts-20261018-112721)); an Azahar with a real controller as `motion_device` (`engine:sdl`) shows the scanner. The oracle home had no motion device, so Azahar fed the game its default motion.

- **Build support:** The Azahar 2126.2 binary contains `InputCommon::CemuhookUDP::UDPMotionFactory`, the engine name `cemuhookudp` and the settings `udp_input_address`, `udp_input_port`, `udp_pad_index` (`strings`). The same three keys appear in the `[Controls]` profile of a normal Azahar config, next to `motion_device` (looked at once, read only).
- **Config the tool writes** (isolated home, section `[Controls]`, profile 1): `motion_device="engine:cemuhookudp,ip:127.0.0.1,port:<P>,pad_index:0"` plus `udp_input_address=127.0.0.1`, `udp_input_port=<P>`, `udp_pad_index=0`. The parameter names `ip`, `port` and `pad_index` of `motion_device` are from memory of the Citra source, not from this build's source; the evidence is that Azahar registers with the server on start (`motion-source.log`: "pad data client registered") and the game's HID accelerometer follows the server.
- **Server:** `tools/oracle_motion.py serve --port P --state FILE`, Python standard library only, started by `tools/azahar_oracle.py` for the session (default UDP port = `--port` + 1000, `--motion-port` overrides; it is recorded in `session.json` and stopped by `--stop`). It answers version and port requests, sends a pad-data packet every 10 ms to every client that registered, and re-reads the state file (written by the script step `tilt AX AY AZ [GP GY GR]`).
- **Protocol source:** the Cemuhook protocol document (https://v1993.github.io/cemuhook-protocol/, protocol version 1001): 16-byte header (magic `DSUS` / `DSUC`, version u16, length u16 without the header, CRC32 of the whole packet with the CRC field zeroed, id u32), message type u32 (0x100000 version, 0x100001 ports, 0x100002 pad data), pad data = 100 bytes with the motion timestamp at packet offset 68, accelerometer in g at 76 and gyroscope in degrees per second at 88. The unit test `tests/test_oracle_motion.py` pins these offsets and the standard CRC32 check value 0xCBF43926.
- **Axes measured** (RPC reads of the HID shared memory at 0x10002000, accelerometer at +0x108, entries of 3 s16 from +0x20, scale 512 per g): DSU accel (x, y, z) arrives in the game as (-x, y, -z) times 512; a read of one test run gave y = -512 for DSU y = -1, +512 for +1, x = -512 for DSU x = +1 and z = -512 for DSU z = +1. The gyroscope axes were not mapped.
- **Result at the scanner** (save state on the roof at the sound scanner trigger of mission 1-2, A pressed, 80 updates later): with DSU gravity on y (-1: HID y = -512) the yaw 0x376894 is NaN (0x7fc00000), as in the earlier runs without a motion source (whose HID values were not read); with DSU (0, 0, -1) (HID z = +512) it reads 1.5499 and the screenshot shows the scanner view (cyan overlay, zoom bar, headphone icon, map on the lower screen; picture not kept). One load per setting (the first run with y gravity started the session, the second loaded the state again). So the NaN depends on the motion input: gravity on the HID y axis breaks it, gravity on HID z does not. The tool's default is therefore "level" = (0, 0, -1) g, zero rotation.
- A scanner run after the first NaN stays NaN while the motion changes (tilt 0 0 -1 applied in the poisoned session did not repair it); reload the state.

Unknowns:
- Which code turns the HID accelerometer into the yaw or camera matrix (not traced) and why y gravity yields NaN; whether a real console held level reports z = +512 (the default is a measured choice, stand-in TOOLS-S1).
- The gyroscope axis mapping, and whether any gyroscope input changes the scanner aim (only zero rotation was run).
- The exact Azahar parameter names of `motion_device` beyond what the registration and the HID values show.

<a id="ts-20261020-232726"></a>

## 2026-10-20T23:27:26+02:00 — Console mode of save states, load pitfalls

- Loading a New 3DS state into Old 3DS mode crashed Azahar with SIGILL in 3 of 3 attempts. A state's `NAME.json` may carry `"new_3ds": true`; the tool then writes `is_new_3ds=true` into the generated config (`state_needs_new_3ds`). The launch records the mode in `session.json` (a missing key means Old 3DS), `savestate` writes the running emulator's mode into `NAME.json`, and `loadstate` in a running session raises `state NAME was saved in <mode> mode; start a new emulator with --state NAME` before it copies anything. Checked by `tests/test_azahar_oracle_launch.py` and a real run (load a New 3DS state, `savestate`, `loadstate`, then a refused `loadstate` of an Old 3DS state; the emulator stayed alive and `--stop` left no `azahar` or `Xvfb`).
- Measurement caveat: New 3DS mode runs the CPU faster than the Old 3DS mode of the earlier measurements. Use New 3DS states for pictures, camera, door and layout checks; repeat timing-sensitive measurements in Old 3DS mode or from a state saved there before pinning rates.
- Renderer: launches with a state made while the player sat in a vehicle crashed during the load with the software renderer (SIGSEGV, 6 of 6, not the known paint race); with `--renderer opengl` they load.
- The quick load reads slot 00 only: a `NAME.json` whose `file` names slot 01..05 loads the previous state or boots a fresh game, so `file` names `00040000000AD600.00.cst`.
- A state saved right after the tool detaches the gdb client is later than the stepped frame (the save key is queued until the emulation thread runs); `NAME.json` `frame` is the stepped frame, not the saved one.
- A state made with GDB `poke` is a measuring aid, not a reference; its heap addresses are valid for that state only. Without RPC the player is placed by poking the node translations (world `node+0x8c`, local `node+0x4c`), which is what `teleport` does over RPC. A heap scan with `memtrace 1 NAME ADDR:LEN ...` over 1 MB regions (0x08500000..0x08d00000) took 170 s.

Unknowns:
- The Old 3DS value of the flag written by `savestate` in an Old 3DS session is checked by the unit test only, not by an emulator run.
- Whether game logic timing differs between the two modes (frame pacing was measured only in Old 3DS mode).

<a id="ts-20261025-113528"></a>

## 2026-10-25T11:35:28+01:00 — Touching the bottom screen through the DSU server (`touch`, `drag`; Azahar 2126.2, state `gameplay-start`, round 20261025-mission13, worker D)

**Why:** bottom-screen menus are the game's main UI and the tool could not touch the screen. Requested by the user (2026-10-25); first user: the fast travel map exit button at pixel (159, 65).

**Route that works (first try, no RPC):**
- **Build support:** `strings` on the Azahar 2126.2 binary shows `InputCommon::CemuhookUDP::UDPTouchFactory` / `UDPTouchDevice` and the config text "UDP touchpad calibration config success: min_x=..., min_y=..., max_x=..., max_y=..." (the calibration keys `min_x`, `min_y`, `max_x`, `max_y`). The `touch_device` parameter names (`engine:cemuhookudp,pad_index:0,min_x,min_y,max_x,max_y`) are from memory of the Citra source, not read from this build; the evidence is the behaviour below.
- **Config the tool now writes** (profile 1, next to `motion_device`): `touch_device="engine:cemuhookudp,pad_index:0,min_x:0,min_y:0,max_x:3200,max_y:2400"`. The server and address are the existing `udp_input_*` keys.
- **Server** (`tools/oracle_motion.py`): the optional key `"touch": [x, y]` (bottom-screen pixels) of the state file is sent as the first touch of the 100-byte pad packet: packet offset 56 active u8, 57 id u8 (1), 58 x u16, 60 y u16; the second touch (62..67) stays zero and the motion timestamp stays at 68 (protocol document https://v1993.github.io/cemuhook-protocol/ section "Controller data", read as a specification only). The pad value is `pixel * 10 + 5` in a 3200 x 2400 pad range (10 pad units per pixel, the middle of the pixel), so Azahar's scaling of the range to the 320 x 240 screen cannot round to a neighbour. Without `touch` the bytes are zero; state files written by `tilt` or by older versions stay valid, and `tilt` and `touch` keep each other's values.
- **Script steps** (`tools/azahar_oracle.py`): `touch X Y [UPDATES]` writes the point, waits 0.15 s real time (the server polls the file every 10 ms and sends a packet every 10 ms; the emulator is halted at the scheduler breakpoint during the wait), advances UPDATES updates like `frames`, then always writes the release (also when the advance fails) and waits again. `drag` does the same with one new point per update and one `advance(1)` each. `touch release` only releases. A session started by an older tool has no `touch_device` in its config (`session.json` has no `touch_device` key): the step stops with an error instead of doing nothing.

**Measured** (state `gameplay-start`: new game, mission 1-1 not started, the player at the start; Old 3DS mode, OpenGL, `--keep`, no RPC; reads of the HID shared memory object through the GDB stub after each update):
- HID shared memory is at 0x10002000; the touch block at +0xa8 is: u64 tick, u64 previous tick, u32 index of the newest entry (header 0x20 bytes), then 8 entries of 8 bytes at +0x20: u16 x, u16 y, u32 valid. The servant's XTEST mouse test read the same entry layout (x=159, y=65, valid=1 for a click at the bottom-screen pixel).
- With a hold, all 8 ring entries read `(x, y, 1)` of the pixel at the end of the first touched update (UPDATES 1, 2 and 12 tried), and `(0, 0, 0)` in all 8 entries after the first update following the release. A touch held outside the script (state file written by hand) stays pressed across `frames` steps.
- **Mapping: the HID value is exactly the pixel, no offset and no scale.** Points read: (0,0), (319,239), (159,65), (1,1), (318,238), (160,120), (37,211), (255,7), (10,20), (155,110), (300,200). Sweeps with `drag` over every position, 320 updates each: x 0..319 at y 100, y 0..239 at x 100, and the diagonal (0,0) to (319,239); every update's ring held the expected pixel, 0 mismatches in 880 updates.
- The game's own reader (0x10fca0 / 0x10a44c) was not traced and no menu was seen reacting (no screenshots without RPC); only the HID words the reader consumes were read.

**Pitfalls:**
- A `read` step after `touch` shows the released state (all entries `(0, 0, 0)` in the run with a 20-update tap), because the step releases before it returns; to see the held touch, read after each update inside the step (done with a throwaway script around `Oracle.advance`) or touch from outside the script by writing the state file. The guest tick of the ring advanced about 0.4 s per stepped update in this setup.
- `tools/azahar_oracle.py` called by a relative path from `artifacts/oracle/...` resolves through the `artifacts` symlink to the main checkout's tool.

Unknowns:
- What the game does with a one-update tap in a given menu (the reader and the menu code were not traced); the default of 6 updates is a tool choice, not game behaviour.
- Whether a second finger could be needed (the game's reader takes one point; only one is supported).
- Azahar's exact touch parameter names beyond what the working config shows (the scale of the range was confirmed by the exact pixel match).

<a id="ts-20261025-131758"></a>

## 2026-10-25T13:17:58+01:00 — Oracle scripts name a user save state; the tool has no state parameter (round 20261025-traffic, recorder)

Decision of the user (words unchanged): "For the decision, create a new open item a new round will fix. Apply your recommendations".

Gap: after the round removed user save-state names from committed files, the committed scripts in `tools/oracle/` still `loadstate` a user save state by name. `tools/azahar_oracle.py` has no state parameter or placeholder in the script language; the only way to pass a state is `--state NAME` on the command line, which loads once after the boot, while these scripts reload the state in the middle of the run. The scripts are `attack-record`, `attack-variants`, `cage-load-animation-ctrace`, `cage-thread-start-ctrace`, `camera-lobby-walk`, `camera-street-near`, `camera-street-seed`, `camera-street-teleport`, `camera-vehicle-drive`, `camera-vehicle-enter`, `camera-vehicle-handback`, `camera-vehicle-handback-near`, `dcam-hold-car`, `dcam-hold-order`, `dcam-police-entry`, `dcam-police-order`, `dcam-police-poked`, `dcam-police-shot`, `script-anim-blend-trap`, `script-anim-order-ctrace`, `step-order-ctrace` and `front-screens`.

Applied recommendation (not done yet): add a state parameter (a placeholder) to the script language and `tools/azahar_oracle.py`, so these scripts load whatever state the developer passes; remove the state names from the scripts; extend `tools/check_local_data.py` to cover `tools/oracle`. The names of the four states that committed scripts create themselves with `savestate` stay (`gameplay-start`, `barrier-settled`, `camwall-east` and `camera-lobby-walk`); `camera-lobby-walk` first copies a user state, which the parameter also resolves.

<a id="ts-20261025-141455"></a>

## 2026-10-25T14:14:55+01:00 — State roles for oracle scripts: `loadstate @ROLE` and `--state-param` (20261025-recorded-A)

Closes TOOLS-20 ([§2026-10-25T13:17:58](#ts-20261025-131758)). Tool design, not game behaviour; nothing here comes from the original, so it is no stand-in.

**Design (the smallest parameter that fits the script language):**
- A state name in `loadstate` and in `--state` may be a role: `@` followed by capital letters, digits and `_`, starting with a letter (`loadstate @PATROL_CAR`). Plain names keep working, so the scripts that load states they save themselves (`gameplay-start`, `barrier-settled`, `camwall-east`, `camera-lobby-walk`) are unchanged.
- The developer names the state of a role with `--state-param ROLE=NAME` (repeatable, one NAME per ROLE; NAME starts with a letter or digit and has only letters, digits, `_`, `-`, `.`). A script run with `--state @ROLE --state-param ROLE=NAME` starts in the state and reloads it in the middle of the run as before.
- Everything is checked before the emulator starts (`scenario_state_error`, `parse_state_params`, `resolve_state`, `state_files_error` in `tools/azahar_oracle.py`): a role without a parameter, a malformed role or parameter, a role given twice, a missing `NAME.cst` / `NAME.json` for a role (names the scenario saves itself are skipped), a `loadstate` or `savestate` with other than one word, and `savestate @ROLE` stop with exit code 2 and a message that names the option or the script line and the bad text. A role in a comment is not read.
- `tools/check_local_data.py` now reads `tools/oracle/*.txt`: a `loadstate` there may name only a role or a state that a committed script saves with `savestate`; `--state NAME` in a comment of a script is checked as in docs and tests. With the extended check the unconverted scripts gave 38 hits; the converted ones give none.
- Tests: `tests/test_azahar_oracle_state_param.py` (parser, resolver, scenario check, command-line rejections through the tool, and all committed scripts valid with every role given and invalid without), `tests/test_check_local_data.py`.

**Scripts converted (the 22 of the entry plus 4 that only named a state in a comment; the other scripts with `loadstate` load `gameplay-start` or a state they save):** the role, its situation in game terms and the scripts that use it. The situations come from the script headers; they are what the script needs, not a description of anyone's state.

| Role | Situation the script needs | Scripts |
|---|---|---|
| `CAGE_USE_SPOT` | Mission 1-2, the player at the use spot of the cage key (A for 3 updates starts `1-2_Cage_Key_Target_Script`) | `cage-load-animation-ctrace`, `cage-thread-start-ctrace`, `script-anim-blend-trap`, `script-anim-order-ctrace`, `step-order-ctrace` |
| `PATROL_CAR` | New 3DS; mission 1-1 in progress, in front of `1-1_Waypoint_02`, before the patrol car is unlocked | `camera-vehicle-enter`, `camera-vehicle-handback`, `camera-vehicle-handback-near`, `dcam-hold-car`, `dcam-hold-order` |
| `CAR_SEATED` | New 3DS; the player seated in `1-1_Police_Car` | `camera-vehicle-drive` (`camera-vehicle-enter` names it in a comment) |
| `TABLE_BEFORE` | On foot, 2.06 m south of `DonutTable_14`, facing it | `attack-record`, `attack-variants`; comment only: `attack-tap`, `attack-tap-hit`, `attack-tap-ctrace`, `attack-combo-hit` |
| `POLICE_DIALOG` | New 3DS; the street at the police station entrance, the question box of `Police_ExtToInt` open | `dcam-police-entry`, `dcam-police-order`, `dcam-police-poked`, `dcam-police-shot` |
| `STREET_AFTER_1_1` | New 3DS; on the street after mission 1-1 | `camera-street-near`, `camera-street-seed`, `camera-street-teleport` |
| `POLICE_LOBBY` | New 3DS; in the police station lobby after mission 1-1 | `camera-lobby-walk` (it saves `camera-lobby-walk` itself from it) |
| `FRONT_LANGUAGE`, `FRONT_STREETPASS`, `FRONT_HOME_NOTICE`, `FRONT_AUTOSAVE`, `FRONT_SELECT_SLOT`, `AGENT_INTRO_DIALOG`, `AGENT_DOUGHNUT_TITLE` | The front-end screen of that name (language choice, StreetPass page, HOME notice, auto-save notice, slot choice, the agent intro dialog, the doughnut title) | `front-screens` |

`front-screens` also named its screenshots after the states; they are now `streetpass`, `homenotice`, `autosave`, `selectslot` and `doughnut-title`.

Unknowns:
- The converted scripts were not run in the emulator in this round (none was started); the new syntax is covered by the tests above and by the check that every committed script parses with all roles given. The first real run is TOOLS-21.
- The situations of the front-end roles are inferred from the screen names in the script; the game progress of each is not recorded in the repository.
