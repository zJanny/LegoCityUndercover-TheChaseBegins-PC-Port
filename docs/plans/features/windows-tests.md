# Windows: tests and tools

## 1. Scope

Running all tests and tools on Windows; skips with reasons.

## 2. Status

IN_PROGRESS. Full `ctest` on Windows: 624 tests, 500 pass, 119 skip with a reason, 5 are disabled, none fail. The game scene tests cannot run, because SDL's offscreen driver has no OpenGL on Windows.

## 3. Verified behaviour

Measured on Windows 11 by 20261112-winhooks-E, 2026-11-12, `ctest --test-dir build -j16 --output-on-failure` in the UCRT64 shell with the extracted game data and `-DPython3_EXECUTABLE` set to the python.org Python:

- 500 tests pass, among them every unit, runtime and real-data test that runs without a window; their pinned values are unchanged.
- 112 tests that start `chase-game` or `chase-viewer` with `SDL_VIDEODRIVER=offscreen` skip: SDL prints `Could not load EGL library`, and CMake marks that output as a skip on Windows (WINTEST-13). The 5 tests that check their pictures (`debug-overlay-area`, `tip-text-panel`, `waypoint-marker-position`, `police-entrance-no-disc`, `front-overlay-area`) are disabled when CMake finds no `libEGL.dll`.
- Other ways to get OpenGL without a window were ruled out: Mesa llvmpipe for Windows offers OpenGL 4.5 only, ANGLE offers OpenGL ES only, and Mesa's d3d12 driver on Windows is an extra dependency. The hidden-window approach is approved ([windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437)).
- `render-ui-draw-window`, `render-frame-keep-window` and `qol-ui-menu-window-real-data` skip with exit code 77 (no OpenGL 4.6 context offscreen), as on a Linux machine without a display.
- `progress-readme` and `priorities-current` skip off main, as on Linux.
- `save-default-path` runs `tests/save_default_path_windows_test.cpp` on Windows (WINPLAT-1) and no longer skips.

## 4. Stand-ins

None.

## 5. Open items

- [ ] WINTEST-2 — floating-point equality with MSYS2 GCC and `-ffp-contract=off`: the pinned values of the 500 passing tests hold unchanged; the game scene tests with pinned movement values are still unchecked; needs: WINBUILD-1, WINTEST-13; evidence: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437)
- [ ] WINTEST-13 — the 112 offscreen OpenGL tests and their 5 picture checks do not run on Windows: SDL's offscreen driver needs `libEGL.dll`, which no listed MSYS2 package ships; on Windows these tests use SDL's normal Windows video driver with a hidden window instead of `SDL_VIDEODRIVER=offscreen`, which gives a WGL context with OpenGL 4.6 on the graphics card (a Windows-only test setup change, approach approved by the user 2026-11-12); blocks: WINTEST-2; evidence: [Verified behaviour](#3-verified-behaviour)

## 6. Findings summary

Decisions: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437). Test commands on Windows: [building.md](../../building.md#build-on-windows).

## 7. Code and tests

`tests/`, `CMakeLists.txt`, `cmake/platform_files.cmake` (`chase_skip_tests_without_offscreen_gl`, `chase_platform_skip_test`).
