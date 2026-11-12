#!/usr/bin/env python3
"""Turn a vehicle camera memtrace (.mem of tools/oracle/camera-vehicle-drive.txt) into the CSV that camera-vehicle-real-data reads.

Per frame the memtrace holds u32 frame, f32 dt and the regions of the scenario: the follow camera object K at 0x589a70 (0x200), the
working copy of the InVehicle preset 0x514170 (0x70), the target entity (0x100), the vehicle record (0x340), the camera object C
(0x100), the command stack area after it (0x300) and the vehicle node (0x100). The CSV carries the vehicle node world translation
(node + 0x8c) and heading (atan2 of the forward row of the node matrix at node + 0x5c (rows of 16 bytes)), the wanted camera position K+4, the camera
tracker K+0x34 (eye), the look point K+0x94 + K+0xf4, the pose eye / look / FOV of C, the top command's routine and weight, and
the number of commands on the stack. With --layout enter (tools/oracle/camera-vehicle-enter.txt) the regions are K, C, the stack area, the
vehicle node, the player node (0xb0) and the player controller (0x140), and the CSV ends with the player world position and the locomotion
state word ctrl+0x6c.
"""
import argparse
import csv
import math
import struct
import sys
from pathlib import Path

LAYOUTS = {
    'drive': [('follow', 0x200), ('preset', 0x70), ('entity', 0x100), ('record', 0x340), ('pose', 0x100), ('stack', 0x300), ('node', 0x100)],
    'enter': [('follow', 0x200), ('pose', 0x100), ('stack', 0x300), ('node', 0x100), ('player', 0xb0), ('controller', 0x140)],
}
CAMERA_OBJECT = 0x08501ad0
HEADER = ('frame dt vehicle_x vehicle_y vehicle_z heading wanted_x wanted_y wanted_z eye_x eye_y eye_z look_x look_y look_z '
          'pose_eye_x pose_eye_y pose_eye_z pose_look_x pose_look_y pose_look_z fov top_routine top_weight depth').split()
ENTER_COLUMNS = 'player_x player_y player_z state'.split()


def floats(data, offset, count):
    return struct.unpack_from('<%df' % count, data, offset)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('trace', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--layout', choices=sorted(LAYOUTS), default='drive')
    arguments = parser.parse_args()
    data = arguments.trace.read_bytes()
    layout = LAYOUTS[arguments.layout]
    record = 8 + sum(size for _, size in layout)
    if len(data) % record:
        sys.exit('trace size %d is not a multiple of %d' % (len(data), record))
    arguments.output.parent.mkdir(parents=True, exist_ok=True)
    with open(arguments.output, 'w', newline='', encoding='utf-8') as out:
        writer = csv.writer(out)
        writer.writerow(HEADER + (ENTER_COLUMNS if arguments.layout == 'enter' else []))
        for index in range(len(data) // record):
            block = data[index * record:(index + 1) * record]
            frame, dt = struct.unpack_from('<If', block, 0)
            regions = {}
            position = 8
            for name, size in layout:
                regions[name] = block[position:position + size]
                position += size
            follow, pose, node = regions['follow'], regions['pose'], regions['node']
            stack = pose + regions['stack']
            entry = struct.unpack_from('<I', pose, 0)[0]
            top_routine = struct.unpack_from('<I', stack, entry - CAMERA_OBJECT + 0x78)[0]
            top_weight = struct.unpack_from('<f', stack, entry - CAMERA_OBJECT + 0x9c)[0]
            depth = 0
            while entry:
                depth += 1
                entry = struct.unpack_from('<I', stack, entry - CAMERA_OBJECT + 0x94)[0]
            forward = floats(node, 0x7c, 3)
            heading = math.atan2(forward[0], forward[2])
            eye = floats(follow, 0x34, 3)
            look = [a + b for a, b in zip(floats(follow, 0x94, 3), floats(follow, 0xf4, 3))]
            writer.writerow([frame, '%.6f' % dt,
                             *('%.6f' % v for v in floats(node, 0x8c, 3)), '%.7f' % heading,
                             *('%.6f' % v for v in floats(follow, 4, 3)),
                             *('%.6f' % v for v in eye), *('%.6f' % v for v in look),
                             *('%.6f' % v for v in floats(pose, 0x70, 3)), *('%.6f' % v for v in floats(pose, 0x4c, 3)),
                             '%.6f' % floats(pose, 0x7c, 1)[0], '%08x' % top_routine, '%.6f' % top_weight, depth,
                             *(['%.6f' % v for v in floats(regions['player'], 0x8c, 3)] + [struct.unpack_from('<H', regions['controller'], 0x6c)[0]]
                               if arguments.layout == 'enter' else [])])
    print('%s: %d rows' % (arguments.output, len(data) // record))


if __name__ == '__main__':
    main()
