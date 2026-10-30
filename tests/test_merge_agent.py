import contextlib
import io
import re
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


def output(cwd: Path, *args: str) -> str:
    return subprocess.run(["git", *args], cwd=cwd, check=True, capture_output=True, text=True).stdout


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
        result = run_script(self.repo, "prepare", "agent", "--no-sync")
        self.assertEqual(result.returncode, 1)
        self.assertIn("game-derived, build or binary files are staged: shot.bmp", result.stderr)

    def test_only_bundled_fonts_pass_the_binary_check(self) -> None:
        sys.path.insert(0, str(SCRIPT.parent))
        import merge_agent

        font = self.repo / "third_party" / "fonts" / "Font.ttf"
        font.parent.mkdir(parents=True)
        font.write_bytes(b"\x00\x01\x00\x00\x00")
        git(self.repo, "add", "-A")
        merge_agent.check_staged_content(self.repo)
        for name in ["fonts/Font.ttf", "third_party/fonts/sub/Font.ttf", "third_party/fonts/Font.otf"]:
            other = self.repo / name
            other.parent.mkdir(parents=True, exist_ok=True)
            other.write_bytes(b"\x00\x01\x00\x00\x00")
            git(self.repo, "add", "-A")
            with self.assertRaisesRegex(merge_agent.MergeError, "binary files are staged: " + re.escape(name)):
                merge_agent.check_staged_content(self.repo)
            git(self.repo, "rm", "-q", "--cached", name)
            other.unlink()

    def test_prepare_without_upstream_stops(self) -> None:
        git(self.repo, "branch", "agent")
        result = run_script(self.repo, "prepare", "agent")
        self.assertEqual(result.returncode, 1)
        self.assertIn("main has no upstream; pass --upstream REMOTE/BRANCH", result.stderr)

    def test_prepare_takes_upstream_or_no_sync_not_both(self) -> None:
        git(self.repo, "branch", "agent")
        result = run_script(self.repo, "prepare", "agent", "--no-sync", "--upstream", "origin/main")
        self.assertEqual(result.returncode, 2)
        self.assertIn("not allowed with argument", result.stderr)

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

    def add_worktree(self, name: str, reason: str | None) -> Path:
        scratch = tempfile.TemporaryDirectory()
        self.addCleanup(scratch.cleanup)
        tree = Path(scratch.name) / name
        git(self.repo, "worktree", "add", "-q", "-b", name, str(tree))
        (tree / "artifacts").symlink_to(self.repo)
        if reason is not None:
            git(self.repo, "worktree", "lock", "--reason", reason, str(tree))
        return tree

    def test_agent_locked_worktree_is_unlocked_and_removed_with_its_branch(self) -> None:
        sys.path.insert(0, str(SCRIPT.parent))
        import merge_agent

        tree = self.add_worktree("tree-a", "claude agent agent-x (pid 1 start 2)")
        merge_agent.remove_worktree_and_branch(self.repo, "tree-a")
        self.assertFalse(tree.exists())
        self.assertNotIn("tree-a", output(self.repo, "branch", "--list"))
        self.assertTrue((self.repo / "a.txt").is_file())

    def test_unlocked_worktree_is_removed(self) -> None:
        sys.path.insert(0, str(SCRIPT.parent))
        import merge_agent

        tree = self.add_worktree("tree-b", None)
        merge_agent.remove_worktree_and_branch(self.repo, "tree-b")
        self.assertFalse(tree.exists())
        self.assertNotIn("tree-b", output(self.repo, "branch", "--list"))

    def test_foreign_lock_keeps_worktree_and_branch_with_leftover_commands(self) -> None:
        sys.path.insert(0, str(SCRIPT.parent))
        import merge_agent

        tree = self.add_worktree("tree-c", "on a usb stick")
        captured = io.StringIO()
        with contextlib.redirect_stderr(captured):
            merge_agent.remove_worktree_and_branch(self.repo, "tree-c")
        self.assertTrue(tree.is_dir())
        self.assertIn("tree-c", output(self.repo, "branch", "--list"))
        self.assertIn(f"warning: worktree {tree} is locked (on a usb stick)", captured.getvalue())
        self.assertIn(f"git worktree remove --force {tree} && git branch -D tree-c", captured.getvalue())

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


class SyncTest(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        base = Path(self.tmp.name)
        self.remote = base / "remote.git"
        git(base, "init", "-q", "--bare", "-b", "main", str(self.remote))
        self.repo = self.clone("lead")
        self.other = self.clone("other")

    def tearDown(self) -> None:
        self.tmp.cleanup()

    def clone(self, name: str) -> Path:
        path = Path(self.tmp.name) / name
        git(Path(self.tmp.name), "clone", "-q", str(self.remote), str(path))
        git(path, "config", "user.email", "test@example.invalid")
        git(path, "config", "user.name", "Test")
        git(path, "checkout", "-q", "-B", "main")
        if not output(path, "ls-remote", "origin", "main").strip():
            (path / "a.txt").write_text("a\n")
            git(path, "add", "a.txt")
            git(path, "commit", "-q", "-m", "init")
            git(path, "push", "-q", "-u", "origin", "main")
        else:
            git(path, "reset", "-q", "--hard", "origin/main")
            git(path, "branch", "-q", "-u", "origin/main")
        return path

    def commit(self, repo: Path, name: str) -> None:
        (repo / name).write_text(name + "\n")
        git(repo, "add", name)
        git(repo, "commit", "-q", "-m", name)

    def test_up_to_date_main_stays(self) -> None:
        result = run_script(self.repo, "sync")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("main is up to date with origin/main (0 own commits not pushed)", result.stdout)

    def test_remote_ahead_is_fast_forwarded(self) -> None:
        self.commit(self.other, "b.txt")
        git(self.other, "push", "-q", "origin", "main")
        result = run_script(self.repo, "sync")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("fast-forwarded main by 1 commits to origin/main", result.stdout)
        self.assertTrue((self.repo / "b.txt").is_file())

    def test_diverged_main_stops_for_the_user(self) -> None:
        self.commit(self.other, "b.txt")
        git(self.other, "push", "-q", "origin", "main")
        self.commit(self.repo, "c.txt")
        result = run_script(self.repo, "sync")
        self.assertEqual(result.returncode, 1)
        self.assertIn("main and origin/main have diverged (1 own, 1 new upstream commits): stop and ask the user", result.stderr)
        self.assertFalse((self.repo / "b.txt").exists())

    def test_own_commits_ahead_are_reported(self) -> None:
        self.commit(self.repo, "c.txt")
        result = run_script(self.repo, "sync")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("(1 own commits not pushed)", result.stdout)

    def test_uncommitted_changes_block_the_fast_forward(self) -> None:
        self.commit(self.other, "b.txt")
        git(self.other, "push", "-q", "origin", "main")
        (self.repo / "a.txt").write_text("changed\n")
        result = run_script(self.repo, "sync")
        self.assertEqual(result.returncode, 1)
        self.assertIn("main has uncommitted changes: stop and ask the user", result.stderr)

    def test_bad_upstream_is_named(self) -> None:
        for value in ("nowhere/main", "origin", "origin/"):
            result = run_script(self.repo, "sync", "--upstream", value)
            self.assertEqual(result.returncode, 1, value)
            self.assertIn(f"--upstream: expected REMOTE/BRANCH with REMOTE one of: origin; got {value!r}", result.stderr)
        result = run_script(self.repo, "sync", "--upstream", "origin/nobranch")
        self.assertEqual(result.returncode, 1)
        self.assertIn("--upstream: origin/nobranch does not exist after the fetch", result.stderr)

    def test_prepare_syncs_before_the_squash(self) -> None:
        git(self.repo, "branch", "agent")
        self.commit(self.other, "b.txt")
        git(self.other, "push", "-q", "origin", "main")
        self.commit(self.repo, "c.txt")
        result = run_script(self.repo, "prepare", "agent")
        self.assertEqual(result.returncode, 1)
        self.assertIn("have diverged", result.stderr)
        self.assertEqual(output(self.repo, "diff", "--cached", "--name-only"), "")


RESERVATIONS = ("# ID reservations\n\nText.\n\n| Prefix | First | Last | Holder | Date |\n|---|---|---|---|---|\n"
                "| F | 3 | 4 | 20261021-audio-A | 2026-10-21T10:00:01+02:00 |\n"
                "| F | 5 | 6 | 20261021-audio-B | 2026-10-21T10:00:02+02:00 |\n")
PROGRESS = "# Progress\n\n## Session Log\n\nIntro.\n\n- 2026-10-21T09:00:01+02:00 — Agent 20261021-audio-A (sfx): done.\n"
ROUND_END = "Lead (20261021-audio end): merged 20261021-audio-A (sfx); pushed main to origin/main after each merge"


class BookkeepingTest(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.repo = Path(self.tmp.name) / "repo"
        self.repo.mkdir()
        git(self.repo, "init", "-q", "-b", "main")
        git(self.repo, "config", "user.email", "test@example.invalid")
        git(self.repo, "config", "user.name", "Test")
        tools = self.repo / "tools"
        tools.mkdir()
        for name in ("check_doc_links.py", "user_docs.py"):
            (tools / name).write_text(SCRIPT.with_name(name).read_text())
        features = self.repo / "docs" / "plans" / "features"
        features.mkdir(parents=True)
        (features / "f.md").write_text("- [ ] F-1 open\n")
        (features / "id-reservations.md").write_text(RESERVATIONS)
        (self.repo / "docs" / "plans" / "linux-port.progress.md").write_text(PROGRESS)
        git(self.repo, "add", "-A")
        git(self.repo, "commit", "-q", "-m", "init")
        self.line = Path(self.tmp.name) / "line.txt"
        self.line.write_text(ROUND_END + "\n")
        self.message = Path(self.tmp.name) / "message.txt"
        self.message.write_text("Log the end\n\nCo-Authored-By: Test <test@example.invalid>\n")
        sys.path.insert(0, str(SCRIPT.parent))
        import merge_agent
        self.merge_agent = merge_agent

    def tearDown(self) -> None:
        self.tmp.cleanup()

    def reservations(self) -> str:
        return (self.repo / "docs" / "plans" / "features" / "id-reservations.md").read_text()

    def progress(self) -> str:
        return (self.repo / "docs" / "plans" / "linux-port.progress.md").read_text()

    def test_release_and_log_go_into_the_staged_merge(self) -> None:
        self.merge_agent.bookkeeping(self.repo, ["20261021-audio-A"], ["linux-port", str(self.line)])
        self.assertNotIn("20261021-audio-A", self.reservations())
        self.assertIn("20261021-audio-B", self.reservations())
        last = self.progress().rstrip("\n").split("\n")[-1]
        self.assertRegex(last, r"^- \d{4}-\d\d-\d\dT\d\d:\d\d:\d\d[+-]\d\d:\d\d — " + re.escape(ROUND_END) + "$")
        staged = output(self.repo, "diff", "--cached", "--name-only").split()
        self.assertEqual(staged, ["docs/plans/features/id-reservations.md", "docs/plans/linux-port.progress.md"])

    def test_bookkeeping_runs_again_after_a_failed_check(self) -> None:
        self.merge_agent.bookkeeping(self.repo, ["20261021-audio-A"], ["linux-port", str(self.line)])
        before = self.progress()
        self.merge_agent.bookkeeping(self.repo, ["20261021-audio-A"], ["linux-port", str(self.line)])
        self.assertEqual(self.progress(), before)
        self.assertEqual(self.progress().count(ROUND_END), 1)

    def test_unknown_holder_is_refused_before_any_change(self) -> None:
        with self.assertRaisesRegex(self.merge_agent.MergeError, "--release: no reservation held by 'r48-A'"):
            self.merge_agent.bookkeeping(self.repo, ["20261021-audio-A", "r48-A"], ["linux-port", str(self.line)])
        self.assertEqual(self.reservations(), RESERVATIONS)
        self.assertEqual(self.progress(), PROGRESS)
        with self.assertRaisesRegex(self.merge_agent.MergeError, "--release: WHO must be a reservation holder"):
            self.merge_agent.bookkeeping(self.repo, ["a|b"], None)

    def test_bad_log_lines_are_refused(self) -> None:
        cases = {
            "Lead (round 49 end): merged r49-A": "with a round name like 20261021-audio (no round name)",
            "Agent 20261021-audio-A (sfx): done": "(no round name)",
            "Lead (20261021-Audio end): merged": "expected YYYYMMDD-slug",
            "Lead (29991021-audio end): merged": "the date is in the future",
            ROUND_END + "\nsecond line": "must hold exactly one line, got 2",
            "": "must hold exactly one line, got 0",
        }
        for text, expected in cases.items():
            self.line.write_text(text)
            with self.assertRaises(self.merge_agent.MergeError, msg=text) as caught:
                self.merge_agent.bookkeeping(self.repo, ["20261021-audio-A"], ["linux-port", str(self.line)])
            self.assertIn(expected, str(caught.exception), text)
            self.assertEqual(self.reservations(), RESERVATIONS, text)
        self.line.write_text(ROUND_END)
        for plan, expected in (("qol", "plan 'qol' has no progress file"), ("Linux", "PLAN must be a plan name")):
            with self.assertRaisesRegex(self.merge_agent.MergeError, re.escape(expected)):
                self.merge_agent.bookkeeping(self.repo, [], [plan, str(self.line)])
        with self.assertRaisesRegex(self.merge_agent.MergeError, "--log: cannot read"):
            self.merge_agent.bookkeeping(self.repo, [], ["linux-port", str(self.line) + ".missing"])

    def test_finish_options_are_parsed_strictly(self) -> None:
        result = run_script(self.repo, "finish", "agent", "-m", str(self.message), "--log", "linux-port")
        self.assertEqual(result.returncode, 2)
        self.assertIn("--log: expected 2 arguments", result.stderr)
        result = run_script(self.repo, "finish", "agent", "-m", str(self.message), "--release")
        self.assertEqual(result.returncode, 2)
        self.assertIn("--release: expected one argument", result.stderr)

    def test_close_needs_something_to_do(self) -> None:
        result = run_script(self.repo, "close", "-m", str(self.message))
        self.assertEqual(result.returncode, 1)
        self.assertIn("close: nothing to do; give --release WHO and/or --log PLAN LINE_FILE", result.stderr)

    def test_close_refuses_an_unknown_holder_without_committing(self) -> None:
        result = run_script(self.repo, "close", "-m", str(self.message), "--release", "nobody")
        self.assertEqual(result.returncode, 1)
        self.assertIn("--release: no reservation held by 'nobody'", result.stderr)
        self.assertEqual(output(self.repo, "log", "--oneline").count("\n"), 1)



class VerifiedFingerprintTest(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.repo = Path(self.tmp.name)
        git(self.repo, "init", "-q", "-b", "main")
        git(self.repo, "config", "user.email", "test@example.invalid")
        git(self.repo, "config", "user.name", "Test")
        (self.repo / "a.txt").write_text("a\n")
        (self.repo / "README.md").write_text("r\n")
        (self.repo / "docs").mkdir()
        (self.repo / "docs" / "progress-trend.svg").write_text("<svg/>\n")
        git(self.repo, "add", "-A")
        git(self.repo, "commit", "-q", "-m", "init")
        sys.path.insert(0, str(SCRIPT.parent))
        import merge_agent
        self.merge_agent = merge_agent
        self.calls: list[str] = []
        self.failing = False
        self.saved = (merge_agent.run, merge_agent.build_warning_free, merge_agent.update_priorities, merge_agent.fix_docs)
        merge_agent.build_warning_free = lambda root: self.calls.append("build")
        merge_agent.update_priorities = lambda root: None
        merge_agent.fix_docs = lambda root: None
        merge_agent.run = self.fake_run

    def tearDown(self) -> None:
        (self.merge_agent.run, self.merge_agent.build_warning_free, self.merge_agent.update_priorities, self.merge_agent.fix_docs) = self.saved
        self.tmp.cleanup()

    def fake_run(self, cmd, cwd, check=True, capture=False):
        name = cmd[0] if cmd[0] in ("ctest",) else "unittest" if "unittest" in cmd else None
        if name:
            self.calls.append(name)
            if self.failing and name == "ctest":
                raise self.merge_agent.MergeError("ctest failed")
            return subprocess.CompletedProcess(cmd, 0, "", "")
        if cmd[0] == "git":
            return self.saved[0](cmd, cwd, check, capture)
        return subprocess.CompletedProcess(cmd, 0, "", "")

    def stage_change(self, text: str) -> None:
        (self.repo / "a.txt").write_text(text)
        git(self.repo, "add", "a.txt")

    def test_matching_record_skips_the_heavy_checks(self) -> None:
        self.stage_change("b\n")
        self.merge_agent.run_checks(self.repo)
        self.assertEqual(self.calls, ["build", "ctest", "unittest"])
        self.calls.clear()
        verified = self.merge_agent.recorded_fingerprint(self.repo) == self.merge_agent.fingerprint(self.repo)
        self.assertTrue(verified)
        self.merge_agent.run_checks(self.repo, skip_heavy=verified)
        self.assertEqual(self.calls, [])

    def test_changed_tree_does_not_match(self) -> None:
        self.stage_change("b\n")
        self.merge_agent.run_checks(self.repo)
        self.stage_change("c\n")
        self.assertNotEqual(self.merge_agent.recorded_fingerprint(self.repo), self.merge_agent.fingerprint(self.repo))

    def test_changed_head_does_not_match(self) -> None:
        self.stage_change("b\n")
        self.merge_agent.run_checks(self.repo)
        git(self.repo, "commit", "-q", "--allow-empty", "-m", "more")
        self.assertNotEqual(self.merge_agent.recorded_fingerprint(self.repo), self.merge_agent.fingerprint(self.repo))

    def test_missing_record_does_not_match(self) -> None:
        self.assertIsNone(self.merge_agent.recorded_fingerprint(self.repo))

    def test_failed_check_records_nothing_and_clears_the_old_record(self) -> None:
        self.stage_change("b\n")
        self.merge_agent.run_checks(self.repo)
        self.assertIsNotNone(self.merge_agent.recorded_fingerprint(self.repo))
        self.failing = True
        with self.assertRaises(self.merge_agent.MergeError):
            self.merge_agent.run_checks(self.repo)
        self.assertIsNone(self.merge_agent.recorded_fingerprint(self.repo))


if __name__ == "__main__":
    unittest.main()
