# Closed items of windows-line-endings

Done items of [windows-line-endings.md](windows-line-endings.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] WINTEST-12 — closed 2026-11-13 (20261113-winopen-G): the step `Line endings of the clone` of the job `windows-build-and-test` in `.github/workflows/ci.yml` sets `core.autocrlf=true` before the checkout, runs `tests/test_line_endings.py` and fails on any text file that `ls-files --eol` shows as `w/crlf` or `w/mixed` (the `-text` camera CSVs excepted); evidence: [windows-build](windows-build.md)
