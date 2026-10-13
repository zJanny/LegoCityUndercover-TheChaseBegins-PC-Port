import struct
import sys
import tempfile
import unittest
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
sys.path.insert(0, str(Path(__file__).parents[1] / 'tools'))
import chase_native as native  # noqa: E402
from fixtures import pooled, run_tool  # noqa: E402


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


@unittest.skipUnless(native.available(), 'C++ asset library not built')
class LevelToolTests(unittest.TestCase):
    def dump(self, data):
        with tempfile.TemporaryDirectory() as d:
            (Path(d) / 'a.blvl').write_bytes(data)
            (Path(d) / 'code.bin').write_bytes(b'\0initialposition\0meshname\0')
            return run_tool('convert_levels.py', Path(d) / 'a.blvl', '--param-names', Path(d) / 'code.bin')

    def test_objects_with_named_parameters(self):
        result = self.dump(level_bytes())
        self.assertEqual(result.returncode, 0)
        self.assertEqual(result.stdout.splitlines(), [
            'leprops/leprop_test Bench01 room1',
            '   initialposition          type=2 [1.0, 2.0, 3.0]',
            '   a3b9aa7f                 type=2 [0.0, 90.0, 0.0]',
            '   meshname                 type=1 Prop_Bench'])

    def test_unconsumed_block_is_rejected(self):
        result = self.dump(level_bytes(extra=True))
        self.assertEqual(result.returncode, 1)
        self.assertIn('unconsumed', result.stderr)


if __name__ == '__main__':
    unittest.main()
