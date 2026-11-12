import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parents[1] / 'tools'))
import generated_file_census as census  # noqa: E402
import priority_report  # noqa: E402
import progress_report  # noqa: E402

TABLE = Path(__file__).parents[1] / census.TABLE_PATH


class TableFormatTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.path = Path(self.tmp.name) / 'table.csv'

    def tearDown(self):
        self.tmp.cleanup()

    def test_round_trip_is_sorted_and_deterministic(self):
        rows = [('priorities', 'f.b.support', 'stand-in', 'census 2026-11-12'), ('readme_progress', 'files_total', 5, 'census 2026-11-12'),
                ('priorities', 'f.a.calls', 3, 'census 2026-11-12')]
        census.write_census(self.path, rows)
        first = self.path.read_bytes()
        census.write_census(self.path, reversed(rows))
        self.assertEqual(first, self.path.read_bytes())
        self.assertEqual(first.decode().splitlines()[0], 'section,key,value,provenance')
        self.assertIn('priorities,f.b.support,"stand-in",census 2026-11-12', first.decode())
        self.assertEqual(census.read_census(self.path), {('priorities', 'f.b.support'): 'stand-in', ('readme_progress', 'files_total'): '5',
                                                         ('priorities', 'f.a.calls'): '3'})

    def test_unchanged_value_keeps_its_provenance(self):
        census.write_census(self.path, [('priorities', 'f.a.calls', 3, 'census 2026-11-12'), ('priorities', 'f.b.calls', 1, 'census 2026-11-12')])
        census.write_census(self.path, [('priorities', 'f.a.calls', 3, 'census 2027-01-01'), ('priorities', 'f.b.calls', 2, 'census 2027-01-01')])
        text = self.path.read_text()
        self.assertIn('f.a.calls,3,census 2026-11-12', text)
        self.assertIn('f.b.calls,2,census 2027-01-01', text)

    def test_provenance_row_changes_only_with_its_section(self):
        old = 'measured 2026-11-12 with x'
        census.write_census(self.path, [('priorities', 'provenance', old, 'p'), ('priorities', 'f.a.calls', 1, 'p')])
        census.write_census(self.path, [('priorities', 'provenance', 'measured 2027-01-01 with x', 'p'), ('priorities', 'f.a.calls', 1, 'p')])
        self.assertIn(old, self.path.read_text())
        census.write_census(self.path, [('priorities', 'provenance', 'measured 2027-01-01 with x', 'p'), ('priorities', 'f.a.calls', 2, 'p')])
        self.assertIn('2027-01-01', self.path.read_text())

    def test_keys_with_awkward_characters_survive(self):
        census.write_census(self.path, [('priorities', 'c.a,b"c.levels', 1, 'census 2026-11-12')])
        self.assertEqual(census.read_census(self.path), {('priorities', 'c.a,b"c.levels'): '1'})

    def test_rejections(self):
        for text, message in (('a,b,c,d\n', 'header'), ('section,key,value,provenance\nnope,k,1,p\n', 'unknown section'),
                              ('section,key,value,provenance\npriorities,k,1\n', 'expected'),
                              ('section,key,value,provenance\npriorities,k,1,p\npriorities,k,2,p\n', 'twice')):
            self.path.write_text(text)
            with self.assertRaisesRegex(census.CensusError, message):
                census.read_census(self.path)


class CommittedTableTests(unittest.TestCase):
    def test_table_is_well_formed_and_carries_provenance(self):
        rows = census.read_rows(TABLE)
        self.assertEqual(rows, sorted(rows, key=lambda row: (row[0], row[1])))
        self.assertTrue(all(row[3] == census.ROW_PROVENANCE for row in rows))
        self.assertEqual({row[0] for row in rows}, {'readme_progress', 'priorities'})

    def test_each_section_states_how_it_was_measured(self):
        table = census.read_census(TABLE)
        for section in ('readme_progress', 'priorities'):
            self.assertIn('generated_file_census.py --refresh', table[(section, census.PROVENANCE_KEY)])

    def test_table_rebuilds_the_report_numbers(self):
        table = census.read_census(TABLE)
        levels, everything, main, files, code = progress_report.measurements_from_table(table)
        self.assertEqual(levels['main_level_files'], 8)
        distinct = everything['script_functions']['distinct']
        self.assertLessEqual(distinct['numerator'], distinct['denominator'])
        self.assertGreater(files['files']['denominator'], 0)
        self.assertGreater(code['functions']['denominator'], code['functions']['numerator'])
        report = {'scripts': {'all_levels': everything['script_functions']}, 'classes': {'all_levels': everything['object_classes']},
                  'files': files, 'code': code,
                  'features': {'open': 1, 'stand_ins': 1, 'features': 1}, 'checks': {'ctest_tests': 1, 'recorded_traces': 1}}
        self.assertEqual(len(progress_report.rows_of(report)), 6)

    def test_table_rebuilds_the_priority_census(self):
        functions, classes = priority_report.census_from_table(census.read_census(TABLE))
        self.assertTrue(all(set(entry) == {'calls', 'levels', 'support'} for entry in functions.values()))
        self.assertTrue(all(set(entry) == {'instances', 'levels'} for entry in classes.values()))
        self.assertIn(next(iter(functions.values()))['support'], ('none', 'implemented', 'stand-in'))

    def test_missing_key_is_reported(self):
        with self.assertRaises(KeyError):
            progress_report.measurements_from_table({})

    def test_generators_reject_a_missing_table_option(self):
        for module in (progress_report, priority_report):
            with self.assertRaises(SystemExit) as stop:
                module.main(['--census', '/nonexistent/table.csv'])
            self.assertEqual(stop.exception.code, 2)


if __name__ == '__main__':
    unittest.main()
