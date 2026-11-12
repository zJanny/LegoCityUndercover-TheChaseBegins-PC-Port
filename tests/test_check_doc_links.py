import contextlib
import io
import subprocess
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

    def test_design_files_keep_anchors_and_size_without_current_state(self):
        design = self.root / 'docs' / 'design'
        design.mkdir()
        (design / 'd.md').write_text('# D\n\n## 2026-01-01T01:01:01+01:00 — Decision\n\nBody.\n')
        errors = links.process(self.root, False)[0]
        self.assertTrue(any('docs/design/d.md' in e and 'entry has no anchor ts-20260101-010101' in e for e in errors))
        self.assertEqual(self.errors('current-state-missing'), [])
        self.assertEqual(links.process(self.root, True)[0], [])
        self.assertIn('<a id="ts-20260101-010101"></a>\n\n## 2026-01-01T01:01:01+01:00 — Decision', (design / 'd.md').read_text())
        (self.features / 'f.md').write_text('See [d.md §2026-01-01T01:01](../../design/d.md).\n')
        self.assertEqual(links.process(self.root, True)[0], [])
        self.assertIn('(../../design/d.md#ts-20260101-010101)', (self.features / 'f.md').read_text())
        (design / 'big.md').write_text('x' * (links.MAX_RE_BYTES + 1))
        self.assertEqual(len(self.errors('re-file-size')), 1)

    def test_design_files_are_edited_in_place(self):
        design = self.root / 'docs' / 'design'
        design.mkdir()
        (design / 'd.md').write_text(
            '# D\n\n## Current state\n\n- New.\n\n'
            '<a id="ts-20260101-010101"></a>\n\n## 2026-01-01T01:01:01+01:00 — Old\n'
            'Superseded by [d.md §2026-01-02T02:02:02](#ts-20260102-020202): wrong.\n'
            'Corrected by [d.md §2026-01-02T02:02:02](#ts-20260102-020202): changed.\n\n'
            '<a id="ts-20260102-020202"></a>\n\n## 2026-01-02T02:02:02+01:00 — New\n\n'
            '[Corrects §2026-01-01T01:01:01](#ts-20260101-010101).\n'
        )
        (design / 'd.history.md').write_text('# History of d\n')
        (self.re_dir / 'a.md').write_text('# A\n\n## Current state\n\n- S.\n\n[Corrects d.md §2026-01-02T02:02:02](../design/d.md#ts-20260102-020202)\n')
        found = self.errors('design-in-place')
        self.assertEqual([e.split(': ')[0] for e in found],
                         ['docs/design/d.md:10', 'docs/design/d.md:11', 'docs/design/d.md:17', 'docs/re/a.md:7'])
        self.assertEqual([e.split(': ')[0] for e in self.errors('design-current-state')], ['docs/design/d.md:3'])
        self.assertEqual([e.split(': ')[0] for e in self.errors('design-history')], ['docs/design/d.history.md:1'])
        self.assertEqual(self.errors('correction-pairing'), [])
        messages = []
        links.process(self.root, True, messages)
        self.assertEqual(messages, [])
        self.assertIn('— Old', (design / 'd.md').read_text())
        self.assertEqual((design / 'd.history.md').read_text(), '# History of d\n')

    def test_design_entries_do_not_need_a_current_state_update(self):
        import subprocess
        design = self.root / 'docs' / 'design'
        design.mkdir()
        subprocess.run(['git', 'init', '-q', str(self.root)], check=True)
        (design / 'd.md').write_text('# D\n')
        old = '# A\n\n## Current state\n\n- S.\n\n<a id="ts-20260101-010101"></a>\n\n## 2026-01-01T01:01:01+01:00 — Old\n'
        (self.re_dir / 'a.md').write_text(old)
        subprocess.run(['git', '-C', str(self.root), 'add', '.'], check=True)
        subprocess.run(['git', '-C', str(self.root), '-c', 'user.name=t', '-c', 'user.email=t@t', 'commit', '-qm', 'x'], check=True)
        entry = '\n<a id="ts-20260102-020201"></a>\n\n## 2026-01-02T02:02:01+01:00 — New\n'
        (design / 'd.md').write_text('# D\n' + entry)
        (self.re_dir / 'a.md').write_text(old + entry)
        stale = [w for w in links.timestamp_warnings(self.root) if 'current-state-stale' in w]
        self.assertEqual([w.split(':')[0] for w in stale], ['docs/re/a.md'])

    def test_dependency_pair_fails_and_fix_adds_the_other_side(self):
        (self.features / 'f.md').write_text(
            '- [ ] F-1 — gap. blocks: F-2, F-S1. evidence: [x](x.md).\n'
            '- [ ] F-2 — gap. evidence: [x](x.md).\n'
            '- F-S1 — stand-in. needs: F-3. uses: A.\n'
            '- [ ] F-3 — gap.\n'
            '- [ ] F-4 — gap. needs: F-9.\n'
        )
        (self.features / 'f.closed.md').write_text('- [x] F-9 — done.\n')
        found = self.errors('dependency-pair')
        self.assertEqual(sorted(e.split(': ')[0] for e in found), ['docs/plans/features/f.md:2', 'docs/plans/features/f.md:3', 'docs/plans/features/f.md:4'])
        messages = []
        links.process(self.root, True, messages)
        self.assertIn('3 dependency counterparts added', messages)
        self.assertEqual((self.features / 'f.md').read_text().split('\n')[:4], [
            '- [ ] F-1 — gap. blocks: F-2, F-S1. evidence: [x](x.md).',
            '- [ ] F-2 — gap. needs: F-1. evidence: [x](x.md).',
            '- F-S1 — stand-in. needs: F-3, F-1. uses: A.',
            '- [ ] F-3 — gap. blocks: F-S1.',
        ])
        self.assertEqual(self.errors('dependency-'), [])
        self.assertEqual(self.errors('item-field-order'), [])

    def test_dependency_conflict_is_not_fixed(self):
        (self.features / 'f.md').write_text('- [ ] F-1 — gap. blocks: F-2.\n- [ ] F-2 — gap. blocks: F-1.\n')
        self.assertEqual(len(self.errors('dependency-conflict')), 2)
        links.process(self.root, True)
        self.assertNotIn('needs', (self.features / 'f.md').read_text())

    def commit_all(self):
        import subprocess
        if not (self.root / '.git').exists():
            subprocess.run(['git', 'init', '-q', str(self.root)], check=True)
        subprocess.run(['git', '-C', str(self.root), 'add', '.'], check=True)
        subprocess.run(['git', '-C', str(self.root), '-c', 'user.name=t', '-c', 'user.email=t@t', 'commit', '-qm', 'x'], check=True)

    def test_new_address_in_an_item_fails_old_ones_stay(self):
        feature = self.features / 'f.md'
        feature.write_text('- [ ] F-1 — gap at 0x1234ab. evidence: [x](x.md).\n- [ ] F-2 — gap.\n')
        self.commit_all()
        self.assertEqual(self.errors('item-address'), [])
        feature.write_text('- [ ] F-1 — gap at 0x1234ab, changed. evidence: [x](x.md).\n'
                           '- [ ] F-2 — gap in 0x2000 and `0x30ab`. evidence: [0x4000](x.md).\n'
                           '- [x] F-3 — closed at 0x5000.\n')
        found = self.errors('item-address')
        self.assertEqual(len(found), 1)
        self.assertTrue(found[0].startswith('docs/plans/features/f.md:2: item-address: F-2 names 0x2000, 0x30ab;'))

    def test_new_log_line_with_ids_or_addresses_fails(self):
        (self.features / 'f.md').write_text('- [ ] F-1 — gap.\n')
        lines = self.write_log(2)
        self.commit_all()
        log = self.root / 'docs' / 'plans' / 'x.progress.md'
        log.write_text(log.read_text().replace('line 1', 'line 1 closed F-1') + '- 2026-02-01T00:00:00+01:00 — Agent 20261025-garage-A: read 0x1f00 for F-S2; see [f.md](features/f.md)\n')
        found = self.errors('log-line-detail')
        self.assertEqual(len(found), 2)
        self.assertIn('names F-1', found[0])
        self.assertIn('names 0x1f00, F-S2;', found[1])
        self.assertTrue(lines)

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

    def write_priorities(self, line_number, text='F-3'):
        target = f'features/f.md?display=source#L{line_number}'
        (self.root / 'docs' / 'plans' / 'priorities.md').write_text(f'# P\n\n[{text}]({target})\n')

    def test_item_source_line_links_are_checked(self):
        (self.features / 'f.md').write_text('# F\n\n- [ ] F-1 open\n- F-S1 stand-in\n- [ ] F-3 third\n')
        self.write_priorities(5)
        self.assertEqual(links.process(self.root, False)[0], [])
        self.write_priorities(3)
        found = links.process(self.root, False)[0]
        self.assertEqual(found, ['docs/plans/priorities.md:3: link features/f.md?display=source#L3 must be features/f.md?display=source#L5'])
        links.process(self.root, True)
        self.assertEqual(links.process(self.root, False)[0], [])
        self.write_priorities(5, 'F-9')
        self.assertEqual(links.process(self.root, False)[0], ['docs/plans/priorities.md:3: no item F-9 in features/f.md'])

    def test_stale_priorities_fail_on_main_and_are_left_alone_on_a_branch(self):
        (self.features / 'f.md').write_text('# F\n\n- [ ] F-1 open\n- F-S1 stand-in\n- [ ] F-3 third\n')
        self.write_priorities(3)
        priorities = self.root / 'docs' / 'plans' / 'priorities.md'
        stale = priorities.read_text()
        subprocess.run(['git', 'init', '-q', '-b', 'main', str(self.root)], check=True)
        self.assertEqual(len(links.process(self.root, False)[0]), 1)
        subprocess.run(['git', '-C', str(self.root), 'checkout', '-q', '-b', 'agent-branch'], check=True)
        self.assertEqual(links.process(self.root, False)[0], [])
        links.process(self.root, True)
        self.assertEqual(priorities.read_text(), stale)
        subprocess.run(['git', '-C', str(self.root), 'symbolic-ref', 'HEAD', 'refs/heads/main'], check=True)
        links.process(self.root, True)
        self.assertIn('#L5', priorities.read_text())

    def test_item_source_line_link_follows_a_move_and_fails_for_a_closed_item(self):
        feature = self.features / 'f.md'
        feature.write_text('# F\n\n- [x] F-2 done\n- [ ] F-3 third\n')
        self.write_priorities(4)
        links.process(self.root, True)
        self.assertEqual(feature.read_text(), '# F\n\n- [ ] F-3 third\n')
        self.assertEqual(links.process(self.root, False)[0], [])
        self.assertIn('?display=source#L3', (self.root / 'docs' / 'plans' / 'priorities.md').read_text())
        self.write_priorities(3, 'F-2')
        self.assertEqual(links.process(self.root, False)[0], ['docs/plans/priorities.md:3: no item F-2 in features/f.md'])

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

    def test_duplicate_id_says_why_and_how_to_fix_it(self):
        (self.features / 'f.md').write_text('- [ ] F-1 open\n- [ ] F-1 from another branch\n- F-S2 guess\n- F-S2 again\n')
        found = self.errors('duplicate-id')
        fix = ('another pull request merged first and took this ID; rebase on main, run '
               '`python3 tools/check_doc_links.py --next PREFIX` again and rename your item')
        self.assertEqual(len(found), 2)
        self.assertTrue(all(fix in e for e in found))
        self.assertTrue(found[0].endswith('(PREFIX here: F)'), found[0])
        self.assertTrue(found[1].endswith('(PREFIX here: F-S)'), found[1])

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

    def make_repository(self):
        (self.root / 'CMakeLists.txt').write_text('project(x)\n')
        (self.root / 'src').mkdir()
        (self.root / 'AGENTS.md').write_text('# Agents\n\nRules: [CLAUDE.md](CLAUDE.md)\n')
        (self.root / 'CLAUDE.md').write_text('# Rules\n')
        (self.root / 'README.md').write_text('This project is not affiliated with anyone.\n')
        (self.root / 'CONTRIBUTING.md').write_text('We are Not Affiliated with Nintendo.\n')
        (self.root / 'LICENSE').write_text('\n                    GNU GENERAL PUBLIC LICENSE\n')
        (self.root / 'REUSE.toml').write_text('SPDX-License-Identifier = "GPL-3.0-or-later"\n')

    def test_project_files_are_skipped_outside_a_repository(self):
        self.assertEqual(self.errors('agents-md'), [])
        self.assertEqual(self.errors('notice'), [])
        self.assertEqual(self.errors('license'), [])
        self.assertEqual(self.errors('reuse'), [])

    def test_project_files_pass_when_complete(self):
        self.make_repository()
        self.assertEqual(links.project_file_errors(self.root), [])

    def test_project_files_missing_are_named(self):
        self.make_repository()
        for name in ('AGENTS.md', 'README.md', 'CONTRIBUTING.md', 'LICENSE', 'REUSE.toml'):
            (self.root / name).unlink()
        found = links.project_file_errors(self.root)
        self.assertEqual(len(found), 5)
        for name in ('AGENTS.md', 'README.md', 'CONTRIBUTING.md', 'LICENSE', 'REUSE.toml'):
            self.assertTrue(any(e.startswith(f'{name}:1:') for e in found), name)

    def test_agents_md_must_link_claude_md_and_stay_short(self):
        self.make_repository()
        (self.root / 'AGENTS.md').write_text('# Agents\n\nSee CLAUDE.md\n')
        self.assertEqual([e.split(':')[2].strip() for e in links.project_file_errors(self.root)], ['agents-md-no-claude-link'])
        (self.root / 'AGENTS.md').write_text('[CLAUDE.md](CLAUDE.md)\n' * 16)
        self.assertEqual([e.split(':')[2].strip() for e in links.project_file_errors(self.root)], ['agents-md-size'])
        (self.root / 'AGENTS.md').write_text('[CLAUDE.md](CLAUDE.md)\n' * 15)
        self.assertEqual(links.project_file_errors(self.root), [])

    def test_notice_license_and_reuse_content(self):
        self.make_repository()
        (self.root / 'README.md').write_text('Unrelated.\n')
        (self.root / 'LICENSE').write_text('MIT License\n')
        (self.root / 'REUSE.toml').write_text('SPDX-License-Identifier = "MIT"\n')
        found = links.project_file_errors(self.root)
        self.assertEqual(sorted(e.split(':')[2].strip() for e in found), ['license-text', 'notice-missing', 'reuse-license'])
        self.assertTrue(any(e.startswith('README.md:1:') for e in found))

    def test_claude_md_size(self):
        (self.root / 'CLAUDE.md').write_text('x' * 23552)
        self.assertEqual(self.errors('claude-md-size'), [])
        (self.root / 'CLAUDE.md').write_text('x' * 23553)
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
