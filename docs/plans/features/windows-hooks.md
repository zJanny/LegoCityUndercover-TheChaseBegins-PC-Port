# Windows: Claude Code hooks

## 1. Scope

The five Claude Code hooks of `tools/claude_hooks.py` (wired in `.claude/settings.json`) start, read their input and write their output correctly on Windows, in Git Bash and in PowerShell, and behave as before on Linux. Path handling is the windows-hook-paths feature, line endings are the windows-line-endings feature.

## 2. Status

IN_PROGRESS. Launcher, tool matcher, encoding and library names are done and verified on Windows; the real Claude Code hook runner was not reproduced.

## 3. Verified behaviour

Measured on Windows 11 with Python 3.11, Git for Windows and Windows PowerShell 5.1 (round 20261112-winhooks, worker A, 2026-11-12).

- On a fresh Windows `python3` is the Microsoft Store placeholder: it prints "Python was not found" and exits 49 (Git Bash) or 9009 (PowerShell, cmd). The old command `python3 "$CLAUDE_PROJECT_DIR/tools/claude_hooks.py" ...` therefore failed in all five hooks.
- `.claude/run_hook.cmd` is one file for `sh` and `cmd` (a label line is a no-op in `sh`, `:;` lines are labels in `cmd`). Git Bash runs a `.cmd` file through `cmd.exe`, so on Windows only the `cmd` half runs; the `sh` half runs on Linux.
- Inside a batch file a `.cmd` shim must be started with `call`, otherwise control never returns; the launcher uses `call` for every probe.
- The command in `.claude/settings.json` is `cd "${CLAUDE_PROJECT_DIR}"; ./.claude/run_hook.cmd <hook>`. Bash expands the variable. In PowerShell mode Claude Code itself rewrites `${CLAUDE_PROJECT_DIR}` to `${env:CLAUDE_PROJECT_DIR}` and appends `if (-not $?) { if ($LASTEXITCODE -eq 2) { exit 2 } }`; the tests apply the same rewrite. A first form that read both `$env:` and `$CLAUDE_PROJECT_DIR` printed `:CLAUDE_PROJECT_DIR: command not found` on stderr in sh and was dropped.
- Run with the exact command string of `settings.json` from a different current folder, with `CLAUDE_PROJECT_DIR` as a Windows path and, for Git Bash, as an `/c/...` path, for all five hooks (`pre-bash` and `pre-bash` with a PowerShell-shaped payload, `pre-agent`, `pre-tool`, `subagent-stop`, `post-edit`), in Git Bash and in Windows PowerShell 5.1: every hook ran, the denials came out as JSON on stdout with exit code 0, stderr stayed empty, and exit codes 1 and 2 of the hook came through. The interpreter probes of the launcher read no stdin.
- The same under six `PATH` situations in both shells (working `python3`; stub `python3` and a working `python`; stub `python3` and only `py -3`; no `python3` and a working `python`; neither and only `py -3`; nothing): the launcher took the first working interpreter and, with none, printed its message and exited with 1. `tests/test_claude_hooks.py` pins these cases (`LauncherTest`).
- Windows PowerShell 5.1 pipes a string to a native command with a UTF-8 byte order mark. The hook reads its input as `utf-8-sig`, so both forms work.
- The hook decodes git and tool output as UTF-8 with `errors="replace"` (the Windows default code page cp1252 mangled or crashed on UTF-8 text and made `check_doc_links.py` fail with no lines), starts its tools with `PYTHONUTF8=1` and writes stdout and stderr as UTF-8.
- `assets_env` finds `libchase_assets_c.so`, `chase_assets_c.dll` and `libchase_assets_c.dll` in `build/`.
- The `PowerShell` tool carries the same `tool_input.command`: `pre-bash` matches `Bash|PowerShell`, and a PowerShell-shaped payload gets the commit trailer, push and git config denials.
- `pre-bash` refuses any command that sets or exports `GIT_DIR`, `GIT_WORK_TREE`, `GIT_COMMON_DIR` or `GIT_INDEX_FILE` (Bash, `env`, `set`, `$env:`, `Set-Item`, `SetEnvironmentVariable`, inside `wsl.exe -e bash -c '...'` strings and heredocs) and names the WSL clone method. A command that only mentions a name in text is allowed. Cause: a WSL check run with such variables wrote `core.worktree` into the shared `.git/config`.
- `pre-bash` crashed on an empty-string argument (`[ "$X" = "" ]`, `echo ""`) with `IndexError` in `statements`; fixed.
- The launcher works from Windows PowerShell 5.1 when the checkout path contains a space: a copy of `run_hook.cmd` and the hook tools in a folder `space dir\repo` answered a `pre-bash` call for a push by an agent with the deny JSON and exit code 0, and an allowed command with empty output and exit code 0, both through `cd "${env:CLAUDE_PROJECT_DIR}"; ./.claude/run_hook.cmd pre-bash` and through the absolute path from another folder (round 20261113-winopen, worker F, 2026-11-13). Note for harnesses: the call must be piped into the launcher directly; piping into a `& { cd ...; ./run_hook.cmd }` script block drops stdin and the hook reports "the hook input is not JSON".
- Not verified on Windows: the launcher inside the real Claude Code process; the `sh` half under Git Bash (Git Bash never runs it); the Linux half is checked in WSL (see the progress file).

## 4. Stand-ins

None.

## 5. Open items

- [ ] WINTEST-3 — Needs a clean Windows machine the user provides. Question for the user: do the hooks deny in a real Claude Code session there? Steps: (1) on a Windows machine whose `python3` is the Store placeholder and that has Python 3.11 or newer as `python` or `py -3`, clone the repo, open Claude Code in it, with the hook shell Bash; (2) ask Claude to run `git commit --allow-empty -m "test"` (no `Co-Authored-By:` line): the hook must deny it with "no Co-Authored-By" and no `Python was not found` text; (3) repeat with the PowerShell hook shell enabled, so that Claude Code runs commands through the `PowerShell` tool; (4) ask for `git push origin main` from a worker agent: must be denied. A hook that does not run, prints a launcher error or allows the commit is the failure to report; evidence: [§3](#3-verified-behaviour)

## 6. Findings summary

The measured facts are in section 3, because `docs/design` holds only user decisions and `docs/re` only findings about the original. Toolchain decisions: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437).

## 7. Code and tests

`.claude/settings.json`, `.claude/run_hook.cmd`, `tools/claude_hooks.py`, `tests/test_claude_hooks.py`, [Claude Code hooks on Windows](../../development.md#claude-code-hooks-on-windows).
