import importlib.util
import json
import os
import signal
import subprocess
import sys
import tempfile
import types
import unittest
from pathlib import Path
from unittest import mock

spec = importlib.util.spec_from_file_location('azahar_oracle', Path(__file__).parents[1] / 'tools/azahar_oracle.py')
oracle = importlib.util.module_from_spec(spec)
spec.loader.exec_module(oracle)


def start_sleeper():
    return subprocess.Popen([sys.executable, '-c', 'import time; time.sleep(60)'], start_new_session=True)


class LaunchedTest(unittest.TestCase):
    def test_stop_kills_and_reaps_both_processes(self):
        emulator, xvfb = start_sleeper(), start_sleeper()
        oracle.Launched(emulator, xvfb, ':0', None).stop()
        self.assertEqual(emulator.returncode, -signal.SIGKILL)
        self.assertEqual(xvfb.returncode, -signal.SIGKILL)
        for pid in (emulator.pid, xvfb.pid):
            with self.assertRaises(ProcessLookupError):
                os.kill(pid, 0)

    def test_stop_leaves_foreign_processes_alone(self):
        other = start_sleeper()
        try:
            oracle.Launched(start_sleeper(), None, None, None).stop()
            self.assertIsNone(other.poll())
        finally:
            oracle.stop_process(other)

    def test_check_raises_after_exit(self):
        process = subprocess.Popen([sys.executable, '-c', 'import os, signal; os.kill(os.getpid(), signal.SIGABRT)'])
        process.wait(timeout=30)
        launched = oracle.Launched(process, None, None, None)
        self.assertTrue(launched.exited())
        with self.assertRaisesRegex(oracle.EmulatorExited, 'SIGABRT'):
            launched.check()

    def test_gdb_connect_stops_waiting_when_the_emulator_exits(self):
        def dead():
            raise oracle.EmulatorExited('gone')
        with self.assertRaises(oracle.EmulatorExited):
            oracle.GdbRemote(1, timeout=30, alive=dead)


class SessionModeTest(unittest.TestCase):
    def launch(self, new_3ds):
        home = Path(tempfile.mkdtemp())
        args = types.SimpleNamespace(display='offscreen', port=24799, rpc=False, renderer='software',
                                    motion_port=25799)
        with mock.patch.object(oracle.subprocess, 'Popen') as popen, \
                mock.patch.object(oracle, 'start_motion_source') as motion:
            popen.return_value.pid = 4242
            motion.return_value.pid = 4243
            oracle.launch_emulator(args, home, '/nonexistent/azahar', Path('game.cci'), new_3ds)
        return json.loads((home / 'session.json').read_text())

    def test_session_records_the_motion_source(self):
        session = self.launch(True)
        self.assertEqual((session['motion_pid'], session['motion_port']), (4243, 25799))

    def test_session_records_new_3ds_mode(self):
        self.assertIs(self.launch(True)['new_3ds'], True)
        self.assertIs(self.launch(False)['new_3ds'], False)


class StateModeTest(unittest.TestCase):
    def oracle_in(self, saved_states, new_3ds):
        remote = mock.Mock()
        keys = mock.Mock()
        return oracle.Oracle(remote, saved_states, keys, saved_states / 'emulator', saved_states, new_3ds=new_3ds)

    def test_savestate_writes_the_running_mode(self):
        saved_states = Path(tempfile.mkdtemp())
        emulator_states = saved_states / 'emulator'
        emulator_states.mkdir()
        instance = self.oracle_in(saved_states, True)

        def fake_run_free(function, *args):
            (emulator_states / 'slot.01.cst').write_bytes(b'state')
            return function(*args)
        instance.run_free = fake_run_free
        instance.keys.window_names.return_value = []
        instance.savestate('k-check')
        meta = json.loads((saved_states / 'k-check.json').read_text())
        self.assertIs(meta['new_3ds'], True)
        self.assertEqual(meta['file'], 'slot.01.cst')

    def test_loadstate_refuses_a_state_from_the_other_mode(self):
        saved_states = Path(tempfile.mkdtemp())
        (saved_states / 'gameplay-start.json').write_text(json.dumps({'file': 'x.cst', 'frame': 0}))
        instance = self.oracle_in(saved_states, True)
        with self.assertRaisesRegex(RuntimeError, 'gameplay-start was saved in Old 3DS mode; start a new emulator '
                                                  'with --state gameplay-start'):
            instance.loadstate('gameplay-start')
        instance.keys.press.assert_not_called()

    def test_loadstate_refuses_a_new_3ds_state_in_old_3ds_mode(self):
        saved_states = Path(tempfile.mkdtemp())
        (saved_states / 'some-state.json').write_text(json.dumps({'file': 'x.cst', 'frame': 0, 'new_3ds': True}))
        instance = self.oracle_in(saved_states, False)
        with self.assertRaisesRegex(RuntimeError, 'some-state was saved in New 3DS mode'):
            instance.loadstate('some-state')


class PlaceStateTest(unittest.TestCase):
    def test_place_state_refuses_a_slot_the_quick_load_does_not_read(self):
        saved_states = Path(tempfile.mkdtemp())
        (saved_states / 'other-state.json').write_text(json.dumps({'file': '00040000000AD600.02.cst', 'new_3ds': True}))
        (saved_states / 'other-state.cst').write_bytes(b'state')
        with self.assertRaisesRegex(SystemExit, 'state other-state: "file" is \'00040000000AD600.02.cst\', but the quick load'):
            oracle.place_state(saved_states, 'other-state', saved_states / 'emulator')
        self.assertFalse((saved_states / 'emulator').exists())

    def test_place_state_copies_a_slot_00_state(self):
        saved_states = Path(tempfile.mkdtemp())
        (saved_states / 'gameplay-start.json').write_text(json.dumps({'file': '00040000000AD600.00.cst'}))
        (saved_states / 'gameplay-start.cst').write_bytes(b'state')
        oracle.place_state(saved_states, 'gameplay-start', saved_states / 'emulator')
        self.assertEqual((saved_states / 'emulator' / '00040000000AD600.00.cst').read_bytes(), b'state')


class FreeDisplayTest(unittest.TestCase):
    def test_lock_of_a_dead_process_is_reused_and_cleaned(self):
        root = Path(tempfile.mkdtemp())
        (root / '.X11-unix').mkdir()
        (root / '.X90-lock').write_text('%10d\n' % os.getpid())
        dead = subprocess.Popen([sys.executable, '-c', 'pass'])
        dead.wait(timeout=30)
        (root / '.X91-lock').write_text('%10d\n' % dead.pid)
        (root / '.X11-unix' / 'X91').write_text('')
        self.assertEqual(oracle.free_display(root), ':91')
        self.assertFalse((root / '.X91-lock').exists())
        self.assertFalse((root / '.X11-unix' / 'X91').exists())
        self.assertTrue((root / '.X90-lock').exists())


class CrashDescriptionTest(unittest.TestCase):
    def home_with_log(self, text):
        home = Path(tempfile.mkdtemp())
        log = home / 'data/azahar-emu/log/azahar_log.txt'
        log.parent.mkdir(parents=True)
        log.write_text(text)
        return home

    def test_known_signature_is_named(self):
        home = self.home_with_log('SoftwareRenderWidget::Present()\nunique_ptr<Service::FS::ArchiveManager>')
        text = oracle.crash_description(home, -signal.SIGABRT)
        self.assertIn('SIGABRT', text)
        self.assertIn('SoftwareRenderWidget::Present', text)

    def test_unknown_crash_is_reported_as_such(self):
        text = oracle.crash_description(self.home_with_log('nothing'), 3)
        self.assertIn('status 3', text)
        self.assertIn('no SoftwareRenderWidget::Present', text)


class RpcFailureMessageTest(unittest.TestCase):
    def home_with_log(self, text):
        home = Path(tempfile.mkdtemp())
        if text is not None:
            log = home / 'data/azahar-emu/log/azahar_log.txt'
            log.parent.mkdir(parents=True)
            log.write_text(text)
        return home

    def test_log_without_rpc_line_names_the_missing_build_option(self):
        error = oracle.RpcError('no RPC reply on UDP port 45987')
        for text in ('Core: started', None):
            message = oracle.rpc_failure_message(self.home_with_log(text), error)
            self.assertIn('ENABLE_SCRIPTING', message)
            self.assertIn('docs/commands.md', message)

    def test_log_with_rpc_line_blames_the_connection(self):
        message = oracle.rpc_failure_message(self.home_with_log('Starting RPC server (synchronous)'),
                                             oracle.RpcError('no RPC reply on UDP port 45987'))
        self.assertNotIn('ENABLE_SCRIPTING', message)
        self.assertIn('does not answer', message)

    def test_connect_failure_stops_with_the_explanation(self):
        launched = types.SimpleNamespace(check=lambda: None)
        with mock.patch.object(oracle, 'connect_rpc', side_effect=oracle.RpcError('no reply')):
            with self.assertRaises(SystemExit) as caught:
                oracle.connect_rpc_or_explain(self.home_with_log('x'), launched)
        self.assertIn('ENABLE_SCRIPTING', str(caught.exception))


if __name__ == '__main__':
    unittest.main()
