#!/usr/bin/env python3
"""Sample the player's vehicle in a kept Azahar session (needs --rpc and --rpc-input) while buttons and the stick follow a plan.

usage: vehicle_drive_sampler.py NAME 'PLAN_JSON' SECONDS
PLAN_JSON is a list of [host_seconds, [buttons], stick_x, stick_y] entries, an optional fifth entry [record_offset, byte] pokes one byte of the
vehicle record. NAME.csv gets one row per changed vehicle pose: 's', host time, clock, right, up and forward axes (x components first: m0 m1 m2,
m4 m5 m6, m8 m9 m10), position x y z, record states +2 and +4. 'input' rows mark every change of the pad. Used for docs/re/vehicles-driving.md.
"""
import json
import struct
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import azahar_oracle as ao

PLAYER_POINTER = 0x3772b8
CLOCK = 0x376b0c
BUTTONS = ao.BUTTONS


class NoGdb:
    breakpoints = set()

    def add_breakpoint(self, address):
        pass


def pointers(rpc):
    entity = rpc.u32(PLAYER_POINTER)
    controller = rpc.u32(entity + 0x7c)
    vehicle = rpc.u32(controller + 0x130)
    return rpc.u32(vehicle + 0x7c), rpc.u32(vehicle + 0x3c)


def sample(name, plan, total):
    rpc = ao.RpcRemote()
    rpc.select_application()
    oracle = ao.Oracle(NoGdb(), Path('.'), rpc=rpc, rpc_input=True)
    record, node = pointers(rpc)
    oracle.held = 0
    oracle.inject_input()
    oracle.free_stick(0, 0)
    rows = []
    last = None
    index = 0
    start = time.monotonic()
    while True:
        elapsed = time.monotonic() - start
        if elapsed > total:
            break
        while index < len(plan) and plan[index][0] <= elapsed:
            entry = plan[index]
            mask = 0
            for button in entry[1]:
                mask |= BUTTONS[button]
            if len(entry) > 4:
                rpc.write(record + entry[4][0], bytes([entry[4][1]]))
            rpc.write(ao.PAD_MAILBOX, struct.pack('<I', mask))
            oracle.free_stick(entry[2], entry[3])
            rows.append(('input', elapsed, rpc.u32(CLOCK), mask, entry[2], entry[3]))
            index += 1
        clock = rpc.u32(CLOCK)
        matrix = struct.unpack('<16f', rpc.read(node + 0x5c, 64))
        states = struct.unpack('<2H', rpc.read(record + 2, 4))
        key = (matrix[12], matrix[13], matrix[14], matrix[8], matrix[10])
        if key != last:
            last = key
            rows.append(('s', elapsed, clock, matrix[0], matrix[1], matrix[2], matrix[4], matrix[5], matrix[6], matrix[8], matrix[9], matrix[10],
                         matrix[12], matrix[13], matrix[14], states[0], states[1]))
    rpc.write(ao.PAD_MAILBOX, struct.pack('<I', 0))
    oracle.free_stick(0, 0)
    with open(name + '.csv', 'w') as output:
        for row in rows:
            output.write(','.join(str(value) for value in row) + '\n')
    return len(rows)


if __name__ == '__main__':
    print(sample(sys.argv[1], json.loads(sys.argv[2]), float(sys.argv[3])), 'rows')
