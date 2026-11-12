import os
import re
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent))
import fixtures  # noqa: E402

INHERITED = {
    'GIT_DIR': 'decoy',
    'GIT_WORK_TREE': 'decoy-tree',
    'GIT_COMMON_DIR': 'decoy',
    'GIT_INDEX_FILE': 'decoy-index',
    'GIT_CONFIG_GLOBAL': 'decoy-config',
    'GIT_CONFIG_COUNT': '1',
    'GIT_CONFIG_KEY_0': 'user.name',
    'GIT_CONFIG_VALUE_0': 'Decoy',
    'GIT_AUTHOR_NAME': 'Decoy',
    'GIT_AUTHOR_EMAIL': 'decoy@example.invalid',
    'GIT_COMMITTER_NAME': 'Decoy',
    'GIT_COMMITTER_EMAIL': 'decoy@example.invalid',
    'GIT_SHALLOW_FILE': 'decoy-shallow',
    'GIT_GRAFT_FILE': 'decoy-graft',
    'GIT_QUARANTINE_PATH': 'decoy-quarantine',
}


def run_git(cwd, *args, **kwargs):
    return subprocess.run(['git', *args], cwd=cwd, check=True, capture_output=True, text=True, encoding='utf-8', **kwargs)


class GitEnvironmentTest(unittest.TestCase):
    def test_without_git_environment_drops_every_git_variable_but_the_kept_ones(self):
        environment = {**INHERITED, 'git_dir': 'lower', 'PATH': '/bin', 'GIT_TERMINAL_PROMPT': '0', 'GIT_EXEC_PATH': '/exec', 'HOME': '/home/x'}
        self.assertEqual(fixtures.without_git_environment(environment),
                         {'PATH': '/bin', 'GIT_TERMINAL_PROMPT': '0', 'GIT_EXEC_PATH': '/exec', 'HOME': '/home/x'})

    def test_isolated_tests_leave_the_inherited_repository_untouched(self):
        with tempfile.TemporaryDirectory() as scratch:
            base = Path(scratch)
            decoy = base / 'decoy'
            run_git(base, 'init', '-q', str(decoy))
            config = decoy / '.git' / 'config'
            before = config.read_bytes()
            inherited = {**INHERITED, 'GIT_DIR': str(decoy / '.git'), 'GIT_WORK_TREE': str(decoy), 'GIT_COMMON_DIR': str(decoy / '.git'),
                         'GIT_INDEX_FILE': str(decoy / '.git' / 'index'), 'GIT_CONFIG_GLOBAL': str(base / 'decoy-config')}
            with mock.patch.dict(os.environ, inherited):
                fixtures.isolate_git_environment()
                self.assertFalse([name for name in os.environ if fixtures.is_git_environment_variable(name)])
                work = base / 'work'
                run_git(base, 'init', '-q', str(work))
                run_git(work, 'config', 'user.name', 'Test')
                run_git(work, 'config', 'user.email', 'test@example.invalid')
                (work / 'a.txt').write_text('a\n', encoding='utf-8', newline='\n')
                run_git(work, 'add', 'a.txt')
                run_git(work, 'commit', '-q', '-m', 'init')
            self.assertEqual(config.read_bytes(), before)
            self.assertFalse((base / 'decoy-config').exists())
            self.assertEqual(run_git(work, 'rev-parse', '--show-toplevel').stdout.strip().replace('\\', '/'), work.resolve().as_posix())
            self.assertEqual(run_git(work, 'log', '-1', '--format=%an').stdout.strip(), 'Test')
            self.assertEqual(run_git(decoy, 'rev-list', '--all', '--count').stdout.strip(), '0')


class EveryGitTestIsolatedTest(unittest.TestCase):
    TESTS = Path(__file__).resolve().parent
    TOOLS = TESTS.parent / 'tools'
    ISOLATED = re.compile(r'^isolate_git_environment\(\)$', re.M)
    GIT_ARGUMENT = re.compile(r'''['"]git['"]''')

    def tools_that_run_git(self):
        sources = {path.stem: path.read_text(encoding='utf-8') for path in self.TOOLS.glob('*.py')}
        found = {name for name, text in sources.items() if self.GIT_ARGUMENT.search(text)}
        while True:
            importers = {name for name, text in sources.items() if name not in found
                         and found & set(re.findall(r'^\s*(?:import|from)\s+(\w+)', text, re.M))}
            if not importers:
                return sorted(found)
            found |= importers

    def test_every_test_that_runs_git_isolates_the_environment(self):
        runners = re.compile(r'\b(' + '|'.join(self.tools_that_run_git()) + r')\b')
        unprotected = []
        for path in sorted(self.TESTS.glob('test_*.py')):
            if path == Path(__file__).resolve():
                continue
            text = path.read_text(encoding='utf-8')
            if (self.GIT_ARGUMENT.search(text) or runners.search(text)) and not self.ISOLATED.search(text):
                unprotected.append(path.name)
        self.assertEqual(unprotected, [], 'these tests run git or a tool that runs git without isolate_git_environment() '
                                          'from tests/fixtures.py at import time')

    def test_the_tools_that_run_git_are_found(self):
        found = self.tools_that_run_git()
        for name in ('merge_agent', 'claude_hooks', 'check_doc_links', 'priority_report', 'generated_file_census'):
            self.assertIn(name, found)


if __name__ == '__main__':
    unittest.main()
