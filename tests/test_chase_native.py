import struct
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
sys.path.insert(0, str(Path(__file__).parents[1] / 'tools'))
import chase_native as native  # noqa: E402
from fixtures import pooled  # noqa: E402


def joint(parent, world_t, local_t):
    def matrix(t):
        rows = [[1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [t[0], t[1], t[2], 1]]
        return b''.join(struct.pack('<4f', *r) for r in rows)
    return struct.pack('<Ii', 0, parent) + matrix(world_t) + matrix(local_t) + bytes(0x10)


def bitstream(fields):
    """[(value, bits)] packed LSB-first like 0x247a48 reads them."""
    value = shift = 0
    for v, n in fields:
        value |= (v & ((1 << n) - 1)) << shift
        shift += n
    return value.to_bytes((shift + 7) // 8 + 4, 'little')


@unittest.skipUnless(native.available(), 'C++ asset library not built')
class NativeTests(unittest.TestCase):
    def test_skin_matrices(self):
        # Translation x channel 9: base 150, delta width 3, deltas [0, 5] (the C++ assets-test curve).
        curve = bitstream([(0x08, 8), (1, 3), (150, 16), (3, 5), (0, 3), (5, 3)])
        skel = pooled([(-14, bytes([0, 1]) + bytes(14)), (-14, b''), (-14, b''),
                       (-14, joint(-1, (0, 0, 0), (0, 0, 0))), (-14, b'root\0')])
        head = bytearray(0x28)
        head[4], head[8] = 2, 1
        anim = pooled([(-14, bytes(head)), (-14, struct.pack('<I', 1)), (-14, b'p'), (-14, curve)])
        skin = native.skin_matrices(skel, anim, 1)
        self.assertEqual(len(skin), 1)
        self.assertAlmostEqual(skin[0][3][0], 1.55, places=5)
        with self.assertRaises(ValueError):
            native.skin_matrices(skel, b'', 0)

    def test_decoders_report_errors_as_value_errors(self):
        for decode in (native.decode_texture, native.decode_sound, native.decode_localisation, native.level_objects):
            with self.assertRaises(ValueError):
                decode(b'not a game file')
        with self.assertRaises(ValueError):
            native.layout_text(b'xx', 'text')
        with tempfile.TemporaryDirectory() as d:
            (Path(d) / 'a.fib').write_bytes(b'not an archive')
            with self.assertRaises(ValueError):
                native.FuseArchive(Path(d) / 'a.fib')
            with self.assertRaises(ValueError):
                native.FuseArchive(Path(d) / 'missing.fib')


if __name__ == '__main__':
    unittest.main()
