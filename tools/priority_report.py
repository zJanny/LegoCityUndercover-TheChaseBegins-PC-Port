#!/usr/bin/env python3
"""Ranking of the open items and stand-ins by their reach in the main levels of the game.

    python3 tools/priority_report.py                                  # print the ranking
    python3 tools/priority_report.py --update docs/plans/priorities.md  # rewrite the file
    python3 tools/priority_report.py --check docs/plans/priorities.md   # exit 1 when the file is stale; 77 without game data

Items come from docs/plans/features/*.md. Script calls and placed objects come from the C++ level census through
tools/chase_native.py. Definitions and limits are in the generated file.
"""
import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(Path(__file__).resolve().parent))

from check_doc_links import ITEM_ID, feature_files  # noqa: E402
from progress_report import SKIP_EXIT  # noqa: E402

DEFAULT_OUTPUT = Path('docs') / 'plans' / 'priorities.md'
FIELD_NAMES = ('needs', 'blocks', 'uses', 'evidence')
FIELD = re.compile(r'(?<![\w-])(%s):' % '|'.join(FIELD_NAMES))
IDENTIFIER = re.compile(r'(?<![\w-])[A-Z][A-Z0-9]*-S?\d+\b')
OPEN_ITEM = re.compile(r'^- (?:\[ \] )?([A-Z][A-Z0-9]*-S?\d+)\b')
LINK = re.compile(r'\[([^\]]*)\]\([^)]*\)')
LINK_TARGET = re.compile(r'\]\([^)]*\)')
BACKTICKS = re.compile(r'`([^`]*)`')
WORD = re.compile(r'[A-Za-z0-9_/]+')
MISSION_NAME = re.compile(r'\d+-\d+_[A-Za-z0-9_]+')
SOURCE_PATH = re.compile(r'(?<![\w/])(?:src|docs|tools|tests)/[\w./-]*')
CAMEL = re.compile(r'[a-z0-9][A-Z]')
FILE_PATH = re.compile(r'^(src|docs|tools|tests)/|\.[A-Za-z]{1,5}$')
TOP_OVERALL = 50
TOP_AREA = 10
GAP_LENGTH = 110
NAME_LIMIT = 4


class PriorityError(Exception):
    pass


def split_fields(text):
    """(gap text, {field: text}) of one item line; a field runs up to the next field."""
    matches = list(FIELD.finditer(text))
    gap = text[:matches[0].start()] if matches else text
    fields = {}
    for index, match in enumerate(matches):
        end = matches[index + 1].start() if index + 1 < len(matches) else len(text)
        fields.setdefault(match.group(1), text[match.end():end].strip())
    return gap, fields


def identifiers_of(text):
    return IDENTIFIER.findall(LINK_TARGET.sub('', text))


def parse_items(text, file_name):
    """Open items and stand-ins of one feature file: [{id, file, gap, blocks, needs, uses}]."""
    items = []
    for number, line in enumerate(text.splitlines(), 1):
        match = ITEM_ID.match(line)
        if not match or not OPEN_ITEM.match(line):
            continue
        item_id = match.group(1)
        remainder = line[match.end():].lstrip()
        remainder = re.sub(r'^[—-]\s*', '', remainder)
        gap, fields = split_fields(remainder)
        uses = None
        if 'uses' in fields:
            uses = [name.strip().strip('`') for name in fields['uses'].rstrip('.').split(',') if name.strip()]
        items.append({'id': item_id, 'file': file_name, 'line': number, 'text': remainder, 'gap': gap.strip(),
                      'blocks': identifiers_of(fields.get('blocks', '')), 'needs': identifiers_of(fields.get('needs', '')),
                      'uses': uses})
    return items


def load_items(root):
    items = []
    for path in feature_files(root):
        items.extend(parse_items(path.read_text(encoding='utf-8'), path.name))
    return items


def parse_areas(text):
    """[(area, [feature file names])] from the '## <Area>' sections of the feature index."""
    areas = []
    for line in text.splitlines():
        heading = re.match(r'##\s+(.+?)\s*$', line)
        if heading:
            areas.append((heading.group(1), []))
            continue
        row = re.match(r'\|\s*\[[^\]]*\]\(([^)#\s]+\.md)\)', line)
        if row and areas:
            areas[-1][1].append(row.group(1))
    return [(name, files) for name, files in areas if files]


def class_names(classes):
    """{lower-case name: set of class keys} for the full class path and its last path part."""
    names = {}
    for key in classes:
        lowered = key.lower()
        names.setdefault(lowered, set()).add(key)
        names.setdefault(lowered.rsplit('/', 1)[-1], set()).add(key)
    return names


def distinctive(name):
    return '_' in name or '/' in name or any(ch.isdigit() for ch in name)


def accepted_tokens(text):
    """Tokens of the item text that may name a native or a class: every token inside backticks (except file
    paths), and outside them only distinctive tokens (underscore, slash or digit) and CamelCase words."""
    plain = SOURCE_PATH.sub(' ', LINK.sub(lambda m: m.group(1), text))
    tokens = MISSION_NAME.findall(plain)
    for span in BACKTICKS.findall(plain):
        if not FILE_PATH.search(span):
            tokens.extend(WORD.findall(span))
    for token in WORD.findall(BACKTICKS.sub(' ', plain)):
        if distinctive(token) or CAMEL.search(token):
            tokens.append(token)
    return tokens


def automatic_names(text, natives, class_lookup):
    native_hits, class_hits = set(), set()
    for token in accepted_tokens(text):
        for word in {token.lower(), *token.lower().split('/')}:
            if word in natives:
                native_hits.add(word)
            if word in class_lookup:
                class_hits |= class_lookup[word]
    return native_hits, class_hits


def explicit_names(item, natives, class_lookup):
    native_hits, class_hits, unknown = set(), set(), []
    for name in item['uses']:
        lowered = name.lower()
        known = False
        if lowered in natives:
            native_hits.add(lowered)
            known = True
        if lowered in class_lookup:
            class_hits |= class_lookup[lowered]
            known = True
        if not known:
            unknown.append(name)
    return native_hits, class_hits, unknown


def reachable_counts(items):
    """{item id: number of distinct items reachable through blocks: (and the matching needs:) chains}."""
    known = {item['id'] for item in items}
    edges = {item_id: set() for item_id in known}
    for item in items:
        for target in item['blocks']:
            if target in known and target != item['id']:
                edges[item['id']].add(target)
        for source in item['needs']:
            if source in known and source != item['id']:
                edges[source].add(item['id'])
    counts = {}
    for start in known:
        seen, stack = set(), list(edges[start])
        while stack:
            current = stack.pop()
            if current in seen or current == start:
                continue
            seen.add(current)
            stack.extend(edges[current] - seen)
        counts[start] = len(seen)
    return counts


def measure(items, functions, classes):
    """Reach columns for every item; raises PriorityError naming every unknown name in a uses: field."""
    class_lookup = class_names(classes)
    unblocks = reachable_counts(items)
    problems, rows = [], []
    for item in items:
        if item['uses'] is not None:
            native_hits, class_hits, unknown = explicit_names(item, functions, class_lookup)
            problems.extend('%s: %s: unknown name in uses: %s' % (item['file'], item['id'], name) for name in unknown)
        else:
            native_hits, class_hits = automatic_names(item['text'], functions, class_lookup)
        calls = sum(functions[name]['calls'] for name in native_hits)
        placed = sum(classes[key]['instances'] for key in class_hits)
        levels = max([functions[name]['levels'] for name in native_hits] + [classes[key]['levels'] for key in class_hits] + [0])
        states = {}
        for name in native_hits:
            states[functions[name]['support']] = states.get(functions[name]['support'], 0) + 1
        rows.append({**item, 'natives': sorted(native_hits), 'classes': sorted(class_hits), 'calls': calls, 'placed': placed,
                     'levels': levels, 'unblocks': unblocks[item['id']], 'states': states,
                     'linked': bool(native_hits or class_hits)})
    if problems:
        raise PriorityError('\n'.join(problems))
    return rows


def sort_key(row):
    return (-(row['placed'] + row['calls']), -row['unblocks'], row['id'])


def short_gap(row):
    text = LINK.sub(lambda m: m.group(1), row['gap'])
    text = re.sub(r'\s+', ' ', text).strip().rstrip('.')
    if len(text) > GAP_LENGTH:
        text = text[:GAP_LENGTH].rsplit(' ', 1)[0].rstrip(',;:')
        text += ('`' if text.count('`') % 2 else '') + ' ...'
    return text.replace('|', '\\|')


def state_text(row):
    order = ('stand-in', 'none', 'implemented')
    parts = ['%d %s' % (row['states'][key], key) for key in order if key in row['states']]
    return ', '.join(parts) if parts else '-'


def linked_text(row):
    names = ['`%s`' % name for name in row['natives']] + ['`%s`' % key for key in row['classes']]
    shown = ', '.join(names[:NAME_LIMIT])
    if len(names) > NAME_LIMIT:
        shown += ' +%d' % (len(names) - NAME_LIMIT)
    return shown.replace('|', '\\|')


def item_link(row):
    return '[%s](features/%s?display=source#L%d)' % (row['id'], row['file'], row['line'])


def table(rows):
    lines = ['| # | Item | Calls | Placed | Main levels | Unblocks | Natives | Linked to | Gap |', '|---|---|---|---|---|---|---|---|---|']
    for rank, row in enumerate(rows, 1):
        lines.append('| %d | %s | %d | %d | %d | %d | %s | %s | %s |' % (
            rank, item_link(row), row['calls'], row['placed'], row['levels'], row['unblocks'], state_text(row),
            linked_text(row), short_gap(row)))
    return lines


INTRO = """# Priorities

Generated by `tools/priority_report.py`; do not edit. It is rewritten at every merge (`tools/merge_agent.py`) and checked by the test `priorities-current`.

It ranks the open items and stand-ins of [the feature files](features/README.md) by how much of the game they touch. Use it to pick work with a wide effect on the whole game. It is a hint, not an order of work.

## How it is made

- Every open item (`- [ ] ID`) and stand-in (`PREFIX-S<n>`) of the feature files is read. Closed items are left out.
- Each item is linked to script natives and object classes. The link comes from the item text: a name in backticks, a name with an underscore, a slash or a digit, or a CamelCase word, written like a native (`EnableObject`) or a class (`props/prop_platform` or `prop_platform`). Plain lower-case words such as `camera` or `wait` do not match by themselves. The optional field `uses: Name, Name` replaces the automatic link for that item. A name in `uses:` that is not a native or a class of the level data is an error.
- The numbers come from the C++ level census of the 8 main levels (`*_main.blvl`), read through `tools/chase_native.py`.

## Columns

- **Calls**: script calls of the linked natives in the 8 main levels.
- **Placed**: objects of the linked classes placed in the 8 main levels.
- **Main levels**: the largest number of main levels that use any one linked name. This is a lower bound for the levels that use any of them.
- **Unblocks**: items reachable through `blocks:` chains, each counted once. A `needs:` on an item counts as a `blocks:` on the other side.
- **Natives**: state of the linked natives in the port: `stand-in`, `none` (no handler) or `implemented`.
- **Gap**: the start of the item text.

Rows are ordered by Placed + Calls, then Unblocks. That sum is only an ordering key; the columns are never combined into one score.

## Limits

- Reach is not importance. A blocker of the play path can have a tiny reach, and an item with a large reach can be cosmetic.
- Crowds and traffic that are placed through files that are not decoded yet ([ASSET-3](features/assets-formats.md)) are missing from Placed.
- Rendering, saves, menus and audio items rarely name a native or a class, so they often appear only in the list of items with unknown reach.
- A wrong link from the text is fixed with `uses:` on that item.
"""


def render(rows, areas):
    linked = sorted((row for row in rows if row['linked']), key=sort_key)
    unlinked = [row for row in rows if not row['linked']]
    lines = [INTRO.rstrip(), '', 'Items: %d open items and stand-ins, %d linked to game usage, %d with unknown reach.' % (
        len(rows), len(linked), len(unlinked)), '', '## Top %d overall' % TOP_OVERALL, '']
    lines += table(linked[:TOP_OVERALL])
    area_of = {name: area for area, files in areas for name in files}
    for area, files in areas:
        members = [row for row in linked if area_of.get(row['file']) == area]
        total = sum(1 for row in rows if area_of.get(row['file']) == area)
        lines += ['', '## %s' % area, '', '%d of %d items linked.' % (len(members), total), '']
        lines += table(members[:TOP_AREA]) if members else ['No linked items.']
    other = [row for row in linked if row['file'] not in area_of]
    if other:
        lines += ['', '## Not in the feature index', ''] + table(other[:TOP_AREA])
    lines += ['', '## Reach unknown', '',
              'No script native or object class is linked to these items (`+N` is Unblocks). Add `uses:` to an item when its fix changes the game.', '']
    widest = sorted((row for row in unlinked if row['unblocks']), key=lambda r: (-r['unblocks'], r['id']))[:TOP_AREA]
    if widest:
        lines += ['Widest Unblocks among them: ' + ', '.join('%s (+%d)' % (item_link(r), r['unblocks']) for r in widest), '']
    by_file = {}
    for row in sorted(unlinked, key=lambda r: (-r['unblocks'], r['id'])):
        by_file.setdefault(row['file'], []).append('%s%s' % (row['id'], ' (+%d)' % row['unblocks'] if row['unblocks'] else ''))
    for name in sorted(by_file):
        lines.append('- [%s](features/%s): %s' % (name, name, ', '.join(by_file[name])))
    return '\n'.join(lines) + '\n'


def load_census(romfs):
    import chase_native
    fib, names = romfs / 'lego_city3ds.fib', romfs / 'lego_city3ds.csv'
    if not fib.exists() or not names.exists():
        return None, 'game data not extracted (%s)' % romfs
    if not chase_native.available():
        return None, 'C++ library not built (%s)' % chase_native.library_path()
    _count, classes, functions = chase_native.level_census(fib, names, True)
    return (functions, classes), None


def build(root, functions, classes):
    index = root / 'docs' / 'plans' / 'features' / 'README.md'
    areas = parse_areas(index.read_text(encoding='utf-8')) if index.exists() else []
    return render(measure(load_items(root), functions, classes), areas)


def existing_directory(text):
    path = Path(text)
    if not path.is_dir():
        raise argparse.ArgumentTypeError('not a directory: %s' % text)
    return path


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--root', type=existing_directory, default=ROOT, help='repository root (default: this checkout)')
    parser.add_argument('--romfs', type=existing_directory, help='extracted RomFS directory (default: <root>/artifacts/romfs/romfs)')
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--update', type=Path, metavar='FILE', help='write the ranking to FILE')
    mode.add_argument('--check', type=Path, metavar='FILE', help='exit 1 when FILE differs from a fresh run; 77 without game data')
    args = parser.parse_args(argv)
    root = args.root.resolve()
    romfs = args.romfs or root / 'artifacts' / 'romfs' / 'romfs'
    if args.check and not args.check.is_file():
        parser.error('--check: no such file: %s' % args.check)
    census, reason = load_census(romfs)
    if census is None:
        print('priorities %s: %s' % ('check skipped' if args.check else 'not written', reason))
        return SKIP_EXIT if args.check else 2
    try:
        text = build(root, *census)
    except PriorityError as error:
        print(error, file=sys.stderr)
        return 1
    if args.check:
        if args.check.read_text(encoding='utf-8') != text:
            print('%s is stale. Run: python3 tools/priority_report.py --update %s' % (args.check, args.check))
            return 1
        print('%s is current' % args.check)
        return 0
    if args.update:
        args.update.write_text(text, encoding='utf-8')
        print('updated %s' % args.update)
        return 0
    sys.stdout.write(text)
    return 0


if __name__ == '__main__':
    sys.exit(main())
