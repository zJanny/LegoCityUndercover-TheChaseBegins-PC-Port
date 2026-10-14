import struct
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parents[1] / 'tools'))
import progress_report as report  # noqa: E402

FEATURE = """# Sample

## 2. Status

IN_PROGRESS, updated 2026-01-01T00:00:00+0100. Something works.

## 4. Stand-ins

- SAMP-S1 A guess.
- SAMP-S2 Another guess.

## 5. Open items

- [ ] SAMP-1 — gap. needs: OTHER-2.
- [x] SAMP-2 — closed.
- [ ] SAMP-3 — gap.
  - [ ] SAMP-3 — repeated line counts once.
- not an item
"""

INDEX = """| Feature | Status | Top open items |
|---|---|---|
| [sample](sample.md) | IN_PROGRESS | SAMP-1 |
| missions (no file yet) | NOT_STARTED | mission flow |
"""


class FeatureParserTests(unittest.TestCase):
    def test_counts_items_and_stand_ins(self):
        parsed = report.parse_feature_file(FEATURE)
        self.assertEqual(parsed['status'], 'IN_PROGRESS')
        self.assertEqual(parsed['open'], ['SAMP-1', 'SAMP-3'])
        self.assertEqual(parsed['done'], ['SAMP-2'])
        self.assertEqual(parsed['stand_ins'], ['SAMP-S1', 'SAMP-S2'])

    def test_mixed_and_unknown_status(self):
        mixed = '## 2. Status\n\nS1 DONE, S3 IN_PROGRESS (rest open). More.\n'
        self.assertEqual(report.parse_feature_file(mixed)['status'], 'MIXED')
        self.assertEqual(report.parse_feature_file('no sections here')['status'], 'UNKNOWN')

    def test_index_rows_without_file(self):
        self.assertEqual(report.parse_feature_index(INDEX), [('missions (no file yet)', 'NOT_STARTED')])

    def test_closed_and_split_files_count_for_their_feature(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            features = root / 'docs' / 'plans' / 'features'
            features.mkdir(parents=True)
            (features / 'sample.md').write_text(FEATURE)
            (features / 'sample.closed.md').write_text('# Closed\n\n- [x] SAMP-9 done\n')
            (features / 'sample-extra.md').write_text('## 5. Open items\n\n- [ ] SAMP-10 gap\n- SAMP-S3 guess\n')
            (features / 'sample-extra.closed.md').write_text('- [x] SAMP-11 done\n')
            (features / 'README.md').write_text(INDEX)
            measured = report.measure_features(root)
            self.assertEqual(list(measured['per_feature']), ['sample'])
            self.assertEqual(measured['features'], 2)
            self.assertEqual(measured['open'], 3)
            self.assertEqual(measured['done'], 3)
            self.assertEqual(measured['stand_ins'], 3)
            self.assertEqual(measured['per_feature']['sample']['status'], 'IN_PROGRESS')

    def test_feature_directory(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            features = root / 'docs' / 'plans' / 'features'
            features.mkdir(parents=True)
            (features / 'sample.md').write_text(FEATURE)
            (features / 'README.md').write_text(INDEX)
            measured = report.measure_features(root)
        self.assertEqual((measured['features'], measured['open'], measured['done'], measured['stand_ins']), (2, 2, 1, 2))


class ReadmeMarkerTests(unittest.TestCase):
    README = 'Intro\n\n%s\nold\n%s\n\nTail\n' % (report.START_MARKER, report.END_MARKER)

    def test_replaces_only_the_marked_text(self):
        updated = report.replace_section(self.README, 'new table')
        self.assertEqual(updated, 'Intro\n\n%s\nnew table\n%s\n\nTail\n' % (report.START_MARKER, report.END_MARKER))

    def test_replacing_twice_is_stable(self):
        once = report.replace_section(self.README, 'new table\n')
        self.assertEqual(report.replace_section(once, 'new table\n'), once)

    def test_missing_markers_are_an_error(self):
        with self.assertRaises(ValueError):
            report.replace_section('no markers', 'x')
        with self.assertRaises(ValueError):
            report.extract_section('%s only start' % report.START_MARKER)

    def test_comparison_ignores_the_updated_line(self):
        first = '| a | b |\n\nUpdated 2026-01-01, commit abc.'
        second = '| a | b |\n\nUpdated 2026-02-02, commit def.'
        self.assertEqual(report.comparable(first), report.comparable(second))
        self.assertNotEqual(report.comparable(first), report.comparable('| a | c |\n\nUpdated 2026-01-01, commit abc.'))

    def test_extract_returns_the_replaced_text(self):
        updated = report.replace_section(self.README, 'new table')
        self.assertEqual(report.extract_section(updated), 'new table')


class CMakeParserTests(unittest.TestCase):
    def test_counts_tests_with_traces(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'tests').mkdir()
            (root / 'artifacts' / 'oracle' / 'a-out').mkdir(parents=True)
            for name in ('one.mem', 'two.mem', 'one.csv'):
                (root / 'artifacts' / 'oracle' / 'a-out' / name).write_text('')
            (root / 'CMakeLists.txt').write_text(
                'set(CHASE_A_TRACE_DIR "${CMAKE_SOURCE_DIR}/artifacts/oracle/a-out" CACHE PATH "x")\n'
                'set(CHASE_B_TRACE_DIR "${CMAKE_SOURCE_DIR}/artifacts/oracle/b-out" CACHE PATH "x")\n'
                'add_test(NAME plain COMMAND tool)\n'
                'add_test(NAME traced COMMAND tool\n  "${CHASE_A_TRACE_DIR}" (nested))\n')
            checks = report.measure_checks(root)
        self.assertEqual((checks['ctest_tests'], checks['ctest_tests_with_traces']), (2, 1))
        self.assertEqual((checks['trace_sets'], checks['recorded_traces']), (2, 2))
        self.assertEqual(checks['missing_trace_directories'], ['artifacts/oracle/b-out'])


class CodeMeasureTests(unittest.TestCase):
    def test_function_entries_from_calls_and_pointers(self):
        base = 0x100000
        words = [0xE1A00000] * 8
        words[0] = 0xEB000002  # bl +8 words ahead: target = base + 8 + 8
        words[1] = 0xFA000000  # blx imm to base + 4 + 8
        code = struct.pack('<8I', *words)
        pointers = struct.pack('<2I', base + 0x18, 0x5000)
        header = bytearray(0x34)
        struct.pack_into('<I', header, 0x1c, 0x34)
        struct.pack_into('<HH', header, 0x2a, 32, 2)
        program = struct.pack('<8I', 1, 0x100, base, base, len(code), len(code), 5, 0x1000)
        program += struct.pack('<8I', 1, 0x120, 0x200000, 0x200000, len(pointers), len(pointers), 4, 0x1000)
        image = bytearray(header) + program
        image += bytes(0x100 - len(image)) + code + pointers
        found_base, size, entries = report.function_entries(bytes(image))
        self.assertEqual((found_base, size), (base, len(code)))
        self.assertEqual(entries, [base, base + 0xC, base + 0x10, base + 0x18])

    def test_cited_addresses_use_six_digit_hex_only(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'docs' / 're').mkdir(parents=True)
            (root / 'src').mkdir()
            (root / 'docs' / 're' / 'a.md').write_text('code 0x1ddd00 and 0x3d4ccccd and 0x1234 and x0x2428c0y')
            (root / 'src' / 'b.cpp').write_text('// 0x23aff8, 0x23AFF8')
            self.assertEqual(report.cited_addresses(root), {0x1ddd00, 0x23aff8})


class FormatTests(unittest.TestCase):
    def test_percent_format(self):
        self.assertEqual(report.format_percent(0), '0.0%')
        self.assertEqual(report.format_percent(0.04), '<0.1%')
        self.assertEqual(report.format_percent(12.345), '12.3%')


if __name__ == '__main__':
    unittest.main()
