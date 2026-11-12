# Windows: platform files

## 1. Scope

Platform code behind plain interfaces: save directory, atomic save write, process CPU time, native library loading.

## 2. Status

IN_PROGRESS. The atomic save write and the process CPU time have POSIX and Windows files; the save folder and the native library name are open.

## 3. Verified behaviour

Measured on Windows 11 by 20261112-winhooks-E, 2026-11-12:

- `src/save/file_replace_windows.cpp` writes a new file next to the target, flushes it with `FlushFileBuffers` and replaces the target with `MoveFileExW` (`MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH`); errors read `<what> <file>: <reason>` as on Linux. CTest `file-replace` and `save-core` pass.
- `src/runtime/process_cpu_time_windows.cpp` sums user and kernel time from `GetProcessTimes`. CTest `process-cpu-time` passes.
- CMake picks the files by `CHASE_PLATFORM` (`posix` or `windows`, `cmake/platform_files.cmake`); Linux builds only the POSIX files.
- The PC settings controls files are replaced with `std::filesystem::rename`, which overwrites on Windows too; Reset all finds `controls.default.cfg` with `std::filesystem::path`, so backslash paths work.
- `chase-game` opens a window, loads the police station and saves a picture after 30 updates when started from the UCRT64 shell.

## 4. Stand-ins

None.

## 5. Open items

- [ ] WINPLAT-1 — the default save path still follows the POSIX rule (`XDG_DATA_HOME`, then `HOME`); on Windows it must be `%APPDATA%\chase\savegame.dat` (folder name decided); CTest `save-default-path` skips on Windows until then; needs: WINBUILD-1; evidence: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437)
- [ ] WINPLAT-2 — `tools/chase_native.py` defaults to `build/libchase_assets_c.so`; on Windows `CHASE_ASSETS_LIB` must name `build/libchase_assets_c.dll` (the DLL itself loads); needs: WINBUILD-1; evidence: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437)
- [ ] WINPLAT-3 — the debug overlay reads its memory line from `/proc/self/status`, so it shows `MEM MB N/A` on Windows; needs a process memory platform file; evidence: [debug-overlay.md](../../debug-overlay.md)
- [ ] WINPLAT-4 — without `SDL2main` the programs get their arguments in the Windows ANSI code page, so a path with letters outside it may not open (not verified on Windows); evidence: [windows-build.md](windows-build.md)

## 6. Findings summary

Decisions: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437).

## 7. Code and tests

`src/save/file_replace*.{hpp,cpp}`, `src/runtime/process_cpu_time*.{hpp,cpp}`, `src/save/save.cpp`, `tools/chase_native.py`; tests `tests/file_replace_test.cpp`, `tests/process_cpu_time_test.cpp`, `tests/save_default_path_test.cpp`.
