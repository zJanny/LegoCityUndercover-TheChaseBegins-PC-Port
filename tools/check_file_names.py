#!/usr/bin/env python3
"""Fail when a file under src/, tests/ or tools/, or a CTest name, executable or library, is named after a mission, round, agent,
date or role instead of by what the code does (CLAUDE.md "Code and files")."""
import argparse
import fnmatch
import re
import subprocess
import sys
from pathlib import Path

CHECKED_FOLDERS = ("src/", "tests/", "tools/")
SKIPPED_FOLDERS = (
    "tools/oracle/",
    "tests/data/",
    "tests/qol_separation_fixtures/",
    "tests/gl_only_in_render/",
    "tests/frame_keep_points/",
)

SCENE_WORDS = {"mission", "mission-number", "number pair"}
BANNED_TOKENS = [
    (re.compile(r"missions?\d*"), "mission", "mission"),
    (re.compile(r"m\d+"), "mission-number", "mission number"),
    (re.compile(r"rounds?"), "round", "round"),
    (re.compile(r"agents?"), "agent", "agent"),
    (re.compile(r"sessions?"), "session", "session"),
    (re.compile(r"fix|fixes|fixed"), "fix", "fix"),
    (re.compile(r"glue"), "glue", "glue"),
    (re.compile(r"patch|patches"), "patch", "patch"),
    (re.compile(r"temp|tmp"), "temp", "temp"),
    (re.compile(r"hacks?"), "hack", "hack"),
    (re.compile(r"wip"), "wip", "wip"),
    (re.compile(r"misc"), "misc", "misc"),
    (re.compile(r"stuff"), "stuff", "stuff"),
    (re.compile(r"workarounds?"), "workaround", "workaround"),
    (re.compile(r"standin"), "standin", "standin"),
    (re.compile(r"todo"), "todo", "todo"),
    (re.compile(r"quick"), "quick", "quick"),
]

ALLOWED = [
    ("src/frontend/round_menu.*", "round", "the roundabout menu of the front end, not a round of the workflow"),
    ("tools/round_log.py", "round", "the lead workflow tool that names and logs rounds"),
    ("tests/test_round_log.py", "round", "test of the lead workflow tool that names and logs rounds"),
    ("tools/merge_agent.py", "agent", "the lead workflow tool that merges the branch of an agent"),
    ("tests/test_merge_agent.py", "agent", "test of the lead workflow tool that merges the branch of an agent"),
    ("round[_-]menu*", "round", "the roundabout menu of the front end, not a round of the workflow"),
]

RULE = "name it by what the code or tool does (CLAUDE.md \"Code and files\")"
RULE_LONG = (
    "Name every file, CTest, executable and library by what the code or tool does, never by the mission, round, agent or date "
    "it was written for or by a role or status word. Banned words: " + ", ".join(sorted({label for _, _, label in BANNED_TOKENS})) +
    ", number pairs such as 1-3 (CTest names that start with game- may name the scene they run)."
)

CMAKE_NAME = re.compile(r"\b(add_test\s*\(\s*NAME|add_executable\s*\(|add_library\s*\()\s*([^\s()]+)")
CMAKE_VARIABLE = re.compile(r"\$\{[^}]*\}")


def words_of(name):
    parts = [part for part in re.split(r"[_\-.]", name.lower()) if part]
    found = []
    for index, part in enumerate(parts):
        for pattern, key, label in BANNED_TOKENS:
            if pattern.fullmatch(part):
                found.append((key, label))
        if part == "stand" and index + 1 < len(parts) and parts[index + 1] == "in":
            found.append(("standin", "stand-in"))
        if part.isdigit() and index + 1 < len(parts) and parts[index + 1].isdigit():
            found.append(("number pair", "number pair %s-%s" % (part, parts[index + 1])))
    return found


def is_allowed(subjects, key):
    return any(key == word and fnmatch.fnmatch(subject, pattern) for pattern, word, _ in ALLOWED for subject in subjects)


def banned_in(text, subjects, scene=False):
    result = []
    for key, label in words_of(text):
        if (scene and key in SCENE_WORDS) or is_allowed(subjects, key) or label in result:
            continue
        result.append(label)
    return result


def is_checked(path):
    return path.startswith(CHECKED_FOLDERS) and not path.startswith(SKIPPED_FOLDERS)


def file_violations(paths):
    errors = []
    for path in paths:
        if not is_checked(path):
            continue
        components = path.split("/")
        for position, component in enumerate(components):
            prefix = "/".join(components[: position + 1])
            kind = "file" if position == len(components) - 1 else "folder"
            for word in banned_in(component, [prefix, component]):
                errors.append("%s: %s name \"%s\" has the banned word \"%s\"; rule: %s" % (path, kind, component, word, RULE))
    return errors


def cmake_names(text):
    names = []
    for match in CMAKE_NAME.finditer(text):
        name = CMAKE_VARIABLE.sub("_", match.group(2).strip('"'))
        names.append(("test" if match.group(1).startswith("add_test") else "target", name))
    return names


def cmake_violations(text, source):
    errors = []
    for kind, name in cmake_names(text):
        scene = kind == "test" and name.startswith("game-")
        for word in banned_in(name, [name], scene):
            errors.append("%s: CTest %s \"%s\" has the banned word \"%s\"; rule: %s" % (source, "name" if kind == "test" else "target", name, word, RULE))
    return errors


def tracked_files(root):
    try:
        result = subprocess.run(["git", "-C", str(root), "ls-files", "-z"], capture_output=True, text=True, check=True)
    except (OSError, subprocess.CalledProcessError) as error:
        raise SystemExit("cannot list the tracked files of %s with git: %s" % (root, error))
    return [path for path in result.stdout.split("\0") if path]


def check(root, tracked=None):
    paths = tracked_files(root) if tracked is None else tracked
    errors = file_violations(paths)
    for path in paths:
        if path == "CMakeLists.txt" or path.endswith("/CMakeLists.txt") and is_checked(path):
            errors.extend(cmake_violations((Path(root) / path).read_text(encoding="utf-8"), path))
    return errors


def parse_arguments(argv):
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent, help="repository root (default: this checkout)")
    arguments = parser.parse_args(argv)
    if not arguments.root.is_dir():
        parser.error("--root: not a folder: %s" % arguments.root)
    return arguments


def main(argv=None):
    arguments = parse_arguments(sys.argv[1:] if argv is None else argv)
    errors = check(arguments.root)
    for error in errors:
        print(error)
    if errors:
        print("%d name(s) that say which task they were written for. %s" % (len(errors), RULE_LONG))
        return 1
    print("all file, test and target names say what the code does")
    return 0


if __name__ == "__main__":
    sys.exit(main())
