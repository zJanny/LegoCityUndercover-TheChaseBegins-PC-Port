#!/usr/bin/env python3
"""List the objects of TT Fusion .blvl level files with their parameters, naming the parameters.

The levels are parsed by the C++ asset runtime (chase::assets::parseLevel) through
tools/chase_native.py; build the project first. Parameter names are stored only as CRC-32
hashes. They are resolved from the printable strings of the decompressed executable
(artifacts/game/code.bin), which stays local. The format is described in docs/re/levels.md.
"""
import argparse
import re
import struct
import sys
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import chase_native as native  # noqa: E402


def param_names(code_path):
    """Map hash -> name using printable strings of the decompressed executable (local only)."""
    names = {}
    for match in re.finditer(rb'[\x20-\x7e]{2,64}', Path(code_path).read_bytes()):
        text = match.group().decode()
        names.setdefault(zlib.crc32(text.lower().encode()), text)
    return names


def cstring(block):
    return block.split(b'\0', 1)[0].decode('latin-1') if block else None


def param_value(kind, value, blocks):
    if kind == 0:
        return struct.unpack('<f', struct.pack('<I', value))[0]
    if kind in (5, 7):
        return value
    if kind == 2 and blocks and blocks[0] and len(blocks[0]) >= 12:
        return list(struct.unpack_from('<3f', blocks[0]))
    if kind == 1 and blocks and blocks[0]:
        return cstring(blocks[0])
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('inputs', type=Path, nargs='+')
    parser.add_argument('--param-names', type=Path, metavar='CODE_BIN', required=True,
                        help='resolve parameter hashes from artifacts/game/code.bin strings')
    args = parser.parse_args()
    try:
        native.library()
        names = param_names(args.param_names)
        for path in args.inputs:
            for class_name, instance, room, params in native.level_objects(path.read_bytes()):
                print(class_name, instance, room)
                for name_hash, kind, value, blocks in params:
                    print('   %-24s type=%d %s' % (names.get(name_hash, '%08x' % name_hash), kind,
                                                   param_value(kind, value, blocks)))
    except (ValueError, OSError, native.NativeUnavailable) as exc:
        parser.exit(1, str(exc) + '\n')
    return 0


if __name__ == '__main__':
    sys.exit(main())
