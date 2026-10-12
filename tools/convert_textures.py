#!/usr/bin/env python3
"""Decode TT Fusion 3DS textures (.btga/.bpng/.bdds) from an unpacked .fib to PNG.

Container: a TT pooled resource (tools/tt_resource.py) with table
[-14, 0x20, -14, data size], i.e. a fixed 0x18-byte prefix
  0x00 u32 0x400 (4 table words), 0x04 u32 0x10, 0x08 table ...,
then the 0x20-byte texture record and the pixel block:
  0x18 u16 width, 0x1a u16 height, 0x1c u32 data size,
  0x20 u8 PICA200 texture format, 0x21-0x22 zero, 0x23 u8 cubemap flag,
  0x24 u32 mip levels, 0x28..0x37 zero.
Pixel data follows at 0x38: every mip level, largest first, in PICA200
8x8 Morton-tiled order. Rows are stored bottom-up (OpenGL convention).
Cubemaps hold six such chains; they are written as a vertical strip in
stored order (the face order is not yet recovered).
"""
import argparse
import json
import struct
import sys
import zlib
from pathlib import Path

HEADER = 0x38
MAGIC = (0x400, 0x10, -14, 0x20, -14)
# format -> (name, bits per pixel)
FORMATS = {0: ('RGBA8', 32), 1: ('RGB8', 24), 2: ('RGBA5551', 16), 3: ('RGB565', 16),
           4: ('RGBA4', 16), 5: ('LA8', 16), 6: ('HILO8', 16), 7: ('L8', 8), 8: ('A8', 8),
           9: ('LA4', 8), 10: ('L4', 4), 11: ('A4', 4), 12: ('ETC1', 4), 13: ('ETC1A4', 8)}
ETC1_TABLE = ((2, 8), (5, 17), (9, 29), (13, 42), (18, 60), (24, 80), (33, 106), (47, 183))
MORTON = [((i & 1) | (i >> 1 & 2) | (i >> 2 & 4), (i >> 1 & 1) | (i >> 2 & 2) | (i >> 3 & 4))
          for i in range(64)]


def level_size(width, height, fmt):
    return width * height * FORMATS[fmt][1] // 8


def parse_header(data):
    if len(data) < HEADER:
        raise ValueError("truncated texture header")
    fields = struct.unpack_from('<IIiIiIHHIBBBBI', data, 0)
    if fields[:5] != MAGIC:
        raise ValueError("unknown texture header")
    size, width, height, size2, fmt, pad1, pad2, cube, levels = fields[5:]
    if fmt not in FORMATS or pad1 or pad2 or cube > 1:
        raise ValueError("unknown PICA texture format bytes %s" % data[0x20:0x24].hex())
    faces = 6 if cube else 1
    if size != size2 or HEADER + size != len(data):
        raise ValueError("texture size fields disagree with file length")
    if not width or not height or width % 8 or height % 8 or not 1 <= levels <= 16:
        raise ValueError("invalid texture dimensions or mip count")
    expected, w, h = 0, width, height
    for _ in range(levels):
        if w < 8 or h < 8:
            raise ValueError("mip level smaller than one tile")
        expected += level_size(w, h, fmt)
        w, h = w // 2, h // 2
    if expected * faces != size:
        raise ValueError("mip chain size %d x %d faces != data size %d" % (expected, faces, size))
    return {'width': width, 'height': height, 'format': fmt, 'faces': faces,
            'format_name': FORMATS[fmt][0], 'levels': levels, 'data_size': size,
            'chain_size': expected}


def expand(value, bits):
    return (value << (8 - bits)) | (value >> (2 * bits - 8)) if bits >= 4 else value * 255 // ((1 << bits) - 1)


def texel(fmt, data, i):
    """RGBA tuple for linear-format texel i (index within the tiled stream)."""
    if fmt == 0:
        a, b, g, r = data[i * 4:i * 4 + 4]
        return r, g, b, a
    if fmt == 1:
        b, g, r = data[i * 3:i * 3 + 3]
        return r, g, b, 255
    if fmt in (2, 3, 4):
        v = data[i * 2] | data[i * 2 + 1] << 8
        if fmt == 2:
            return expand(v >> 11, 5), expand(v >> 6 & 31, 5), expand(v >> 1 & 31, 5), 255 * (v & 1)
        if fmt == 3:
            return expand(v >> 11, 5), expand(v >> 5 & 63, 6), expand(v & 31, 5), 255
        return tuple((v >> s & 15) * 17 for s in (12, 8, 4, 0))
    if fmt == 5:
        a, l = data[i * 2:i * 2 + 2]
        return l, l, l, a
    if fmt == 6:
        g, r = data[i * 2:i * 2 + 2]
        return r, g, 0, 255
    if fmt == 7:
        return data[i], data[i], data[i], 255
    if fmt == 8:
        return 0, 0, 0, data[i]
    if fmt == 9:
        l, a = (data[i] >> 4) * 17, (data[i] & 15) * 17
        return l, l, l, a
    nibble = (data[i // 2] >> (4 * (i & 1)) & 15) * 17
    return (nibble, nibble, nibble, 255) if fmt == 10 else (0, 0, 0, nibble)


def etc1_block(word, alpha):
    """Decode one 4x4 ETC1 block (u64 little-endian) into {(x, y): rgba}."""
    high, low = word >> 32, word & 0xffffffff
    if high & 2:
        base = []
        for shift in (27, 19, 11):
            c = high >> shift & 31
            d = high >> (shift - 3) & 7
            base.append((c, c + (d - 8 if d & 4 else d)))
        colors = [tuple(expand(base[ch][sub] & 31, 5) for ch in range(3)) for sub in (0, 1)]
    else:
        colors = [tuple((high >> s & 15) * 17 for s in (28, 20, 12)),
                  tuple((high >> s & 15) * 17 for s in (24, 16, 8))]
    tables = (ETC1_TABLE[high >> 5 & 7], ETC1_TABLE[high >> 2 & 7])
    flip = high & 1
    out = {}
    for x in range(4):
        for y in range(4):
            i = x * 4 + y
            sub = (y if flip else x) >= 2
            idx = (low >> (i + 16) & 1) << 1 | (low >> i & 1)
            mod = tables[sub][idx & 1] * (-1 if idx & 2 else 1)
            rgb = tuple(min(255, max(0, c + mod)) for c in colors[sub])
            a = (alpha >> (4 * i) & 15) * 17 if alpha is not None else 255
            out[x, y] = rgb + (a,)
    return out


def decode_level(data, width, height, fmt):
    """Return bottom-up-corrected RGBA rows for one mip level."""
    pixels = [[(0, 0, 0, 0)] * width for _ in range(height)]
    tiles_x = width // 8
    if fmt in (12, 13):
        step = 16 if fmt == 13 else 8
        offset = 0
        for tile in range(tiles_x * (height // 8)):
            tx, ty = tile % tiles_x * 8, tile // tiles_x * 8
            for bx, by in ((0, 0), (4, 0), (0, 4), (4, 4)):
                alpha = struct.unpack_from('<Q', data, offset)[0] if fmt == 13 else None
                word = struct.unpack_from('<Q', data, offset + step - 8)[0]
                offset += step
                for (x, y), rgba in etc1_block(word, alpha).items():
                    pixels[ty + by + y][tx + bx + x] = rgba
    else:
        for tile in range(tiles_x * (height // 8)):
            tx, ty = tile % tiles_x * 8, tile // tiles_x * 8
            for j, (x, y) in enumerate(MORTON):
                pixels[ty + y][tx + x] = texel(fmt, data, tile * 64 + j)
    pixels.reverse()
    return pixels


def decode(data):
    info = parse_header(data)
    size = level_size(info['width'], info['height'], info['format'])
    rows = []
    for face in range(info['faces']):
        start = HEADER + face * info['chain_size']
        rows += decode_level(data[start:start + size], info['width'], info['height'], info['format'])
    return info, rows


def png(rows):
    width, height = len(rows[0]), len(rows)
    raw = b''.join(b'\0' + bytes(c for px in row for c in px) for row in rows)

    def chunk(kind, body):
        return struct.pack('>I', len(body)) + kind + body + struct.pack('>I', zlib.crc32(kind + body))
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0))
            + chunk(b'IDAT', zlib.compress(raw, 6)) + chunk(b'IEND', b''))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('inputs', type=Path, nargs='+', help='texture files or directories to scan')
    parser.add_argument('--output', type=Path, help='PNG output directory (must not exist)')
    parser.add_argument('--census', action='store_true', help='only validate headers and print a format summary')
    parser.add_argument('--checksum', action='store_true',
                        help='print FNV-1a (32-bit) of each decoded RGBA image, for cross-checking the C++ decoder')
    args = parser.parse_args()
    if args.checksum:
        for path in args.inputs:
            info, rows = decode(path.read_bytes())
            value = 0x811c9dc5
            for byte in (c for row in rows for px in row for c in px):
                value = ((value ^ byte) * 0x01000193) & 0xffffffff
            print('%08x %dx%d %s' % (value, len(rows[0]), len(rows), path))
        return 0
    if not args.census and not args.output:
        parser.error('--output is required unless --census is given')
    files = []
    for item in args.inputs:
        files += sorted(p for p in item.rglob('*') if p.suffix in ('.btga', '.bpng', '.bdds')) if item.is_dir() else [item]
    if args.output:
        if args.output.exists():
            parser.exit(1, 'output already exists; choose a new directory\n')
        args.output.mkdir(parents=True)
    census, failures = {}, []
    roots = [i for i in args.inputs if i.is_dir()]
    for path in files:
        try:
            data = path.read_bytes()
            if args.census:
                info = parse_header(data)
            else:
                info, rows = decode(data)
                base = next((r for r in roots if r in path.parents), path.parent)
                target = args.output / path.relative_to(base).with_suffix(path.suffix + '.png')
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(png(rows))
        except (ValueError, struct.error, OSError) as exc:
            failures.append({'path': str(path), 'error': str(exc)})
            continue
        key = '%s %s%s' % (path.suffix, info['format_name'], ' cube' if info['faces'] == 6 else '')
        census[key] = census.get(key, 0) + 1
    print(json.dumps({'files': len(files), 'formats': dict(sorted(census.items())),
                      'failures': failures[:20], 'failure_count': len(failures)}, indent=1))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
