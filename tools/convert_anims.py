#!/usr/bin/env python3
"""Parse TT Fusion skeletons (.bfnskl) and animations (.bfnanm) by replaying
their loaders (Ghidra): skeleton 0x10f26c, animation 0x10edec with track
records 0x253764. Both formats use only the -14 pool, consumed in order.

Every block must be consumed exactly once (checked). Keyframe encodings and
the evaluation code are not recovered; tracks are reported structurally.
"""
import argparse
import json
import math
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


# Channel table at 0x376494 (base bits, delta-width bits) with scales written by the
# static initializer at 0x347c5c: channels 0-2 rotation (0.02), 3-5 scale (0.01,
# default 1.0), 6-8 unknown (0.01), 9-11 translation (0.01, 16-bit base).
CHANNELS = [(12, 4, 0.02)] * 3 + [(12, 4, 0.01)] * 6 + [(16, 5, 0.01)] * 3


class Bits:
    """LSB-first bit reader (0x247a48)."""

    def __init__(self, data):
        self.data, self.pos = data, 0

    def peek(self, at, count):
        value = int.from_bytes(self.data[at >> 3:(at >> 3) + 5].ljust(5, b'\0'), 'little')
        return (value >> (at & 7)) & ((1 << count) - 1)

    def read(self, count):
        value = self.peek(self.pos, count)
        self.pos += count
        return value


def decode_channels(blob, frame, frames):
    """Replica of 0x248910: 12 channels of one joint at `frame`, plus the channel mask.

    Returns (channels, mask); mask 0xfff with raw=matrix for the 0xfa raw-matrix form."""
    bits = Bits(blob)
    head = bits.read(8)
    if head == 0xfa:
        start = 1 + frame * 0x40
        return list(struct.unpack_from('<16f', blob, start)), -1
    shift, mask = head >> 4, 0
    for group in range(4):
        if head & (1 << group):
            mask |= bits.read(3) << (3 * group)
    extra = 0
    if shift & 8:
        shift, extra = shift & 7, 2
    out = []
    for channel in range(12):
        if not mask & (1 << channel):
            out.append(1.0 if 3 <= channel < 6 else 0.0)
            continue
        base_bits, width_bits, scale = CHANNELS[channel]
        if shift:
            base_bits += shift + (extra if channel >= 9 else 0)
            width_bits += shift
            scale /= 1 << shift
        base = bits.read(base_bits)
        if base & (1 << (base_bits - 1)):
            base -= 1 << base_bits
        width = bits.peek(bits.pos, width_bits) & 0xff
        if width:
            base += bits.peek(bits.pos + width_bits + frame * width, width)
        bits.pos += width_bits + frames * width
        value = base * scale
        if 3 <= channel < 6 and value < 0.0001:
            value = 0.0001
        out.append(value)
    return out, mask


def euler_to_quaternion(x, y, z):
    """Replica of 0x248248 (sin/cos order from 0x247ac8 = sin, 0x247c90 = cos); returns (x, y, z, w)."""
    sx, cx = math.sin(x / 2), math.cos(x / 2)
    sy, cy = math.sin(y / 2), math.cos(y / 2)
    sz, cz = math.sin(z / 2), math.cos(z / 2)
    return (-(cy * sx * cz) - sy * cx * sz, cy * sx * sz - sy * cx * cz,
            -(sy * sx * cz) - cy * cx * sz, cy * cx * cz - sy * sx * sz)


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
