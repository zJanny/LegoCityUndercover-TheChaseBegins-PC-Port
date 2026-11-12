import contextlib
import io
import os
import re
import subprocess
import sys
import tempfile
import unittest
from unittest import mock
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


CENSUS_STAND_IN = '''"""Stand-in for tools/generated_file_census.py of the CI round, only for these tests: the interface without game data."""
ROWS = MEASURED


def measure_census():
    return ROWS


def write_census(path, rows):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("section,key,value,provenance\\n" + "".join(",".join(map(str, row)) + "\\n" for row in sorted(rows)))
'''


class CensusRefreshTest(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.repo = Path(self.tmp.name)
        git(self.repo, "init", "-q", "-b", "main")
        git(self.repo, "config", "user.email", "test@example.invalid")
        git(self.repo, "config", "user.name", "Test")
        (self.repo / "a.txt").write_text("a\n")
        git(self.repo, "add", "a.txt")
        git(self.repo, "commit", "-q", "-m", "init")
        sys.path.insert(0, str(SCRIPT.parent))
        import merge_agent
        self.merge_agent = merge_agent
        self.table = self.repo / merge_agent.CENSUS

    def tearDown(self) -> None:
        self.tmp.cleanup()

    def stand_in(self, measured: str) -> None:
        (self.repo / "tools").mkdir(exist_ok=True)
        (self.repo / "tools" / "generated_file_census.py").write_text(CENSUS_STAND_IN.replace("MEASURED", measured))

    def refresh(self) -> str:
        captured = io.StringIO()
        with contextlib.redirect_stdout(captured):
            self.merge_agent.refresh_census(self.repo)
        return captured.getvalue()

    def test_measured_table_is_written_and_staged(self) -> None:
        self.stand_in("[('readme_progress', 'files_total', 12, 'census 2026-11-12')]")
        self.assertIn("census table refreshed and staged", self.refresh())
        self.assertEqual(self.table.read_text(), "section,key,value,provenance\nreadme_progress,files_total,12,census 2026-11-12\n")
        self.assertIn(self.merge_agent.CENSUS, output(self.repo, "diff", "--cached", "--name-only").split())

    def test_unchanged_table_stays_out_of_the_commit(self) -> None:
        self.stand_in("[('priorities', 'f.x.calls', 3, 'census 2026-11-12')]")
        self.refresh()
        git(self.repo, "commit", "-q", "-m", "table")
        self.assertIn("census table unchanged", self.refresh())
        self.assertEqual(output(self.repo, "diff", "--cached", "--name-only"), "")

    def test_without_game_data_the_table_is_kept(self) -> None:
        self.stand_in("None")
        self.assertIn("census table kept: no game data", self.refresh())
        self.assertFalse(self.table.exists())

    def test_without_the_census_module_nothing_happens(self) -> None:
        self.assertIn("tools/generated_file_census.py is missing", self.refresh())
        self.assertFalse(self.table.exists())


class CheckPullRequestTest(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        base = Path(self.tmp.name)
        self.remote = base / "github.git"
        git(base, "init", "-q", "--bare", "-b", "main", str(self.remote))
        self.repo = base / "lead"
        git(base, "clone", "-q", str(self.remote), str(self.repo))
        git(self.repo, "config", "user.email", "test@example.invalid")
        git(self.repo, "config", "user.name", "Test")
        git(self.repo, "checkout", "-q", "-B", "main")
        (self.repo / "a.txt").write_text("a\n")
        git(self.repo, "add", "a.txt")
        git(self.repo, "commit", "-q", "-m", "init")
        git(self.repo, "push", "-q", "origin", "main")
        (self.repo / "artifacts").mkdir()
        self.scratch = base / "temp"
        self.scratch.mkdir()
        self.saved_tempdir = tempfile.tempdir
        tempfile.tempdir = str(self.scratch)
        sys.path.insert(0, str(SCRIPT.parent))
        import merge_agent
        self.merge_agent = merge_agent
        self.saved_checks = merge_agent.pull_request_checks
        self.checked: list[tuple[Path, dict]] = []
        self.result: list[str] | BaseException = []
        merge_agent.pull_request_checks = self.fake_checks

    def tearDown(self) -> None:
        self.merge_agent.pull_request_checks = self.saved_checks
        tempfile.tempdir = self.saved_tempdir
        self.tmp.cleanup()

    def fake_checks(self, tree: Path, env: dict) -> list[str]:
        self.checked.append((tree, env))
        self.seen = {
            "files": sorted(output(tree, "ls-files").split()),
            "branch": output(tree, "rev-parse", "--abbrev-ref", "HEAD").strip(),
            "main": output(tree, "rev-parse", "main").strip(),
            "artifacts": (tree / "artifacts").resolve() if (tree / "artifacts").is_symlink() else None,
            "hooks": sorted(path.name for path in (tree / ".git" / "hooks").glob("*")) if (tree / ".git" / "hooks").is_dir() else [],
        }
        if isinstance(self.result, BaseException):
            raise self.result
        return self.result

    def open_pull_request(self, number: int, files: dict[str, bytes]) -> None:
        fork = Path(self.tmp.name) / f"fork-{number}"
        git(Path(self.tmp.name), "clone", "-q", str(self.remote), str(fork))
        git(fork, "config", "user.email", "fork@example.invalid")
        git(fork, "config", "user.name", "Fork")
        for name, data in files.items():
            (fork / name).parent.mkdir(parents=True, exist_ok=True)
            (fork / name).write_bytes(data)
        git(fork, "add", "-A")
        git(fork, "commit", "-q", "-m", "contribution")
        git(fork, "push", "-q", "origin", f"HEAD:refs/pull/{number}/head")

    def check(self, number: int) -> tuple[list[str], str]:
        captured = io.StringIO()
        with contextlib.redirect_stdout(captured):
            reasons = self.merge_agent.check_pr(self.repo, number, "origin")
        return reasons, captured.getvalue()

    def assertCleanedUp(self, number: int) -> None:
        self.assertEqual(list(self.scratch.iterdir()), [])
        self.assertEqual(output(self.repo, "for-each-ref", "refs/check-pr/").strip(), "")
        self.assertEqual(output(self.repo, "worktree", "list").count("\n"), 1)

    def test_game_derived_and_binary_files_stop_before_any_pull_request_code_runs(self) -> None:
        self.open_pull_request(7, {"shot.bmp": b"BM\x00\x01", "tests/data/table.inc": b"\x01\x00\x02", "tools/run_me.py": b"print(1)\n"})
        reasons, printed = self.check(7)
        self.assertEqual(self.checked, [])
        self.assertEqual(reasons, ["game-derived, build or binary files: shot.bmp (game-derived or build path, new in the pull request), "
                                   "tests/data/table.inc (binary content, new in the pull request)"])
        self.assertIn(f"Read the diff of pull request 7 first: https://github.com/{self.merge_agent.CANONICAL_REPO}/pull/7/files", printed)
        self.assertIn("files changed against main that run code on this machine (read them first):\n  tools/run_me.py", printed)
        self.assertCleanedUp(7)

    def test_clean_pull_request_is_built_and_tested_in_a_separate_checkout(self) -> None:
        self.open_pull_request(8, {"docs/notes.md": b"notes\n", "third_party/fonts/Font.ttf": b"\x00\x01\x00\x00"})
        with mock.patch.dict(os.environ, {"SSH_AUTH_SOCK": "/agent.sock", "GH_TOKEN": "secret"}):
            reasons, _printed = self.check(8)
        self.assertEqual(reasons, [])
        self.assertEqual(len(self.checked), 1)
        tree, env = self.checked[0]
        self.assertNotIn(self.repo, tree.parents)
        self.assertEqual(self.seen["files"], ["a.txt", "docs/notes.md", "third_party/fonts/Font.ttf"])
        self.assertEqual(self.seen["branch"], "pull-8")
        self.assertEqual(self.seen["main"], output(self.repo, "rev-parse", "main").strip())
        self.assertEqual(self.seen["artifacts"], (self.repo / "artifacts").resolve())
        self.assertEqual(self.seen["hooks"], [])
        for name in ("SSH_AUTH_SOCK", "GH_TOKEN"):
            self.assertNotIn(name, env)
        self.assertEqual(env["GIT_TERMINAL_PROMPT"], "0")
        self.assertCleanedUp(8)

    def test_pull_request_code_gets_no_agent_tokens_or_git_redirects(self) -> None:
        names = {"SSH_AUTH_SOCK": "/a", "GPG_AGENT_INFO": "/b", "GIT_DIR": "/c", "GITHUB_TOKEN": "d", "MY_SECRET": "e", "DB_PASSWORD": "f"}
        with mock.patch.dict(os.environ, {**names, "HOME": "/home/x", "PATH": "/usr/bin"}, clear=True):
            env = self.merge_agent.pull_request_env()
        self.assertEqual(env, {"HOME": "/home/x", "PATH": "/usr/bin", "GIT_TERMINAL_PROMPT": "0"})

    def test_failed_checks_are_reported_and_the_checkout_removed(self) -> None:
        self.open_pull_request(9, {"docs/notes.md": b"notes\n"})
        self.result = ["ctest failed (exit 8); the failing tests are listed above"]
        reasons, _printed = self.check(9)
        self.assertEqual(reasons, self.result)
        self.assertCleanedUp(9)
        captured = io.StringIO()
        with contextlib.redirect_stdout(captured):
            self.assertEqual(self.merge_agent.report_pull_request(9, reasons), 1)
        self.assertEqual(captured.getvalue(), "check-pr 9: FAIL\n- ctest failed (exit 8); the failing tests are listed above\n")

    def test_interrupt_removes_the_checkout(self) -> None:
        self.open_pull_request(10, {"docs/notes.md": b"notes\n"})
        self.result = KeyboardInterrupt()
        with self.assertRaises(KeyboardInterrupt):
            self.check(10)
        self.assertCleanedUp(10)

    def test_own_artifacts_entry_stops_before_the_link_and_the_checks(self) -> None:
        self.open_pull_request(12, {"artifacts": b"not a folder\n"})
        with self.assertRaisesRegex(self.merge_agent.MergeError, "has its own top-level artifacts entry"):
            self.check(12)
        self.assertEqual(self.checked, [])
        self.assertCleanedUp(12)

    def test_missing_pull_request_is_named(self) -> None:
        with self.assertRaisesRegex(self.merge_agent.MergeError, "check-pr: cannot fetch pull request 11 from origin: "):
            self.check(11)
        self.assertEqual(list(self.scratch.iterdir()), [])

    def test_github_remote_is_found_by_its_url(self) -> None:
        find = self.merge_agent.github_remote
        canonical = self.merge_agent.CANONICAL_REPO
        listing = "{0}\t{1} (fetch)\n{0}\t{1} (push)\n"
        own = listing.format("origin", "ssh://git@example.invalid:222/owner/repo.git")
        self.assertEqual(find(own + listing.format("hub", f"https://github.com/{canonical}.git")), "hub")
        self.assertEqual(find(listing.format("gh", "git@github.com:someone/fork")), "gh")
        both = listing.format("fork", "git@github.com:someone/LegoCityUndercover-TheChaseBegins-PC-Port.git") + listing.format(
            "upstream", f"ssh://git@github.com/{canonical}")
        self.assertEqual(find(both), "upstream")
        with self.assertRaisesRegex(self.merge_agent.MergeError, "no git remote points to github.com; add one with: git remote add github "
                                    f"https://github.com/{canonical}.git"):
            find(own)
        with self.assertRaisesRegex(self.merge_agent.MergeError, r"several remotes point to github.com \(a, b\)"):
            find(listing.format("a", "https://github.com/x/one") + listing.format("b", "https://github.com/y/two"))

    def test_arguments_are_parsed_strictly(self) -> None:
        for value in ("0", "-3", "x", "1x", "07", "+4", "1.5"):
            result = run_script(self.repo, "check-pr", value)
            self.assertEqual(result.returncode, 2, value)
            self.assertIn(f"argument N: expected a positive pull request number, got {value!r}", result.stderr)
        result = run_script(self.repo, "check-pr")
        self.assertEqual(result.returncode, 2)
        self.assertIn("the following arguments are required: N", result.stderr)
        for extra in (["1", "2"], ["5", "--foo"]):
            result = run_script(self.repo, "check-pr", *extra)
            self.assertEqual(result.returncode, 2, extra)
            self.assertIn(f"unrecognized arguments: {extra[1]}", result.stderr)

    def test_command_line_without_a_github_remote_stops_before_fetching(self) -> None:
        result = run_script(self.repo, "check-pr", "5")
        self.assertEqual(result.returncode, 1)
        self.assertIn("check-pr: no git remote points to github.com", result.stderr)
        self.assertEqual(list(self.scratch.iterdir()), [])


if __name__ == "__main__":
    unittest.main()
