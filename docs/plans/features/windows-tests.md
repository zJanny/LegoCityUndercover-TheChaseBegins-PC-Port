# Windows: tests and tools

## 1. Scope

Running all tests and tools on Windows; skips with reasons.

## 2. Status

IN_PROGRESS. Full `ctest` on Windows with the extracted game data: 635 tests, 633 pass and 2 skip off main (as on Linux). The tests that start `chase-game` or `chase-viewer` and the 5 picture checks run on the graphics card through a hidden window. Not verified on a machine without a graphics card (WINTEST-14).

## 3. Verified behaviour

Measured on Windows 11 by 20261113-winopen-A, 2026-11-13, `ctest --test-dir build -j4 --output-on-failure` with MSYS2 UCRT64 first on the `PATH`, the extracted game data and `-DPython3_EXECUTABLE` set to the python.org Python, on a graphics card with OpenGL 4.6 (WGL context in a hidden window):

- First full run, before the save folder change (WINPLAT-1) was merged: 623 pass, 3 skip, 1 fails and 1 does not run. `game-save-new-game` failed because its pinned save path is joined with `/` and the Windows save code joins with a backslash; `game-save-reload` needs its save file. Every other test passed, among them the 112 `chase-game` and `chase-viewer` tests, the 5 picture checks (`debug-overlay-area`, `tip-text-panel`, `waypoint-marker-position`, `police-entrance-no-disc`, `front-overlay-area`) and every game scene test with pinned movement values.
- After merging main (save folder `%APPDATA%\chase`, UTF-8 manifest) and the per-test `APPDATA`: all 635 tests pass or skip off main, among them `game-save-new-game` and `game-save-reload`, with no platform skip list. Their output check accepts `/` or `\` before `chase` and `savegame.dat`, because the Windows path mixes both. Total time 585 s; 54 tests are labelled `slow`.
- Mechanism: on Windows `chase_use_hidden_window_for_gl_tests` (`cmake/platform_files.cmake`) rewrites the environment of every test that has `SDL_VIDEODRIVER=offscreen` to `SDL_VIDEODRIVER=windows` plus `CHASE_HIDDEN_WINDOW=1`, and adds the skip pattern `no OpenGL 4.6 context` to every test. `chase-game` and `chase-viewer` add `hiddenWindowFlagsFromEnvironment()` (`src/render/window_creation.cpp`) to their window flags: unset or `0` shows the window, `1` hides it, any other value stops with `CHASE_HIDDEN_WINDOW: expected 0 or 1, got '<text>'` (tests `render-window-creation*`, `game-hidden-window-rejects`). No command-line option was added; the Linux tests and the game with default options are unchanged. Reason for an environment switch: only the test setup needs it, and it adds no documented option.
- A missing OpenGL 4.6 context: `chase-game` and `chase-viewer` stop with `no OpenGL 4.6 context: <SDL error>` when the window or the context cannot be created, and CTest turns that output into a skip. Provoked with `SDL_VIDEODRIVER=dummy` (SDL has no OpenGL window there): CTest reports the test as skipped. The window tests of `chase_render` keep their exit code 77. A driver that hands out a context below 4.6 (Microsoft's OpenGL 1.1 on the CI runner, where GitHub Windows run 43387159801 showed `render-ui-draw-window` and `render-frame-keep-window` aborting because libepoxy found no `glCreateProgram` or `glGenFramebuffers`) is caught the same way: `currentGlContextProblem` reads `GL_VERSION` after the context is created and the programs and tests stop with `no OpenGL 4.6 context: the context is OpenGL 1.1, 4.6 is needed` (20261113-windlls-D, 2026-11-13; unit test `render-window-creation` with the version strings of 4.6, 4.5, 3.3, 1.1 and malformed ones; not yet seen on the CI runner).
- Other ways to get OpenGL without a window were ruled out: Mesa llvmpipe for Windows offers OpenGL 4.5 only, ANGLE offers OpenGL ES only, and Mesa's d3d12 driver on Windows is an extra dependency. The hidden-window approach is approved ([windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437)).
- `progress-readme` and `priorities-current` skip off main, as on Linux.
- `save-default-path` runs `tests/save_default_path_windows_test.cpp` on Windows (WINPLAT-1) and no longer skips.
- `fmv-real-smoke` runs with the Windows video driver and shows a window while it runs (WINTEST-15).
- Test isolation: on Windows every test gets `APPDATA=<build>/test-xdg/<test>` next to `XDG_DATA_HOME` (`chase_isolate_test_appdata` in `cmake/platform_files.cmake`, run at the end of configuring so it also covers the tests of the subdirectories and the deferred ones), so a test that saves writes to `<build>/test-xdg/<test>/chase/savegame.dat` and never to the real `%APPDATA%\chase`. CMake stops when a test sets `APPDATA` itself, and the test `test-appdata-isolated` (`cmake/check_test_appdata.cmake`) fails when the `APPDATA` of its own environment lies outside `<build>/test-xdg` (checked by hand with the real Roaming folder: it fails).

## 4. Stand-ins

None.

## 5. Open items

- [ ] WINTEST-2 — floating-point equality with MSYS2 GCC and `-ffp-contract=off`: the pinned values of all passing tests hold unchanged, among them the game scene tests; the cause analysis is still open; needs: WINTEST-13; evidence: [Verified behaviour](#3-verified-behaviour)
- [ ] WINTEST-14 — the version check after context creation (added after two window tests crashed on the CI runner with only Microsoft's OpenGL 1.1) is not yet seen on that runner: check on the next GitHub Windows job that every such test skips with `no OpenGL 4.6 context` and none fails; needs: WINBUILD-3, WINBUILD-7; evidence: [Verified behaviour](#3-verified-behaviour)
- [ ] WINTEST-15 — `fmv-real-smoke` (`chase-fmv`, SDL renderer, no OpenGL) shows a visible window on Windows because `chase-fmv` does not read `CHASE_HIDDEN_WINDOW`; question for the user: may `chase-fmv` read the same switch (one more line of glue, and `chase-fmv` then links `chase_render`)?; evidence: [Verified behaviour](#3-verified-behaviour)

## 6. Findings summary

Decisions: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437). Test commands on Windows: [building.md](../../building.md#build-on-windows).

## 7. Code and tests

`tests/render_window_creation_test.cpp`, `src/render/window_creation.cpp`, `CMakeLists.txt`, `cmake/platform_files.cmake` (`chase_use_hidden_window_for_gl_tests`, `chase_isolate_test_appdata`, `chase_platform_skip_test`), `cmake/check_test_appdata.cmake`.
