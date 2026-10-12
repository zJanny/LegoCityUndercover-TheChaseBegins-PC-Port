#!/usr/bin/env python3
"""Parse TT Fusion navigation grids (.bfnnav) in the block order of loader 0x1971c4.

A .bfnnav belongs to one room of a level; its counts and placement come from the
level's navigation descriptor (convert_levels.nav_rooms):
  - 0x3c-byte grid records: u16 x cell, u16 height, u16 z cell, u16 width,
    u16 (unknown, ~1400 in lvl_00), u16 depth, 16 u16 neighbour links at +0x14
    (0xffff = none), u16 +0x34 count of 4-byte extra entries (bits 0-13);
  - per grid: width x depth cell bytes (bit 0 set on edges and obstacles, bits 2-4
    edge direction; meanings inferred from the viewer overlay),
    then the extra entries when present;
  - 0x20-byte path records: f32 x3 position, f32 x3, u32 +0x18 index count;
  - per path record: u16 indices.
World position of a cell (data fit, not traced in code): x = origin.x + cell * gx,
z = origin.z + cell * gz, y = origin.y + height * height_unit / 256.
"""
import argparse
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import tt_resource  # noqa: E402
import convert_levels  # noqa: E402

GRID = 0x3c
PATH = 0x20


def padded(size, wanted):
    """Pooled block sizes are rounded up to a multiple of four."""
    return size == wanted or (size % 4 == 0 and wanted <= size < wanted + 4)


def parse_nav(data):
    blocks = iter(tt_resource.parse(data)['blocks'])

    def take():
        try:
            block = next(blocks)
        except StopIteration:
            raise ValueError("navigation file ends early") from None
        return data[block['offset']:block['offset'] + block['size']]

    records = take()
    if len(records) % GRID:
        raise ValueError("grid block is not a multiple of 0x3c")
    grids = []
    for i in range(len(records) // GRID):
        raw = records[i * GRID:(i + 1) * GRID]
        x, height, z, width, unknown, depth = struct.unpack_from('<6H', raw, 0)
        cells = take()
        if not padded(len(cells), width * depth):
            raise ValueError("grid %d: %d cell bytes for %dx%d" % (i, len(cells), width, depth))
        extra_count = struct.unpack_from('<H', raw, 0x34)[0] & 0x3fff
        extra = take() if extra_count else b''
        if len(extra) != extra_count * 4:
            raise ValueError("grid %d: extra block size mismatch" % i)
        grids.append({'x': x, 'z': z, 'height': height, 'width': width, 'depth': depth, 'unknown': unknown,
                      'links': [n for n in struct.unpack_from('<16H', raw, 0x14) if n != 0xffff],
                      'cells': cells[:width * depth], 'extra': extra})
    path_block = take()
    if len(path_block) % PATH:
        raise ValueError("path block is not a multiple of 0x20")
    paths = []
    for i in range(len(path_block) // PATH):
        raw = path_block[i * PATH:(i + 1) * PATH]
        count = struct.unpack_from('<I', raw, 0x18)[0]
        indices = take()
        if not padded(len(indices), count * 2):
            raise ValueError("path %d: index block size mismatch" % i)
        paths.append({'position': struct.unpack_from('<3f', raw, 0), 'extent': struct.unpack_from('<3f', raw, 12),
                      'indices': list(struct.unpack_from('<%dH' % count, indices, 0))})
    if next(blocks, None) is not None:
        raise ValueError("unconsumed blocks")
    return {'grids': grids, 'paths': paths}


def cell_world(room, grid, gx, gz):
    """Centre of cell (gx, gz) of a grid in world space (see module docstring)."""
    ox, oy, oz = room['origin']
    return (ox + room['cell'] * (grid['x'] + gx + 0.5), oy + grid['height'] * room['height_unit'] / 256,
            oz + room['cell'] * (grid['z'] + gz + 0.5))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('files', type=Path, help='unpacked .fib files/ directory')
    parser.add_argument('--census', action='store_true')
    args = parser.parse_args()
    failures, totals = [], {'files': 0, 'grids': 0, 'cells': 0, 'flagged': 0, 'paths': 0, 'descriptor_checked': 0}
    for level_path in sorted(args.files.rglob('*.blvl')):
        level = convert_levels.parse_level(level_path.read_bytes())
        for room in convert_levels.nav_rooms(level):
            nav_path = level_path.with_name('%s_%s.bfnnav' % (level_path.stem, room['room'].lower()))
            if not nav_path.exists():
                continue
            try:
                nav = parse_nav(nav_path.read_bytes())
                if (len(nav['grids']), len(nav['paths'])) != (room['grids'], room['paths']):
                    raise ValueError("counts %d/%d differ from the level descriptor %d/%d" % (
                        len(nav['grids']), len(nav['paths']), room['grids'], room['paths']))
                totals['descriptor_checked'] += 1
            except ValueError as exc:
                failures.append({'path': str(nav_path), 'error': str(exc)})
    for nav_path in sorted(args.files.rglob('*.bfnnav')):
        try:
            nav = parse_nav(nav_path.read_bytes())
        except ValueError as exc:
            failures.append({'path': str(nav_path), 'error': str(exc)})
            continue
        totals['files'] += 1
        totals['grids'] += len(nav['grids'])
        totals['paths'] += len(nav['paths'])
        for grid in nav['grids']:
            totals['cells'] += len(grid['cells'])
            totals['flagged'] += sum(c & 1 for c in grid['cells'])
    print(json.dumps({'failure_count': len(failures), 'failures': failures[:10], 'totals': totals}, indent=1))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
