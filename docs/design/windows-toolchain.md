# Windows: toolchain, dependencies, commands, packaging, saves, tests

The user's approved decisions for the `windows-port` plan. Plan: [windows-port.md](../plans/windows-port.md); rules: [windows.md](../rules/windows.md); feature files: [windows-build.md](../plans/features/windows-build.md), [windows-platform.md](../plans/features/windows-platform.md), [windows-tests.md](../plans/features/windows-tests.md).

<a id="ts-20261112-205437"></a>

## 2026-11-12T20:54:37+01:00 — Windows build decisions

Asked of the user by the planner on 2026-11-12 (relayed by the lead); the quoted labels are the user's selections.

- Toolchain: "MSYS2 UCRT64 GCC (Recommended)". GCC with the flags of the Linux build, including `-ffp-contract=off`.
- Dependencies (SDL2, epoxy, FFmpeg, pkg-config): "MSYS2 package manager (Recommended)". No vendored binaries.
- Commands: "Ninja in MSYS2 shell (Recommended)". The CMake and ctest commands are those of [commands.md](../commands.md) run from the MSYS2 UCRT64 shell with the Ninja generator.
- Packaging: "None for now (Recommended)". The game runs from the build folder.
- Save directory: "%APPDATA%\<name> (Recommended)", in a platform file with the same interface as the Linux one.
- Tests that cannot run on Windows (Azahar oracle, Xvfb and similar): "Skip with reason (Recommended)", each skip listed in the feature file.
- ID prefixes: "WINBUILD/WINPLAT/WINTEST (Recommended)".
- CI: open. The user asked (2026-11-12) to check what GitHub offers for Windows CI and report back; the decision is the user's. No Windows workflow is added until they decide.

Open: Windows CI (above); the exact MSYS2 package list and the `<name>` of the save folder, to be fixed by the first Windows session with the user.
