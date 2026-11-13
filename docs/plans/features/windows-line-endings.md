# Windows: line endings

## 1. Scope

A Windows clone made with the Git for Windows default (`core.autocrlf=true`) must read the same LF files as a Linux clone, so that size limits, tests and recorded data give the same results. Covers `.gitattributes`, the byte counts of `tools/check_doc_links.py` and `tools/check_tests_data_size.py`, and `tests/test_line_endings.py`.

## 2. Status

IN_PROGRESS. Verified on Windows (20261112-winhooks-D, 2026-11-12); the CI step that checks a clone with `core.autocrlf=true` is written (20261113-winopen-G, 2026-11-13) and has not run on GitHub yet (WINBUILD-7).

## 3. Verified behaviour

Measured facts (20261112-winhooks-D, 2026-11-12, Windows, Git for Windows with `core.autocrlf=true` in its system config):

- Before: `git clone -c core.autocrlf=true --no-checkout <repo> clone; git -C clone checkout <commit before this change>` gave 1952 text files `w/crlf` in `git ls-files --eol`; `python3 tools/check_doc_links.py` failed with `claude-md-size: 23679 bytes, limit 23552` and six `re-file-size` findings (41079 to 41223 bytes, limit 40960).
- After: the same clone commands at the commit with `.gitattributes` `* text=auto eol=lf` gave 1917 files `w/lf`, 35 `w/crlf` (the camera CSVs, `-text`, byte-exact as committed) and 1 `w/-text` (the font); `python3 tools/check_doc_links.py` reported `388 files checked, all links and anchors resolve`, exit 0; `CLAUDE.md` measured 23546 bytes.
- `git add --renormalize` on a copy of the index changed no binary file and no CSV; only `docs/plans/features/id-reservations.md` was stored with CRLF (written by `check_doc_links.py` through `write_text` in text mode on Windows) and was converted to LF.
- `tools/check_doc_links.py` now writes with `newline="\n"`, prints paths with `/`, decodes git output as UTF-8, and its re-file, feature-file and CLAUDE.md limits count LF-normalised bytes; `tools/check_tests_data_size.py` does the same. Neither tool starts a `python3` subprocess, so nothing there needed `sys.executable`.
- `tests/test_check_doc_links.py` (60 tests) and `tests/test_check_tests_data_size.py` pass on Windows; before this change 35 of 57 failed there (cp1252 default encoding of `write_text`, backslashes in reported paths). CRLF and LF copies of a file exactly at each limit give the same verdict.

## 4. Stand-ins

None.

## 5. Open items

- [ ] WINTEST-11 — The 35 camera CSVs under `tests/data/camera` are committed with CRLF and kept byte-exact with `*.csv -text`; converting them to LF needs the port owner's OK because they are recorded reference data; evidence: [windows.md](../../rules/windows.md)

## 6. Findings summary

Decisions: [windows-toolchain.md §2026-11-12T20:54:37](../../design/windows-toolchain.md#ts-20261112-205437). Rule: [windows.md](../../rules/windows.md) "Linux stays green".

## 7. Code and tests

`.gitattributes`, `tools/check_doc_links.py`, `tools/check_tests_data_size.py`, `tests/test_check_doc_links.py`, `tests/test_check_tests_data_size.py`, `tests/test_line_endings.py`, `.github/workflows/ci.yml`.
