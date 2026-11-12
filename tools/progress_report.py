#!/usr/bin/env python3
"""Progress report of the PC port: objective measures against numbers taken from the game itself.

    python3 tools/progress_report.py                              # short table
    python3 tools/progress_report.py --json report.json           # full data
    python3 tools/progress_report.py --update-readme README.md    # rewrite the marked README section
    python3 tools/progress_report.py --check README.md            # exit 1 when the README section is stale

Decoded game data comes from the C++ library through tools/chase_native.py. Definitions: docs/progress.md.
"""
import argparse
import bisect
import csv
import datetime
import json
import re
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(Path(__file__).resolve().parent))

import generated_file_census  # noqa: E402
from progress_trend import END_MARKER, START_MARKER, TREND_IMAGE, trend_is_current, write_trend  # noqa: E402

SKIP_EXIT = 77
CODE_ADDRESS = re.compile(r'(?<![0-9A-Za-z_])0x([0-9a-fA-F]{6})(?![0-9A-Za-z_])')
FEATURE_STATUS_WORDS = ('NOT_STARTED', 'IN_PROGRESS', 'DONE', 'BLOCKED')


def percent(part, whole):
    return 100.0 * part / whole if whole else 0.0


def ratio(part, whole):
    return {'numerator': part, 'denominator': whole, 'percent': round(percent(part, whole), 2)}


def format_percent(value):
    if 0 < value < 0.1:
        return '<0.1%'
    return '%.1f%%' % value


def skipped(reason):
    return {'skipped': reason}


def load_archive_files(romfs):
    """[(archive path, decoded size)] of every archive entry, named through the shipped CSV index."""
    import chase_native
    names = {}
    with open(romfs / 'lego_city3ds.csv', newline='', encoding='utf-8') as handle:
        rows = csv.reader(handle)
        next(rows)
        for row in rows:
            names[int(row[0], 16)] = row[1].strip().lower()
    with chase_native.FuseArchive(romfs / 'lego_city3ds.fib') as archive:
        return [(names.get(crc, ''), size) for crc, _offset, size, _flags in archive.entries]


def extension_of(path):
    name = path.rsplit('/', 1)[-1]
    return name.rsplit('.', 1)[-1] if '.' in name else '(none)'


def measure_files(romfs):
    import chase_native
    files = load_archive_files(romfs)
    by_type = {}
    for path, size in files:
        entry = by_type.setdefault(extension_of(path) if path else '(unnamed)', {'files': 0, 'bytes': 0, 'decoded_files': 0,
                                                                                 'decoded_bytes': 0, 'decoders': set()})
        entry['files'] += 1
        entry['bytes'] += size
        decoder = chase_native.file_decoder(path) if path else ''
        if decoder:
            entry['decoded_files'] += 1
            entry['decoded_bytes'] += size
            entry['decoders'].add(decoder)
    for entry in by_type.values():
        entry['decoders'] = sorted(entry['decoders'])
    total_files = sum(e['files'] for e in by_type.values())
    total_bytes = sum(e['bytes'] for e in by_type.values())
    decoded_files = sum(e['decoded_files'] for e in by_type.values())
    decoded_bytes = sum(e['decoded_bytes'] for e in by_type.values())
    return {'files': ratio(decoded_files, total_files), 'bytes': ratio(decoded_bytes, total_bytes),
            'by_type': dict(sorted(by_type.items(), key=lambda item: -item[1]['bytes']))}


def summarise_census(level_files, classes, functions):
    standin = sum(1 for f in functions.values() if f['support'] == 'stand-in')
    implemented = sum(1 for f in functions.values() if f['support'] == 'implemented')
    total_calls = sum(f['calls'] for f in functions.values())
    implemented_calls = sum(f['calls'] for f in functions.values() if f['support'] == 'implemented')
    standin_calls = sum(f['calls'] for f in functions.values() if f['support'] == 'stand-in')
    behaving_classes = sum(1 for c in classes.values() if c['behaviours'])
    instances = sum(c['instances'] for c in classes.values())
    behaving_instances = sum(c['behaving'] for c in classes.values())
    return {
        'level_files': level_files,
        'script_functions': {
            'distinct': ratio(implemented, len(functions)),
            'distinct_with_stand_in': ratio(implemented + standin, len(functions)),
            'calls': ratio(implemented_calls, total_calls),
            'calls_with_stand_in': ratio(implemented_calls + standin_calls, total_calls),
            'per_function': dict(sorted(functions.items(), key=lambda item: -item[1]['calls'])),
        },
        'object_classes': {
            'distinct': ratio(behaving_classes, len(classes)),
            'instances': ratio(behaving_instances, instances),
            'per_class': dict(sorted(classes.items(), key=lambda item: -item[1]['instances'])),
        },
    }


def measure_levels(romfs):
    import chase_native
    fib, names = romfs / 'lego_city3ds.fib', romfs / 'lego_city3ds.csv'
    everything = summarise_census(*chase_native.level_census(fib, names, False))
    main = summarise_census(*chase_native.level_census(fib, names, True))
    del main['script_functions']['per_function']
    del main['object_classes']['per_class']
    return everything, main


def parse_feature_file(text):
    """Status, open and done item IDs and stand-in IDs of one docs/plans/features/<feature>.md."""
    status_line = ''
    lines = text.splitlines()
    for index, line in enumerate(lines):
        if re.match(r'##\s+2\.\s+Status', line):
            for following in lines[index + 1:]:
                if following.strip():
                    status_line = following
                    break
            break
    sentence = re.split(r'\.\s|\.$', status_line)[0]
    found = [word for word in FEATURE_STATUS_WORDS if re.search(r'\b%s\b' % word, sentence)]
    status = found[0] if len(found) == 1 else ('MIXED' if found else 'UNKNOWN')
    open_ids, done_ids, standin_ids = [], [], []
    for line in lines:
        item = re.match(r'\s*-\s+\[( |x|X)\]\s+([A-Z]+-\d+)\b', line)
        if item:
            (open_ids if item.group(1) == ' ' else done_ids).append(item.group(2))
            continue
        standin = re.match(r'\s*-\s+(?:\[[ xX]\]\s+)?([A-Z]+-S\d+)\b', line)
        if standin:
            standin_ids.append(standin.group(1))
    return {'status': status, 'open': sorted(set(open_ids)), 'done': sorted(set(done_ids)), 'stand_ins': sorted(set(standin_ids))}


def parse_feature_index(text):
    """[(name, status)] of the rows of the feature index table that have no feature file."""
    rows = []
    for line in text.splitlines():
        match = re.match(r'\|\s*([^|]*\(no file yet\))\s*\|\s*([^|]*?)\s*\|', line)
        if match:
            rows.append((match.group(1), match.group(2)))
    return rows


def feature_base_name(stem, stems):
    candidates = [other for other in stems if stem.startswith(other + '-')]
    return min(candidates, key=len) if candidates else stem


def measure_features(root):
    directory = root / 'docs' / 'plans' / 'features'
    paths = [p for p in sorted(directory.glob('*.md')) if p.name != 'README.md']
    closed_suffix = '.closed'
    stems = [p.stem for p in paths if not p.stem.endswith(closed_suffix)]
    features = {}
    for path in paths:
        closed = path.stem.endswith(closed_suffix)
        owner = feature_base_name(path.stem[:-len(closed_suffix)] if closed else path.stem, stems)
        parsed = parse_feature_file(path.read_text(encoding='utf-8'))
        if owner not in features:
            features[owner] = {'status': 'UNKNOWN', 'open': [], 'done': [], 'stand_ins': []}
        merged = features[owner]
        if path.stem == owner:
            merged['status'] = parsed['status']
        for key in ('open', 'done', 'stand_ins'):
            merged[key] = sorted(set(merged[key]) | set(parsed[key]))
    index = directory / 'README.md'
    without_file = parse_feature_index(index.read_text(encoding='utf-8')) if index.exists() else []
    return {
        'features': len(features) + len(without_file),
        'open': sum(len(f['open']) for f in features.values()),
        'done': sum(len(f['done']) for f in features.values()),
        'stand_ins': sum(len(f['stand_ins']) for f in features.values()),
        'without_file': [{'name': name, 'status': status} for name, status in without_file],
        'per_feature': features,
    }


def cmake_blocks(text, call):
    """Argument text of every `call(...)` in a CMake file, with balanced parentheses."""
    blocks = []
    for match in re.finditer(r'\b%s\(' % re.escape(call), text):
        depth, position = 1, match.end()
        while depth and position < len(text):
            depth += {'(': 1, ')': -1}.get(text[position], 0)
            position += 1
        blocks.append(text[match.end():position - 1])
    return blocks


DIFFERENTIAL_TESTS = frozenset({
    'locomotion-real-data', 'collision-real-data', 'fall-real-data', 'jump-real-data', 'slide-real-data', 'slope-real-data',
    'camera-real-data', 'camera-blend-real-data', 'camera-vehicle-real-data', 'camera-sweep-real-data',
    'studs-real-data', 'hud-real-data', 'audio-real-data',
})


def measure_checks(root):
    cmake = (root / 'CMakeLists.txt').read_text(encoding='utf-8')
    names = set()
    for block in cmake_blocks(cmake, 'add_test'):
        name = re.search(r'NAME\s+(\S+)', block)
        if name:
            names.add(name.group(1))
    data = root / 'tests' / 'data'
    files = [p for p in data.rglob('*') if p.is_file()] if data.is_dir() else []
    recordings = [p for p in files if p.suffix in ('.csv', '.mem')]
    for table in (p for p in files if p.suffix == '.inc'):
        groups = set(re.findall(r'^\{"([^"@]+)', table.read_text(encoding='utf-8'), re.M))
        recordings.extend((table, group) for group in groups)
    python_tests = sum(len(re.findall(r'^\s+def test_', p.read_text(encoding='utf-8'), re.M)) for p in (root / 'tests').glob('test_*.py'))
    return {
        'ctest_tests': len(names),
        'ctest_tests_with_traces': len(names & DIFFERENTIAL_TESTS),
        'recorded_traces': len(recordings),
        'python_unit_tests': python_tests,
    }


def elf_segments(data):
    phoff, = struct.unpack_from('<I', data, 0x1c)
    phentsize, phnum = struct.unpack_from('<HH', data, 0x2a)
    segments = []
    for index in range(phnum):
        kind, offset, vaddr, _paddr, filesz, _memsz, flags, _align = struct.unpack_from('<8I', data, phoff + index * phentsize)
        if kind == 1:
            segments.append({'offset': offset, 'address': vaddr, 'size': filesz, 'executable': bool(flags & 1)})
    return segments


def function_entries(data):
    """Heuristic function starts of the ARM code: targets of BL / BLX immediate calls and code pointers in data."""
    segments = elf_segments(data)
    text = next(s for s in segments if s['executable'])
    base, size = text['address'], text['size']
    words = struct.unpack_from('<%dI' % (size // 4), data, text['offset'])
    entries = {base}
    for index, word in enumerate(words):
        if (word >> 25) == 0x7D:
            offset = word & 0xFFFFFF
            adjust = ((word >> 24) & 1) * 2
        elif (word >> 28) != 0xF and (word >> 24) & 0xF == 0xB:
            offset = word & 0xFFFFFF
            adjust = 0
        else:
            continue
        if offset & 0x800000:
            offset -= 1 << 24
        target = base + index * 4 + 8 + offset * 4 + adjust
        if base <= target < base + size:
            entries.add(target)
    for segment in segments:
        if segment['executable']:
            continue
        for (value,) in struct.iter_unpack('<I', data[segment['offset']:segment['offset'] + segment['size'] // 4 * 4]):
            if base <= value < base + size and value % 4 == 0:
                entries.add(value)
    return base, size, sorted(entries)


def cited_addresses(root):
    sources = list((root / 'docs' / 're').glob('*.md')) + [p for p in (root / 'src').rglob('*') if p.suffix in ('.cpp', '.hpp')]
    cited = set()
    for path in sources:
        for match in CODE_ADDRESS.finditer(path.read_text(encoding='utf-8', errors='replace')):
            cited.add(int(match.group(1), 16))
    return cited


def measure_code(root, elf):
    data = elf.read_bytes()
    base, size, entries = function_entries(data)
    cited = {a for a in cited_addresses(root) if base <= a < base + size}
    functions = {entries[bisect.bisect_right(entries, a) - 1] for a in cited}
    return {'functions': ratio(len(functions), len(entries)), 'cited_code_addresses': len(cited),
            'text_bytes': size, 'method': 'BL/BLX targets plus code pointers in data; address to containing function by nearest entry below'}


def git_info(root):
    def run(*args):
        try:
            return subprocess.run(['git', '-C', str(root), *args], capture_output=True, text=True, check=True).stdout.strip()
        except (OSError, subprocess.CalledProcessError):
            return ''
    return {'commit': run('rev-parse', 'HEAD'), 'short': run('rev-parse', '--short', 'HEAD')}


def ratio_from_table(table, total_key, part_key):
    return ratio(int(table[('readme_progress', part_key)]), int(table[('readme_progress', total_key)]))


def summary_from_table(table, prefix):
    def cell(total, part):
        return ratio_from_table(table, '%s_%s' % (prefix, total), '%s_%s' % (prefix, part))
    return {
        'level_files': int(table[('readme_progress', '%s_level_files' % prefix)]),
        'script_functions': {'distinct': cell('script_functions_total', 'script_functions_implemented'),
                             'distinct_with_stand_in': cell('script_functions_total', 'script_functions_with_stand_in'),
                             'calls': cell('script_calls_total', 'script_calls_implemented'),
                             'calls_with_stand_in': cell('script_calls_total', 'script_calls_with_stand_in')},
        'object_classes': {'distinct': cell('classes_total', 'classes_behaving'), 'instances': cell('instances_total', 'instances_behaving')},
    }


def measurements_from_table(table):
    everything, main = summary_from_table(table, 'all_levels'), summary_from_table(table, 'main_levels')
    levels = {'level_files': everything['level_files'], 'main_level_files': main['level_files']}
    files = {'files': ratio_from_table(table, 'files_total', 'files_decoded'), 'bytes': ratio_from_table(table, 'bytes_total', 'bytes_decoded'),
             'by_type': {}}
    code = {'functions': ratio_from_table(table, 'code_functions_total', 'code_functions_documented'),
            'cited_code_addresses': int(table[('readme_progress', 'code_cited_addresses')]),
            'text_bytes': int(table[('readme_progress', 'code_text_bytes')]), 'method': 'recorded in the census table'}
    return levels, everything, main, files, code


def collect(root, romfs, elf, census=None, use_census=False):
    report = {'timestamp': datetime.datetime.now().astimezone().isoformat(timespec='seconds'), 'git': git_info(root)}
    import chase_native
    live = (not use_census and (romfs / 'lego_city3ds.fib').exists() and (romfs / 'lego_city3ds.csv').exists()
            and chase_native.available() and elf.exists())
    if not live and census is not None:
        try:
            levels, everything, main, files, code = measurements_from_table(census)
        except KeyError as error:
            raise generated_file_census.CensusError('the census table lacks %s %s' % error.args[0])
        report['levels'] = levels
        report['scripts'] = {'all_levels': everything['script_functions'], 'main_levels': main['script_functions']}
        report['classes'] = {'all_levels': everything['object_classes'], 'main_levels': main['object_classes']}
        report['files'] = files
        report['code'] = code
    elif not (romfs / 'lego_city3ds.fib').exists() or not (romfs / 'lego_city3ds.csv').exists():
        reason = 'game data not extracted (%s)' % romfs
        report['scripts'] = report['classes'] = report['files'] = skipped(reason)
        report['code'] = measure_code(root, elf) if elf.exists() else skipped('analysis ELF not extracted (%s)' % elf)
    elif not chase_native.available():
        reason = 'C++ library not built (%s)' % chase_native.library_path()
        report['scripts'] = report['classes'] = report['files'] = skipped(reason)
        report['code'] = measure_code(root, elf) if elf.exists() else skipped('analysis ELF not extracted (%s)' % elf)
    else:
        everything, main = measure_levels(romfs)
        report['levels'] = {'level_files': everything['level_files'], 'main_level_files': main['level_files']}
        report['scripts'] = {'all_levels': everything['script_functions'], 'main_levels': main['script_functions']}
        report['classes'] = {'all_levels': everything['object_classes'], 'main_levels': main['object_classes']}
        report['files'] = measure_files(romfs)
        report['code'] = measure_code(root, elf) if elf.exists() else skipped('analysis ELF not extracted (%s)' % elf)
    report['features'] = measure_features(root)
    report['checks'] = measure_checks(root)
    return report


def rows_of(report):
    """[(measure, value, meaning)] in README order; None for a measure that could not be taken."""
    rows = []
    scripts, classes, files, code = report['scripts'], report['classes'], report['files'], report['code']
    if 'skipped' in scripts:
        rows.append(('Script commands', None, 'The level scripts call commands such as EnableObject. How many the port runs.'))
    else:
        d, c = scripts['all_levels']['distinct'], scripts['all_levels']['calls']
        extra = scripts['all_levels']['distinct_with_stand_in']['numerator'] - d['numerator']
        rows.append(('Script commands', '%d of %d (%s)' % (d['numerator'], d['denominator'], format_percent(d['percent'])),
                     'Different commands the level scripts call that the port runs. They cover %s of all calls. %d more run with a stand-in result.'
                     % (format_percent(c['percent']), extra)))
    if 'skipped' in classes:
        rows.append(('Object types', None, 'The levels place objects of many types. How many have recovered behaviour.'))
    else:
        d, i = classes['all_levels']['distinct'], classes['all_levels']['instances']
        rows.append(('Object types', '%d of %d (%s)' % (d['numerator'], d['denominator'], format_percent(d['percent'])),
                     'Different object types in the levels that the port treats with recovered rules (collision, doors, studs). They cover %s of all placed objects.'
                     % format_percent(i['percent'])))
    if 'skipped' in files:
        rows.append(('Game files read', None, 'Share of the files in the game archive that the port can decode.'))
    else:
        f, b = files['files'], files['bytes']
        rows.append(('Game files read', '%s of files, %s of data' % (format_percent(f['percent']), format_percent(b['percent'])),
                     'Files in the game archive (%d) that the port can decode, by count and by size.' % f['denominator']))
    feats = report['features']
    rows.append(('Open work', '%d open items, %d stand-ins' % (feats['open'], feats['stand_ins']),
                 'Known gaps and temporary guesses in %d feature areas. It has no total to reach; watch the trend.' % feats['features']))
    checks = report['checks']
    rows.append(('Checked against the original', '%d tests, %d recorded traces' % (checks['ctest_tests'], checks['recorded_traces']),
                 'Automatic tests, and recordings of the original game that the tests compare with.'))
    if 'skipped' in code:
        rows.append(('Game code documented', None, 'Functions of the game program that the notes describe.'))
    else:
        r = code['functions']
        rows.append(('Game code documented', '%d of about %d (%s)' % (r['numerator'], r['denominator'], format_percent(r['percent'])),
                     'Functions of the game program that the notes or the source point to. The total is an estimate.'))
    return rows


def render_section(report):
    rows = rows_of(report)
    lines = ['| Measure | Now | What it means |', '|---|---|---|']
    for measure, value, meaning in rows:
        lines.append('| %s | %s | %s |' % (measure, value if value is not None else 'not measured', meaning))
    git = report['git']
    date = report['timestamp'][:10]
    lines += ['', TREND_IMAGE, '', 'Updated %s, commit %s. How the numbers are measured: [docs/progress.md](docs/progress.md).' % (date, git['short'] or 'unknown')]
    return '\n'.join(lines)


def comparable(section):
    return '\n'.join(line for line in section.strip().splitlines() if not line.startswith('Updated '))


def replace_section(readme, section):
    """README text with the text between the progress markers replaced by `section`; ValueError without both markers."""
    start, end = readme.find(START_MARKER), readme.find(END_MARKER)
    if start < 0 or end < start:
        raise ValueError('README has no %s ... %s markers' % (START_MARKER, END_MARKER))
    return readme[:start + len(START_MARKER)] + '\n' + section.strip() + '\n' + readme[end:]


def extract_section(readme):
    start, end = readme.find(START_MARKER), readme.find(END_MARKER)
    if start < 0 or end < start:
        raise ValueError('README has no %s ... %s markers' % (START_MARKER, END_MARKER))
    return readme[start + len(START_MARKER):end].strip()


def render_text(report):
    rows = rows_of(report)
    width = max(len(row[0]) for row in rows)
    lines = ['Progress of the PC port (%s, commit %s)' % (report['timestamp'], report['git']['short'] or 'unknown'), '']
    for measure, value, _meaning in rows:
        lines.append('%-*s  %s' % (width, measure, value if value is not None else 'not measured'))
    for key in ('scripts', 'classes', 'files', 'code'):
        if 'skipped' in report[key]:
            lines.append('')
            lines.append('Skipped %s: %s' % (key, report[key]['skipped']))
    if 'skipped' not in report['scripts']:
        main = report['scripts']['main_levels']
        classes_main = report['classes']['main_levels']
        everything_calls = report['scripts']['all_levels']['calls']
        everything_objects = report['classes']['all_levels']['instances']
        lines += ['', 'All %d level files, weighted by use: %s of script calls, %s of placed objects' % (
            report['levels']['level_files'], format_percent(everything_calls['percent']), format_percent(everything_objects['percent']))]
        lines += ['Only the %d main levels: script commands %d of %d (%s of calls), object types %d of %d (%s of objects)' % (
            report['levels']['main_level_files'], main['distinct']['numerator'], main['distinct']['denominator'],
            format_percent(main['calls']['percent']), classes_main['distinct']['numerator'], classes_main['distinct']['denominator'],
            format_percent(classes_main['instances']['percent']))]
    return '\n'.join(lines)


def missing_data(report):
    return [key for key in ('scripts', 'classes', 'files', 'code') if 'skipped' in report[key]]


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--root', type=Path, default=ROOT, help='repository root (default: this checkout)')
    parser.add_argument('--romfs', type=Path, help='extracted RomFS directory (default: <root>/artifacts/romfs/romfs)')
    parser.add_argument('--elf', type=Path, help='analysis ELF (default: <root>/artifacts/game/game.arm.elf)')
    parser.add_argument('--json', type=Path, metavar='PATH', help='write the full data as JSON')
    parser.add_argument('--update-readme', type=Path, metavar='README', help='rewrite the text between the progress markers')
    parser.add_argument('--check', type=Path, metavar='README', help='exit 1 when the README section differs from a fresh run; 77 without game data')
    parser.add_argument('--census', type=generated_file_census.existing_file, metavar='TABLE',
                        help='values table used when the game data is absent (default: <root>/%s)' % generated_file_census.TABLE_PATH)
    parser.add_argument('--from-census', action='store_true', help='use the values table even when the game data is present')
    args = parser.parse_args(argv)
    root = args.root.resolve()
    romfs = args.romfs or root / 'artifacts' / 'romfs' / 'romfs'
    elf = args.elf or root / 'artifacts' / 'game' / 'game.arm.elf'
    table_path = args.census or generated_file_census.default_table(root)
    if args.from_census and not table_path.is_file():
        parser.error('--from-census: no such file: %s' % table_path)
    try:
        census = generated_file_census.read_census(table_path) if table_path.is_file() else None
    except generated_file_census.CensusError as error:
        parser.error(str(error))
    try:
        report = collect(root, romfs, elf, census, args.from_census)
    except generated_file_census.CensusError as error:
        parser.error(str(error))
    if args.check:
        gaps = missing_data(report)
        if gaps:
            print('progress check skipped: %s' % '; '.join(dict.fromkeys(report[key]['skipped'] for key in gaps)))
            return SKIP_EXIT
        try:
            current = extract_section(args.check.read_text(encoding='utf-8'))
        except ValueError as error:
            print(error)
            return 1
        fresh = render_section(report)
        stale = comparable(current) != comparable(fresh)
        if stale:
            print('README progress section is stale. Run: python3 tools/progress_report.py --update-readme %s\n\nfresh:\n%s' % (args.check, fresh))
        if not trend_is_current(root, fresh):
            print('docs/progress-trend.svg is stale. Run: python3 tools/progress_report.py --update-readme %s' % args.check)
            stale = True
        if stale:
            return 1
        print('README progress section and trend graphic are current')
        return 0
    if args.json:
        args.json.write_text(json.dumps(report, indent=1) + '\n', encoding='utf-8', newline='\n')
    if args.update_readme:
        gaps = missing_data(report)
        if gaps:
            print('not updating the README, missing data: %s' % '; '.join(dict.fromkeys(report[key]['skipped'] for key in gaps)))
            return 2
        readme = args.update_readme.read_text(encoding='utf-8')
        section = render_section(report)
        args.update_readme.write_text(replace_section(readme, section), encoding='utf-8', newline='\n')
        write_trend(root, section)
        print('updated %s and docs/progress-trend.svg' % args.update_readme)
    if not (args.json or args.update_readme):
        print(render_text(report))
    return 0


if __name__ == '__main__':
    sys.exit(main())
