import contextlib
import io
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parents[1] / 'tools'))
import priority_report as report  # noqa: E402

FEATURE = """# Sample

## 5. Open items

- [ ] SAMP-1 — `EnableObject` stays a stand-in. needs: SAMP-9. blocks: SAMP-2, SAMP-3. evidence: [x](x.md#ts-20260101-000000).
- [ ] SAMP-2 — Platforms of props/prop_platform do not move. blocks: SAMP-4.
- [ ] SAMP-3 — The camera looks wrong. blocks: SAMP-1.
- [ ] SAMP-4 — The WarningDialogBox is missing. uses: enableobject, Prop_Platform. evidence: e.
- [x] SAMP-5 — closed item, not ranked.
- SAMP-S1 A guess for `wait` in 1-8_rejoinfight. blocks: SAMP-S1.
- SAMP-6 Plain words camera and wait do not link.
- [ ] SAMP-7 — Only `props/prop_platform` here, from src/camera.cpp.
"""

FUNCTIONS = {
    'enableobject': {'support': 'stand-in', 'calls': 10, 'levels': 3},
    'warningdialogbox': {'support': 'none', 'calls': 2, 'levels': 1},
    'wait': {'support': 'implemented', 'calls': 100, 'levels': 8},
    '1-8_rejoinfight': {'support': 'none', 'calls': 5, 'levels': 1},
}
CLASSES = {
    'props/prop_platform': {'instances': 7, 'levels': 4, 'behaviours': [], 'behaving': 0},
    'camera': {'instances': 8, 'levels': 8, 'behaviours': [], 'behaving': 0},
}


def rows_of(text):
    return {row['id']: row for row in report.measure(report.parse_items(text, 'sample.md'), FUNCTIONS, CLASSES)}


class ParsingTests(unittest.TestCase):
    def test_items_and_stand_ins_without_closed_ones(self):
        items = report.parse_items(FEATURE, 'sample.md')
        self.assertEqual([item['id'] for item in items], ['SAMP-1', 'SAMP-2', 'SAMP-3', 'SAMP-4', 'SAMP-S1', 'SAMP-6', 'SAMP-7'])
        first = items[0]
        self.assertEqual((first['needs'], first['blocks'], first['uses']), (['SAMP-9'], ['SAMP-2', 'SAMP-3'], None))
        self.assertEqual(items[3]['uses'], ['enableobject', 'Prop_Platform'])

    def test_fields_stop_at_the_next_field(self):
        gap, fields = report.split_fields('gap text. blocks: A-1, A-2. uses: X, Y. evidence: [l](blocks: f.md)')
        self.assertEqual(gap, 'gap text. ')
        self.assertEqual(fields['blocks'], 'A-1, A-2.')
        self.assertEqual(fields['uses'], 'X, Y.')

    def test_areas_follow_the_index_headings(self):
        areas = report.parse_areas('# Index\n\n## One\n\n| F | S | T |\n|---|---|---|\n| [a](a.md) | X | y |\n\n## Empty\n\n## Two\n\n| [b](b.md) | X | y |\n')
        self.assertEqual(areas, [('One', ['a.md']), ('Two', ['b.md'])])


class LinkingTests(unittest.TestCase):
    def test_automatic_links_need_backticks_or_distinctive_names(self):
        rows = rows_of(FEATURE)
        self.assertEqual((rows['SAMP-1']['natives'], rows['SAMP-1']['calls']), (['enableobject'], 10))
        self.assertEqual((rows['SAMP-2']['classes'], rows['SAMP-2']['placed']), (['props/prop_platform'], 7))
        self.assertEqual(rows['SAMP-3']['linked'], False)
        self.assertEqual(rows['SAMP-6']['linked'], False)
        self.assertEqual(rows['SAMP-S1']['natives'], ['1-8_rejoinfight', 'wait'])
        self.assertEqual(rows['SAMP-7']['classes'], ['props/prop_platform'])
        self.assertEqual(rows['SAMP-7']['placed'], 7)

    def test_uses_replaces_the_automatic_link(self):
        row = rows_of(FEATURE)['SAMP-4']
        self.assertEqual((row['natives'], row['classes']), (['enableobject'], ['props/prop_platform']))
        self.assertEqual((row['calls'], row['placed'], row['levels']), (10, 7, 4))

    def test_last_path_part_and_case_are_accepted_in_uses(self):
        row = rows_of('- [ ] A-1 — gap. uses: PROP_PLATFORM, Camera\n')['A-1']
        self.assertEqual(sorted(row['classes']), ['camera', 'props/prop_platform'])

    def test_unknown_name_in_uses_names_the_item_and_the_name(self):
        with self.assertRaises(report.PriorityError) as caught:
            rows_of('- [ ] A-1 — gap. uses: EnableObject, NoSuchThing\n- [ ] A-2 — gap. uses: AlsoMissing\n')
        message = str(caught.exception)
        self.assertIn('A-1: unknown name in uses: NoSuchThing', message)
        self.assertIn('A-2: unknown name in uses: AlsoMissing', message)


class UnblocksTests(unittest.TestCase):
    def test_chains_are_transitive_and_counted_once(self):
        rows = rows_of(FEATURE)
        self.assertEqual(rows['SAMP-1']['unblocks'], 3)
        self.assertEqual(rows['SAMP-2']['unblocks'], 1)

    def test_cycles_terminate(self):
        rows = rows_of(FEATURE)
        self.assertEqual(rows['SAMP-3']['unblocks'], 3)
        self.assertEqual(rows['SAMP-S1']['unblocks'], 0)

    def test_needs_counts_like_blocks_on_the_other_side(self):
        items = report.parse_items('- [ ] A-1 — gap.\n- [ ] A-2 — gap. needs: A-1.\n- [ ] A-3 — gap. needs: A-2.\n', 'f.md')
        self.assertEqual(report.reachable_counts(items), {'A-1': 2, 'A-2': 1, 'A-3': 0})


class RenderTests(unittest.TestCase):
    def test_output_is_deterministic_and_ordered(self):
        items = report.parse_items(FEATURE, 'sample.md')
        areas = [('Area', ['sample.md'])]
        first = report.render(report.measure(items, FUNCTIONS, CLASSES), areas)
        second = report.render(report.measure(list(reversed(items)), FUNCTIONS, CLASSES), areas)
        self.assertEqual(first, second)
        table = first.split('## Top 50 overall')[1]
        self.assertLess(table.index('SAMP-S1'), table.index('SAMP-1'))
        self.assertLess(table.index('SAMP-1'), table.index('SAMP-2'))
        self.assertIn('## Reach unknown', first)
        self.assertIn('[sample.md](features/sample.md): SAMP-3 (+3)', first)

    def test_item_links_point_at_the_item_line_of_the_source_view(self):
        items = report.parse_items(FEATURE, 'sample.md')
        self.assertEqual([item['line'] for item in items][:2], [5, 6])
        text = report.render(report.measure(items, FUNCTIONS, CLASSES), [('Area', ['sample.md'])])
        self.assertIn('| [SAMP-1](features/sample.md?display=source#L5) |', text)
        self.assertIn('[SAMP-3](features/sample.md?display=source#L7) (+3)', text)


class ArgumentTests(unittest.TestCase):
    def run_main(self, *args):
        output, errors = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(output), contextlib.redirect_stderr(errors):
            try:
                code = report.main(list(args))
            except SystemExit as stop:
                code = stop.code
        return code, output.getvalue(), errors.getvalue()

    def test_unknown_option_is_rejected(self):
        code, _out, err = self.run_main('--bogus')
        self.assertEqual(code, 2)
        self.assertIn('--bogus', err)

    def test_missing_root_is_rejected(self):
        code, _out, err = self.run_main('--root', '/no/such/directory')
        self.assertEqual(code, 2)
        self.assertIn('/no/such/directory', err)

    def test_check_and_update_exclude_each_other(self):
        code, _out, err = self.run_main('--check', 'a', '--update', 'b')
        self.assertEqual(code, 2)
        self.assertIn('not allowed with', err)

    def test_check_of_a_missing_file_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            code, _out, err = self.run_main('--check', str(Path(directory) / 'missing.md'))
        self.assertEqual(code, 2)
        self.assertIn('missing.md', err)

    def test_no_game_data_skips_the_check(self):
        with tempfile.TemporaryDirectory() as directory:
            target = Path(directory) / 'p.md'
            target.write_text('x')
            code, out, _err = self.run_main('--root', directory, '--romfs', directory, '--check', str(target))
        self.assertEqual(code, report.SKIP_EXIT)
        self.assertIn('game data not extracted', out)


if __name__ == '__main__':
    unittest.main()
