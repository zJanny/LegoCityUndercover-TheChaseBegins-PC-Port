#!/usr/bin/env python3
"""Extract the decrypted RomFS of NCSD partition 0; never execute or modify the game image."""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from extract_game import checked_read, digest  # noqa: E402

IVFC_HEADER = 0x60
LEVEL3_HEADER = 0x28
NONE = 0xffffffff
CHUNK = 1 << 20
MAX_META = 64 * 1024 * 1024


def align(value, block):
    return (value + block - 1) // block * block


def locate_romfs(stream, size):
    """Return (offset, size, superblock hash, hash region) of partition 0's RomFS."""
    h = checked_read(stream, 0, 512, size)
    if h[256:260] != b'NCSD':
        raise ValueError("expected a Nintendo 3DS NCSD/CCI image")
    offset, length = (v * 512 for v in struct.unpack_from('<II', h, 0x120))
    if not length or offset < 512 or offset + length > size:
        raise ValueError("partition 0 missing or outside image")
    ncch = checked_read(stream, offset, 512, offset + length)
    if ncch[256:260] != b'NCCH':
        raise ValueError("invalid NCCH magic")
    if not ncch[0x18f] & 4:
        raise ValueError("encrypted NCCH: this tool requires decrypted content")
    rom_off, rom_units, region = struct.unpack_from('<III', ncch, 0x1b0)
    if not rom_off or not rom_units:
        raise ValueError("partition 0 has no RomFS")
    base, rom_size = offset + rom_off * 512, rom_units * 512
    if base + rom_size > offset + length or not 0 < region <= rom_units:
        raise ValueError("RomFS outside partition")
    return base, rom_size, ncch[0x1e0:0x200], region * 512


def block_hashes(data, block):
    """SHA256 of each block, zero-padding the last one."""
    for i in range(0, len(data), block):
        yield hashlib.sha256(data[i:i + block].ljust(block, b'\0')).digest()


def read_ivfc(stream, base, size):
    """Validate the IVFC tree; return (level3 offset, level3 size) relative to base."""
    header = checked_read(stream, base, IVFC_HEADER, base + size)
    magic, version, master_size = struct.unpack_from('<4sII', header, 0)
    if magic != b'IVFC' or version != 0x10000:
        raise ValueError("invalid IVFC header")
    levels = []
    for i in range(3):
        _logical, length, log2, _ = struct.unpack_from('<QQII', header, 0xc + i * 0x18)
        if not 9 <= log2 <= 20 or length <= 0:
            raise ValueError("invalid IVFC level")
        levels.append((length, 1 << log2))
    (l1s, b1), (l2s, b2), (l3s, b3) = levels
    if master_size != -(-l1s // b1) * 32 or l1s < -(-l2s // b2) * 32 or l2s < -(-l3s // b3) * 32:
        raise ValueError("IVFC hash level sizes are inconsistent")
    # Physical order is level 3, then 1, then 2; each starts on its block size.
    l3 = align(IVFC_HEADER + master_size, b3)
    l1 = align(l3 + l3s, b1)
    l2 = align(l1 + l1s, b2)
    if l2 + l2s > size or l2s > MAX_META:
        raise ValueError("IVFC levels outside RomFS")
    master = checked_read(stream, base + IVFC_HEADER, master_size, base + size)
    level1 = checked_read(stream, base + l1, l1s, base + size)
    level2 = checked_read(stream, base + l2, l2s, base + size)
    if b''.join(block_hashes(level1, b1)) != master:
        raise ValueError("IVFC level 1 SHA256 mismatch")
    if b''.join(block_hashes(level2, b2)) != level1[:-(-l2s // b2) * 32]:
        raise ValueError("IVFC level 2 SHA256 mismatch")
    stream.seek(base + l3)
    for i in range(-(-l3s // b3)):
        block = stream.read(min(b3, l3s - i * b3))
        if len(block) != min(b3, l3s - i * b3):
            raise ValueError("truncated RomFS level 3")
        if hashlib.sha256(block.ljust(b3, b'\0')).digest() != level2[i * 32:i * 32 + 32]:
            raise ValueError("IVFC level 3 SHA256 mismatch at block %d" % i)
    return l3, l3s


def safe_name(raw):
    name = raw.decode('utf-16-le')
    if (not name or name in ('.', '..') or any(c in name for c in '/\\\0')
            or len(name.encode('utf-8')) > 255):
        raise ValueError("unsafe RomFS name")
    return name


def entry(table, offset, fixed):
    """Return (fields, name bytes) of a metadata entry with a bounded name."""
    if offset % 4 or offset + fixed > len(table):
        raise ValueError("RomFS metadata entry out of bounds")
    name_len = struct.unpack_from('<I', table, offset + fixed - 4)[0]
    if name_len % 2 or offset + fixed + name_len > len(table):
        raise ValueError("RomFS name out of bounds")
    return offset, table[offset + fixed:offset + fixed + name_len]


def read_tree(stream, base, l3s):
    """Walk the level 3 directory tree; return [(relative path, data offset, size)]."""
    header = checked_read(stream, base, LEVEL3_HEADER, base + l3s)
    fields = struct.unpack('<10I', header)
    if fields[0] != LEVEL3_HEADER:
        raise ValueError("invalid RomFS level 3 header")
    tables = []
    for off, length in zip(fields[1:9:2], fields[2:9:2]):
        if off < LEVEL3_HEADER or length > MAX_META or off + length > l3s:
            raise ValueError("RomFS table outside level 3")
        tables.append(checked_read(stream, base + off, length, base + l3s))
    dirs, files, data_off = tables[1], tables[3], fields[9]
    if not LEVEL3_HEADER <= data_off <= l3s:
        raise ValueError("RomFS data outside level 3")
    root, name = entry(dirs, 0, 0x18)
    if name or struct.unpack_from('<I', dirs, 0)[0] != 0:
        raise ValueError("invalid RomFS root directory")
    result, seen_dirs, seen_files = [], set(), set()
    pending = [(0, ())]
    while pending:
        offset, path = pending.pop()
        if offset in seen_dirs:
            raise ValueError("RomFS directory cycle")
        seen_dirs.add(offset)
        _parent, _sibling, child, file_off, _hash, _len = struct.unpack_from('<6I', dirs, offset)
        names = set()
        while file_off != NONE:
            if file_off in seen_files:
                raise ValueError("RomFS file cycle")
            seen_files.add(file_off)
            _, raw = entry(files, file_off, 0x20)
            _parent, sibling, data, length, _hash, _len = struct.unpack_from('<IIQQII', files, file_off)
            name = safe_name(raw)
            if name in names:
                raise ValueError("duplicate RomFS name")
            names.add(name)
            if data_off + data + length > l3s:
                raise ValueError("RomFS file data outside level 3")
            result.append(('/'.join(path + (name,)), data_off + data, length))
            file_off = sibling
        while child != NONE:
            _, raw = entry(dirs, child, 0x18)
            name = safe_name(raw)
            if name in names:
                raise ValueError("duplicate RomFS name")
            names.add(name)
            pending.append((child, path + (name,)))
            child = struct.unpack_from('<I', dirs, child + 4)[0]
            if len(names) > len(dirs) + len(files):
                raise ValueError("RomFS sibling cycle")
    return result


def inspect_romfs(stream, base, size, superblock=None, region=0):
    """Verify the full hash chain and list files as absolute (path, offset, size)."""
    if superblock is not None:
        if hashlib.sha256(checked_read(stream, base, region, base + size)).digest() != superblock:
            raise ValueError("RomFS superblock SHA256 mismatch")
    l3, l3s = read_ivfc(stream, base, size)
    return [(p, base + l3 + o, n) for p, o, n in read_tree(stream, base + l3, l3s)]


def extract(path, output):
    path, output = Path(path), Path(output)
    if output.exists():
        raise ValueError("output already exists; choose a new directory")
    size = path.stat().st_size
    with path.open('rb') as stream:
        base, rom_size, superblock, region = locate_romfs(stream, size)
        listing = inspect_romfs(stream, base, rom_size, superblock, region)
        output.mkdir(parents=True, exist_ok=False)
        root = output / 'romfs'
        entries = []
        for rel, offset, length in sorted(listing):
            target = root / rel
            target.parent.mkdir(parents=True, exist_ok=True)
            sha = hashlib.sha256()
            stream.seek(offset)
            with target.open('xb') as out:
                remaining = length
                while remaining:
                    chunk = stream.read(min(CHUNK, remaining))
                    if not chunk:
                        raise ValueError("truncated RomFS file data")
                    sha.update(chunk)
                    out.write(chunk)
                    remaining -= len(chunk)
            entries.append({'path': rel, 'offset': offset, 'size': length, 'sha256': sha.hexdigest()})
    manifest = {'schema_version': 1, 'image': {'filename': path.name, 'size': size},
                'romfs': {'offset': base, 'size': rom_size, 'superblock_verified': True,
                          'ivfc_verified': True},
                'file_count': len(entries), 'total_bytes': sum(e['size'] for e in entries),
                'files': entries}
    (output / 'romfs-manifest.json').write_text(json.dumps(manifest, indent=1) + '\n')
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('image', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    try:
        manifest = extract(args.image, args.output)
    except (ValueError, OSError, UnicodeError, struct.error) as exc:
        parser.exit(1, 'RomFS extraction failed: ' + str(exc) + '\n')
    print('%d files, %d bytes -> %s' % (manifest['file_count'], manifest['total_bytes'], args.output))


if __name__ == '__main__':
    main()
