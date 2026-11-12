#!/usr/bin/env python3
"""Round names and round-end log lines of the lead workflow (.claude/skills/lead-round).

  round_log.py name NAME   check a new round name: YYYYMMDD-slug with one to three slug words, a real date that is
                           not in the future, and not used yet in a progress log, its archive, the ID reservations or a
                           local round folder; exit 1 when it is taken
  round_log.py audit       count the round-end lines of all progress logs since the last "docs audit: done" or
                           "docs audit: declined" and say whether the user docs audit is due (after ten round ends)

Rounds are named, not numbered, so that sessions on two machines do not take the same name. Old lines that say
"round N" are still counted as round ends.
"""
from __future__ import annotations

import argparse
import re
import sys
from datetime import date, datetime
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SLUG_WORD = r"[a-z][a-z0-9]{0,15}"
NAME = re.compile(r"(\d{4})(\d{2})(\d{2})-(" + SLUG_WORD + r"(?:-" + SLUG_WORD + r"){0,2})")
AGENT_SUFFIXES = ("servant", "audit")
ROUND_END = re.compile(r"^- (\S+) — Lead \((?:round \d+|\d{8}-" + SLUG_WORD + r"(?:-" + SLUG_WORD + r"){0,2}) end\b")
AUDIT_MARK = re.compile(r"docs audit: (?:done|declined)\b")
AUDIT_EVERY = 10
RESERVATIONS = Path("docs/plans/features/id-reservations.md")
ROUND_FOLDERS = Path("artifacts/rounds")


def name_error(name: str, today: date) -> str | None:
    match = NAME.fullmatch(name)
    if not match:
        return "expected YYYYMMDD-slug with one to three lowercase slug words (a-z, 0-9, starting with a letter)"
    try:
        start = date(int(match.group(1)), int(match.group(2)), int(match.group(3)))
    except ValueError:
        return "the date is not a calendar date"
    if start > today:
        return "the date is in the future"
    words = match.group(4).split("-")
    if any(word in AGENT_SUFFIXES for word in words):
        return "slug words must not be " + " or ".join(AGENT_SUFFIXES) + " (agent names end with them)"
    return None


def name_pattern(name: str) -> re.Pattern:
    suffix = r"(?:-(?:[A-Z]|" + "|".join(AGENT_SUFFIXES) + r"))?"
    return re.compile(r"(?<![\w-])" + re.escape(name) + suffix + r"(?![\w-])")


def log_files(root: Path) -> list[Path]:
    plans = root / "docs" / "plans"
    return sorted(plans.glob("*.progress.md")) + sorted(plans.glob("*.progress-archive.md")) if plans.is_dir() else []


def name_uses(root: Path, name: str) -> list[str]:
    pattern = name_pattern(name)
    uses = []
    for path in log_files(root) + [root / RESERVATIONS]:
        if not path.is_file():
            continue
        for number, line in enumerate(path.read_text(encoding="utf-8").split("\n"), 1):
            if line.startswith(("- ", "| ")) and pattern.search(line):
                uses.append(f"{path.relative_to(root).as_posix()}:{number}")
    if (root / ROUND_FOLDERS / name).exists():
        uses.append((ROUND_FOLDERS / name).as_posix() + " (local)")
    return uses


def round_end_lines(root: Path) -> list[tuple[str, str]]:
    seen = {}
    for path in log_files(root):
        for line in path.read_text(encoding="utf-8").split("\n"):
            match = ROUND_END.match(line)
            if match:
                seen[line] = match.group(1)
    return sorted(((stamp, line) for line, stamp in seen.items()), key=lambda item: datetime.fromisoformat(item[0]))


def audit_state(root: Path) -> tuple[int, str | None]:
    ends = round_end_lines(root)
    last = max((n for n, (_, line) in enumerate(ends) if AUDIT_MARK.search(line)), default=None)
    since = ends[last + 1:] if last is not None else ends
    return len(since), ends[last][0] if last is not None else None


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--root", type=Path, default=ROOT)
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("name").add_argument("name")
    sub.add_parser("audit")
    args = parser.parse_args(argv)
    root = args.root.resolve()
    if args.command == "name":
        problem = name_error(args.name, date.today())
        if problem:
            parser.error(f"name: {problem}, got {args.name!r}")
        uses = name_uses(root, args.name)
        if uses:
            print(f"round name {args.name} is taken: " + ", ".join(uses[:5]) + "; add a slug word to make it unique")
            return 1
        print(f"round name {args.name} is free")
        return 0
    count, since = audit_state(root)
    where = f"since the last docs audit ({since})" if since else "and no docs audit recorded"
    verdict = "due" if count >= AUDIT_EVERY else "not due"
    print(f"user docs audit {verdict}: {count} round ends {where}; due after {AUDIT_EVERY}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
