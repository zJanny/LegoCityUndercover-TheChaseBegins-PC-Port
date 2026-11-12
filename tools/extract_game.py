#!/usr/bin/env python3
"""Extract a decrypted NCSD executable; never execute or modify the game image."""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path

MAX_CODE = 64 * 1024 * 1024


def digest(data):
    return hashlib.sha256(data).hexdigest()


def checked_read(stream, offset, size, limit):
    if offset < 0 or size < 0 or offset + size > limit:
        raise ValueError("range outside container")
    stream.seek(offset)
    data = stream.read(size)
    if len(data) != size:
        raise ValueError("truncated container")
    return data


def decompress_blz(data):
    """Backwards LZ used by ExeFS .code, with strict bounds and output cap."""
    if len(data) < 8:
        raise ValueError("truncated BLZ footer")
    packed, extra = struct.unpack_from('<II', data, len(data) - 8)
    encoded, footer = packed & 0xffffff, packed >> 24
    total = len(data) + extra
    if not 8 <= footer <= encoded <= len(data) or total > MAX_CODE:
        raise ValueError("invalid BLZ sizes")
    stop = len(data) - encoded
    cursor = len(data) - footer
    output = bytearray(total)
    output[:len(data)] = data
    out = total
    while cursor > stop:
        cursor -= 1
        control = data[cursor]
        for bit in range(8):
            if cursor <= stop:
                break
            if control & (0x80 >> bit):
                if cursor - 2 < stop:
                    raise ValueError("truncated BLZ reference")
                cursor -= 2
                token = struct.unpack_from('<H', data, cursor)[0]
                count, distance = (token >> 12) + 3, (token & 0xfff) + 2
                if out - count < stop:
                    raise ValueError("BLZ output underflow")
                for _ in range(count):
                    source = out + distance
                    if source >= total:
                        raise ValueError("BLZ reference outside decoded suffix")
                    out -= 1
                    output[out] = output[source]
            else:
                if out <= stop:
                    raise ValueError("BLZ literal underflow")
                cursor -= 1
                out -= 1
                output[out] = data[cursor]
    if out != stop:
        raise ValueError("incomplete BLZ stream")
    return bytes(output)


def make_elf(code, segments, bss):
    """Synthetic ELF32 ARM analysis carrier. Segment layout comes from exheader."""
    if sum(s['pages'] * 4096 for s in segments) != len(code):
        raise ValueError("decoded .code does not match declared page layout")
    if bss > MAX_CODE or len(code) + bss > MAX_CODE:
        raise ValueError("BSS exceeds analysis limit")
    ident = b'\x7fELF' + bytes([1, 1, 1, 0]) + bytes(8)
    header = struct.pack('<16sHHIIIIIHHHHHH', ident, 2, 40, 1,
                         segments[0]['address'], 52, 0, 0x05000000,
                         52, 32, 3, 0, 0, 0)
    ph = bytearray()
    cursor = 0
    previous_end = 0
    for i, segment in enumerate(segments):
        size = segment['pages'] * 4096
        memory = size + (bss if i == 2 else 0)
        address = segment['address']
        if segment['size'] > size or address % 4096 or address < previous_end or address + memory > 2**32:
            raise ValueError("invalid or overlapping ARM segments")
        ph.extend(struct.pack('<IIIIIIII', 1, 4096 + cursor, address, address,
                              size, memory, (5, 4, 6)[i], 4096))
        cursor += size
        previous_end = address + memory
    return header + ph + bytes(4096 - len(header) - len(ph)) + code


def inspect_image(path):
    path = Path(path)
    size = path.stat().st_size
    with path.open('rb') as stream:
        h = checked_read(stream, 0, 512, size)
        if h[256:260] != b'NCSD':
            raise ValueError("expected a Nintendo 3DS NCSD/CCI image")
        partitions = []
        for i in range(8):
            offset, length = struct.unpack_from('<II', h, 0x120 + i * 8)
            if not length:
                continue
            offset, length = offset * 512, length * 512
            if offset < 512 or offset + length > size or length < 512:
                raise ValueError("partition outside image")
            if any(offset < p['offset'] + p['size'] and p['offset'] < offset + length for p in partitions):
                raise ValueError("overlapping partitions")
            partitions.append({'index': i, 'offset': offset, 'size': length})
        main = next((p for p in partitions if p['index'] == 0), None)
        if main is None:
            raise ValueError("missing executable partition 0")
        base, end = main['offset'], main['offset'] + main['size']
        ncch = checked_read(stream, base, 512, end)
        if ncch[256:260] != b'NCCH':
            raise ValueError("invalid NCCH magic")
        if struct.unpack_from('<I', ncch, 0x104)[0] * 512 != main['size']:
            raise ValueError("NCCH size differs from partition table")
        if not ncch[0x18f] & 4:
            raise ValueError("encrypted NCCH: this tool requires decrypted content")
        ex_size = struct.unpack_from('<I', ncch, 0x180)[0]
        if ex_size != 1024:
            raise ValueError("expected 0x400-byte executable extended header")
        ex = checked_read(stream, base + 512, ex_size, end)
        if hashlib.sha256(ex).digest() != ncch[0x160:0x180]:
            raise ValueError("extended-header SHA256 mismatch")
        ex_off, ex_units = struct.unpack_from('<II', ncch, 0x1a0)
        ex_base, ex_len = base + ex_off * 512, ex_units * 512
        if ex_off < 3 or ex_len < 512 or ex_base + ex_len > end:
            raise ValueError("invalid ExeFS bounds")
        table = checked_read(stream, ex_base, 512, end)
        entries, blobs, ranges = [], {}, []
        for i in range(10):
            raw_name, off, length = struct.unpack_from('<8sII', table, i * 16)
            if raw_name == bytes(8):
                continue
            name = raw_name.rstrip(b'\0').decode('ascii')
            if not re.fullmatch(r'[.A-Za-z0-9_-]{1,8}', name) or name in ('.', '..') or name in blobs:
                raise ValueError("unsafe or duplicate ExeFS name")
            if length > MAX_CODE or 512 + off + length > ex_len:
                raise ValueError("ExeFS member exceeds bounds")
            if any(off < b and a < off + length for a, b in ranges):
                raise ValueError("overlapping ExeFS members")
            ranges.append((off, off + length))
            data = checked_read(stream, ex_base + 512 + off, length, end)
            expected = table[0xc0 + (9-i)*32:0xc0 + (10-i)*32]
            if hashlib.sha256(data).digest() != expected:
                raise ValueError("ExeFS SHA256 mismatch: " + name)
            blobs[name] = data
            entries.append({'name': name, 'offset': ex_base + 512 + off,
                            'size': length, 'sha256': digest(data), 'hash_verified': True})
        if '.code' not in blobs:
            raise ValueError("missing .code")
        segments = []
        for name, off in zip(('text', 'rodata', 'data'), (0x10, 0x20, 0x30)):
            address, pages, actual = struct.unpack_from('<III', ex, off)
            segments.append({'name': name, 'address': address, 'pages': pages, 'size': actual})
        compressed = bool(ex[0xd] & 1)
        code = decompress_blz(blobs['.code']) if compressed else blobs['.code']
        bss = struct.unpack_from('<I', ex, 0x3c)[0]
        elf = make_elf(code, segments, bss)
        rom_off, rom_units = struct.unpack_from('<II', ncch, 0x1b0)
        if rom_off and (base + (rom_off + rom_units) * 512 > end):
            raise ValueError("RomFS outside partition")
        # Full source identity is streamed, without loading a 1 GiB image into RAM.
        stream.seek(0)
        image_hash = hashlib.file_digest(stream, 'sha256').hexdigest()
    manifest = {'schema_version': 1, 'image': {'filename': path.name, 'size': size, 'sha256': image_hash},
                'partitions': partitions, 'product_code': ncch[0x150:0x160].rstrip(b'\0').decode('ascii'),
                'program_name': ex[:8].rstrip(b'\0').decode('ascii'), 'no_crypto': True,
                'exheader': {'offset': base+512, 'size': ex_size, 'sha256': digest(ex), 'hash_verified': True},
                'exefs': entries, 'compressed_code': compressed,
                'arm_segments': segments, 'bss_size': bss,
                'romfs': {'offset': base+rom_off*512, 'size': rom_units*512},
                'decoded_code': {'size': len(code), 'sha256': digest(code)},
                'analysis_elf': {'size': len(elf), 'sha256': digest(elf),
                                 'limitations': 'Synthetic ARM ELF wrapper, inferred entry at text base; not an original ELF, recovered source, or runnable PC port.'}}
    return manifest, blobs, ex, code, elf


def extract(path, output):
    output = Path(output)
    if output.exists():
        raise ValueError("output already exists; choose a new directory")
    manifest, blobs, ex, code, elf = inspect_image(path)
    output.mkdir(parents=True, exist_ok=False)
    (output / 'exefs').mkdir()
    for name, data in blobs.items():
        (output / 'exefs' / ('code.compressed.bin' if name == '.code' else name + '.bin')).write_bytes(data)
    (output / 'exheader.bin').write_bytes(ex)
    (output / 'code.bin').write_bytes(code)
    (output / 'game.arm.elf').write_bytes(elf)
    (output / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8', newline='\n')
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('image', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    try:
        manifest = extract(args.image, args.output)
    except (ValueError, OSError, UnicodeError, struct.error) as exc:
        parser.exit(1, 'Extraction failed: ' + str(exc) + '\n')
    print(json.dumps(manifest, indent=2))


if __name__ == '__main__':
    main()
