import importlib.util
import struct
import unittest
import zlib
from pathlib import Path

spec = importlib.util.spec_from_file_location('convert_textures', Path(__file__).parents[1] / 'tools/convert_textures.py')
t = importlib.util.module_from_spec(spec)
spec.loader.exec_module(t)


def texture(width, height, fmt, levels, payload, cube=0):
    header = struct.pack('<IIiIiIHHIBBBBI', 0x400, 0x10, -14, 0x20, -14, len(payload), width, height,
                         len(payload), fmt, 0, 0, cube, levels)
    return header + bytes(16) + payload


def etc1(high, low=0):
    return struct.pack('<Q', high << 32 | low)


class TextureTests(unittest.TestCase):
    def test_morton_tiling_and_bottom_up_rows(self):
        # RGBA8 is stored as A,B,G,R; red carries the stream index.
        payload = b''.join(bytes((255, 0, 0, i)) for i in range(64))
        info, rows = t.decode(texture(8, 8, 0, 1, payload))
        self.assertEqual(info['format_name'], 'RGBA8')
        self.assertEqual(rows[7][0], (0, 0, 0, 255))
        self.assertEqual(rows[7][1][0], 1)   # x=1,y=0 is texel 1
        self.assertEqual(rows[6][0][0], 2)   # x=0,y=1 is texel 2
        self.assertEqual(rows[7][2][0], 4)   # x=2,y=0 is texel 4
        self.assertEqual(rows[0][7][0], 63)  # top-right of the image is the last texel

    def test_etc1_individual_and_differential(self):
        high = 8 << 28 | 15 << 24 | 8 << 20 | 15 << 16 | 8 << 12 | 15 << 8
        block = t.etc1_block(high << 32 | 1 << 6 | 1 << 22, None)
        self.assertEqual(block[0, 0], (138, 138, 138, 255))  # 136 + 2
        self.assertEqual(block[3, 3], (255, 255, 255, 255))  # 255 + 2 clamps
        self.assertEqual(block[1, 2], (128, 128, 128, 255))  # index 3: 136 - 8
        diff = 16 << 27 | 7 << 24 | 16 << 19 | 7 << 16 | 16 << 11 | 7 << 8 | 2
        block = t.etc1_block(diff << 32, None)
        self.assertEqual(block[0, 0][0], 134)  # expand5(16) = 132, + 2
        self.assertEqual(block[2, 0][0], 125)  # expand5(15) = 123, + 2

    def test_etc1a4_alpha_order_and_tile_blocks(self):
        flip_top = 8 << 28 | 8 << 24 | 8 << 20 | 8 << 16 | 8 << 12 | 8 << 8 | 1
        alpha = struct.pack('<Q', 0xF << (4 * 1))  # pixel x=0,y=1 opaque
        payload = (alpha + etc1(flip_top)) + (bytes(8) + etc1(0)) * 3
        _, rows = t.decode(texture(8, 8, 13, 1, payload))
        self.assertEqual(rows[6][0][3], 255)  # stored y=1 shows as row 6
        self.assertEqual(rows[7][0][3], 0)
        self.assertEqual(rows[6][0][0], 138)
        self.assertEqual(rows[6][4][0], 2)  # second block (x+4) has black base

    def test_cube_strip_and_mip_validation(self):
        face = bytes(8 * 8 * 4) + bytes(8 * 8 * 4)  # wrong: two levels need 16x16 first
        with self.assertRaisesRegex(ValueError, 'smaller'):
            t.parse_header(texture(8, 8, 0, 2, face))
        info, rows = t.decode(texture(8, 8, 7, 1, bytes(range(64)) * 6, cube=1))
        self.assertEqual((info['faces'], len(rows)), (6, 48))

    def test_header_rejections(self):
        good = texture(8, 8, 7, 1, bytes(64))
        for bad, message in ((good[:20], 'truncated'), (b'\0\0' + good[2:], 'unknown texture'),
                             (good[:0x20] + b'\x0e' + good[0x21:], 'format'),
                             (good + b'!', 'size fields'),
                             (texture(12, 8, 7, 1, bytes(96)), 'dimensions')):
            with self.assertRaisesRegex(ValueError, message):
                t.parse_header(bad)

    def test_png_roundtrip(self):
        data = t.png([[(1, 2, 3, 4), (5, 6, 7, 8)]])
        self.assertEqual(data[:8], b'\x89PNG\r\n\x1a\n')
        idat = data.index(b'IDAT')
        length = struct.unpack('>I', data[idat - 4:idat])[0]
        self.assertEqual(zlib.decompress(data[idat + 4:idat + 4 + length]), b'\0\1\2\3\4\5\6\7\x08')


if __name__ == '__main__':
    unittest.main()
