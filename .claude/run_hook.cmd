:;hook="${0%/*}/../tools/claude_hooks.py"; probe="import sys; sys.exit(sys.version_info < (3, 11))"
:;if python3 -c "$probe" </dev/null >/dev/null 2>&1; then exec python3 "$hook" "$@"; fi
:;if python -c "$probe" </dev/null >/dev/null 2>&1; then exec python "$hook" "$@"; fi
:;if py -3 -c "$probe" </dev/null >/dev/null 2>&1; then exec py -3 "$hook" "$@"; fi
:;echo "claude hooks: no Python 3.11 or newer found as python3, python or py -3; see docs/development.md" >&2; exit 1
@echo off
set "PY="
call python3 -c "import sys; sys.exit(sys.version_info < (3, 11))" <nul >nul 2>nul && set "PY=python3"
if not defined PY call python -c "import sys; sys.exit(sys.version_info < (3, 11))" <nul >nul 2>nul && set "PY=python"
if not defined PY call py -3 -c "import sys; sys.exit(sys.version_info < (3, 11))" <nul >nul 2>nul && set "PY=py -3"
if not defined PY (echo claude hooks: no Python 3.11 or newer found as python3, python or py -3; see docs/development.md 1>&2 & exit /b 1)
call %PY% "%~dp0..\tools\claude_hooks.py" %*
exit /b %errorlevel%
