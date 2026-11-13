import io
import subprocess
import sys
import tempfile
import unittest
import zipfile
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path

REPOSITORY_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPOSITORY_ROOT / 'tools'))
sys.path.insert(0, str(Path(__file__).resolve().parent))

import package_windows  # noqa: E402
from fixtures import isolate_git_environment  # noqa: E402

isolate_git_environment()

COMMIT = 'a' * 40
REPOSITORY = 'https://example.org/owner/chase'
GAME_DATA = b'ORIGINAL-GAME-BYTES-MUST-NEVER-SHIP'
PROGRAM = b'MZ\x90\x00 fake program'
PACKAGE = 'mingw-w64-ucrt-x86_64-sdl2-compat'
LICENSE_TEXT = b'zlib license text\n'


def fake_pacman(licensed=True, spdx='spdx:Zlib'):
    def run(command):
        assert command[0] == 'pacman'
        if command[1] == '-Qo':
            name = command[2].rsplit('/', 1)[-1]
            if name == 'SDL2.dll':
                return subprocess.CompletedProcess(command, 0, '%s is owned by %s 2.32.74-5\n' % (command[2], PACKAGE), '')
            return subprocess.CompletedProcess(command, 1, '', 'error: No package owns %s\n' % command[2])
        if command[1] == '-Ql' and command[2] == PACKAGE:
            lines = ['%s /ucrt64/bin/SDL2.dll' % PACKAGE, '%s /ucrt64/share/licenses/' % PACKAGE,
                     '%s /ucrt64/share/licenses/sdl2-compat/' % PACKAGE]
            if licensed:
                lines.append('%s /ucrt64/share/licenses/sdl2-compat/LICENSE.txt' % PACKAGE)
            return subprocess.CompletedProcess(command, 0, '\n'.join(lines) + '\n', '')
        if command[1] == '-Qi' and command[2] == PACKAGE:
            text = 'Name            : %s\nVersion         : 2.32.74-5\nURL             : https://libsdl.org\n' % PACKAGE
            return subprocess.CompletedProcess(command, 0, text + 'Licenses        : %s\n' % spdx, '')
        return subprocess.CompletedProcess(command, 1, '', 'error: unexpected %s\n' % command)
    return run


def owned_by_sdl(command):
    if command[1] == '-Qo':
        return subprocess.CompletedProcess(command, 0, '%s is owned by %s 2.32.74-5\n' % (command[2], PACKAGE), '')
    return fake_pacman()(command)


def write(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data if isinstance(data, bytes) else data.encode('utf-8'))


class PackageWindowsTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        base = Path(self.temporary.name)
        self.root = base / 'repo'
        self.build = self.root / 'build'
        self.output = base / 'release'
        self.runtime = base / 'msys' / 'ucrt64' / 'bin'
        write(self.root / 'LICENSE', 'GNU GENERAL PUBLIC LICENSE\n')
        write(self.root / 'config' / 'controls.cfg', '# bindings\n')
        write(self.root / 'config' / 'controls.default.cfg', '# defaults\n')
        write(self.root / 'tools' / 'extract_one.py', 'import extract_base\n')
        write(self.root / 'tools' / 'extract_base.py', 'import json\n')
        write(self.root / 'tools' / 'convert_two.py', 'import chase_native as native\nimport convert_helper\n')
        write(self.root / 'tools' / 'convert_helper.py', 'import chase_native\n')
        write(self.root / 'tools' / 'chase_native.py', 'import ctypes\n')
        write(self.root / 'tools' / 'merge_agent.py', 'print("a developer tool the docs never name")\n')
        write(self.root / 'tools' / 'game_dump.cci', GAME_DATA)
        write(self.root / 'artifacts' / 'romfs' / 'secret.dat', GAME_DATA)
        write(self.root / 'artifacts' / 'extract_leak.py', GAME_DATA)
        for name in ('chase-game', 'chase-input-lab', 'render-window-test', 'make-test-save', 'camera-lock-census', 'input-test'):
            write(self.build / (name + '.exe'), PROGRAM)
        for name in ('libchase_assets_c.dll', 'SDL2.dll'):
            write(self.build / name, PROGRAM)
        write(self.build / 'game.cci', GAME_DATA)
        write(self.build / 'lego_city3ds.fib', GAME_DATA)
        write(self.build / 'texture.png', GAME_DATA)
        write(self.build / 'artifacts' / 'extract_nested.dll', GAME_DATA)
        write(self.build / 'CMakeCache.txt', 'cache\n')
        write(self.runtime / 'SDL2.dll', PROGRAM)
        write(self.runtime.parent / 'share' / 'licenses' / 'sdl2-compat' / 'LICENSE.txt', LICENSE_TEXT)

    def arguments(self, **overrides):
        values = {'--version': 'v0.1.0', '--commit': COMMIT, '--repository': REPOSITORY, '--output-dir': str(self.output),
                  '--build-dir': str(self.build), '--runtime-dir': str(self.runtime), '--root': str(self.root)}
        values.update(overrides)
        return [item for pair in values.items() for item in pair]

    def run_tool(self, runner=None, **overrides):
        out, err = io.StringIO(), io.StringIO()
        with redirect_stdout(out), redirect_stderr(err):
            code = package_windows.main(self.arguments(**overrides), runner=runner or fake_pacman())
        return code, out.getvalue(), err.getvalue()

    def zip_path(self):
        return self.output / 'chase-windows-v0.1.0.zip'

    def names(self):
        with zipfile.ZipFile(self.zip_path()) as archive:
            return sorted(name.split('/', 1)[1] for name in archive.namelist())

    def test_layout(self):
        code, _, error = self.run_tool()
        self.assertEqual(code, 0, error)
        self.assertEqual(self.names(), [
            'LICENSE',
            'README.txt',
            'build/SDL2.dll',
            'build/chase-game.exe',
            'build/chase-input-lab.exe',
            'build/libchase_assets_c.dll',
            'chase-game.bat',
            'config/controls.cfg',
            'config/controls.default.cfg',
            'licenses/PACKAGES.txt',
            'licenses/sdl2-compat/LICENSE.txt',
            'tools/chase_native.py',
            'tools/convert_helper.py',
            'tools/convert_two.py',
            'tools/extract_base.py',
            'tools/extract_one.py',
        ])
        with zipfile.ZipFile(self.zip_path()) as archive:
            self.assertEqual(archive.read('chase-windows-v0.1.0/licenses/sdl2-compat/LICENSE.txt'), LICENSE_TEXT)
            self.assertIn(PACKAGE, archive.read('chase-windows-v0.1.0/licenses/PACKAGES.txt').decode('utf-8'))
            self.assertIn(b'build\\chase-game.exe', archive.read('chase-windows-v0.1.0/chase-game.bat'))

    def test_test_programs_and_developer_tools_are_left_out(self):
        self.assertEqual(self.run_tool()[0], 0)
        names = self.names()
        for left_out in ('build/render-window-test.exe', 'build/make-test-save.exe', 'build/camera-lock-census.exe',
                         'build/input-test.exe', 'tools/merge_agent.py'):
            self.assertNotIn(left_out, names)

    def test_game_files_never_reach_the_zip(self):
        self.assertEqual(self.run_tool()[0], 0)
        names = self.names()
        for decoy in ('game.cci', 'lego_city3ds.fib', 'texture.png', 'game_dump.cci', 'secret.dat', 'extract_leak.py',
                      'extract_nested.dll', 'CMakeCache.txt'):
            self.assertFalse([name for name in names if decoy in name], decoy)
        with zipfile.ZipFile(self.zip_path()) as archive:
            for name in archive.namelist():
                self.assertNotIn(GAME_DATA, archive.read(name), name)
        for name in names:
            self.assertIsNone(package_windows.forbidden_files.forbidden_reason(
                name[len('build/'):] if name.startswith('build/') else name, None), name)

    def test_game_data_renamed_to_a_library_is_refused(self):
        write(self.build / 'extra.dll', GAME_DATA)
        write(self.runtime / 'extra.dll', PROGRAM)
        code, _, error = self.run_tool(runner=owned_by_sdl)
        self.assertEqual(code, 1)
        self.assertIn('extra.dll is not a Windows program or library', error)
        self.assertFalse(self.zip_path().exists())

    def test_symbolic_link_is_refused(self):
        target = self.root / 'artifacts' / 'romfs' / 'secret.dat'
        link = self.build / 'chase-link.exe'
        try:
            link.symlink_to(target)
        except (OSError, NotImplementedError):
            self.skipTest('symbolic links are not allowed here')
        code, _, error = self.run_tool()
        self.assertEqual(code, 1)
        self.assertIn('not a regular file', error)
        self.assertFalse(self.zip_path().exists())

    def test_build_folder_inside_artifacts_is_refused(self):
        inner = self.root / 'artifacts' / 'copy'
        inner.mkdir(parents=True)
        write(inner / 'chase-game.exe', PROGRAM)
        code, _, error = self.run_tool(**{'--build-dir': str(inner)})
        self.assertEqual(code, 1)
        self.assertIn('inside artifacts/', error)

    def test_entry_rules(self):
        refused = [
            package_windows.Entry('artifacts/romfs/a.txt', None, data=b'text\n'),
            package_windows.Entry('game.cci', None, data=b'text\n'),
            package_windows.Entry('tools/picture.png', None, data=b'text\n'),
            package_windows.Entry('licenses/x/texture.png', None, data=b'text\n'),
            package_windows.Entry('tools/data.py', None, data=b'binary\0content'),
            package_windows.Entry('build/chase-game.exe', None, data=GAME_DATA),
            package_windows.Entry('build/artifacts/romfs/x.dll', None, data=PROGRAM),
            package_windows.Entry('README.md', None, data=b'text\n'),
            package_windows.Entry('tools/extract_leak.py', self.root / 'artifacts' / 'extract_leak.py', base=self.root),
        ]
        for entry in refused:
            with self.assertRaises(package_windows.PackagingError, msg=entry.name):
                package_windows.check_entry(entry, self.root)
        package_windows.check_entry(package_windows.Entry('README.txt', None, data=b'text\n'), self.root)

    def test_finished_zip_is_checked_again(self):
        self.output.mkdir()
        for inner, data in (('chase-windows-v1/notes.txt', b'text\n'), ('chase-windows-v1/tools/a.py', b'\0\0'),
                            ('chase-windows-v1/tools/a.cci', b'text\n'), ('elsewhere/LICENSE', b'text\n')):
            path = self.output / 'bad.zip'
            with zipfile.ZipFile(path, 'w') as archive:
                archive.writestr(inner, data)
            with self.assertRaises(package_windows.PackagingError, msg=inner):
                package_windows.verify_zip(path, 'chase-windows-v1')
            path.unlink()

    def test_package_without_license_files_gets_its_package_information(self):
        self.assertEqual(self.run_tool(runner=fake_pacman(licensed=False))[0], 0)
        with zipfile.ZipFile(self.zip_path()) as archive:
            info = archive.read('chase-windows-v0.1.0/licenses/sdl2-compat/PACKAGE-INFO.txt').decode('utf-8')
        self.assertIn('spdx:Zlib', info)
        self.assertIn(PACKAGE, info)

    def test_package_without_license_files_and_without_license_name_fails(self):
        code, _, error = self.run_tool(runner=fake_pacman(licensed=False, spdx='None'))
        self.assertEqual(code, 1)
        self.assertIn('no license file', error)
        self.assertFalse(self.zip_path().exists())

    def test_library_without_package_fails(self):
        write(self.build / 'orphan.dll', PROGRAM)
        write(self.runtime / 'orphan.dll', PROGRAM)
        code, _, error = self.run_tool()
        self.assertEqual(code, 1)
        self.assertIn('pacman -Qo', error)

    def test_library_missing_from_runtime_folder_fails(self):
        write(self.build / 'stray.dll', PROGRAM)
        code, _, error = self.run_tool()
        self.assertEqual(code, 1)
        self.assertIn('stray.dll is not in the runtime folder', error)

    def test_readme_text(self):
        self.assertEqual(self.run_tool()[0], 0)
        with zipfile.ZipFile(self.zip_path()) as archive:
            text = archive.read('chase-windows-v0.1.0/README.txt').decode('utf-8')
        project_readme = (REPOSITORY_ROOT / 'README.md').read_text(encoding='utf-8')
        self.assertIn(package_windows.NOT_AFFILIATED, project_readme)
        self.assertIn(package_windows.NOT_AFFILIATED, text)
        for needed in ('NOT A PLAYABLE GAME YET', 'Python 3.11', 'extract_romfs.py', COMMIT, REPOSITORY, 'GNU General Public License',
                       'v0.1.0'):
            self.assertIn(needed, text)

    def test_extraction_tools_of_the_real_repository(self):
        names = [path.name for path in package_windows.tool_files(REPOSITORY_ROOT / 'tools')]
        for needed in ('extract_game.py', 'extract_romfs.py', 'extract_fib.py', 'convert_textures.py', 'convert_font.py',
                       'convert_audio.py', 'convert_loc.py', 'convert_levels.py', 'chase_native.py'):
            self.assertIn(needed, names)
        self.assertNotIn('merge_agent.py', names)

    def test_strict_arguments(self):
        bad = [{'--version': 'v1/../x'}, {'--version': ''}, {'--commit': 'abc123'}, {'--commit': 'A' * 40},
               {'--repository': 'http://example.org/x'}, {'--repository': 'https://example.org/x/'},
               {'--build-dir': str(self.root / 'missing')}, {'--runtime-dir': str(self.root / 'missing')}]
        for override in bad:
            with self.assertRaises(SystemExit, msg=str(override)), redirect_stderr(io.StringIO()):
                package_windows.main(self.arguments(**override), runner=fake_pacman())
        with self.assertRaises(SystemExit), redirect_stderr(io.StringIO()):
            package_windows.main(['--version', 'v1'], runner=fake_pacman())

    def test_existing_zip_is_not_overwritten(self):
        self.assertEqual(self.run_tool()[0], 0)
        code, _, error = self.run_tool()
        self.assertEqual(code, 2)
        self.assertIn('refusing to overwrite', error)

    def test_build_without_programs_fails(self):
        for path in self.build.glob('chase-*.exe'):
            path.unlink()
        code, _, error = self.run_tool()
        self.assertEqual(code, 1)
        self.assertIn('no chase-*.exe', error)


if __name__ == '__main__':
    unittest.main()
