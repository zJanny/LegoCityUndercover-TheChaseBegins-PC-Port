# Windows: build

## 1. Scope

MSYS2 UCRT64 GCC build with Ninja: dependency discovery and flags.

## 2. Status

IN_PROGRESS. The project configures and builds warning-free on Windows 11 with MSYS2 UCRT64 GCC and Ninja; all CTest tests pass or skip (see [windows-tests](windows-tests.md)). The GitHub Windows job is still open.

## 3. Verified behaviour

Measured on Windows 11 by 20261112-winhooks-E, 2026-11-12, MSYS2 UCRT64 with GCC 16.2, CMake 4.4, Ninja 1.13:

- `pkg_check_modules` finds SDL2, libepoxy and the five FFmpeg libraries of the MSYS2 packages unchanged. SDL2 is `sdl2-compat` 2.32 on SDL3 3.4: MSYS2's `ffmpeg` depends on it, and it conflicts with the classic `SDL2` package.
- MSYS2's `sdl2.pc` adds `SDL2main`, `mingw32` and `-mwindows`; `cmake/sdl2.cmake` drops them and defines `SDL_MAIN_HANDLED`, so every program keeps its own `main()` and its console output.
- MinGW's `<cmath>` defines `M_PI` only with `_USE_MATH_DEFINES` (`cmake/math_constants.cmake`), because the build uses `-std=c++17` without GNU extensions.
- `find_package(Python3 3.11)` picks the Python of the CTest Python tests. In the UCRT64 shell MSYS2's Python (an FFmpeg dependency) comes first, so the docs pin the python.org one with `-DPython3_EXECUTABLE`.
- `libchase_assets_c.dll` is linked with `-static`, so it needs only system DLLs and a python.org Python loads it through ctypes.
- The build with `-ffp-contract=off` and `-Wall -Wextra -Wpedantic` gives no warning.
- `tools/merge_agent.py` configures with `-DPython3_EXECUTABLE` set to its own Python.

## 4. Stand-ins

None.

## 5. Open items

- [ ] WINBUILD-3 — Windows job in `.github/workflows/ci.yml` (`msys2/setup-msys2@v2`, UCRT64, Ninja, `ctest -LE slow`), added once the first local Windows build passes; render tests need Mesa llvmpipe or a skip, game-data tests skip; needs: WINBUILD-1; evidence: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437)
- [ ] WINBUILD-4 — `tools/merge_agent.py` neither chooses Ninja nor finds the MSYS2 tools on Windows (`build_warning_free`, `run_checks`); the caller sets `PATH` and `CMAKE_GENERATOR` (command in [commands-windows.md](../../commands-windows.md#merging-an-agent-branch)); evidence: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437)

## 6. Findings summary

Decisions: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437). Windows setup for users: [building.md](../../building.md#build-on-windows).

## 7. Code and tests

`CMakeLists.txt`, `cmake/sdl2.cmake`, `cmake/math_constants.cmake`, `cmake/platform_files.cmake`, `tools/merge_agent.py`.
