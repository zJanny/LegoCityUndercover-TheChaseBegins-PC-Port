#!/usr/bin/env python3
"""Read TT Fusion bitmap fonts (.tnf) and draw text with them (verification aid).

Layout (from shipped data): u16 end code, u8 first code (0x20), u8 line height, then
one (u16 width, u16 height, u16 x, u16 y) record per code from first to end - 1,
addressing the font atlas <name>.btga. Codes 0x100 and up hold the characters of
localisation/extended_chars_utf8.txt in file order (text escape 0x01 '2' c is code
0x100 + c - 0x30); Russian text drawn this way reads correctly.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import convert_textures  # noqa: E402

EXTENDED_BASE = 0x100


def parse_font(data):
    if len(data) < 4:
        raise ValueError("truncated font header")
    end, first, height = struct.unpack_from('<HBB', data, 0)
    if end <= first or len(data) != 4 + (end - first) * 8:
        raise ValueError("font size does not match its code range")
    glyphs = {first + i: struct.unpack_from('<4H', data, 4 + i * 8) for i in range(end - first)}
    return {'first': first, 'end': end, 'height': height, 'glyphs': glyphs}


def glyph_code(char, extended):
    return EXTENDED_BASE + extended.index(char) if char in extended else ord(char)


def draw(font, atlas_rows, text, extended=''):
    """Return RGBA rows with `text` drawn left to right, one pixel between glyphs."""
    codes = [glyph_code(c, extended) for c in text]
    width = sum(font['glyphs'][c][0] + 1 for c in codes if c in font['glyphs']) + 1
    rows = [[(0, 0, 0, 0)] * width for _ in range(font['height'])]
    x = 0
    for code in codes:
        if code not in font['glyphs']:
            continue
        w, h, gx, gy = font['glyphs'][code]
        for y in range(min(h, font['height'])):
            for i in range(w):
                rows[y][x + i] = atlas_rows[gy + y][gx + i]
        x += w + 1
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('font', type=Path, help='.tnf file; the .btga atlas must sit next to it')
    parser.add_argument('text')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--extended', type=Path, help='extended_chars_utf8.txt')
    args = parser.parse_args()
    font = parse_font(args.font.read_bytes())
    _, atlas = convert_textures.decode(args.font.with_suffix('.btga').read_bytes())
    extended = args.extended.read_text(encoding='utf-8').strip() if args.extended else ''
    args.output.write_bytes(convert_textures.png(draw(font, atlas, args.text, extended)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
