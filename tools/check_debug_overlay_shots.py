#!/usr/bin/env python3
"""Checks that a chase-game screenshot with the debug overlay differs from the one without it only inside the
overlay panel (top-left corner, at most PANEL_WIDTH x PANEL_HEIGHT pixels) and differs there."""
import struct
import sys

PANEL_WIDTH = 480
PANEL_HEIGHT = 280


def read_bmp(path):
    data = open(path, "rb").read()
    if data[:2] != b"BM":
        raise SystemExit(f"{path}: not a BMP")
    offset = struct.unpack_from("<I", data, 10)[0]
    width, height = struct.unpack_from("<ii", data, 18)
    bits = struct.unpack_from("<H", data, 28)[0]
    if bits != 32:
        raise SystemExit(f"{path}: expected 32 bits per pixel, got {bits}")
    return width, abs(height), height < 0, data[offset:offset + width * abs(height) * 4]


def main(argv):
    if len(argv) != 3:
        raise SystemExit("usage: check_debug_overlay_shots.py OFF.bmp ON.bmp")
    off, on = read_bmp(argv[1]), read_bmp(argv[2])
    if off[:3] != on[:3]:
        raise SystemExit("screenshots differ in size or row order")
    width, height, top_down, _ = off
    differing = []
    for y in range(height):
        row_start = y * width * 4
        if off[3][row_start:row_start + width * 4] == on[3][row_start:row_start + width * 4]:
            continue
        for x in range(width):
            if off[3][row_start + x * 4:row_start + x * 4 + 4] != on[3][row_start + x * 4:row_start + x * 4 + 4]:
                differing.append((x, y if top_down else height - 1 - y))
    if not differing:
        raise SystemExit("the overlay changed no pixel")
    outside = [p for p in differing if p[0] >= PANEL_WIDTH or p[1] >= PANEL_HEIGHT]
    if outside:
        raise SystemExit(f"{len(outside)} pixel(s) differ outside the overlay panel, first at {outside[0]}")
    print(f"overlay changed {len(differing)} pixel(s), all inside the top-left {PANEL_WIDTH}x{PANEL_HEIGHT} panel area")


main(sys.argv)
