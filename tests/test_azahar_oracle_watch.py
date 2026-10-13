import importlib.util
import struct
import tempfile
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location('azahar_oracle', Path(__file__).parents[1] / 'tools/azahar_oracle.py')
oracle = importlib.util.module_from_spec(spec)
spec.loader.exec_module(oracle)


class RecordingRemote(oracle.GdbRemote):
    def __init__(self, replies):
        self.replies = list(replies)
        self.sent = []
        self.breakpoints = set()
        self.watchpoints = set()
        self.last_stop = None

    def command(self, body):
        self.sent.append(body)
        return self.replies.pop(0)


class StopReplyTest(unittest.TestCase):
    def test_watch_stop_reply_is_split_into_signal_and_fields(self):
        signal, fields = oracle.parse_stop_reply('T050f:60cf2000;0d:10dbff0f;thread:1;watch:8568bfc;')
        self.assertEqual(signal, 5)
        self.assertEqual(fields['watch'], '8568bfc')
        self.assertEqual(fields['0f'], '60cf2000')

    def test_plain_stop_reply_has_no_fields(self):
        self.assertEqual(oracle.parse_stop_reply('S05'), (5, {}))


class WatchpointTest(unittest.TestCase):
    def test_write_watchpoint_uses_z2_with_hex_length(self):
        remote = RecordingRemote(['OK'])
        remote.add_watchpoint(0x8568bfc, 12)
        self.assertEqual(remote.sent, ['Z2,8568bfc,c'])
        self.assertEqual(remote.watchpoints, {(0x8568bfc, 2)})

    def test_remove_clears_every_watchpoint(self):
        remote = RecordingRemote(['OK'] * 4)
        remote.add_watchpoint(0x1000, 4)
        remote.add_watchpoint(0x2000, 4, kind=4)
        remote.remove_watchpoints()
        self.assertEqual(remote.sent[2:], ['z2,1000,1', 'z4,2000,1'])
        self.assertEqual(remote.watchpoints, set())

    def test_refused_watchpoint_raises(self):
        remote = RecordingRemote([''])
        with self.assertRaises(IOError):
            remote.add_watchpoint(0x1000, 4)


class FakeMemory:
    def __init__(self):
        self.words = {0x1000: 0x2000}

    def u32(self, address):
        return self.words[address]

    def read(self, address, length):
        return bytes((address + i) & 0xff for i in range(length))


class MemtraceTest(unittest.TestCase):
    def test_memtrace_writes_frame_dt_and_regions_per_frame(self):
        with tempfile.TemporaryDirectory() as directory:
            probe = object.__new__(oracle.Oracle)
            probe.output = Path(directory)
            probe.frame = 7
            probe.frame_object = 0x500
            probe.rpc = FakeMemory()
            probe.advance = lambda count, on_frame: [on_frame() for _ in range(count)]
            reads = {0x500 + 0x78: struct.pack('<f', 0.033)}
            memory = FakeMemory()
            original = memory.read
            memory.read = lambda address, length: reads.get(address, original(address, length))
            probe.rpc = memory
            probe.memtrace(2, 'demo', ['0x10:4', '*0x1000+8:2'])
            data = (Path(directory) / 'demo.mem').read_bytes()
            record = 8 + 4 + 2
            self.assertEqual(len(data), 2 * record)
            frame, dt = struct.unpack_from('<If', data, 0)
            self.assertEqual(frame, 7)
            self.assertAlmostEqual(dt, 0.033, places=6)
            self.assertEqual(data[8:12], bytes([0x10, 0x11, 0x12, 0x13]))
            self.assertEqual(data[12:14], bytes([0x08, 0x09]))


if __name__ == '__main__':
    unittest.main()
