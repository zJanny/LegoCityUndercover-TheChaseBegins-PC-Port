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


if __name__ == '__main__':
    unittest.main()
