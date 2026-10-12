#!/usr/bin/env python3
"""Unpack a FUSE1.00 .fib asset archive extracted from the RomFS.

Format recovered from the game's ARM code (see docs/plans/linux-port.progress.md):
- header (20 B): "FUSE", "1.00", u32 entry count, u32 second-table count,
  u32 directory offset (open routine 0x17dea8);
- directory: count x {u32 name CRC, u32 offset, u32 (size << 5) | flags};
- flags bits 0-1 = codec, bits 2-4 = n with block size 0x8000 << n
  (entry open 0x128830, read 0x12894c);
- codec 0 = stored; 1 = chunks of {u32 length, RefPack-variant stream}
  (decoder 0x12efe0); 2 = chunks stored verbatim; 3 = other decoder 0x12d50c
  (not recovered, reported as unsupported).
"""
import argparse
import csv
import hashlib
import json
import struct
import sys
from pathlib import Path, PurePosixPath

sys.path.insert(0, str(Path(__file__).resolve().parent))
from extract_game import checked_read  # noqa: E402

MAX_ENTRY = 64 * 1024 * 1024
MAX_ENTRIES = 1 << 20


def decompress_refpack(data, limit):
    """Decode one codec-1 chunk; fail on any read or reference outside bounds."""
    out = bytearray()
    pos, end = 0, len(data)

    def take(count):
        nonlocal pos
        if pos + count > end:
            raise ValueError("truncated compressed chunk")
        chunk = data[pos:pos + count]
        pos += count
        return chunk

    def literals(count):
        out.extend(take(count))

    def match(distance, length):
        if distance > len(out):
            raise ValueError("compressed reference before chunk start")
        start = len(out) - distance
        if distance >= length:
            out.extend(out[start:start + length])
        else:
            for i in range(length):
                out.append(out[start + i])

    while True:
        if len(out) > limit:
            raise ValueError("compressed chunk exceeds block size")
        b = take(1)[0]
        if not b & 0x80:
            b1 = take(1)[0]
            literals((b >> 2) & 3)
            match(((b & 3) << 8 | b1) + 1, ((b >> 4) & 7) + 3)
        elif not b & 0x40:
            b1, b2 = take(2)
            literals(b1 >> 6)
            match(((b1 & 0x3f) << 8 | b2) + 1, (b & 0x3f) + 4)
        elif not b & 0x20:
            b1, b2, b3 = take(3)
            literals((b >> 3) & 3)
            match(((b & 1) << 16 | b1 << 8 | b2) + 1, ((b & 6) << 7) + b3 + 5)
        else:
            run = (b & 0x1f) * 4 + 4
            if run > 112:
                literals(b & 3)
                break
            literals(run)
    if len(out) > limit:
        raise ValueError("compressed chunk exceeds block size")
    if pos != end:
        raise ValueError("trailing bytes after compressed chunk")
    return bytes(out)


def read_entry(stream, file_size, offset, size, flags):
    codec, block = flags & 3, 0x8000 << ((flags >> 2) & 7)
    if size > MAX_ENTRY:
        raise ValueError("entry exceeds size cap")
    if codec == 0:
        return checked_read(stream, offset, size, file_size)
    if codec == 3:
        raise NotImplementedError("codec 3 (decoder 0x12d50c) not recovered")
    out = bytearray()
    cursor = offset
    while len(out) < size:
        length = struct.unpack('<I', checked_read(stream, cursor, 4, file_size))[0]
        chunk = checked_read(stream, cursor + 4, length, file_size)
        cursor += 4 + length
        expected = min(block, size - len(out))
        data = decompress_refpack(chunk, expected) if codec == 1 else chunk
        if len(data) != expected:
            raise ValueError("chunk decoded to %d bytes, expected %d" % (len(data), expected))
        out += data
    return bytes(out), cursor - offset


def read_directory(stream, file_size):
    header = checked_read(stream, 0, 20, file_size)
    magic, version, count, extra, table = struct.unpack('<4s4sIII', header)
    if magic != b'FUSE':
        raise ValueError("not a FUSE archive")
    if extra:
        raise ValueError("second FUSE table not supported (count %d)" % extra)
    if count > MAX_ENTRIES:
        raise ValueError("too many FUSE entries")
    raw = checked_read(stream, table, count * 12, file_size)
    entries = [struct.unpack_from('<III', raw, i * 12) for i in range(count)]
    if len({crc for crc, _, _ in entries}) != count:
        raise ValueError("duplicate FUSE name CRC")
    return version.decode('ascii', 'replace'), table, entries


def read_names(path):
    """CRC -> (name, compressed size, size, offset) from the shipped CSV index."""
    names = {}
    with open(path, newline='', encoding='utf-8') as handle:
        rows = csv.reader(handle)
        next(rows)
        for row in rows:
            crc, name, packed, size, offset = (field.strip() for field in row)
            names[int(crc, 16)] = (name, int(packed), int(size), int(offset, 16))
    return names


def safe_path(name):
    path = PurePosixPath(name.replace('\\', '/'))
    if (path.is_absolute() or not path.parts or '\0' in name
            or any(part in ('', '.', '..') for part in path.parts)):
        raise ValueError("unsafe archive name: %r" % name)
    return path


def extract(fib, output, names_csv=None):
    fib, output = Path(fib), Path(output)
    if output.exists():
        raise ValueError("output already exists; choose a new directory")
    names = read_names(names_csv) if names_csv else {}
    file_size = fib.stat().st_size
    files, unsupported, codecs, used = [], [], {}, set()
    with fib.open('rb') as stream:
        version, table, entries = read_directory(stream, file_size)
        output.mkdir(parents=True)
        for crc, offset, packed in sorted(entries, key=lambda e: e[1]):
            size, flags = packed >> 5, packed & 0x1f
            codecs[flags & 3] = codecs.get(flags & 3, 0) + 1
            known = names.get(crc)
            name = known[0] if known else 'unnamed/%08x.bin' % crc
            rel = safe_path(name)
            if str(rel).lower() in used:
                raise ValueError("duplicate archive path: %s" % rel)
            used.add(str(rel).lower())
            record = {'crc': '%08x' % crc, 'path': str(rel), 'offset': offset,
                      'size': size, 'flags': flags, 'named': bool(known)}
            if known and (known[2] != size or known[3] != offset):
                raise ValueError("CSV disagrees with directory for %s" % name)
            try:
                result = read_entry(stream, file_size, offset, size, flags)
            except NotImplementedError as exc:
                record['error'] = str(exc)
                unsupported.append(record)
                continue
            data, stored = result if isinstance(result, tuple) else (result, size)
            if known and known[1] != stored:
                raise ValueError("CSV compressed size disagrees for %s" % name)
            target = output / 'files' / rel
            target.parent.mkdir(parents=True, exist_ok=True)
            with target.open('xb') as handle:
                handle.write(data)
            record['stored_size'] = stored
            record['sha256'] = hashlib.sha256(data).hexdigest()
            files.append(record)
    manifest = {'schema_version': 1, 'archive': {'filename': fib.name, 'size': file_size,
                                                 'version': version, 'directory_offset': table},
                'entry_count': len(entries), 'extracted': len(files),
                'named': sum(r['named'] for r in files), 'codec_counts': codecs,
                'unsupported': unsupported, 'files': files}
    (output / 'fib-manifest.json').write_text(json.dumps(manifest, indent=1) + '\n')
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('archive', type=Path)
    parser.add_argument('--names', type=Path, help='CSV index shipped next to the archive')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    try:
        m = extract(args.archive, args.output, args.names)
    except (ValueError, OSError, UnicodeError, struct.error) as exc:
        parser.exit(1, 'FUSE extraction failed: ' + str(exc) + '\n')
    print('%d/%d entries extracted (%d named), codecs %s, %d unsupported -> %s' % (
        m['extracted'], m['entry_count'], m['named'], m['codec_counts'],
        len(m['unsupported']), args.output))


if __name__ == '__main__':
    main()
