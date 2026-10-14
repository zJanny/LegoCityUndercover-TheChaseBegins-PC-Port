#!/usr/bin/env python3
"""Checks the chase-game picture at the police entrance of Cherry Tree Hills: the ground under the player holds no
pale blue-grey disc. Before LEVOBJ-11 was fixed, the hidden floor plaque (Police_Plaque_Floor) drew one there: 11129
pixels of rows 150..299 had blue 15 above red, blue above 110 and green above 110; the fixed picture has 71.
The ground of the street is warm (red above blue), so the count must stay far below the old value."""
import struct
import sys

FIRST_ROW = 150
LAST_ROW = 299
LIMIT = 500


def main(argv):
    if len(argv) != 2:
        raise SystemExit("usage: check_police_entrance_shot.py SHOT.bmp")
    data = open(argv[1], "rb").read()
    if data[:2] != b"BM":
        raise SystemExit(f"{argv[1]}: not a BMP")
    offset = struct.unpack_from("<I", data, 10)[0]
    width, height = struct.unpack_from("<ii", data, 18)
    bits = struct.unpack_from("<H", data, 28)[0]
    if bits != 32:
        raise SystemExit(f"{argv[1]}: expected 32 bits per pixel, got {bits}")
    top_down = height < 0
    height = abs(height)
    if height <= LAST_ROW:
        raise SystemExit(f"{argv[1]}: picture too small")
    pale = 0
    for row in range(FIRST_ROW, LAST_ROW + 1):
        start = offset + (row if top_down else height - 1 - row) * width * 4
        for x in range(width):
            blue, green, red = data[start + x * 4], data[start + x * 4 + 1], data[start + x * 4 + 2]
            if blue - red >= 15 and blue > 110 and green > 110:
                pale += 1
    if pale >= LIMIT:
        raise SystemExit(f"{pale} pale blue-grey pixels under the player (limit {LIMIT}): the floor plaque disc is drawn")
    print(f"{pale} pale blue-grey pixels in rows {FIRST_ROW}..{LAST_ROW} (limit {LIMIT})")


main(sys.argv)
