import struct
import sys
import tempfile
import unittest
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
sys.path.insert(0, str(Path(__file__).parents[1] / 'tools'))
import chase_native as native  # noqa: E402
import convert_textures  # noqa: E402
from fixtures import read_png, run_json_tool, texture  # noqa: E402


def etc1(high, low=0):
    return struct.pack('<Q', high << 32 | low)


def convert(directory, name, data):
    source = Path(directory) / 'in'
    source.mkdir(exist_ok=True)
    (source / name).write_bytes(data)
    result, _ = run_json_tool('convert_textures.py', source, '--output', Path(directory) / 'out')
    return result, read_png(Path(directory) / 'out' / (name + '.png'))


@unittest.skipUnless(native.available(), 'C++ asset library not built')
class TextureToolTests(unittest.TestCase):
    # Expected pixels are derived by hand from the PICA200 layout; tests/assets_test.cpp pins the same values.
    def test_morton_tiling_and_bottom_up_rows(self):
        payload = b''.join(bytes((255, 0, 0, i)) for i in range(64))   # RGBA8 is stored A,B,G,R
        with tempfile.TemporaryDirectory() as d:
            result, (width, height, rows) = convert(d, 'a.btga', texture(8, 8, 0, 1, payload))
        self.assertEqual(result.returncode, 0)
        self.assertEqual((width, height), (8, 8))
        self.assertEqual(rows[7][0], (0, 0, 0, 255))
        self.assertEqual(rows[7][1][0], 1)    # x=1,y=0 is texel 1
        self.assertEqual(rows[6][0][0], 2)    # x=0,y=1 is texel 2
        self.assertEqual(rows[7][2][0], 4)    # x=2,y=0 is texel 4
        self.assertEqual(rows[0][7][0], 63)   # top-right of the image is the last texel

    def test_etc1_individual_and_differential(self):
        individual = 8 << 28 | 15 << 24 | 8 << 20 | 15 << 16 | 8 << 12 | 15 << 8
        differential = 16 << 27 | 7 << 24 | 16 << 19 | 7 << 16 | 16 << 11 | 7 << 8 | 2
        with tempfile.TemporaryDirectory() as d:
            _, (_, _, rows) = convert(d, 'a.btga', texture(8, 8, 12, 1, etc1(individual, 1 << 6 | 1 << 22) + etc1(0) * 3))
            self.assertEqual(rows[7][0], (138, 138, 138, 255))   # 136 + 2
            self.assertEqual(rows[4][3], (255, 255, 255, 255))   # 255 + 2 clamps
            self.assertEqual(rows[5][1], (128, 128, 128, 255))   # index 3: 136 - 8
        with tempfile.TemporaryDirectory() as d:
            _, (_, _, rows) = convert(d, 'a.btga', texture(8, 8, 12, 1, etc1(differential) + etc1(0) * 3))
            self.assertEqual(rows[7][0][0], 134)   # expand5(16) = 132, + 2
            self.assertEqual(rows[7][2][0], 125)   # expand5(15) = 123, + 2

    def test_etc1a4_alpha_order_and_tile_blocks(self):
        flip_top = 8 << 28 | 8 << 24 | 8 << 20 | 8 << 16 | 8 << 12 | 8 << 8 | 1
        alpha = struct.pack('<Q', 0xF << (4 * 1))   # pixel x=0,y=1 opaque
        payload = (alpha + etc1(flip_top)) + (bytes(8) + etc1(0)) * 3
        with tempfile.TemporaryDirectory() as d:
            _, (_, _, rows) = convert(d, 'a.btga', texture(8, 8, 13, 1, payload))
        self.assertEqual(rows[6][0][3], 255)   # stored y=1 shows as row 6
        self.assertEqual(rows[7][0][3], 0)
        self.assertEqual(rows[6][0][0], 138)
        self.assertEqual(rows[6][4][0], 2)     # second block (x+4) has black base

    def test_cube_strip_and_census(self):
        with tempfile.TemporaryDirectory() as d:
            _, (width, height, _) = convert(d, 'cube.btga', texture(8, 8, 7, 1, bytes(range(64)) * 6, cube=1))
            self.assertEqual((width, height), (8, 48))
            result, summary = run_json_tool('convert_textures.py', '--census', Path(d) / 'in')
        self.assertEqual(summary['formats'], {'.btga L8 cube': 1})
        self.assertEqual(result.returncode, 0)

    def test_failures_are_reported(self):
        good = texture(8, 8, 7, 1, bytes(64))
        cases = {'short.btga': good[:20], 'magic.btga': b'\0\0' + good[2:], 'format.btga': good[:0x20] + b'\x0e' + good[0x21:],
                 'size.btga': good + b'!', 'width.btga': texture(12, 8, 7, 1, bytes(96)),
                 'mips.btga': texture(8, 8, 0, 2, bytes(8 * 8 * 4 * 2))}
        with tempfile.TemporaryDirectory() as d:
            source = Path(d) / 'in'
            source.mkdir()
            for name, data in cases.items():
                (source / name).write_bytes(data)
            result, summary = run_json_tool('convert_textures.py', '--census', source)
        self.assertEqual(result.returncode, 1)
        self.assertEqual(summary['failure_count'], len(cases))

    def test_existing_output_is_refused(self):
        with tempfile.TemporaryDirectory() as d:
            result = run_json_tool('convert_textures.py', Path(d), '--output', d)[0]
        self.assertEqual(result.returncode, 1)
        self.assertIn('already exists', result.stderr)

    def test_png_roundtrip(self):
        data = convert_textures.png(2, 1, bytes((1, 2, 3, 4, 5, 6, 7, 8)))
        self.assertEqual(data[:8], b'\x89PNG\r\n\x1a\n')
        idat = data.index(b'IDAT')
        length = struct.unpack('>I', data[idat - 4:idat])[0]
        self.assertEqual(zlib.decompress(data[idat + 4:idat + 4 + length]), b'\0\1\2\3\4\5\6\7\x08')


if __name__ == '__main__':
    unittest.main()
