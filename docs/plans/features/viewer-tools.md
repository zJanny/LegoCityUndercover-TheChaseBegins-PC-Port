# Viewer, development tools and the Azahar oracle

## 1. Scope

The developer programs and scripts: `chase-viewer` (models, levels, animated characters, cutscenes), `chase-soundtest`, `chase-fmv`, `chase-savecheck`, `chase-tabledump`, `chase-frontshot`, the Python tools over the C ABI, and the Azahar runtime oracle (`tools/azahar_oracle.py`, scenarios in `tools/oracle/`, save states). Rules for using the oracle are in CLAUDE.md "Runtime oracle" and [oracle.md](../../re/oracle.md). The game runtime is `chase-game` ([player-movement.md](player-movement.md) and the other feature files).

## 2. Status

IN_PROGRESS, updated 2026-10-25T11:36:10+01:00. All tools run; the oracle supports save states, an RPC channel, watch/trap/memtrace/teleport/poke steps and the `tilt`, `touch` and `drag` steps of the virtual motion source.

## 3. Verified behaviour

- Oracle: Azahar 2126.2 with a gdb stub; `--keep`/`--attach`/`--stop`, Xvfb with XTEST hotkeys, `savestate`/`loadstate`/`--state`, `--rpc` (screenshot 0.05 s) and `--rpc-input`; route from boot `tools/oracle/to-gameplay.txt`; stick and button state survive `loadstate`; `teleport X Y Z` and `poke` steps for positioning and writing state; `python3 -m unittest` 63 OK (including `FreeDisplayTest`).
- chase-game options are parsed strictly (`src/game_options.cpp`, `src/strict_parse.hpp`): a bad number, an out-of-range value, a malformed `--start`, `--script` or `--front-script` step, or an unknown option stops with `chase-game: <option>: <reason>: '<text>'` and exit code 1; checked by `game-options-core` and the `game-option-*` tests, which need no game data.
- Debug overlay of `chase-game` ([user doc](../../debug-overlay.md)): F3 (action `DebugOverlay` in `config/controls.cfg`, toggle) or `--debug-overlay` shows FPS, frame time (last, average, min, max over 1 s), resident and peak memory (`/proc/self/status`), process CPU percent (`getrusage`, 100 = one core), GPU time and draw calls of the level draw, updates per second, level, player position and heading, script threads; on the front-end screens the timing lines and the screen name. Off by default; with it off the screenshots `game-*` and `front-*` (except `front-handoff`, which already varies from run to run on main) are byte-identical to main (`cmp`); with it on only the top-left panel area differs (`debug-overlay-area`). Developer tool, no game behaviour, not a stand-in.
- Viewer screenshots are byte-identical across the renderer split; `viewer-real-smoke`, `viewer-cutscene-smoke`.
- Python converters (`extract_fib`, `convert_textures`, `convert_audio`, `convert_loc`, `convert_font`, `convert_levels --param-names`) call the C++ runtime and produced byte-identical output on the real data.

- Oracle touch: the steps `touch X Y [UPDATES]`, `touch release` and `drag X1 Y1 X2 Y2 [UPDATES]` (strictly parsed, [commands.md](../../commands.md#touching-the-bottom-screen)) send the point as the first touch of the DSU pad data and `touch_device` in the generated config; the game's HID touch entry held exactly the pixel for every x (0..319) and every y (0..239) swept, with no RPC and no game memory write. Evidence: [oracle-setup.md §2026-10-25T11:35:28](../../re/oracle-setup.md#ts-20261025-113528).

- Progress report: `tools/progress_report.py` measures script commands the port handles, object types with recovered behaviour, archive files with a decoder, open items and stand-ins, tests and recorded traces, and documented game functions, in about 2 seconds; it rewrites the marked README table and `--check` gates it (`progress-readme`). The lists of handled script commands (`handledScriptFunctions`), object behaviours (`objectBehaviours`) and file decoders (`fileDecoder`) are C++ tables read through `chase_level_census` / `chase_file_decoder` / `chase_script_handlers`. Definitions: [docs/progress.md](../../progress.md).

## 4. Stand-ins

- [ ] TOOLS-S1 — The oracle's level motion pose (DSU accel 0 0 -1 g, zero rotation) was chosen because the scanner works with it and not with gravity on y; it is not read from the original or from a console. evidence: [oracle-setup.md §2026-10-18T22:03:32](../../re/oracle-setup.md#ts-20261018-220332).
- The viewer uses the level lights of RENDER-1 with the room stand-in RENDER-S60.

## 5. Open items

- [ ] TOOLS-1 — The meaning of oracle controller state 255; whether the cutscene timing (free-run waits) is long enough on slower hosts. evidence: [oracle.md §2026-10-13T11:50](../../re/oracle.md#ts-20261013-115030).
- [ ] TOOLS-2 — Whether Azahar can crash during a plain boot without a state load, whether the unmapped render window changes emulation timing, and the cause of one launch that ended after 4 loads (the Xvfb abort is mitigated, not removed). Only one emulator per machine can use `--rpc`. evidence: [oracle-setup.md §2026-10-13T12:41](../../re/oracle-setup.md#ts-20261013-124127).

- [ ] TOOLS-3 — The function total of the progress report is an estimate (7575 call and pointer targets against 8086 procedures in REA); an exact list needs a stored REA procedure list. evidence: [arm-executable.md §2026-10-13T22:17](../../re/arm-executable.md#ts-20261013-221729).
- [ ] TOOLS-4 — "Object types with behaviour" is decided per class from the class file (props with a `CollisionBound`, the prop family, doors, studs); it does not check that an instance produced a shape, and characters and vehicles count as none until they get rules (COLL-2).
- [ ] TOOLS-19 — A menu reacting to a `touch` was not observed (no screenshots without RPC) and the game's touch reader 0x10fca0 was not traced; the default tap of 6 updates is a tool choice; no second finger. evidence: [oracle-setup.md §2026-10-25T11:35:28](../../re/oracle-setup.md#ts-20261025-113528).
- [ ] TOOLS-15 — The gyroscope axis mapping of the virtual motion source, and the code that turns the accelerometer into the NaN camera yaw when gravity lies on the HID y axis. evidence: [oracle-setup.md §2026-10-18T22:03:32](../../re/oracle-setup.md#ts-20261018-220332).

## 6. Findings summary

- Oracle touch through the DSU server, measured pixel mapping: [oracle-setup.md §2026-10-25T11:35:28](../../re/oracle-setup.md#ts-20261025-113528).
- Virtual motion source (DSU), axes, scanner result: [oracle-setup.md §2026-10-18T22:03:32](../../re/oracle-setup.md#ts-20261018-220332).
- Setup, fidelity rules, fast workflow: [oracle.md §2026-10-13T11:50](../../re/oracle.md#ts-20261013-115030); RPC channel: [oracle-rpc.md §2026-10-13T12:02](../../re/oracle-rpc.md#ts-20261013-120221); route to gameplay: [oracle-routes.md §2026-10-13T12:43](../../re/oracle-routes.md#ts-20261013-124333).
- Function count estimate of the game program: [arm-executable.md §2026-10-13T22:17](../../re/arm-executable.md#ts-20261013-221729).
- Watchpoints, traps, memory traces, teleport, poke: [oracle-measuring.md §2026-10-13T13:45](../../re/oracle-measuring.md#ts-20261013-134500), [oracle-routes.md §2026-10-13T16:05](../../re/oracle-routes.md#ts-20261013-160530), [oracle-measuring.md §2026-10-13T21:18](../../re/oracle-measuring.md#ts-20261013-211800).

## 7. Code and tests

- `src/game_options.cpp`, `src/strict_parse.hpp`, `src/frontend/script.*`, `tests/game_options_test.cpp`, `src/viewer.cpp`, `src/soundtest.cpp`, `src/fmv.cpp`, `src/savecheck.cpp`, `src/tabledump.cpp`, `src/frontshot.cpp`, `tools/*.py`, `tools/azahar_oracle.py`, `tests/test_*.py`; command reference `docs/commands.md`.
- `tools/progress_report.py`, `tests/test_progress_report.py`, `tools/check_doc_links.py`, `tests/test_check_doc_links.py`, `src/assets/object_classes.*`, `src/assets/file_decoders.*`, `handledScriptFunctions` in `src/runtime/level_start.*`, the census entry points in `src/assets/c_api.cpp`, `tests/progress_tables_test.cpp`.
- Debug overlay: `src/runtime/debug_metrics.*` (no SDL: frame-time window, `/proc/self/status` parser, CPU meter, text lines), `src/render/debug_text.*` (own 5x7 font), `src/render/debug_overlay.*` (draws with the HUD shaders), `LevelSession::debugValues`, `--debug-overlay` in `src/game_options.cpp`, `tools/check_debug_overlay_shots.py`, `tests/debug_metrics_test.cpp`, `tests/debug_text_test.cpp`; user doc `docs/debug-overlay.md`. CTest: `debug-metrics-core`, `debug-text-core`, `debug-overlay-off`, `debug-overlay-on`, `debug-overlay-area`, `game-option-debug-overlay-value`.
- CTest: `game-options-core`, `game-option-slot-range`, `game-option-number-trailing`, `game-option-start-malformed`, `game-option-script-step`, `game-option-front-script-step`, `game-option-unknown`, `viewer-option-number`, `viewer-option-cull`, `viewer-option-fps`, `viewer-option-eye`, `viewer-option-unknown`, `soundtest-option-id-range`, `soundtest-option-seconds`, `soundtest-option-missing`, `fmv-option-time`, `frontshot-option-language`, `frontshot-option-script`, `hudshot-option-script`, `hudshot-option-unknown`, `savecheck-option-unknown`, `tabledump-option-unknown`, `input-lab-option-unknown`, `progress-tables`, `progress-readme`, `doc-links`, `viewer-real-smoke`, `viewer-cutscene-smoke`, `soundtest-real-smoke`, `soundtest-id-smoke`, `fmv-real-smoke`, `frontshot-real-smoke`; Python: `python3 -m unittest discover -s tests -p 'test_*.py'`., `debug-metrics-core`, `debug-text-core`, `debug-overlay-off`, `debug-overlay-on`, `debug-overlay-area`, `game-option-debug-overlay-value`
- Virtual motion source: `tools/oracle_motion.py`, wiring and the `tilt`, `touch` and `drag` steps in `tools/azahar_oracle.py`, tests `tests/test_oracle_motion.py`, `tests/test_azahar_oracle_touch.py` and `tests/test_azahar_oracle_launch.py` (Python unittest, no emulator needed).
