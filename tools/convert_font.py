#!/usr/bin/env python3
"""Draw text with a TT Fusion bitmap font (.tnf) into a PNG (verification aid).

The glyph layout comes from the C++ asset runtime (chase::assets::layoutText) through
tools/chase_native.py; build the project first. The font atlas <name>.btga must sit next
to the .tnf file. The format is described in docs/re/text-localisation-fonts.md.
"""
import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import chase_native as native  # noqa: E402
import convert_textures  # noqa: E402


def draw(font_data, atlas_width, atlas_rgba, text, extended=''):
    """Return (width, height, RGBA bytes) with `text` drawn left to right, one pixel between glyphs."""
    line_height, placed = native.layout_text(font_data, text, extended)
    width = sum(w + 1 for _, _, _, w, _ in placed) + 1
    canvas = bytearray(width * line_height * 4)
    for x, glyph_x, glyph_y, glyph_width, glyph_height in placed:
        if glyph_x + glyph_width > atlas_width or (glyph_y + glyph_height) * atlas_width * 4 > len(atlas_rgba):
            raise ValueError("glyph outside the font atlas")
        for y in range(min(glyph_height, line_height)):
            source = ((glyph_y + y) * atlas_width + glyph_x) * 4
            target = (y * width + x) * 4
            canvas[target:target + glyph_width * 4] = atlas_rgba[source:source + glyph_width * 4]
    return width, line_height, bytes(canvas)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('font', type=Path, help='.tnf file; the .btga atlas must sit next to it')
    parser.add_argument('text')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--extended', type=Path, help='extended_chars_utf8.txt')
    args = parser.parse_args()
    try:
        info, atlas = native.decode_texture(args.font.with_suffix('.btga').read_bytes())
        extended = args.extended.read_text(encoding='utf-8').strip() if args.extended else ''
        width, height, rgba = draw(args.font.read_bytes(), info['width'], atlas, args.text, extended)
    except (ValueError, OSError, native.NativeUnavailable) as exc:
        parser.exit(1, str(exc) + '\n')
    args.output.write_bytes(convert_textures.png(width, height, rgba))
    return 0


if __name__ == '__main__':
    sys.exit(main())
