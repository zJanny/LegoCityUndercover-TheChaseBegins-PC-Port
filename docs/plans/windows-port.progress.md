# Progress â€” Windows port

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

- 2026-11-12T20:54:37+01:00 â€” Planner: opened the windows-port plan after scouting the Linux checkout. See [windows-build](features/windows-build.md).
- 2026-11-12T22:09:12+01:00 â€” Agent 20261112-winhooks-B (hook paths): `tools/hook_paths.py` makes worktree detection, `cd /c/...` and the artifacts link (junction fallback) work on Windows; CONTRIBUTING gets a Windows section. See [windows-hook-paths](features/windows-hook-paths.md).
- 2026-11-12T22:12:15+01:00 â€” Agent 20261112-winhooks-D (line endings): forced LF with `.gitattributes`, LF-based size checks and LF writes in the doc tools, a guard test; verified on Windows. See [windows-line-endings](features/windows-line-endings.md).
- 2026-11-12T22:16:38+01:00 â€” Agent 20261112-winhooks-C (Python tools on Windows): Python tools call their own interpreter and use UTF-8, Windows-only tests skip with reasons, tests that run git isolate their environment. See [windows-python-tools](features/windows-python-tools.md).
- 2026-11-12T22:18:23+01:00 â€” Agent 20261112-winhooks-A (hooks on Windows): launcher `.claude/run_hook.cmd` for sh and cmd, `Bash|PowerShell` matcher, UTF-8 decoding, Windows library names, tests and docs; seven hook tests wait for the path fixes. See [windows-hooks](features/windows-hooks.md).
- 2026-11-12T23:12:05+01:00 â€” Agent 20261112-winhooks-E (first Windows build): S2 done, the project builds warning-free with MSYS2 UCRT64 and Ninja and all CTest tests pass or skip with a reason; platform files for the save write and CPU time; Windows setup in the user docs. See [windows-build](features/windows-build.md), [windows-platform](features/windows-platform.md), [windows-tests](features/windows-tests.md).
- 2026-11-13T00:26:30+01:00 â€” Lead (20261112-winhooks end): merged 20261112-winhooks-E (first MSYS2 build, platform files, Windows setup docs), -B (hook path handling, artifacts junction), -A (hook launcher for Git Bash and PowerShell, GIT_* deny), -D (LF safeguard, LF size checks) and -C (Python tools on Windows, git isolation and guard test) as one combined squash through tools/merge_agent.py with all tests passing on Windows and Linux green (WSL), per the user's decision; ID ranges released in the merge commit; pushed main to origin/main; the five worker worktrees and branches are kept until the user agrees to remove them. See [windows-build.md](features/windows-build.md), [windows-hooks.md](features/windows-hooks.md), [windows-hook-paths.md](features/windows-hook-paths.md), [windows-python-tools.md](features/windows-python-tools.md), [windows-line-endings.md](features/windows-line-endings.md), [windows-tests.md](features/windows-tests.md)
- 2026-11-13T01:02:31+01:00 â€” Agent 20261113-winopen-D (python tools on windows): merge_agent removes the artifacts junction before a worktree (git followed it and emptied the data), finds the MSYS2 tools and Ninja itself, chase_native picks the DLL on Windows. See [windows-python-tools](features/windows-python-tools.md).
- 2026-11-13T00:53:27+01:00 â€” Agent 20261113-winopen-E (docs checker and skill text): the user docs check accepts listed options of external programs (wsl, gcc, cmake, git and others), the Windows docs use their common forms again, the lead-round skill and worker brief carry the Windows forms; the flaky touch test did not fail in 70 WSL runs and is an open item. See [windows-python-tools](features/windows-python-tools.md).
