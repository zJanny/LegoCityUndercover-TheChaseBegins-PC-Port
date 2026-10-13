import struct
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
sys.path.insert(0, str(Path(__file__).parents[1] / 'tools'))
import chase_native as native  # noqa: E402
from fixtures import read_png, rgba8_texture, run_tool  # noqa: E402


def font_file(end=0x23, overrides=None):
    """Codes 0x20..end-1, each glyph w=1+i, h=2 at x=3*i, y=0, unless overridden by code."""
    records = b''.join(struct.pack('<4H', *(overrides or {}).get(0x20 + i, (1 + i, 2, 3 * i, 0))) for i in range(end - 0x20))
    return struct.pack('<HBB', end, 0x20, 2) + records


def draw(directory, font, text, extended=None):
    (Path(directory) / 'f.tnf').write_bytes(font)
    (Path(directory) / 'f.btga').write_bytes(rgba8_texture(16, 8, lambda x, y: (x, y, 0, 255)))
    args = [Path(directory) / 'f.tnf', text, '--output', Path(directory) / 'out.png']
    if extended is not None:
        (Path(directory) / 'extended.txt').write_text(extended, encoding='utf-8')
        args += ['--extended', Path(directory) / 'extended.txt']
    result = run_tool('convert_font.py', *args)
    return result, read_png(Path(directory) / 'out.png') if result.returncode == 0 else None


@unittest.skipUnless(native.available(), 'C++ asset library not built')
class FontToolTests(unittest.TestCase):
    # Expected pixels are derived by hand from the glyph records and the atlas gradient (red = x, green = y).
    def test_draw_places_glyphs_with_one_pixel_gap(self):
        with tempfile.TemporaryDirectory() as d:
            result, (width, height, rows) = draw(d, font_file(), '!"')
        self.assertEqual(result.returncode, 0)
        self.assertEqual((width, height), (2 + 1 + 3 + 1 + 1, 2))
        self.assertEqual(rows[1][3], (6, 1, 0, 255))   # '"' starts after '!' and one gap
        self.assertEqual(rows[0][0], (3, 0, 0, 255))   # '!' is atlas x=3
        self.assertEqual(rows[0][2], (0, 0, 0, 0))     # the gap

    def test_extended_characters_follow_the_list_order(self):
        font = font_file(0x102, {0x101: (1, 1, 5, 0)})
        with tempfile.TemporaryDirectory() as d:
            _, (width, height, rows) = draw(d, font, 'б', 'аб\n')
        self.assertEqual((width, height, rows[0][0]), (3, 2, (5, 0, 0, 255)))

    def test_rejects_size_mismatch(self):
        with tempfile.TemporaryDirectory() as d:
            result, _ = draw(d, font_file() + b'x', '!')
        self.assertEqual(result.returncode, 1)
        self.assertIn('code range', result.stderr)


if __name__ == '__main__':
    unittest.main()
