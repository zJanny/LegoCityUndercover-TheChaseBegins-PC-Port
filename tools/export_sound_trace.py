#!/usr/bin/env python3
"""Turn a `trap` result of tools/azahar_oracle.py (a NAME.txt of register dumps at the sound call sites) into the compact CSV
that tests/audio_real_test.cpp replays.

Only the calls of one object are kept: the player entity (the first argument r1 of the object wrapper 0x23ae84, `--object`)
or a stud pickup record (r1 of 0x227880's call). Columns: update (frame number minus --first-update), id (r0), object (r1),
caller (lr). For a stud record the three floats at its start (the stud centre) are appended as x, y, z.

  export_sound_trace.py out/walk.csv ev_walk.txt --first-update 35 --object 0x85526f0
"""
import argparse
import csv
import struct
import sys
from pathlib import Path


def parse(path):
    rows, current = [], None
    for line in Path(path).read_text(encoding='utf-8').splitlines():
        text = line.strip()
        if line.startswith('frame'):
            current = {'frame': int(line.split()[1])}
            rows.append(current)
        elif text.startswith('r0-r15'):
            current['r'] = [int(word, 16) for word in text.split()[1:]]
        elif text.startswith('[r1]'):
            current['memory'] = bytes.fromhex(''.join(text.split()[1:]))
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('output', type=Path)
    parser.add_argument('traces', type=Path, nargs='+', help='NAME.txt results of consecutive trap blocks of one scenario')
    parser.add_argument('--first-update', type=int, required=True, help='frame number of the first update of the scenario')
    parser.add_argument('--object', type=lambda text: int(text, 0), action='append', default=[],
                        help='keep calls whose r1 is this address (repeatable)')
    parser.add_argument('--caller', type=lambda text: int(text, 0), action='append', default=[],
                        help='keep calls with this return address and append the floats at r1 as x y z (repeatable)')
    args = parser.parse_args()
    kept = []
    rows = [row for trace in args.traces for row in parse(trace)]
    for row in rows:
        registers = row.get('r')
        if not registers:
            continue
        if registers[1] in args.object:
            kept.append([row['frame'] - args.first_update, registers[0], '0x%x' % registers[1], '0x%x' % registers[14]])
        elif registers[14] in args.caller and len(row.get('memory', b'')) >= 12:
            x, y, z = struct.unpack('<3f', row['memory'][:12])
            kept.append([row['frame'] - args.first_update, registers[0], '0x%x' % registers[1], '0x%x' % registers[14],
                         '%.4f' % x, '%.4f' % y, '%.4f' % z])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open('w', newline='', encoding='utf-8') as handle:
        writer = csv.writer(handle)
        writer.writerow(['update', 'id', 'object', 'caller', 'x', 'y', 'z'])
        writer.writerows(kept)
    print('%s: %d calls' % (args.output, len(kept)))


if __name__ == '__main__':
    sys.exit(main())
