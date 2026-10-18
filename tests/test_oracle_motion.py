import importlib.util
import json
import socket
import struct
import subprocess
import sys
import tempfile
import time
import unittest
import zlib
from pathlib import Path

TOOLS = Path(__file__).parents[1] / 'tools'


def load(name):
    spec = importlib.util.spec_from_file_location(name, TOOLS / (name + '.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


motion = load('oracle_motion')
oracle = load('azahar_oracle')


def free_port():
    probe = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    probe.bind(('127.0.0.1', 0))
    port = probe.getsockname()[1]
    probe.close()
    return port


class PacketTest(unittest.TestCase):
    def test_crc32_is_the_standard_check_value(self):
        self.assertEqual(zlib.crc32(b'123456789'), 0xCBF43926)

    def test_version_packet_layout(self):
        packet = motion.version_packet()
        magic, version, length, crc, _ = struct.unpack_from('<4sHHII', packet)
        self.assertEqual((magic, version, length, len(packet)), (b'DSUS', 1001, 6, 22))
        self.assertEqual(struct.unpack_from('<IH', packet, 16), (0x100000, 1001))
        self.assertEqual(crc, zlib.crc32(packet[:8] + bytes(4) + packet[12:]))

    def test_ports_packet_is_32_bytes_and_marks_slot_0_connected(self):
        connected, absent = motion.ports_packet(0), motion.ports_packet(1)
        self.assertEqual((len(connected), len(absent)), (32, 32))
        self.assertEqual(struct.unpack_from('<I', connected, 16)[0], 0x100001)
        self.assertEqual(connected[20:24], bytes([0, 2, 2, 2]))
        self.assertEqual(absent[20:32], bytes([1]) + bytes(11))

    def test_pad_data_packet_offsets_follow_the_protocol_table(self):
        packet = motion.pad_data_packet(7, 123456, {'accel': [0.5, -1.0, 2.0], 'gyro': [3.0, 4.0, 5.0]})
        self.assertEqual(len(packet), 100)
        self.assertEqual(struct.unpack_from('<4sHH', packet), (b'DSUS', 1001, 84))
        self.assertEqual(struct.unpack_from('<I', packet, 16)[0], 0x100002)
        self.assertEqual(packet[20], 0)
        self.assertEqual(packet[31], 1)
        self.assertEqual(struct.unpack_from('<I', packet, 32)[0], 7)
        self.assertEqual(struct.unpack_from('<Q', packet, 68)[0], 123456)
        self.assertEqual(struct.unpack_from('<6f', packet, 76), (0.5, -1.0, 2.0, 3.0, 4.0, 5.0))
        self.assertEqual(struct.unpack_from('<I', packet, 8)[0], zlib.crc32(packet[:8] + bytes(4) + packet[12:]))

    def test_level_state_puts_gravity_on_the_axis_the_game_accepts(self):
        self.assertEqual(motion.level_state(), {'accel': [0.0, 0.0, -1.0], 'gyro': [0.0, 0.0, 0.0]})


class RequestTest(unittest.TestCase):
    def test_version_and_registration_requests_are_answered(self):
        replies, registers = motion.answer(motion.build_request(0x100000))
        self.assertEqual((len(replies), registers), (1, False))
        replies, registers = motion.answer(motion.build_request(0x100002, bytes(8)))
        self.assertEqual((replies, registers), ([], True))

    def test_ports_request_gets_one_packet_per_slot(self):
        replies, registers = motion.answer(motion.build_request(0x100001, struct.pack('<i', 2) + bytes([0, 1])))
        self.assertEqual((len(replies), registers), (2, False))

    def test_malformed_requests_are_rejected(self):
        good = motion.build_request(0x100000)
        corrupt = bytearray(good)
        corrupt[-1] ^= 1
        for name, datagram, pattern in (
                ('short', good[:10], 'shorter'),
                ('magic', b'DSUS' + good[4:], 'magic'),
                ('crc', bytes(corrupt), 'CRC'),
                ('length', good + b'\0', 'length'),
                ('unknown type', motion.build_request(0x123456), 'unknown message type'),
                ('count', motion.build_request(0x100001, struct.pack('<i', 5) + bytes(5)), 'slot count'),
                ('count bytes', motion.build_request(0x100001, struct.pack('<i', 2) + bytes([0])), 'slot count'),
                ('no count', motion.build_request(0x100001), 'without a slot count')):
            with self.subTest(name), self.assertRaisesRegex(motion.MotionError, pattern):
                motion.answer(datagram)

    def test_wrong_protocol_version_is_rejected(self):
        packet = bytearray(motion.build_request(0x100000))
        struct.pack_into('<H', packet, 4, 1000)
        struct.pack_into('<I', packet, 8, 0)
        struct.pack_into('<I', packet, 8, zlib.crc32(bytes(packet)))
        with self.assertRaisesRegex(motion.MotionError, 'protocol version'):
            motion.parse_request(bytes(packet))


class TiltTest(unittest.TestCase):
    def test_accel_only_and_full_forms(self):
        self.assertEqual(motion.parse_tilt(['0', '0', '-1']), {'accel': [0.0, 0.0, -1.0], 'gyro': [0.0, 0.0, 0.0]})
        self.assertEqual(motion.parse_tilt(['0.5', '-0.5', '1', '10', '20', '30'])['gyro'], [10.0, 20.0, 30.0])

    def test_bad_tilt_arguments_are_rejected(self):
        for name, words, pattern in (
                ('none', [], 'needs 3 accel'),
                ('two', ['0', '1'], 'needs 3 accel'),
                ('four', ['0', '0', '0', '1'], 'needs 3 accel'),
                ('text', ['0', 'up', '0'], "'up' is not a number"),
                ('nan', ['0', 'nan', '0'], 'outside'),
                ('big accel', ['0', '9', '0'], 'outside'),
                ('big gyro', ['0', '0', '-1', '0', '3000', '0'], 'outside')):
            with self.subTest(name), self.assertRaisesRegex(motion.MotionError, pattern):
                motion.parse_tilt(words)

    def test_bad_state_files_are_rejected(self):
        for text in ('', '[]', '{"accel": [0, 0], "gyro": [0, 0, 0]}', '{"accel": [0, 0, 0]}',
                     '{"accel": [0, 0, "x"], "gyro": [0, 0, 0]}'):
            with self.subTest(text), self.assertRaises(motion.MotionError):
                motion.load_state(text)


class ScriptCommandTest(unittest.TestCase):
    def test_tilt_writes_the_state_file(self):
        with tempfile.TemporaryDirectory() as directory:
            state = Path(directory) / 'motion.json'
            oracle.run_scenario(None, ['tilt 0 0 -1 1 2 3  # comment'], state)
            self.assertEqual(json.loads(state.read_text()), {'accel': [0.0, 0.0, -1.0], 'gyro': [1.0, 2.0, 3.0]})

    def test_tilt_errors_name_the_line(self):
        with tempfile.TemporaryDirectory() as directory:
            state = Path(directory) / 'motion.json'
            with self.assertRaisesRegex(ValueError, r'line 2: tilt: .*needs 3 accel'):
                oracle.run_scenario(None, ['tilt 0 0 -1', 'tilt 1'], state)

    def test_tilt_without_a_motion_source_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'line 1: tilt needs the virtual motion source'):
            oracle.run_scenario(None, ['tilt 0 0 -1'])


class CommandLineTest(unittest.TestCase):
    def run_tool(self, *args):
        return subprocess.run([sys.executable, *map(str, args)], capture_output=True, text=True, timeout=30)

    def test_serve_rejects_bad_options(self):
        with tempfile.TemporaryDirectory() as directory:
            state = Path(directory) / 'motion.json'
            state.write_text(json.dumps(motion.level_state()))
            for args, message in (
                    (['--port', '80', '--state', state], '--port 80 is outside'),
                    (['--port', '70000', '--state', state], '--port 70000 is outside'),
                    (['--port', 'abc', '--state', state], 'invalid int value'),
                    (['--port', '25000', '--state', Path(directory) / 'missing.json'], 'is not a file'),
                    (['--state', state], 'required: --port'),
                    (['--port', '25000'], 'required: --state')):
                with self.subTest(args):
                    result = self.run_tool(TOOLS / 'oracle_motion.py', 'serve', *args)
                    self.assertEqual(result.returncode, 2)
                    self.assertIn(message, result.stderr)

    def test_oracle_rejects_a_bad_motion_port(self):
        for value, message in (('80', '--motion-port 80 is outside'), ('70000', '--motion-port 70000 is outside'),
                               ('x', 'invalid int value')):
            with self.subTest(value):
                result = self.run_tool(TOOLS / 'azahar_oracle.py', '--motion-port', value)
                self.assertEqual(result.returncode, 2)
                self.assertIn(message, result.stderr)


class ServerTest(unittest.TestCase):
    def test_server_registers_a_client_and_follows_the_state_file(self):
        with tempfile.TemporaryDirectory() as directory:
            state = Path(directory) / 'motion.json'
            state.write_text(json.dumps(motion.level_state()))
            port = free_port()
            server = subprocess.Popen([sys.executable, str(TOOLS / 'oracle_motion.py'), 'serve', '--port', str(port),
                                       '--state', str(state)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            client = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            client.settimeout(0.2)
            try:
                version = self.request(client, port, motion.build_request(0x100000))
                self.assertEqual(struct.unpack_from('<I', version, 16)[0], 0x100000)
                client.sendto(motion.build_request(0x100002, bytes(8)), ('127.0.0.1', port))
                self.assertEqual(struct.unpack_from('<3f', self.pad_packet(client), 76), (0.0, 0.0, -1.0))
                state.write_text(json.dumps({'accel': [1.0, 0.0, 0.0], 'gyro': [0.0, 0.0, 0.0]}))
                deadline = time.monotonic() + 5.0
                accel = None
                while time.monotonic() < deadline and accel != (1.0, 0.0, 0.0):
                    accel = struct.unpack_from('<3f', self.pad_packet(client), 76)
                self.assertEqual(accel, (1.0, 0.0, 0.0))
            finally:
                client.close()
                server.kill()
                server.wait(timeout=10)

    def request(self, client, port, datagram):
        deadline = time.monotonic() + 10.0
        while time.monotonic() < deadline:
            client.sendto(datagram, ('127.0.0.1', port))
            try:
                return client.recv(1024)
            except socket.timeout:
                continue
        self.fail('no answer from the motion source')

    def pad_packet(self, client):
        while True:
            packet = client.recv(1024)
            if len(packet) == 100:
                return packet


if __name__ == '__main__':
    unittest.main()
