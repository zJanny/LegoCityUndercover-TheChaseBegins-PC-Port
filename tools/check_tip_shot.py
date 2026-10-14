#!/usr/bin/env python3
"""Checks a chase-game screenshot (400x480, both screens) taken while the tip text "Use <stick> to move Chase." of mission
1-1 is fully faded in: the tip panel border (light blue) must lie on the bottom screen inside the box measured on the
oracle (x 61..340, y 312..408 of the window), the text colour (74, 173, 255) must cover a plausible number of pixels
inside the box, and the top screen must contain no text of that colour."""
import struct
import sys


def read_bmp(path):
    data = open(path, "rb").read()
    if data[:2] != b"BM":
        raise SystemExit(f"{path}: not a BMP")
    offset = struct.unpack_from("<I", data, 10)[0]
    width, height = struct.unpack_from("<ii", data, 18)
    bits = struct.unpack_from("<H", data, 28)[0]
    if bits != 32 or width != 400 or abs(height) != 480:
        raise SystemExit(f"{path}: expected a 400x480 32-bit BMP")
    rows = abs(height)

    def pixel(x, y):
        row = y if height < 0 else rows - 1 - y
        base = offset + (row * width + x) * 4
        return data[base + 2], data[base + 1], data[base]

    return pixel


def near(colour, target, tolerance=10):
    return all(abs(a - b) <= tolerance for a, b in zip(colour, target))


def main(argv):
    if len(argv) != 2:
        raise SystemExit("usage: check_tip_shot.py SHOT.bmp")
    pixel = read_bmp(argv[1])
    text_colour = (74, 173, 255)
    inside = sum(1 for y in range(312, 409) for x in range(61, 341) if near(pixel(x, y), text_colour))
    top = sum(1 for y in range(0, 240) for x in range(400) if near(pixel(x, y), text_colour, 4))
    if not 150 <= inside <= 4000:
        raise SystemExit(f"{inside} text-coloured pixels in the tip box, expected 150..4000")
    if top > 40:
        raise SystemExit(f"{top} text-coloured pixels on the top screen")
    print(f"tip panel: {inside} text-coloured pixels inside the box, {top} on the top screen")


main(sys.argv)
