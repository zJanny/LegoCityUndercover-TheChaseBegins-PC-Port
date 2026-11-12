import subprocess
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from fixtures import isolate_git_environment  # noqa: E402

isolate_git_environment()

ROOT = Path(__file__).resolve().parent.parent


def tracked_eol_rows():
    result = subprocess.run(['git', '-C', str(ROOT), 'ls-files', '--eol'], capture_output=True, encoding='utf-8')
    if result.returncode != 0:
        return None
    rows = []
    for line in result.stdout.splitlines():
        info, _, path = line.partition('\t')
        index, worktree, attribute = info.split()[:3]
        rows.append((path, index, worktree, attribute))
    return rows


class LineEndingTests(unittest.TestCase):
    def setUp(self):
        self.rows = tracked_eol_rows()
        if self.rows is None:
            self.skipTest('not a git checkout')

    def test_gitattributes_forces_lf(self):
        text = (ROOT / '.gitattributes').read_text(encoding='utf-8').split('\n')
        self.assertIn('* text=auto eol=lf', text)

    def test_no_tracked_text_file_is_stored_with_crlf(self):
        found = [path for path, index, _, attribute in self.rows
                 if index in ('i/crlf', 'i/mixed') and attribute not in ('attr/-text', 'attr/binary')]
        self.assertEqual(found, [])

    def test_no_text_file_is_checked_out_with_crlf(self):
        found = [path for path, _, worktree, attribute in self.rows
                 if worktree in ('w/crlf', 'w/mixed') and attribute not in ('attr/-text', 'attr/binary')]
        self.assertEqual(found, [])


if __name__ == '__main__':
    unittest.main()
