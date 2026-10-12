#!/usr/bin/env python3
"""Decode TT Fusion .bwav sounds (wrapped Nintendo CWAV) from an unpacked .fib to WAV.

Wrapper (recovered from shipped files; see progress log): u32 version 0x400
(one CWAV at 0x44) or 0x600 (CWAVs from 0x4c, one per channel, back to back).
Each CWAV is the standard 3DS BCWAV layout (INFO + DATA blocks) with PCM8,
PCM16, DSP-ADPCM or IMA-ADPCM samples. Loop points are kept in the report.
"""
import argparse
import json
import struct
import sys
import wave
from pathlib import Path

CWAV_START = {0x400: 0x44, 0x600: 0x4c}
ENCODINGS = {0: 'PCM8', 1: 'PCM16', 2: 'DSP-ADPCM', 3: 'IMA-ADPCM'}
IMA_STEPS = [7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60,
             66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371,
             408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707,
             1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
             7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385,
             24623, 27086, 29794, 32767]
IMA_INDEX = [-1, -1, -1, -1, 2, 4, 6, 8]
MAX_SAMPLES = 1 << 26


def clamp16(value):
    return max(-32768, min(32767, value))


def u32(data, offset):
    if offset < 0 or offset + 4 > len(data):
        raise ValueError("CWAV field out of bounds")
    return struct.unpack_from('<I', data, offset)[0]


def reference(data, offset, kind):
    ref_type, ref_offset = struct.unpack_from('<HxxI', data, offset)
    if ref_type != kind:
        raise ValueError("CWAV reference type 0x%04x, expected 0x%04x" % (ref_type, kind))
    return ref_offset


def split_wrapper(data):
    """Return the CWAV byte strings inside a .bwav wrapper."""
    if len(data) < 4 or u32(data, 0) not in CWAV_START:
        raise ValueError("unknown .bwav wrapper version")
    offset, parts = CWAV_START[u32(data, 0)], []
    while offset < len(data):
        if data[offset:offset + 4] != b'CWAV':
            raise ValueError("expected CWAV at 0x%x" % offset)
        size = u32(data, offset + 0xc)
        if size < 0x40 or offset + size > len(data):
            raise ValueError("CWAV size out of bounds")
        parts.append(data[offset:offset + size])
        offset += size
    if not parts:
        raise ValueError("wrapper contains no CWAV")
    return parts


def parse_cwav(cwav):
    magic, bom, header_size, version, size, blocks = struct.unpack_from('<4sHHIIH', cwav, 0)
    if magic != b'CWAV' or bom != 0xfeff or size != len(cwav) or blocks != 2:
        raise ValueError("invalid CWAV header")
    info = reference(cwav, 0x14, 0x7000)
    data_block = reference(cwav, 0x20, 0x7001)
    if cwav[info:info + 4] != b'INFO' or cwav[data_block:data_block + 4] != b'DATA':
        raise ValueError("missing CWAV INFO/DATA block")
    encoding, loop, rate, loop_start, samples = struct.unpack_from('<BBxxIII', cwav, info + 8)
    if encoding not in ENCODINGS or not 0 < rate <= 192000 or samples > MAX_SAMPLES:
        raise ValueError("unsupported CWAV encoding or parameters")
    table = info + 0x1c
    channels = []
    for i in range(u32(cwav, table)):
        channel = table + reference(cwav, table + 4 + i * 8, 0x7100)
        start = data_block + 8 + reference(cwav, channel, 0x1f00)
        adpcm = None
        if encoding in (2, 3):
            adpcm = channel + reference(cwav, channel + 8, 0x0300 if encoding == 2 else 0x0301)
        channels.append(decode_channel(cwav, encoding, start, samples, adpcm))
    return {'encoding': ENCODINGS[encoding], 'loop': bool(loop), 'rate': rate,
            'loop_start': loop_start, 'samples': samples, 'version': '%08x' % version}, channels


def need(cwav, start, length):
    if start < 0 or start + length > len(cwav):
        raise ValueError("CWAV sample data out of bounds")
    return cwav[start:start + length]


def decode_channel(cwav, encoding, start, samples, adpcm):
    if encoding == 0:
        return [b - 256 if b > 127 else b for b in need(cwav, start, samples)]
    if encoding == 1:
        return list(struct.unpack('<%dh' % samples, need(cwav, start, samples * 2)))
    if encoding == 2:
        coefs = struct.unpack_from('<16h', cwav, adpcm)
        _, hist1, hist2 = struct.unpack_from('<Hhh', cwav, adpcm + 32)
        return decode_dsp(need(cwav, start, (samples + 13) // 14 * 8), coefs, hist1, hist2, samples)
    predictor, index = struct.unpack_from('<hB', cwav, adpcm)
    return decode_ima(need(cwav, start, (samples + 1) // 2), predictor, index, samples)


def decode_dsp(data, coefs, hist1, hist2, samples):
    out = []
    for frame in range(0, len(data), 8):
        header = data[frame]
        scale, pred = 1 << (header & 15), header >> 4
        if pred > 7:
            raise ValueError("invalid DSP-ADPCM predictor")
        c1, c2 = coefs[pred * 2], coefs[pred * 2 + 1]
        for byte in data[frame + 1:frame + 8]:
            for nibble in (byte >> 4, byte & 15):
                if len(out) == samples:
                    return out
                nibble = nibble - 16 if nibble > 7 else nibble
                value = clamp16(((nibble * scale << 11) + 1024 + c1 * hist1 + c2 * hist2) >> 11)
                hist2, hist1 = hist1, value
                out.append(value)
    return out


def decode_ima(data, predictor, index, samples):
    out = []
    index = min(max(index, 0), 88)
    for byte in data:
        for nibble in (byte & 15, byte >> 4):
            if len(out) == samples:
                return out
            step = IMA_STEPS[index]
            diff = step >> 3
            if nibble & 4:
                diff += step
            if nibble & 2:
                diff += step >> 1
            if nibble & 1:
                diff += step >> 2
            predictor = clamp16(predictor - diff if nibble & 8 else predictor + diff)
            index = min(max(index + IMA_INDEX[nibble & 7], 0), 88)
            out.append(predictor)
    return out


def decode(data):
    """Return (info, channels) for a whole .bwav; parts become extra channels."""
    infos, channels = [], []
    for part in split_wrapper(data):
        info, decoded = parse_cwav(part)
        infos.append(info)
        channels += decoded
    if len({i['rate'] for i in infos}) != 1 or not channels:
        raise ValueError("CWAV parts disagree on sample rate")
    info = dict(infos[0], parts=len(infos), channels=len(channels))
    length = max(len(c) for c in channels)
    if encodings := {i['encoding'] for i in infos} - {info['encoding']}:
        info['encoding'] += '+' + '+'.join(sorted(encodings))
    return info, [c + [0] * (length - len(c)) for c in channels]


def write_wav(path, rate, channels):
    frames = bytearray()
    for sample in zip(*channels):
        frames += struct.pack('<%dh' % len(sample), *sample)
    with wave.open(str(path), 'wb') as out:
        out.setnchannels(len(channels))
        out.setsampwidth(2)
        out.setframerate(rate)
        out.writeframes(bytes(frames))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('inputs', type=Path, nargs='+', help='.bwav files or directories to scan')
    parser.add_argument('--output', type=Path, help='WAV output directory (must not exist)')
    parser.add_argument('--census', action='store_true', help='only parse headers and print a summary')
    args = parser.parse_args()
    if not args.census and not args.output:
        parser.error('--output is required unless --census is given')
    files = []
    for item in args.inputs:
        files += sorted(item.rglob('*.bwav')) if item.is_dir() else [item]
    if args.output:
        if args.output.exists():
            parser.exit(1, 'output already exists; choose a new directory\n')
        args.output.mkdir(parents=True)
    roots = [i for i in args.inputs if i.is_dir()]
    census, failures, sounds = {}, [], {}
    for path in files:
        try:
            data = path.read_bytes()
            if args.census:
                parts = [parse_header_only(p) for p in split_wrapper(data)]
                info = dict(parts[0], parts=len(parts))
            else:
                info, channels = decode(data)
                base = next((r for r in roots if r in path.parents), path.parent)
                rel = path.relative_to(base)
                target = args.output / rel.with_suffix('.wav')
                target.parent.mkdir(parents=True, exist_ok=True)
                write_wav(target, info['rate'], channels)
                sounds[str(rel)] = info
        except (ValueError, struct.error, OSError) as exc:
            failures.append({'path': str(path), 'error': str(exc)})
            continue
        key = '%s %d Hz x%d' % (info['encoding'], info['rate'], info['parts'])
        census[key] = census.get(key, 0) + 1
    if args.output:
        (args.output / 'audio-manifest.json').write_text(json.dumps(sounds, indent=1) + '\n')
    print(json.dumps({'files': len(files), 'kinds': dict(sorted(census.items())),
                      'failures': failures[:20], 'failure_count': len(failures)}, indent=1))
    return 1 if failures else 0


def parse_header_only(cwav):
    magic, bom, _, _, size, _ = struct.unpack_from('<4sHHIIH', cwav, 0)
    if magic != b'CWAV' or bom != 0xfeff or size != len(cwav):
        raise ValueError("invalid CWAV header")
    info = reference(cwav, 0x14, 0x7000)
    encoding, loop, rate, loop_start, samples = struct.unpack_from('<BBxxIII', cwav, info + 8)
    if encoding not in ENCODINGS:
        raise ValueError("unsupported CWAV encoding %d" % encoding)
    return {'encoding': ENCODINGS[encoding], 'loop': bool(loop), 'rate': rate,
            'loop_start': loop_start, 'samples': samples}


if __name__ == '__main__':
    sys.exit(main())
