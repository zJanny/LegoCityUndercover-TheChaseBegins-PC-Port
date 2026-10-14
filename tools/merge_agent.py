#!/usr/bin/env python3
"""Squash-merge an agent branch into main with the mechanical checks of CLAUDE.md.

  merge_agent.py prepare BRANCH          squash the branch into the index of main, then run the checks
  merge_agent.py check                   run the checks on the staged merge (after resolving conflicts or review edits)
  merge_agent.py finish BRANCH -m FILE   run the checks, commit with the message in FILE, remove the worktree and branch

Checks: no game-derived or binary files staged, doc links, README progress section regenerated,
warning-free build, ctest, Python tests. Pushing stays a separate, explicitly authorized step.
"""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

TRAILER = (
    "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>\n"
    "Claude-Session: https://claude.ai/code/session_01NCqAGJNfJ4YNtAckpDHk7s"
)
FORBIDDEN = re.compile(
    r"(^|/)(artifacts|build[^/]*|graphify-out)(/|$)|\.(cci|3ds|cst|cia|bin|bmp|png|jpe?g|wav|bcstm|bcwav|fib|blvl|bfnmdl|dat)$",
    re.IGNORECASE,
)


class MergeError(Exception):
    pass


def run(cmd: list[str], cwd: Path, check: bool = True, capture: bool = False) -> subprocess.CompletedProcess:
    result = subprocess.run(cmd, cwd=cwd, text=True, capture_output=capture)
    if check and result.returncode != 0:
        detail = (result.stdout or "") + (result.stderr or "") if capture else ""
        raise MergeError(f"command failed ({result.returncode}): {' '.join(cmd)}\n{detail}".rstrip())
    return result


def git(root: Path, *args: str) -> str:
    return run(["git", *args], root, capture=True).stdout


def repo_root() -> Path:
    result = subprocess.run(["git", "rev-parse", "--show-toplevel"], text=True, capture_output=True)
    if result.returncode != 0:
        raise MergeError("not inside a git repository")
    return Path(result.stdout.strip())


def require_main(root: Path) -> None:
    branch = git(root, "rev-parse", "--abbrev-ref", "HEAD").strip()
    if branch != "main":
        raise MergeError(f"the main checkout must be on main, not '{branch}'")


def require_branch(root: Path, branch: str) -> None:
    if run(["git", "rev-parse", "--verify", "--quiet", f"refs/heads/{branch}"], root, check=False, capture=True).returncode:
        raise MergeError(f"branch '{branch}' does not exist")


def staged_files(root: Path) -> list[str]:
    return [line for line in git(root, "diff", "--cached", "--name-only").splitlines() if line]


def check_staged_content(root: Path) -> None:
    unmerged = git(root, "diff", "--name-only", "--diff-filter=U").split()
    if unmerged:
        raise MergeError("unresolved conflicts: " + ", ".join(unmerged))
    unstaged = git(root, "diff", "--name-only").split()
    if unstaged:
        raise MergeError("unstaged changes to tracked files (stage or drop them): " + ", ".join(unstaged))
    files = staged_files(root)
    if not files:
        raise MergeError("nothing is staged")
    bad = [name for name in files if FORBIDDEN.search(name)]
    for line in git(root, "diff", "--cached", "--numstat").splitlines():
        added, removed, name = line.split("\t", 2)
        if added == "-" and removed == "-" and name not in bad:
            bad.append(name)
    if bad:
        raise MergeError("game-derived, build or binary files are staged: " + ", ".join(bad))


def build_warning_free(root: Path) -> None:
    run(["cmake", "-S", ".", "-B", "build", "-DCMAKE_BUILD_TYPE=Debug"], root, capture=True)
    result = run(["cmake", "--build", "build", "-j"], root, check=False, capture=True)
    output = result.stdout + result.stderr
    if result.returncode != 0:
        raise MergeError("build failed:\n" + output[-4000:])
    warnings = [line for line in output.splitlines() if "warning:" in line]
    if warnings:
        raise MergeError("build has warnings:\n" + "\n".join(warnings[:40]))


def run_checks(root: Path) -> None:
    check_staged_content(root)
    print("== doc links")
    run(["python3", "tools/check_doc_links.py", "--root", "."], root)
    print("== README progress section")
    run(["python3", "tools/progress_report.py", "--update-readme", "README.md"], root)
    run(["git", "add", "README.md"], root)
    print("== build")
    build_warning_free(root)
    print("== ctest")
    run(["ctest", "--test-dir", "build", "--output-on-failure", "-j", "4"], root)
    print("== python tests")
    run(["python3", "-m", "unittest", "discover", "-s", "tests", "-p", "test_*.py"], root)
    print("== staged")
    print(git(root, "diff", "--cached", "--stat").rstrip())
    print("checks passed")


def prepare(root: Path, branch: str) -> None:
    require_main(root)
    require_branch(root, branch)
    if git(root, "status", "--porcelain", "--untracked-files=no").strip():
        raise MergeError("main has uncommitted changes")
    result = run(["git", "merge", "--squash", branch], root, check=False, capture=True)
    if result.returncode != 0:
        unmerged = git(root, "diff", "--name-only", "--diff-filter=U").split()
        raise MergeError(
            "conflicts in: " + ", ".join(unmerged) + "\nresolve them (regenerate the README section), stage, then run: "
            "merge_agent.py check"
        )
    run_checks(root)


def worktree_of(root: Path, branch: str) -> Path | None:
    path = None
    for line in git(root, "worktree", "list", "--porcelain").splitlines():
        if line.startswith("worktree "):
            path = Path(line[len("worktree "):])
        elif line == f"branch refs/heads/{branch}":
            return path
    return None


def finish(root: Path, branch: str, message_file: Path) -> None:
    require_main(root)
    require_branch(root, branch)
    message = message_file.read_text().rstrip()
    if not message:
        raise MergeError(f"message file '{message_file}' is empty")
    if "Claude-Session:" not in message:
        message += "\n\n" + TRAILER
    run_checks(root)
    subprocess.run(["git", "commit", "-q", "-F", "-"], cwd=root, input=message + "\n", text=True, check=True)
    worktree = worktree_of(root, branch)
    if worktree is not None:
        link = worktree / "artifacts"
        if link.is_symlink():
            link.unlink()
        run(["git", "worktree", "remove", "--force", str(worktree)], root)
    run(["git", "branch", "-D", branch], root)
    print(git(root, "log", "--oneline", "-1").rstrip())
    print("merged; push with `git push` when authorized")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("prepare").add_argument("branch")
    sub.add_parser("check")
    finish_parser = sub.add_parser("finish")
    finish_parser.add_argument("branch")
    finish_parser.add_argument("-m", "--message", required=True, type=Path)
    args = parser.parse_args(argv)
    try:
        root = repo_root()
        if args.command == "prepare":
            prepare(root, args.branch)
        elif args.command == "check":
            run_checks(root)
        else:
            finish(root, args.branch, args.message)
    except MergeError as error:
        print(f"merge_agent: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
