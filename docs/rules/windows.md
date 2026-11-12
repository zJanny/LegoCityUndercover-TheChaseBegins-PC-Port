# Rules of the `windows-port` plan

Every agent that works for the `windows-port` plan reads this file in full after [CLAUDE.md](../../CLAUDE.md). Plan index and gate: [plans/README.md](../plans/README.md).

**Gate: open** (user, 2026-11-12: "I open the windows gate. I am currently on linux, so just do scouting what we need for a windows port. I will then start implementing once i switched to windpws"). Plan: [windows-port.md](../plans/windows-port.md). Windows work starts when the user continues on the Windows partition; before that only scouting and documents, no code.

## Where the work happens

- Windows work runs on the user's Windows partition, in its own checkout with its own Claude Code session (lead and agents). The Linux checkout stays the place of the port.
- Builds and tests for Windows run on that machine. A change counts as verified for Windows only when it was built and tested there; say "not verified on Windows" otherwise.
- The Graphify graph is per checkout: on Windows it is built and queried in the Windows main checkout.

## Behaviour

- The Windows build behaves exactly like the Linux build. It has no game behaviour of its own; the port decides what the game does (`docs/rules/port.md`). A difference between the two builds is a Windows bug, unless the Linux build is wrong compared with the original, which is a port item.
- Design decisions (toolchain, platform interfaces, packaging) come from specs the user approved in `docs/design/`. What no spec decides is asked of the user, never chosen.
- The same tests give the same results on Windows. A test that cannot run on Windows is skipped with a reason in its CTest properties and listed in the feature file.

## Code

- Platform code lives in files of its own per platform behind a plain interface, as `chase_render` keeps OpenGL; no operating-system checks spread through game or runtime code.
- The architecture rules of CLAUDE.md apply unchanged (no SDL in the core libraries, strict input, C++ as the source of truth).

## Linux stays green

- A Windows branch that touches shared code (anything outside the platform files of Windows) also passes the Linux build and all Linux tests before it is merged.
- The Linux check runs in WSL on the Windows machine, called from the agent's shell through `wsl.exe`, in a fresh clone inside the WSL home, never in a Windows checkout or worktree, and never with `GIT_DIR`, `GIT_WORK_TREE` or any other `GIT_*` variable set. Reason: a WSL run in the Windows worktree made tests rewrite the shared git config (`core.worktree`, `user.name`, `user.email`), and git inside WSL cannot use a Windows worktree anyway. Commit first (the clone sees only committed work), then `wsl.exe -e bash -lc 'rm -rf ~/wslcheck/<name> && git clone -q --branch <branch> <repository path as /mnt/<drive>/...> ~/wslcheck/<name> && cd ~/wslcheck/<name> && <checks>'`. Never change git config. The checks are configure, build warning-free, `ctest --output-on-failure` and the Python unittests, as in the CLAUDE.md "Commands", in a build folder of the clone; the clone has no `artifacts/` unless it is linked, so real-data tests skip.
- The checkout keeps LF line endings, so both builds read the same files. The repo's `.gitattributes` forces LF in the working tree whatever the clone's `core.autocrlf`, and the size checks of `tools/check_doc_links.py` and `tools/check_tests_data_size.py` count LF-normalised bytes. `core.autocrlf=false` stays recommended (Git for Windows defaults to `true`); check a clone with `git ls-files --eol`, where every text file must show `w/lf`.
- **Relaxed for Python-only branches** (user, 2026-11-12, chose "Relax for Python-only"): a branch touching only `tools/`, `tests/*.py`, `.claude/` and docs runs the Python unittests plus the doc checks in WSL; a branch touching C++ or CMake (package E) runs the full build and `ctest` as above.
- Report the result as `Linux green (WSL)`, and say whether real-data tests were skipped because the extracted game data was not reachable from WSL.
- **WSL missing or wrong: warn and stop.** When `wsl.exe` is missing, has no Linux distribution, lacks a build dependency (cmake, compiler, SDL2, epoxy, Python 3.11+), cannot reach the checkout, or the check fails to start for any other reason than a failing test, stop and warn the user through the lead with the exact command and its output. Never work around it: no other machine, no skipped tests, no merge without the check, no installing or reconfiguring WSL without the user's OK.

## Briefs and review

- **Every Windows brief also names:** the spec entries in `docs/design/` it implements, and that the worker verifies on Windows.
- **Review checks beyond the tests:** game behaviour unchanged against Linux; platform code only in its own files; `Linux green (WSL)` reported for shared code; every choice backed by a spec entry.

## Decided (spec: [windows-toolchain.md §2026-11-12T20:54:37](../design/windows-toolchain.md#ts-20261112-205437))

- Toolchain: MSYS2 UCRT64 GCC, flags as on Linux. Dependencies (SDL2, epoxy, FFmpeg, pkg-config): MSYS2 package manager, nothing vendored.
- Commands: the CMake and ctest commands of CLAUDE.md "Commands" run in the MSYS2 UCRT64 shell with the Ninja generator (`-G Ninja`); recorded in `docs/commands.md` and `docs/building.md` by the first Windows session.
- Packaging: none for now. Saves: `%APPDATA%\<name>` in a Windows platform file.
- Tests that cannot run on Windows are skipped with a reason and listed in the feature file. Windows CI: a GitHub job once the first local build passes, none on Forgejo.
- ID prefixes: WINBUILD (toolchain, CMake), WINPLAT (platform files), WINTEST (tests and tools).

Windows CI: a job in the GitHub workflow after the first local build passes, none on Forgejo (design entry above). Package list and save folder name (`%APPDATA%\chase`): decided in the same entry.

## Python on Windows

- Requirement: Python 3.11 or newer. The repo's tools start their own interpreter (`sys.executable`), but the documented commands say `python3`, so the machine needs a working `python3`. On a fresh Windows the `python3` name is the Microsoft Store stub: install Python 3.11 or newer from python.org with "Add python.exe to PATH" ticked, turn off the alias for `python3.exe` under "Settings, Apps, Advanced app settings, App execution aliases", and put a copy of `python.exe` named `python3.exe` next to it in the Python folder. Check in Git Bash and in PowerShell with `python3 --version`: it prints 3.11 or newer, not a Store page.
- Text files are read and written as UTF-8 with LF line endings by naming `encoding='utf-8'` and `newline='\n'` in every `read_text`, `write_text` and text `open` of the tools and tests; the Windows default code page does not apply.
- Tests that need a Linux-only part or a privilege skip with a reason instead of failing: the six `tests/test_merge_agent.py` tests that create a symbolic link skip when the user may not create links (Developer Mode off), and the Azahar oracle launch tests skip on Windows (Xvfb, POSIX signals). The list with names and the open items: [windows-python-tools.md](../plans/features/windows-python-tools.md).
- Every Python test that runs git (directly or through a tool) calls `isolate_git_environment()` from `tests/fixtures.py` at import time. It removes every `GIT_*` variable except `GIT_EXEC_PATH` and `GIT_TERMINAL_PROMPT` (among them `GIT_DIR`, `GIT_WORK_TREE`, `GIT_COMMON_DIR`, `GIT_INDEX_FILE`, `GIT_CONFIG*`, `GIT_AUTHOR_*`, `GIT_COMMITTER_*`) from the environment, so a test repository in a temporary folder never writes into an inherited repository. `tests/test_git_environment.py` proves it with a decoy repository and fails when a test file runs git, or a tool that runs git, without the call. A new test that runs git adds the call.
