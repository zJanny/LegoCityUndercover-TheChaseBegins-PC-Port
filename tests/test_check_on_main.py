import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parent.parent / "tools" / "check_on_main.py"


def git(cwd, *args):
    subprocess.run(["git", *args], cwd=cwd, check=True, capture_output=True)


def run(cwd, *args):
    return subprocess.run([sys.executable, str(SCRIPT), *args], cwd=cwd, capture_output=True, text=True)


class CheckOnMainTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.repo = Path(self.tmp.name)
        git(self.repo, "init", "-q", "-b", "main")
        git(self.repo, "config", "user.email", "test@example.invalid")
        git(self.repo, "config", "user.name", "Test")
        git(self.repo, "commit", "-q", "--allow-empty", "-m", "init")

    def tearDown(self):
        self.tmp.cleanup()

    def test_main_runs_the_command_and_keeps_its_exit_code(self):
        self.assertEqual(run(self.repo, sys.executable, "-c", "pass").returncode, 0)
        self.assertEqual(run(self.repo, sys.executable, "-c", "raise SystemExit(1)").returncode, 1)

    def test_branch_skips_without_running_the_command(self):
        git(self.repo, "checkout", "-q", "-b", "feature")
        result = run(self.repo, sys.executable, "-c", "raise SystemExit(1)")
        self.assertEqual(result.returncode, 77)
        self.assertIn("feature", result.stdout)

    def test_detached_head_runs_the_command(self):
        git(self.repo, "checkout", "-q", "--detach")
        self.assertEqual(run(self.repo, sys.executable, "-c", "raise SystemExit(1)").returncode, 1)

    def test_missing_command_is_rejected(self):
        self.assertEqual(run(self.repo).returncode, 2)


if __name__ == "__main__":
    unittest.main()
