# Windows: toolchain, dependencies, commands, packaging, saves, tests

The user's approved decisions for the `windows-port` plan. Plan: [windows-port.md](../plans/windows-port.md); rules: [windows.md](../rules/windows.md); feature files: [windows-build.md](../plans/features/windows-build.md), [windows-platform.md](../plans/features/windows-platform.md), [windows-tests.md](../plans/features/windows-tests.md).

<a id="ts-20261112-205437"></a>

## 2026-11-12T20:54:37+01:00 — Windows build decisions

Asked of the user by the planner on 2026-11-12 (relayed by the lead); the quoted labels are the user's selections.

- Toolchain: "MSYS2 UCRT64 GCC (Recommended)". GCC with the flags of the Linux build, including `-ffp-contract=off`.
- Dependencies (SDL2, epoxy, FFmpeg, pkg-config): "MSYS2 package manager (Recommended)". No vendored binaries.
- Commands: "Ninja in MSYS2 shell (Recommended)". The CMake and ctest commands are those of [commands.md](../commands.md) run from the MSYS2 UCRT64 shell with the Ninja generator.
- Save directory: "%APPDATA%\<name> (Recommended)", in a platform file with the same interface as the Linux one.
- Tests that cannot run on Windows (Azahar oracle, Xvfb and similar): "Skip with reason (Recommended)", each skip listed in the feature file.
- ID prefixes: "WINBUILD/WINPLAT/WINTEST (Recommended)".
- CI: "GitHub job after local build (Recommended)" (2026-11-12, after the lead's web check). Add a Windows job to `.github/workflows/ci.yml` (`msys2/setup-msys2@v2`, msystem UCRT64, MSYS2 packages, Ninja, `ctest -LE slow`) once the first local Windows build passes. Forgejo gets no Windows job, because it has no hosted Windows runners. Facts checked: `windows-latest` is Windows Server 2025 with VS 2026; standard GitHub-hosted runners are free in public repos and Windows minutes count double in private repos; the runners have no GPU, so render tests need Mesa llvmpipe or a skip; game-data tests skip in CI.

- Package list (2026-11-12, asked by the first Windows session, relayed by the lead): "Accept, drop python" ("Use the Windows python.org Python instead of MSYS2's python for the tools; the list keeps the rest"). SDL (2026-11-12, after the first install showed that MSYS2's `ffmpeg` depends on `sdl2-compat`, which conflicts with the classic `SDL2` package): "SDL2 → sdl2-compat (Recommended)". The MSYS2 UCRT64 packages are `gcc`, `cmake`, `ninja`, `pkgconf`, `sdl2-compat`, `libepoxy` and `ffmpeg` (`mingw-w64-ucrt-x86_64-*`); the SDL2 API comes from `sdl2-compat` on SDL3. The Python tools use the python.org Python 3.11 or newer for Windows. Git for Windows is used for cloning. MSYS2's own Python still comes in as a dependency of `ffmpeg`, so it is first on the PATH of the UCRT64 shell.
- Python for the tests (2026-11-12, relayed by the lead): "(B) CMake picks Python" ("CMake finds one Python 3.11+ (find_package(Python3)) and every Python test uses it; -DPython3_EXECUTABLE= can pin the python.org one. Shared CMake change, Linux behaviour unchanged").
- Save folder name (2026-11-12, relayed by the lead): "chase (Recommended)" ("%APPDATA%\chase\savegame.dat — same name as Linux, short, matches the chase-* program names"). The save file on Windows is `%APPDATA%\chase\savegame.dat`.
- Platform source for Windows (2026-11-12, relayed by the lead): asked "The Windows build is blocked by debug_metrics.cpp (getrusage) and save.cpp (POSIX atomic save write). What should worker E do?", the user chose "(b) Port both (Recommended)": "Platform files for CPU time and the atomic save write; complete build, so merges become possible this round. Save folder name stays open." The atomic save write and the process CPU time of the debug overlay get Windows files next to the POSIX ones ([windows-platform.md](../plans/features/windows-platform.md)).
- Four failing tests on Windows (2026-11-12, relayed by the lead): `qol-controls-file` (the test keeps files open), `qol-controls-bindings` (folder found only by `/`), `qol-pc-vehicle-controls` (rewrite while reading) and `audio-real-data` (POSIX command line). Asked what worker E should do with them, the user chose "Fix all four": "Also fix the two test sources (close files in the test; portable launch in audio_real_test). More work for E, nothing skipped." Earlier, asked "May E fix that one line in QoL code (std::filesystem::rename, same error message, same Linux behaviour)?", the user chose "Yes, fix the line (Recommended)" ([windows-tests.md](../plans/features/windows-tests.md)).
- Linux check in WSL (2026-11-12, relayed by the lead): "If this works, document it in the windows setting up docs". The WSL steps with the D3D12 driver are in [building.md](../building.md#linux-tests-in-wsl).
- Offscreen OpenGL tests on Windows (2026-11-12, relayed by the lead): "Yes, put it on the list" for running them with SDL's normal Windows video driver and a hidden window instead of `SDL_VIDEODRIVER=offscreen` (WINTEST-13, [windows-tests.md](../plans/features/windows-tests.md)).

Open: none.

<a id="ts-20261113-075540"></a>

## 2026-11-13T07:55:40+01:00 — Programs start from the build folder

Asked of the user by the lead on 2026-11-13: started from the Explorer, `build/chase-game.exe` reports missing DLLs (`SDL2.dll`, `libepoxy-0.dll`, `libstdc++-6.dll`, `avcodec-63.dll` and others), because they live in the MSYS2 UCRT64 `bin` folder, which only the UCRT64 window has on its `PATH`. The lead offered: "after building, copy the needed DLLs from MSYS2 into build/ next to the .exe files. Then the build folder works on its own". The user answered: "Please fix this now" and "So that i can run it from the build folder".

- The Windows build copies the non-system DLLs that every program and library of the build needs, transitively, from the folder of the compiler into the build folder next to the `.exe` files. Every program then starts from the Explorer, PowerShell or Git Bash without MSYS2 on the `PATH`. Windows system DLLs are not copied.

Open: none.

<a id="ts-20261113-082335"></a>

## 2026-11-13T08:23:35+01:00 — Windows zip for the Releases tab

Asked by the lead in the session of 2026-11-13; the user's words and the labels they chose.

- Wish (2026-11-13, the maintainer): "make it so the windows pipeline on main will produce a .zip file with the exe that can just be started anywhere in the releases tab to download. NO GAME ASSETS OR COPYRIGHTED MATERIAL!!!! inside it. You will still have to extract the game data".
- Release type (2026-11-13): "Only on a version tag". The zip is published on the GitHub Releases tab only when a tag like `v0.1.0` is pushed.
- Zip content (2026-11-13): "Programs + extraction tools". Besides the programs and their DLLs, the zip holds the Python extraction tools. The zip alone covers extract, then start; it still needs Python 3.11 or newer installed.
- Question (2026-11-13, asked by the lead): "On every push to main, should GitHub keep the Windows zip as a downloadable CI artifact? Releases still appear only on a v* tag." User: "Yes, keep it". The Windows job builds the zip on every push to `main` and uploads it as a workflow artifact, so the packaging is tested.
- Question (2026-11-13, asked by the lead): should the zip contain a `licenses/` folder with the bundled packages' license texts, plus a `README.txt` (not playable yet, the not-affiliated notice, extraction and start steps, the source commit link)? User: "Yes, both".
- No game data, ever: the packaging tool takes only files an allow rule names and refuses everything game-derived ([windows-build.md](../plans/features/windows-build.md)).


Open: none.
