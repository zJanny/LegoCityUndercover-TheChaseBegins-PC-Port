import json
import struct
import sys
import tempfile
import unittest
import wave
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
sys.path.insert(0, str(Path(__file__).parents[1] / 'tools'))
import chase_native as native  # noqa: E402
from fixtures import run_json_tool  # noqa: E402

SOUND_RECORD = 0x2c


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
    """Pooled resource: -14 sound record, then one -14 block per CWAV part."""
    table = [-14, SOUND_RECORD]
    for part in parts:
        table += [-14, len(part)]
    head = struct.pack('<II', len(table) << 8, 4 * len(table)) + struct.pack('<%di' % len(table), *table)
    return head + bytes(SOUND_RECORD) + b''.join(parts)


def convert(directory, data):
    source = Path(directory) / 'in'
    source.mkdir(exist_ok=True)
    (source / 'x.bwav').write_bytes(data)
    out = Path(directory) / 'out'
    result, _ = run_json_tool('convert_audio.py', source, '--output', out)
    with wave.open(str(out / 'x.wav')) as wav:
        frames = wav.readframes(wav.getnframes())
        shape = (wav.getnchannels(), wav.getframerate())
    samples = struct.unpack('<%dh' % (len(frames) // 2), frames)
    return result, shape, samples, json.loads((out / 'audio-manifest.json').read_text(encoding='utf-8'))['x.bwav']


@unittest.skipUnless(native.available(), 'C++ asset library not built')
class AudioToolTests(unittest.TestCase):
    # Expected samples are computed by hand from the codec definitions; tests/assets_test.cpp pins the DSP values too.
    def test_dsp_adpcm_frame(self):
        # scale 1<<4, predictor 0 with c1=2048 (0.5 * hist1); nibbles 7, -8, 0
        coefs = (2048, 0) + (0,) * 14
        frame = bytes([0x04, 0x78]) + bytes(6)
        with tempfile.TemporaryDirectory() as d:
            result, shape, samples, entry = convert(d, wrapper(cwav(2, 3, frame, coefs)))
        # s0 = (112<<11 + 1024) >> 11 = 112; s1 = (-128<<11 + 1024 + 2048*112) >> 11 = -16 (floor);
        # s2 = (1024 + 2048*-16) >> 11 = -16
        self.assertEqual(result.returncode, 0)
        self.assertEqual((shape, samples), ((1, 12000), (112, -16, -16)))
        self.assertEqual(entry, {'encoding': 'DSP-ADPCM', 'loop': False, 'rate': 12000, 'loop_start': 0, 'samples': 3,
                                 'version': '02010000', 'parts': 1, 'channels': 1})

    def test_pcm_and_ima(self):
        with tempfile.TemporaryDirectory() as d:
            self.assertEqual(convert(d, wrapper(cwav(1, 2, struct.pack('<hh', 1000, -1000))))[2], (1000, -1000))
        with tempfile.TemporaryDirectory() as d:
            self.assertEqual(convert(d, wrapper(cwav(0, 2, bytes([5, 0xfb]))))[2], (5, -5))
        with tempfile.TemporaryDirectory() as d:
            # IMA: step 7, nibble 4 -> +7 (diff = 7>>3 + 7); low nibble first
            result, _, samples, entry = convert(d, wrapper(cwav(3, 2, bytes([0x04]), ima=(0, 0))))
            self.assertEqual(samples[0], 7)
            self.assertEqual(entry['encoding'], 'IMA-ADPCM')

    def test_parts_become_interleaved_channels(self):
        left, right = cwav(1, 1, struct.pack('<h', 1)), cwav(1, 1, struct.pack('<h', -1))
        with tempfile.TemporaryDirectory() as d:
            _, shape, samples, entry = convert(d, wrapper(left, right))
        self.assertEqual((shape, samples, entry['parts'], entry['channels']), ((2, 12000), (1, -1), 2, 2))

    def test_mixed_encodings_are_named(self):
        parts = (cwav(1, 1, struct.pack('<h', 1)), cwav(0, 1, bytes([2])))
        with tempfile.TemporaryDirectory() as d:
            self.assertEqual(convert(d, wrapper(*parts))[3]['encoding'], 'PCM16+PCM8')

    def test_census_and_failures(self):
        good = cwav(1, 2, struct.pack('<hh', 1, 2))
        bad_ref = bytearray(wrapper(good))
        bad_ref[8 + 16 + SOUND_RECORD + 0x15] = 0   # INFO reference type 0x7000 -> 0x0000
        cases = {'ok.bwav': wrapper(good), 'wrapper.bwav': b'\0\5\0\0' + bytes(0x60), 'cut.bwav': wrapper(good)[:-1],
                 'junk.bwav': wrapper(good) + b'junk', 'samples.bwav': wrapper(cwav(1, 9, struct.pack('<hh', 1, 2))),
                 'reference.bwav': bytes(bad_ref)}
        with tempfile.TemporaryDirectory() as d:
            source = Path(d) / 'in'
            source.mkdir()
            for name, data in cases.items():
                (source / name).write_bytes(data)
            result, summary = run_json_tool('convert_audio.py', '--census', source)
        self.assertEqual(result.returncode, 1)
        self.assertEqual((summary['failure_count'], summary['kinds']), (len(cases) - 1, {'PCM16 12000 Hz x1': 1}))


if __name__ == '__main__':
    unittest.main()
