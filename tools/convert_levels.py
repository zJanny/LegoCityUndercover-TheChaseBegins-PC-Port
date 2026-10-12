#!/usr/bin/env python3
"""Parse TT Fusion .blvl level files by replaying loader 0x1cfc50 (Ghidra).

Every pool block must be consumed exactly once (checked). Section names below
are positional (by header offset); their gameplay meaning is not recovered
unless stated. Parameters use 0x208ae0: a 12-byte record whose u16 at +6 is
the type (1/2/3/6 one block, 4 one block if +8 is non-zero, 8 two blocks).
"""
import argparse
import json
import math
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from convert_models import Pools, cstring, u16, u32  # noqa: E402


def read_params(pools, block, count):
    params = []
    for i in range(count):
        item = block[i * 12:(i + 1) * 12]
        kind = u16(item, 6)
        values = []
        if kind in (1, 2, 3, 6) or (kind == 4 and u32(item, 8)):
            values.append(pools.next())
        elif kind == 8:
            values += [pools.next(), pools.next()]
        params.append({'raw': item, 'type': kind, 'blocks': values})
    return params


def parse_level(data):
    pools = Pools(data)
    pools.push('m15')
    head = pools.next()
    n30, n10, n14, n18, n1c, n20, n24, n2c = (head[1], u16(head, 2), u16(head, 4), u16(head, 6),
                                             u16(head, 8), u16(head, 10), u16(head, 12), u16(head, 14))
    level = {'header': head}
    level['s10'] = read_params(pools, pools.next(), n10)
    level['s14'] = read_params(pools, pools.next(), n14)
    block18 = pools.next()
    level['s18'] = []
    for i in range(n18):
        entry = block18[i * 0x1c:(i + 1) * 0x1c]
        pools.next()
        pools.next()
        for _ in range(u32(entry, 0x14)):
            pools.next()
        pools.push('m16')
        names = [cstring(pools.next()) for _ in range(u16(entry, 6))]
        pools.pop()
        level['s18'].append({'raw': entry, 'functions': names})
    block1c = pools.next()
    level['s1c'] = []
    for i in range(n1c):
        a, b = pools.next(), pools.next()
        pools.push('m16')
        kind = cstring(pools.next())
        strings = [cstring(pools.next()) for _ in range(2)]
        pools.pop()
        level['s1c'].append({'raw': block1c[i * 0x18:(i + 1) * 0x18], 'kind': kind, 'strings': strings,
                             'a': a, 'b': b})
    block20 = pools.next()
    level['s20'] = []
    for i in range(n20):
        entry = block20[i * 0x28:(i + 1) * 0x28]
        first = pools.next() if u32(entry, 4) else None
        second = pools.next()
        third = fourth = None
        if u32(entry, 0x10):
            third, fourth = pools.next(), pools.next()
        level['s20'].append({'raw': entry, 'blocks': [first, second, third, fourth]})
    block24 = pools.next()
    level['s24'] = []
    for i in range(n24):
        entry = block24[i * 0x28:(i + 1) * 0x28]
        name = pools.next()
        extra = pools.next() if u32(entry, 4) else None
        level['s24'].append({'raw': entry, 'name': cstring(name), 'extra': extra})
    level['s28'] = pools.next()
    block2c = pools.next()
    objects = []
    for i in range(n2c):
        entry = block2c[i * 0x18:(i + 1) * 0x18]
        a, b = pools.next(), pools.next()
        p1 = read_params(pools, pools.next(), entry[0xd])
        p2 = read_params(pools, pools.next(), entry[0xe])
        objects.append({'raw': entry, 'a': a, 'b': b, 'params1': p1, 'params2': p2})
    s34 = pools.next()
    if s34:
        pools.next()
        for _ in range(u32(s34, 0)):
            pools.next()
    pools.push('m16')
    for obj in objects:
        obj['name'] = cstring(pools.next())
    pools.pop()
    level['s2c'] = objects
    level['s30'] = []
    if n30:
        block30 = pools.next()
        for i in range(n30):
            entry = block30[i * 0x28:(i + 1) * 0x28]
            pools.next()
            names = [cstring(pools.next()) for _ in range(u16(entry, 0))]
            nav = pools.next()
            level['s30'].append({'raw': entry, 'names': names, 'nav': nav})
    left = pools.leftover()
    if left:
        raise ValueError("unconsumed blocks: %s" % left)
    return level


# Parameter names are CRC-32 (zlib polynomial) of the lower-cased name (hash 0x24da28).
INITIAL_POSITION = 0x86166f20     # crc32("initialposition")
INITIAL_ORIENTATION = 0xa3b9aa7f  # crc32("initialorientation"), degrees
MESH_NAME = 0xacd1ebb8            # crc32("meshname")


def param_names(code_path):
    """Map hash -> name using printable strings of the decompressed executable (local only)."""
    import re
    import zlib
    names = {}
    for match in re.finditer(rb'[\x20-\x7e]{2,64}', Path(code_path).read_bytes()):
        text = match.group().decode()
        names.setdefault(zlib.crc32(text.lower().encode()), text)
    return names


def param_value(param):
    raw, blocks = param['raw'], param['blocks']
    if param['type'] == 0:
        return struct.unpack_from('<f', raw, 8)[0]
    if param['type'] in (5, 7):
        return u32(raw, 8)
    if param['type'] == 2 and blocks and blocks[0] and len(blocks[0]) >= 12:
        return list(struct.unpack_from('<3f', blocks[0]))
    if param['type'] == 1 and blocks and blocks[0]:
        return cstring(blocks[0])
    return None


def placements(level):
    """Objects that carry a meshname, with initial position/orientation."""
    out = []
    for obj in level['s2c']:
        values = {u32(p['raw'], 0): param_value(p) for p in obj['params1'] + obj['params2']}
        if not values.get(MESH_NAME):
            continue
        out.append({'class': obj['name'], 'instance': cstring(obj['a']), 'room': cstring(obj['b']),
                    'mesh': values[MESH_NAME],
                    'position': values.get(INITIAL_POSITION) or [0.0, 0.0, 0.0],
                    'orientation_deg': values.get(INITIAL_ORIENTATION) or [0.0, 0.0, 0.0]})
    return out


def placement_matrix(placement):
    """Row-vector object matrix: rotation of 0x237c48 (Rz * Ry * Rx, x applied first) from
    the orientation in degrees (x 0.0174533, as at 0x1a48b0), position as translation row."""
    x, y, z = (a * 0.0174532942 for a in placement['orientation_deg'])
    sx, cx, sy, cy, sz, cz = math.sin(x), math.cos(x), math.sin(y), math.cos(y), math.sin(z), math.cos(z)
    return [[cy * cz, sx * sy * cz + cx * sz, sx * sz - cx * sy * cz, 0.0],
            [-cy * sz, cx * cz - sx * sy * sz, sx * cz + cx * sy * sz, 0.0],
            [sy, -sx * cy, cx * cy, 0.0],
            list(placement['position']) + [1.0]]


def nav_rooms(level):
    """Section +0x30: one navigation descriptor per room group, used by loader 0x1971c4.

    Floats at +0x0c..+0x1c: height range / 256, cell size, origin x, y, z. The block holds
    one 0x18 descriptor per room name, starting with u16 grid count and u16 path count of
    <level>_<room>.bfnnav."""
    out = []
    for entry in level['s30']:
        height_unit, cell, ox, oy, oz = struct.unpack_from('<5f', entry['raw'], 0xc)
        for i, name in enumerate(entry['names']):
            nav = entry['nav'][i * 0x18:(i + 1) * 0x18] if entry['nav'] else b''
            out.append({'room': name, 'cell': cell, 'origin': (ox, oy, oz), 'height_unit': height_unit,
                        'grids': u16(nav, 0) if nav else 0, 'paths': u16(nav, 2) if nav else 0})
    return out


def model_path(mesh):
    """Archive path for a meshname: 'Dir/File' -> models/dir/file, 'Name' -> models/name/name."""
    mesh = mesh.replace('\\', '/').lower()
    if '/' not in mesh:
        mesh = mesh + '/' + mesh
    return 'models/%s.bfnmdl' % mesh


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('inputs', type=Path, nargs='+')
    parser.add_argument('--census', action='store_true')
    parser.add_argument('--placements', action='store_true', help='print mesh placements as JSON')
    parser.add_argument('--param-names', type=Path, metavar='CODE_BIN',
                        help='resolve parameter hashes from artifacts/game/code.bin strings')
    args = parser.parse_args()
    if args.placements:
        for path in args.inputs:
            print(json.dumps([dict(p, model=model_path(p['mesh'])) for p in placements(parse_level(path.read_bytes()))],
                             indent=1))
        return 0
    if args.param_names:
        names = param_names(args.param_names)
        for path in args.inputs:
            level = parse_level(path.read_bytes())
            for obj in level['s2c']:
                print(obj['name'], cstring(obj['a']), cstring(obj['b']))
                for p in obj['params1'] + obj['params2']:
                    h = u32(p['raw'], 0)
                    print('   %-24s type=%d %s' % (names.get(h, '%08x' % h), p['type'], param_value(p)))
        return 0
    files = []
    for item in args.inputs:
        files += sorted(item.rglob('*.blvl')) if item.is_dir() else [item]
    failures, counts = [], {}
    for path in files:
        try:
            level = parse_level(path.read_bytes())
        except (ValueError, struct.error, IndexError, TypeError) as exc:
            failures.append({'path': str(path), 'error': '%s: %s' % (type(exc).__name__, exc)})
            continue
        for key in ('s10', 's14', 's18', 's1c', 's20', 's24', 's2c', 's30'):
            counts[key] = counts.get(key, 0) + len(level[key])
        if not args.census:
            print(json.dumps({'path': str(path), 'counts': {k: len(level[k]) for k in
                              ('s10', 's14', 's18', 's1c', 's20', 's24', 's2c', 's30')},
                              'objects': [o['name'] for o in level['s2c']][:40],
                              's24_names': [e['name'] for e in level['s24']][:40],
                              's1c': [(e['kind'], e['strings']) for e in level['s1c']][:20]}, indent=1))
    if args.census:
        print(json.dumps({'files': len(files), 'failure_count': len(failures), 'failures': failures[:15],
                          'section_totals': counts}, indent=1))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
