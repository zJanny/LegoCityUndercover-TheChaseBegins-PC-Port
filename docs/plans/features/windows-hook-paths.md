# Windows: hook paths and the artifacts link

## 1. Scope

`tools/claude_hooks.py` on Windows: native backslash paths, Git Bash style `/c/...` paths in commands, and the `artifacts` link of a worktree. The path code lives in `tools/hook_paths.py`.

## 2. Status

IN_PROGRESS. Worktree detection, `cd /c/...` resolution and the junction fallback are implemented and tested on Windows. The open items below remain.

## 3. Verified behaviour

Measured on Windows 11 (round 20261112-winhooks, worker B, 2026-11-12):

- Without Developer Mode, `ln -s` in Git Bash makes a copy of the folder, not a link. With `MSYS=winsymlinks:nativestrict` it fails with "Operation not permitted" instead.
- Without Developer Mode (and without administrator rights), Python's `os.symlink` raises `OSError` with errno 22 (`WinError 1314`, privilege not held). `_winapi.CreateJunction` makes a working directory junction without any privilege; `os.readlink` of it returns a `\?\`-prefixed path.
- `New-Item -ItemType Junction` in PowerShell makes the same junction.
- Before the change, the hooks did not see a worktree path with backslashes (no agent push rule, no artifacts-deletion rule, no report check at SubagentStop), and a commit after `cd /c/Users/...` passed unchecked, because `Path.resolve()` turned the path into `C:\c\Users\...`. After it, `tests/test_hook_paths.py` pins the denials (5 end-to-end tests that fail with the old hooks).
- Git inside WSL cannot read a Windows worktree (its `.git` file holds a Windows path). Exporting `GIT_DIR` and `GIT_WORK_TREE` to work around it is wrong: the tests' own `git init`, `git config` and `git commit` calls then act on the checkout and write into the shared git config. The WSL check runs on a clone made inside WSL ([CONTRIBUTING.md](../../../CONTRIBUTING.md)).
- On Windows the artifacts rules (link, deletion, code writes) first turn every backslash drive path in the command (`C:\...`, `d:\...`) into slashes (`hook_paths.drive_paths_with_slashes`), because the shell-word parser takes a backslash as an escape and an `rm` of `C:\...rtifacts\...` went unseen; on Linux the command is left as it is (round 20261112-winhooks, integrator, 2026-11-13).
- On Windows the hook makes the junction before an unguarded `ln -s ... artifacts` runs and answers that the link is in place, so no 800 MB copy is made.

## 4. Stand-ins

None.

## 5. Open items

- [ ] WINTEST-6 — Paths of the MSYS root (`/tmp`, `/usr`, `/home`) in commands are not mapped, because only drive paths (`/c/...`, `/cygdrive/c/...`) have a fixed rule; mapping the rest needs `cygpath`; evidence: `tests/test_hook_paths.py`

## 6. Findings summary

Measured facts are in section 3. Decisions: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437).

## 7. Code and tests

`tools/hook_paths.py`, `tools/claude_hooks.py`, `tests/test_hook_paths.py`.
