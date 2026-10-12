#!/usr/bin/env python3
"""Parse TT Fusion .bfnmdl models by replaying the game's deserializer.

The block order mirrors the loader recovered with Ghidra (see progress log):
0x10ccf4 (model), 0x114860 (material), 0x11528c (vertex buffer),
0x114640 / 0x11fd20 (optional section at model+0x1c). Every pool block must
be consumed exactly once, which is checked; a NULL pointer is a size-0 block.

Field meanings are those the loader itself relies on (counts, strides, the
texture-name list). Anything else is reported raw and must not be treated
as recovered semantics.
"""
import argparse
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from tt_resource import parse as parse_pools  # noqa: E402


class Pools:
    """Hand out blocks per pool in table order, like 0x253800."""

    def __init__(self, data):
        self.data = data
        self.queues = {'m14': [], 'm15': [], 'm16': []}
        for block in parse_pools(data)['blocks']:
            self.queues[block['pool']].append(block)
        self.cursor = {k: 0 for k in self.queues}
        self.stack = []

    def push(self, pool):
        self.stack.append(pool)

    def pop(self):
        self.stack.pop()

    def next(self):
        """Return block bytes of the current pool, or None for a NULL (size-0) block."""
        pool = self.stack[-1]
        index = self.cursor[pool]
        if index >= len(self.queues[pool]):
            raise ValueError("pool %s exhausted" % pool)
        self.cursor[pool] += 1
        block = self.queues[pool][index]
        if not block['size']:
            return None
        return self.data[block['offset']:block['offset'] + block['size']]

    def leftover(self):
        return {k: len(q) - self.cursor[k] for k, q in self.queues.items() if len(q) != self.cursor[k]}


def u8(b, o):
    return b[o]


def u16(b, o):
    return struct.unpack_from('<H', b, o)[0]


def u32(b, o):
    return struct.unpack_from('<I', b, o)[0]


def cstring(block):
    return block.split(b'\0', 1)[0].decode('latin-1') if block else None


# Tables read by 0x12095c (rodata 0x3798c4, 0x3798d1, 0x3798e9, 0x3798f8).
# The u16 table at 0x3798c4 overlaps the byte table at 0x3798d1, exactly as in memory.
_RODATA = bytes.fromhex(
    '80004000c00080000000400000004000' '000000000000000b' '0b03030b040b0303' '0b0608090a'  # 0x3798c4..e8
    '00010100010b0b0b0b010b0b0b000b'                                                  # 0x3798e9..f7
    '00040f00040f0f0f000f000000000000')                                               # 0x3798f8..
assert len(_RODATA) == 0x44
BASE_FLAGS = struct.unpack_from('<12H', _RODATA, 0)    # 0x3798c4
BY_M1 = _RODATA[0x0d:0x25]                             # 0x3798d1
BY_M2 = _RODATA[0x25:0x34]                             # 0x3798e9
M2_EXTRA = _RODATA[0x34:]                              # 0x3798f8


def vertex_flags(m):
    """Replica of 0x12095c: vertex attribute mask from the material record."""
    if m[1] < 9:
        index = BY_M2[m[2]]
        if index == 0 and m[3] in (5, 6, 7):
            index = 2
    else:
        index = BY_M1[m[1]]
    flags = BASE_FLAGS[index]
    flags |= {0: 0, 1: 2, 2: 1, 3: 3}[m[0]]
    if m[10] & 7:
        flags |= 4 if (m[10] & 7) == 1 else 0xc
    if m[10] & 0x40 or M2_EXTRA[m[2]] == 4 or m[3] == 7:
        flags |= 0x80
    return flags


# Attribute order and sizes, recovered from shipped vertex data (see progress log):
# position f32x3 | uv0 s16x2 (4) | uv1 s16x2 (8) | normal s8x3+pad (0x40) |
# colour RGBA8 (0x80) | 8 bytes, probably skin weights/indices (2).
ATTRIBUTES = ((4, 'uv0', 4), (8, 'uv1', 4), (0x40, 'normal', 4), (0x80, 'color', 4), (2, 'skin', 8))


def vertex_layout(flags):
    if flags & 1:
        raise ValueError("vertex flag 1 layout not recovered")
    layout, offset = [('position', 0)], 12
    for bit, name, size in ATTRIBUTES:
        if flags & bit:
            layout.append((name, offset))
            offset += size
    return layout, offset


def decode_vertices(part, offset=None):
    """Decode a part's vertices; `offset` (the mesh centre) is added to positions when given.

    Mesh byte 0 bit 0 marks vertices stored relative to the mesh centre at mesh+8
    (checked on all 6648 shipped meshes); use mesh_offset() to obtain it."""
    buffer = part['buffer']
    layout, stride = vertex_layout(vertex_flags(part['material']['raw']))
    if stride != buffer['stride']:
        raise ValueError("computed stride %d != stored %d" % (stride, buffer['stride']))
    data, out = buffer['vertex_data'], []
    # UV scales live in the vertex-buffer header at +0x50 (uv0.xy, uv1.xy); the
    # vertex shaders' "UVScales" uniform multiplies the raw s16 values by them.
    scales = {'uv0': struct.unpack_from('<2f', buffer['header'], 0x50),
              'uv1': struct.unpack_from('<2f', buffer['header'], 0x58)}
    for v in range(buffer['vertices']):
        base, vertex = v * stride, {}
        for name, offset in layout:
            at = base + offset
            if name == 'position':
                vertex[name] = struct.unpack_from('<3f', data, at)
                if offset:
                    vertex[name] = tuple(a + b for a, b in zip(vertex[name], offset))
            elif name in ('uv0', 'uv1'):
                vertex[name] = tuple(x * s for x, s in zip(struct.unpack_from('<2h', data, at), scales[name]))
            elif name == 'normal':
                vertex[name] = tuple(x / 127 for x in struct.unpack_from('<3b', data, at))
            elif name == 'color':
                vertex[name] = tuple(data[at:at + 4])
            else:
                vertex[name] = tuple(data[at:at + 8])
        out.append(vertex)
    return out


def mesh_offset(mesh):
    return mesh['f8'] if mesh['raw'][0] & 1 else None


def triangles(part):
    count = part['buffer']['indices']
    return [struct.unpack_from('<3H', part['buffer']['index_data'], i * 2) for i in range(0, count, 3)]


def read_material(pools):
    """0x114860: material record (0x60), extra pointer, texture slots (-16 pool)."""
    material = pools.next()
    extra = pools.next()
    count = (material[10] & 0x3f) >> 3
    pools.push('m16')
    slots = pools.next()
    names = [cstring(pools.next()) for _ in range(count)]
    pools.pop()
    textures = []
    for i, name in enumerate(names):
        if name is not None:
            textures.append({'name': name, 'u16': u16(slots, i * 8 + 4), 'b6': slots[i * 8 + 6],
                             'b7': slots[i * 8 + 7]})
    return {'raw': material, 'extra': extra, 'textures': textures, 'uv_sets': material[10] & 7}


def read_vertex_buffer(pools):
    """0x11528c: header (0x78), optional block, index group, vertex group, optional group."""
    header = pools.next()
    if u32(header, 0x24):
        pools.next()
    vertices, indices, stride = u16(header, 2), u32(header, 4), u16(header, 0x1a)
    pools.push('m14')
    index_data = pools.next()
    vertex_data = pools.next()
    extra = pools.next() if u32(header, 0x48) else None
    pools.pop()
    expected = stride * vertices
    if u32(header, 0x44):
        expected += vertices * (u32(header, 0x40) - 1) * 12
    if vertices and (vertex_data is None or len(vertex_data) != expected):
        raise ValueError("vertex block %s != %d x %d" % (len(vertex_data or b''), vertices, stride))
    if indices and (index_data is None or len(index_data) != (indices + 1) // 2 * 4):
        raise ValueError("index block size mismatch")
    return {'vertices': vertices, 'indices': indices, 'stride': stride, 'header': header,
            'index_data': index_data or b'', 'vertex_data': vertex_data or b'',
            'morph_frames': u32(header, 0x40) if u32(header, 0x44) else 0, 'extra': extra}


def read_section_1c(pools, count):
    """0x114640 (pool -14): per item several blocks, chunked arrays, 0x11fd20 when flagged."""
    pools.push('m14')
    items = pools.next()
    for i in range(count):
        pools.next()
        first = pools.next()
        second = pools.next()
        _ = first or second
        pools.next()
        pools.next()
    for i in range(count):
        item = items[i * 0x2c:(i + 1) * 0x2c]
        for _ in range(0, u32(item, 0x10), 0x400):
            pools.next()
        for _ in range(0, u32(item, 0x14), 0x400):
            pools.next()
        pools.next()
        if u32(item, 0x28):
            pools.next()
            pools.next()
            pools.next()
            pools.next()
    pools.pop()


def parse_model(data):
    pools = Pools(data)
    pools.push('m15')
    model = pools.next()
    flags, n_nodes, n_meshes, n_textures = u16(model, 0), u16(model, 2), u16(model, 4), model[6]
    pools.next()                                        # +0x08
    nodes_block = pools.next()                          # +0x0c, n_nodes x 0x18
    nodes = []
    for i in range(n_nodes):
        node = nodes_block[i * 0x18:(i + 1) * 0x18]
        name = cstring(pools.next())
        if u32(node, 0x14):
            pools.next()
        nodes.append({'name': name, 'f4': struct.unpack_from('<3f', node, 4)})
    meshes_block = pools.next()                         # +0x10, n_meshes x 0x24
    parts_blocks = [pools.next() for _ in range(n_meshes)]
    pools.next()                                        # +0x14 texture pointer array
    if u32(model, 0x20):
        pools.next()
    pools.pop()
    if u32(model, 0x1c):
        pools.push('m15')
        section = pools.next()
        if u32(section, 0):
            pools.next()
            for _ in range(u32(section, 0)):
                pools.next()
        pools.pop()
        if u32(section, 0xc):
            read_section_1c(pools, u32(section, 8))
    meshes = []
    pools.push('m15')
    for m in range(n_meshes):
        mesh = meshes_block[m * 0x24:(m + 1) * 0x24]
        n_parts = mesh[2]
        parts = []
        materials = [read_material(pools) for _ in range(n_parts)]
        pools.push('m14')
        for p in range(n_parts):
            parts.append({'material': materials[p], 'buffer': read_vertex_buffer(pools),
                          'raw': parts_blocks[m][p * 0x24:(p + 1) * 0x24]})
        pools.pop()
        meshes.append({'raw': mesh, 'f8': struct.unpack_from('<3f', mesh, 8), 'parts': parts})
    pools.pop()
    pools.push('m16')
    records = pools.next()
    textures = []
    for i in range(n_textures):
        first = pools.next()
        name = cstring(pools.next())
        textures.append({'name': name, 'record': records[i * 0x28:(i + 1) * 0x28] if records else b'',
                         'first': first})
    pools.pop()
    left = pools.leftover()
    if left:
        raise ValueError("unconsumed blocks: %s" % left)
    return {'flags': flags, 'nodes': nodes, 'meshes': meshes, 'textures': textures}


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('inputs', type=Path, nargs='+')
    parser.add_argument('--census', action='store_true')
    parser.add_argument('--vertices', type=int, default=0, help='print the first N raw vertices of each part')
    args = parser.parse_args()
    files = []
    for item in args.inputs:
        files += sorted(item.rglob('*.bfnmdl')) if item.is_dir() else [item]
    failures, strides, prims, examples, checks = [], {}, {}, {}, {}
    totals = {'parts': 0, 'vertices': 0, 'triangles': 0}
    for path in files:
        try:
            model = parse_model(path.read_bytes())
        except (ValueError, struct.error, IndexError, TypeError) as exc:
            failures.append({'path': str(path), 'error': '%s: %s' % (type(exc).__name__, exc)})
            continue
        for mesh in model['meshes']:
            for part in mesh['parts']:
                m, b = part['material']['raw'], part['buffer']
                key = 'm0=%d m1=%d m2=%d m3=%d uv=%d tex=%d b10=0x%02x stride=%d' % (
                    m[0], m[1], m[2], m[3], m[10] & 7, (m[10] & 0x3f) >> 3, m[10], b['stride'])
                strides[key] = strides.get(key, 0) + 1
                examples.setdefault(key, str(path))
                try:
                    computed = vertex_layout(vertex_flags(m))[1]
                    verdict = 'match' if computed == b['stride'] else 'mismatch %s' % key
                except (ValueError, IndexError) as exc:
                    verdict = 'error %s' % exc
                checks[verdict] = checks.get(verdict, 0) + 1
                prims['idx%%3=%d' % (b['indices'] % 3)] = prims.get('idx%%3=%d' % (b['indices'] % 3), 0) + 1
                totals['parts'] += 1
                totals['vertices'] += b['vertices']
                totals['triangles'] += b['indices'] // 3
        if args.vertices:
            for mi, mesh in enumerate(model['meshes']):
                for pi, part in enumerate(mesh['parts']):
                    b, m = part['buffer'], part['material']['raw']
                    print('%s mesh%d part%d stride=%d m0-3=%s b10=0x%02x verts=%d' % (
                        path.name, mi, pi, b['stride'], m[:4].hex(), m[10], b['vertices']))
                    for v in range(min(args.vertices, b['vertices'])):
                        raw = b['vertex_data'][v * b['stride']:(v + 1) * b['stride']]
                        pos = struct.unpack_from('<3f', raw)
                        rest = raw[12:]
                        print('  pos=(%8.3f %8.3f %8.3f) rest=%s s16=%s' % (
                            *pos, rest.hex(' ', 4), struct.unpack('<%dh' % (len(rest) // 2), rest)))
            continue
        if not args.census:
            print(json.dumps({'path': str(path), 'nodes': model['nodes'], 'textures': [t['name'] for t in model['textures']],
                              'meshes': [{'f8': m['f8'], 'parts': [
                                  {'vertices': p['buffer']['vertices'], 'indices': p['buffer']['indices'],
                                   'stride': p['buffer']['stride'], 'textures': p['material']['textures']}
                                  for p in m['parts']]} for m in model['meshes']]}, indent=1, default=str))
    if args.census:
        print(json.dumps({'files': len(files), 'failure_count': len(failures), 'failures': failures[:15],
                          'primitive_hint': prims, 'stride_check': checks, 'totals': totals,
                          'layouts': {k: [v, examples[k]] for k, v in
                                      sorted(strides.items(), key=lambda kv: -kv[1])[:40]}}, indent=1))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
