#!/usr/bin/env python3
"""Fail when a tracked file is game-derived, a build output or an unlisted binary file.

    python3 tools/check_no_game_files.py [--root DIR]

The rules are the ones of the merge tool (tools/forbidden_files.py), applied to `git ls-files`.
"""
import argparse
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(Path(__file__).resolve().parent))

import forbidden_files  # noqa: E402

HEAD_BYTES = 8192


def tracked_files(root):
    result = subprocess.run(['git', '-C', str(root), 'ls-files', '-z'], capture_output=True, check=False)
    if result.returncode != 0:
        raise OSError('git ls-files failed in %s' % root)
    return [name.decode('utf-8', 'surrogateescape') for name in result.stdout.split(b'\0') if name]


def head_reader(root):
    def read_head(path):
        target = root / path
        if target.is_symlink() or not target.is_file():
            return None
        with open(target, 'rb') as handle:
            return handle.read(HEAD_BYTES)
    return read_head


def existing_directory(text):
    path = Path(text)
    if not path.is_dir():
        raise argparse.ArgumentTypeError('not a directory: %s' % text)
    return path


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--root', type=existing_directory, default=ROOT, help='repository (default: this checkout)')
    args = parser.parse_args(argv)
    root = args.root.resolve()
    try:
        files = tracked_files(root)
    except OSError as error:
        print(error, file=sys.stderr)
        return 2
    bad = forbidden_files.check_paths(files, head_reader(root))
    for path, reason in bad:
        print('%s: %s' % (path, reason))
    if bad:
        print('%d tracked files are game-derived, build outputs or binary files. They must not be in the repository.' % len(bad))
        return 1
    print('no game-derived or binary files among %d tracked files' % len(files))
    return 0


if __name__ == '__main__':
    sys.exit(main())
