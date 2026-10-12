#!/usr/bin/env python3
"""Split TT Fusion binary resources (.bfnmdl, .btga, .bwav, ...) into pool blocks.

Layout, from the loader at 0x24b740 / 0x253800 and checked against every
shipped file (see progress log):
  u32 table_words << 8 (low byte 0), u32 header_bytes,
  table of table_words u32 words: a pool marker (-14, -15 or -16) followed
    by the sizes of that pool's blocks, repeated,
  data of all -16 blocks (header_bytes covers table + -16 data),
  data of all -15 and -14 blocks in table order, ending at end of file.
The game hands blocks out in this order (0x253800) while deserializing; the
meaning of each block is per resource type and is not decoded here.
"""
import argparse
import json
import struct
import sys
from pathlib import Path

MARKERS = {-14: 'm14', -15: 'm15', -16: 'm16'}


def parse(data):
    """Return {'table_words', 'header_bytes', 'blocks': [{pool, size, offset}]}."""
    if len(data) < 8:
        raise ValueError("truncated resource header")
    first, header_bytes = struct.unpack_from('<II', data, 0)
    words = first >> 8
    if first & 0xff or not words or 8 + 4 * words > 8 + header_bytes or 8 + header_bytes > len(data):
        raise ValueError("not a pooled resource header")
    table, pool = [], None
    for word in struct.unpack_from('<%di' % words, data, 8):
        if word in MARKERS:
            pool = word
        elif pool is None or word < 0:
            raise ValueError("block size before pool marker or negative size")
        else:
            table.append([pool, word])
    offset = 8 + 4 * words
    blocks, cursor = [], offset
    for pool, size in table:
        if pool == -16:
            blocks.append({'pool': MARKERS[pool], 'size': size, 'offset': cursor})
            cursor += size
    if cursor != 8 + header_bytes:
        raise ValueError("static (-16) data does not fill header")
    for pool, size in table:
        if pool != -16:
            blocks.append({'pool': MARKERS[pool], 'size': size, 'offset': cursor})
            cursor += size
    if cursor != len(data):
        raise ValueError("blocks end at 0x%x, file is 0x%x" % (cursor, len(data)))
    blocks.sort(key=lambda b: b['offset'])
    return {'table_words': words, 'header_bytes': header_bytes, 'blocks': blocks}


def preview(chunk, limit=48):
    text = ''.join(chr(b) if 32 <= b < 127 else '.' for b in chunk[:limit])
    return chunk[:limit].hex(' ', 4), text


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('inputs', type=Path, nargs='+')
    parser.add_argument('--census', action='store_true', help='parse every file; summarize by extension')
    args = parser.parse_args()
    if not args.census:
        for path in args.inputs:
            data = path.read_bytes()
            info = parse(data)
            print('%s table_words=%d header=0x%x blocks=%d' % (
                path, info['table_words'], info['header_bytes'], len(info['blocks'])))
            for i, b in enumerate(info['blocks']):
                hexed, text = preview(data[b['offset']:b['offset'] + b['size']])
                print('  #%-3d %s @0x%06x size=0x%-6x %s |%s|' % (i, b['pool'], b['offset'], b['size'], hexed, text))
        return 0
    files = []
    for item in args.inputs:
        files += sorted(p for p in item.rglob('*') if p.is_file()) if item.is_dir() else [item]
    summary, failures = {}, []
    for path in files:
        entry = summary.setdefault(path.suffix or '(none)', {'ok': 0, 'fail': 0, 'blocks': 0, 'pools': {}})
        try:
            info = parse(path.read_bytes())
        except (ValueError, struct.error) as exc:
            entry['fail'] += 1
            failures.append({'path': str(path), 'error': str(exc)})
            continue
        entry['ok'] += 1
        entry['blocks'] += len(info['blocks'])
        for block in info['blocks']:
            entry['pools'][block['pool']] = entry['pools'].get(block['pool'], 0) + 1
    print(json.dumps({'summary': dict(sorted(summary.items())), 'failures': failures[:30],
                      'failure_count': len(failures)}, indent=1))
    return 0


if __name__ == '__main__':
    sys.exit(main())
