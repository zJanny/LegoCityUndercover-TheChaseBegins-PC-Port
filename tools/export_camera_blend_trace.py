#!/usr/bin/env python3
"""Turn a camera blend memtrace (.mem of tools/oracle/camera-lobby-east.txt) into the CSV that camera-blend-real-data reads.

Per frame the memtrace holds u32 frame, f32 dt and the regions of the scenario (layout seed: the follow camera object 0x589a70 (0x200), the camera object, its stack area and the player node): the camera object C (0x100), the command
stack area after it (0x300), the blend block 0x58d694 (0x60), the previous record set 0x58d880 (0x30), the player node
(0xb0) and 0x3770c0 (0x20). The CSV carries the player node world position, the camera pose C+0x4c (look), C+0x70 (eye),
C+0x7c (FOV), the filter factors C+0xbc / C+0xcc, the top command entry pointer C+0x00, its weight (+0x9c) and the
number of entries on the stack (chain +0x94). The last six columns are the look point and the eye stored in the follow
controller's own stack entry (command routine 0x2a8bfc at entry +0x78); empty while it is not on the stack.
"""
import argparse
import csv
import struct
import sys
from pathlib import Path

LAYOUTS = {
    'blend': [('pose', 0x100), ('stack', 0x300), ('blocks', 0x60), ('previous', 0x30), ('node', 0xb0), ('other', 0x20)],
    'seed': [('follow', 0x200), ('pose', 0x100), ('stack', 0x300), ('node', 0xb0)],
}
CAMERA_OBJECT = 0x08501ad0
FOLLOW_ROUTINE = 0x002a8bfc


def floats(data, offset, count):
    return struct.unpack_from('<%df' % count, data, offset)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('trace', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--layout', choices=sorted(LAYOUTS), default='blend')
    arguments = parser.parse_args()
    data = arguments.trace.read_bytes()
    layout = LAYOUTS[arguments.layout]
    record = 8 + sum(size for _, size in layout)
    if len(data) % record:
        sys.exit('trace size %d is not a multiple of %d' % (len(data), record))
    arguments.output.parent.mkdir(parents=True, exist_ok=True)
    with open(arguments.output, 'w', newline='', encoding='utf-8') as out:
        writer = csv.writer(out)
        writer.writerow('frame dt player_x player_y player_z look_x look_y look_z eye_x eye_y eye_z fov '
                        'eye_factor look_factor top top_weight depth follow_look_x follow_look_y follow_look_z '
                        'follow_eye_x follow_eye_y follow_eye_z'.split())
        for index in range(len(data) // record):
            block = data[index * record:(index + 1) * record]
            frame, dt = struct.unpack_from('<If', block, 0)
            regions = {}
            position = 8
            for name, size in layout:
                regions[name] = block[position:position + size]
                position += size
            pose, node = regions['pose'], regions['node']
            stack = pose + regions['stack']
            entry = struct.unpack_from('<I', pose, 0)[0]
            top_weight = struct.unpack_from('<f', stack, entry + 0x9c - CAMERA_OBJECT)[0]
            depth = 0
            follow = [''] * 6
            while entry:
                depth += 1
                offset = entry - CAMERA_OBJECT
                if struct.unpack_from('<I', stack, offset + 0x78)[0] == FOLLOW_ROUTINE:
                    follow = ['%.6f' % v for v in floats(stack, offset, 3) + floats(stack, offset + 0x24, 3)]
                entry = struct.unpack_from('<I', stack, offset + 0x94)[0]
            writer.writerow([frame, '%.6f' % dt,
                             *('%.6f' % v for v in floats(node, 0x8c, 3)),
                             *('%.6f' % v for v in floats(pose, 0x4c, 3)),
                             *('%.6f' % v for v in floats(pose, 0x70, 3)),
                             '%.6f' % floats(pose, 0x7c, 1)[0],
                             '%.6f' % floats(pose, 0xcc, 1)[0], '%.6f' % floats(pose, 0xbc, 1)[0],
                             '%08x' % struct.unpack_from('<I', pose, 0)[0], '%.6f' % top_weight, depth, *follow])
    print('%s: %d rows' % (arguments.output, len(data) // record))


if __name__ == '__main__':
    main()
