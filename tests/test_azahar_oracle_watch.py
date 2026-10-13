import importlib.util
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


if __name__ == '__main__':
    unittest.main()
