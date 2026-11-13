#!/usr/bin/env python3
"""Build the downloadable Windows zip: the programs, the extraction tools, the licenses. Never game data.

    python3 tools/package_windows.py --version v0.1.0 --commit <40 hex> --repository https://github.com/owner/name --output-dir release

Takes from the build folder every `chase-*.exe` and every `.dll` (the MSYS2 runtime DLLs the build copied next to the programs),
the Python tools of the extraction docs with the local modules they import, `config/controls*.cfg`, LICENSE, the license files of the
MSYS2 package of every bundled DLL (`pacman -Qo`, `pacman -Ql`) and a generated README.txt. Every file is checked against an
allow-rule and against the rules of tools/forbidden_files.py before it goes in, and the finished zip is checked again; the tool
fails and removes the zip when anything else would be inside. Folder inside the zip: build/ (programs and DLLs), tools/, config/,
licenses/, so the commands of the docs work unchanged from the unpacked folder.
"""
from __future__ import annotations

import argparse
import ast
import re
import shutil
import subprocess
import sys
import zipfile
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(Path(__file__).resolve().parent))

import forbidden_files  # noqa: E402

TOOL_ROOT_PATTERNS = ('extract_*.py', 'convert_*.py')
MAGIC_WINDOWS_EXECUTABLE = b'MZ'
HEAD_BYTES = 8192
PACKAGE_PREFIX = 'mingw-w64-ucrt-x86_64-'
FIXED_TIME = (1980, 1, 1, 0, 0, 0)
NOT_AFFILIATED = (
    'This is an unofficial fan project. It is not affiliated with, endorsed by or sponsored by Nintendo or any other right holder '
    'of *LEGO City Undercover: The Chase Begins*. All names and trademarks belong to their owners.'
)

ALLOWED_ARCHIVE_NAMES = (
    re.compile(r'^build/chase-[a-z0-9-]+\.exe$'),
    re.compile(r'^build/[A-Za-z0-9_.+-]+\.dll$'),
    re.compile(r'^tools/[a-z0-9_]+\.py$'),
    re.compile(r'^config/controls(\.default)?\.cfg$'),
    re.compile(r'^LICENSE$'),
    re.compile(r'^README\.txt$'),
    re.compile(r'^chase-game\.bat$'),
    re.compile(r'^licenses/PACKAGES\.txt$'),
    re.compile(r'^licenses/[A-Za-z0-9_.+-]+/([A-Za-z0-9_.+ -]+/)*[A-Za-z0-9_.+ -]+$'),
)
BINARY_NAMES = re.compile(r'^build/.*\.(exe|dll)$')


class PackagingError(Exception):
    pass


@dataclass
class Entry:
    name: str
    source: Path | None
    data: bytes | None = None
    base: Path | None = None
    executable: bool = False

    def content(self) -> bytes:
        if self.data is not None:
            return self.data
        return self.source.read_bytes()


def checked_name(name: str) -> str:
    if any(part in ('.', '..') for part in name.split('/')):
        raise PackagingError('not allowed in the zip: %s' % name)
    if not any(pattern.match(name) for pattern in ALLOWED_ARCHIVE_NAMES):
        raise PackagingError('not allowed in the zip: %s' % name)
    return name


def check_entry(entry: Entry, root: Path) -> None:
    checked_name(entry.name)
    if entry.source is not None:
        if entry.source.is_symlink() or not entry.source.is_file():
            raise PackagingError('not a regular file: %s' % entry.source)
        if entry.base is None:
            raise PackagingError('no base folder for %s' % entry.source)
        relative = entry.source.resolve().relative_to(entry.base.resolve()).as_posix()
        reason = forbidden_files.forbidden_reason(relative, None)
        if reason:
            raise PackagingError('%s is refused (%s): %s' % (entry.source, reason, relative))
    path_name = entry.name[len('build/'):] if entry.name.startswith('build/') else entry.name
    reason = forbidden_files.forbidden_reason(path_name, None)
    if reason:
        raise PackagingError('%s is refused (%s)' % (entry.name, reason))
    head = entry.content()[:HEAD_BYTES]
    if BINARY_NAMES.match(entry.name):
        if not head.startswith(MAGIC_WINDOWS_EXECUTABLE):
            raise PackagingError('%s is not a Windows program or library' % entry.name)
    else:
        reason = forbidden_files.forbidden_reason(path_name, head)
        if reason:
            raise PackagingError('%s is refused (%s)' % (entry.name, reason))


def local_imports(tool: Path, tools: Path) -> list[Path]:
    tree = ast.parse(tool.read_text(encoding='utf-8'), filename=str(tool))
    names = []
    for node in ast.walk(tree):
        if isinstance(node, ast.Import):
            names.extend(alias.name.split('.')[0] for alias in node.names)
        elif isinstance(node, ast.ImportFrom) and node.level == 0 and node.module:
            names.append(node.module.split('.')[0])
    return [tools / (name + '.py') for name in dict.fromkeys(names) if (tools / (name + '.py')).is_file()]


def tool_files(tools: Path) -> list[Path]:
    pending = sorted(path for pattern in TOOL_ROOT_PATTERNS for path in tools.glob(pattern))
    if not pending:
        raise PackagingError('no extraction tools found in %s' % tools)
    found = {}
    while pending:
        path = pending.pop()
        if path.name in found:
            continue
        found[path.name] = path
        pending.extend(local_imports(path, tools))
    return [found[name] for name in sorted(found)]


def program_entries(build: Path) -> list[Entry]:
    entries = []
    for path in sorted(build.glob('chase-*.exe')):
        entries.append(Entry('build/' + path.name, path, base=build, executable=True))
    for path in sorted(build.glob('*.dll')):
        entries.append(Entry('build/' + path.name, path, base=build))
    if not any(entry.name.endswith('.exe') for entry in entries):
        raise PackagingError('no chase-*.exe in %s' % build)
    return entries


def pacman_output(runner, arguments: list[str]) -> str:
    result = runner(['pacman'] + arguments)
    if result.returncode != 0:
        raise PackagingError('pacman %s failed: %s' % (' '.join(arguments), (result.stderr or result.stdout).strip()))
    return result.stdout


def default_runner(command: list[str]):
    return subprocess.run(command, capture_output=True, text=True, encoding='utf-8', check=False)


def package_of(dll: Path, runner) -> tuple[str, str]:
    output = pacman_output(runner, ['-Qo', dll.as_posix()])
    match = re.search(r' is owned by (\S+) (\S+)\s*$', output.strip().splitlines()[-1])
    if not match:
        raise PackagingError('cannot read the package of %s from: %s' % (dll, output.strip()))
    return match.group(1), match.group(2)


def license_files_of(package: str, prefix: Path, runner) -> list[tuple[str, Path]]:
    found = []
    for line in pacman_output(runner, ['-Ql', package]).splitlines():
        match = re.match(r'^\S+ /[^/]+/share/licenses/(.+[^/])$', line.strip())
        if match:
            path = prefix / 'share' / 'licenses' / match.group(1)
            if path.is_file():
                found.append((match.group(1).split('/', 1)[-1], path))
    return sorted(found)


def package_info_entry(package: str, folder: str, runner) -> Entry:
    fields = {}
    for line in pacman_output(runner, ['-Qi', package]).splitlines():
        key, separator, value = line.partition(':')
        if separator and key.strip() in ('Name', 'Version', 'Licenses', 'URL'):
            fields[key.strip()] = value.strip()
    if not fields.get('Licenses') or fields.get('Licenses') == 'None':
        raise PackagingError('no license file installed by %s and no license named in its package information' % package)
    lines = [('%s %s' % (fields.get('Name', package), fields.get('Version', ''))).strip(),
             'License (SPDX): ' + fields['Licenses'],
             'URL: ' + fields.get('URL', 'unknown'),
             'The MSYS2 package installs no license file; the GNU General Public License text is in LICENSE next to this folder.',
             'Source code and build recipe: https://packages.msys2.org/', '']
    return Entry('licenses/%s/PACKAGE-INFO.txt' % folder, None, data='\n'.join(lines).encode('utf-8'))


def license_entries(dlls: list[Entry], runtime_dir: Path, runner) -> list[Entry]:
    prefix = runtime_dir.parent
    packages = {}
    owners = []
    for entry in dlls:
        name = Path(entry.name).name
        if name.startswith('libchase_'):
            continue
        original = runtime_dir / name
        if not original.is_file():
            raise PackagingError('%s is not in the runtime folder %s, so its license cannot be found' % (name, runtime_dir))
        package, version = package_of(original, runner)
        packages[package] = version
        owners.append('%s: %s %s' % (name, package, version))
    entries = []
    for package in sorted(packages):
        folder = package[len(PACKAGE_PREFIX):] if package.startswith(PACKAGE_PREFIX) else package
        files = license_files_of(package, prefix, runner)
        entries.extend(Entry('licenses/%s/%s' % (folder, relative), path, base=prefix) for relative, path in files)
        if not files:
            entries.append(package_info_entry(package, folder, runner))
    listing = ['Third-party libraries bundled in build/ (MSYS2 UCRT64 packages). Each package\'s license files are in the folder named',
               'after it in this folder. Source code and build recipes of the packages: https://packages.msys2.org/', '']
    listing.extend(sorted(owners))
    entries.append(Entry('licenses/PACKAGES.txt', None, data=('\n'.join(listing) + '\n').encode('utf-8')))
    return entries


def readme_text(version: str, commit: str, repository: str, has_game_launcher: bool) -> str:
    start = ('Double-click chase-game.bat, or in a terminal in this folder run: build\\chase-game.exe'
             if has_game_launcher else 'In a terminal in this folder run one of the programs in build\\')
    lines = [
        'LEGO City Undercover: The Chase Begins - PC port, Windows development build %s' % version,
        '',
        'THIS IS NOT A PLAYABLE GAME YET. It is a development runtime of an unfinished port: you can look at the start screens,',
        'walk the main character around a street of the city and play parts of the first missions. It contains no game data.',
        'You need your own decrypted .cci image of the game, and Python 3.11 or newer (https://www.python.org/downloads/windows/).',
        '',
        NOT_AFFILIATED,
        '',
        'What is in this folder',
        '  build\\      the programs (chase-*.exe) and the libraries they need',
        '  tools\\      the Python tools that read your game image',
        '  config\\     the key bindings',
        '  licenses\\   the licenses of the bundled third-party libraries',
        '  LICENSE     GNU General Public License, version 3 or later, for the programs and tools',
        '',
        'Step 1: extract the game files from your own image (it is never changed). Open a terminal in this folder and run',
        '  py -3 tools\\extract_romfs.py "C:\\path\\to\\game.cci" --output artifacts\\romfs',
        'The output folder must not exist yet. The tools refuse an encrypted image.',
        '',
        'Step 2: start the development runtime.',
        '  ' + start,
        'The runtime reads artifacts\\romfs of the folder it is started in. It saves to %APPDATA%\\chase\\savegame.dat.',
        '',
        'More tools (unpacking the asset archive, converting textures, sounds and text) are described in the project documentation:',
        repository + '/blob/' + commit + '/docs/extracting-game-data.md',
        '',
        'Source code (GPL source offer): ' + repository,
        'This build was made from commit ' + commit + ': ' + repository + '/tree/' + commit,
        '',
    ]
    return '\n'.join(lines)


def launcher_text() -> str:
    return '@echo off\r\ncd /d "%~dp0"\r\nbuild\\chase-game.exe %*\r\nif errorlevel 1 pause\r\n'


def collect(root: Path, build: Path, runtime_dir: Path, version: str, commit: str, repository: str, runner) -> list[Entry]:
    resolved_build = build.resolve()
    if 'artifacts' in resolved_build.relative_to(root.resolve()).parts if resolved_build.is_relative_to(root.resolve()) else False:
        raise PackagingError('the build folder is inside artifacts/: %s' % build)
    programs = program_entries(build)
    has_game = any(entry.name == 'build/chase-game.exe' for entry in programs)
    entries = list(programs)
    tools = root / 'tools'
    entries.extend(Entry('tools/' + path.name, path, base=tools) for path in tool_files(tools))
    for name in ('controls.cfg', 'controls.default.cfg'):
        entries.append(Entry('config/' + name, root / 'config' / name, base=root / 'config'))
    entries.append(Entry('LICENSE', root / 'LICENSE', base=root))
    entries.extend(license_entries([entry for entry in programs if entry.name.endswith('.dll')], runtime_dir, runner))
    entries.append(Entry('README.txt', None, data=readme_text(version, commit, repository, has_game).encode('utf-8')))
    if has_game:
        entries.append(Entry('chase-game.bat', None, data=launcher_text().encode('utf-8')))
    return entries


def write_zip(entries: list[Entry], root: Path, destination: Path, top: str) -> None:
    names = [entry.name for entry in entries]
    if len(set(names)) != len(names):
        raise PackagingError('a file is in the zip twice')
    for entry in entries:
        check_entry(entry, root)
    with zipfile.ZipFile(destination, 'w', zipfile.ZIP_DEFLATED) as archive:
        for entry in sorted(entries, key=lambda item: item.name):
            info = zipfile.ZipInfo(top + '/' + entry.name, FIXED_TIME)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = (0o755 if entry.executable else 0o644) << 16
            archive.writestr(info, entry.content())


def verify_zip(destination: Path, top: str) -> list[str]:
    with zipfile.ZipFile(destination) as archive:
        names = archive.namelist()
        for name in names:
            if not name.startswith(top + '/'):
                raise PackagingError('outside the top folder: %s' % name)
            inner = name[len(top) + 1:]
            checked_name(inner)
            path_name = inner[len('build/'):] if inner.startswith('build/') else inner
            reason = forbidden_files.forbidden_reason(path_name, None)
            if reason:
                raise PackagingError('%s is refused (%s)' % (name, reason))
            if not BINARY_NAMES.match(inner):
                reason = forbidden_files.forbidden_reason(path_name, archive.read(name)[:HEAD_BYTES])
                if reason:
                    raise PackagingError('%s is refused (%s)' % (name, reason))
    return names


def existing_directory(text: str) -> Path:
    path = Path(text)
    if not path.is_dir():
        raise argparse.ArgumentTypeError('not a directory: %s' % text)
    return path


def version_text(text: str) -> str:
    if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9._-]*', text):
        raise argparse.ArgumentTypeError('not a version label (letters, digits, . _ -): %r' % text)
    return text


def commit_text(text: str) -> str:
    if not re.fullmatch(r'[0-9a-f]{40}', text):
        raise argparse.ArgumentTypeError('not a full commit hash (40 lowercase hex digits): %r' % text)
    return text


def repository_text(text: str) -> str:
    if not re.fullmatch(r'https://[A-Za-z0-9._/-]+[A-Za-z0-9_-]', text):
        raise argparse.ArgumentTypeError('not an https repository address without a trailing slash: %r' % text)
    return text


def parse_arguments(argv):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--version', type=version_text, required=True, help='label in the zip name, a tag or a short commit')
    parser.add_argument('--commit', type=commit_text, required=True, help='full hash of the commit the build was made from')
    parser.add_argument('--repository', type=repository_text, required=True, help='https address of the source repository')
    parser.add_argument('--output-dir', type=Path, required=True, help='folder for the zip (created when missing)')
    parser.add_argument('--build-dir', type=existing_directory, default=ROOT / 'build', help='folder with the programs (default: build)')
    parser.add_argument('--runtime-dir', type=existing_directory, default=None,
                        help='MSYS2 bin folder the build copied the DLLs from (default: the folder of gcc)')
    parser.add_argument('--root', type=existing_directory, default=ROOT, help='repository (default: this checkout)')
    return parser.parse_args(argv)


def main(argv=None, runner=default_runner) -> int:
    arguments = parse_arguments(argv)
    runtime_dir = arguments.runtime_dir
    if runtime_dir is None:
        gcc = shutil.which('gcc')
        if gcc is None:
            print('--runtime-dir is needed: gcc was not found on the PATH', file=sys.stderr)
            return 2
        runtime_dir = Path(gcc).parent
    top = 'chase-windows-' + arguments.version
    destination = arguments.output_dir / (top + '.zip')
    if destination.exists():
        print('refusing to overwrite %s' % destination, file=sys.stderr)
        return 2
    try:
        entries = collect(arguments.root, arguments.build_dir, runtime_dir, arguments.version, arguments.commit,
                          arguments.repository, runner)
        arguments.output_dir.mkdir(parents=True, exist_ok=True)
        try:
            write_zip(entries, arguments.root, destination, top)
            names = verify_zip(destination, top)
        except BaseException:
            destination.unlink(missing_ok=True)
            raise
    except PackagingError as error:
        print('package_windows: %s' % error, file=sys.stderr)
        return 1
    if not any(entry.name.endswith('.dll') and not Path(entry.name).name.startswith('libchase_') for entry in entries):
        print('warning: no runtime DLLs were found next to the programs; the zip will not start on a clean machine', file=sys.stderr)
    print('%s: %d files' % (destination, len(names)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
