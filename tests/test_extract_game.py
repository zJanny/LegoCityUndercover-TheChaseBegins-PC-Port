import importlib.util
import struct
import tempfile
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location('extract_game', Path(__file__).parents[1] / 'tools/extract_game.py')
x = importlib.util.module_from_spec(spec)
spec.loader.exec_module(x)


def fixture():
    image = bytearray(0x6000)
    image[0x100:0x104] = b'NCSD'
    struct.pack_into('<II', image, 0x120, 8, 40)
    base = 0x1000
    image[base+0x100:base+0x104] = b'NCCH'
    struct.pack_into('<I', image, base+0x104, 40)
    struct.pack_into('<I', image, base+0x180, 1024)
    image[base+0x18f] = 4
    ex = bytearray(1024)
    ex[:8] = b'TESTGAME'
    for off, address in zip((0x10,0x20,0x30), (0x100000,0x101000,0x102000)):
        struct.pack_into('<III', ex, off, address, 1, 4)
    image[base+512:base+1536] = ex
    image[base+0x160:base+0x180] = bytes.fromhex(x.digest(ex))
    struct.pack_into('<II', image, base+0x1a0, 3, 25)
    ebase = base+1536
    code = b'ABCD' + bytes(3*4096-4)
    struct.pack_into('<8sII', image, ebase, b'.code', 0, len(code))
    image[ebase+0x1e0:ebase+0x200] = bytes.fromhex(x.digest(code))
    image[ebase+512:ebase+512+len(code)] = code
    return image


class ExtractTests(unittest.TestCase):
    def inspect(self, data):
        with tempfile.TemporaryDirectory() as directory:
            p = Path(directory) / 'test.cci'
            p.write_bytes(data)
            return x.inspect_image(p)

    def test_valid_image_layout_hash_and_elf(self):
        m, blobs, ex, code, elf = self.inspect(fixture())
        self.assertTrue(m['exheader']['hash_verified'])
        self.assertEqual(blobs['.code'], code)
        self.assertEqual(elf[:4], b'\x7fELF')
        self.assertEqual(struct.unpack_from('<H', elf, 18)[0], 40)
        self.assertEqual(struct.unpack_from('<I', elf, 24)[0], 0x100000)
        self.assertEqual(elf[4096:], code)

    def test_truncated_and_bad_partition(self):
        for data in (fixture()[:300], fixture()[:5000]):
            with self.assertRaises(ValueError):
                self.inspect(data)

    def test_encrypted(self):
        data = fixture(); data[0x118f] = 0
        with self.assertRaisesRegex(ValueError, 'encrypted'):
            self.inspect(data)

    def test_hash_corruption(self):
        for offset in (0x1200, 0x1800):
            data = fixture(); data[offset] ^= 1
            with self.assertRaisesRegex(ValueError, 'SHA256'):
                self.inspect(data)

    def test_unsafe_name(self):
        data = fixture(); data[0x1600:0x1608] = b'../bad\0\0'
        with self.assertRaisesRegex(ValueError, 'unsafe'):
            self.inspect(data)

    def test_existing_output_not_overwritten(self):
        with tempfile.TemporaryDirectory() as d:
            output = Path(d) / 'out'; output.mkdir()
            marker = output / 'keep'; marker.write_text('preserve')
            with self.assertRaisesRegex(ValueError, 'already exists'):
                x.extract(Path(d) / 'missing', output)
            self.assertEqual(marker.read_text(), 'preserve')

    def test_blz_literal_and_overlapping_reference(self):
        # Three backwards literals then one 18-byte reference make 21 A bytes.
        stream = b'\0\xf0AAA\x10'
        packed = stream + struct.pack('<II', (8 << 24) | 14, 7)
        self.assertEqual(x.decompress_blz(packed), b'A' * 21)

    def test_blz_rejects_invalid_sizes_and_reference(self):
        samples = (b'', bytes(8), b'\0\xf0\x80'+struct.pack('<II',(8<<24)|11,10),
                   b'A\0'+struct.pack('<II',(8<<24)|10,0xffffffff))
        for sample in samples:
            with self.assertRaises(ValueError):
                x.decompress_blz(sample)

    def test_elf_rejects_size_and_overlap(self):
        segments = [{'address': 0x100000,'pages':1,'size':1}] * 3
        with self.assertRaisesRegex(ValueError, 'layout'):
            x.make_elf(bytes(10),segments,0)
        with self.assertRaisesRegex(ValueError, 'overlapping'):
            x.make_elf(bytes(12288),segments,0)


if __name__ == '__main__':
    unittest.main()
