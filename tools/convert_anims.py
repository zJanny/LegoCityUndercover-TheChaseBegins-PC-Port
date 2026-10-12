#!/usr/bin/env python3
"""Parse TT Fusion skeletons (.bfnskl) and animations (.bfnanm) by replaying
their loaders (Ghidra): skeleton 0x10f26c, animation 0x10edec with track
records 0x253764. Both formats use only the -14 pool, consumed in order.

Every block must be consumed exactly once (checked). Keyframe encodings and
the evaluation code are not recovered; tracks are reported structurally.
"""
import argparse
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from convert_models import Pools, cstring, u16, u32  # noqa: E402


def parse_skeleton(data):
    pools = Pools(data)
    pools.push('m14')
    head = pools.next()
    count = head[1]
    b8, bc = pools.next(), pools.next()
    joints_block = pools.next()
    joints = []
    for i in range(count):
        record = joints_block[i * 0x98:(i + 1) * 0x98]
        # +4 parent index (-1 root); +8 world bind matrix and +0x48 parent-relative
        # matrix, both row-major 4x4 with the translation in the last row.
        joints.append({'name': cstring(pools.next()), 'raw': record,
                       'parent': struct.unpack_from('<i', record, 4)[0],
                       'world': [list(struct.unpack_from('<4f', record, 8 + 16 * r)) for r in range(4)],
                       'local': [list(struct.unpack_from('<4f', record, 0x48 + 16 * r)) for r in range(4)]})
    left = pools.leftover()
    if left:
        raise ValueError("unconsumed blocks: %s" % left)
    return {'header': head, 'joints': joints, 'b8': b8, 'bc': bc}


def matmul(a, b):
    return [[sum(a[i][k] * b[k][j] for k in range(4)) for j in range(4)] for i in range(4)]


def check_hierarchy(skel, tolerance=2e-3):
    """Parents precede children and world == local * parent.world (row vectors)."""
    worst = 0.0
    for index, joint in enumerate(skel['joints']):
        parent = joint['parent']
        if parent >= index or parent < -1:
            raise ValueError("joint %d has parent %d" % (index, parent))
        expect = joint['local'] if parent < 0 else matmul(joint['local'], skel['joints'][parent]['world'])
        scale = max(1.0, max(abs(x) for row in joint['world'] for x in row))
        worst = max(worst, max(abs(expect[i][j] - joint['world'][i][j]) for i in range(4) for j in range(4)) / scale)
    if worst > tolerance:
        raise ValueError("world != local * parent.world (max relative error %.4g)" % worst)
    return worst


def read_track(pools, record):
    """0x253764: a 0x18 record; first block's u32 is copied to +8, then the pointer
    array at +0xc (its own block) with u16(+6) entry blocks, then +0x10 and +0x14."""
    first = pools.next()
    pools.next()
    keys = [pools.next() for _ in range(u16(record, 6))]
    return {'raw': record, 'first': first, 'keys': keys, 'b10': pools.next(), 'b14': pools.next()}


def parse_animation(data):
    pools = Pools(data)
    pools.push('m14')
    head = pools.next()
    anim = {'header': head, 'tracks': [], 'sub': []}
    if u32(head, 8):
        table = pools.next()
        count = (u32(table, 0) & 0x1fff) + ((u32(table, 0) & 0x10000000) >> 28)
        pools.next()
        anim['s8'] = [pools.next() for _ in range(count)]
    if u32(head, 0xc):
        pools.next()
        anim['sc'] = [[pools.next() for _ in range(3)] for _ in range(head[1])]
    if u32(head, 0x10):
        pools.next()
        anim['s10'] = [[pools.next() for _ in range(2)] for _ in range(head[2])]
    if head[3]:
        pools.next()
        anim['s14'] = [[pools.next() for _ in range(3)] for _ in range(head[3])]
    if u16(head, 6):
        tracks = pools.next()
        anim['tracks'] = [read_track(pools, tracks[i * 0x18:(i + 1) * 0x18]) for i in range(u16(head, 6))]
    if u32(head, 0x20):
        pools.next()
        for _ in range(u32(head, 0x20)):
            fixups = pools.next()
            count = u32(pools.next(), 0)
            tracks = pools.next()
            subtracks = [read_track(pools, tracks[i * 0x18:(i + 1) * 0x18]) for i in range(count)]
            pools.next()
            for _ in range(u32(fixups, 0)):
                pools.next()
            anim['sub'].append(subtracks)
    left = pools.leftover()
    if left:
        raise ValueError("unconsumed blocks: %s" % left)
    return anim


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('inputs', type=Path, nargs='+')
    parser.add_argument('--census', action='store_true')
    args = parser.parse_args()
    files = []
    for item in args.inputs:
        files += sorted(p for p in item.rglob('*') if p.suffix in ('.bfnskl', '.bfnanm')) if item.is_dir() else [item]
    failures, totals = [], {'skeletons': 0, 'joints': 0, 'animations': 0, 'tracks': 0, 'subanims': 0}
    for path in files:
        try:
            if path.suffix == '.bfnskl':
                skel = parse_skeleton(path.read_bytes())
                totals['worst_hierarchy_error'] = max(totals.get('worst_hierarchy_error', 0.0), check_hierarchy(skel))
                totals['skeletons'] += 1
                totals['joints'] += len(skel['joints'])
                if not args.census:
                    print(path, [j['name'] for j in skel['joints']])
            else:
                anim = parse_animation(path.read_bytes())
                totals['animations'] += 1
                totals['tracks'] += len(anim['tracks'])
                totals['subanims'] += len(anim['sub'])
                if not args.census:
                    print(path, 'tracks=%d sub=%d header=%s' % (len(anim['tracks']), len(anim['sub']),
                                                                anim['header'][:0x24].hex(' ', 4)))
        except (ValueError, struct.error, IndexError, TypeError) as exc:
            failures.append({'path': str(path), 'error': '%s: %s' % (type(exc).__name__, exc)})
    if args.census:
        print(json.dumps({'files': len(files), 'failure_count': len(failures), 'failures': failures[:12],
                          'totals': totals}, indent=1))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
