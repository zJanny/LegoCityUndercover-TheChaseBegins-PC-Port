# Windows: build

## 1. Scope

MSYS2 UCRT64 GCC build with Ninja: dependency discovery and flags.

## 2. Status

IN_PROGRESS. The project configures and builds warning-free on Windows 11 with MSYS2 UCRT64 GCC and Ninja; all CTest tests pass or skip (see [windows-tests](windows-tests.md)). The GitHub Windows job is written in `.github/workflows/ci.yml` and has not run on GitHub yet.

## 3. Verified behaviour

Measured on Windows 11 by 20261112-winhooks-E, 2026-11-12, MSYS2 UCRT64 with GCC 16.2, CMake 4.4, Ninja 1.13:

- `pkg_check_modules` finds SDL2, libepoxy and the five FFmpeg libraries of the MSYS2 packages unchanged. SDL2 is `sdl2-compat` 2.32 on SDL3 3.4: MSYS2's `ffmpeg` depends on it, and it conflicts with the classic `SDL2` package.
- MSYS2's `sdl2.pc` adds `SDL2main`, `mingw32` and `-mwindows`; `cmake/sdl2.cmake` drops them and defines `SDL_MAIN_HANDLED`, so every program keeps its own `main()` and its console output.
- MinGW's `<cmath>` defines `M_PI` only with `_USE_MATH_DEFINES` (`cmake/math_constants.cmake`), because the build uses `-std=c++17` without GNU extensions.
- `find_package(Python3 3.11)` picks the Python of the CTest Python tests. In the UCRT64 shell MSYS2's Python (an FFmpeg dependency) comes first, so the docs pin the python.org one with `-DPython3_EXECUTABLE`.
- `libchase_assets_c.dll` is linked with `-static`, so it needs only system DLLs and a python.org Python loads it through ctypes.
- The build with `-ffp-contract=off` and `-Wall -Wextra -Wpedantic` gives no warning.
- `tools/merge_agent.py` configures with `-DPython3_EXECUTABLE` set to its own Python.
- Measured by 20261113-winopen-D, 2026-11-13, Windows 11, python.org Python 3.11, Git Bash: `MSYSTEM_PREFIX` is `C:/Program Files/Git/mingw64` in Git Bash (Git's own prefix, no compiler there), `C:/msys64/ucrt64` for a native program started from the MSYS2 UCRT64 shell, and unset in PowerShell.
- `tools/merge_agent.py` (`windows_tool_environment`, used by `build_warning_free`, `run_checks` and `check-pr`) starts its child processes on Windows with the folder of its own Python and then the MSYS2 UCRT64 `bin` folder in front of the caller's `PATH`, unless that folder is already on the `PATH`, and sets `CMAKE_GENERATOR=Ninja` when it is unset and `ninja` is found. The folder is the `bin` folder under `MSYSTEM_PREFIX` when that holds `gcc`, else `C:\msys64\ucrt64\bin` when it exists. Linux passes its environment on unchanged. Unit tests with an injected environment: `tests/test_merge_agent.py` `WindowsToolEnvironmentTest`.
- Measured by 20261113-windlls-A, 2026-11-13, Windows 11, MSYS2 UCRT64 GCC 16.2, CMake 4.4: the build copies the 102 non-system DLLs (147 MB, 103 files with the project's own library) that the 401 programs and the library import, transitively, from the folder of the compiler into the build folder. The first run reads the import tables of all programs with `objdump -p` (about 30 s); later runs read only changed files (cache `runtime-dlls-imports.cache` by size and time, about 1 s) and copy with `copy_if_different`. A Ninja depfile lists the source DLLs, so a changed MSYS2 DLL is copied on the next build. A DLL found in no searched folder stops the build with its name (checked with an empty search folder). `sdl2-compat` loads `SDL3.dll` with `LoadLibrary`, which no import table shows: without it `chase-input-lab.exe` failed with 0xc0000142 when only the Windows folders were on the `PATH`, so `CHASE_RUNTIME_LOADED_DLLS` names it. With the copy, `build/chase-input-lab.exe` runs and `build/chase-game.exe` gets past DLL loading from PowerShell with only `System32` on the `PATH`. Tests `runtime-dlls-present` (slow) and `runtime-dlls-start-without-msys2` pass; on Linux the first registers as a skip.

## 4. Stand-ins

None.

## 5. Open items

- [ ] WINBUILD-7 — first run of the Windows job on GitHub is green; fix what it reports; the job excludes `slow` and `needs-game-data` tests, and the OpenGL tests have to skip with `no OpenGL 4.6 context` on the GPU-less runner (WINTEST-14, see [windows-tests](windows-tests.md)); first run to check: `update: false` of setup-msys2 with a stale package database, `$pythonLocation` and `python.exe` in the MSYS2 shell, `shell: bash` resolving to Git Bash, tests that skip locally without symlink rights and now run, a newer MSYS2 GCC with new warnings under `-Werror`; the first run (GitHub run 43380150738) built, but 4 Python tests failed because the runner's TEMP is an 8.3 short path while resolved paths are long (`check_doc_links.process` now resolves its root; the merge-tool test compares against the resolved temp dir), fixed, awaiting the next run; needs: WINBUILD-3; blocks: WINBUILD-8, WINTEST-14, WINTEST-23. evidence: `.github/workflows/ci.yml`
- [ ] WINBUILD-5 — question for the user: the spec names only the MSYS2 UCRT64 toolchain; the brief of 20261113-winopen-D took the folder from `MSYSTEM_PREFIX` when set, else `C:\msys64\ucrt64\bin`; in Git Bash `MSYSTEM_PREFIX` is Git's own `mingw64` prefix (no compiler, no Ninja), so the tool takes the `MSYSTEM_PREFIX` folder only when it holds `gcc` and otherwise the default folder. Is that reading right, and may the tool also look elsewhere (a `PATH` entry with `gcc`, another drive)? evidence: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437)
- [ ] WINBUILD-8 — Question for the user: should the Windows job cache the MSYS2 packages (a cache action keyed on the package list), or is installing them on every run acceptable? The spec does not decide; none is cached now; needs: WINBUILD-7; evidence: `.github/workflows/ci.yml`
- [ ] WINTEST-23 — Question for the user: should the nightly full workflow (`.github/workflows/full.yml`, which runs the `slow` tests) get a Windows job too, or do the `slow` tests run on Windows only locally? The spec decides only `ctest -LE slow` for the pull-request job; `full.yml` is unchanged; needs: WINBUILD-7; evidence: `.github/workflows/full.yml`

## 6. Findings summary

Decisions: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437), [windows-toolchain.md §2026-11-13T07:55:40](../../design/windows-toolchain.md#ts-20261113-075540) (DLLs next to the programs). Windows setup for users: [building.md](../../building.md#build-on-windows).

## 7. Code and tests

`CMakeLists.txt`, `cmake/sdl2.cmake`, `cmake/math_constants.cmake`, `cmake/platform_files.cmake`, `cmake/windows_runtime_dlls.cmake`, `cmake/copy_runtime_dlls.cmake`, `cmake/windows_dll_scan.cmake`, `cmake/check_runtime_dlls.cmake`, `cmake/start_without_msys2.cmake`, `tools/merge_agent.py`, `.github/workflows/ci.yml`.
