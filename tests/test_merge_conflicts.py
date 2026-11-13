import ast
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
import merge_conflicts  # noqa: E402
sys.path.insert(0, str(Path(__file__).resolve().parent))
from fixtures import isolate_git_environment  # noqa: E402

isolate_git_environment()

SCRIPT = Path(merge_conflicts.__file__)
LOG = "docs/plans/linux-port.progress.md"
RE = "docs/re/topic.md"
DESIGN = "docs/design/topic.md"
DECISION = "# Topic\n\n## 2026-01-01T01:01:01+01:00 — Decision\n\n- answer\n"
OTHER = "docs/notes.md"


def git(cwd, *args, check=True):
    return subprocess.run(["git", *args], cwd=cwd, check=check, capture_output=True, text=True)


class MergeConflictsTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.repo = Path(self.tmp.name)
        git(self.repo, "init", "-q", "-b", "main")
        git(self.repo, "config", "user.email", "test@example.invalid")
        git(self.repo, "config", "user.name", "Test")
        self.write(LOG, "# Log\n\n- first\n")
        self.write(RE, "# Topic\n\n## Current state\n\nx\n")
        self.write(OTHER, "notes\n")
        self.write(DESIGN, DECISION)
        git(self.repo, "add", ".")
        git(self.repo, "commit", "-q", "-m", "init")
        git(self.repo, "checkout", "-q", "-b", "branch")

    def tearDown(self):
        self.tmp.cleanup()

    def write(self, name, text):
        path = self.repo / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8", newline='\n')

    def commit_on(self, branch, changes):
        git(self.repo, "checkout", "-q", branch)
        for name, text in changes.items():
            self.write(name, text)
        git(self.repo, "add", ".")
        git(self.repo, "commit", "-q", "-m", "change")

    def conflicted_merge(self, main_changes, branch_changes):
        self.commit_on("branch", branch_changes)
        self.commit_on("main", main_changes)
        result = git(self.repo, "merge", "--squash", "branch", check=False)
        self.assertNotEqual(result.returncode, 0)

    def run_script(self):
        return subprocess.run([sys.executable, str(SCRIPT), "--root", str(self.repo)], capture_output=True, text=True)

    def test_non_ascii_text_survives_a_non_utf8_default_encoding(self):
        env = {name: value for name, value in os.environ.items() if name not in ("PYTHONIOENCODING", "PYTHONUTF8", "LC_ALL", "LANG")}
        env.update(PYTHONUTF8="0", PYTHONCOERCECLOCALE="0", LC_ALL="C")
        dash = "—"
        self.conflicted_merge({LOG: f"# Progress {dash} Port\n\n- first\n- main {dash} one\n"},
                              {LOG: f"# Progress {dash} Port\n\n- first\n- branch {dash} two\n"})
        result = subprocess.run([sys.executable, "-X", "utf8=0", str(SCRIPT), "--root", str(self.repo)], capture_output=True, env=env)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        expected = f"# Progress {dash} Port\n\n- first\n- main {dash} one\n- branch {dash} two\n"
        self.assertEqual((self.repo / LOG).read_bytes(), expected.encode("utf-8"))

    def test_appended_lines_of_both_sides_are_kept_in_order(self):
        self.conflicted_merge({LOG: "# Log\n\n- first\n- main\n", RE: "# Topic\n\n## Current state\n\nx\n\n## main entry\n"},
                              {LOG: "# Log\n\n- first\n- branch\n", RE: "# Topic\n\n## Current state\n\nx\n\n## branch entry\n"})
        result = self.run_script()
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertEqual((self.repo / LOG).read_text(encoding='utf-8'), "# Log\n\n- first\n- main\n- branch\n")
        self.assertEqual((self.repo / RE).read_text(encoding='utf-8'), "# Topic\n\n## Current state\n\nx\n\n## main entry\n\n## branch entry\n")
        self.assertEqual(git(self.repo, "diff", "--name-only", "--diff-filter=U").stdout, "")

    def test_changed_existing_lines_stay_conflicted(self):
        self.conflicted_merge({LOG: "# Log\n\n- first changed by main\n"}, {LOG: "# Log\n\n- first changed by branch\n"})
        result = self.run_script()
        self.assertEqual(result.returncode, 1)
        self.assertIn(LOG, result.stdout)
        self.assertIn("<<<<<<<", (self.repo / LOG).read_text(encoding='utf-8'))

    def test_other_files_are_refused_and_listed(self):
        self.conflicted_merge({OTHER: "notes\nmain\n"}, {OTHER: "notes\nbranch\n"})
        result = self.run_script()
        self.assertEqual(result.returncode, 1)
        self.assertIn(OTHER, result.stdout)
        self.assertIn("<<<<<<<", (self.repo / OTHER).read_text(encoding='utf-8'))

    def test_mixed_conflicts_resolve_only_the_allowed_file(self):
        self.conflicted_merge({LOG: "# Log\n\n- first\n- main\n", OTHER: "notes\nmain\n"},
                              {LOG: "# Log\n\n- first\n- branch\n", OTHER: "notes\nbranch\n"})
        result = self.run_script()
        self.assertEqual(result.returncode, 1)
        self.assertIn("resolved: " + LOG, result.stdout)
        self.assertEqual(git(self.repo, "diff", "--name-only", "--diff-filter=U").stdout.split(), [OTHER])

    def test_design_conflicts_stay_for_a_manual_resolution(self):
        self.conflicted_merge({DESIGN: DECISION + "- main\n", RE: "# Topic\n\n## Current state\n\nx\n\n## main entry\n"},
                              {DESIGN: DECISION + "- branch\n", RE: "# Topic\n\n## Current state\n\nx\n\n## branch entry\n"})
        result = self.run_script()
        self.assertEqual(result.returncode, 1)
        self.assertIn("resolved: " + RE, result.stdout)
        self.assertEqual(git(self.repo, "diff", "--name-only", "--diff-filter=U").stdout.split(), [DESIGN])
        self.assertIn("<<<<<<<", (self.repo / DESIGN).read_text(encoding='utf-8'))

    def test_unknown_option_is_rejected(self):
        result = subprocess.run([sys.executable, str(SCRIPT), "--bogus"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 2)


class TextDecodingGuardTest(unittest.TestCase):
    def test_tools_decode_subprocess_text_as_utf8(self):
        offenders = []
        for path in sorted((Path(__file__).resolve().parent.parent / "tools").glob("*.py")):
            for node in ast.walk(ast.parse(path.read_text(encoding="utf-8"))):
                if not isinstance(node, ast.Call):
                    continue
                keywords = {keyword.arg: keyword.value for keyword in node.keywords}
                text_mode = any(isinstance(keywords.get(name), ast.Constant) and keywords[name].value is True for name in ("text", "universal_newlines"))
                encoding = keywords.get("encoding")
                if text_mode and not (isinstance(encoding, ast.Constant) and encoding.value == "utf-8"):
                    offenders.append(f"{path.name}:{node.lineno}")
        self.assertEqual(offenders, [], "subprocess text mode without encoding='utf-8' decodes with the Windows code page")


if __name__ == "__main__":
    unittest.main()
