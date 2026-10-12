import importlib.util
import struct
import sys
import unittest
from pathlib import Path

TOOLS = Path(__file__).parents[1] / 'tools'
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location('convert_loc', TOOLS / 'convert_loc.py')
cl = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cl)


def loc_file(entries):
    """entries: [(key, text)] -> LOCA bytes with sorted keys."""
    strings, table = b'', b''
    for key, text in sorted(entries):
        table += struct.pack('<II', key, len(strings))
        strings += text.encode('latin-1') + b'\0'
    return b'LOCA' + struct.pack('<4I', 2, len(entries), len(table), len(strings)) + table + strings


class LocTests(unittest.TestCase):
    def test_key_hash(self):
        self.assertEqual(cl.key_hash('No'), 78 * 65599 + 79)
        self.assertEqual(cl.key_hash('ok'), 0x4f13bc)

    def test_parse(self):
        data = loc_file([(cl.key_hash('OK'), 'OK'), (cl.key_hash('FRENCH'), 'Fran\xe7ais')])
        entries = cl.parse_loc(data)
        self.assertEqual(entries[cl.key_hash('FRENCH')], 'Fran\xe7ais')

    def test_decode_text(self):
        self.assertEqual(cl.decode_text('\x0120\x0123 x\x019Z\x012:', 'abcd'), 'ad x\x019Z\x012:')

    def test_rejects_bad_tables(self):
        data = bytearray(loc_file([(1, 'a'), (2, 'b')]))
        with self.assertRaisesRegex(ValueError, 'sizes'):
            cl.parse_loc(bytes(data[:-1]))
        struct.pack_into('<I', data, 20, 5)          # first key above the second
        with self.assertRaisesRegex(ValueError, 'sorted'):
            cl.parse_loc(bytes(data))


if __name__ == '__main__':
    unittest.main()
