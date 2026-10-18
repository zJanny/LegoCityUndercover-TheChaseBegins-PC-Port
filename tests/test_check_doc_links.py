import contextlib
import io
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parents[1] / 'tools'))
import check_doc_links as links  # noqa: E402

RE_FILE = """# Topic

## Current state

- Summary.

## 2026-01-02T03:04:05+01:00 — First

Text.

## 2026-01-02T03:09:00+01:00 — Second

```
## 2026-01-02T03:10:00+01:00 — inside a code block
```
"""

FEATURE = """# Feature

See [topic.md §2026-01-02T03:04](../../re/topic.md) and `[x](missing.md)`.
"""


class CheckDocLinksTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        (self.root / 'docs' / 're').mkdir(parents=True)
        (self.root / 'docs' / 'plans' / 'features').mkdir(parents=True)
        (self.root / 'docs' / 're' / 'topic.md').write_text(RE_FILE)
        self.feature = self.root / 'docs' / 'plans' / 'features' / 'f.md'
        self.feature.write_text(FEATURE)

    def tearDown(self):
        self.tmp.cleanup()

    def test_unfixed_tree_fails(self):
        errors = links.process(self.root, False)[0]
        self.assertTrue(any('entry has no anchor ts-20260102-030405' in e for e in errors))
        self.assertTrue(any('must be ../../re/topic.md#ts-20260102-030405' in e for e in errors))
        self.assertFalse(any('missing.md' in e for e in errors))

    def test_fix_is_idempotent_and_passes(self):
        errors, added, rewritten, _ = links.process(self.root, True)
        self.assertEqual((added, rewritten), (2, 1))
        self.assertEqual(links.process(self.root, False)[0], [])
        snapshot = {p: p.read_text() for p in self.root.rglob('*.md')}
        self.assertEqual(links.process(self.root, True)[1:3], (0, 0))
        self.assertEqual(snapshot, {p: p.read_text() for p in self.root.rglob('*.md')})
        self.assertIn('(../../re/topic.md#ts-20260102-030405)', self.feature.read_text())
        self.assertNotIn('ts-20260102-031000', (self.root / 'docs' / 're' / 'topic.md').read_text())

    def test_anchor_directly_above_heading_is_not_duplicated(self):
        topic = self.root / 'docs' / 're' / 'topic.md'
        topic.write_text(RE_FILE.replace('## 2026-01-02T03:04:05', '<a id="ts-20260102-030405"></a>\n## 2026-01-02T03:04:05'))
        links.process(self.root, True)
        text = topic.read_text()
        self.assertEqual(text.count('<a id="ts-20260102-030405"></a>'), 1)
        self.assertIn('<a id="ts-20260102-030405"></a>\n\n## 2026-01-02T03:04:05', text)
        self.assertEqual(links.process(self.root, False)[0], [])

    def test_duplicate_anchor_fails_and_fix_removes_it(self):
        topic = self.root / 'docs' / 're' / 'topic.md'
        links.process(self.root, True)
        anchor = '<a id="ts-20260102-030405"></a>'
        topic.write_text(topic.read_text().replace(anchor, f'{anchor}\n{anchor}'))
        errors = links.process(self.root, False)[0]
        self.assertTrue(any('duplicate anchor ts-20260102-030405' in e for e in errors))
        links.process(self.root, True)
        self.assertEqual(topic.read_text().count(anchor), 1)
        self.assertEqual(links.process(self.root, False)[0], [])

    def test_missing_file_anchor_and_stamp(self):
        links.process(self.root, True)
        self.feature.write_text(
            '[a](nope.md) [b](../../re/topic.md#nothing) [c](#top) [d](../../re/topic.md "t")\n'
            '[e topic.md §2030-01-01T00:00](../../re/topic.md)\n# Top\n'
        )
        errors = links.process(self.root, False)[0]
        self.assertEqual(len(errors), 3)
        self.assertTrue(any('missing file nope.md' in e for e in errors))
        self.assertTrue(any('missing anchor #nothing' in e for e in errors))
        self.assertTrue(any('no entry §2030-01-01T00:00' in e for e in errors))

    def test_same_file_links_are_plain_fragments(self):
        links.process(self.root, True)
        topic = self.root / 'docs' / 're' / 'topic.md'
        topic.write_text(topic.read_text() + '\n[topic.md §2026-01-02T03:04](topic.md#ts-20260102-030405) [x](topic.md#ts-20260102-030900)\n')
        errors = links.process(self.root, False)[0]
        self.assertTrue(any('link topic.md#ts-20260102-030405 must be #ts-20260102-030405' in e for e in errors))
        self.assertTrue(any('same-file link topic.md#ts-20260102-030900 must be #ts-20260102-030900' in e for e in errors))
        links.process(self.root, True)
        self.assertEqual(links.process(self.root, False)[0], [])
        self.assertIn('(#ts-20260102-030405) [x](#ts-20260102-030900)', topic.read_text())

    def test_heading_anchor_and_external_links(self):
        links.process(self.root, True)
        self.feature.write_text('# My Title: (x)\n[a](#my-title-x) [b](https://example.org/x.md#y)\n')
        self.assertEqual(links.process(self.root, False)[0], [])


class RuleChecksTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.re_dir = self.root / 'docs' / 're'
        self.features = self.root / 'docs' / 'plans' / 'features'
        self.re_dir.mkdir(parents=True)
        self.features.mkdir(parents=True)

    def tearDown(self):
        self.tmp.cleanup()

    def errors(self, rule):
        return [e for e in links.process(self.root, False)[0] if rule in e]

    def test_re_file_size(self):
        (self.re_dir / 'big.md').write_text('x' * 40960)
        (self.re_dir / 'huge.md').write_text('x' * 40961)
        found = self.errors('re-file-size')
        self.assertEqual(len(found), 1)
        self.assertTrue(found[0].startswith('docs/re/huge.md:1: re-file-size'))

    def test_feature_byte_limit(self):
        (self.features / 'ok.md').write_text('x' * 16384)
        (self.features / 'big.md').write_text('x' * 16385)
        (self.features / 'big.closed.md').write_text('x' * 20000)
        self.assertEqual(len(self.errors('feature-file-size')), 1)
        self.assertTrue(self.errors('feature-file-size')[0].startswith('docs/plans/features/big.md:1: feature-file-size: 16385 bytes, limit 16384'))

    def test_index_has_no_line_limit(self):
        (self.features / 'README.md').write_text('line\n' * 200)
        self.assertEqual(self.errors('index-'), [])

    def test_index_row_rules(self):
        (self.features / 'f.md').write_text('# F\n')
        index = self.features / 'README.md'
        index.write_text('# Index\n\n## Area\n| Feature | Status |\n|---|---|\n| [f](f.md) | ' + 'x' * 300 + ' |\n| [f](f.md) | again |\n')
        found = self.errors('index-')
        self.assertEqual(sorted(e.split(': ')[1] for e in found), ['index-duplicate-row', 'index-row-length', 'index-table-spacing'])
        index.write_text('# Index\n\n## Area\n\n| Feature | Status |\n|---|---|\n| [f](f.md) | ' + 'x' * 250 + ' |\n')
        self.assertEqual(self.errors('index-'), [])

    def test_next_counts_closed_split_files_and_skips_reservations(self):
        (self.features / 'f.md').write_text('- [ ] F-3 a\n- [ ] F-S2 stand-in\n')
        (self.features / 'f.closed.md').write_text('- [x] F-7 done\n')
        (self.features / 'f-sub.md').write_text('- [ ] F-5 b\n')
        self.assertEqual(links.next_ids(self.root, 'F', 2), ['F-8', 'F-9'])
        self.assertEqual(links.next_ids(self.root, 'F-S', 1), ['F-S3'])
        links.reserve_ids(self.root, 'F', 3, 'agent a')
        self.assertEqual(links.next_ids(self.root, 'F', 2), ['F-11', 'F-12'])
        rows = links.reservation_rows(self.root)
        self.assertEqual(rows[0][:4], ('F', 8, 10, 'agent a'))
        self.assertEqual(links.release_ids(self.root, 'agent a'), 1)
        self.assertEqual(links.next_ids(self.root, 'F', 1), ['F-8'])
        self.assertEqual(self.errors('index-'), [])

    def test_next_rejects_bad_input(self):
        for argv in (['--next', 'lower'], ['--next', 'F', '0'], ['--next', 'F', 'x'], ['--reserve', 'F', '0', 'who']):
            with self.assertRaises(SystemExit), contextlib.redirect_stderr(io.StringIO()) as err:
                links.main(['--root', str(self.root)] + argv)
            self.assertIn(argv[0], err.getvalue())

    def test_unknown_prefix_and_unknown_holder_fail(self):
        (self.features / 'f.md').write_text('- [ ] F-1 a\n')
        for argv in (['--next', 'NOPE'], ['--reserve', 'NOPE', '2', 'who'], ['--release', 'ghost']):
            with self.assertRaises(SystemExit), contextlib.redirect_stderr(io.StringIO()) as err:
                links.main(['--root', str(self.root)] + argv)
            self.assertIn(argv[0], err.getvalue())
        with contextlib.redirect_stdout(io.StringIO()) as out:
            self.assertEqual(links.main(['--root', str(self.root), '--next', 'NEW', '--new-prefix']), 0)
        self.assertEqual(out.getvalue().strip(), 'NEW-1')
        with contextlib.redirect_stdout(io.StringIO()) as out:
            links.main(['--root', str(self.root), '--next', 'F-S'])
        self.assertEqual(out.getvalue().strip(), 'F-S1')

    def test_future_timestamp_in_a_findings_body_is_an_error(self):
        (self.re_dir / 'a.md').write_text('## Current state\n\n- Seen at 2099-01-01T01:01:01+01:00.\n')
        self.assertEqual(len(self.errors('future-timestamp')), 1)

    def test_added_line_starting_with_plus_plus_is_content(self):
        import subprocess
        subprocess.run(['git', 'init', '-q', str(self.root)], check=True)
        self.features.joinpath('f.md').write_text('# F\n')
        subprocess.run(['git', '-C', str(self.root), 'add', '.'], check=True)
        subprocess.run(['git', '-C', str(self.root), '-c', 'user.name=t', '-c', 'user.email=t@t', 'commit', '-qm', 'x'], check=True)
        self.features.joinpath('f.md').write_text('# F\n++ b/x at 2026-01-01T01:00:00+01:00\n')
        added = links.added_lines(self.root)
        self.assertEqual([text for _, text in added['docs/plans/features/f.md']], ['++ b/x at 2026-01-01T01:00:00+01:00'])

    def test_future_timestamp_is_an_error(self):
        (self.re_dir / 'a.md').write_text('<a id="ts-20260101-010101"></a>\n\n## 2026-01-01T01:01:01+01:00 — Old\n')
        (self.re_dir / 'b.md').write_text('<a id="ts-20990101-010101"></a>\n\n## 2099-01-01T01:01:01+01:00 — Future\n')
        found = self.errors('future-timestamp')
        self.assertEqual(len(found), 1)
        self.assertTrue(found[0].startswith('docs/re/b.md:3:'))

    def test_round_seconds_warning_only_for_new_lines(self):
        import subprocess
        subprocess.run(['git', 'init', '-q', str(self.root)], check=True)
        self.re_dir.joinpath('a.md').write_text('## Current state\n\n- S.\n\n<a id="ts-20260101-010000"></a>\n\n## 2026-01-01T01:00:00+01:00 — Old\n')
        subprocess.run(['git', '-C', str(self.root), 'add', '.'], check=True)
        subprocess.run(['git', '-C', str(self.root), '-c', 'user.name=t', '-c', 'user.email=t@t', 'commit', '-qm', 'x'], check=True)
        self.assertEqual(links.timestamp_warnings(self.root), [])
        self.re_dir.joinpath('a.md').write_text(self.re_dir.joinpath('a.md').read_text() + '\n<a id="ts-20260102-020000"></a>\n\n## 2026-01-02T02:00:00+01:00 — New\n')
        warnings = [w for w in links.timestamp_warnings(self.root) if 'round-seconds-timestamp' in w]
        self.assertEqual(len(warnings), 1)
        self.assertEqual(links.process(self.root, False)[0], [])

    def test_item_field_order(self):
        (self.features / 'f.md').write_text(
            '- [ ] F-1 — gap. needs: F-2. blocks: F-3. evidence: [x](x.md)\n'
            '- [ ] F-2 — gap. blocks: F-3. needs: F-1.\n'
            '- F-S1 — stand-in. evidence: [x](x.md) needs: F-1\n'
            '- F-S2 stand-in without a dash. blocks: F-1. needs: F-2.\n'
        )
        (self.features / 'f.closed.md').write_text('- [x] F-3 — done. evidence: e. blocks: F-1\n')
        found = self.errors('item-field-order')
        self.assertEqual([e.split(': ')[0] for e in found], ['docs/plans/features/f.md:2', 'docs/plans/features/f.md:3', 'docs/plans/features/f.md:4', 'docs/plans/features/f.closed.md:1'])

    def test_item_field_order_with_uses(self):
        (self.features / 'f.md').write_text(
            '- [ ] F-1 — gap. needs: F-2. blocks: F-3. uses: EnableObject, props/prop_platform. evidence: [x](x.md)\n'
            '- [ ] F-2 — gap. uses: EnableObject. blocks: F-3.\n'
            '- [ ] F-3 — gap. evidence: [x](x.md) uses: EnableObject\n'
            '- F-S1 stand-in. uses: A.\n'
        )
        found = self.errors('item-field-order')
        self.assertEqual([e.split(': ')[0] for e in found], ['docs/plans/features/f.md:2', 'docs/plans/features/f.md:3'])

    def test_superseded_entry_moves_to_history_and_links_follow(self):
        (self.re_dir / 'a.md').write_text(
            '# A\n\n## Current state\n\nSummary.\n\n'
            '<a id="ts-20260101-010101"></a>\n\n## 2026-01-01T01:01:01+01:00 — Old\n\n'
            'Superseded by [a.md §2026-01-02T02:02:02](#ts-20260102-020202): wrong.\n\nSee [a.md §2026-01-02T02:02:02](#ts-20260102-020202).\n\n'
            '<a id="ts-20260102-020202"></a>\n\n## 2026-01-02T02:02:02+01:00 — New\n\nBody, links [old](#ts-20260101-010101).\n'
        )
        (self.features / 'f.md').write_text('See [a.md §2026-01-01T01:01:01](../../re/a.md#ts-20260101-010101).\n')
        self.assertEqual(links.process(self.root, False)[0], [])
        messages = []
        errors = links.process(self.root, True, messages)[0]
        self.assertEqual(errors, [])
        self.assertEqual(messages, ['1 superseded entries moved'])
        topic = (self.re_dir / 'a.md').read_text()
        history = (self.re_dir / 'a.history.md').read_text()
        self.assertNotIn('Old', topic)
        self.assertIn('## Current state', topic)
        self.assertIn('[a.md](a.md)', history)
        self.assertIn('<a id="ts-20260101-010101"></a>\n\n## 2026-01-01T01:01:01+01:00 — Old', history)
        self.assertIn('(a.md#ts-20260102-020202)', history)
        self.assertIn('(a.history.md#ts-20260101-010101)', topic)
        self.assertIn('(../../re/a.history.md#ts-20260101-010101)', (self.features / 'f.md').read_text())
        self.assertEqual(links.process(self.root, False)[0], [])
        self.assertEqual(links.process(self.root, True)[1:3], (0, 0))

    def test_current_state_is_required(self):
        (self.re_dir / 'a.md').write_text('<a id="ts-20260101-010101"></a>\n\n## 2026-01-01T01:01:01+01:00 — E\n')
        self.assertEqual(len(self.errors('current-state-missing')), 1)

    def test_split_leftover_warns_only_for_changed_files(self):
        import subprocess
        subprocess.run(['git', 'init', '-q', str(self.root)], check=True)
        body = '## Current state\n\n- Fact [b-x.md §2026-01-01T01:01](b-x.md#ts-20260101-010101)\n\n<a id="ts-20260101-010102"></a>\n\n## 2026-01-01T01:01:02+01:00 — E\n'
        (self.re_dir / 'b.md').write_text(body)
        (self.re_dir / 'b-x.md').write_text('## Current state\n\n- S.\n\n<a id="ts-20260101-010101"></a>\n\n## 2026-01-01T01:01:01+01:00 — Moved\n')
        subprocess.run(['git', '-C', str(self.root), 'add', '.'], check=True)
        subprocess.run(['git', '-C', str(self.root), '-c', 'user.name=t', '-c', 'user.email=t@t', 'commit', '-qm', 'x'], check=True)
        (self.re_dir / 'b.md').write_text(body + '\nSee also [b-x.md](b-x.md)\n')
        self.assertEqual(links.split_leftover_warnings(self.root), [])
        (self.re_dir / 'b.md').write_text(body + '\nSubtopics: [b-x.md](b-x.md)\n')
        warnings = links.split_leftover_warnings(self.root)
        self.assertEqual(len(warnings), 1)
        self.assertIn('current-state-split', warnings[0])

    def test_current_state_must_precede_entries(self):
        (self.re_dir / 'a.md').write_text('<a id="ts-20260101-010101"></a>\n\n## 2026-01-01T01:01:01+01:00 — E\n\n## Current state\n')
        self.assertEqual(len(self.errors('current-state-position')), 1)

    def test_long_item_lines_are_not_limited(self):
        (self.features / 'f.md').write_text('- [ ] FOO-1 ' + 'y' * 900 + '\n')
        self.assertEqual(links.process(self.root, False)[0], [])

    def write_log(self, count):
        plans = self.root / 'docs' / 'plans'
        lines = [f'- 2026-01-{n:02d}T00:00:00+01:00 — line {n}' for n in range(1, count + 1)]
        (plans / 'x.progress.md').write_text('# P\n\n## Status\n\n- not a log line\n\n## Session Log\n\nIntro.\n\n' + '\n'.join(lines) + '\n')
        return lines

    def test_session_log_length_fails_and_fix_rotates(self):
        lines = self.write_log(23)
        found = self.errors('session-log-length')
        self.assertEqual(len(found), 1)
        self.assertIn('23 entry lines, limit 20', found[0])
        messages = []
        links.process(self.root, True, messages)
        self.assertEqual(messages, ['3 log lines archived'])
        plans = self.root / 'docs' / 'plans'
        log = (plans / 'x.progress.md').read_text()
        self.assertIn('- not a log line', log)
        self.assertEqual([l for l in log.split('\n') if l.startswith('- 2026')], lines[3:])
        archive = (plans / 'x.progress-archive.md').read_text()
        self.assertEqual([l for l in archive.split('\n') if l.startswith('- 2026')], lines[:3])
        self.assertEqual(self.errors('session-log-length'), [])

    def test_session_log_rotation_appends_and_is_idempotent(self):
        lines = self.write_log(20)
        plans = self.root / 'docs' / 'plans'
        links.process(self.root, True)
        self.assertFalse((plans / 'x.progress-archive.md').exists())
        extra = ['- 2026-02-01T00:00:00+01:00 — new 1', '- 2026-02-02T00:00:00+01:00 — new 2']
        log = plans / 'x.progress.md'
        log.write_text(log.read_text() + '\n'.join(extra) + '\n')
        links.process(self.root, True)
        snapshot = {p: p.read_text() for p in plans.glob('*.md')}
        links.process(self.root, True)
        self.assertEqual(snapshot, {p: p.read_text() for p in plans.glob('*.md')})
        archive = [l for l in (plans / 'x.progress-archive.md').read_text().split('\n') if l.startswith('- 2026')]
        self.assertEqual(archive, lines[:2])
        log.write_text(log.read_text() + '- 2026-02-03T00:00:00+01:00 — new 3\n')
        links.process(self.root, True)
        archive = [l for l in (plans / 'x.progress-archive.md').read_text().split('\n') if l.startswith('- 2026')]
        self.assertEqual(archive, lines[:3])
        kept = [l for l in log.read_text().split('\n') if l.startswith('- 2026')]
        self.assertEqual(kept, lines[3:] + extra + ['- 2026-02-03T00:00:00+01:00 — new 3'])

    def test_session_log_rotation_after_merge_keeps_one_copy(self):
        lines = self.write_log(23)
        plans = self.root / 'docs' / 'plans'
        archive = plans / 'x.progress-archive.md'
        archive.write_text('# Session log archive\n\n' + '\n'.join([lines[0], lines[1], lines[0]]) + '\n')
        messages = []
        links.process(self.root, True, messages)
        self.assertEqual(messages, ['1 duplicate archive log lines removed', '1 log lines archived'])
        entries = [l for l in archive.read_text().split('\n') if l.startswith('- 2026')]
        self.assertEqual(entries, lines[:3])
        kept = [l for l in (plans / 'x.progress.md').read_text().split('\n') if l.startswith('- 2026')]
        self.assertEqual(kept, lines[3:])

    def test_fix_moves_closed_items_in_order(self):
        feature = self.features / 'f.md'
        feature.write_text('# F\n\n- [ ] F-1 open\n- [x] F-2 done\n- F-S1 stand-in\n- [x] F-3 also done\nProse.\n')
        messages = []
        links.process(self.root, True, messages)
        self.assertEqual(messages, ['2 closed items moved'])
        self.assertEqual(feature.read_text(), '# F\n\n- [ ] F-1 open\n- F-S1 stand-in\nProse.\n')
        closed = (self.features / 'f.closed.md').read_text()
        self.assertTrue(closed.startswith('# Closed items of f\n'))
        self.assertTrue(closed.endswith('- [x] F-2 done\n- [x] F-3 also done\n'))
        feature.write_text(feature.read_text() + '- [x] F-4 later\n')
        links.process(self.root, True)
        self.assertTrue((self.features / 'f.closed.md').read_text().endswith('- [x] F-3 also done\n- [x] F-4 later\n'))
        snapshot = {p: p.read_text() for p in self.features.glob('*.md')}
        links.process(self.root, True)
        self.assertEqual(snapshot, {p: p.read_text() for p in self.features.glob('*.md')})
        self.assertEqual(links.process(self.root, False)[0], [])

    def test_duplicate_ids_across_feature_and_closed_file(self):
        (self.features / 'f.md').write_text('- [ ] F-1 open\n- F-S1 guess\n')
        (self.features / 'f.closed.md').write_text('- [x] F-1 done\n- F-S1 again\n- [x] F-2 fine\n')
        found = self.errors('duplicate-id')
        self.assertEqual(len(found), 2)
        self.assertTrue(any('f.closed.md:1: duplicate-id: F-1 is already defined at docs/plans/features/f.md:1' in e for e in found))
        self.assertTrue(any('f.closed.md:2: duplicate-id: F-S1' in e for e in found))

    def test_duplicate_ids_inside_one_file_and_split_siblings(self):
        (self.features / 'f.md').write_text('- [ ] F-1 a\n- [ ] F-1 b\n')
        (self.features / 'f-sub.md').write_text('- [ ] F-1 c\n')
        (self.features / 'g.md').write_text('- [ ] F-1 other feature, allowed\n')
        self.assertEqual(len(self.errors('duplicate-id')), 2)

    def test_every_feature_file_has_an_index_row(self):
        (self.features / 'f.md').write_text('# F\n')
        (self.features / 'f-sub.md').write_text('# F sub\n')
        (self.features / 'f.closed.md').write_text('- [x] F-1 done\n')
        index = self.features / 'README.md'
        index.write_text('# Index\n\nSee [f](f.md) in prose.\n\n| Feature | Status |\n|---|---|\n| [f](f.md) | IN_PROGRESS |\n')
        found = self.errors('index-')
        self.assertEqual(found, ["docs/plans/features/f-sub.md:1: index-missing-row: f-sub.md has no row in docs/plans/features/README.md; add one"])
        index.write_text(index.read_text() + '| [f-sub](f-sub.md) | IN_PROGRESS |\n| [f closed](f.closed.md) | DONE |\n')
        self.assertEqual(self.errors('index-'), ["docs/plans/features/README.md:9: index-closed-file: f.closed.md is a closed-items file and has no index row"])
        index.write_text(index.read_text().replace('| [f closed](f.closed.md) | DONE |\n', ''))
        self.assertEqual(self.errors('index-'), [])

    def test_claude_md_size(self):
        (self.root / 'CLAUDE.md').write_text('x' * 20480)
        self.assertEqual(self.errors('claude-md-size'), [])
        (self.root / 'CLAUDE.md').write_text('x' * 20481)
        self.assertEqual(len(self.errors('claude-md-size')), 1)

    def write_pair(self, corrected_by_line, corrects_text='Corrects a.md §2026-01-01T01:00'):
        (self.re_dir / 'a.md').write_text(
            '# A\n\n<a id="ts-20260101-010000"></a>\n\n## 2026-01-01T01:00:00+01:00 — Old\n\n'
            + corrected_by_line + '\nBody.\n'
        )
        (self.re_dir / 'b.md').write_text(
            '# B\n\n<a id="ts-20260102-020000"></a>\n\n## 2026-01-02T02:00:00+01:00 — New\n\n'
            f'[{corrects_text}](a.md#ts-20260101-010000)\n'
        )

    def test_correction_pairing_complete(self):
        self.write_pair('Corrected by [b.md §2026-01-02T02:00](b.md#ts-20260102-020000): value changed')
        self.assertEqual(self.errors('correction-pairing'), [])

    def test_corrects_without_corrected_by(self):
        self.write_pair('Nothing here.')
        found = self.errors('correction-pairing')
        self.assertEqual(len(found), 1)
        self.assertTrue(found[0].startswith('docs/re/b.md:7:'))

    def test_corrected_by_without_corrects(self):
        self.write_pair(
            'Corrected by [b.md §2026-01-02T02:00](b.md#ts-20260102-020000): value changed',
            corrects_text='See a.md §2026-01-01T01:00',
        )
        found = self.errors('correction-pairing')
        self.assertTrue(any(e.startswith('docs/re/a.md:7:') and 'no matching' in e for e in found))
        self.assertEqual(len(found), 1)

    def test_corrected_by_must_stand_under_heading(self):
        (self.re_dir / 'a.md').write_text(
            '<a id="ts-20260101-010000"></a>\n\n## 2026-01-01T01:00:00+01:00 — Old\n\nBody.\n\n'
            'Corrected by [b.md §2026-01-02T02:00](b.md#ts-20260102-020000): late\n'
        )
        (self.re_dir / 'b.md').write_text(
            '<a id="ts-20260102-020000"></a>\n\n## 2026-01-02T02:00:00+01:00 — New\n\n'
            '[Corrects a.md §2026-01-01T01:00](a.md#ts-20260101-010000)\n'
        )
        found = self.errors('correction-pairing')
        self.assertEqual(len(found), 1)
        self.assertIn('directly under the heading', found[0])


if __name__ == '__main__':
    unittest.main()
