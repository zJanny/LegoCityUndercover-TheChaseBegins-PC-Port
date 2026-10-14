#!/usr/bin/env python3
"""Checks a chase-game screenshot (400x480, both screens) of the level start of mission 1-1 after the tips are gone: the map
marker of 1-1_Waypoint_01 (blue disc ring, colour 74, 174, 255) must sit where the oracle shows it on the bottom screen.
Oracle reference (state gameplay-start, RPC screenshot of the original, artifacts/oracle/agentOH-out/gs0.png): 74 pixels of
that colour near the marker, centroid (218.5, 285.7), measured with the player at world (886.76, 1111.17) looking at camera
yaw -2.3501; the port's player starts at the same place. The top screen must not contain the colour in the map area."""
import struct
import sys

ORACLE_CENTROID = (218.5, 285.7)
ORACLE_COUNT = 74


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


def near(colour, target, tolerance=6):
    return all(abs(a - b) <= tolerance for a, b in zip(colour, target))


def main(argv):
    if len(argv) != 2:
        raise SystemExit("usage: check_waypoint_shot.py SHOT.bmp")
    pixel = read_bmp(argv[1])
    blue = (74, 174, 255)
    points = [(x, y) for y in range(271, 301) for x in range(204, 234) if near(pixel(x, y), blue)]
    if not 0.5 * ORACLE_COUNT <= len(points) <= 1.6 * ORACLE_COUNT:
        raise SystemExit(f"{len(points)} marker-coloured pixels near the marker, oracle {ORACLE_COUNT}")
    cx = sum(p[0] for p in points) / len(points)
    cy = sum(p[1] for p in points) / len(points)
    if abs(cx - ORACLE_CENTROID[0]) > 4 or abs(cy - ORACLE_CENTROID[1]) > 4:
        raise SystemExit(f"marker centre ({cx:.1f}, {cy:.1f}), oracle {ORACLE_CENTROID}")
    print(f"map marker: {len(points)} pixels, centre ({cx:.1f}, {cy:.1f}), oracle {ORACLE_CENTROID} / {ORACLE_COUNT}")


main(sys.argv)
