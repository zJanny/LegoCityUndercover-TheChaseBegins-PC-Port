import importlib.util
import struct
import sys
import unittest
from pathlib import Path

TOOLS = Path(__file__).parents[1] / 'tools'
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location('convert_anims', TOOLS / 'convert_anims.py')
ca = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ca)
from test_convert_models import pooled  # noqa: E402


def skeleton(extra=False):
    head = bytes([0, 2]) + bytes(14)
    blocks = [(-14, head), (-14, b'b8'), (-14, b'bc'), (-14, bytes(2 * 0x98)), (-14, b'root\0'), (-14, b'spine\0')]
    return pooled(blocks + ([(-14, b'x')] if extra else []))


def animation():
    """One sub-animation with one track of two keys, like csdata *_0000.bfnanm."""
    head = bytearray(0x28)
    struct.pack_into('<I', head, 0x20, 1)
    track = struct.pack('<HHHH4I', 0, 0, 0, 0, 0, 0, 0, 0)[:0x18]
    blocks = [(-14, bytes(head)), (-14, bytes(0x10)),                  # header, +0x1c array
              (-14, struct.pack('<I', 0)), (-14, struct.pack('<I', 1)),  # fixups (none), track count
              (-14, track), (-14, b'\xe0\x33\xd6\xdc'), (-14, b''),     # tracks, name hash, empty key array
              (-14, struct.pack('<2f', 0, 1)), (-14, bytes(40)),         # key times, key values
              (-14, b'')]                                               # sub-animation data block
    return pooled(blocks)


class AnimTests(unittest.TestCase):
    def test_skeleton(self):
        skel = ca.parse_skeleton(skeleton())
        self.assertEqual([j['name'] for j in skel['joints']], ['root', 'spine'])
        with self.assertRaisesRegex(ValueError, 'unconsumed'):
            ca.parse_skeleton(skeleton(extra=True))

    def test_animation_subtracks(self):
        anim = ca.parse_animation(animation())
        self.assertEqual(len(anim['sub']), 1)
        track = anim['sub'][0][0]
        self.assertEqual(track['first'], b'\xe0\x33\xd6\xdc')
        self.assertEqual(struct.unpack('<2f', track['b10']), (0.0, 1.0))
        self.assertEqual(len(track['b14']), 40)


if __name__ == '__main__':
    unittest.main()
