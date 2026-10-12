#!/usr/bin/env python3
"""List the PICA200 shader programs (DVLB/DVLE) embedded in the decoded .code.

Uses the public SHBIN layout: DVLB header with DVLE offsets, one DVLP with the
shared code and operand descriptors, and per program a DVLE with entry points,
constant/uniform tables and a symbol table. Prints program type, entry range,
uniform names and register ranges, output semantics, and constants.
"""
import argparse
import struct
import sys
from pathlib import Path

OUTPUT_TYPES = {0: 'position', 1: 'normal_quat', 2: 'color', 3: 'texcoord0', 4: 'texcoord0w',
                5: 'texcoord1', 6: 'texcoord2', 8: 'view'}


def cstr(data, offset):
    end = data.index(b'\0', offset)
    return data[offset:end].decode('ascii', 'replace')


def dump(data, base):
    if data[base:base + 4] != b'DVLB':
        raise ValueError("no DVLB at 0x%x" % base)
    count = struct.unpack_from('<I', data, base + 4)[0]
    if not 0 < count < 256:
        raise ValueError("implausible DVLE count %d" % count)
    offsets = struct.unpack_from('<%dI' % count, data, base + 8)
    dvlp = base + 8 + 4 * count
    if data[dvlp:dvlp + 4] != b'DVLP':
        raise ValueError("missing DVLP")
    code_off, code_words, opdesc_off, opdesc_count = struct.unpack_from('<IIII', data, dvlp + 8)
    print('DVLB at 0x%x: %d programs, %d code words, %d operand descriptors' % (base, count, code_words, opdesc_count))
    for i, off in enumerate(offsets):
        dvle = base + off
        if data[dvle:dvle + 4] != b'DVLE':
            raise ValueError("missing DVLE %d" % i)
        kind = data[dvle + 6]
        main, endmain = struct.unpack_from('<II', data, dvle + 8)
        (const_off, const_n, label_off, label_n, out_off, out_n, uni_off, uni_n,
         sym_off, sym_size) = struct.unpack_from('<10I', data, dvle + 0x18)
        print('\nprogram %d: %s shader, main=0x%x..0x%x' % (i, 'geometry' if kind else 'vertex', main, endmain))
        outs = []
        for k in range(out_n):
            otype, oreg, mask = struct.unpack_from('<HHH', data, dvle + out_off + 8 * k)
            outs.append('o%d=%s(mask %x)' % (oreg, OUTPUT_TYPES.get(otype, otype), mask & 0xf))
        print('  outputs:', ', '.join(outs))
        for k in range(uni_n):
            sym, start, end = struct.unpack_from('<IHH', data, dvle + uni_off + 8 * k)
            name = cstr(data, dvle + sym_off + sym)
            print('  uniform %-24s regs 0x%02x-0x%02x' % (name, start, end))
        for k in range(const_n):
            ctype, reg = struct.unpack_from('<BxH', data, dvle + const_off + 20 * k)
            raw = data[dvle + const_off + 20 * k + 4:dvle + const_off + 20 * k + 20]
            print('  const type=%d reg=%d raw=%s' % (ctype, reg, raw.hex(' ', 4)))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('code', type=Path, help='artifacts/game/code.bin')
    args = parser.parse_args()
    data = args.code.read_bytes()
    start = 0
    found = 0
    while (start := data.find(b'DVLB', start)) >= 0:
        try:
            dump(data, start)
            found += 1
        except (ValueError, struct.error) as exc:
            print('candidate at 0x%x rejected: %s' % (start, exc))
        start += 4
    return 0 if found else 1


if __name__ == '__main__':
    sys.exit(main())
