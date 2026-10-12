import importlib.util
import struct
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location('tt_resource', Path(__file__).parents[1] / 'tools/tt_resource.py')
r = importlib.util.module_from_spec(spec)
spec.loader.exec_module(r)


def resource(table, static=b'', body=b''):
    head = struct.pack('<II', len(table) << 8, 4 * len(table) + len(static))
    return head + struct.pack('<%di' % len(table), *table) + static + body


class PoolTests(unittest.TestCase):
    def test_layout_like_room60(self):
        # -16 blocks live in the header; -15 then -14 data follow in table order.
        data = resource([-16, 0, 4, -15, 2, 0, 3, -14, 1, -14, 2], b'SSSS', b'AABBBCDD')
        info = r.parse(data)
        self.assertEqual(info['table_words'], 11)
        got = [(b['pool'], data[b['offset']:b['offset'] + b['size']]) for b in info['blocks']]
        self.assertEqual(got, [('m16', b''), ('m16', b'SSSS'), ('m15', b'AA'), ('m15', b''),
                               ('m15', b'BBB'), ('m14', b'C'), ('m14', b'DD')])

    def test_texture_shape(self):
        info = r.parse(resource([-14, 0x20, -14, 4], body=bytes(0x24)))
        self.assertEqual([b['offset'] for b in info['blocks']], [0x18, 0x38])

    def test_rejections(self):
        good = resource([-14, 2], body=b'xx')
        cases = (good[:6], good + b'!', good[:-1],
                 resource([5, -14], body=bytes(5)),                  # size before marker
                 resource([-14, -3]),                                # negative size
                 b'\x01' + good[1:],                                 # low byte of word count set
                 resource([-16, 4], b'SS'))                          # static data short
        for data in cases:
            with self.assertRaises(ValueError):
                r.parse(data)


if __name__ == '__main__':
    unittest.main()
