#!/usr/bin/env python3
"""Squash-merge an agent branch into main with the mechanical checks of CLAUDE.md.

  merge_agent.py prepare BRANCH          squash the branch into the index of main, then run the checks
  merge_agent.py check                   run the checks on the staged merge (after resolving conflicts or review edits)
  merge_agent.py finish BRANCH -m FILE   run the checks, commit with the message in FILE, remove the worktree and branch

Conflicts inside the README progress section and in docs/progress-trend.svg are regenerated, and a conflict in
docs/plans/priorities.md takes main's copy (the checks regenerate it); a README conflict outside that
section stops the merge like any other. Conflicts in a session log archive keep the lines of both sides (check_doc_links.py
--fix then drops duplicates). Other conflicts stop the merge.

Checks: no game-derived or binary files staged, doc fixes (check_doc_links.py --fix, restaged), doc links, README progress section and docs/plans/priorities.md regenerated,
warning-free build, ctest, Python tests. Pushing stays a separate, explicitly authorized step.
"""
from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
from pathlib import Path

JOBS = max(1, (os.cpu_count() or 2) - 1)

FORBIDDEN = re.compile(
    r"(^|/)(artifacts|build(-[^/]*)?|graphify-out)/|\.(cci|3ds|cst|cia|bin|bmp|png|jpe?g|wav|bcstm|bcwav|fib|blvl|bfnmdl|dat)$",
    re.IGNORECASE,
)
PRIORITIES = "docs/plans/priorities.md"
GENERATED_FILES = ["README.md", "docs/progress-trend.svg", PRIORITIES]
PROGRESS_START = "<!-- progress:start -->"
PROGRESS_END = "<!-- progress:end -->"
ARCHIVE_SUFFIX = "-archive.md"
CONFLICT_MARKER = re.compile(r"^(<<<<<<< |=======$|>>>>>>> )")


class MergeError(Exception):
    pass


def keep_both_sides(path: Path) -> None:
    lines = path.read_text(encoding="utf-8").split("\n")
    path.write_text("\n".join(line for line in lines if not CONFLICT_MARKER.match(line)), encoding="utf-8")


def resolve_progress_conflicts(text: str) -> str | None:
    lines = text.split("\n")
    out: list[str] = []
    inside = False
    i = 0
    while i < len(lines):
        line = lines[i]
        if line.startswith("<<<<<<< "):
            middle = next(n for n in range(i + 1, len(lines)) if lines[n] == "=======")
            end = next(n for n in range(middle + 1, len(lines)) if lines[n].startswith(">>>>>>> "))
            hunk = lines[i + 1:end]
            if not inside or any(PROGRESS_START in l or PROGRESS_END in l for l in hunk):
                return None
            out.extend(lines[i + 1:middle])
            i = end + 1
            continue
        if PROGRESS_START in line:
            inside = True
        elif PROGRESS_END in line:
            inside = False
        out.append(line)
        i += 1
    return "\n".join(out)


def resolve_generated(root: Path, unmerged: list[str]) -> list[str]:
    remaining = list(unmerged)
    readme_files = [path for path in ("README.md", "docs/progress-trend.svg") if path in remaining]
    if PRIORITIES in remaining:
        run(["git", "checkout", "--ours", "--", PRIORITIES], root)
        run(["git", "add", PRIORITIES], root)
        remaining.remove(PRIORITIES)
    if "README.md" in remaining:
        resolved = resolve_progress_conflicts((root / "README.md").read_text(encoding="utf-8"))
        if resolved is None:
            return remaining
        (root / "README.md").write_text(resolved, encoding="utf-8")
        remaining.remove("README.md")
    if "docs/progress-trend.svg" in remaining:
        run(["git", "checkout", "--ours", "--", "docs/progress-trend.svg"], root)
        remaining.remove("docs/progress-trend.svg")
    if readme_files:
        run(["python3", "tools/progress_report.py", "--update-readme", "README.md"], root)
        run(["git", "add", "README.md", "docs/progress-trend.svg"], root)
    return remaining


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
    run(["cmake", "-S", ".", "-B", "build", "-DCMAKE_BUILD_TYPE=RelWithDebInfo"], root, capture=True)
    result = run(["cmake", "--build", "build", "-j", str(JOBS)], root, check=False, capture=True)
    output = result.stdout + result.stderr
    if result.returncode != 0:
        raise MergeError("build failed:\n" + output[-4000:])
    warnings = [line for line in output.splitlines() if "warning:" in line]
    if warnings:
        raise MergeError("build has warnings:\n" + "\n".join(warnings[:40]))


def fix_docs(root: Path) -> None:
    run(["python3", "tools/check_doc_links.py", "--root", ".", "--fix"], root)
    run(["git", "add", "-A", "--", "docs"], root)


def run_checks(root: Path) -> None:
    check_staged_content(root)
    print("== doc fixes (session log rotation, closed items, anchors)")
    fix_docs(root)
    print("== doc links")
    run(["python3", "tools/check_doc_links.py", "--root", "."], root)
    print("== build")
    build_warning_free(root)
    print("== README progress section")
    run(["python3", "tools/progress_report.py", "--update-readme", "README.md"], root)
    run(["git", "add", "README.md", "docs/progress-trend.svg"], root)
    print("== priorities")
    run(["python3", "tools/priority_report.py", "--update", PRIORITIES], root, capture=True)
    run(["git", "add", PRIORITIES], root)
    print("== ctest")
    run(["ctest", "--test-dir", "build", "--output-on-failure", "-j", str(JOBS)], root)
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
        if any(path in unmerged for path in GENERATED_FILES):
            unmerged = resolve_generated(root, unmerged)
        for path in [path for path in unmerged if path.endswith(ARCHIVE_SUFFIX)]:
            keep_both_sides(root / path)
            run(["git", "add", path], root)
            unmerged.remove(path)
    if result.returncode != 0 and unmerged:
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
    if not re.search(r"^Co-Authored-By: .+ <.+>$", message, re.MULTILINE):
        raise MergeError(f"message file '{message_file}' has no Co-Authored-By line; the merging agent adds its own")
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
    sys.stdout.reconfigure(line_buffering=True)
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
