#!/usr/bin/env python3
"""Fail when a CTest test that needs the game image lacks the label `needs-game-data` (CI excludes that label)."""
import argparse
import re
import sys
from pathlib import Path

LABEL = "needs-game-data"
ADD_TEST = re.compile(r"^add_test\((\S+) (.*)\)\s*$", re.DOTALL)
SET_PROPERTIES = re.compile(r"^set_tests_properties\((\S+) PROPERTIES\s+(.*)$", re.DOTALL)
STATEMENT_START = re.compile(r"\n(?=[A-Za-z_]+\()")
LABELS = re.compile(r'LABELS "([^"]*)"')
SKIP_CODE = re.compile(r'SKIP_RETURN_CODE "77"')
CACHE_ROMFS = re.compile(r"^CHASE_ROMFS_DIR:[A-Z]+=(.*)$", re.MULTILINE)
GAME_DATA_PATH = re.compile(r"artifacts/(?:romfs|game|fib|textures|audio|progress\.json)")


def unquote(name: str) -> str:
    return name.strip('"')


def read_tests(build: Path) -> tuple[dict[str, str], dict[str, str]]:
    commands: dict[str, str] = {}
    properties: dict[str, str] = {}
    files = sorted(build.rglob("CTestTestfile.cmake"))
    if not files:
        raise ValueError(f"no CTestTestfile.cmake under {build}")
    for path in files:
        for line in STATEMENT_START.split(path.read_text()):
            added = ADD_TEST.match(line)
            if added:
                commands[unquote(added.group(1))] = added.group(2)
            setting = SET_PROPERTIES.match(line)
            if setting:
                name = unquote(setting.group(1))
                properties[name] = properties.get(name, "") + " " + setting.group(2)
    return commands, properties


def romfs_directory(build: Path) -> str:
    cache = build / "CMakeCache.txt"
    if not cache.is_file():
        raise ValueError(f"no CMakeCache.txt in {build}")
    found = CACHE_ROMFS.search(cache.read_text())
    return found.group(1).strip() if found else ""


def fixtures(setting: str, kind: str) -> set[str]:
    found = re.search(kind + r' "([^"]*)"', setting)
    return set(found.group(1).split(";")) if found else set()


def needing_game_data(commands: dict[str, str], properties: dict[str, str], romfs: str) -> dict[str, str]:
    needing: dict[str, str] = {}
    for name, command in commands.items():
        setting = properties.get(name, "")
        if romfs and romfs in command:
            needing[name] = "its command line names the RomFS folder"
        elif GAME_DATA_PATH.search(command):
            needing[name] = "its command line names a game-data folder"
        elif SKIP_CODE.search(setting):
            needing[name] = "it skips with code 77 when game data is missing"
    pairs = [
        (provider, user)
        for provider in commands
        for user in commands
        if provider != user
        and (fixtures(properties.get(provider, ""), "FIXTURES_SETUP") | fixtures(properties.get(provider, ""), "FIXTURES_CLEANUP"))
        & fixtures(properties.get(user, ""), "FIXTURES_REQUIRED")
    ]
    changed = True
    while changed:
        changed = False
        for provider, user in pairs:
            if provider in needing and user not in needing:
                needing[user] = f"it requires a fixture set up by {provider}"
                changed = True
            elif user in needing and provider not in needing:
                needing[provider] = f"it sets up a fixture required by {user}"
                changed = True
    return needing


def unlabelled(commands: dict[str, str], properties: dict[str, str], romfs: str) -> list[tuple[str, str]]:
    missing = []
    for name, reason in sorted(needing_game_data(commands, properties, romfs).items()):
        setting = properties.get(name, "")
        labels = {label for found in LABELS.findall(setting) for label in found.split(";")}
        if LABEL not in labels:
            missing.append((name, reason))
    return missing


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path, help="CMake build directory")
    args = parser.parse_args()
    try:
        commands, properties = read_tests(args.build)
        missing = unlabelled(commands, properties, romfs_directory(args.build))
    except ValueError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    for name, reason in missing:
        print(f"{name}: needs the label {LABEL}: {reason}", file=sys.stderr)
    if missing:
        print(f"{len(missing)} test(s) without the label {LABEL}; add them to the list at the end of CMakeLists.txt", file=sys.stderr)
        return 1
    print(f"{len(commands)} tests checked")
    return 0


if __name__ == "__main__":
    sys.exit(main())
