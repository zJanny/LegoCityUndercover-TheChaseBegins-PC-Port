import contextlib
import io
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / 'tools'))
import check_tests_data_size as script  # noqa: E402


def run(*args):
    out = io.StringIO()
    with contextlib.redirect_stdout(out):
        code = script.main(list(args))
    return code, out.getvalue()


class TestsDataSizeTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        (self.root / 'tests' / 'data' / 'sub').mkdir(parents=True)

    def tearDown(self):
        self.tmp.cleanup()

    def test_small_files_pass_quietly(self):
        (self.root / 'tests' / 'data' / 'a.csv').write_bytes(b'x' * 100_000)
        code, text = run('--root', str(self.root))
        self.assertEqual(code, 0)
        self.assertNotIn('WARNING', text)

    def test_large_file_warns_and_still_exits_zero(self):
        (self.root / 'tests' / 'data' / 'sub' / 'big.inc').write_bytes(b'x' * 100_001)
        code, text = run('--root', str(self.root))
        self.assertEqual(code, 0)
        self.assertIn('WARNING: tests/data/sub/big.inc', text)

    def test_limit_option(self):
        (self.root / 'tests' / 'data' / 'a.csv').write_bytes(b'x' * 3000)
        self.assertIn('WARNING', run('--root', str(self.root), '--limit-kb', '2')[1])

    def test_rejections(self):
        for args in (['--limit-kb', '0'], ['--limit-kb', 'abc'], ['--root', str(self.root / 'missing')], ['--nope']):
            with contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit):
                script.main(args)


if __name__ == '__main__':
    unittest.main()
