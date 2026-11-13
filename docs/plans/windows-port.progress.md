# Progress — Windows port

## Status

- Overall: IN_PROGRESS
- Current Step: S1
- Last Updated: 2026-11-12T20:54:37+01:00
- Plan: docs/plans/windows-port.md

## Decisions

### Decision: Windows toolchain and scope (2026-11-12T20:54:37+01:00)
MSYS2 UCRT64 GCC, MSYS2 packages, Ninja, no packaging, `%APPDATA%` saves, skip Linux-only tests with a reason. A GitHub Windows job follows the first local build; none on Forgejo. See [windows-toolchain.md](../design/windows-toolchain.md#ts-20261112-205437).

## Blockers

The user is on Linux; implementation starts when the user switches to Windows (S1 is theirs).

## Next Action

S1: the user sets up MSYS2 UCRT64 on the Windows partition. Then the first Windows session runs S2 (configure and build) and fixes the package list.

## Session Log

One line per session, newest last; `--fix` keeps the newest 20 and moves older ones to `windows-port.progress-archive.md`.

- 2026-11-12T22:09:12+01:00 — Agent 20261112-winhooks-B (hook paths): `tools/hook_paths.py` makes worktree detection, `cd /c/...` and the artifacts link (junction fallback) work on Windows; CONTRIBUTING gets a Windows section. See [windows-hook-paths](features/windows-hook-paths.md).
- 2026-11-12T22:12:15+01:00 — Agent 20261112-winhooks-D (line endings): forced LF with `.gitattributes`, LF-based size checks and LF writes in the doc tools, a guard test; verified on Windows. See [windows-line-endings](features/windows-line-endings.md).
- 2026-11-12T22:16:38+01:00 — Agent 20261112-winhooks-C (Python tools on Windows): Python tools call their own interpreter and use UTF-8, Windows-only tests skip with reasons, tests that run git isolate their environment. See [windows-python-tools](features/windows-python-tools.md).
- 2026-11-12T22:18:23+01:00 — Agent 20261112-winhooks-A (hooks on Windows): launcher `.claude/run_hook.cmd` for sh and cmd, `Bash|PowerShell` matcher, UTF-8 decoding, Windows library names, tests and docs; seven hook tests wait for the path fixes. See [windows-hooks](features/windows-hooks.md).
- 2026-11-12T23:12:05+01:00 — Agent 20261112-winhooks-E (first Windows build): S2 done, the project builds warning-free with MSYS2 UCRT64 and Ninja and all CTest tests pass or skip with a reason; platform files for the save write and CPU time; Windows setup in the user docs. See [windows-build](features/windows-build.md), [windows-platform](features/windows-platform.md), [windows-tests](features/windows-tests.md).
- 2026-11-13T00:26:30+01:00 — Lead (20261112-winhooks end): merged 20261112-winhooks-E (first MSYS2 build, platform files, Windows setup docs), -B (hook path handling, artifacts junction), -A (hook launcher for Git Bash and PowerShell, GIT_* deny), -D (LF safeguard, LF size checks) and -C (Python tools on Windows, git isolation and guard test) as one combined squash through tools/merge_agent.py with all tests passing on Windows and Linux green (WSL), per the user's decision; ID ranges released in the merge commit; pushed main to origin/main; the five worker worktrees and branches are kept until the user agrees to remove them. See [windows-build.md](features/windows-build.md), [windows-hooks.md](features/windows-hooks.md), [windows-hook-paths.md](features/windows-hook-paths.md), [windows-python-tools.md](features/windows-python-tools.md), [windows-line-endings.md](features/windows-line-endings.md), [windows-tests.md](features/windows-tests.md)
- 2026-11-13T00:46:10+01:00 — Agent 20261113-winopen-G (windows ci job): GitHub job `windows-build-and-test` (MSYS2 UCRT64, Ninja, warnings as errors, Python tests, `ctest -LE 'slow|needs-game-data'`) and a clone check with `core.autocrlf=true` in `.github/workflows/ci.yml`; commands run by hand on Windows, the job itself has not run on GitHub. See [windows-build](features/windows-build.md), [windows-line-endings](features/windows-line-endings.md).
- 2026-11-13T00:49:17+01:00 — Agent 20261113-winopen-F (hook root paths): hooks map MSYS root paths such as /tmp with cygpath on Windows (unmapped when cygpath fails); launcher checked from PowerShell with a space in the path. See [windows-hook-paths](features/windows-hook-paths.md).
- 2026-11-13T00:53:27+01:00 — Agent 20261113-winopen-E (docs checker and skill text): the user docs check accepts listed options of external programs (wsl, gcc, cmake, git and others), the Windows docs use their common forms again, the lead-round skill and worker brief carry the Windows forms; the flaky touch test did not fail in 70 WSL runs and is an open item. See [windows-python-tools](features/windows-python-tools.md).
- 2026-11-13T01:02:31+01:00 — Agent 20261113-winopen-D (python tools on windows): merge_agent removes the artifacts junction before a worktree (git followed it and emptied the data), finds the MSYS2 tools and Ninja itself, chase_native picks the DLL on Windows. See [windows-python-tools](features/windows-python-tools.md).
- 2026-11-13T01:04:19+01:00 — Agent 20261113-winopen-B (save folder and memory): the save path is `%APPDATA%\chase\savegame.dat` on Windows and the overlay memory line reads the working set, each in its own platform file with a test; verified on Windows. See [windows-platform](features/windows-platform.md).
- 2026-11-13T01:08:32+01:00 — Agent 20261113-winopen-C (UTF-8 paths): Windows programs read their paths as UTF-8 through a manifest in every executable, with CTests for it on Windows; how far it got: done for the programs, Python tools still open. See [windows-platform](features/windows-platform.md)
- 2026-11-13T02:02:20+01:00 — Agent 20261113-winopen-H (merge tools and UTF-8): the merge tools decode git output and encode commit messages as UTF-8 on Windows, a test runs the conflict merge under a non-UTF-8 default encoding, and the damaged progress file is repaired. See [windows-python-tools](features/windows-python-tools.md).
- 2026-11-13T02:34:55+01:00 — Agent 20261113-winopen-A (offscreen GL tests): on Windows the offscreen OpenGL tests and the 5 picture checks run with SDL's Windows video driver and a hidden window (`CHASE_HIDDEN_WINDOW`, strict parser and tests), a missing OpenGL 4.6 context skips, all tests pass with the real data, and every test runs in its own APPDATA folder on Windows. See [windows-tests](features/windows-tests.md).
- 2026-11-13T07:38:03+01:00 — Lead (20261113-winopen end): merged 20261113-winopen-D (junction-safe worktree removal, Windows tool environment, DLL name), -E (doc checker options of external programs, skill), -F (MSYS root paths in hooks), -G (Windows CI job, not yet run on GitHub), -C (UTF-8 manifest), -H (UTF-8 subprocess decoding in tools), -B (save in %APPDATA%\chase, process memory) and -A (hidden-window OpenGL tests, per-test APPDATA) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. Follow-ups: the progress-log conflict still needs a hand merge every time; Windows builds with debug info take about 12 GB each. See [windows-tests.md](features/windows-tests.md), [windows-platform.md](features/windows-platform.md), [windows-build.md](features/windows-build.md), [windows-python-tools.md](features/windows-python-tools.md)
- 2026-11-13T08:13:30+01:00 — Agent 20261113-windlls-B (short temp path): `check_doc_links.process` resolves its root so a short 8.3 name, junction or symlink works, a link-root regression test, the merge-tool lock test uses the resolved temp dir. See [windows-build](features/windows-build.md).
- 2026-11-13T08:15:36+01:00 — Agent 20261113-windlls-A (DLLs next to the programs): the Windows build copies the DLLs the programs need from the compiler folder into the build folder, so they start without MSYS2 on the PATH; two CTests on Windows, user docs updated; verified on Windows. See [windows-build](features/windows-build.md).
- 2026-11-13T08:24:36+01:00 — Agent 20261113-windlls-C (Windows zip): `tools/package_windows.py` builds the zip of programs, DLLs, extraction tools and per-DLL license files and refuses every game-derived file, with tests; the Windows CI job packages, smoke-starts the unzipped programs with only System32 on the PATH, uploads the artifact and publishes on a `v*` tag; not run on GitHub yet. See [windows-build](features/windows-build.md).
- 2026-11-13T09:42:20+01:00 — Agent 20261113-windlls-D (no-GPU window tests): a shared OpenGL version check after context creation makes `chase-game`, `chase-viewer` and the window tests skip on a machine without OpenGL 4.6; verified on Windows with a graphics card, not yet on the CI runner. See [windows-tests](features/windows-tests.md).
- 2026-11-13T10:12:14+01:00 — Lead (20261113-windlls end): merged 20261113-windlls-B (doc checker root resolved for the Windows CI runner), -A (MSYS2 runtime DLLs copied into the Windows build folder), -C (Windows release zip: artifact on main, release on v* tags, no game data) and -D (OpenGL programs skip cleanly below 4.6) through tools/merge_agent.py with all tests passing; pushed main to origin/main and GitHub after each merge. See [windows-build.md](features/windows-build.md), [windows-tests.md](features/windows-tests.md)
