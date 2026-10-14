import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parents[1] / 'tools'))
import check_doc_links as links  # noqa: E402

RE_FILE = """# Topic

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

    def test_feature_and_index_length(self):
        (self.features / 'ok.md').write_text('line\n' * 150)
        (self.features / 'long.md').write_text('line\n' * 151)
        (self.features / 'README.md').write_text('line\n' * 41)
        found = self.errors('-length')
        self.assertEqual(len(found), 2)
        self.assertTrue(any('long.md:151: feature-file-length' in e for e in found))
        self.assertTrue(any('README.md:41: index-file-length' in e for e in found))

    def test_item_and_stand_in_length(self):
        long_text = 'y' * 400
        (self.features / 'f.md').write_text(
            f'- [ ] FOO-1 {long_text}\n'
            f'- [x] FOO-2 short\n'
            f'- FOO-S1 {long_text}\n'
            f'- FOO-S2 short\n'
            f'Prose line {long_text}\n'
        )
        found = self.errors('item-length')
        self.assertEqual(len(found), 2)
        self.assertTrue(any('f.md:1: item-length' in e for e in found))
        self.assertTrue(any('f.md:3: item-length' in e for e in found))

    def test_claude_md_size(self):
        (self.root / 'CLAUDE.md').write_text('x' * 12800)
        self.assertEqual(self.errors('claude-md-size'), [])
        (self.root / 'CLAUDE.md').write_text('x' * 12801)
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
