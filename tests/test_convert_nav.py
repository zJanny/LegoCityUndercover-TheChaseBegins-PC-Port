import importlib.util
import struct
import sys
import unittest
from pathlib import Path

TOOLS = Path(__file__).parents[1] / 'tools'
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location('convert_nav', TOOLS / 'convert_nav.py')
cn = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cn)
from test_convert_models import pooled  # noqa: E402


def grid(x, height, z, width, depth, links=(), extra=0):
    raw = bytearray(0x3c)
    struct.pack_into('<6H', raw, 0, x, height, z, width, 0, depth)
    struct.pack_into('<16H', raw, 0x14, *(list(links) + [0xffff] * (16 - len(links))))
    struct.pack_into('<H', raw, 0x34, extra)
    return bytes(raw)


def nav_file(extra_block=False, cells=6):
    """Two grids (3x2 with one extra entry, 1x1) and one path record with three indices."""
    path = struct.pack('<6fI4x', 1.0, 2.0, 3.0, 0.5, 0.5, 0.5, 3)
    blocks = [(-14, grid(10, 512, 20, 3, 2, links=[1], extra=1) + grid(13, 0, 20, 1, 1)),
              (-14, bytes([1, 0, 0, 4, 12, 1]) + bytes(cells - 6) + bytes(2)), (-14, b'\x01\x02\x03\x04'),
              (-14, b'\x01\x00\x00\x00'),
              (-14, path), (-14, struct.pack('<3H2x', 0, 1, 1))]
    if extra_block:
        blocks.append((-14, b'xx'))
    return pooled(blocks)


class NavTests(unittest.TestCase):
    def test_parse(self):
        nav = cn.parse_nav(nav_file())
        first = nav['grids'][0]
        self.assertEqual((first['x'], first['z'], first['width'], first['depth']), (10, 20, 3, 2))
        self.assertEqual(first['links'], [1])
        self.assertEqual(first['cells'], bytes([1, 0, 0, 4, 12, 1]))
        self.assertEqual(first['extra'], b'\x01\x02\x03\x04')
        self.assertEqual(nav['paths'][0]['position'], (1.0, 2.0, 3.0))
        self.assertEqual(nav['paths'][0]['indices'], [0, 1, 1])

    def test_rejects_bad_layouts(self):
        with self.assertRaisesRegex(ValueError, 'unconsumed'):
            cn.parse_nav(nav_file(extra_block=True))
        with self.assertRaisesRegex(ValueError, 'cell bytes'):
            cn.parse_nav(nav_file(cells=10))

    def test_cell_world(self):
        room = {'origin': (-10.0, -2.0, 5.0), 'cell': 0.5, 'height_unit': 0.25}
        world = cn.cell_world(room, cn.parse_nav(nav_file())['grids'][0], 1, 1)
        self.assertEqual(world, (-10.0 + 0.5 * 11.5, -2.0 + 512 * 0.25 / 256, 5.0 + 0.5 * 21.5))


if __name__ == '__main__':
    unittest.main()
