#!/usr/bin/env python3
"""Warn about files under tests/data/ over a size limit. Always exits 0.

    python3 tools/check_tests_data_size.py [--root DIR] [--limit-kb N]

A new or grown file over the limit needs a reason in the pull request: it must hold recorded values with
provenance, not a raw dump.
"""
import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_LIMIT_KB = 100


def large_files(root, limit_bytes):
    data = root / 'tests' / 'data'
    if not data.is_dir():
        return []
    return sorted((path.relative_to(root).as_posix(), path.stat().st_size) for path in data.rglob('*')
                  if path.is_file() and path.stat().st_size > limit_bytes)


def existing_directory(text):
    path = Path(text)
    if not path.is_dir():
        raise argparse.ArgumentTypeError('not a directory: %s' % text)
    return path


def positive_integer(text):
    if not text.isdigit() or int(text) < 1:
        raise argparse.ArgumentTypeError('not a positive integer: %s' % text)
    return int(text)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--root', type=existing_directory, default=ROOT, help='repository (default: this checkout)')
    parser.add_argument('--limit-kb', type=positive_integer, default=DEFAULT_LIMIT_KB, help='size limit in KB (default: %d)' % DEFAULT_LIMIT_KB)
    args = parser.parse_args(argv)
    found = large_files(args.root.resolve(), args.limit_kb * 1000)
    for path, size in found:
        print('WARNING: %s is %d KB, over %d KB. A new or grown file this size needs a reason in the pull request: recorded values with provenance, not a raw dump.'
              % (path, (size + 999) // 1000, args.limit_kb))
    if not found:
        print('no file under tests/data is over %d KB' % args.limit_kb)
    return 0


if __name__ == '__main__':
    sys.exit(main())
