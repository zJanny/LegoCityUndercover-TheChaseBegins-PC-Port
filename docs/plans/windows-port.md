# Plan: Windows port

## 1. Summary
Build and run the faithful port natively on Windows x86_64 with the same behaviour and the same tests as Linux. Platform differences live in platform files behind plain interfaces. It adds no game behaviour and no packaging.

## 2. Gate
Opened by the user on 2026-11-12T20:51:55+01:00: "I open the windows gate. I am currently on linux, so just do scouting what we need for a windows port. I will then start implementing once i switched to windpws".

## 3. Requirements
- R1: The project configures and builds warning-free with MSYS2 UCRT64 GCC and Ninja; accept: the CMake build of CLAUDE.md "Commands" in the MSYS2 shell.
- R2: Saves go to `%APPDATA%\<name>` through a Windows platform file; accept: the save tests pass on Windows.
- R3: The Python tools find the native library on Windows; accept: the tests using `tools/chase_native.py` run, not skip.
- R4: All ctest tests pass or are skipped with a reason in their CTest properties (Azahar oracle, Xvfb); accept: full ctest on Windows, skips listed in the feature file.
- R5: Same behaviour as Linux, including floating-point results with `-ffp-contract=off`; accept: the pinned-value tests pass unchanged.
- R6: Linux stays green; accept: `Linux green (WSL)` for every change to shared code.

## 4. Scope and non-goals
In scope: MSYS2 build, a Windows job in the GitHub workflow after the first local build, dependency discovery, platform files (save directory, library loading), test and tool portability, the docs of Windows commands. Out of scope: packaging, installers, a Windows job on Forgejo (no hosted Windows runners), MSVC and other toolchains, any game behaviour. Game logic stays the port's job.

## 5. Design decisions
- Toolchain, dependencies, commands, packaging, saves, tests: [docs/design/windows-toolchain.md §2026-11-12T20:54:37](../design/windows-toolchain.md#ts-20261112-205437)

## 6. Steps
- S1 (R1): on the Windows partition install MSYS2 UCRT64 with the packages, clone with `core.autocrlf=false`, extract the game data; needs: none; verify: `cmake --version`, `gcc --version` in the UCRT64 shell.
- S2 (R1): isolate dependency discovery (`pkg_check_modules` of SDL2, epoxy, FFmpeg) so it works with MSYS2 package manager, and build warning-free with Ninja; needs: S1; verify: the build command. Items: WINBUILD-1.
- S3 (R2, R3): platform files for the save directory (`src/save/save.cpp` uses `<unistd.h>`, XDG and HOME) and for the native library name in `tools/chase_native.py`; needs: S2; verify: save and binding tests. Items: WINPLAT-1, WINPLAT-2.
- S4 (R4, R5): run all ctest and the Python tests on Windows, mark Linux-only tests with skip reasons, check floating-point results; needs: S3; verify: full ctest. Items: WINTEST-1, WINTEST-2.
- S5 (R6): after every shared-code change the WSL check into `build-wsl`; needs: S2; verify: `Linux green (WSL)`.
- S6: update `docs/commands.md`, `docs/building.md` and the README path with the Windows commands; needs: S4; verify: `doc-links`.

## 7. Verification
Target: the user's Windows partition, MSYS2 UCRT64 shell. A change counts as verified for Windows only when built and tested there; otherwise "not verified on Windows". The port's tests stay unchanged and pass.

## 8. Rules
Rule file: [docs/rules/windows.md](../rules/windows.md). Features: under `## Windows port` in [features/README.md](features/README.md).
