#!/usr/bin/env python3
"""Decode TT Fusion 3DS textures (.btga/.bpng/.bdds) from an unpacked .fib to PNG.

The textures are decoded by the C++ asset runtime (chase::assets::decodeTexture) through
tools/chase_native.py; build the project first. Only the largest size of each texture is
written. Cubemaps become a vertical strip of six faces in stored order. The format is
described in docs/re/textures.md.
"""
import argparse
import json
import struct
import sys
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import chase_native as native  # noqa: E402

FORMAT_NAMES = ('RGBA8', 'RGB8', 'RGBA5551', 'RGB565', 'RGBA4', 'LA8', 'HILO8', 'L8', 'A8', 'LA4', 'L4', 'A4',
                'ETC1', 'ETC1A4')


def png(width, height, rgba):
    stride = width * 4
    raw = b''.join(b'\0' + rgba[y * stride:(y + 1) * stride] for y in range(height))

    def chunk(kind, body):
        return struct.pack('>I', len(body)) + kind + body + struct.pack('>I', zlib.crc32(kind + body))
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0))
            + chunk(b'IDAT', zlib.compress(raw, 6)) + chunk(b'IEND', b''))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('inputs', type=Path, nargs='+', help='texture files or directories to scan')
    parser.add_argument('--output', type=Path, help='PNG output directory (must not exist)')
    parser.add_argument('--census', action='store_true', help='only decode and print a format summary')
    args = parser.parse_args()
    if not args.census and not args.output:
        parser.error('--output is required unless --census is given')
    try:
        native.library()
    except native.NativeUnavailable as exc:
        parser.exit(1, str(exc) + '\n')
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
            info, rgba = native.decode_texture(path.read_bytes())
            if not args.census:
                base = next((r for r in roots if r in path.parents), path.parent)
                target = args.output / path.relative_to(base).with_suffix(path.suffix + '.png')
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(png(info['width'], info['height'], rgba))
        except (ValueError, OSError) as exc:
            failures.append({'path': str(path), 'error': str(exc)})
            continue
        key = '%s %s%s' % (path.suffix, FORMAT_NAMES[info['format']], ' cube' if info['faces'] == 6 else '')
        census[key] = census.get(key, 0) + 1
    print(json.dumps({'files': len(files), 'formats': dict(sorted(census.items())),
                      'failures': failures[:20], 'failure_count': len(failures)}, indent=1))
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
