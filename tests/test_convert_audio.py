import importlib.util
import struct
import tempfile
import unittest
import wave
from pathlib import Path

spec = importlib.util.spec_from_file_location('convert_audio', Path(__file__).parents[1] / 'tools/convert_audio.py')
a = importlib.util.module_from_spec(spec)
spec.loader.exec_module(a)


def cwav(encoding, samples, payload, coefs=(0,) * 16, ima=(0, 0)):
    """Minimal one-channel CWAV laid out like the shipped files."""
    info = bytearray(b'INFO' + bytes(4))
    info += struct.pack('<BBxxIII', encoding, 0, 12000, 0, samples) + bytes(4)
    table = len(info)
    info += struct.pack('<I', 1) + struct.pack('<HxxI', 0x7100, 0xc)
    channel = len(info)
    info += struct.pack('<HxxI', 0x1f00, 0x18) + struct.pack('<HxxI', 0x0300 if encoding != 3 else 0x0301, 0x14)
    info += bytes(4)
    assert len(info) - channel == 0x14 and channel - table == 0xc
    if encoding == 3:
        info += struct.pack('<hBx', *ima)
    else:
        info += struct.pack('<16h', *coefs) + bytes(16)
    info += bytes(-len(info) % 32)
    struct.pack_into('<I', info, 4, len(info))
    data = b'DATA' + struct.pack('<I', 0x20 + len(payload)) + bytes(0x18) + payload
    total = 0x40 + len(info) + len(data)
    header = struct.pack('<4sHHIIHxx', b'CWAV', 0xfeff, 0x40, 0x02010000, total, 2)
    header += struct.pack('<HxxII', 0x7000, 0x40, len(info)) + struct.pack('<HxxII', 0x7001, 0x40 + len(info), len(data))
    return header + bytes(0x40 - len(header)) + bytes(info) + data


def wrapper(*parts):
    version = 0x400 if len(parts) == 1 else 0x600
    return struct.pack('<I', version) + bytes(a.CWAV_START[version] - 4) + b''.join(parts)


class AudioTests(unittest.TestCase):
    def test_dsp_adpcm_frame(self):
        # scale 1<<4, predictor 0 with c1=2048 (0.5 * hist1); nibbles 7, -8, 0
        coefs = (2048, 0) + (0,) * 14
        frame = bytes([0x04, 0x78]) + bytes(6)
        info, channels = a.decode(wrapper(cwav(2, 3, frame, coefs)))
        # s0 = (112<<11 + 1024) >> 11 = 112; s1 = (-128<<11 + 1024 + 2048*112) >> 11 = -16 (floor);
        # s2 = (1024 + 2048*-16) >> 11 = -16
        self.assertEqual(channels, [[112, -16, -16]])
        self.assertEqual((info['encoding'], info['rate'], info['channels']), ('DSP-ADPCM', 12000, 1))

    def test_pcm_and_ima(self):
        _, channels = a.decode(wrapper(cwav(1, 2, struct.pack('<hh', 1000, -1000))))
        self.assertEqual(channels, [[1000, -1000]])
        _, channels = a.decode(wrapper(cwav(0, 2, bytes([5, 0xfb]))))
        self.assertEqual(channels, [[5, -5]])
        # IMA: step 7, nibble 4 -> +7+0 (diff = 7>>3 + 7) = +7; low nibble first
        _, channels = a.decode(wrapper(cwav(3, 2, bytes([0x04]), ima=(0, 0))))
        self.assertEqual(channels[0][0], 7)

    def test_v6_parts_become_channels(self):
        left, right = cwav(1, 1, struct.pack('<h', 1)), cwav(1, 1, struct.pack('<h', -1))
        info, channels = a.decode(wrapper(left, right))
        self.assertEqual((info['parts'], channels), (2, [[1], [-1]]))
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / 'x.wav'
            a.write_wav(path, info['rate'], channels)
            with wave.open(str(path)) as w:
                self.assertEqual((w.getnchannels(), w.getframerate(), w.readframes(1)),
                                 (2, 12000, struct.pack('<hh', 1, -1)))

    def test_rejects_malformed(self):
        good = cwav(1, 2, struct.pack('<hh', 1, 2))
        cases = (b'\0\5\0\0' + bytes(0x60),                      # unknown wrapper version
                 wrapper(good)[:-1],                            # truncated CWAV
                 wrapper(good) + b'junk',                       # trailing data that is not a CWAV
                 wrapper(cwav(1, 9, struct.pack('<hh', 1, 2)))) # samples beyond data
        for data in cases:
            with self.assertRaises(ValueError):
                a.decode(data)
        bad_ref = bytearray(wrapper(good))
        bad_ref[a.CWAV_START[0x400] + 0x15] = 0  # INFO reference type 0x7000 -> 0x0000
        with self.assertRaisesRegex(ValueError, 'reference type'):
            a.decode(bytes(bad_ref))


if __name__ == '__main__':
    unittest.main()
