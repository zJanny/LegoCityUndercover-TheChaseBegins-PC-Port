import contextlib
import io
import subprocess
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ElementTree
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parents[1] / 'tools'))
import progress_report as report  # noqa: E402
import progress_trend as trend  # noqa: E402
sys.path.insert(0, str(Path(__file__).resolve().parent))
from fixtures import isolate_git_environment  # noqa: E402

isolate_git_environment()


def section(script=(34, 239), objects=(104, 506), open_work=(208, 106), code=(1087, 7575, '14.3'), date='2026-10-14', commit='5cf4da3'):
    return '\n'.join([
        '| Measure | Now | What it means |',
        '|---|---|---|',
        '| Script commands | %d of %d (14.2%%) | Commands. |' % script,
        '| Object types | %d of %d (20.6%%) | Objects. |' % objects,
        '| Game files read | 99.6%% of files, 99.9%% of data | Files. |',
        '| Open work | %d open items, %d stand-ins | Gaps. |' % open_work,
        '| Checked against the original | 120 tests, 200 recorded traces | Tests. |',
        '| Game code documented | %d of about %d (%s%%) | Functions. |' % code,
        '',
        trend.TREND_IMAGE,
        '',
        'Updated %s, commit %s. How the numbers are measured: [docs/progress.md](docs/progress.md).' % (date, commit),
    ])


def readme(progress):
    return 'Intro\n\n%s\n%s\n%s\n\nOutro\n' % (trend.START_MARKER, progress, trend.END_MARKER)


def git(directory, *args):
    return subprocess.run(['git', '-C', str(directory), '-c', 'user.name=Test', '-c', 'user.email=test@example.invalid', *args],
                          capture_output=True, text=True, check=True).stdout


def make_repo(directory, versions):
    git(directory, 'init', '-q')
    for number, text in enumerate(versions):
        (directory / 'README.md').write_text(readme(text), encoding='utf-8', newline='\n')
        git(directory, 'add', 'README.md')
        git(directory, 'commit', '-q', '-m', 'version %d' % number)


class ParseSectionTests(unittest.TestCase):
    def test_reads_every_measure_of_a_table(self):
        point = trend.parse_section(section())
        self.assertEqual(point['date'], '2026-10-14')
        self.assertEqual(point['stamp'], 'Updated 2026-10-14, commit 5cf4da3')
        self.assertEqual((point['script'], point['script_max']), (34, 239))
        self.assertEqual((point['objects'], point['objects_max']), (104, 506))
        self.assertEqual((point['open'], point['stand_ins']), (208, 106))
        self.assertEqual((point['code'], point['code_text']), (14.3, '14.3%'))

    def test_skips_a_version_without_a_row_or_with_unreadable_values(self):
        without_code = section().replace('| Game code documented', '| Code')
        self.assertIsNone(trend.parse_section(without_code))
        unreadable = section().replace('34 of 239 (14.2%)', 'not measured')
        self.assertIsNone(trend.parse_section(unreadable))
        self.assertIsNone(trend.parse_section('| Script commands | 3 |\n'))

    def test_skips_a_version_without_an_updated_line(self):
        self.assertIsNone(trend.parse_section(section().replace('Updated ', 'Changed ')))


class BuildPointsTests(unittest.TestCase):
    def test_one_point_per_stamp_oldest_first_with_current_last(self):
        older = section(script=(10, 239), date='2026-10-13', commit='aaaaaaa')
        newer = section(script=(20, 239), date='2026-10-13', commit='bbbbbbb')
        current = section(script=(30, 239), date='2026-10-14', commit='ccccccc')
        points = trend.build_points([older, newer, older, 'no table here'], current)
        self.assertEqual([p['script'] for p in points], [10, 20, 30])
        self.assertEqual(points[-1]['stamp'], 'Updated 2026-10-14, commit ccccccc')

    def test_duplicate_stamp_counts_once(self):
        first = section(script=(10, 239), commit='aaaaaaa')
        again = section(script=(99, 239), commit='aaaaaaa')
        points = trend.build_points([first, again], again)
        self.assertEqual([p['script'] for p in points], [10])

    def test_runs_of_identical_values_keep_the_oldest_point(self):
        committed = section(script=(10, 239), date='2026-10-14', commit='aaaaaaa')
        same_later = section(script=(10, 239), date='2026-10-14', commit='bbbbbbb')
        changed = section(script=(20, 239), date='2026-10-14', commit='ccccccc')
        points = trend.build_points([committed, same_later], changed)
        self.assertEqual([p['stamp'] for p in points], ['Updated 2026-10-14, commit aaaaaaa', 'Updated 2026-10-14, commit ccccccc'])
        self.assertEqual(trend.build_points([committed], committed), trend.build_points([committed, committed], committed))

    def test_no_points_is_an_error(self):
        with self.assertRaises(ValueError):
            trend.trend_svg([])


class TrendSvgTests(unittest.TestCase):
    def points(self):
        return trend.build_points([section(script=(10, 239), date='2026-10-13', commit='aaaaaaa')], section())

    def test_is_well_formed_svg_with_four_panels_and_no_scripts_or_links(self):
        svg = trend.trend_svg(self.points())
        root = ElementTree.fromstring(svg)
        self.assertEqual(root.tag, '{http://www.w3.org/2000/svg}svg')
        self.assertEqual(len(root.findall('.//{http://www.w3.org/2000/svg}polyline')), 5)
        self.assertNotIn('<script', svg)
        self.assertNotIn('href', svg)
        self.assertNotIn('@import', svg)
        self.assertIn('@media (prefers-color-scheme: dark)', svg)

    def test_shows_latest_values_and_end_labels(self):
        svg = trend.trend_svg(self.points())
        self.assertIn('>34 of 239<', svg)
        self.assertIn('>104 of 506<', svg)
        self.assertIn('>208 / 106<', svg)
        self.assertIn('>14.3%<', svg)
        self.assertIn('>open 208<', svg)
        self.assertIn('>stand-ins 106<', svg)
        self.assertIn('>13 Oct<', svg)
        self.assertIn('>14 Oct<', svg)

    def test_output_is_deterministic(self):
        self.assertEqual(trend.trend_svg(self.points()), trend.trend_svg(self.points()))

    def test_history_is_read_from_the_git_commits_of_readme(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            make_repo(root, [section(script=(10, 239), date='2026-10-13', commit='aaaaaaa'),
                             'no progress table in this version',
                             section(script=(20, 239), date='2026-10-13', commit='bbbbbbb')])
            sections = trend.history_sections(root)
            self.assertEqual(len(sections), 3)
            points = trend.build_points(sections, section(script=(20, 239), date='2026-10-13', commit='ccccccc'))
            self.assertEqual([p['script'] for p in points], [10, 20])


class TrendCheckTests(unittest.TestCase):
    def fake_report(self):
        return {
            'timestamp': '2026-10-14T10:00:00+02:00',
            'git': {'commit': 'abc', 'short': 'abc1234'},
            'scripts': {'all_levels': {'distinct': report.ratio(34, 239), 'distinct_with_stand_in': report.ratio(111, 239),
                                       'calls': report.ratio(65, 100)}},
            'classes': {'all_levels': {'distinct': report.ratio(104, 506), 'instances': report.ratio(52, 100)}},
            'files': {'files': report.ratio(99, 100), 'bytes': report.ratio(99, 100)},
            'features': {'open': 208, 'stand_ins': 106, 'features': 23},
            'checks': {'ctest_tests': 120, 'recorded_traces': 200},
            'code': {'functions': report.ratio(1087, 7575)},
        }

    def test_check_fails_for_a_stale_or_missing_svg_and_passes_after_update(self):
        fake = self.fake_report()
        current = report.render_section(fake)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            make_repo(root, [current])
            (root / 'docs').mkdir()
            readme_path = root / 'README.md'
            readme_path.write_text(readme(current), encoding='utf-8', newline='\n')
            original_collect = report.collect
            report.collect = lambda *args: fake
            arguments = ['--root', str(root), '--check', str(readme_path)]
            try:
                with contextlib.redirect_stdout(io.StringIO()) as output:
                    self.assertEqual(report.main(arguments), 1)
                self.assertIn('docs/progress-trend.svg is stale', output.getvalue())
                (root / 'docs' / 'progress-trend.svg').write_text('<svg/>\n', encoding='utf-8', newline='\n')
                with contextlib.redirect_stdout(io.StringIO()):
                    self.assertEqual(report.main(arguments), 1)
                trend.write_trend(root, current)
                with contextlib.redirect_stdout(io.StringIO()):
                    self.assertEqual(report.main(arguments), 0)
            finally:
                report.collect = original_collect

    def test_rendered_section_embeds_the_trend_under_the_table(self):
        section_text = report.render_section(self.fake_report())
        table_end = section_text.index('| Game code documented')
        self.assertLess(table_end, section_text.index(trend.TREND_IMAGE))
        self.assertLess(section_text.index(trend.TREND_IMAGE), section_text.index('Updated 2026-10-14'))


if __name__ == '__main__':
    unittest.main()
