import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parents[1] / 'tools'))
import user_docs  # noqa: E402
sys.path.insert(0, str(Path(__file__).resolve().parent))
from fixtures import isolate_git_environment  # noqa: E402

isolate_git_environment()

COMMANDS = "# Commands\n\n`chase-game --level NAME` starts a level.\n\n```\nchase-game --frames 5\n```\n"


class UserDocsTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        (self.root / 'docs').mkdir()
        (self.root / 'src').mkdir()
        (self.root / 'tools').mkdir()
        (self.root / 'tools' / 'x.py').write_text('', encoding='utf-8', newline='\n')
        (self.root / 'CMakeLists.txt').write_text('add_executable(chase-game src/game.cpp)\n', encoding='utf-8', newline='\n')
        (self.root / 'src' / 'game.cpp').write_text('const char* a = "--level"; const char* b = "--frames";\n', encoding='utf-8', newline='\n')
        (self.root / 'docs' / 'commands.md').write_text(COMMANDS, encoding='utf-8', newline='\n')

    def tearDown(self):
        self.tmp.cleanup()

    def errors(self, rule):
        return [e for e in user_docs.user_doc_errors(self.root) if rule in e]

    def test_consistent_docs_pass(self):
        self.assertEqual(user_docs.user_doc_errors(self.root), [])

    def test_option_the_code_accepts_must_be_documented(self):
        (self.root / 'src' / 'game.cpp').write_text('"--level" "--frames" "--new-thing"', encoding='utf-8', newline='\n')
        found = self.errors('user-doc-option-undocumented')
        self.assertEqual(len(found), 1)
        self.assertIn('--new-thing', found[0])
        (self.root / 'README.md').write_text('Use `--new-thing`.\n', encoding='utf-8', newline='\n')
        (self.root / 'tools' / 'x.py').write_text('--new-thing', encoding='utf-8', newline='\n')
        self.assertEqual(self.errors('user-doc-option-undocumented'), [])

    def test_documented_option_must_exist(self):
        (self.root / 'docs' / 'commands.md').write_text(COMMANDS + '\nUse `--gone` or\n\n```\nchase-game --gone2\n```\n', encoding='utf-8', newline='\n')
        found = self.errors('user-doc-option-unknown')
        self.assertEqual(len(found), 2)
        self.assertTrue(any('--gone ' in e for e in found))
        self.assertTrue(any('--gone2' in e for e in found))

    def test_options_of_other_tools_found_in_tools_pass(self):
        (self.root / 'tools' / 'x.py').write_text("parser.add_argument('--keep')", encoding='utf-8', newline='\n')
        (self.root / 'docs' / 'commands.md').write_text(COMMANDS + '\n`x.py --keep`\n', encoding='utf-8', newline='\n')
        self.assertEqual(self.errors('user-doc-option-unknown'), [])

    def test_program_must_be_documented_and_exist(self):
        (self.root / 'CMakeLists.txt').write_text('add_executable(chase-game src/game.cpp)\nadd_executable(chase-tool src/t.cpp)\n', encoding='utf-8', newline='\n')
        self.assertEqual(len(self.errors('user-doc-program-undocumented')), 1)
        (self.root / 'docs' / 'commands.md').write_text(COMMANDS + '\n`chase-tool`, `chase-ghost`\n', encoding='utf-8', newline='\n')
        self.assertEqual(self.errors('user-doc-program-undocumented'), [])
        found = self.errors('user-doc-program-unknown')
        self.assertEqual(len(found), 1)
        self.assertIn('chase-ghost', found[0])

    def test_named_paths_must_exist(self):
        (self.root / 'docs' / 'commands.md').write_text(COMMANDS + '\n`tools/x.py`, `tools/missing.py`, `tools/oracle/jump-`, `src/*.cpp`\n', encoding='utf-8', newline='\n')
        found = self.errors('user-doc-path-missing')
        self.assertEqual(len(found), 1)
        self.assertIn('tools/missing.py', found[0])

    def test_prose_outside_code_is_not_checked(self):
        (self.root / 'docs' / 'commands.md').write_text(COMMANDS + '\nThe --gone option and chase-ghost in prose, tools/missing.py too.\n', encoding='utf-8', newline='\n')
        self.assertEqual(user_docs.user_doc_errors(self.root), [])

    def test_subfolder_docs_are_not_user_docs(self):
        (self.root / 'docs' / 're').mkdir()
        (self.root / 'docs' / 're' / 't.md').write_text('`--gone` `tools/missing.py`\n', encoding='utf-8', newline='\n')
        self.assertEqual(user_docs.user_doc_errors(self.root), [])

    def test_option_and_program_checks_need_a_source_tree(self):
        (self.root / 'src' / 'game.cpp').unlink()
        (self.root / 'src').rmdir()
        (self.root / 'docs' / 'commands.md').write_text('`--gone` `chase-ghost`\n', encoding='utf-8', newline='\n')
        self.assertEqual(user_docs.user_doc_errors(self.root), [])


if __name__ == '__main__':
    unittest.main()
