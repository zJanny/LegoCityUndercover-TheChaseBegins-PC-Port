#!/usr/bin/env python3
"""Resolve append-style conflicts of docs-only files in a conflicted merge.

    python3 tools/merge_conflicts.py [--root DIR]

For each unmerged file of the allowed docs kinds (session logs, the feature index, id-reservations, dated entries of
docs/re) where every conflicting hunk is an insertion on both sides (the common ancestor is empty there), the lines of
main and of the branch are both kept, main's first. The result is staged. Every other unmerged file, and any hunk that
changes existing lines, stays conflicted and is listed for a human or a cheap agent (exit 1). docs/design is not
append-only (its decisions are edited in place), so any conflict there stays for a manual resolution.
"""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
import tempfile
from pathlib import Path

APPEND_FILES = [
    re.compile(r"^docs/plans/[^/]+\.progress\.md$"),
    re.compile(r"^docs/plans/features/README\.md$"),
    re.compile(r"^docs/plans/features/id-reservations\.md$"),
    re.compile(r"^docs/re/[^/]+\.md$"),
]


def is_append_file(path: str) -> bool:
    return any(pattern.match(path) for pattern in APPEND_FILES)


def resolve_text(text: str) -> str | None:
    out: list[str] = []
    lines = text.split("\n")
    i = 0
    while i < len(lines):
        line = lines[i]
        if not line.startswith("<<<<<<< "):
            out.append(line)
            i += 1
            continue
        try:
            base_at = next(n for n in range(i + 1, len(lines)) if lines[n].startswith("||||||| "))
            middle = next(n for n in range(base_at + 1, len(lines)) if lines[n] == "=======")
            end = next(n for n in range(middle + 1, len(lines)) if lines[n].startswith(">>>>>>> "))
        except StopIteration:
            return None
        if base_at + 1 != middle:
            return None
        out.extend(lines[i + 1:base_at])
        out.extend(lines[middle + 1:end])
        i = end + 1
    return "\n".join(out)


def git(root: Path, *args: str) -> subprocess.CompletedProcess:
    return subprocess.run(["git", *args], cwd=root, capture_output=True, text=True)


def stage_blob(root: Path, stage: int, path: str, target: Path) -> bool:
    result = subprocess.run(["git", "show", f":{stage}:{path}"], cwd=root, capture_output=True)
    if result.returncode != 0:
        return False
    target.write_bytes(result.stdout)
    return True


def merged_text(root: Path, path: str) -> str | None:
    with tempfile.TemporaryDirectory() as tmp:
        parts = [Path(tmp) / name for name in ("ours", "base", "theirs")]
        if not all(stage_blob(root, stage, path, part) for stage, part in zip((2, 1, 3), parts)):
            return None
        result = subprocess.run(["git", "merge-file", "-p", "--diff3", *map(str, parts)], cwd=root, capture_output=True, text=True)
        return result.stdout if result.returncode >= 0 else None


def resolve_append_only(root: Path, unmerged: list[str]) -> list[str]:
    remaining = list(unmerged)
    for path in unmerged:
        if not is_append_file(path):
            continue
        text = merged_text(root, path)
        resolved = resolve_text(text) if text is not None else None
        if resolved is None:
            continue
        (root / path).write_text(resolved, encoding="utf-8", newline='\n')
        git(root, "add", "--", path)
        remaining.remove(path)
    return remaining


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--root", type=Path, default=Path("."))
    args = parser.parse_args(argv)
    root = args.root.resolve()
    unmerged = git(root, "diff", "--name-only", "--diff-filter=U").stdout.split()
    remaining = resolve_append_only(root, unmerged)
    for path in unmerged:
        if path not in remaining:
            print(f"resolved: {path}")
    if remaining:
        print("needs a human or a cheap agent:\n" + "\n".join(remaining))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
