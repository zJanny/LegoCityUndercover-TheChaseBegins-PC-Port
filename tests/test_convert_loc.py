import json
import struct
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
sys.path.insert(0, str(Path(__file__).parents[1] / 'tools'))
import chase_native as native  # noqa: E402
from fixtures import run_tool  # noqa: E402


def loc_file(entries):
    """entries: [(key, text)] -> LOCA bytes with sorted keys."""
    strings, table = b'', b''
    for key, text in sorted(entries):
        table += struct.pack('<II', key, len(strings))
        strings += text.encode('latin-1') + b'\0'
    return b'LOCA' + struct.pack('<4I', 2, len(entries), len(table), len(strings)) + table + strings


@unittest.skipUnless(native.available(), 'C++ asset library not built')
class LocToolTests(unittest.TestCase):
    def test_key_hash(self):
        # sdbm hash h = h * 65599 + c of the upper-case name, computed by hand.
        self.assertEqual(native.text_key('No'), 78 * 65599 + 79)
        self.assertEqual(native.text_key('ok'), 0x4f13bc)

    def test_tables_names_and_escapes(self):
        escapes = '\x0120\x0123 x\x019Z\x012:'   # extended characters 0 and 3, then escapes left as they are
        data = loc_file([(native.text_key('OK'), 'OK'), (native.text_key('FRENCH'), 'Fran\xe7ais'), (7, escapes)])
        with tempfile.TemporaryDirectory() as d:
            (Path(d) / 'a.loc').write_bytes(data)
            (Path(d) / 'extended.txt').write_text('abcd\n', encoding='utf-8', newline='\n')
            (Path(d) / 'code.bin').write_bytes(b'\0French\0PAUSE\0')
            result = run_tool('convert_loc.py', Path(d) / 'a.loc', '--extended', Path(d) / 'extended.txt', '--names', Path(d) / 'code.bin')
            census = run_tool('convert_loc.py', '--census', Path(d) / 'a.loc', '--names', Path(d) / 'code.bin')
        tables = json.loads(result.stdout)
        self.assertEqual(tables['a.loc'], {'00000007': 'ad x\x019Z\x012:', 'FRENCH': 'Fran\xe7ais', '004f13bc': 'OK'})
        self.assertEqual(json.loads(census.stdout)['totals'], {'files': 1, 'entries': 3, 'named': 1})

    def test_rejects_bad_tables(self):
        data = bytearray(loc_file([(1, 'a'), (2, 'b')]))
        unsorted_keys = bytearray(data)
        struct.pack_into('<I', unsorted_keys, 20, 5)   # first key above the second
        for bad, message in ((bytes(data[:-1]), 'sizes'), (bytes(unsorted_keys), 'sorted')):
            with tempfile.TemporaryDirectory() as d:
                (Path(d) / 'a.loc').write_bytes(bad)
                result = run_tool('convert_loc.py', '--census', Path(d) / 'a.loc')
            self.assertEqual(result.returncode, 1)
            self.assertIn(message, result.stdout)


if __name__ == '__main__':
    unittest.main()
