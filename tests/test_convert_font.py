import importlib.util
import struct
import sys
import unittest
from pathlib import Path

TOOLS = Path(__file__).parents[1] / 'tools'
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location('convert_font', TOOLS / 'convert_font.py')
cf = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cf)


def font_file(end=0x23):
    """Codes 0x20..end-1, each glyph w=1+i, h=2 at x=3*i, y=0."""
    records = b''.join(struct.pack('<4H', 1 + i, 2, 3 * i, 0) for i in range(end - 0x20))
    return struct.pack('<HBB', end, 0x20, 2) + records


class FontTests(unittest.TestCase):
    def test_parse_and_draw(self):
        font = cf.parse_font(font_file())
        self.assertEqual(font['glyphs'][0x22], (3, 2, 6, 0))
        atlas = [[(x, y, 0, 255) for x in range(9)] for y in range(2)]
        rows = cf.draw(font, atlas, '!"')
        self.assertEqual(len(rows[0]), 2 + 1 + 3 + 1 + 1)
        self.assertEqual(rows[1][3], (6, 1, 0, 255))     # '"' starts after '!' and one gap

    def test_extended_codes(self):
        self.assertEqual(cf.glyph_code('б', 'аб'), 0x101)
        self.assertEqual(cf.glyph_code('A', 'аб'), 0x41)

    def test_rejects_size_mismatch(self):
        with self.assertRaisesRegex(ValueError, 'code range'):
            cf.parse_font(font_file() + b'x')


if __name__ == '__main__':
    unittest.main()
