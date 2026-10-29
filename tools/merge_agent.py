#!/usr/bin/env python3
"""Squash-merge an agent branch into main with the mechanical checks of CLAUDE.md.

  merge_agent.py sync                    fetch, compare main with its upstream, fast-forward when main has no own commits
  merge_agent.py prepare BRANCH          sync, squash the branch into the index of main, then run the checks
  merge_agent.py check                   run the checks on the staged merge (after resolving conflicts or review edits)
  merge_agent.py finish BRANCH -m FILE [--release WHO]... [--log PLAN LINE_FILE]
                                         release WHO's ID ranges and append the round-end log line inside the squash,
                                         run the checks, commit with the message in FILE, remove the worktree and branch
                                         (the lock of a Claude agent is lifted first; a worktree with any other lock stays,
                                         with a warning and the commands to remove it later)
  merge_agent.py close -m FILE [--release WHO]... [--log PLAN LINE_FILE]
                                         the same bookkeeping as its own commit, for a round that ends without a merge

sync and prepare compare main with its upstream, or with --upstream REMOTE/BRANCH; prepare --no-sync skips that for a
session the user runs without a remote. When the upstream moved ahead and main has own commits, they stop: the user
decides. LINE_FILE holds one line "Lead (<round> end): ..."; its timestamp comes from the clock.

Conflicts inside the README progress section and in docs/progress-trend.svg are regenerated, and a conflict in
docs/plans/priorities.md takes main's copy (the checks regenerate it); a README conflict outside that
section stops the merge like any other. Conflicts in a session log archive keep the lines of both sides (check_doc_links.py
--fix then drops duplicates). Append-style conflicts of docs files (session logs, feature index, id-reservations,
docs/re and docs/design entries) keep both sides (tools/merge_conflicts.py). Other conflicts stop the merge.

Checks: no game-derived or binary files staged, warning-free build, docs/plans/priorities.md regenerated (its item links are checked by the doc fixes),
doc fixes (check_doc_links.py --fix, restaged), doc links, README progress section and priorities regenerated,
ctest, Python tests. Pushing stays a separate, explicitly authorized step.
"""
from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
from datetime import datetime
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from merge_conflicts import resolve_append_only  # noqa: E402
from round_log import name_error  # noqa: E402

JOBS = max(1, (os.cpu_count() or 2) - 1)

FORBIDDEN = re.compile(
    r"(^|/)(artifacts|build(-[^/]*)?|graphify-out)/|\.(cci|3ds|cst|cia|bin|bmp|png|jpe?g|wav|bcstm|bcwav|fib|blvl|bfnmdl|dat|mem)$|(^|/)session\.json$",
    re.IGNORECASE,
)
PRIORITIES = "docs/plans/priorities.md"
GENERATED_FILES = ["README.md", "docs/progress-trend.svg", PRIORITIES]
PROGRESS_START = "<!-- progress:start -->"
PROGRESS_END = "<!-- progress:end -->"
ARCHIVE_SUFFIX = "-archive.md"
CONFLICT_MARKER = re.compile(r"^(<<<<<<< |=======$|>>>>>>> )")
RESERVATIONS = "docs/plans/features/id-reservations.md"
LOG_HEADING = "## Session Log"
PLAN_NAME = re.compile(r"[a-z][a-z0-9-]*")
AGENT_LOCK = "claude agent "
ROUND_IN_LINE = re.compile(r"Lead \((\S+) end\b")


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


def update_priorities(root: Path) -> None:
    run(["python3", "tools/priority_report.py", "--update", PRIORITIES], root, capture=True)
    run(["git", "add", PRIORITIES], root)


def run_checks(root: Path) -> None:
    check_staged_content(root)
    print("== build")
    build_warning_free(root)
    print("== priorities (before the doc fixes, which check its item links)")
    update_priorities(root)
    print("== doc fixes (session log rotation, closed items, anchors)")
    fix_docs(root)
    print("== doc links")
    run(["python3", "tools/check_doc_links.py", "--root", "."], root)
    print("== README progress section")
    run(["python3", "tools/progress_report.py", "--update-readme", "README.md"], root)
    run(["git", "add", "README.md", "docs/progress-trend.svg"], root)
    print("== priorities")
    update_priorities(root)
    print("== ctest")
    run(["ctest", "--test-dir", "build", "--output-on-failure", "-j", str(JOBS)], root)
    print("== python tests")
    run(["python3", "-m", "unittest", "discover", "-s", "tests", "-p", "test_*.py"], root)
    print("== staged")
    print(git(root, "diff", "--cached", "--stat").rstrip())
    print("checks passed")


def upstream_of(root: Path, upstream: str | None) -> tuple[str, str]:
    if upstream is None:
        result = run(["git", "rev-parse", "--abbrev-ref", "main@{upstream}"], root, check=False, capture=True)
        if result.returncode != 0:
            raise MergeError("main has no upstream; pass --upstream REMOTE/BRANCH")
        upstream = result.stdout.strip()
    remotes = git(root, "remote").split()
    remote, _, branch = upstream.partition("/")
    if not branch or remote not in remotes:
        raise MergeError(f"--upstream: expected REMOTE/BRANCH with REMOTE one of: {', '.join(remotes) or 'none'}; got {upstream!r}")
    return remote, branch


def sync(root: Path, upstream: str | None) -> None:
    require_main(root)
    remote, branch = upstream_of(root, upstream)
    run(["git", "fetch", "-q", remote], root, capture=True)
    target = f"{remote}/{branch}"
    if run(["git", "rev-parse", "--verify", "--quiet", f"refs/remotes/{target}"], root, check=False, capture=True).returncode:
        raise MergeError(f"--upstream: {target} does not exist after the fetch")
    ahead, behind = (int(n) for n in git(root, "rev-list", "--left-right", "--count", f"main...{target}").split())
    if behind == 0:
        print(f"main is up to date with {target} ({ahead} own commits not pushed)")
        return
    if ahead:
        raise MergeError(f"main and {target} have diverged ({ahead} own, {behind} new upstream commits): stop and ask the user")
    if git(root, "status", "--porcelain", "--untracked-files=no").strip():
        raise MergeError(f"{target} is {behind} commits ahead, but main has uncommitted changes: stop and ask the user")
    run(["git", "merge", "-q", "--ff-only", target], root, capture=True)
    print(f"fast-forwarded main by {behind} commits to {target}")


def prepare(root: Path, branch: str, upstream: str | None = None, no_sync: bool = False) -> None:
    require_main(root)
    require_branch(root, branch)
    if git(root, "status", "--porcelain", "--untracked-files=no").strip():
        raise MergeError("main has uncommitted changes")
    if not no_sync:
        sync(root, upstream)
    result = run(["git", "merge", "--squash", branch], root, check=False, capture=True)
    if result.returncode != 0:
        unmerged = git(root, "diff", "--name-only", "--diff-filter=U").split()
        if any(path in unmerged for path in GENERATED_FILES):
            unmerged = resolve_generated(root, unmerged)
        for path in [path for path in unmerged if path.endswith(ARCHIVE_SUFFIX)]:
            keep_both_sides(root / path)
            run(["git", "add", path], root)
            unmerged.remove(path)
        unmerged = resolve_append_only(root, unmerged)
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


def worktree_lock(root: Path, worktree: Path) -> str | None:
    path = None
    for line in git(root, "worktree", "list", "--porcelain").splitlines():
        if line.startswith("worktree "):
            path = Path(line[len("worktree "):])
        elif path == worktree and (line == "locked" or line.startswith("locked ")):
            return line[len("locked "):]
    return None


def remove_worktree(root: Path, worktree: Path) -> bool:
    reason = worktree_lock(root, worktree)
    if reason is not None and not reason.startswith(AGENT_LOCK):
        print(f"warning: worktree {worktree} is locked ({reason or 'no reason given'}); it is not removed", file=sys.stderr)
        return False
    link = worktree / "artifacts"
    if link.is_symlink():
        link.unlink()
    if reason is not None:
        run(["git", "worktree", "unlock", str(worktree)], root)
    run(["git", "worktree", "remove", "--force", str(worktree)], root)
    return True


def remove_worktree_and_branch(root: Path, branch: str) -> None:
    worktree = worktree_of(root, branch)
    if worktree is not None and not remove_worktree(root, worktree):
        print(f"leftover, once nobody uses it: git worktree remove --force {worktree} && git branch -D {branch}", file=sys.stderr)
        return
    run(["git", "branch", "-D", branch], root)


def read_message(message_file: Path) -> str:
    try:
        message = message_file.read_text().rstrip()
    except OSError as error:
        raise MergeError(f"--message: cannot read '{message_file}': {error.strerror}")
    if not message:
        raise MergeError(f"message file '{message_file}' is empty")
    if not re.search(r"^Co-Authored-By: .+ <.+>$", message, re.MULTILINE):
        raise MergeError(f"message file '{message_file}' has no Co-Authored-By line; the merging agent adds its own")
    return message


def head_text(root: Path, path: str) -> str:
    result = run(["git", "show", f"HEAD:{path}"], root, check=False, capture=True)
    return result.stdout if result.returncode == 0 else ""


def holds(text: str, holder: str) -> bool:
    for line in text.split("\n"):
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if len(cells) == 5 and cells[1].isdigit() and cells[2].isdigit() and cells[3] == holder:
            return True
    return False


def holders_to_release(root: Path, holders: list[str]) -> list[str]:
    path = root / RESERVATIONS
    current = path.read_text(encoding="utf-8") if path.is_file() else ""
    pending = []
    for holder in holders:
        if not holder.strip() or "|" in holder or holder != holder.strip():
            raise MergeError(f"--release: WHO must be a reservation holder without '|' or outer spaces, got {holder!r}")
        if holds(current, holder):
            pending.append(holder)
        elif not holds(head_text(root, RESERVATIONS), holder):
            raise MergeError(f"--release: no reservation held by {holder!r}")
    return pending


def read_log_line(root: Path, plan: str, line_file: Path) -> tuple[Path, str]:
    if not PLAN_NAME.fullmatch(plan):
        raise MergeError(f"--log: PLAN must be a plan name like linux-port, got {plan!r}")
    progress = root / "docs" / "plans" / f"{plan}.progress.md"
    if not progress.is_file() or LOG_HEADING not in progress.read_text(encoding="utf-8").split("\n"):
        raise MergeError(f"--log: plan {plan!r} has no progress file with a '{LOG_HEADING}' section")
    try:
        lines = [line for line in line_file.read_text(encoding="utf-8").split("\n") if line.strip()]
    except OSError as error:
        raise MergeError(f"--log: cannot read '{line_file}': {error.strerror}")
    if len(lines) != 1:
        raise MergeError(f"--log: '{line_file}' must hold exactly one line, got {len(lines)}")
    text = lines[0].strip()
    named = ROUND_IN_LINE.match(text)
    problem = name_error(named.group(1), datetime.now().date()) if named else "no round name"
    if problem:
        raise MergeError(f"--log: expected one line 'Lead (<round> end): ...' with a round name like 20261021-audio "
                         f"({problem}); got {text[:80]!r}")
    return progress, text


def append_log_line(root: Path, progress: Path, text: str) -> None:
    relative = progress.relative_to(root).as_posix()
    lines = progress.read_text(encoding="utf-8").split("\n")
    committed = set(head_text(root, relative).split("\n"))
    if any(line.endswith(" — " + text) and line not in committed for line in lines):
        print(f"the round-end line is already in {relative}")
        return
    entry = f"- {datetime.now().astimezone().isoformat(timespec='seconds')} — {text}"
    while lines and not lines[-1].strip():
        lines.pop()
    progress.write_text("\n".join(lines + [entry]) + "\n", encoding="utf-8")
    run(["git", "add", "--", relative], root)
    print(f"appended the round-end line to {relative}")


def bookkeeping(root: Path, holders: list[str], log: list[str] | None) -> None:
    logged = read_log_line(root, log[0], Path(log[1])) if log else None
    for holder in holders_to_release(root, holders):
        run(["python3", "tools/check_doc_links.py", "--root", ".", "--release", holder], root, capture=True)
        run(["git", "add", "--", RESERVATIONS], root)
        print(f"released the ID ranges of {holder}")
    if logged:
        append_log_line(root, *logged)


def finish(root: Path, branch: str, message_file: Path, holders: list[str] | None = None, log: list[str] | None = None) -> None:
    require_main(root)
    require_branch(root, branch)
    message = read_message(message_file)
    bookkeeping(root, holders or [], log)
    run_checks(root)
    subprocess.run(["git", "commit", "-q", "-F", "-"], cwd=root, input=message + "\n", text=True, check=True)
    remove_worktree_and_branch(root, branch)
    print(git(root, "log", "--oneline", "-1").rstrip())
    print("merged; push with `git push` when authorized")


def close(root: Path, message_file: Path, holders: list[str], log: list[str] | None) -> None:
    require_main(root)
    if not holders and not log:
        raise MergeError("close: nothing to do; give --release WHO and/or --log PLAN LINE_FILE")
    message = read_message(message_file)
    if git(root, "status", "--porcelain", "--untracked-files=no").strip():
        raise MergeError("main has uncommitted changes")
    bookkeeping(root, holders, log)
    fix_docs(root)
    run(["python3", "tools/check_doc_links.py", "--root", "."], root)
    run(["python3", "tools/progress_report.py", "--update-readme", "README.md"], root)
    run(["git", "add", "README.md", "docs/progress-trend.svg"], root)
    subprocess.run(["git", "commit", "-q", "-F", "-"], cwd=root, input=message + "\n", text=True, check=True)
    print(git(root, "log", "--oneline", "-1").rstrip())
    print("committed; push with `git push` when authorized")


def add_bookkeeping_options(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("-m", "--message", required=True, type=Path)
    parser.add_argument("--release", action="append", default=[], metavar="WHO", help="drop the ID ranges WHO holds, in this commit")
    parser.add_argument("--log", nargs=2, metavar=("PLAN", "LINE_FILE"), help="append the round-end line to the plan's session log")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("sync").add_argument("--upstream", metavar="REMOTE/BRANCH")
    prepare_parser = sub.add_parser("prepare")
    prepare_parser.add_argument("branch")
    prepare_choice = prepare_parser.add_mutually_exclusive_group()
    prepare_choice.add_argument("--upstream", metavar="REMOTE/BRANCH")
    prepare_choice.add_argument("--no-sync", action="store_true", help="do not compare with a remote (a session without one)")
    sub.add_parser("check")
    finish_parser = sub.add_parser("finish")
    finish_parser.add_argument("branch")
    add_bookkeeping_options(finish_parser)
    add_bookkeeping_options(sub.add_parser("close"))
    args = parser.parse_args(argv)
    sys.stdout.reconfigure(line_buffering=True)
    try:
        root = repo_root()
        if args.command == "sync":
            sync(root, args.upstream)
        elif args.command == "prepare":
            prepare(root, args.branch, args.upstream, args.no_sync)
        elif args.command == "check":
            run_checks(root)
        elif args.command == "close":
            close(root, args.message, args.release, args.log)
        else:
            finish(root, args.branch, args.message, args.release, args.log)
    except MergeError as error:
        print(f"merge_agent: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
