import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parent.parent / "tools" / "merge_agent.py"


def run_script(cwd: Path, *args: str) -> subprocess.CompletedProcess:
    return subprocess.run([sys.executable, str(SCRIPT), *args], cwd=cwd, text=True, capture_output=True)


def git(cwd: Path, *args: str) -> None:
    subprocess.run(["git", *args], cwd=cwd, check=True, capture_output=True)


class MergeAgentTest(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.repo = Path(self.tmp.name)
        git(self.repo, "init", "-q", "-b", "main")
        git(self.repo, "config", "user.email", "test@example.invalid")
        git(self.repo, "config", "user.name", "Test")
        (self.repo / "a.txt").write_text("a\n")
        git(self.repo, "add", "a.txt")
        git(self.repo, "commit", "-q", "-m", "init")

    def tearDown(self) -> None:
        self.tmp.cleanup()

    def test_unknown_command_is_rejected(self) -> None:
        result = run_script(self.repo, "merge", "x")
        self.assertEqual(result.returncode, 2)
        self.assertIn("invalid choice: 'merge'", result.stderr)

    def test_finish_needs_a_message(self) -> None:
        result = run_script(self.repo, "finish", "agent")
        self.assertEqual(result.returncode, 2)
        self.assertIn("--message", result.stderr)

    def test_missing_branch_is_named(self) -> None:
        result = run_script(self.repo, "prepare", "no-such-branch")
        self.assertEqual(result.returncode, 1)
        self.assertIn("branch 'no-such-branch' does not exist", result.stderr)

    def test_not_on_main_is_rejected(self) -> None:
        git(self.repo, "checkout", "-q", "-b", "other")
        result = run_script(self.repo, "prepare", "main")
        self.assertEqual(result.returncode, 1)
        self.assertIn("must be on main, not 'other'", result.stderr)

    def test_game_derived_files_are_refused(self) -> None:
        git(self.repo, "checkout", "-q", "-b", "agent")
        (self.repo / "shot.bmp").write_bytes(b"BM\x00\x01")
        git(self.repo, "add", "shot.bmp")
        git(self.repo, "commit", "-q", "-m", "shot")
        git(self.repo, "checkout", "-q", "main")
        result = run_script(self.repo, "prepare", "agent")
        self.assertEqual(result.returncode, 1)
        self.assertIn("game-derived, build or binary files are staged: shot.bmp", result.stderr)


if __name__ == "__main__":
    unittest.main()
