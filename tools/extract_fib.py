#!/usr/bin/env python3
"""Unpack a FUSE1.00 .fib asset archive extracted from the RomFS.

The archive is read by the C++ asset runtime (chase::assets::FuseArchive) through tools/chase_native.py;
build the project first. The format is described in docs/re/container-and-archive.md.
"""
import argparse
import csv
import hashlib
import json
import struct
import sys
from pathlib import Path, PurePosixPath

sys.path.insert(0, str(Path(__file__).resolve().parent))
import chase_native as native  # noqa: E402

CODEC_UNSUPPORTED = 3


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


def read_header(path):
    with open(path, 'rb') as stream:
        header = stream.read(20)
    _, version, _, _, table = struct.unpack('<4s4sIII', header)
    return version.decode('ascii', 'replace'), table


def extract(fib, output, names_csv=None):
    fib, output = Path(fib), Path(output)
    if output.exists():
        raise ValueError("output already exists; choose a new directory")
    names = read_names(names_csv) if names_csv else {}
    file_size = fib.stat().st_size
    files, unsupported, codecs, used = [], [], {}, set()
    with native.FuseArchive(fib) as archive:
        version, table = read_header(fib)
        output.mkdir(parents=True)
        for index in sorted(range(len(archive.entries)), key=lambda i: archive.entries[i][1]):
            crc, offset, size, flags = archive.entries[index]
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
            if flags & 3 == CODEC_UNSUPPORTED:
                record['error'] = "codec 3 (decoder 0x12d50c) not recovered"
                unsupported.append(record)
                continue
            data, stored = archive.read(index)
            if known and known[1] != stored:
                raise ValueError("CSV compressed size disagrees for %s" % name)
            target = output / 'files' / rel
            target.parent.mkdir(parents=True, exist_ok=True)
            with target.open('xb') as handle:
                handle.write(data)
            record['stored_size'] = stored
            record['sha256'] = hashlib.sha256(data).hexdigest()
            files.append(record)
        entry_count = len(archive.entries)
    manifest = {'schema_version': 1, 'archive': {'filename': fib.name, 'size': file_size,
                                                 'version': version, 'directory_offset': table},
                'entry_count': entry_count, 'extracted': len(files),
                'named': sum(r['named'] for r in files), 'codec_counts': codecs,
                'unsupported': unsupported, 'files': files}
    (output / 'fib-manifest.json').write_text(json.dumps(manifest, indent=1) + '\n', encoding='utf-8', newline='\n')
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('archive', type=Path)
    parser.add_argument('--names', type=Path, help='CSV index shipped next to the archive')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    try:
        m = extract(args.archive, args.output, args.names)
    except (ValueError, OSError, UnicodeError, struct.error, native.NativeUnavailable) as exc:
        parser.exit(1, 'FUSE extraction failed: ' + str(exc) + '\n')
    print('%d/%d entries extracted (%d named), codecs %s, %d unsupported -> %s' % (
        m['extracted'], m['entry_count'], m['named'], m['codec_counts'],
        len(m['unsupported']), args.output))


if __name__ == '__main__':
    main()
