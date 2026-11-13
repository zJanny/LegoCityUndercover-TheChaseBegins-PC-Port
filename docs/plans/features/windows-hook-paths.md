# Windows: hook paths and the artifacts link

## 1. Scope

`tools/claude_hooks.py` on Windows: native backslash paths, Git Bash style `/c/...` paths in commands, and the `artifacts` link of a worktree. The path code lives in `tools/hook_paths.py`.

## 2. Status

DONE. Worktree detection, `cd /c/...` and MSYS root (`/tmp`) resolution and the junction fallback are implemented and tested on Windows.

## 3. Verified behaviour

Measured on Windows 11 (round 20261112-winhooks, worker B, 2026-11-12):

- Without Developer Mode, `ln -s` in Git Bash makes a copy of the folder, not a link. With `MSYS=winsymlinks:nativestrict` it fails with "Operation not permitted" instead.
- Without Developer Mode (and without administrator rights), Python's `os.symlink` raises `OSError` with errno 22 (`WinError 1314`, privilege not held). `_winapi.CreateJunction` makes a working directory junction without any privilege; `os.readlink` of it returns a `\?\`-prefixed path.
- `New-Item -ItemType Junction` in PowerShell makes the same junction.
- Before the change, the hooks did not see a worktree path with backslashes (no agent push rule, no artifacts-deletion rule, no report check at SubagentStop), and a commit after `cd /c/Users/...` passed unchecked, because `Path.resolve()` turned the path into `C:\c\Users\...`. After it, `tests/test_hook_paths.py` pins the denials (5 end-to-end tests that fail with the old hooks).
- Git inside WSL cannot read a Windows worktree (its `.git` file holds a Windows path). Exporting `GIT_DIR` and `GIT_WORK_TREE` to work around it is wrong: the tests' own `git init`, `git config` and `git commit` calls then act on the checkout and write into the shared git config. The WSL check runs on a clone made inside WSL ([CONTRIBUTING.md](../../../CONTRIBUTING.md)).
- On Windows the artifacts rules (link, deletion, code writes) first turn every backslash drive path in the command (`C:\...`, `d:\...`) into slashes (`hook_paths.drive_paths_with_slashes`), because the shell-word parser takes a backslash as an escape and an `rm` of `C:\...rtifacts\...` went unseen; on Linux the command is left as it is (round 20261112-winhooks, integrator, 2026-11-13).
- On Windows every other path that starts with a single `/` (the MSYS root: `/tmp`, `/usr`, `/home`, `/etc`, ...; not `/dev/...`, which stays as written so `2>/dev/null` costs no extra process) is mapped with `cygpath -w -- <path>` (`hook_paths.msys_root_path`), because it depends on where Git for Windows is installed. `cygpath` is found on the `PATH` or next to the `bash.exe` or `git.exe` of Git for Windows (`usr\bin`), started with an argument list and a 3 s timeout, and its answer is cached per path for the life of the hook. A missing, failing, timing-out or garbled `cygpath` (empty, several lines, not a drive or UNC path) leaves the path as written, so the old behaviour stands and no hook raises or denies because of it. On Linux nothing changes. `tests/test_hook_paths.py` pins found, failing, timing-out and garbled answers with an injected runner, the argument list, one real `cygpath` comparison (skips with a reason when `cygpath` is absent) and an end-to-end deny of `cd /tmp/<folder> && git commit` and `git -C /tmp/<folder> commit` without a trailer, which passed unchecked before (round 20261113-winopen, worker F, 2026-11-13; run on Windows 11 with Git for Windows and MSYS2).
- On Windows the hook makes the junction before an unguarded `ln -s ... artifacts` runs and answers that the link is in place, so no 800 MB copy is made.

## 4. Stand-ins

None.

## 5. Open items

None.

## 6. Findings summary

Measured facts are in section 3. Decisions: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437).

## 7. Code and tests

`tools/hook_paths.py`, `tools/claude_hooks.py`, `tests/test_hook_paths.py`.
