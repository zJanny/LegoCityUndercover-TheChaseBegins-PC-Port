#!/usr/bin/env python3
"""Fail when a tracked text file names data that exists on one machine only (CLAUDE.md "No local-only data in the repo")."""
import argparse
import fnmatch
import re
import subprocess
import sys
from pathlib import Path

ALLOWED_ARTIFACT_DIRS = {"game", "romfs", "fib", "textures", "audio", "progress.json"}
ARTIFACT_PATH = re.compile(r"artifacts/([^\s`'\"),;:]*)")
PLACEHOLDER_START = "<$*{[("
OTHER_PATTERNS = [
    (re.compile(r"azahar-emu/(states|sdmc)"), "an emulator folder of one machine"),
    (re.compile(r"~/\.local/share/azahar-emu"), "an emulator folder of one machine"),
    (re.compile(r"/home/[A-Za-z_][\w.-]*/"), "a home folder of one machine"),
]
HOW_TO_FIX = (
    "write the value into the test or the docs/re entry with its provenance (round, agent, state name, date, command) "
    "instead of naming the file"
)

PLACEHOLDER = r"(?:<[^/>]*>|\{\{[^}/]*\}\}|r(?:N|\{\{N\}\})[^/]*)[\w.-]*"
TOOL_FOLDERS = r"oracle(?:/(?:out|home|states))?/?"
RULE_FOLDERS = (
    TOOL_FOLDERS + r"|oracle/states/(?:NAME\.(?:cst|json)|README\.md)|oracle/" + PLACEHOLDER + r"(?:/" + PLACEHOLDER + r")?/?"
    + r"|rounds/?|rounds/" + PLACEHOLDER + r"(?:/" + PLACEHOLDER + r")?/?|scratch/?"
)

ARTIFACT_EXEMPTIONS = [
    ("tools/check_local_data.py", r".*", "holds the patterns it searches for"),
    ("tests/test_check_local_data.py", r".*", "holds the rejection cases"),
    ("tools/azahar_oracle.py", TOOL_FOLDERS, "the oracle tool's own default output folders"),
    ("docs/re/oracle*.md", TOOL_FOLDERS + r"|oracle/<[^/>]*>/?|oracle/states/NAME\.cst", "describe the folders the oracle tool creates"),
    ("docs/commands.md", TOOL_FOLDERS, "default folders of the oracle tool"),
    ("docs/rules/*.md", RULE_FOLDERS, "say where generated material goes (local only)"),
    ("CLAUDE.md", RULE_FOLDERS, "the rule names the local folders"),
    (".claude/skills/lead-round/*", RULE_FOLDERS, "the skill names the local round and oracle folders"),
    (".claude/skills/lead-round/evals/evals.json", r"rounds/20261020-buildit/20261020-buildit-[BC]\.md", "made-up round in an eval prompt"),
    ("tools/round_log.py", r"rounds", "looks whether a local round folder already has the new name"),
    ("tests/test_round_log.py", r"rounds/20261019-docs", "a round folder the test makes in a temporary directory"),
    ("docs/plans/linux-port.progress.md", r"oracle/states/", "user decision on where states go"),
]
LINE_EXEMPTIONS = [
    ("tools/check_local_data.py", r".*", "holds the patterns it searches for"),
    ("tests/test_check_local_data.py", r".*", "holds the rejection cases"),
    ("tools/azahar_oracle.py", r"azahar-emu", "the emulator folders it creates inside its home"),
    ("tests/test_azahar_oracle*.py", r"azahar-emu", "tests of the oracle tool's folders"),
    ("docs/re/oracle*.md", r"azahar-emu", "describe the folders the emulator creates"),
    ("tests/save_test.cpp", r"/home/u/", "test strings for the XDG path logic"),
]


def tracked_files(root):
    out = subprocess.run(["git", "ls-files", "-z"], cwd=root, check=True, capture_output=True).stdout
    return [name for name in out.decode("utf-8").split("\0") if name]


def artifact_exempt(name, rest):
    return any(fnmatch.fnmatch(name, path) and re.fullmatch(pattern, rest) for path, pattern, _ in ARTIFACT_EXEMPTIONS)


def line_exempt(name, line):
    return any(fnmatch.fnmatch(name, path) and re.search(pattern, line) for path, pattern, _ in LINE_EXEMPTIONS)


def violations(name, text):
    for number, line in enumerate(text.split("\n"), 1):
        for match in ARTIFACT_PATH.finditer(line):
            rest = match.group(1).rstrip(".")
            first = rest.split("/")[0]
            if not first or first[0] in PLACEHOLDER_START or first in ALLOWED_ARTIFACT_DIRS:
                continue
            if artifact_exempt(name, rest):
                continue
            yield number, line, "a local-only artifacts path"
            break
        else:
            if line_exempt(name, line):
                continue
            for pattern, why in OTHER_PATTERNS:
                if pattern.search(line):
                    yield number, line, why
                    break


def check(root):
    errors = []
    for name in tracked_files(root):
        path = root / name
        if not path.is_file():
            continue
        try:
            text = path.read_text(encoding="utf-8")
        except (UnicodeDecodeError, OSError):
            continue
        for number, line, why in violations(name, text):
            errors.append("%s:%d: %s — %s; %s" % (name, number, line.strip()[:160], why, HOW_TO_FIX))
    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    errors = check(parser.parse_args().root)
    for error in errors:
        print(error)
    if errors:
        print("%d local-only reference(s)" % len(errors))
        return 1
    print("no local-only data referenced")
    return 0


if __name__ == "__main__":
    sys.exit(main())
