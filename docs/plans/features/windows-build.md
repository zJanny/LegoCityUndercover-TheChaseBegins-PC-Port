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

## 4. Stand-ins

None.

## 5. Open items

- [ ] WINBUILD-7 — first run of the Windows job on GitHub is green; fix what it reports; the job excludes `slow` and `needs-game-data` tests, and the OpenGL tests skip with a reason on the GPU-less runner once WINTEST-13 is merged (see [windows-tests](windows-tests.md)); first run to check: `update: false` of setup-msys2 with a stale package database, `$pythonLocation` and `python.exe` in the MSYS2 shell, `shell: bash` resolving to Git Bash, tests that skip locally without symlink rights and now run, a newer MSYS2 GCC with new warnings under `-Werror`; needs: WINBUILD-3; blocks: WINBUILD-8, WINTEST-23. evidence: `.github/workflows/ci.yml`
- [ ] WINBUILD-5 — question for the user: the spec names only the MSYS2 UCRT64 toolchain; the brief of 20261113-winopen-D took the folder from `MSYSTEM_PREFIX` when set, else `C:\msys64\ucrt64\bin`; in Git Bash `MSYSTEM_PREFIX` is Git's own `mingw64` prefix (no compiler, no Ninja), so the tool takes the `MSYSTEM_PREFIX` folder only when it holds `gcc` and otherwise the default folder. Is that reading right, and may the tool also look elsewhere (a `PATH` entry with `gcc`, another drive)? evidence: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437)
- [ ] WINBUILD-8 — Question for the user: should the Windows job cache the MSYS2 packages (a cache action keyed on the package list), or is installing them on every run acceptable? The spec does not decide; none is cached now; needs: WINBUILD-7; evidence: `.github/workflows/ci.yml`
- [ ] WINTEST-23 — Question for the user: should the nightly full workflow (`.github/workflows/full.yml`, which runs the `slow` tests) get a Windows job too, or do the `slow` tests run on Windows only locally? The spec decides only `ctest -LE slow` for the pull-request job; `full.yml` is unchanged; needs: WINBUILD-7; evidence: `.github/workflows/full.yml`

## 6. Findings summary

Decisions: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437). Windows setup for users: [building.md](../../building.md#build-on-windows).

## 7. Code and tests

`CMakeLists.txt`, `cmake/sdl2.cmake`, `cmake/math_constants.cmake`, `cmake/platform_files.cmake`, `tools/merge_agent.py`, `.github/workflows/ci.yml`.
