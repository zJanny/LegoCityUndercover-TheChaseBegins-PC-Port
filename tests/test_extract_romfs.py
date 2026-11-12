import hashlib
import importlib.util
import io
import json
import struct
import tempfile
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location('extract_romfs', Path(__file__).parents[1] / 'tools/extract_romfs.py')
r = importlib.util.module_from_spec(spec)
spec.loader.exec_module(r)

NONE = 0xffffffff
BLOCK = 512


def utf16(name):
    raw = name.encode('utf-16-le')
    return raw, raw + bytes(-len(raw) % 4)


def level3(sub='sub', first='a.txt', mutate=None):
    """Root holds `first` and directory `sub`, which holds b.bin."""
    a, b = b'hello', b'xyz'
    sub_raw, sub_pad = utf16(sub)
    a_raw, a_pad = utf16(first)
    b_raw, b_pad = utf16('b.bin')
    dirs = struct.pack('<6I', 0, NONE, 0x18, 0, NONE, 0)
    dirs += struct.pack('<6I', 0, NONE, NONE, 0x20 + len(a_pad), NONE, len(sub_raw)) + sub_pad
    files = struct.pack('<IIQQII', 0, NONE, 0, len(a), NONE, len(a_raw)) + a_pad
    files += struct.pack('<IIQQII', 0x18, NONE, 8, len(b), NONE, len(b_raw)) + b_pad
    tables = [bytes(4), dirs, bytes(4), files]
    offsets, cursor = [], 0x28
    for t in tables:
        offsets += [cursor, len(t)]
        cursor += len(t)
    data_off = (cursor + 15) & ~15
    body = struct.pack('<10I', 0x28, *offsets, data_off) + b''.join(tables)
    body = bytearray(body.ljust(data_off, b'\0') + a.ljust(8, b'\0') + b)
    if mutate:
        mutate(body)
    return bytes(body)


def hashes(data):
    return b''.join(hashlib.sha256(data[i:i + BLOCK].ljust(BLOCK, b'\0')).digest()
                    for i in range(0, len(data), BLOCK))


def romfs(l3):
    l2 = hashes(l3)
    l1 = hashes(l2)
    master = hashes(l1)
    header = bytearray(0x60)
    struct.pack_into('<4sII', header, 0, b'IVFC', 0x10000, len(master))
    for i, data in enumerate((l1, l2, l3)):
        struct.pack_into('<QQII', header, 0xc + i * 0x18, 0, len(data), 9, 0)
    out = bytearray(header + master)
    for data in (l3, l1, l2):
        out += bytes(-len(out) % BLOCK) + data
    return bytes(out + bytes(-len(out) % BLOCK))


def image(rom):
    data = bytearray(0x400)
    data[0x100:0x104] = b'NCSD'
    units = 1 + len(rom) // 512
    struct.pack_into('<II', data, 0x120, 1, units)
    ncch = bytearray(0x200)
    ncch[0x100:0x104] = b'NCCH'
    ncch[0x18f] = 4
    struct.pack_into('<III', ncch, 0x1b0, 1, len(rom) // 512, 1)
    ncch[0x1e0:0x200] = hashlib.sha256(rom[:512]).digest()
    data[0x200:0x400] = ncch
    return bytearray(data + rom)


class RomfsTests(unittest.TestCase):
    def listing(self, rom):
        return r.inspect_romfs(io.BytesIO(rom), 0, len(rom))

    def test_tree_and_offsets(self):
        rom = romfs(level3())
        found = {p: rom[o:o + n] for p, o, n in self.listing(rom)}
        self.assertEqual(found, {'a.txt': b'hello', 'sub/b.bin': b'xyz'})

    def test_hash_levels_rejected(self):
        rom = bytearray(romfs(level3()))
        l3 = 0x200  # header + master hash fit in the first block
        for offset, level in ((l3 + 0x30, 'level 3'), (len(rom) - BLOCK + 1, 'level 2')):
            bad = bytearray(rom); bad[offset] ^= 1
            with self.assertRaisesRegex(ValueError, level):
                self.listing(bytes(bad))

    def test_unsafe_and_duplicate_names(self):
        for kwargs, message in (({'sub': '..'}, 'unsafe'), ({'first': 'a/b'}, 'unsafe'),
                                ({'first': 'sub'}, 'duplicate')):
            with self.assertRaisesRegex(ValueError, message):
                self.listing(romfs(level3(**kwargs)))

    def test_directory_cycle_and_out_of_bounds(self):
        def cycle(body):  # sub's first child is sub itself
            struct.pack_into('<I', body, 0x2c + 0x18 + 8, 0x18)

        def oob(body):  # a.txt size runs past level 3
            struct.pack_into('<Q', body, 0x2c + 0x38 + 4 + 16, 1 << 20)
        for mutate, message in ((cycle, 'cycle'), (oob, 'outside')):
            with self.assertRaisesRegex(ValueError, message):
                self.listing(romfs(level3(mutate=mutate)))

    def test_extract_image_and_refuse_existing_output(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / 'test.cci'
            p.write_bytes(image(romfs(level3())))
            m = r.extract(p, Path(d) / 'out')
            self.assertEqual((Path(d) / 'out/romfs/sub/b.bin').read_bytes(), b'xyz')
            self.assertEqual(m['file_count'], 2)
            saved = json.loads((Path(d) / 'out/romfs-manifest.json').read_text(encoding='utf-8'))
            self.assertEqual(saved['files'][0]['sha256'], hashlib.sha256(b'hello').hexdigest())
            with self.assertRaisesRegex(ValueError, 'already exists'):
                r.extract(p, Path(d) / 'out')

    def test_superblock_and_encryption(self):
        with tempfile.TemporaryDirectory() as d:
            for offset, message in ((0x200 + 0x1e0, 'superblock'), (0x200 + 0x18f, 'encrypted')):
                bad = image(romfs(level3())); bad[offset] ^= 4
                p = Path(d) / 'bad.cci'; p.write_bytes(bad)
                with self.assertRaisesRegex(ValueError, message):
                    r.extract(p, Path(d) / ('out' + message))


if __name__ == '__main__':
    unittest.main()
