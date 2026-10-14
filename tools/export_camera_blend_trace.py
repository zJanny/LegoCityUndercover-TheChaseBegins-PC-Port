#!/usr/bin/env python3
"""Turn a camera blend memtrace (.mem of tools/oracle/camera-lobby-east.txt) into the CSV that camera-blend-real-data reads.

Per frame the memtrace holds u32 frame, f32 dt and the regions of the scenario: the camera object C (0x100), the command
stack area after it (0x300), the blend block 0x58d694 (0x60), the previous record set 0x58d880 (0x30), the player node
(0xb0) and 0x3770c0 (0x20). The CSV carries the player node world position, the camera pose C+0x4c (look), C+0x70 (eye),
C+0x7c (FOV), the filter factors C+0xbc / C+0xcc, the top command entry pointer C+0x00, its weight (+0x9c) and the
number of entries on the stack (chain +0x94).
"""
import argparse
import csv
import struct
import sys
from pathlib import Path

REGIONS = [0x100, 0x300, 0x60, 0x30, 0xb0, 0x20]
STACK_BASE = 0x08501bd0  # first address of the second region (command stack entries of the camera object)


def floats(data, offset, count):
    return struct.unpack_from('<%df' % count, data, offset)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('trace', type=Path)
    parser.add_argument('output', type=Path)
    arguments = parser.parse_args()
    data = arguments.trace.read_bytes()
    record = 8 + sum(REGIONS)
    if len(data) % record:
        sys.exit('trace size %d is not a multiple of %d' % (len(data), record))
    arguments.output.parent.mkdir(parents=True, exist_ok=True)
    with open(arguments.output, 'w', newline='') as out:
        writer = csv.writer(out)
        writer.writerow('frame dt player_x player_y player_z look_x look_y look_z eye_x eye_y eye_z fov '
                        'eye_factor look_factor top top_weight depth'.split())
        for index in range(len(data) // record):
            block = data[index * record:(index + 1) * record]
            frame, dt = struct.unpack_from('<If', block, 0)
            pose = block[8:8 + REGIONS[0]]
            node = block[8 + sum(REGIONS[:4]):8 + sum(REGIONS[:5])]
            stack = block[8 + REGIONS[0]:8 + REGIONS[0] + REGIONS[1]]
            entry = struct.unpack_from('<I', pose, 0)[0]
            top_weight = struct.unpack_from('<f', stack, entry + 0x9c - STACK_BASE)[0]
            depth = 0
            while entry:
                depth += 1
                entry = struct.unpack_from('<I', stack, entry + 0x94 - STACK_BASE)[0]
            writer.writerow([frame, '%.6f' % dt,
                             *('%.6f' % v for v in floats(node, 0x8c, 3)),
                             *('%.6f' % v for v in floats(pose, 0x4c, 3)),
                             *('%.6f' % v for v in floats(pose, 0x70, 3)),
                             '%.6f' % floats(pose, 0x7c, 1)[0],
                             '%.6f' % floats(pose, 0xcc, 1)[0], '%.6f' % floats(pose, 0xbc, 1)[0],
                             '%08x' % struct.unpack_from('<I', pose, 0)[0], '%.6f' % top_weight, depth])
    print('%s: %d rows' % (arguments.output, len(data) // record))


if __name__ == '__main__':
    main()
