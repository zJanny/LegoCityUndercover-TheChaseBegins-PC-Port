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
OUTPUT_FOLDER = re.compile(
    r"\bagent[A-Za-z0-9_]*-out\b|\b(?:r[0-9]+|[0-9]{8}-[a-z]+)-servant-out\b|\b(?!graphify-out|servant-out)[A-Za-z0-9_]{3,}-out/(?:[\w.-]+/)*[\w-]+\.(?:png|jpe?g|mem|json|txt|log|bin|cst|csv)\b|\b[\w.-]+\.mem\b"
)
DISTRO_LOCATION = re.compile(r"/usr/bin/azahar|/usr/src/debug/|Arch package|\bpacman\b")
MACHINE_PHRASE = re.compile(r"this machine|my machine|the user's own (?:Azahar|config|save)", re.IGNORECASE)
USER_NAME = re.compile(r"\bjan\b|(?i:zjanny)")
STATE_LOAD = re.compile(r"(?:--state|\bloadstate)\s+`?([a-z0-9]+(?:-[a-z0-9_]+)+)")
STATE_SCOPES = ("docs/*", "tests/*", "src/*")
SAVESTATE_LINE = re.compile(r"^\s*savestate\s+([\w-]+)", re.MULTILINE)
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


RULE_SELF = [
    ("tools/check_local_data.py", r".*", "holds the patterns it searches for"),
    ("tests/test_check_local_data.py", r".*", "holds the rejection cases"),
]
ERROR_RULES = [
    (
        ("tests/*", "docs/*"), OUTPUT_FOLDER,
        "an output folder or trace file of one agent's scratch (write round or agent, state name, date and command instead)",
        RULE_SELF + [
            ("tests/test_azahar_oracle*.py", r"\.mem\b", "tests of the oracle tool, which writes NAME.mem"),
            ("tests/test_merge_agent.py", r"trace\.mem", "a file name the merge script must refuse"),
            ("docs/re/oracle*.md", r"\bNAME\.mem\b", "describes the trace file the oracle tool writes"),
        ],
    ),
    (
        ("*",), DISTRO_LOCATION,
        "a distro or package location (the install sections of README.md, getting-started.md and building.md may name packages)",
        RULE_SELF + [
            ("README.md", r".*", "install section"),
            ("docs/getting-started.md", r".*", "install section"),
            ("docs/building.md", r".*", "install section"),
        ],
    ),
]
WARNING_RULES = [
    (("docs/re/*",), MACHINE_PHRASE, "wording about one machine or the user's own setup", RULE_SELF),
    (
        ("*",), USER_NAME, "a user name",
        RULE_SELF + [
            ("LICENSE", r".*", "licence text"),
            ("REUSE.toml", r"SPDX-FileCopyrightText", "copyright holder"),
            ("README.md", r"github\.com/zJanny/", "clone URL of the project"),
        ],
    ),
]


def tracked_files(root):
    out = subprocess.run(["git", "ls-files", "-z"], cwd=root, check=True, capture_output=True).stdout
    return [name for name in out.decode("utf-8").split("\0") if name]


def artifact_exempt(name, rest):
    return any(fnmatch.fnmatch(name, path) and re.fullmatch(pattern, rest) for path, pattern, _ in ARTIFACT_EXEMPTIONS)


def line_exempt(name, line):
    return any(fnmatch.fnmatch(name, path) and re.search(pattern, line) for path, pattern, _ in LINE_EXEMPTIONS)


def rule_hits(rules, name, number, line):
    for scopes, pattern, why, exemptions in rules:
        if not any(fnmatch.fnmatch(name, scope) for scope in scopes) or not pattern.search(line):
            continue
        if any(fnmatch.fnmatch(name, path) and re.search(exempt, line) for path, exempt, _ in exemptions):
            continue
        return why
    return None


def warnings(name, text):
    for number, line in enumerate(text.split("\n"), 1):
        why = rule_hits(WARNING_RULES, name, number, line)
        if why:
            yield number, line, why


def state_load_hits(name, line, created):
    if not any(fnmatch.fnmatch(name, scope) for scope in STATE_SCOPES):
        return None
    if name in ("tools/check_local_data.py", "tests/test_check_local_data.py") or name.endswith(".py"):
        return None
    for match in STATE_LOAD.finditer(line):
        if match.group(1) not in created:
            return "a save state that no committed script saves (say \"recorded from a save state\" and describe the game situation)"
    return None


def violations(name, text, created=frozenset()):
    for number, line in enumerate(text.split("\n"), 1):
        why = state_load_hits(name, line, created)
        if why:
            yield number, line, why
            continue
        why = rule_hits(ERROR_RULES, name, number, line)
        if why:
            yield number, line, why
            continue
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


def read_tracked(root):
    for name in tracked_files(root):
        path = root / name
        if not path.is_file():
            continue
        try:
            yield name, path.read_text(encoding="utf-8")
        except (UnicodeDecodeError, OSError):
            continue


def check(root):
    errors = []
    files = dict(read_tracked(root))
    created = {state for path, text in files.items() if fnmatch.fnmatch(path, "tools/oracle/*.txt")
               for state in SAVESTATE_LINE.findall(text)}
    for name, text in files.items():
        for number, line, why in violations(name, text, created):
            errors.append("%s:%d: %s — %s; %s" % (name, number, line.strip()[:160], why, HOW_TO_FIX))
    return errors


def check_warnings(root):
    files = dict(read_tracked(root))
    result = []
    for name, text in files.items():
        for number, line, why in warnings(name, text):
            result.append("%s:%d: %s — %s" % (name, number, line.strip()[:160], why))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    root = parser.parse_args().root
    errors = check(root)
    for error in errors:
        print(error)
    found = check_warnings(root)
    for warning in found:
        print("warning: " + warning)
    if found:
        print("%d warning(s) (not errors)" % len(found))
    if errors:
        print("%d local-only reference(s)" % len(errors))
        return 1
    print("no local-only data referenced")
    return 0


if __name__ == "__main__":
    sys.exit(main())
