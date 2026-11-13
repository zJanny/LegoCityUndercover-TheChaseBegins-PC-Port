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
- Skipped on Windows with a reason (`POSIX_ONLY` in `tests/test_azahar_oracle_launch.py`): three `LaunchedTest` tests and `FreeDisplayTest` of `tests/test_azahar_oracle_launch.py` (Xvfb, POSIX signals and process groups). The six tests of `tests/test_merge_agent.py` that need the `artifacts` link (`requires_directory_links` in `tests/fixtures.py`) skipped here before 2026-11-13 because symbolic links need Developer Mode; since the item WINTEST-8 they use a directory junction and run; they skip only where neither a symbolic link nor a junction can be created.
- Artifacts link (20261113-winopen-D, 2026-11-13, Windows 11, Git for Windows, python.org Python 3.11, decoy folders in a temporary folder, never the real `artifacts/`): `git worktree remove --force` on a worktree whose `artifacts` is a directory junction FOLLOWS the junction and deletes the contents of its target. `Path.is_symlink()` is False for a junction, so the old `remove_worktree` left it in place; this emptied the main `artifacts/` after the merge of round 20261112-winhooks. `os.unlink` and `os.rmdir` of a junction remove only the link, and `shutil.rmtree` does not follow one. Fixed: `merge_agent.remove_directory_link` (symlink or junction, found by `is_directory_link`: `is_symlink` or reparse tag `IO_REPARSE_TAG_MOUNT_POINT`) removes the link before the worktree removal and before the removal of the `check-pr` checkout; `check-pr` makes its link with `hook_paths.create_link` and stops with an error when no link can be made. Tests with a decoy data folder: `tests/test_merge_agent.py` `test_removing_the_worktree_never_follows_the_artifacts_link_into_the_data`, `test_removing_the_artifacts_link_keeps_the_data_behind_it`, `test_clean_pull_request_is_built_and_tested_in_a_separate_checkout`. Safe manual step before removing a worktree by hand: `cmd /c rmdir <worktree>\artifacts` (removes only a junction, refuses a folder with content).
- Git isolation: every Python test that runs git calls `isolate_git_environment()` (`tests/fixtures.py`), so inherited `GIT_DIR`, `GIT_WORK_TREE`, `GIT_CONFIG*`, `GIT_AUTHOR_*` and similar variables cannot send a test repository's commands into another repository; every `GIT_*` variable except `GIT_EXEC_PATH` and `GIT_TERMINAL_PROMPT` is removed. `tests/test_git_environment.py` pins it with a decoy repository and, by scanning `tests/test_*.py`, fails when a test runs git or a tool that runs git without the call (20261112-winhooks-C, 2026-11-12).
- Text from git (20261113-winopen-H, 2026-11-13, Windows 11, python.org Python 3.11): `merge_conflicts.py` decoded the `git merge-file` output with the Windows code page and wrote it back as UTF-8, turning every em dash of a merged progress file into three characters; every subprocess text mode in `tools/` now names `encoding='utf-8'` and commit messages are sent as UTF-8 bytes. `tests/test_merge_conflicts.py` runs the merge under a non-UTF-8 default encoding and scans `tools/*.py` for text mode without the encoding.

## 4. Stand-ins

None.

## 5. Open items

- [ ] WINTEST-17 — question for the user: `git worktree remove` deletes the game data behind a worktree's `artifacts` junction, with `--force` and also without it on a clean worktree (decoy check by the reviewer of 20261113-winopen-D, 2026-11-13), and so may `git clean -x`, `rm -rf` or a cleanup of agent worktrees outside `merge_agent.py`; the hooks re-create the junction in every agent worktree. Which guard do you want: a hook that refuses these commands while the junction exists, a rule in the lead-round skill and CONTRIBUTING.md, or both? evidence: `tools/merge_agent.py` `remove_directory_link`
- [ ] WINTEST-10 — The Azahar oracle tools (`tools/azahar_oracle.py`, `tools/oracle_motion.py`) assume Linux (Xvfb, `os.killpg`, `signal.SIGKILL`, `os.kill(pid, 0)`, which terminates the process on Windows); they are not ported, the four launch tests skip; evidence: `tools/azahar_oracle.py` `kill_group`, `free_display`
- [ ] WINTEST-19 — `tests/test_azahar_oracle_touch.py` (`until_released`, `until_touched`) failed once in WSL (seen by the lead, 2026-11-12, failure text not recorded) and did not fail in 70 WSL runs of the file (30 plain and 40 under 32 busy loops; 20261113-winopen-E, 2026-11-13, fresh clone). Question for the lead: what was the exact failure text? Both helpers read the first packet before checking the deadline, so a stale packet of the previous state is returned when the 5 s deadline passes first; a fix needs the text to pick the cause; evidence: `tests/test_azahar_oracle_touch.py` `until_released`

## 6. Findings summary

Measured facts are kept in section 3, because `docs/design` holds only user decisions and `docs/re` only findings about the original. The Windows Python requirement and the skip list: [rules/windows.md, Python on Windows](../../rules/windows.md#python-on-windows).

## 7. Code and tests

`tools/*.py`, `tests/fixtures.py`, `tests/test_merge_agent.py`, `tests/test_azahar_oracle_launch.py`.
