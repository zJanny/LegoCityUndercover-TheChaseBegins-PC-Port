import contextlib
import io
import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

TOOLS = Path(__file__).resolve().parent.parent / "tools"
sys.path.insert(0, str(TOOLS))

import hook_paths  # noqa: E402

sys.path.insert(0, str(Path(__file__).resolve().parent))
from fixtures import isolate_git_environment  # noqa: E402

isolate_git_environment()

A = "artifacts"
CO_AUTHOR = "Co-Authored-By: Test <test@example.invalid>"
WINDOWS_TREE = "C:\\work\\repo\\.claude\\worktrees\\w\\src\\a.cpp"


def git(cwd, *args):
    return subprocess.run(["git", *args], cwd=cwd, check=True, capture_output=True, text=True).stdout


def refuse(*_, **__):
    raise OSError(22, "A required privilege is not held by the client")


class PathFunctionTest(unittest.TestCase):
    def test_separators_are_normalised_on_windows_only(self):
        self.assertEqual(hook_paths.posix(WINDOWS_TREE, windows=True), "C:/work/repo/.claude/worktrees/w/src/a.cpp")
        self.assertEqual(hook_paths.posix("/home/a\\b/c", windows=False), "/home/a\\b/c")

    def test_worktree_paths_are_found_with_either_separator(self):
        for text, windows in ((WINDOWS_TREE, True), ("C:/work/repo/.claude/worktrees/w/src/a.cpp", True), ("/work/repo/.claude/worktrees/w/src", False)):
            self.assertTrue(hook_paths.in_worktree(text, windows), text)
        for text, windows in (("C:\\work\\repo\\src", True), ("/work/repo", False), ("/work/repo\\.claude\\worktrees\\w", False)):
            self.assertFalse(hook_paths.in_worktree(text, windows), text)

    def test_the_main_checkout_and_the_worktree_name_are_split_off(self):
        main, name = hook_paths.worktree_parts(WINDOWS_TREE, windows=True)
        self.assertEqual((main.as_posix(), name), ("C:/work/repo", "w"))
        main, name = hook_paths.worktree_parts("/work/repo/.claude/worktrees/agent-1", windows=False)
        self.assertEqual((main.as_posix(), name), ("/work/repo", "agent-1"))
        self.assertIsNone(hook_paths.worktree_parts("/work/repo/src", windows=False))
        self.assertEqual(hook_paths.main_of_worktree(WINDOWS_TREE, windows=True).as_posix(), "C:/work/repo")
        self.assertIsNone(hook_paths.main_of_worktree("C:\\work\\repo", windows=True))

    def test_git_bash_drive_paths_map_to_windows_paths(self):
        for text, expected in (("/c/Users/a/repo", "C:/Users/a/repo"), ("/d/x", "D:/x"), ("/cygdrive/c/Users/a", "C:/Users/a"),
                               ("/c", "C:/"), ("/c/", "C:/")):
            self.assertEqual(hook_paths.native_path(text, windows=True), expected)

    def test_other_paths_are_left_alone(self):
        for text in ("/tmp/x", "/cx/y", "/usr/bin", "C:/Users/a", "C:\\Users\\a", "src/a.cpp", "../c/x", "~/x", "", "/cygdrive", "/cygdrive/cc/x"):
            self.assertEqual(hook_paths.native_path(text, windows=True), text)
        self.assertEqual(hook_paths.native_path("/c/Users/a", windows=False), "/c/Users/a")

    def test_backslash_drive_paths_in_commands_get_slashes_on_windows(self):
        for command, expected in (("rm C:\\w\\repo/src/x", "rm C:/w/repo/src/x"),
                                  ("cd d:\\w && rm -rf 'C:\\a\\b'", "cd d:/w && rm -rf 'C:/a/b'"),
                                  ("echo a\\ b; rm \\x C:/y", "echo a\\ b; rm \\x C:/y"), ("rm xC:\\a", "rm xC:\\a")):
            self.assertEqual(hook_paths.drive_paths_with_slashes(command, windows=True), expected)
        self.assertEqual(hook_paths.drive_paths_with_slashes("rm C:\\a\\b", windows=False), "rm C:\\a\\b")

    def test_join_maps_before_it_joins(self):
        base = Path(tempfile.gettempdir())
        self.assertEqual(hook_paths.join(base, "src", windows=True), base / "src")
        if os.name == "nt":
            self.assertEqual(hook_paths.join(base, "/c/Users/a", windows=True), Path("C:/Users/a"))


class LinkTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.main = self.root / "repo"
        self.tree = self.main / ".claude" / "worktrees" / "w"
        (self.main / "artifacts").mkdir(parents=True)
        (self.tree / "src").mkdir(parents=True)
        (self.tree / "CLAUDE.md").write_text("# Rules\n")
        self.calls = []

    def tearDown(self):
        self.tmp.cleanup()

    def symlink(self, target, link, target_is_directory=False):
        self.calls.append(("symlink", Path(link), Path(target)))

    def junction(self, link, target):
        self.calls.append(("junction", Path(link), Path(target)))

    def failing_junction(self, link, target):
        self.calls.append(("junction", Path(link), Path(target)))
        raise OSError(5, "access denied")

    def ensure(self, path, windows, symlink, junction):
        stderr = io.StringIO()
        with contextlib.redirect_stderr(stderr):
            result = hook_paths.ensure_artifacts_link(path, windows, symlink, junction)
        return result, stderr.getvalue()

    def test_a_symlink_is_tried_first(self):
        result, message = self.ensure(self.tree / "src", True, self.symlink, self.junction)
        self.assertEqual((result, message), ("symlink", ""))
        self.assertEqual(self.calls, [("symlink", self.tree / "artifacts", self.main / "artifacts")])

    def test_a_junction_replaces_a_refused_symlink_on_windows(self):
        result, message = self.ensure(str(self.tree / "src"), True, refuse, self.junction)
        self.assertEqual((result, message), ("junction", ""))
        self.assertEqual(self.calls, [("junction", self.tree / "artifacts", self.main / "artifacts")])

    def test_a_link_that_cannot_be_made_is_reported_once_and_never_raises(self):
        result, message = self.ensure(self.tree, True, refuse, self.failing_junction)
        self.assertIsNone(result)
        self.assertEqual(len(message.strip().splitlines()), 1)
        self.assertIn("was not created", message)
        self.assertIn("access denied", message)

    def test_no_junction_is_tried_off_windows(self):
        result, message = self.ensure(self.tree.as_posix(), False, refuse, self.junction)
        self.assertIsNone(result)
        self.assertEqual(self.calls, [])
        self.assertIn("was not created", message)

    def test_existing_links_and_foreign_paths_are_left_alone(self):
        (self.tree / "artifacts").mkdir()
        self.assertEqual(self.ensure(self.tree, True, self.symlink, self.junction), (None, ""))
        self.assertEqual(self.ensure(self.main / "src", True, self.symlink, self.junction), (None, ""))
        self.assertEqual(self.calls, [])

    def test_the_junction_it_makes_is_a_working_link(self):
        if os.name != "nt":
            self.skipTest("directory junctions exist on Windows only")
        (self.main / "artifacts" / "f.txt").write_text("x\n")
        self.assertEqual(hook_paths.create_link(self.tree / "artifacts", self.main / "artifacts", True, refuse), "junction")
        self.assertEqual((self.tree / "artifacts" / "f.txt").read_text(), "x\n")
        self.assertEqual((self.tree / "artifacts").resolve(), (self.main / "artifacts").resolve())


class HookRunTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.repo = Path(self.tmp.name) / "repo"
        (self.repo / "tools").mkdir(parents=True)
        for name in ("claude_hooks.py", "hook_paths.py", "forbidden_files.py"):
            shutil.copy(TOOLS / name, self.repo / "tools")
        (self.repo / "CLAUDE.md").write_text("# Rules\n")
        git(self.repo, "init", "-q", "-b", "main")
        git(self.repo, "config", "user.email", "test@example.invalid")
        git(self.repo, "config", "user.name", "Test")
        git(self.repo, "add", ".")
        git(self.repo, "commit", "-q", "-m", "init")
        self.tree = self.repo / ".claude" / "worktrees" / "w"
        (self.tree / "src").mkdir(parents=True)
        (self.tree / "CLAUDE.md").write_text("# Rules\n")
        (self.repo / "artifacts" / "oracle" / "states").mkdir(parents=True)
        (self.repo / "artifacts" / "oracle" / "states" / "a.cst").write_text("state\n")

    def tearDown(self):
        self.tmp.cleanup()

    def hook(self, mode, data):
        return subprocess.run([sys.executable, str(self.repo / "tools" / "claude_hooks.py"), mode], input=json.dumps(data),
                              capture_output=True, text=True, encoding="utf-8")

    def bash(self, command, cwd=None, **extra):
        return self.hook("pre-bash", {"cwd": str(cwd or self.repo), "tool_input": {"command": command}, **extra})

    def reason(self, result):
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertNotEqual(result.stdout, "", "the hook allowed the call")
        decision = json.loads(result.stdout)["hookSpecificOutput"]
        self.assertEqual(decision["permissionDecision"], "deny")
        return decision["permissionDecisionReason"]

    def test_a_worktree_is_recognised_by_its_path_without_an_agent_id(self):
        self.assertIn("agents never push", self.reason(self.bash("git push origin main", cwd=self.tree)))

    def test_the_artifacts_link_is_made_without_a_symlink_privilege_and_protects_the_main_artifacts(self):
        result = self.bash(f"rm {A}/oracle/states/a.cst", cwd=self.tree)
        self.assertIn("main checkout's artifacts/", self.reason(result))
        self.assertTrue((self.tree / "artifacts" / "oracle" / "states" / "a.cst").is_file())

    @unittest.skipUnless(os.name == "nt", "ln -s copies the folder on Windows without the symlink privilege")
    def test_a_windows_worker_never_gets_a_copy_from_ln(self):
        result = self.bash(f"ln -s {self.git_bash_style(self.repo)}/{A} {A}", cwd=self.tree)
        self.assertIn("already exists here", self.reason(result))
        self.assertTrue((self.tree / A / "oracle" / "states" / "a.cst").is_file())
        self.assertEqual((self.tree / A).resolve(), (self.repo / A).resolve())

    def test_the_stop_check_finds_the_report_of_a_worker_in_a_worktree(self):
        rounds = self.repo / "artifacts" / "rounds" / "r"
        rounds.mkdir(parents=True)
        (rounds / "r-A.md").write_text(f"Write your report to `{A}/rounds/r/r-A.report.md`.\n")
        prompt = f"You are worker r-A of round r. Read your brief {rounds / 'r-A.md'} and follow it completely."
        transcript = Path(self.tmp.name) / "agent.jsonl"
        transcript.write_text(json.dumps({"type": "user", "message": {"role": "user", "content": prompt}}) + "\n", encoding="utf-8")
        data = {"cwd": str(self.tree), "hook_event_name": "SubagentStop", "agent_id": "a1", "agent_transcript_path": str(transcript)}
        result = self.hook("subagent-stop", data)
        self.assertEqual(json.loads(result.stdout)["decision"], "block")
        self.assertIn("r-A.report.md", json.loads(result.stdout)["reason"])
        (rounds / "r-A.report.md").write_text("r-A: DONE\n")
        self.assertEqual(self.hook("subagent-stop", data).stdout, "")

    def git_bash_style(self, path, prefix=""):
        text = str(path).replace("\\", "/")
        return f"{prefix}/{text[0].lower()}{text[2:]}"

    @unittest.skipUnless(os.name == "nt", "Git Bash drive paths exist on Windows only")
    def test_a_commit_after_a_git_bash_style_cd_is_checked(self):
        (self.repo / "a.txt").write_text("a\n")
        git(self.repo, "add", "a.txt")
        for prefix in ("", "/cygdrive"):
            target = self.git_bash_style(self.repo, prefix)
            self.assertIn("no Co-Authored-By", self.reason(self.bash(f"cd {target} && git commit -m x", cwd=self.tree)))
            self.assertIn("no Co-Authored-By", self.reason(self.bash(f"cd '{target}' && git commit -m x")))
        self.assertEqual(self.bash(f'cd {self.git_bash_style(self.repo)} && git commit -m "x\n\n{CO_AUTHOR}"').returncode, 0)

    @unittest.skipUnless(os.name == "nt", "Git Bash drive paths exist on Windows only")
    def test_a_git_bash_style_cd_into_a_worktree_marks_the_agent(self):
        target = self.git_bash_style(self.tree)
        self.assertIn("agents never push", self.reason(self.bash(f"cd {target} && git push origin main")))


if __name__ == "__main__":
    unittest.main()
