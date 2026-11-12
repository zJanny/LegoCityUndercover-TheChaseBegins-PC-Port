#!/usr/bin/env python3
"""Turn a camera memtrace of tools/oracle/camera-trace.txt (.mem) into the CSV the camera differential test reads.

Per frame the memtrace holds: u32 frame, f32 dt, then the camera pose object (0x100 bytes at C), the camera
system (0x200 bytes at 0x589a70), the strategy (0x40 bytes), the player node (0xb0) and two more regions.
Traces of tools/oracle/camwall-*.txt carry further regions after them; --extra gives their total size.
The pad state that was injected for the trace is not stored in the memtrace, so it is given here.
"""
import argparse
import csv
import struct
import sys
from pathlib import Path

REGIONS = [0x100, 0x200, 0x40, 0xb0, 0x60, 0x30]


def floats(data, offset, count):
    return struct.unpack_from('<%df' % count, data, offset)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('trace', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--stick-x', type=int, default=0)
    parser.add_argument('--stick-y', type=int, default=0)
    parser.add_argument('--held', type=lambda text: int(text, 0), default=0, help='HID button mask (R 0x100, L 0x200)')
    parser.add_argument('--extra', type=lambda text: int(text, 0), default=0, help='total size of the regions after the first six')
    arguments = parser.parse_args()
    data = arguments.trace.read_bytes()
    record = 8 + sum(REGIONS) + arguments.extra
    if len(data) % record:
        sys.exit('trace size %d is not a multiple of %d' % (len(data), record))
    arguments.output.parent.mkdir(parents=True, exist_ok=True)
    with open(arguments.output, 'w', newline='', encoding='utf-8') as out:
        writer = csv.writer(out)
        writer.writerow('frame dt stick_x stick_y held player_x player_y player_z facing_x facing_z '
                        'look_x look_y look_z eye_x eye_y eye_z wanted_x wanted_y wanted_z clearance spring_hold'.split())
        for index in range(len(data) // record):
            block = data[index * record:(index + 1) * record]
            frame, dt = struct.unpack_from('<If', block, 0)
            pose = block[8:8 + REGIONS[0]]
            system = block[8 + REGIONS[0]:8 + REGIONS[0] + REGIONS[1]]
            node = block[8 + sum(REGIONS[:3]):8 + sum(REGIONS[:4])]
            strategy = block[8 + sum(REGIONS[:2]):8 + sum(REGIONS[:3])]
            writer.writerow([frame, '%.6f' % dt, arguments.stick_x, arguments.stick_y, arguments.held,
                             *('%.6f' % v for v in floats(node, 0x8c, 3)),
                             '%.6f' % floats(node, 0x7c, 1)[0], '%.6f' % floats(node, 0x84, 1)[0],
                             *('%.6f' % v for v in floats(pose, 0x10, 3)),
                             *('%.6f' % v for v in floats(pose, 0x34, 3)),
                             *('%.6f' % v for v in floats(system, 4, 3)),
                             '%.6f' % floats(strategy, 4, 1)[0], '%.6f' % floats(strategy, 0x24, 1)[0]])
    print('%s: %d rows' % (arguments.output, len(data) // record))


if __name__ == '__main__':
    main()
