#!/usr/bin/env python3
"""Read TT Fusion localisation tables (.loc, magic "LOCA") as JSON.

The tables are decoded by the C++ asset runtime (chase::assets::parseLocalisation) through
tools/chase_native.py; build the project first. Keys are the sdbm hash of the upper-case
identifier, so entries without a known name are listed by their hash. The format is
described in docs/re/text-localisation-fonts.md.
"""
import argparse
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import chase_native as native  # noqa: E402


def candidate_names(paths):
    names = set()
    for path in paths:
        for match in re.finditer(rb'[A-Za-z0-9_]{2,64}', path.read_bytes()):
            names.add(match.group().decode().upper())
    return {native.text_key(name): name for name in names}


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('inputs', type=Path, nargs='+', help='.loc files or directories')
    parser.add_argument('--census', action='store_true')
    parser.add_argument('--extended', type=Path, help='extended_chars_utf8.txt for decoding escapes')
    parser.add_argument('--names', type=Path, action='append', default=[],
                        help='files to take candidate key names from (local use; output is game-derived)')
    args = parser.parse_args()
    try:
        native.library()
    except native.NativeUnavailable as exc:
        parser.exit(1, str(exc) + '\n')
    files = []
    for item in args.inputs:
        files += sorted(item.rglob('*.loc')) if item.is_dir() else [item]
    names = candidate_names(args.names)
    extended = args.extended.read_text(encoding='utf-8').strip() if args.extended else ''

    failures, totals, tables = [], {'files': 0, 'entries': 0, 'named': 0}, {}
    for path in files:
        try:
            entries = native.decode_localisation(path.read_bytes(), extended)
        except ValueError as exc:
            failures.append({'path': str(path), 'error': str(exc)})
            continue
        totals['files'] += 1
        totals['entries'] += len(entries)
        totals['named'] += sum(1 for key in entries if key in names)
        tables[path.name] = {names.get(key, '%08x' % key): text for key, text in entries.items()}
    if args.census:
        print(json.dumps({'failure_count': len(failures), 'failures': failures[:10], 'totals': totals}, indent=1))
    else:
        json.dump(tables, sys.stdout, indent=1, ensure_ascii=False)
        print()
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
