import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parent.parent / 'tools' / 'check_dco.py'
SIGN = 'Signed-off-by: A Person <12345+person@users.noreply.github.com>'


def git(root, *args, author='A Person'):
    subprocess.run(['git', '-C', str(root), '-c', 'user.name=%s' % author, '-c', 'user.email=a@example.invalid', *args],
                   check=True, capture_output=True)


def run(root, *args):
    return subprocess.run([sys.executable, str(SCRIPT), '--root', str(root), *args], capture_output=True, text=True)


class DcoTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        git(self.root, 'init', '-q', '-b', 'main')
        git(self.root, 'commit', '-q', '--allow-empty', '-m', 'base')
        git(self.root, 'branch', 'base')
        git(self.root, 'checkout', '-q', '-b', 'topic')

    def tearDown(self):
        self.tmp.cleanup()

    def commit(self, message, author='A Person'):
        git(self.root, 'commit', '-q', '--allow-empty', '-m', message, author=author)

    def test_signed_commits_pass_and_the_base_is_not_checked(self):
        self.commit('One\n\n' + SIGN)
        self.commit('Two\n\n' + SIGN)
        result = run(self.root, '--base', 'base', '--head', 'topic')
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn('2 commits checked', result.stdout)

    def test_unsigned_commit_fails_with_the_fix(self):
        self.commit('One\n\n' + SIGN)
        self.commit('Two')
        result = run(self.root, '--base', 'base', '--head', 'topic')
        self.assertEqual(result.returncode, 1)
        self.assertIn('1 of 2 commits', result.stdout)
        for text in ('git rebase --signoff main', 'git commit --amend -s', 'force-push', 'noreply'):
            self.assertIn(text, result.stdout)

    def test_sign_off_must_have_a_name_and_an_address(self):
        self.commit('One\n\nSigned-off-by: nobody')
        self.assertEqual(run(self.root, '--base', 'base', '--head', 'topic').returncode, 1)

    def test_bot_commits_are_not_checked(self):
        self.commit('Update generated files', author='github-actions[bot]')
        self.assertEqual(run(self.root, '--base', 'base', '--head', 'topic').returncode, 0)

    def test_merge_commits_are_not_checked(self):
        git(self.root, 'checkout', '-q', 'main')
        self.commit('Main moves on\n\n' + SIGN)
        git(self.root, 'checkout', '-q', 'topic')
        self.commit('One\n\n' + SIGN)
        git(self.root, 'merge', '-q', '--no-ff', '-m', 'Merge main', 'main')
        self.assertEqual(run(self.root, '--base', 'base', '--head', 'topic').returncode, 0)

    def test_rejections_name_the_option(self):
        for args, text in ((('--base', 'nope', '--head', 'topic'), '--base'), (('--base', 'base', '--head', 'x y'), '--head'),
                           (('--base', 'base'), '--head'), (('--head', 'topic'), '--base'), (('--base', 'base', '--head', 'topic', '--x'), '--x')):
            result = run(self.root, *args)
            self.assertEqual(result.returncode, 2, args)
            self.assertIn(text, result.stderr)


if __name__ == '__main__':
    unittest.main()
