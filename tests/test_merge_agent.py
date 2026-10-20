import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parent.parent / "tools" / "merge_agent.py"


def run_script(cwd: Path, *args: str) -> subprocess.CompletedProcess:
    return subprocess.run([sys.executable, str(SCRIPT), *args], cwd=cwd, text=True, capture_output=True)


def git(cwd: Path, *args: str) -> None:
    subprocess.run(["git", *args], cwd=cwd, check=True, capture_output=True)


class MergeAgentTest(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.repo = Path(self.tmp.name)
        git(self.repo, "init", "-q", "-b", "main")
        git(self.repo, "config", "user.email", "test@example.invalid")
        git(self.repo, "config", "user.name", "Test")
        (self.repo / "a.txt").write_text("a\n")
        git(self.repo, "add", "a.txt")
        git(self.repo, "commit", "-q", "-m", "init")

    def tearDown(self) -> None:
        self.tmp.cleanup()

    def test_unknown_command_is_rejected(self) -> None:
        result = run_script(self.repo, "merge", "x")
        self.assertEqual(result.returncode, 2)
        self.assertIn("invalid choice: 'merge'", result.stderr)

    def test_finish_needs_a_message(self) -> None:
        result = run_script(self.repo, "finish", "agent")
        self.assertEqual(result.returncode, 2)
        self.assertIn("--message", result.stderr)

    def test_missing_branch_is_named(self) -> None:
        result = run_script(self.repo, "prepare", "no-such-branch")
        self.assertEqual(result.returncode, 1)
        self.assertIn("branch 'no-such-branch' does not exist", result.stderr)

    def test_not_on_main_is_rejected(self) -> None:
        git(self.repo, "checkout", "-q", "-b", "other")
        result = run_script(self.repo, "prepare", "main")
        self.assertEqual(result.returncode, 1)
        self.assertIn("must be on main, not 'other'", result.stderr)

    def test_message_without_co_author_is_refused(self) -> None:
        git(self.repo, "branch", "agent")
        message = self.repo / "message.txt"
        message.write_text("Subject\n\nBody\n")
        result = run_script(self.repo, "finish", "agent", "-m", str(message))
        self.assertEqual(result.returncode, 1)
        self.assertIn("has no Co-Authored-By line", result.stderr)

    def test_game_derived_files_are_refused(self) -> None:
        git(self.repo, "checkout", "-q", "-b", "agent")
        (self.repo / "shot.bmp").write_bytes(b"BM\x00\x01")
        git(self.repo, "add", "shot.bmp")
        git(self.repo, "commit", "-q", "-m", "shot")
        git(self.repo, "checkout", "-q", "main")
        result = run_script(self.repo, "prepare", "agent")
        self.assertEqual(result.returncode, 1)
        self.assertIn("game-derived, build or binary files are staged: shot.bmp", result.stderr)

    def test_fix_docs_runs_the_doc_fixer_and_restages(self) -> None:
        tools = self.repo / "tools"
        tools.mkdir()
        (tools / "check_doc_links.py").write_text(SCRIPT.with_name("check_doc_links.py").read_text())
        (tools / "user_docs.py").write_text(SCRIPT.with_name("user_docs.py").read_text())
        features = self.repo / "docs" / "plans" / "features"
        features.mkdir(parents=True)
        (features / "f.md").write_text("- [ ] F-1 open\n- [x] F-2 done\n")
        git(self.repo, "add", "-A")
        sys.path.insert(0, str(SCRIPT.parent))
        import merge_agent

        merge_agent.fix_docs(self.repo)
        staged = subprocess.run(["git", "diff", "--cached", "--name-only"], cwd=self.repo, text=True, capture_output=True).stdout.split()
        self.assertIn("docs/plans/features/f.closed.md", staged)
        self.assertEqual((features / "f.md").read_text(), "- [ ] F-1 open\n")

    def test_forbidden_matches_build_directories_only(self) -> None:
        sys.path.insert(0, str(SCRIPT.parent))
        import merge_agent

        self.assertTrue(merge_agent.FORBIDDEN.search("build/chase-game"))
        self.assertTrue(merge_agent.FORBIDDEN.search("build-release/x.o"))
        self.assertTrue(merge_agent.FORBIDDEN.search("artifacts/game/code.bin"))
        self.assertFalse(merge_agent.FORBIDDEN.search("src/runtime/buildit.cpp"))
        self.assertFalse(merge_agent.FORBIDDEN.search("docs/re/buildit.md"))

    def test_oracle_outputs_are_refused_but_test_tables_are_not(self) -> None:
        sys.path.insert(0, str(SCRIPT.parent))
        import merge_agent

        for name in ("trace.mem", "tests/data/trace.mem", "state.cst", "oracle/session.json", "shot.png"):
            self.assertTrue(merge_agent.FORBIDDEN.search(name), name)
        for name in ("tests/data/camera/blend/street_in.csv", "tests/data/session_notes.json", "tools/oracle/to-gameplay.txt"):
            self.assertFalse(merge_agent.FORBIDDEN.search(name), name)

    def test_archive_conflict_keeps_both_sides(self) -> None:
        sys.path.insert(0, str(SCRIPT.parent))
        import merge_agent

        archive = self.repo / "x.progress-archive.md"
        archive.write_text("# Archive\n\n- old\n<<<<<<< HEAD\n- from main\n=======\n- from branch\n- old\n>>>>>>> agent\n")
        merge_agent.keep_both_sides(archive)
        self.assertEqual(archive.read_text(), "# Archive\n\n- old\n- from main\n- from branch\n- old\n")

    def test_readme_conflict_inside_progress_section_keeps_main(self) -> None:
        sys.path.insert(0, str(SCRIPT.parent))
        import merge_agent

        text = ("# T\n\ncmake RelWithDebInfo\n<!-- progress:start -->\n<<<<<<< HEAD\n| 5 tests |\n=======\n| 4 tests |\n"
                ">>>>>>> agent\n<!-- progress:end -->\n")
        self.assertEqual(merge_agent.resolve_progress_conflicts(text),
                         "# T\n\ncmake RelWithDebInfo\n<!-- progress:start -->\n| 5 tests |\n<!-- progress:end -->\n")

    def test_readme_conflict_outside_progress_section_is_not_resolved(self) -> None:
        sys.path.insert(0, str(SCRIPT.parent))
        import merge_agent

        text = ("# T\n\n<<<<<<< HEAD\ncmake RelWithDebInfo\n=======\ncmake Debug\n>>>>>>> agent\n<!-- progress:start -->\n"
                "| 5 tests |\n<!-- progress:end -->\n")
        self.assertIsNone(merge_agent.resolve_progress_conflicts(text))

    def test_priorities_conflict_takes_main_and_is_staged(self) -> None:
        sys.path.insert(0, str(SCRIPT.parent))
        import merge_agent

        target = self.repo / merge_agent.PRIORITIES
        target.parent.mkdir(parents=True)
        target.write_text("base\n")
        git(self.repo, "add", "-A")
        git(self.repo, "commit", "-q", "-m", "priorities")
        git(self.repo, "checkout", "-q", "-b", "agent")
        target.write_text("from agent\n")
        git(self.repo, "commit", "-q", "-am", "agent")
        git(self.repo, "checkout", "-q", "main")
        target.write_text("from main\n")
        git(self.repo, "commit", "-q", "-am", "main")
        subprocess.run(["git", "merge", "--squash", "agent"], cwd=self.repo, capture_output=True)
        unmerged = subprocess.run(["git", "diff", "--name-only", "--diff-filter=U"], cwd=self.repo, text=True, capture_output=True).stdout.split()
        self.assertEqual(unmerged, [merge_agent.PRIORITIES])
        self.assertEqual(merge_agent.resolve_generated(self.repo, unmerged), [])
        self.assertEqual(target.read_text(), "from main\n")
        left = subprocess.run(["git", "diff", "--name-only", "--diff-filter=U"], cwd=self.repo, text=True, capture_output=True).stdout
        self.assertEqual(left, "")


if __name__ == "__main__":
    unittest.main()
