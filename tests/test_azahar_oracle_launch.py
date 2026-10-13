import importlib.util
import os
import signal
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

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
        launched = oracle.Launched(process, None, None, None)
        self.assertTrue(launched.exited())
        with self.assertRaisesRegex(oracle.EmulatorExited, 'SIGABRT'):
            launched.check()

    def test_gdb_connect_stops_waiting_when_the_emulator_exits(self):
        def dead():
            raise oracle.EmulatorExited('gone')
        with self.assertRaises(oracle.EmulatorExited):
            oracle.GdbRemote(1, timeout=30, alive=dead)


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


if __name__ == '__main__':
    unittest.main()
