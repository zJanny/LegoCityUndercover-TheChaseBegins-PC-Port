#!/usr/bin/env python3
"""Read TT Fusion localisation tables (.loc, magic "LOCA").

Layout (from shipped data): "LOCA", u32 version (2), u32 entry count, u32 table bytes
(count x 8), u32 string bytes; then (u32 key hash, u32 string offset) pairs sorted by
hash; then NUL-terminated strings. Keys are the sdbm hash (h = h * 65599 + c) of the
upper-case identifier, e.g. "PAUSEMENU" -> "Pause Menu". Text is Latin-1, except the
escape 0x01 '2' c, which stands for character c - 0x30 of
localisation/extended_chars_utf8.txt (Cyrillic and Polish letters; found by letter
frequency and confirmed by readable Russian). Other escapes are left as they are.
"""
import argparse
import json
import re
import struct
import sys
from pathlib import Path

MAGIC = b'LOCA'


def key_hash(name):
    value = 0
    for byte in name.upper().encode('latin-1'):
        value = (value * 65599 + byte) & 0xffffffff
    return value


def parse_loc(data):
    if len(data) < 20 or data[:4] != MAGIC:
        raise ValueError("not a LOCA file")
    version, count, table_bytes, string_bytes = struct.unpack_from('<4I', data, 4)
    if version != 2 or table_bytes != count * 8 or 20 + table_bytes + string_bytes != len(data):
        raise ValueError("LOCA header sizes do not match the file")
    base, entries, previous = 20 + table_bytes, {}, -1
    for i in range(count):
        key, offset = struct.unpack_from('<II', data, 20 + i * 8)
        if key <= previous:
            raise ValueError("keys are not sorted")
        if offset >= string_bytes:
            raise ValueError("string offset outside the string block")
        end = data.find(b'\0', base + offset)
        if end < 0:
            raise ValueError("unterminated string")
        entries[key] = data[base + offset:end].decode('latin-1')
        previous = key
    return entries


def decode_text(text, extended):
    """Replace 0x01 '2' c escapes by extended[ord(c) - 0x30]."""
    out, i = [], 0
    while i < len(text):
        if text[i] == '\x01' and i + 2 < len(text) and text[i + 1] == '2' and \
                0 <= ord(text[i + 2]) - 0x30 < len(extended):
            out.append(extended[ord(text[i + 2]) - 0x30])
            i += 3
        else:
            out.append(text[i])
            i += 1
    return ''.join(out)


def candidate_names(paths):
    names = set()
    for path in paths:
        for match in re.finditer(rb'[A-Za-z0-9_]{2,64}', path.read_bytes()):
            names.add(match.group().decode().upper())
    return {key_hash(name): name for name in names}


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('inputs', type=Path, nargs='+', help='.loc files or directories')
    parser.add_argument('--census', action='store_true')
    parser.add_argument('--extended', type=Path, help='extended_chars_utf8.txt for decoding escapes')
    parser.add_argument('--names', type=Path, action='append', default=[],
                        help='files to take candidate key names from (local use; output is game-derived)')
    args = parser.parse_args()
    files = []
    for item in args.inputs:
        files += sorted(item.rglob('*.loc')) if item.is_dir() else [item]
    names = candidate_names(args.names)
    extended = args.extended.read_text(encoding='utf-8').strip() if args.extended else ''

    failures, totals, tables = [], {'files': 0, 'entries': 0, 'named': 0}, {}
    for path in files:
        try:
            entries = parse_loc(path.read_bytes())
        except ValueError as exc:
            failures.append({'path': str(path), 'error': str(exc)})
            continue
        totals['files'] += 1
        totals['entries'] += len(entries)
        totals['named'] += sum(1 for key in entries if key in names)
        tables[path.name] = {names.get(key, '%08x' % key): decode_text(text, extended)
                             for key, text in entries.items()}
    if args.census:
        print(json.dumps({'failure_count': len(failures), 'failures': failures[:10], 'totals': totals}, indent=1))
    else:
        json.dump(tables, sys.stdout, indent=1, ensure_ascii=False)
        print()
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
