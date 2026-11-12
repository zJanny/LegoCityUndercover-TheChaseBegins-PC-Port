import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parents[1] / 'tools'))
import generated_file_census as census  # noqa: E402
import regenerate_generated_files as regenerate  # noqa: E402

README = 'Intro\n\n<!-- progress:start -->\nold\n<!-- progress:end -->\n\nOutro\n'


def git(root, *args):
    subprocess.run(['git', '-C', str(root), '-c', 'user.name=Test', '-c', 'user.email=test@example.invalid', *args], check=True, capture_output=True)


class RegenerateTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        (self.root / 'tests' / 'data').mkdir(parents=True)
        shutil.copy(Path(__file__).parents[1] / census.TABLE_PATH, self.root / census.TABLE_PATH)
        (self.root / 'README.md').write_text(README)
        (self.root / 'CMakeLists.txt').write_text('')
        (self.root / 'docs' / 'plans').mkdir(parents=True)
        git(self.root, 'init', '-q', '-b', 'main')
        git(self.root, 'add', '-A')
        git(self.root, 'commit', '-q', '-m', 'first')

    def tearDown(self):
        self.tmp.cleanup()

    def test_first_run_writes_the_three_files_and_the_second_changes_nothing(self):
        self.assertEqual(regenerate.main(['--root', str(self.root), '--check']), 1)
        self.assertEqual(regenerate.main(['--root', str(self.root)]), 0)
        for name in ('README.md', 'docs/progress-trend.svg', 'docs/plans/priorities.md'):
            self.assertTrue((self.root / name).is_file(), name)
        self.assertEqual(regenerate.regenerate(self.root, False), [])
        self.assertEqual(regenerate.main(['--root', str(self.root), '--check']), 0)

    def test_a_new_commit_alone_changes_nothing(self):
        regenerate.main(['--root', str(self.root)])
        git(self.root, 'add', '-A')
        git(self.root, 'commit', '-q', '-m', 'generated')
        before = (self.root / 'README.md').read_bytes()
        git(self.root, 'commit', '-q', '--allow-empty', '-m', 'another commit')
        self.assertEqual(regenerate.regenerate(self.root, True), [])
        self.assertEqual(before, (self.root / 'README.md').read_bytes())

    def test_changed_numbers_change_the_files(self):
        regenerate.main(['--root', str(self.root)])
        path = self.root / census.TABLE_PATH
        path.write_text(path.read_text().replace('readme_progress,files_total,', 'readme_progress,files_total,1', 1))
        self.assertEqual(regenerate.regenerate(self.root, False)[0], 'README.md')

    def test_marker_guard_skips_only_marked_commits(self):
        self.assertFalse(regenerate.head_is_marked(self.root))
        git(self.root, 'commit', '-q', '--allow-empty', '-m', 'Regenerate generated files %s' % regenerate.COMMIT_MARKER)
        self.assertTrue(regenerate.head_is_marked(self.root))
        self.assertEqual(regenerate.main(['--root', str(self.root), '--skip-marked']), 0)
        self.assertFalse((self.root / 'docs' / 'progress-trend.svg').exists())
        self.assertEqual(regenerate.main(['--root', str(self.root)]), 0)
        self.assertTrue((self.root / 'docs' / 'progress-trend.svg').exists())

    def test_rejections(self):
        with self.assertRaises(SystemExit):
            regenerate.main(['--root', str(self.root / 'missing')])
        with self.assertRaises(SystemExit):
            regenerate.main(['--unknown'])


if __name__ == '__main__':
    unittest.main()
