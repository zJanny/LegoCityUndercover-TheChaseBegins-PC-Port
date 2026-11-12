import json
import os
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib
from pathlib import Path

TOOLS = Path(__file__).parents[1] / 'tools'

KEPT_GIT_VARIABLES = frozenset(('GIT_EXEC_PATH', 'GIT_TERMINAL_PROMPT'))


def is_git_environment_variable(name):
    return name.upper().startswith('GIT_') and name.upper() not in KEPT_GIT_VARIABLES


def without_git_environment(environment):
    return {name: value for name, value in environment.items() if not is_git_environment_variable(name)}


def isolate_git_environment():
    for name in [name for name in os.environ if is_git_environment_variable(name)]:
        del os.environ[name]


def symlinks_allowed():
    with tempfile.TemporaryDirectory() as scratch:
        try:
            (Path(scratch) / 'link').symlink_to(Path(scratch), target_is_directory=True)
        except (OSError, NotImplementedError):
            return False
    return True


requires_symlinks = unittest.skipUnless(
    symlinks_allowed(), 'creating symbolic links needs a privilege the current user lacks (Windows without Developer Mode)')


def pooled(blocks):
    """blocks: [(marker, bytes)] in table order -> pooled resource bytes."""
    table, static, body, current = [], b'', b'', None
    for marker, data in blocks:
        if marker != current:
            table.append(marker)
            current = marker
        table.append(len(data))
        if marker == -16:
            static += data
        else:
            body += data
    head = struct.pack('<II', len(table) << 8, 4 * len(table) + len(static))
    return head + struct.pack('<%di' % len(table), *table) + static + body


def run_tool(name, *args):
    return subprocess.run([sys.executable, str(TOOLS / name), *map(str, args)], capture_output=True, text=True)


def run_json_tool(name, *args):
    result = run_tool(name, *args)
    return result, json.loads(result.stdout) if result.stdout.strip().startswith('{') else None


def read_png(path):
    """(width, height, rows) of an 8-bit RGBA PNG with unfiltered rows; rows are lists of RGBA tuples."""
    data = Path(path).read_bytes()
    assert data[:8] == b'\x89PNG\r\n\x1a\n'
    width, height = struct.unpack('>II', data[16:24])
    idat = data.index(b'IDAT')
    length = struct.unpack('>I', data[idat - 4:idat])[0]
    raw = zlib.decompress(data[idat + 4:idat + 4 + length])
    rows = []
    for y in range(height):
        row = raw[y * (width * 4 + 1):(y + 1) * (width * 4 + 1)]
        assert row[0] == 0
        rows.append([tuple(row[1 + 4 * x:5 + 4 * x]) for x in range(width)])
    return width, height, rows


MORTON = [((i & 1) | (i >> 1 & 2) | (i >> 2 & 4), (i >> 1 & 1) | (i >> 2 & 2) | (i >> 3 & 4)) for i in range(64)]


def texture(width, height, fmt, levels, payload, cube=0):
    header = struct.pack('<IIiIiIHHIBBBBI', 0x400, 0x10, -14, 0x20, -14, len(payload), width, height,
                         len(payload), fmt, 0, 0, cube, levels)
    return header + bytes(16) + payload


def rgba8_texture(width, height, pixel):
    """RGBA8 texture whose displayed pixel (x, y), y from the top, is pixel(x, y) -> (r, g, b, a)."""
    payload = bytearray()
    for tile in range(width // 8 * (height // 8)):
        tx, ty = tile % (width // 8) * 8, tile // (width // 8) * 8
        for mx, my in MORTON:
            r, g, b, a = pixel(tx + mx, height - 1 - (ty + my))
            payload += bytes((a, b, g, r))
    return texture(width, height, 0, 1, bytes(payload))
