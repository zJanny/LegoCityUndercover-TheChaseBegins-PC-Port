# Windows: platform files

## 1. Scope

Platform code behind plain interfaces: save directory, atomic save write, process CPU time, native library loading.

## 2. Status

IN_PROGRESS. The atomic save write, the process CPU time, the save folder, the process memory and the native library name have POSIX and Windows variants; every program reads UTF-8 paths through a manifest; the Python tools and old Windows 10 versions are open.

## 3. Verified behaviour

Measured on Windows 11 by 20261112-winhooks-E, 2026-11-12:

- `src/save/file_replace_windows.cpp` writes a new file next to the target, flushes it with `FlushFileBuffers` and replaces the target with `MoveFileExW` (`MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH`); errors read `<what> <file>: <reason>` as on Linux. CTest `file-replace` and `save-core` pass.
- `src/runtime/process_cpu_time_windows.cpp` sums user and kernel time from `GetProcessTimes`. CTest `process-cpu-time` passes.
- `src/save/save_directory_windows.cpp` returns `%APPDATA%\chase` and `defaultSavePath()` appends `savegame.dat` (WINPLAT-1, measured by 20261113-winopen-B, 2026-11-13). The returned string uses backslashes when `APPDATA` has them: `C:\Users\u\AppData\Roaming\chase\savegame.dat`; slashes in `APPDATA` stay (`std::filesystem::path` appends the preferred separator, and both name the same file). An unset or empty `APPDATA` throws `APPDATA is not set`, and `chase-game` then prints `save: writing disabled (APPDATA is not set)`, as for `HOME` on POSIX. `store()` creates the `chase` folder, which `tests/save_default_path_windows_test.cpp` checks with a temporary `APPDATA`. The POSIX rule moved unchanged into `src/save/save_directory_posix.cpp`, and CTest `save-default-path` runs the old test there. Command: `ctest --test-dir build -R save-default-path`.
- `src/runtime/process_memory_windows.cpp` reads the working set and the peak working set from `GetProcessMemoryInfo` (library `psapi`, linked to `chase_runtime` on Windows only), in kilobytes of 1024 bytes; `src/runtime/process_memory_posix.cpp` reads `/proc/self/status` through `parseProcessStatus`. `DebugMetrics::readout()` calls `currentProcessMemory()`, so the overlay shows its memory line on Windows. CTest `process-memory` passes: a plausible size, and touching 64 MB raises it by at least 48 MB.
- Labelled choice (WINPLAT-3): working set = resident size and peak working set = peak resident size, the Windows counterparts of `VmRSS` and `VmHWM`. Measured on Windows 11 by 20261113-winopen-B, 2026-11-13, with a scratch program that calls `currentProcessMemory()`: 4272 kB resident and peak at the start; 69840 kB after writing one byte per page of 64 MB (+65568 kB); 4304 kB resident and 69840 kB peak after freeing it; 4308 kB after reserving 256 MB without touching it; 266448 kB after touching it. As on Linux, untouched memory does not count, freed memory leaves the size and the peak stays.
- CMake picks the files by `CHASE_PLATFORM` (`posix` or `windows`, `cmake/platform_files.cmake`); Linux builds only the POSIX files.
- The PC settings controls files are replaced with `std::filesystem::rename`, which overwrites on Windows too; Reset all finds `controls.default.cfg` with `std::filesystem::path`, so backslash paths work.
- `tools/chase_native.py` defaults to `build/libchase_assets_c.dll` on Windows and `build/libchase_assets_c.so` elsewhere (`default_library(windows)`); `CHASE_ASSETS_LIB` still wins. Measured by 20261113-winopen-D, 2026-11-13: the MinGW build names the file with the `lib` prefix (`libchase_assets_c.dll`, next to an import library `libchase_assets_c.dll.a`); `objdump -p` lists only `KERNEL32.dll` and `api-ms-win-crt-*.dll` as imports (the library is linked `-static`), and a python.org Python with no MSYS2 folder on `PATH` loads it through `ctypes` and runs `tests/test_convert_loc.py`, so no `PATH` entry and no DLL search directory is needed. Tests: `tests/test_chase_native_library.py`.
- `chase-game` opens a window, loads the police station and saves a picture after 30 updates when started from the UCRT64 shell.

Measured on Windows 11 (ANSI code page 1252) by 20261113-winopen-C, 2026-11-13, MSYS2 UCRT64 GCC 16.2:

- Reproduction without the manifest: `chase-input-lab --smoke-test --config <folder>/controls.cfg` in a folder named `Zuerich-` plus a Cyrillic letter, a hyphen and two Japanese letters fails with `Cannot open controls`. The step that fails is `argv`: the argument arrives with `?` in place of each letter outside the code page (bytes `3f`), so `std::ifstream` and `std::filesystem::path(std::string)` get a wrong name. A probe program built from `std::ifstream` and `std::filesystem::exists` shows `argv` bytes `2d 3f 2d 3f 3f` and both calls false.
- With the manifest, `argv` carries the UTF-8 bytes (`d0 96`, `e6 97 a5 e6 9c ac`), `std::ifstream` and `std::filesystem::exists` find the file, and the program echoes the folder name unchanged in its error for a missing file.
- `cmake/windows_utf8.cmake` compiles `cmake/windows_utf8.rc` (the manifest `cmake/windows_utf8.manifest` with `activeCodePage` UTF-8) once through `windres` as an object library and, in a deferred call at the end of the top-level directory, adds it to every executable of every directory, test programs included. Libraries get no manifest (the program that loads them decides). No game or runtime code changed; Linux builds nothing of it.
- CTests `utf8-path-argument` and `utf8-path-argument-missing` (with `utf8-path-prepare`, `utf8-path-copy`, `utf8-path-cleanup`) run the real `chase-input-lab` from such a folder; both fail when the manifest is not linked (checked by taking it out once). On Linux `utf8-path-argument` is a skipped test with a reason.
- Requirement: Windows 10 version 1903 or later (the `activeCodePage` setting); recorded in [building.md](../../building.md#build-on-windows).
- Python tools: `tools/chase_native.py` sends UTF-8 paths to `libchase_assets_c.dll`, which runs inside `python.exe`, a process without the manifest: `chase_fib_open` for a path with such letters fails with `cannot open archive` (see WINPLAT-7).

## 4. Stand-ins

None.

## 5. Open items

- [ ] WINPLAT-5 — the save folder comes from `APPDATA` as a narrow string; with the UTF-8 manifest the save path should carry letters outside the ANSI code page unchanged on Windows 10 1903 and newer, but no test sets such an `APPDATA` and saves (not verified on Windows); before 1903 it stays ANSI (WINPLAT-8); needs: WINPLAT-8; evidence: [Verified behaviour](#3-verified-behaviour)
- [ ] WINPLAT-7 — the Python tools cannot open a path with letters outside the ANSI code page: `tools/chase_native.py` passes UTF-8 bytes to `libchase_assets_c.dll`, which opens them with the ANSI code page of `python.exe`. Question for the maintainer: convert the UTF-8 path in a Windows platform file of `chase_assets_c` (a plain interface, C++ only), or document that the Python tools need an ANSI-safe path? evidence: [§3](#3-verified-behaviour)
- [ ] WINPLAT-8 — Windows 10 before version 1903 ignores the manifest setting, so the programs there still read arguments in the ANSI code page. Question for the maintainer: is Windows 10 before 1903 supported (then a wide-argument start in a platform file is needed), or is 1903 the minimum? blocks: WINPLAT-5; evidence: [§3](#3-verified-behaviour)

## 6. Findings summary

Decisions: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437).

## 7. Code and tests

`src/save/file_replace*.{hpp,cpp}`, `src/save/save_directory*.{hpp,cpp}`, `src/runtime/process_cpu_time*.{hpp,cpp}`, `src/runtime/process_memory*.{hpp,cpp}`, `src/save/save.cpp`, `tools/chase_native.py`, `cmake/windows_utf8.cmake`, `cmake/windows_utf8.rc`, `cmake/windows_utf8.manifest`; tests `tests/file_replace_test.cpp`, `tests/process_cpu_time_test.cpp`, `tests/process_memory_test.cpp`, `tests/save_default_path_test.cpp` (POSIX), `tests/save_default_path_windows_test.cpp`, CTest `utf8-path-argument` (defined in `cmake/windows_utf8.cmake`).
