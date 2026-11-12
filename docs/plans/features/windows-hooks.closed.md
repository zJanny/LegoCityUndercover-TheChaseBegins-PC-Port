# Closed items of windows-hooks

Done items of [windows-hooks.md](windows-hooks.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] WINTEST-5 — Keep stderr clean in Bash: the `settings.json` command uses `${CLAUDE_PROJECT_DIR}`; verified in `sh` (WSL) and in Git Bash and PowerShell 5.1 on Windows; evidence: [§3](windows-hooks.md#3-verified-behaviour)
- [x] WINTEST-4 — closed 2026-11-13: with all fixes of round 20261112-winhooks merged, `tests/test_claude_hooks.py` passes on Windows (WINTEST-7 gave the last two tests their Windows forms); needs: WINTEST-7; evidence: [windows-hook-paths.md §3](windows-hook-paths.md#3-verified-behaviour)
