# Closed items of windows-hook-paths

Done items of [windows-hook-paths.md](windows-hook-paths.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] WINTEST-7 — closed 2026-11-13: `test_artifacts_link_is_made_once` asserts the Windows behaviour (junction made, unguarded `ln -s` denied) and keeps the Linux one; the artifacts rules map backslash drive paths, so `test_agents_never_delete_from_the_main_artifacts` passes unchanged; blocks: WINTEST-4; evidence: `tests/test_hook_paths.py`
