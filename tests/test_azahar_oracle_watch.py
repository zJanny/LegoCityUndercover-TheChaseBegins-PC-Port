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


class TeleportTest(unittest.TestCase):
    def make_probe(self, world, local):
        probe = object.__new__(oracle.Oracle)
        probe.writes = []
        probe.player = lambda: {'node': 0x8568bb0, 'world': world, 'local': local}
        probe.rpc = None
        probe.remote = type('Memory', (), {'write': lambda self, address, data: probe.writes.append((address, data))})()
        return probe

    def test_teleport_writes_world_and_local_translation(self):
        probe = self.make_probe((1.0, 2.0, 3.0), (1.0, 2.0, 3.0))
        probe.teleport(886.5, 40.0, 1111.25)
        position = struct.pack('<3f', 886.5, 40.0, 1111.25)
        self.assertEqual(probe.writes, [(0x8568bb0 + 0x8c, position), (0x8568bb0 + 0x4c, position)])

    def test_teleport_refuses_a_node_with_a_parent_transform(self):
        probe = self.make_probe((1.0, 2.0, 3.0), (0.0, 2.0, 3.0))
        with self.assertRaises(RuntimeError):
            probe.teleport(0, 0, 0)
        self.assertEqual(probe.writes, [])


class FreeTrapRemote:
    def __init__(self, stops):
        self.stops = list(stops)
        self.breakpoints = {0x100f44}
        self.sent = []
        self.registers = None

    def command(self, body):
        self.sent.append(body)
        if body == 'g':
            return ''.join(struct.pack('<I', v).hex() for v in self.registers)
        return 'OK'

    def register(self, index):
        return 0

    def send(self, body):
        self.sent.append(body)

    def wait_stop(self, deadline):
        if not self.stops:
            return None
        self.registers = self.stops.pop(0)
        return 'T05'

    def read(self, address, length):
        return bytes(length)


class FreeTrapTest(unittest.TestCase):
    def test_free_trap_records_each_stop_and_restores_breakpoints(self):
        with tempfile.TemporaryDirectory() as directory:
            probe = object.__new__(oracle.Oracle)
            probe.output = Path(directory)
            stop = [0x0870f6f0, 0x08618d7c, 0, 0] + [0] * 10 + [0x17b040, 0x22ce9c]
            probe.remote = FreeTrapRemote([stop])
            probe.free_trap(5.0, [0x22ce9c], 'demo')
            text = (Path(directory) / 'demo.txt').read_text()
            self.assertIn('pc 0x0022ce9c', text)
            self.assertIn('[r1] 00000000', text)
            sent = probe.remote.sent
            self.assertEqual(sent[:2], ['z0,100f44,4', 'Z0,22ce9c,4'])
            self.assertEqual(sent[-2:], ['z0,22ce9c,4', 'Z0,100f44,4'])
            self.assertIn('s', sent)

    def test_malformed_freetrap_lines_are_rejected(self):
        probe = type('Probe', (), {'free_trap': lambda self, *args: self.calls.append(args)})()
        probe.calls = []
        cases = (('freetrap 10 0x22ce9c', '10 0x22ce9c'), ('freetrap 10 0x22ce9c demo extra', 'demo extra'),
                 ('freetrap 0 0x22ce9c demo', "'0'"), ('freetrap -1 0x22ce9c demo', "'-1'"), ('freetrap ten 0x22ce9c demo', "'ten'"),
                 ('freetrap inf 0x22ce9c demo', "'inf'"), ('freetrap nan 0x22ce9c demo', "'nan'"),
                 ('freetrap 10 0x22zz demo', "'0x22zz'"), ('freetrap 10 0x22ce9c,,0x2388e4 demo', "''"),
                 ('freetrap 10 0x22ce9e demo', "'0x22ce9e'"), ('freetrap 10 0x1000 demo', "'0x1000'"),
                 ('freetrap 10 -0x22ce9c demo', "'-0x22ce9c'"), ('freetrap 10 0x22ce9c ../demo', "'../demo'"))
        for line, bad in cases:
            with self.assertRaises(ValueError, msg=line) as caught:
                oracle.run_scenario(probe, [line])
            self.assertIn('freetrap', str(caught.exception))
            self.assertIn(bad, str(caught.exception))
        self.assertEqual(probe.calls, [])
        oracle.run_scenario(probe, ['freetrap 2.5 0x22ce9c,0x2388e4 demo'])
        self.assertEqual(probe.calls, [(2.5, [0x22ce9c, 0x2388e4], 'demo')])


if __name__ == '__main__':
    unittest.main()
