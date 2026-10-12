import importlib.util
import struct
import sys
import unittest
import zlib
from pathlib import Path

TOOLS = Path(__file__).parents[1] / 'tools'
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location('convert_levels', TOOLS / 'convert_levels.py')
cl = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cl)
from test_convert_models import pooled  # noqa: E402


def param(name, kind, inline=0):
    return struct.pack('<IHHI', zlib.crc32(name.encode()), 2, kind, inline)


def level_bytes(extra=False):
    """One object (s2c) with position, orientation and meshname parameters."""
    head = bytearray(0x38)
    struct.pack_into('<H', head, 14, 1)                   # one object in section +0x2c
    obj = bytearray(0x18)
    obj[0xd] = 3                                          # three parameters in the first list
    params = param('initialposition', 2, 1) + param('initialorientation', 2, 1) + param('meshname', 1, 1)
    blocks = [(-15, bytes(head))] + [(-15, b'')] * 7 + [   # s10 s14 s18 s1c s20 s24 s28
        (-15, bytes(obj)), (-15, b'Bench01\0'), (-15, b'room1\0\0\0'), (-15, params),
        (-15, struct.pack('<3f', 1, 2, 3)), (-15, struct.pack('<3f', 0, 90, 0)), (-15, b'Prop_Bench\0\0'),
        (-15, b''), (-15, b''),                           # second parameter list, s34
        (-16, b'leprops/leprop_test\0')]
    if extra:
        blocks.append((-14, b'xx'))
    return pooled(blocks)


class LevelTests(unittest.TestCase):
    def test_hash_names(self):
        self.assertEqual(zlib.crc32(b'meshname'), cl.MESH_NAME)
        self.assertEqual(zlib.crc32(b'initialposition'), cl.INITIAL_POSITION)
        self.assertEqual(zlib.crc32(b'initialorientation'), cl.INITIAL_ORIENTATION)

    def test_parse_and_placements(self):
        level = cl.parse_level(level_bytes())
        self.assertEqual(level['s2c'][0]['name'], 'leprops/leprop_test')
        place = cl.placements(level)[0]
        self.assertEqual((place['instance'], place['room'], place['mesh']), ('Bench01', 'room1', 'Prop_Bench'))
        self.assertEqual(place['position'], [1.0, 2.0, 3.0])
        self.assertEqual(place['orientation_deg'], [0.0, 90.0, 0.0])

    def test_unconsumed_and_model_path(self):
        with self.assertRaisesRegex(ValueError, 'unconsumed'):
            cl.parse_level(level_bytes(extra=True))
        self.assertEqual(cl.model_path('Prop_Plant_01'), 'models/prop_plant_01/prop_plant_01.bfnmdl')
        self.assertEqual(cl.model_path('A/B_atlas'), 'models/a/b_atlas.bfnmdl')


if __name__ == '__main__':
    unittest.main()
