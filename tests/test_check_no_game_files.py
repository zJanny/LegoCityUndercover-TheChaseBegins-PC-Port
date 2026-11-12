import importlib
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

TOOLS = Path(__file__).resolve().parent.parent / 'tools'
sys.path.insert(0, str(TOOLS))

try:
    importlib.import_module('forbidden_files')
except ImportError:
    import types

    stand_in = types.ModuleType('forbidden_files')

    def forbidden_reason(path, data_head):
        if path.startswith('artifacts/') or path.endswith('.cci'):
            return 'game-derived file'
        if data_head is not None and b'\0' in data_head:
            return 'binary file'
        return None

    def check_paths(paths, read_head):
        return [(path, reason) for path in paths for reason in [forbidden_reason(path, read_head(path))] if reason]

    stand_in.forbidden_reason = forbidden_reason
    stand_in.check_paths = check_paths
    sys.modules['forbidden_files'] = stand_in

import check_no_game_files as script  # noqa: E402
sys.path.insert(0, str(Path(__file__).resolve().parent))
from fixtures import isolate_git_environment  # noqa: E402

isolate_git_environment()


def git(root, *args):
    subprocess.run(['git', '-C', str(root), '-c', 'user.name=T', '-c', 'user.email=t@example.invalid', *args], check=True, capture_output=True)


class NoGameFilesTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        git(self.root, 'init', '-q', '-b', 'main')

    def tearDown(self):
        self.tmp.cleanup()

    def add(self, name, data):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        git(self.root, 'add', name)

    def test_clean_repository_passes(self):
        self.add('src/a.cpp', b'int x;\n')
        self.assertEqual(script.main(['--root', str(self.root)]), 0)

    def test_game_derived_file_fails(self):
        self.add('src/a.cpp', b'int x;\n')
        self.add('artifacts/game/code.txt', b'x')
        self.assertEqual(script.main(['--root', str(self.root)]), 1)

    def test_binary_file_fails(self):
        self.add('tests/data/dump.xyz', b'\x00\x01\x02')
        self.assertEqual(script.main(['--root', str(self.root)]), 1)

    def test_tracked_files_come_from_git(self):
        self.add('a.txt', b'x')
        (self.root / 'untracked.cci').write_bytes(b'x')
        self.assertEqual(script.tracked_files(self.root), ['a.txt'])

    def test_rejections(self):
        with self.assertRaises(SystemExit):
            script.main(['--root', str(self.root / 'missing')])
        with self.assertRaises(SystemExit):
            script.main(['--nope'])


if __name__ == '__main__':
    unittest.main()
