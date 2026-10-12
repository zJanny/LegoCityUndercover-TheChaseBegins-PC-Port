import importlib.util
import struct
import sys
import unittest
from pathlib import Path

TOOLS = Path(__file__).parents[1] / 'tools'
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location('convert_models', TOOLS / 'convert_models.py')
cm = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cm)


def pooled(blocks):
    """blocks: [(marker, bytes)] in table order -> pooled resource bytes."""
    table, static, body, current = [], b'', b'', None
    for marker, data in blocks:
        if marker != current:
            table.append(marker)
            current = marker
        table.append(len(data))
        if marker == -16:
            static += data
        else:
            body += data
    head = struct.pack('<II', len(table) << 8, 4 * len(table) + len(static))
    return head + struct.pack('<%di' % len(table), *table) + static + body


def model_bytes(extra_block=False):
    model = bytearray(0x24)
    struct.pack_into('<HHHB', model, 0, 0, 0, 1, 0)
    mesh = bytearray(0x24)
    mesh[2] = 1
    material = bytearray(0x60)
    material[10] = 0x09                     # one texture, one UV set -> flags 0x84, stride 20
    header = bytearray(0x78)
    struct.pack_into('<HHI', header, 0, 0, 3, 3)
    struct.pack_into('<H', header, 0x1a, 20)
    struct.pack_into('<4f', header, 0x50, 1 / 32767, 1 / 32767, 1, 1)   # UV scales
    vertices = b''.join(struct.pack('<3f2h4B', x, y, 0.0, u, v, 255, 0, 0, 255)
                        for x, y, u, v in ((0, 0, 0, 0), (1, 0, 32767, 0), (0, 1, 0, 32767)))
    slot = struct.pack('<IHBB', 1, 0, 5, 0)
    blocks = [(-15, bytes(model)), (-15, b''), (-15, b''), (-15, bytes(mesh)), (-15, bytes(0x24)),
              (-15, b''), (-15, bytes(material)), (-15, b''),
              (-16, slot), (-16, b'tex.tga\0'),
              (-14, bytes(header)), (-14, struct.pack('<4H', 0, 1, 2, 0)), (-14, vertices),
              (-16, b'')]
    if extra_block:
        blocks.append((-14, b'xx'))
    return pooled(blocks)


class ModelTests(unittest.TestCase):
    def test_parse_decode_and_triangles(self):
        model = cm.parse_model(model_bytes())
        part = model['meshes'][0]['parts'][0]
        self.assertEqual(part['material']['textures'][0]['name'], 'tex.tga')
        verts = cm.decode_vertices(part)
        self.assertEqual(verts[1]['position'], (1.0, 0.0, 0.0))
        self.assertEqual(cm.decode_vertices(part, (10.0, 0.0, -2.0))[1]['position'], (11.0, 0.0, -2.0))
        self.assertAlmostEqual(verts[2]['uv0'][1], 1.0, places=6)
        self.assertEqual(verts[2]['uv0'][0], 0.0)
        self.assertEqual(verts[0]['color'], (255, 0, 0, 255))
        self.assertEqual(cm.triangles(part), [(0, 1, 2)])

    def test_bone_batches_resolve_skin_joints(self):
        def part(shared_index=0, last_index=1):
            header = bytearray(0x78)
            struct.pack_into('<I', header, 0x20, 2)
            block = bytearray(2 * 0x110)
            for b, palette in enumerate(((3, 7), (3, 9))):
                struct.pack_into('<4I2B', block, b * 0x110, 1, 2, b, 3, *palette)   # vertices 0-2, 1-3
            return {'buffer': {'header': bytes(header), 'b24': bytes(block), 'indices': 6,
                               'index_data': struct.pack('<6H', 0, 1, 2, 1, 3, 2)}}, \
                [{'skin': (255, 0, 0, 0, j, 0, 0, 0)} for j in (1, shared_index, 0, last_index)]
        good, verts = part()
        self.assertEqual([b['palette'] for b in cm.bone_batches(good)], [[3, 7], [3, 9]])
        self.assertEqual([j[1][0] for j in cm.skin_joints(good, verts)], [7, 3, 3, 9])
        with self.assertRaisesRegex(ValueError, 'different joints'):
            cm.skin_joints(*part(shared_index=1))
        with self.assertRaisesRegex(ValueError, 'outside'):
            cm.skin_joints(*part(last_index=2))

    def test_unconsumed_block_rejected(self):
        with self.assertRaisesRegex(ValueError, 'unconsumed'):
            cm.parse_model(model_bytes(extra_block=True))

    def test_vertex_flags_match_shipped_strides(self):
        def material(m0, m1, m2, m3, b10):
            m = bytearray(0x60)
            m[0:4] = bytes((m0, m1, m2, m3))
            m[10] = b10
            return m
        cases = (((0, 0, 0, 0, 0x00), 16), ((0, 0, 1, 0, 0x00), 20), ((0, 0, 0, 0, 0x09), 20),
                 ((0, 0, 1, 0, 0x09), 24), ((0, 0, 2, 1, 0x51), 24), ((0, 0, 2, 1, 0x11), 20),
                 ((0, 1, 1, 0, 0x12), 28), ((1, 0, 2, 1, 0x51), 32), ((0, 0, 0, 6, 0x11), 24))
        for args, stride in cases:
            self.assertEqual(cm.vertex_layout(cm.vertex_flags(material(*args)))[1], stride, args)
        with self.assertRaisesRegex(ValueError, 'flag 1'):
            cm.vertex_layout(cm.vertex_flags(material(2, 0, 0, 0, 0)))


if __name__ == '__main__':
    unittest.main()
