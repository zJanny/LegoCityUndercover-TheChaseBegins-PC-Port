#!/usr/bin/env python3
"""Values table that lets the generated files be rebuilt without game data.

    python3 tools/generated_file_census.py --measure              # print the measured rows
    python3 tools/generated_file_census.py --refresh [TABLE]      # measure and rewrite the table (needs game data)
    python3 tools/generated_file_census.py --check [TABLE]        # exit 1 when the table differs from a measurement; 77 without game data

The README progress section and docs/plans/priorities.md take a few numbers from the game data: the level census, the
archive file coverage and the code function count. They are kept in tests/data/generated_file_census.csv with the columns
section,key,value,provenance. The generators read the table when the game data is absent.
"""
import argparse
import csv
import datetime
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(Path(__file__).resolve().parent))

TABLE_PATH = Path('tests') / 'data' / 'generated_file_census.csv'
HEADER = ('section', 'key', 'value', 'provenance')
SECTIONS = ('readme_progress', 'priorities', 'progress_trend')
SKIP_EXIT = 77
INTEGER = re.compile(r'-?\d+$')
PLAIN_FIELD = re.compile(r'[A-Za-z0-9_./ :+-]+$')
SCOPES = ('all_levels', 'main_levels')
PROVENANCE_KEY = 'provenance'
ROW_PROVENANCE = 'see provenance'


class CensusError(Exception):
    pass


def quote(text):
    return '"%s"' % text.replace('"', '""')


def format_field(text, force_quote=False):
    return quote(text) if force_quote or not PLAIN_FIELD.match(text) else text


def format_row(row):
    section, key, value, provenance = row
    value = str(value)
    return ','.join([format_field(section), format_field(key), value if INTEGER.match(value) else quote(value), format_field(provenance)])


def read_rows(path):
    with open(path, newline='', encoding='utf-8') as handle:
        reader = csv.reader(handle)
        header = next(reader, None)
        if tuple(header or ()) != HEADER:
            raise CensusError('%s: the header must be %s' % (path, ','.join(HEADER)))
        rows = []
        for number, row in enumerate(reader, 2):
            if len(row) != 4 or not all(row[:2]) or not row[3]:
                raise CensusError('%s:%d: expected section,key,value,provenance with a section, key and provenance' % (path, number))
            if row[0] not in SECTIONS:
                raise CensusError('%s:%d: unknown section %s' % (path, number, row[0]))
            rows.append(tuple(row))
    return rows


def read_census(path):
    """{(section, key): value text}; a repeated key is an error."""
    table = {}
    for section, key, value, _provenance in read_rows(path):
        if (section, key) in table:
            raise CensusError('%s: %s %s is listed twice' % (path, section, key))
        table[(section, key)] = value
    return table


def write_census(path, rows):
    """Write sorted rows; a row whose value did not change keeps the provenance already in the file."""
    path = Path(path)
    previous = {(s, k): (v, p) for s, k, v, p in read_rows(path)} if path.exists() else {}
    merged = {}
    for section, key, value, provenance in rows:
        value = str(value)
        old = previous.get((section, key))
        merged[(section, key)] = (value, old[1] if old and old[0] == value else provenance)
    changed = {section for (section, key), (value, _p) in merged.items()
               if key != PROVENANCE_KEY and previous.get((section, key), (None,))[0] != value}
    for (section, key), old in previous.items():
        if key == PROVENANCE_KEY and section not in changed and (section, key) in merged:
            merged[(section, key)] = (old[0], merged[(section, key)][1])
    lines = [','.join(HEADER)] + [format_row((s, k, v, p)) for (s, k), (v, p) in sorted(merged.items())]
    path.write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\n')


def summary_rows(section, prefix, summary, provenance):
    scripts, classes = summary['script_functions'], summary['object_classes']
    values = {
        'script_functions_total': scripts['distinct']['denominator'],
        'script_functions_implemented': scripts['distinct']['numerator'],
        'script_functions_with_stand_in': scripts['distinct_with_stand_in']['numerator'],
        'script_calls_total': scripts['calls']['denominator'],
        'script_calls_implemented': scripts['calls']['numerator'],
        'script_calls_with_stand_in': scripts['calls_with_stand_in']['numerator'],
        'classes_total': classes['distinct']['denominator'],
        'classes_behaving': classes['distinct']['numerator'],
        'instances_total': classes['instances']['denominator'],
        'instances_behaving': classes['instances']['numerator'],
        'level_files': summary['level_files'],
    }
    return [(section, '%s_%s' % (prefix, key), value, provenance) for key, value in values.items()]


def measure_census(root=ROOT, romfs=None, elf=None, today=None):
    """Rows measured from the game data, or None when the data or the C++ library is missing."""
    import chase_native
    import progress_report
    romfs = romfs or root / 'artifacts' / 'romfs' / 'romfs'
    elf = elf or root / 'artifacts' / 'game' / 'game.arm.elf'
    fib, names = romfs / 'lego_city3ds.fib', romfs / 'lego_city3ds.csv'
    if not (fib.exists() and names.exists() and elf.exists() and chase_native.available()):
        return None
    provenance = ROW_PROVENANCE
    everything, main = progress_report.measure_levels(romfs)
    rows = summary_rows('readme_progress', 'all_levels', everything, provenance)
    rows += summary_rows('readme_progress', 'main_levels', main, provenance)
    files = progress_report.measure_files(romfs)
    for name, ratio in (('files', files['files']), ('bytes', files['bytes'])):
        rows.append(('readme_progress', '%s_total' % name, ratio['denominator'], provenance))
        rows.append(('readme_progress', '%s_decoded' % name, ratio['numerator'], provenance))
    code = progress_report.measure_code(root, elf)
    rows += [('readme_progress', 'code_functions_total', code['functions']['denominator'], provenance),
             ('readme_progress', 'code_functions_documented', code['functions']['numerator'], provenance),
             ('readme_progress', 'code_cited_addresses', code['cited_code_addresses'], provenance),
             ('readme_progress', 'code_text_bytes', code['text_bytes'], provenance)]
    _count, classes, functions = chase_native.level_census(fib, names, True)
    for name, entry in functions.items():
        rows += [('priorities', 'f.%s.%s' % (name, field), entry[field], provenance) for field in ('calls', 'levels', 'support')]
    for name, entry in classes.items():
        rows += [('priorities', 'c.%s.%s' % (name, field), entry[field], provenance) for field in ('instances', 'levels')]
    day = (today or datetime.date.today()).isoformat()
    text = 'measured %s with python3 tools/generated_file_census.py --refresh on the extracted game data' % day
    return rows + [(section, PROVENANCE_KEY, text, ROW_PROVENANCE) for section in ('readme_progress', 'priorities')]


def default_table(root):
    return Path(root) / TABLE_PATH


def differing_keys(measured, path):
    stored = read_census(path) if Path(path).exists() else {}
    fresh = {(s, k): str(v) for s, k, v, _p in measured}
    return sorted(key for key in set(stored) | set(fresh) if key[1] != PROVENANCE_KEY and stored.get(key) != fresh.get(key))


def existing_file(text):
    path = Path(text)
    if not path.is_file():
        raise argparse.ArgumentTypeError('not a file: %s' % text)
    return path


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--root', type=Path, default=ROOT, help='repository root (default: this checkout)')
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--measure', action='store_true', help='print the measured rows')
    mode.add_argument('--refresh', nargs='?', const='', metavar='TABLE', help='rewrite the table (default: <root>/%s)' % TABLE_PATH)
    mode.add_argument('--check', nargs='?', const='', metavar='TABLE', help='exit 1 when the table differs from a measurement; 77 without game data')
    args = parser.parse_args(argv)
    root = args.root.resolve()
    rows = measure_census(root)
    if rows is None:
        print('census %s: game data or the C++ library is missing' % ('check skipped' if args.check is not None else 'not measured'))
        return SKIP_EXIT if args.check is not None else 2
    if args.measure:
        print('\n'.join(format_row(row) for row in rows))
        return 0
    path = Path(args.refresh or args.check) if (args.refresh or args.check) else default_table(root)
    if args.check is not None:
        if not path.is_file():
            parser.error('--check: no such file: %s' % path)
        different = differing_keys(rows, path)
        if different:
            print('%s differs from the measurement in %d rows, first: %s %s. Run: python3 tools/generated_file_census.py --refresh'
                  % (path, len(different), *different[0]))
            return 1
        print('%s is current' % path)
        return 0
    write_census(path, rows)
    print('wrote %s' % path)
    return 0


if __name__ == '__main__':
    sys.exit(main())
