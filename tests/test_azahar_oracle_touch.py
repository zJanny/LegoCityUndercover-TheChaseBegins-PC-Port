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
from unittest import mock

TOOLS = Path(__file__).parents[1] / 'tools'


def load(name):
    spec = importlib.util.spec_from_file_location(name, TOOLS / (name + '.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


motion = load('oracle_motion')
oracle = load('azahar_oracle')

# Measured by 20261025-mission13-D (state gameplay-start, Old 3DS, OpenGL, no RPC; the script step touch/drag of this
# branch; memory read of the HID shared memory at 0x10002000, touch ring at +0xa8, entries at +0x20 of u16 x, u16 y,
# u32 valid, docs/re/oracle-setup.md): the pixel sent and the entry the game's HID memory then held.
MEASURED_HID_ENTRIES = (
    ((0, 0), (0, 0, 1)), ((319, 239), (319, 239, 1)), ((159, 65), (159, 65, 1)), ((1, 1), (1, 1, 1)),
    ((318, 238), (318, 238, 1)), ((160, 120), (160, 120, 1)), ((37, 211), (37, 211, 1)), ((255, 7), (255, 7, 1)),
    ((10, 20), (10, 20, 1)), ((155, 110), (155, 110, 1)), ((300, 200), (300, 200, 1)))


class FakeOracle:
    def __init__(self, state_file, fail_after=None):
        self.state_file = state_file
        self.fail_after = fail_after
        self.seen = []

    def advance(self, count):
        if self.fail_after is not None and len(self.seen) >= self.fail_after:
            raise IOError('connection lost')
        for _ in range(count):
            self.seen.append(json.loads(self.state_file.read_text()).get('touch'))


class ParseTouchTest(unittest.TestCase):
    def test_tap_forms(self):
        self.assertEqual(motion.parse_touch(['159', '65']), [(159, 65)] * motion.DEFAULT_TAP_UPDATES)
        self.assertEqual(motion.parse_touch(['0', '239', '3']), [(0, 239)] * 3)
        self.assertEqual(motion.parse_touch(['319', '0', '600']), [(319, 0)] * 600)
        self.assertEqual(motion.parse_touch(['release']), [])

    def test_bad_tap_arguments_are_rejected(self):
        for name, words, pattern in (
                ('none', [], 'needs X Y'),
                ('one', ['5'], 'needs X Y'),
                ('four', ['1', '2', '3', '4'], 'needs X Y'),
                ('release with more', ['release', '3'], "touch release takes no arguments, got '3'"),
                ('unknown word', ['tap', '3'], "touch x: 'tap' is not an integer"),
                ('word y', ['3', 'up'], "touch y: 'up' is not an integer"),
                ('float', ['1.5', '2'], "'1.5' is not an integer"),
                ('plus sign', ['+1', '2'], r"'\+1' is not an integer"),
                ('negative x', ['-1', '2'], "touch x: '-1' is outside 0..319"),
                ('negative y', ['1', '-2'], "touch y: '-2' is outside 0..239"),
                ('x too big', ['320', '0'], "touch x: '320' is outside 0..319"),
                ('y too big', ['0', '240'], "touch y: '240' is outside 0..239"),
                ('zero updates', ['1', '2', '0'], "touch updates: '0' is outside 1..600"),
                ('negative updates', ['1', '2', '-3'], "touch updates: '-3' is outside 1..600"),
                ('too many updates', ['1', '2', '601'], "touch updates: '601' is outside 1..600"),
                ('text updates', ['1', '2', 'many'], "touch updates: 'many' is not an integer")):
            with self.subTest(name), self.assertRaisesRegex(motion.MotionError, pattern):
                motion.parse_touch(words)


class ParseDragTest(unittest.TestCase):
    def test_drag_runs_in_a_straight_line_with_one_point_per_update(self):
        plan = motion.parse_drag(['10', '20', '300', '200', '3'])
        self.assertEqual(plan, [(10, 20), (155, 110), (300, 200)])
        plan = motion.parse_drag(['0', '0', '319', '239'])
        self.assertEqual((len(plan), plan[0], plan[-1]), (motion.DEFAULT_DRAG_UPDATES, (0, 0), (319, 239)))
        self.assertEqual(motion.parse_drag(['100', '100', '90', '80', '2']), [(100, 100), (90, 80)])

    def test_every_pixel_of_a_full_width_sweep_is_visited_once(self):
        plan = motion.parse_drag(['0', '100', '319', '100', '320'])
        self.assertEqual(plan, [(x, 100) for x in range(320)])

    def test_bad_drag_arguments_are_rejected(self):
        for name, words, pattern in (
                ('none', [], 'needs X1 Y1 X2 Y2'),
                ('three', ['1', '2', '3'], 'needs X1 Y1 X2 Y2'),
                ('six', ['1', '2', '3', '4', '5', '6'], 'needs X1 Y1 X2 Y2'),
                ('text', ['1', '2', 'x', '4'], "drag end x: 'x' is not an integer"),
                ('start x', ['320', '2', '3', '4'], "drag start x: '320' is outside 0..319"),
                ('start y', ['1', '240', '3', '4'], "drag start y: '240' is outside 0..239"),
                ('end x', ['1', '2', '-3', '4'], "drag end x: '-3' is outside 0..319"),
                ('end y', ['1', '2', '3', '999'], "drag end y: '999' is outside 0..239"),
                ('one update', ['1', '2', '3', '4', '1'], "drag updates: '1' is outside 2..600"),
                ('zero updates', ['1', '2', '3', '4', '0'], "drag updates: '0' is outside 2..600"),
                ('too many', ['1', '2', '3', '4', '601'], "drag updates: '601' is outside 2..600")):
            with self.subTest(name), self.assertRaisesRegex(motion.MotionError, pattern):
                motion.parse_drag(words)


class TouchPacketTest(unittest.TestCase):
    def packet(self, touch):
        state = {'accel': [0.0, 0.0, -1.0], 'gyro': [0.0, 0.0, 0.0]}
        if touch is not None:
            state['touch'] = touch
        return motion.pad_data_packet(1, 2, state)

    def test_first_touch_fields_follow_the_protocol_table(self):
        # Cemuhook protocol document (https://v1993.github.io/cemuhook-protocol/, version 1001), pad data: first
        # touch at packet offset 56 = active u8, id u8, x u16, y u16; the second touch follows at 62 and the motion
        # timestamp at 68. Read as a specification only.
        packet = self.packet([159, 65])
        self.assertEqual(len(packet), 100)
        self.assertEqual(struct.unpack_from('<BBHH', packet, 56), (1, 1, 1595, 655))
        self.assertEqual(packet[62:68], bytes(6))
        self.assertEqual(struct.unpack_from('<Q', packet, 68)[0], 2)
        self.assertEqual(struct.unpack_from('<I', packet, 8)[0], zlib.crc32(packet[:8] + bytes(4) + packet[12:]))

    def test_no_touch_leaves_both_touch_slots_zero(self):
        self.assertEqual(self.packet(None)[56:68], bytes(12))
        self.assertEqual(motion.pad_data_packet(1, 2, {'accel': [0.0] * 3, 'gyro': [0.0] * 3, 'touch': None})[56:68],
                         bytes(12))

    def test_pad_range_is_ten_pad_units_per_pixel(self):
        self.assertEqual((motion.TOUCH_PAD_WIDTH, motion.TOUCH_PAD_HEIGHT), (3200, 2400))
        self.assertEqual(motion.touch_pad_point((0, 0)), (5, 5))
        self.assertEqual(motion.touch_pad_point((319, 239)), (3195, 2395))

    def test_pad_value_lands_on_the_measured_pixel(self):
        # Azahar normalises the pad value by the configured range and scales it to the 320x240 screen; the game's HID
        # memory then held exactly the pixel for every measured point (MEASURED_HID_ENTRIES).
        for pixel, entry in MEASURED_HID_ENTRIES:
            x, y = motion.touch_pad_point(pixel)
            with self.subTest(pixel):
                self.assertEqual(
                    (x * motion.SCREEN_WIDTH // motion.TOUCH_PAD_WIDTH, y * motion.SCREEN_HEIGHT // motion.TOUCH_PAD_HEIGHT,
                     1), entry)


class StateFileTest(unittest.TestCase):
    def test_touch_state_round_trips_and_validates(self):
        text = json.dumps({'accel': [0, 0, -1], 'gyro': [0, 0, 0], 'touch': [159, 65]})
        self.assertEqual(motion.load_state(text)['touch'], [159, 65])
        self.assertNotIn('touch', motion.load_state(json.dumps({'accel': [0, 0, -1], 'gyro': [0, 0, 0], 'touch': None})))
        for touch in ([320, 0], [0, 240], [-1, 0], [1], 'x', 5):
            with self.subTest(touch), self.assertRaises(motion.MotionError):
                motion.load_state(json.dumps({'accel': [0, 0, -1], 'gyro': [0, 0, 0], 'touch': touch}))

    def test_tilt_and_touch_keep_each_others_values(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'motion.json'
            path.write_text(json.dumps(motion.level_state()))
            motion.update_state(path, touch=[1, 2])
            motion.update_state(path, **motion.parse_tilt(['1', '0', '0']))
            self.assertEqual(json.loads(path.read_text()),
                             {'accel': [1.0, 0.0, 0.0], 'gyro': [0.0, 0.0, 0.0], 'touch': [1, 2]})
            motion.update_state(path, touch=None)
            self.assertEqual(json.loads(path.read_text()), {'accel': [1.0, 0.0, 0.0], 'gyro': [0.0, 0.0, 0.0]})

    def test_a_state_without_touch_is_still_read_by_the_old_format(self):
        self.assertEqual(motion.load_state('{"accel": [0, 0, -1], "gyro": [0, 0, 0]}'),
                         {'accel': [0.0, 0.0, -1.0], 'gyro': [0.0, 0.0, 0.0]})


class ConfigTest(unittest.TestCase):
    def test_config_template_names_the_touch_device_with_the_pad_range(self):
        self.assertIn(r'profiles\1\touch_device="engine:cemuhookudp,pad_index:0,min_x:0,min_y:0,'
                      'max_x:{touch_max_x},max_y:{touch_max_y}"', oracle.CONFIG)
        self.assertIn(r'profiles\1\touch_device\default=false', oracle.CONFIG)


@mock.patch.object(oracle, 'TOUCH_SETTLE_SECONDS', 0.0)
class ScriptCommandTest(unittest.TestCase):
    def run_lines(self, lines, fake_fail=None, touch_device=True):
        with tempfile.TemporaryDirectory() as directory:
            state = Path(directory) / 'motion.json'
            state.write_text(json.dumps(motion.level_state()))
            fake = FakeOracle(state, fake_fail)
            try:
                oracle.run_scenario(fake, lines, state, touch_device)
            finally:
                final = json.loads(state.read_text())
            return fake, final

    def test_tap_holds_the_point_for_the_updates_and_releases(self):
        fake, final = self.run_lines(['touch 159 65 4  # comment', 'frames 0'])
        self.assertEqual(fake.seen, [[159, 65]] * 4)
        self.assertNotIn('touch', final)

    def test_drag_moves_one_point_per_update(self):
        fake, final = self.run_lines(['drag 10 20 300 200 3'])
        self.assertEqual(fake.seen, [[10, 20], [155, 110], [300, 200]])
        self.assertNotIn('touch', final)

    def test_release_clears_a_touch(self):
        with tempfile.TemporaryDirectory() as directory:
            state = Path(directory) / 'motion.json'
            motion.write_state(state, dict(motion.level_state(), touch=[3, 4]))
            oracle.run_scenario(FakeOracle(state), ['touch release'], state)
            self.assertNotIn('touch', json.loads(state.read_text()))

    def test_an_error_during_the_hold_still_releases(self):
        with tempfile.TemporaryDirectory() as directory:
            state = Path(directory) / 'motion.json'
            state.write_text(json.dumps(motion.level_state()))
            with self.assertRaises(IOError):
                oracle.run_scenario(FakeOracle(state, 2), ['touch 5 6 10'], state)
            self.assertNotIn('touch', json.loads(state.read_text()))

    def test_tilt_during_a_script_keeps_the_released_state(self):
        _, final = self.run_lines(['touch 1 1 2', 'tilt 0.5 0 -1'])
        self.assertEqual(final, {'accel': [0.5, 0.0, -1.0], 'gyro': [0.0, 0.0, 0.0]})

    def test_errors_name_the_line_the_step_and_the_bad_text(self):
        for lines, pattern in (
                (['frames 0', 'touch 320 5'], r"line 2: touch: touch x: '320' is outside 0\.\.319"),
                (['touch 1'], r'line 1: touch: touch needs X Y'),
                (['touch 1 2 0'], r"line 1: touch: touch updates: '0' is outside 1\.\.600"),
                (['touch -1 2'], r"line 1: touch: touch x: '-1' is outside"),
                (['touch a b'], r"line 1: touch: touch x: 'a' is not an integer"),
                (['touch release now'], r"line 1: touch: touch release takes no arguments, got 'now'"),
                (['drag 1 2 3'], r'line 1: drag: drag needs X1 Y1 X2 Y2'),
                (['drag 1 2 3 4 1'], r"line 1: drag: drag updates: '1' is outside 2\.\.600"),
                (['drag 1 2 3 240'], r"line 1: drag: drag end y: '240' is outside 0\.\.239")):
            with self.subTest(lines), self.assertRaisesRegex(ValueError, pattern):
                self.run_lines(lines)

    def test_a_rejected_step_changes_nothing(self):
        with tempfile.TemporaryDirectory() as directory:
            state = Path(directory) / 'motion.json'
            motion.write_state(state, motion.level_state())
            before = state.read_text()
            with self.assertRaises(ValueError):
                oracle.run_scenario(FakeOracle(state), ['touch 999 0'], state)
            self.assertEqual(state.read_text(), before)

    def test_without_the_motion_source_or_the_touch_device_the_step_is_rejected(self):
        for op in ('touch 1 2', 'drag 1 2 3 4'):
            with self.subTest(op):
                with self.assertRaisesRegex(ValueError, 'line 1: %s needs the virtual motion source' % op.split()[0]):
                    oracle.run_scenario(None, [op])
                with self.assertRaisesRegex(ValueError, 'line 1: %s needs a session launched with the touch device'
                                            % op.split()[0]):
                    self.run_lines([op], touch_device=False)


class ServerTest(unittest.TestCase):
    def test_server_sends_the_touch_point_from_the_state_file(self):
        probe = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        probe.bind(('127.0.0.1', 0))
        port = probe.getsockname()[1]
        probe.close()
        with tempfile.TemporaryDirectory() as directory:
            state = Path(directory) / 'motion.json'
            motion.write_state(state, motion.level_state())
            server = subprocess.Popen([sys.executable, str(TOOLS / 'oracle_motion.py'), 'serve', '--port', str(port),
                                       '--state', str(state)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            client = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            client.settimeout(0.5)
            try:
                deadline = time.monotonic() + 10.0
                registered = False
                while not registered and time.monotonic() < deadline:
                    client.sendto(motion.build_request(0x100002, bytes(8)), ('127.0.0.1', port))
                    try:
                        registered = len(client.recv(1024)) == 100
                    except socket.timeout:
                        pass
                self.assertTrue(registered, 'no pad data from the motion source')
                self.assertEqual(self.touch_field(client), (0, 0, 0, 0))
                motion.update_state(state, touch=[159, 65])
                self.assertEqual(self.until_touched(client), (1, 1, 1595, 655))
                motion.update_state(state, touch=None)
                self.assertEqual(self.until_released(client), (0, 0, 0, 0))
            finally:
                client.close()
                server.kill()
                server.wait(timeout=10)

    def touch_field(self, client):
        while True:
            packet = client.recv(1024)
            if len(packet) == 100:
                return struct.unpack_from('<BBHH', packet, 56)

    def until_touched(self, client):
        deadline = time.monotonic() + 5.0
        field = self.touch_field(client)
        while field[0] == 0 and time.monotonic() < deadline:
            field = self.touch_field(client)
        return field

    def until_released(self, client):
        deadline = time.monotonic() + 5.0
        field = self.touch_field(client)
        while field[0] == 1 and time.monotonic() < deadline:
            field = self.touch_field(client)
        return field


if __name__ == '__main__':
    unittest.main()
