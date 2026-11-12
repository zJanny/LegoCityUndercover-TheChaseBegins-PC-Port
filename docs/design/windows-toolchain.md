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
- CI: "GitHub job after local build (Recommended)" (2026-11-12, after the lead's web check). Add a Windows job to `.github/workflows/ci.yml` (`msys2/setup-msys2@v2`, msystem UCRT64, MSYS2 packages, Ninja, `ctest -LE slow`) once the first local Windows build passes. Forgejo gets no Windows job, because it has no hosted Windows runners. Facts checked: `windows-latest` is Windows Server 2025 with VS 2026; standard GitHub-hosted runners are free in public repos and Windows minutes count double in private repos; the runners have no GPU, so render tests need Mesa llvmpipe or a skip; game-data tests skip in CI.

Open: the exact MSYS2 package list and the `<name>` of the save folder, to be fixed by the first Windows session with the user.
