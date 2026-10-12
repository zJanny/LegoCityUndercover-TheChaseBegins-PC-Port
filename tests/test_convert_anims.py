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


class AnimTests(unittest.TestCase):
    def test_channel_decode(self):
        # head 0x08: only group 3 (channels 9-11) has a mask; mask 0b001 -> channel 9.
        # Channel 9: 16-bit signed base 150, 5-bit delta width 3, deltas [0, 5] for 2 frames.
        blob = bitstream([(0x08, 8), (1, 3), (150, 16), (3, 5), (0, 3), (5, 3)])
        frame0, mask = ca.decode_channels(blob, 0, 2)
        frame1, _ = ca.decode_channels(blob, 1, 2)
        self.assertEqual(mask, 1 << 9)
        self.assertAlmostEqual(frame0[9], 1.50)
        self.assertAlmostEqual(frame1[9], 1.55)
        self.assertEqual(frame0[3:6], [1.0, 1.0, 1.0])   # absent scale channels default to 1
        negative = bitstream([(0x01, 8), (1, 3), ((1 << 12) - 100, 12), (0, 4)])   # rotation x = -100
        self.assertAlmostEqual(ca.decode_channels(negative, 0, 1)[0][0], -2.0)
        self.assertEqual(ca.euler_to_quaternion(0, 0, 0), (0.0, 0.0, -0.0, 1.0))

    def test_hierarchy_check(self):
        good = joint(-1, (0, 1, 0), (0, 1, 0)) + joint(0, (0, 1.5, 2), (0, 0.5, 2))
        blocks = [(-14, bytes([0, 2]) + bytes(14)), (-14, b''), (-14, b''), (-14, good), (-14, b'a\0'), (-14, b'b\0')]
        skel = ca.parse_skeleton(pooled(blocks))
        self.assertEqual(skel['joints'][1]['parent'], 0)
        self.assertLess(ca.check_hierarchy(skel), 1e-6)
        bad = joint(-1, (0, 1, 0), (0, 1, 0)) + joint(0, (0, 9, 2), (0, 0.5, 2))
        blocks[3] = (-14, bad)
        with self.assertRaisesRegex(ValueError, 'parent.world'):
            ca.check_hierarchy(ca.parse_skeleton(pooled(blocks)))

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
