import importlib.util
import json
import struct
import sys
import tempfile
import unittest
from pathlib import Path

TOOLS = Path(__file__).parents[1] / 'tools'
sys.path.insert(0, str(TOOLS))
import chase_native as native  # noqa: E402

spec = importlib.util.spec_from_file_location('extract_fib', TOOLS / 'extract_fib.py')
f = importlib.util.module_from_spec(spec)
spec.loader.exec_module(f)

# One token of each form, then the terminator with one trailing literal:
# 4-literal run, short overlapping match, medium match with a literal,
# long match at distance 1, end.
STREAM = (b'\xe0abca' + b'\x20\x02' + b'\x81\x40\x09Y' + b'\xc0\x00\x00\x00' + b'\xfdX')
DECODED = b'abcabcabc' + b'Y' + b'abcab' + b'bbbbb' + b'X'


def archive(entries, extra=0):
    """entries: [(crc, codec, size, payload)] -> FUSE bytes."""
    body, directory = bytearray(), bytearray()
    for crc, codec, size, payload in entries:
        directory += struct.pack('<III', crc, 20 + len(body), size << 5 | codec)
        body += payload
    header = struct.pack('<4s4sIII', b'FUSE', b'1.00', len(entries), extra, 20 + len(body))
    return header + body + directory


def chunk(data):
    return struct.pack('<I', len(data)) + data


@unittest.skipUnless(native.available(), 'C++ asset library not built')
class FibTests(unittest.TestCase):
    def write(self, d, entries, names=None, extra=0):
        fib = Path(d) / 'a.fib'
        fib.write_bytes(archive(entries, extra))
        csv = None
        if names is not None:
            csv = Path(d) / 'a.csv'
            csv.write_text('Filename CRC, Filename, Compressed Size, File Size, Offset\n' + ''.join(
                '0x%08x,%s,%d,%d,0x%08x\n' % row for row in names))
        return fib, csv

    def test_extract_codecs_names_and_manifest(self):
        big = bytes(range(256)) * 129  # 33024 B: two codec-2 chunks of 0x8000 blocks
        entries = [(1, 0, 3, b'raw'), (2, 1, len(DECODED), chunk(STREAM)),
                   (3, 2, len(big), chunk(big[:0x8000]) + chunk(big[0x8000:]))]
        offsets = [20, 23, 23 + 4 + len(STREAM)]
        names = [(1, 'a/raw.txt', 3, 3, offsets[0]),
                 (2, 'b/packed.bin', 4 + len(STREAM), len(DECODED), offsets[1])]
        with tempfile.TemporaryDirectory() as d:
            fib, csv = self.write(d, entries, names)
            m = f.extract(fib, Path(d) / 'out', csv)
            files = Path(d) / 'out/files'
            self.assertEqual((files / 'a/raw.txt').read_bytes(), b'raw')
            self.assertEqual((files / 'b/packed.bin').read_bytes(), DECODED)
            self.assertEqual((files / 'unnamed/00000003.bin').read_bytes(), big)
            self.assertEqual((m['extracted'], m['named']), (3, 2))
            saved = json.loads((Path(d) / 'out/fib-manifest.json').read_text())
            self.assertEqual(saved['codec_counts'], {'0': 1, '1': 1, '2': 1})
            self.assertEqual([r['stored_size'] for r in m['files']], [3, 4 + len(STREAM), 8 + len(big)])
            self.assertEqual((saved['archive']['version'], saved['archive']['directory_offset']), ('1.00', 20 + 3 + 4 + len(STREAM) + 8 + len(big)))
            with self.assertRaisesRegex(ValueError, 'already exists'):
                f.extract(fib, Path(d) / 'out', csv)

    def test_codec3_reported_not_guessed(self):
        with tempfile.TemporaryDirectory() as d:
            fib, _ = self.write(d, [(7, 3, 4, chunk(b'????'))])
            m = f.extract(fib, Path(d) / 'out')
            self.assertEqual(m['extracted'], 0)
            self.assertIn('0x12d50c', m['unsupported'][0]['error'])

    def test_rejects_unsafe_names_mismatches_and_bad_headers(self):
        cases = (
            ([(1, 0, 3, b'raw')], [(1, '../escape', 3, 3, 20)], {}, 'unsafe'),
            ([(1, 0, 3, b'raw')], [(1, 'ok.txt', 3, 3, 21)], {}, 'disagrees'),
            ([(1, 1, 99, chunk(STREAM))], None, {}, 'wrong size'),
            ([(1, 0, 3, b'raw'), (1, 0, 3, b'raw')], None, {}, 'duplicate'),
            ([(1, 0, 3, b'raw')], None, {'extra': 1}, 'second'),
        )
        for entries, names, kwargs, message in cases:
            with tempfile.TemporaryDirectory() as d:
                fib, csv = self.write(d, entries, names, **kwargs)
                with self.assertRaisesRegex(ValueError, message):
                    f.extract(fib, Path(d) / 'out', csv)


if __name__ == '__main__':
    unittest.main()
