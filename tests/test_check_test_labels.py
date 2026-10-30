import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

TOOL = Path(__file__).resolve().parent.parent / "tools" / "check_test_labels.py"

TESTFILE = """\
add_test(plain "/b/plain-test")
add_test(romfs-labelled "/b/t" "/data/romfs/a.fib")
set_tests_properties(romfs-labelled PROPERTIES  LABELS "slow;needs-game-data")
add_test(romfs-unlabelled "/b/t" "/data/romfs/a.fib")
add_test(default-folder "/b/t" "/src/artifacts/romfs/romfs/a.fib")
add_test(skips "/b/t")
set_tests_properties(skips PROPERTIES  SKIP_RETURN_CODE "77")
add_test(skips-labelled "/b/t")
set_tests_properties(skips-labelled PROPERTIES  SKIP_RETURN_CODE "77" LABELS "needs-game-data")
add_test(multiline "/b/t" "/data/romfs/a.fib")
set_tests_properties(multiline PROPERTIES  FAIL_REGULAR_EXPRESSION "first line
second line" LABELS "needs-game-data" TIMEOUT "9")
add_test(skips-display "/b/t")
set_tests_properties(skips-display PROPERTIES  SKIP_RETURN_CODE "77" LABELS "needs-display")
add_test(setup-shot "/b/t")
set_tests_properties(setup-shot PROPERTIES  FIXTURES_SETUP "shot" SKIP_RETURN_CODE "77")
add_test(check-shot "/b/check.py" "shot.bmp")
set_tests_properties(check-shot PROPERTIES  FIXTURES_REQUIRED "shot")
"""


def run(testfile: str, cache: str = "CHASE_ROMFS_DIR:PATH=/data/romfs\n", *extra: str) -> subprocess.CompletedProcess:
    with tempfile.TemporaryDirectory() as directory:
        build = Path(directory)
        (build / "CTestTestfile.cmake").write_text(testfile)
        if cache is not None:
            (build / "CMakeCache.txt").write_text(cache)
        return subprocess.run([sys.executable, str(TOOL), str(build), *extra], capture_output=True, text=True)


class CheckTestLabelsTest(unittest.TestCase):
    def test_lists_exactly_the_unlabelled_game_data_tests(self) -> None:
        result = run(TESTFILE)
        self.assertEqual(result.returncode, 1)
        flagged = sorted(line.split(":")[0] for line in result.stderr.splitlines() if "needs the label" in line)
        self.assertEqual(flagged, ["check-shot", "default-folder", "romfs-unlabelled", "setup-shot", "skips"])

    def test_passes_when_every_test_is_labelled_or_independent(self) -> None:
        result = run("add_test(plain \"/b/t\")\nset_tests_properties(plain PROPERTIES  LABELS \"slow\")\n")
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_missing_cache_is_rejected(self) -> None:
        result = run(TESTFILE, None)
        self.assertEqual(result.returncode, 2)
        self.assertIn("CMakeCache.txt", result.stderr)

    def test_missing_testfile_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            result = subprocess.run([sys.executable, str(TOOL), directory], capture_output=True, text=True)
        self.assertEqual(result.returncode, 2)
        self.assertIn("CTestTestfile.cmake", result.stderr)

    def test_unknown_option_is_rejected(self) -> None:
        result = run(TESTFILE, "CHASE_ROMFS_DIR:PATH=/data/romfs\n", "--bogus")
        self.assertEqual(result.returncode, 2)
        self.assertIn("--bogus", result.stderr)


if __name__ == "__main__":
    unittest.main()
