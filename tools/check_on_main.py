#!/usr/bin/env python3
"""Run a command only on main; exit 77 (skip) on any other branch.

    python3 tools/check_on_main.py COMMAND [ARG...]

The generated files (README progress section, docs/plans/priorities.md, docs/progress-trend.svg) are
produced at the merge, so their checks must not fail in a branch. Main, master and a detached HEAD run the command.
"""
import subprocess
import sys

SKIP_EXIT = 77
MAIN_BRANCHES = ("main", "master")


def current_branch() -> str:
    result = subprocess.run(["git", "symbolic-ref", "--short", "-q", "HEAD"], capture_output=True, text=True)
    return result.stdout.strip() if result.returncode == 0 else ""


def main(argv: list[str]) -> int:
    if not argv:
        print("usage: check_on_main.py COMMAND [ARG...]", file=sys.stderr)
        return 2
    branch = current_branch()
    if branch and branch not in MAIN_BRANCHES:
        print(f"skipped on branch {branch}: generated files are produced at the merge")
        return SKIP_EXIT
    return subprocess.run(argv).returncode


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
