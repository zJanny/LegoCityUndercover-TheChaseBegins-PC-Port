#!/usr/bin/env python3
"""Path handling of tools/claude_hooks.py that differs between Linux and Windows.

On Windows the hooks see native paths (backslashes) from Claude Code and Git Bash style paths (/c/Users/..., /cygdrive/c/...)
inside the commands an agent writes. Every function takes `windows` so the tests can run both forms on any platform.
Paths of the MSYS root (/tmp, /usr, /home, /etc) depend on where Git for Windows is installed, so `cygpath -w` maps them;
when `cygpath` is missing, fails or times out, the path stays as written.
"""
import functools
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

WORKTREES = "/.claude/worktrees/"
IS_WINDOWS = os.name == "nt"
MSYS_DRIVE = re.compile(r"^/(?:cygdrive/)?([a-zA-Z])(?=/|$)")
MSYS_ROOT = re.compile(r"^/(?!/|dev/)")
WINDOWS_PATH = re.compile(r"^(?:[a-zA-Z]:[\\/]|\\\\)")
CYGPATH_TIMEOUT = 3
DRIVE_PATH = re.compile(r"(?<![\w\\/])[a-zA-Z]:\\[^\s'\";|&<>()`]*")


def posix(text, windows=IS_WINDOWS):
    text = str(text)
    return text.replace("\\", "/") if windows else text


def drive_paths_with_slashes(command, windows=IS_WINDOWS):
    if not windows:
        return command
    return DRIVE_PATH.sub(lambda match: match.group(0).replace("\\", "/"), command)


@functools.lru_cache(maxsize=None)
def find_cygpath():
    found = shutil.which("cygpath")
    if found:
        return found
    folders = []
    for tool in ("bash", "git"):
        located = shutil.which(tool)
        if located:
            folder = Path(located).parent
            folders += [folder, folder.parent / "usr" / "bin", folder.parent.parent / "usr" / "bin"]
    for folder in folders:
        for name in ("cygpath.exe", "cygpath"):
            if (folder / name).is_file():
                return str(folder / name)
    return None


@functools.lru_cache(maxsize=256)
def run_cygpath(text):
    program = find_cygpath()
    if not program:
        return None
    try:
        result = subprocess.run([program, "-w", "--", text], capture_output=True, text=True, encoding="utf-8",
                                errors="replace", timeout=CYGPATH_TIMEOUT)
    except (OSError, subprocess.SubprocessError, ValueError):
        return None
    return result.stdout if result.returncode == 0 else None


def msys_root_path(text, run=None):
    if not MSYS_ROOT.match(text) or "\n" in text:
        return text
    try:
        mapped = (run or run_cygpath)(text)
    except Exception:
        return text
    mapped = mapped.strip() if isinstance(mapped, str) else ""
    return mapped if WINDOWS_PATH.match(mapped) and "\n" not in mapped else text


def native_path(text, windows=IS_WINDOWS, cygpath=None):
    text = str(text)
    if not windows:
        return text
    if MSYS_DRIVE.match(text):
        mapped = MSYS_DRIVE.sub(lambda match: match.group(1).upper() + ":", text, count=1)
        return mapped + "/" if mapped.endswith(":") else mapped
    return msys_root_path(text, cygpath)


def in_worktree(path, windows=IS_WINDOWS):
    return WORKTREES in posix(path, windows)


def worktree_parts(path, windows=IS_WINDOWS):
    text = posix(path, windows)
    at = text.find(WORKTREES)
    if at < 0:
        return None
    return Path(text[:at]), text[at + len(WORKTREES):].split("/")[0]


def main_of_worktree(path, windows=IS_WINDOWS):
    parts = worktree_parts(path, windows)
    return parts[0] if parts else None


def join(base, text, windows=IS_WINDOWS, cygpath=None):
    return Path(base) / os.path.expanduser(native_path(text, windows, cygpath))


def resolve_in(base, text, windows=IS_WINDOWS, cygpath=None):
    return join(base, text, windows, cygpath).resolve()


def make_junction(link, target):
    import _winapi
    _winapi.CreateJunction(str(target), str(link))


def create_link(link, target, windows=IS_WINDOWS, symlink=os.symlink, junction=make_junction):
    try:
        symlink(target, link, target_is_directory=True)
        return "symlink"
    except OSError as error:
        failure = error
    if windows:
        try:
            junction(link, target)
            return "junction"
        except OSError as error:
            failure = error
    print(f"claude_hooks.py: the artifacts link {link} was not created ({failure}); create it by hand, on Windows with "
          f"mklink /J (CONTRIBUTING.md \"Working on Windows\")", file=sys.stderr)
    return None


def ensure_artifacts_link(path, windows=IS_WINDOWS, symlink=os.symlink, junction=make_junction):
    parts = worktree_parts(native_path(path, windows), windows)
    if not parts:
        return None
    main, name = parts
    tree = main / ".claude" / "worktrees" / name
    link = tree / "artifacts"
    if (tree / "CLAUDE.md").is_file() and (main / "artifacts").is_dir() and not os.path.lexists(link):
        return create_link(link, main / "artifacts", windows, symlink, junction)
    return None
