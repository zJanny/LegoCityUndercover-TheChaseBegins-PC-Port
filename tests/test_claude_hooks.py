import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

TOOLS = Path(__file__).resolve().parent.parent / "tools"
CO_AUTHOR = "Co-Authored-By: Test <test@example.invalid>"


def git(cwd, *args):
    return subprocess.run(["git", *args], cwd=cwd, check=True, capture_output=True, text=True).stdout


class HookTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.repo = Path(self.tmp.name) / "repo"
        (self.repo / "tools").mkdir(parents=True)
        shutil.copy(TOOLS / "claude_hooks.py", self.repo / "tools")
        (self.repo / "CLAUDE.md").write_text("# Rules\n")
        (self.repo / ".gitignore").write_text("/ignored/\n")
        git(self.repo, "init", "-q", "-b", "main")
        git(self.repo, "config", "user.email", "test@example.invalid")
        git(self.repo, "config", "user.name", "Test")
        git(self.repo, "add", ".")
        git(self.repo, "commit", "-q", "-m", "init")

    def tearDown(self):
        self.tmp.cleanup()

    def copy_tools(self, *names):
        for name in names:
            shutil.copy(TOOLS / name, self.repo / "tools")

    def hook(self, mode, data):
        return subprocess.run([sys.executable, str(self.repo / "tools" / "claude_hooks.py"), mode], input=json.dumps(data),
                              capture_output=True, text=True)

    def bash(self, command, **extra):
        return self.hook("pre-bash", {"cwd": str(self.repo), "tool_input": {"command": command}, **extra})

    def assertDenied(self, result, reason):
        self.assertEqual(result.returncode, 0, result.stderr)
        decision = json.loads(result.stdout)["hookSpecificOutput"]
        self.assertEqual(decision["permissionDecision"], "deny")
        self.assertIn(reason, decision["permissionDecisionReason"])

    def assertAllowed(self, result):
        self.assertEqual((result.returncode, result.stdout), (0, ""), result.stderr)

    def test_usage_and_input_are_strict(self):
        result = subprocess.run([sys.executable, str(TOOLS / "claude_hooks.py"), "pre-edit"], input="{}", capture_output=True, text=True)
        self.assertEqual(result.returncode, 1)
        self.assertIn("got: pre-edit", result.stderr)
        result = subprocess.run([sys.executable, str(TOOLS / "claude_hooks.py"), "pre-bash"], input="not json", capture_output=True, text=True)
        self.assertEqual(result.returncode, 1)
        self.assertIn("not JSON", result.stderr)

    def test_finish_runs_alone(self):
        finish = "python3 tools/merge_agent.py finish b -m /x/msg --release r-A"
        self.assertAllowed(self.bash(finish + " 2>&1 | tail -15"))
        self.assertAllowed(self.bash(f"cd {self.repo} && {finish}"))
        for chained in (" && git push origin main", "; echo done", " 2>&1 | tail -15 && graphify update .", "\ngit push", " || true"):
            self.assertDenied(self.bash(finish + chained), "finish runs alone")
        self.assertDenied(self.bash("python3 tools/merge_agent.py close -m m; git push"), "finish runs alone")

    def test_merge_message_file_needs_co_author_and_no_session(self):
        message = Path(self.tmp.name) / "msg"
        message.write_text("Port the lever\n\nClaude-Session: https://example.invalid/s\n" + CO_AUTHOR + "\n")
        self.assertDenied(self.bash(f"python3 tools/merge_agent.py finish b -m {message} 2>&1 | tail -15"), "Claude-Session")
        message.write_text("Port the lever\n")
        self.assertDenied(self.bash(f"python3 tools/merge_agent.py finish b -m {message}"), "no Co-Authored-By")
        message.write_text("Port the lever\n\n" + CO_AUTHOR + "\n")
        self.assertAllowed(self.bash(f"python3 tools/merge_agent.py finish b -m {message} 2>&1 | tail -15"))

    def test_agents_never_push(self):
        self.assertDenied(self.bash("git push origin main", agent_id="a1"), "agents never push")
        tree = self.repo / ".claude" / "worktrees" / "w"
        tree.mkdir(parents=True)
        self.assertDenied(self.hook("pre-bash", {"cwd": str(self.repo), "tool_input": {"command": f"cd {tree} && git -C {self.repo} push"}}),
                          "agents never push")
        self.assertAllowed(self.bash("git push origin main:main 2>&1 | tail -2"))
        self.assertAllowed(self.bash('grep -c "git push" notes.txt'))

    def test_push_with_a_staged_squash_is_denied(self):
        (self.repo / "a.txt").write_text("a\n")
        git(self.repo, "add", "a.txt")
        self.assertDenied(self.bash("git push origin main"), "staged changes")

    def test_other_repositories_pass(self):
        other = Path(self.tmp.name) / "other"
        other.mkdir()
        git(other, "init", "-q")
        self.assertAllowed(self.bash(f"git -C {other} config user.name X && git -C {other} push", agent_id="a1"))

    def test_git_config_writes_are_denied(self):
        for command in ("git config user.name X", "git config --global user.email a@b", "git config --unset core.x", "git -c a=b config --add x.y z"):
            self.assertDenied(self.bash(command), "never change git config")
        for command in ("git config user.name", "git config --get user.email", "git config -l", "git config --file .gitmodules --get x.y"):
            self.assertAllowed(self.bash(command))

    def test_commit_message_rules(self):
        self.assertDenied(self.bash(f'git commit -m "Add x" -m "Claude-Session: https://example.invalid/s"'), "Claude-Session")
        self.assertDenied(self.bash('git add -A && git commit -qm "Add x"'), "no Co-Authored-By")
        self.assertDenied(self.bash("python3 -m unittest && git commit -am 'Add x'"), "no Co-Authored-By")
        self.assertAllowed(self.bash(f"git commit -q -F - <<'EOF'\nAdd x\n\n{CO_AUTHOR}\nEOF"))
        self.assertAllowed(self.bash(f'git commit -m "Add x" -m "{CO_AUTHOR}"'))
        self.assertAllowed(self.bash("git commit --amend --no-edit"))
        message = self.repo / "ignored" / "msg"
        message.parent.mkdir()
        message.write_text("Add x\n")
        self.assertDenied(self.bash("git commit -F ignored/msg"), "no Co-Authored-By")
        message.write_text("Add x\n\n" + CO_AUTHOR + "\n")
        self.assertAllowed(self.bash("git commit -F ignored/msg"))

    def test_commit_fixes_and_stages_docs_and_reports_what_remains(self):
        self.copy_tools("check_doc_links.py", "user_docs.py")
        features = self.repo / "docs" / "plans" / "features"
        features.mkdir(parents=True)
        (features / "README.md").write_text("# Features\n\n| File | Status |\n|---|---|\n| [f](f.md) | open |\n")
        (features / "f.md").write_text("# F\n\n- [ ] F-1 — gap. blocks: F-2.\n- [ ] F-2 — gap.\n")
        git(self.repo, "add", ".")
        git(self.repo, "commit", "-q", "-m", "docs")
        commit = f'git commit -m "Add x" -m "{CO_AUTHOR}"'
        self.assertAllowed(self.bash(commit))
        self.assertIn("- [ ] F-2 — gap. needs: F-1.", (features / "f.md").read_text())
        self.assertEqual(git(self.repo, "diff", "--cached", "--name-only").split(), ["docs/plans/features/f.md"])
        (features / "f.md").write_text("# F\n\n- [ ] F-1 — gap. blocks: F-2.\n- [ ] F-2 — gap. blocks: F-1.\n")
        self.assertDenied(self.bash(commit), "dependency-conflict")

    def test_commit_rejects_staged_binary_files(self):
        (self.repo / "dump.raw").write_bytes(b"\0\1\2")
        git(self.repo, "add", "dump.raw")
        self.assertDenied(self.bash(f'git commit -m "Add x" -m "{CO_AUTHOR}"'), "dump.raw")

    def test_worker_needs_its_brief(self):
        prompt = "You are worker 20261026-hooks-A of round 20261026-hooks. Read your brief artifacts/rounds/20261026-hooks/20261026-hooks-A.md and follow it completely."
        data = {"cwd": str(self.repo), "tool_input": {"prompt": prompt}}
        self.assertDenied(self.hook("pre-agent", data), "does not exist")
        brief = self.repo / "artifacts" / "rounds" / "20261026-hooks" / "20261026-hooks-A.md"
        brief.parent.mkdir(parents=True)
        brief.write_text("brief\n")
        self.assertAllowed(self.hook("pre-agent", data))
        self.assertAllowed(self.hook("pre-agent", {"cwd": str(self.repo), "tool_input": {"prompt": "Find the callers of X."}}))

    def test_worktree_gets_the_artifacts_link(self):
        (self.repo / "artifacts").mkdir()
        tree = self.repo / ".claude" / "worktrees" / "agent-x"
        (tree / "src").mkdir(parents=True)
        (tree / "CLAUDE.md").write_text("# Rules\n")
        self.assertAllowed(self.hook("pre-tool", {"cwd": str(self.repo), "tool_input": {"file_path": str(tree / "src" / "a.cpp")}}))
        self.assertEqual(os.readlink(tree / "artifacts"), str(self.repo / "artifacts"))
        self.assertAllowed(self.hook("pre-tool", {"cwd": str(tree), "tool_input": {}}))

    def post_edit(self, path):
        return self.hook("post-edit", {"cwd": str(self.repo), "tool_input": {"file_path": str(path)}})

    def test_edit_reports_doc_and_local_data_findings_of_that_file(self):
        self.copy_tools("check_doc_links.py", "user_docs.py", "check_local_data.py")
        features = self.repo / "docs" / "plans" / "features"
        features.mkdir(parents=True)
        (features / "README.md").write_text("# Features\n\n| File | Status |\n|---|---|\n| [f](f.md) | open |\n| [g](g.md) | open |\n")
        (features / "f.md").write_text("# F\n\n- [ ] F-1 — gap. blocks: F-2.\n- [ ] F-2 — gap.\n")
        (features / "g.md").write_text("# G\n\n- [ ] G-1 — gap, see /home/someone/notes.txt.\n")
        result = self.post_edit(features / "g.md")
        self.assertEqual(result.returncode, 2)
        self.assertIn("docs/plans/features/g.md:3:", result.stderr)
        self.assertIn("home folder", result.stderr)
        self.assertNotIn("f.md", result.stderr.replace("g.md", ""))
        result = self.post_edit(features / "f.md")
        self.assertEqual(result.returncode, 2)
        self.assertIn("dependency-pair", result.stderr)
        (features / "g.md").write_text("# G\n\n- [ ] G-1 — gap.\n")
        self.assertEqual(self.post_edit(features / "g.md").returncode, 0)
        ignored = self.repo / "ignored" / "x.md"
        ignored.parent.mkdir()
        ignored.write_text("/home/someone/x\n")
        self.assertEqual(self.post_edit(ignored).returncode, 0)


if __name__ == "__main__":
    unittest.main()
