# Windows: Python tools and their tests

## 1. Scope

The Python tools in `tools/` and the unittests in `tests/test_*.py` on Windows: the interpreter the tools call, text encoding and line endings, path separators, and the tests that cannot run on Windows.

## 2. Status

IN_PROGRESS. The tools and tests of this file run on Windows; what is left is in section 5. The hook tests (`tests/test_claude_hooks.py`) and the doc-link tests (`tests/test_check_doc_links.py`) belong to the feature files windows-hooks and windows-line-endings.

## 3. Verified behaviour

Measured with `python3 -m unittest discover -s tests -p 'test_*.py'` on Windows (20261112-winhooks-C, 2026-11-12, Python 3.11, no C++ toolchain, game data extracted, converted data absent).

- Before: 427 tests, 17 failures, 57 errors, 25 skipped. The 74 failing tests by file: `test_check_doc_links` 35, `test_claude_hooks` 15, `test_merge_agent` 11, `test_round_log` 5, `test_azahar_oracle_launch` 4, `test_progress_report` 2, `test_azahar_oracle_touch` 1, `test_oracle_motion` 1. Causes in my files: text written by tests in the Windows default encoding (cp1252) and read by tools as UTF-8; symbolic links without the privilege; Linux-only Azahar oracle code; UDP reset on a port with no server yet; read-only `.git` files left in the `check-pr` scratch folder; backslashes in printed paths.
- Fixed: every text read, write and `open` of the tools and tests of this file names `encoding='utf-8'` (and writes name `newline='\n'`), so output is the same on Linux and Windows; `tools/merge_agent.py` calls its own interpreter (`sys.executable`) instead of `python3`; `merge_agent.remove_tree` clears the read-only flag of files; `round_log.py` and `user_docs.py` print `/` in paths; UDP tests retry on `ConnectionResetError`.
- After: 431 tests (four new git-isolation tests), 14 failures, 36 errors, 35 skipped in about 90 s; all 50 remaining failures are in `tests/test_check_doc_links.py` (35) and `tests/test_claude_hooks.py` (15), which the other Windows feature files own. The 35 skips are 25 without the C++ asset library, 6 symbolic links and 4 Azahar oracle.
- Skipped on Windows with a reason (`tests/fixtures.py` `requires_symlinks`, `POSIX_ONLY` in `tests/test_azahar_oracle_launch.py`): six tests of `tests/test_merge_agent.py` that create a symbolic link (`test_agent_locked_worktree_is_unlocked_and_removed_with_its_branch`, `test_foreign_lock_keeps_worktree_and_branch_with_leftover_commands`, `test_unlocked_worktree_is_removed`, `test_clean_pull_request_is_built_and_tested_in_a_separate_checkout`, `test_failed_checks_are_reported_and_the_checkout_removed`, `test_interrupt_removes_the_checkout`) without Developer Mode; three `LaunchedTest` tests and `FreeDisplayTest` of `tests/test_azahar_oracle_launch.py` (Xvfb, POSIX signals and process groups).

- Git isolation: every Python test that runs git calls `isolate_git_environment()` (`tests/fixtures.py`), so inherited `GIT_DIR`, `GIT_WORK_TREE`, `GIT_CONFIG*`, `GIT_AUTHOR_*` and similar variables cannot send a test repository's commands into another repository; every `GIT_*` variable except `GIT_EXEC_PATH` and `GIT_TERMINAL_PROMPT` is removed. `tests/test_git_environment.py` pins it with a decoy repository and, by scanning `tests/test_*.py`, fails when a test runs git or a tool that runs git without the call (20261112-winhooks-C, 2026-11-12).

## 4. Stand-ins

None.

## 5. Open items

- [ ] WINTEST-8 — `merge_agent.py` `check-pr` and the worktree removal create and remove the `artifacts` link with `os.symlink`, which needs Developer Mode on Windows; use the directory junction helper `tools/hook_paths.py` of the hooks work for the link when symlinks are not allowed (answer relayed by the lead, 2026-11-12), with removal that never follows the junction into the data; the six tests above stay skipped until then; evidence: `tools/merge_agent.py` `checkout_pull_request`, `remove_worktree`
- [ ] WINTEST-10 — The Azahar oracle tools (`tools/azahar_oracle.py`, `tools/oracle_motion.py`) assume Linux (Xvfb, `os.killpg`, `signal.SIGKILL`, `os.kill(pid, 0)`, which terminates the process on Windows); they are not ported, the four launch tests skip; evidence: `tools/azahar_oracle.py` `kill_group`, `free_display`

## 6. Findings summary

Measured facts are kept in section 3, because `docs/design` holds only user decisions and `docs/re` only findings about the original. The Windows Python requirement and the skip list: [rules/windows.md, Python on Windows](../../rules/windows.md#python-on-windows).

## 7. Code and tests

`tools/*.py`, `tests/fixtures.py`, `tests/test_merge_agent.py`, `tests/test_azahar_oracle_launch.py`.
