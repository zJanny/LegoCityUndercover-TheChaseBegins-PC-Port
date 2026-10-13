# Viewer, development tools and the Azahar oracle

## 1. Scope

The developer programs and scripts: `chase-viewer` (models, levels, animated characters, cutscenes), `chase-soundtest`, `chase-fmv`, `chase-savecheck`, `chase-tabledump`, `chase-frontshot`, the Python tools over the C ABI, and the Azahar runtime oracle (`tools/azahar_oracle.py`, scenarios in `tools/oracle/`, save states). Rules for using the oracle are in CLAUDE.md "Runtime oracle" and [oracle.md](../../re/oracle.md). The game runtime is `chase-game` ([player-movement.md](player-movement.md) and the other feature files).

## 2. Status

IN_PROGRESS, updated 2026-10-13T22:17:29+02:00. All tools run; the oracle supports save states, an RPC channel, watch/trap/memtrace/teleport/poke steps.

## 3. Verified behaviour

- Oracle: Azahar 2126.2 with a gdb stub; `--keep`/`--attach`/`--stop`, Xvfb with XTEST hotkeys, `savestate`/`loadstate`/`--state`, `--rpc` (screenshot 0.05 s) and `--rpc-input`; route from boot `tools/oracle/to-gameplay.txt`; stick and button state survive `loadstate`; `teleport X Y Z` and `poke` steps for positioning and writing state; `python3 -m unittest` 63 OK (including `FreeDisplayTest`).
- Viewer screenshots are byte-identical across the renderer split; `viewer-real-smoke`, `viewer-cutscene-smoke`.
- Python converters (`extract_fib`, `convert_textures`, `convert_audio`, `convert_loc`, `convert_font`, `convert_levels --param-names`) call the C++ runtime and produced byte-identical output on the real data.

- Progress report: `tools/progress_report.py` measures script commands the port handles, object types with recovered behaviour, archive files with a decoder, open items and stand-ins, tests and recorded traces, and documented game functions, in about 2 seconds; it rewrites the marked README table and `--check` gates it (`progress-readme`). The lists of handled script commands (`handledScriptFunctions`), object behaviours (`objectBehaviours`) and file decoders (`fileDecoder`) are C++ tables read through `chase_level_census` / `chase_file_decoder` / `chase_script_handlers`. Definitions: [docs/progress.md](../../progress.md).

## 4. Stand-ins

- None. (The viewer's lighting is RENDER-S1.)

## 5. Open items

- [ ] TOOLS-1 — The meaning of oracle controller state 255; whether the cutscene timing (free-run waits) is long enough on slower hosts. evidence: [oracle.md §2026-10-13T11:50](../../re/oracle.md).
- [ ] TOOLS-2 — Whether Azahar can crash during a plain boot without a state load, whether the unmapped render window changes emulation timing, and the cause of one launch that ended after 4 loads (the software-renderer abort under Xvfb is mitigated, not removed). Only one emulator per machine can use `--rpc`. evidence: [oracle.md §2026-10-13T12:41](../../re/oracle.md).

- [ ] TOOLS-3 — The function total of the progress report is an estimate (7575 call and pointer targets against 8086 procedures in REA); an exact list needs a stored REA procedure list. evidence: [arm-executable.md §2026-10-13T22:17](../../re/arm-executable.md).
- [ ] TOOLS-4 — "Object types with behaviour" is decided per class from the class file (props with a `CollisionBound`, the prop family, doors, studs); it does not check that an instance produced a shape, and characters and vehicles count as none until they get rules (COLL-2).

## 6. Findings summary

- Setup, fidelity rules, fast workflow: [oracle.md §2026-10-13T11:50](../../re/oracle.md); RPC channel: [oracle.md §2026-10-13T12:02](../../re/oracle.md); route to gameplay: [oracle.md §2026-10-13T12:43](../../re/oracle.md).
- Function count estimate of the game program: [arm-executable.md §2026-10-13T22:17](../../re/arm-executable.md).
- Watchpoints, traps, memory traces, teleport, poke: [oracle.md §2026-10-13T13:45](../../re/oracle.md), [oracle.md §2026-10-13T16:05](../../re/oracle.md), [oracle.md §2026-10-13T21:18](../../re/oracle.md).

## 7. Code and tests

- `src/viewer.cpp`, `src/soundtest.cpp`, `src/fmv.cpp`, `src/savecheck.cpp`, `src/tabledump.cpp`, `src/frontshot.cpp`, `tools/*.py`, `tools/azahar_oracle.py`, `tests/test_*.py`; command reference `docs/commands.md`.
- `tools/progress_report.py`, `tests/test_progress_report.py`, `src/assets/object_classes.*`, `src/assets/file_decoders.*`, `handledScriptFunctions` in `src/runtime/level_start.*`, the census entry points in `src/assets/c_api.cpp`, `tests/progress_tables_test.cpp`.
- CTest: `progress-tables`, `progress-readme`, `viewer-real-smoke`, `viewer-cutscene-smoke`, `soundtest-real-smoke`, `soundtest-id-smoke`, `fmv-real-smoke`, `frontshot-real-smoke`; Python: `python3 -m unittest discover -s tests -p 'test_*.py'`.
