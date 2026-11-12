#!/usr/bin/env python3
"""Rebuild the README progress section, docs/plans/priorities.md and docs/progress-trend.svg from tracked files only.

    python3 tools/regenerate_generated_files.py                 # rewrite the files that differ, print their names
    python3 tools/regenerate_generated_files.py --check         # exit 1 when a file differs, write nothing
    python3 tools/regenerate_generated_files.py --skip-marked   # do nothing when the HEAD commit message carries the marker

The numbers that come from game data are read from tests/data/generated_file_census.csv. The "Updated <date>, commit <hash>."
line of the README section is not compared, so a new commit alone never changes a file. The commit that the workflow
`Generated files` makes carries COMMIT_MARKER; with --skip-marked the script then does nothing, which ends any retrigger.
"""
import argparse
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(Path(__file__).resolve().parent))

import generated_file_census  # noqa: E402
import priority_report  # noqa: E402
import progress_report  # noqa: E402
import progress_trend  # noqa: E402

COMMIT_MARKER = '[generated-files]'
README = 'README.md'
PRIORITIES = Path('docs') / 'plans' / 'priorities.md'
TREND = progress_trend.TREND_PATH


def head_message(root):
    result = subprocess.run(['git', '-C', str(root), 'log', '-1', '--format=%B'], capture_output=True, text=True, check=False)
    return result.stdout if result.returncode == 0 else ''


def head_is_marked(root):
    return COMMIT_MARKER in head_message(root)


def regenerate(root, write):
    """Names of the generated files that differ from a fresh run; the files are rewritten when `write`."""
    table = generated_file_census.read_census(generated_file_census.default_table(root))
    missing = root / 'no-game-data'
    report = progress_report.collect(root, missing, missing / 'game.arm.elf', table, use_census=True)
    section = progress_report.render_section(report)
    readme_path = root / README
    readme = readme_path.read_text(encoding='utf-8')
    changed = []
    current = progress_report.extract_section(readme)
    if progress_report.comparable(current) != progress_report.comparable(section):
        changed.append(README)
        current = section
        if write:
            readme_path.write_text(progress_report.replace_section(readme, section), encoding='utf-8')
    if not progress_trend.trend_is_current(root, current):
        changed.append(str(TREND))
        if write:
            progress_trend.write_trend(root, current)
    functions, classes = priority_report.census_from_table(table)
    priorities = priority_report.build(root, functions, classes)
    priorities_path = root / PRIORITIES
    if not priorities_path.exists() or priorities_path.read_text(encoding='utf-8') != priorities:
        changed.append(str(PRIORITIES))
        if write:
            priorities_path.write_text(priorities, encoding='utf-8')
    return changed


def existing_directory(text):
    path = Path(text)
    if not path.is_dir():
        raise argparse.ArgumentTypeError('not a directory: %s' % text)
    return path


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--root', type=existing_directory, default=ROOT, help='repository root (default: this checkout)')
    parser.add_argument('--check', action='store_true', help='exit 1 when a file differs, write nothing')
    parser.add_argument('--skip-marked', action='store_true', help='do nothing when the HEAD commit message carries %s' % COMMIT_MARKER)
    args = parser.parse_args(argv)
    root = args.root.resolve()
    if args.skip_marked and head_is_marked(root):
        print('HEAD carries %s: nothing to do' % COMMIT_MARKER)
        return 0
    try:
        changed = regenerate(root, not args.check)
    except (generated_file_census.CensusError, priority_report.PriorityError, ValueError, OSError) as error:
        print(error, file=sys.stderr)
        return 2
    if not changed:
        print('generated files are current')
        return 0
    print('\n'.join(changed))
    return 1 if args.check else 0


if __name__ == '__main__':
    sys.exit(main())
