import importlib.util
import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

TEXT = {"encoding": "utf-8", "errors": "replace"}
SH = shutil.which("sh")

sys.path.insert(0, str(Path(__file__).resolve().parent))
from fixtures import isolate_git_environment  # noqa: E402

isolate_git_environment()

TOOLS = Path(__file__).resolve().parent.parent / "tools"
A = "artifacts"
CO_AUTHOR = "Co-Authored-By: Test <test@example.invalid>"


def link_directory(target, link):
    try:
        os.symlink(target, link, target_is_directory=True)
    except OSError:
        if sys.platform != "win32":
            raise
        subprocess.run(["cmd", "/c", "mklink", "/J", str(link), str(target)], check=True, capture_output=True)


def git(cwd, *args):
    return subprocess.run(["git", *args], cwd=cwd, check=True, capture_output=True, **TEXT).stdout


class HookTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.repo = Path(self.tmp.name) / "repo"
        (self.repo / "tools").mkdir(parents=True)
        shutil.copy(TOOLS / "claude_hooks.py", self.repo / "tools")
        shutil.copy(TOOLS / "forbidden_files.py", self.repo / "tools")
        shutil.copy(TOOLS / "hook_paths.py", self.repo / "tools")
        (self.repo / "CLAUDE.md").write_text("# Rules\n", encoding="utf-8", newline="\n")
        (self.repo / ".gitignore").write_text("/ignored/\n", encoding="utf-8", newline="\n")
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
                              capture_output=True, **TEXT)

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
        result = subprocess.run([sys.executable, str(TOOLS / "claude_hooks.py"), "pre-edit"], input="{}", capture_output=True, **TEXT)
        self.assertEqual(result.returncode, 1)
        self.assertIn("got: pre-edit", result.stderr)
        result = subprocess.run([sys.executable, str(TOOLS / "claude_hooks.py"), "pre-bash"], input="not json", capture_output=True, **TEXT)
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
        message.write_text("Port the lever\n\nClaude-Session: https://example.invalid/s\n" + CO_AUTHOR + "\n", encoding="utf-8", newline="\n")
        self.assertDenied(self.bash(f"python3 tools/merge_agent.py finish b -m {message} 2>&1 | tail -15"), "Claude-Session")
        message.write_text("Port the lever\n", encoding="utf-8", newline="\n")
        self.assertDenied(self.bash(f"python3 tools/merge_agent.py finish b -m {message}"), "no Co-Authored-By")
        message.write_text("Port the lever\n\n" + CO_AUTHOR + "\n", encoding="utf-8", newline="\n")
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
        (self.repo / "a.txt").write_text("a\n", encoding="utf-8", newline="\n")
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
        message.write_text("Add x\n", encoding="utf-8", newline="\n")
        self.assertDenied(self.bash("git commit -F ignored/msg"), "no Co-Authored-By")
        message.write_text("Add x\n\n" + CO_AUTHOR + "\n", encoding="utf-8", newline="\n")
        self.assertAllowed(self.bash("git commit -F ignored/msg"))

    def test_commit_fixes_and_stages_docs_and_reports_what_remains(self):
        self.copy_tools("check_doc_links.py", "user_docs.py")
        features = self.repo / "docs" / "plans" / "features"
        features.mkdir(parents=True)
        (features / "README.md").write_text("# Features\n\n| File | Status |\n|---|---|\n| [f](f.md) | open |\n", encoding="utf-8", newline="\n")
        (features / "f.md").write_text("# F\n\n- [ ] F-1 — gap. blocks: F-2.\n- [ ] F-2 — gap.\n", encoding="utf-8", newline="\n")
        git(self.repo, "add", ".")
        git(self.repo, "commit", "-q", "-m", "docs")
        commit = f'git commit -m "Add x" -m "{CO_AUTHOR}"'
        self.assertAllowed(self.bash(commit))
        self.assertIn("- [ ] F-2 — gap. needs: F-1.", (features / "f.md").read_text(encoding="utf-8"))
        self.assertEqual(git(self.repo, "diff", "--cached", "--name-only").split(), ["docs/plans/features/f.md"])
        (features / "f.md").write_text("# F\n\n- [ ] F-1 — gap. blocks: F-2.\n- [ ] F-2 — gap. blocks: F-1.\n", encoding="utf-8", newline="\n")
        self.assertDenied(self.bash(commit), "dependency-conflict")

    def docs_repo(self):
        self.copy_tools("check_doc_links.py", "user_docs.py")
        features = self.repo / "docs" / "plans" / "features"
        features.mkdir(parents=True)
        (features / "README.md").write_text("# Features\n\n| File | Status |\n|---|---|\n| [f](f.md) | open |\n", encoding="utf-8", newline="\n")
        (features / "f.md").write_text("# F\n\n- [ ] F-1 — gap.\n", encoding="utf-8", newline="\n")
        (self.repo / "docs" / "notes.md").write_text("notes\n", encoding="utf-8", newline="\n")
        git(self.repo, "add", ".")
        git(self.repo, "commit", "-q", "-m", "docs")
        return features

    def staged(self):
        return git(self.repo, "diff", "--cached", "--name-only").split()

    def unstaged(self):
        return git(self.repo, "diff", "--name-only").split()

    def test_commit_stages_a_file_its_own_fix_changed_even_when_the_edit_was_unstaged(self):
        features = self.docs_repo()
        (features / "f.md").write_text("# F\n\n- [ ] F-1 — gap.\n- [ ] F-3 — gap.\n- [x] F-2 — done.\n", encoding="utf-8", newline="\n")
        (self.repo / "docs" / "notes.md").write_text("notes, edited on purpose\n", encoding="utf-8", newline="\n")
        self.assertAllowed(self.bash(f'git commit -m "Add x" -m "{CO_AUTHOR}"'))
        self.assertEqual(self.staged(), ["docs/plans/features/f.closed.md", "docs/plans/features/f.md"])
        self.assertEqual(self.unstaged(), ["docs/notes.md"])
        self.assertNotIn("F-2", (features / "f.md").read_text(encoding="utf-8"))

    def test_commit_with_all_flag_takes_the_fixed_files(self):
        features = self.docs_repo()
        (features / "f.md").write_text("# F\n\n- [ ] F-1 — gap.\n- [ ] F-3 — gap.\n- [x] F-2 — done.\n", encoding="utf-8", newline="\n")
        self.assertAllowed(self.bash(f'git commit -am "Add x" -m "{CO_AUTHOR}"'))
        self.assertEqual(self.staged(), ["docs/plans/features/f.closed.md", "docs/plans/features/f.md"])

    def test_commit_with_all_flag_rejects_a_modified_binary_file(self):
        (self.repo / "dump.raw").write_bytes(b"\0\1\2")
        git(self.repo, "add", "dump.raw")
        git(self.repo, "commit", "-q", "-m", "raw")
        (self.repo / "dump.raw").write_bytes(b"\0\1\3")
        self.assertDenied(self.bash(f'git commit -am "Add x" -m "{CO_AUTHOR}"'), "dump.raw")

    def test_commit_with_paths_must_cover_what_the_hook_staged(self):
        features = self.docs_repo()
        (features / "f.md").write_text("# F\n\n- [ ] F-1 — gap.\n- [ ] F-3 — gap.\n- [x] F-2 — done.\n", encoding="utf-8", newline="\n")
        commit = f'git commit docs/plans/features/f.md -m "Add x" -m "{CO_AUTHOR}"'
        self.assertDenied(self.bash(commit), "git commit with paths ignores the index")
        self.assertDenied(self.bash(f'git commit -q -m "Add x" -m "{CO_AUTHOR}" -- docs/plans/features/f.md 2>&1 | tail -3'), "ignores the index")
        self.assertDenied(self.bash(f'git commit -q -m "Add x\n\n{CO_AUTHOR}" docs/plans/features/f.md'), "ignores the index")
        self.assertDenied(self.bash(f'git commit -qam "Add x\n\n{CO_AUTHOR}" docs/plans/features/f.md'), "ignores the index")
        both = f'git commit docs/plans/features/f.md docs/plans/features/f.closed.md -m "Add x" -m "{CO_AUTHOR}"'
        self.assertAllowed(self.bash(both))

    def test_commit_with_paths_is_allowed_when_nothing_else_is_staged(self):
        self.docs_repo()
        (self.repo / "docs" / "notes.md").write_text("changed\n", encoding="utf-8", newline="\n")
        self.assertAllowed(self.bash(f'git commit docs/notes.md -m "Add x" -m "{CO_AUTHOR}"'))
        self.assertEqual(self.staged(), [])

    README = "# T\n\nSteps.\n<!-- progress:start -->\n| 5 tests |\n<!-- progress:end -->\n\nLinks.\n"

    def test_branch_commit_denies_staged_generated_files(self):
        for name in ("README.md", "docs/plans/priorities.md", "docs/progress-trend.svg"):
            path = self.repo / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(self.README if name == "README.md" else "old\n", encoding="utf-8", newline="\n")
        git(self.repo, "add", ".")
        git(self.repo, "commit", "-q", "-m", "generated")
        commit = f'git commit -m "Add x" -m "{CO_AUTHOR}"'
        changed = {"README.md": self.README.replace("5 tests", "6 tests"), "docs/plans/priorities.md": "new\n", "docs/progress-trend.svg": "new\n"}
        for name, text in changed.items():
            (self.repo / name).write_text(text, encoding="utf-8", newline="\n")
            git(self.repo, "add", name)
            self.assertAllowed(self.bash(commit))
            git(self.repo, "reset", "-q", "--hard")
        git(self.repo, "checkout", "-q", "-b", "feature")
        for name, text in changed.items():
            (self.repo / name).write_text(text, encoding="utf-8", newline="\n")
            git(self.repo, "add", name)
            self.assertDenied(self.bash(commit), name)
            self.assertDenied(self.bash(f'git commit -am "Add x" -m "{CO_AUTHOR}"'), name)
            git(self.repo, "reset", "-q", "--hard")
        (self.repo / "README.md").write_text("# T\n\nSteps.\n", encoding="utf-8", newline="\n")
        git(self.repo, "add", "README.md")
        self.assertDenied(self.bash(commit), "README.md (its progress section)")
        git(self.repo, "reset", "-q", "--hard")
        (self.repo / "README.md").write_text(self.README.replace("5 tests", "7 tests"), encoding="utf-8", newline="\n")
        self.assertDenied(self.bash(f'git commit -am "Add x" -m "{CO_AUTHOR}"'), "README.md (its progress section)")
        git(self.repo, "reset", "-q", "--hard")
        (self.repo / "docs").mkdir(exist_ok=True)
        (self.repo / "docs" / "notes.md").write_text("x\n", encoding="utf-8", newline="\n")
        git(self.repo, "add", "docs/notes.md")
        self.assertAllowed(self.bash(commit))

    def test_branch_commit_allows_readme_edits_outside_the_progress_section(self):
        (self.repo / "README.md").write_text(self.README, encoding="utf-8", newline="\n")
        git(self.repo, "add", ".")
        git(self.repo, "commit", "-q", "-m", "readme")
        git(self.repo, "checkout", "-q", "-b", "feature")
        (self.repo / "README.md").write_text(self.README.replace("Steps.", "Steps, now with a fix.").replace("Links.", "More links."), encoding="utf-8", newline="\n")
        git(self.repo, "add", "README.md")
        self.assertAllowed(self.bash(f'git commit -m "Fix the quick start" -m "{CO_AUTHOR}"'))
        (self.repo / "README.md").write_text(self.README.replace("Links.", "Other links."), encoding="utf-8", newline="\n")
        self.assertAllowed(self.bash(f'git commit -am "Fix the links" -m "{CO_AUTHOR}"'))

    def test_commit_of_claude_md_needs_the_development_doc(self):
        (self.repo / "docs").mkdir()
        (self.repo / "docs" / "development.md").write_text("# Development\n", encoding="utf-8", newline="\n")
        git(self.repo, "add", ".")
        git(self.repo, "commit", "-q", "-m", "docs")
        commit = f'git commit -m "Change a rule" -m "{CO_AUTHOR}"'
        for branch in ("main", "feature"):
            git(self.repo, "checkout", "-q", "-B", branch)
            (self.repo / "CLAUDE.md").write_text(f"# Rules\n\nA rule for {branch}.\n", encoding="utf-8", newline="\n")
            git(self.repo, "add", "CLAUDE.md")
            self.assertDenied(self.bash(commit), "CLAUDE.md is staged without docs/development.md")
            self.assertDenied(self.bash(f'git commit -am "Change a rule" -m "{CO_AUTHOR}"'), "Edit docs/development.md to match and stage it")
            git(self.repo, "reset", "-q", "--hard")
            (self.repo / "CLAUDE.md").write_text(f"# Rules\n\nA rule for {branch}.\n", encoding="utf-8", newline="\n")
            (self.repo / "docs" / "development.md").write_text(f"# Development\n\nThe rule for {branch}.\n", encoding="utf-8", newline="\n")
            self.assertAllowed(self.bash(f'git commit -am "Change a rule" -m "{CO_AUTHOR}"'))
            git(self.repo, "reset", "-q", "--hard")
        (self.repo / "docs" / "development.md").write_text("# Development\n\nOnly the human file.\n", encoding="utf-8", newline="\n")
        git(self.repo, "add", "docs/development.md")
        self.assertAllowed(self.bash(commit))

    def test_branch_commit_of_a_moved_feature_line_leaves_priorities_alone(self):
        features = self.docs_repo()
        self.copy_tools("priority_report.py", "progress_report.py")
        priorities = self.repo / "docs" / "plans" / "priorities.md"
        priorities.write_text("# P\n\n[F-1](features/f.md?display=source#L3)\n", encoding="utf-8", newline="\n")
        git(self.repo, "add", ".")
        git(self.repo, "commit", "-q", "-m", "priorities")
        git(self.repo, "checkout", "-q", "-b", "feature")
        (features / "f.md").write_text("# F\n\nProse.\n\n- [ ] F-1 — gap.\n", encoding="utf-8", newline="\n")
        self.assertAllowed(self.bash(f'git commit -am "Move x" -m "{CO_AUTHOR}"'))
        self.assertEqual(priorities.read_text(encoding="utf-8"), "# P\n\n[F-1](features/f.md?display=source#L3)\n")
        self.assertNotIn("docs/plans/priorities.md", self.staged())

    GENERATED = ("README.md", "docs/plans/priorities.md", "docs/progress-trend.svg")

    def start_merge_of_main(self):
        for name in self.GENERATED:
            path = self.repo / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("old\n", encoding="utf-8", newline="\n")
        git(self.repo, "add", ".")
        git(self.repo, "commit", "-q", "-m", "generated")
        git(self.repo, "checkout", "-q", "-b", "feature")
        (self.repo / "docs" / "notes.md").write_text("x\n", encoding="utf-8", newline="\n")
        git(self.repo, "add", "docs/notes.md")
        git(self.repo, "commit", "-q", "-m", "notes")
        git(self.repo, "checkout", "-q", "main")
        for name in self.GENERATED:
            (self.repo / name).write_text("main\n", encoding="utf-8", newline="\n")
        git(self.repo, "commit", "-qam", "main moved")
        git(self.repo, "checkout", "-q", "feature")
        git(self.repo, "merge", "-q", "--no-commit", "--no-ff", "main")

    def test_merge_commit_accepts_generated_files_of_main(self):
        self.start_merge_of_main()
        self.assertAllowed(self.bash(f'git commit -m "Merge main" -m "{CO_AUTHOR}"'))

    def test_merge_commit_denies_edited_generated_file(self):
        self.start_merge_of_main()
        (self.repo / "docs" / "progress-trend.svg").write_text("hand made\n", encoding="utf-8", newline="\n")
        git(self.repo, "add", "docs/progress-trend.svg")
        self.assertDenied(self.bash(f'git commit -m "Merge main" -m "{CO_AUTHOR}"'), "docs/progress-trend.svg")

    def test_detached_head_counts_as_main(self):
        (self.repo / "README.md").write_text("new\n", encoding="utf-8", newline="\n")
        git(self.repo, "add", "README.md")
        git(self.repo, "checkout", "-q", "--detach")
        self.assertAllowed(self.bash(f'git commit -m "Add x" -m "{CO_AUTHOR}"'))

    def test_commit_rejects_staged_binary_files(self):
        (self.repo / "dump.raw").write_bytes(b"\0\1\2")
        git(self.repo, "add", "dump.raw")
        self.assertDenied(self.bash(f'git commit -m "Add x" -m "{CO_AUTHOR}"'), "dump.raw")

    def test_checkout_without_the_rules_module_uses_the_hooks_own(self):
        older = Path(self.tmp.name) / "older"
        git(Path(self.tmp.name), "clone", "-q", str(self.repo), str(older))
        git(older, "rm", "-q", "tools/forbidden_files.py")
        (older / "dump.raw").write_bytes(b"\0\1\2")
        git(older, "add", "dump.raw")
        result = self.hook("pre-bash", {"cwd": str(older), "tool_input": {"command": f'git commit -m "Add x" -m "{CO_AUTHOR}"'}})
        self.assertDenied(result, "dump.raw (binary content)")

    def test_commit_allows_only_bundled_fonts_as_binary_files(self):
        self.copy_tools("merge_agent.py", "merge_conflicts.py", "round_log.py")
        font = self.repo / "third_party" / "fonts" / "Font.ttf"
        font.parent.mkdir(parents=True)
        font.write_bytes(b"\0\1\0\0\0")
        git(self.repo, "add", "-A")
        self.assertAllowed(self.bash(f'git commit -m "Add x" -m "{CO_AUTHOR}"'))
        (self.repo / "fonts").mkdir()
        (self.repo / "fonts" / "Font.ttf").write_bytes(b"\0\1\0\0\0")
        git(self.repo, "add", "-A")
        self.assertDenied(self.bash(f'git commit -m "Add x" -m "{CO_AUTHOR}"'), "fonts/Font.ttf")

    def test_worker_needs_its_brief(self):
        prompt = "You are worker 20261026-hooks-A of round 20261026-hooks. Read your brief artifacts/rounds/20261026-hooks/20261026-hooks-A.md and follow it completely."
        data = {"cwd": str(self.repo), "tool_input": {"prompt": prompt}}
        self.assertDenied(self.hook("pre-agent", data), "does not exist")
        brief = self.repo / "artifacts" / "rounds" / "20261026-hooks" / "20261026-hooks-A.md"
        brief.parent.mkdir(parents=True)
        brief.write_text("brief\n", encoding="utf-8", newline="\n")
        self.assertAllowed(self.hook("pre-agent", data))
        self.assertAllowed(self.hook("pre-agent", {"cwd": str(self.repo), "tool_input": {"prompt": "Find the callers of X."}}))

    def test_worktree_gets_the_artifacts_link(self):
        (self.repo / "artifacts").mkdir()
        tree = self.repo / ".claude" / "worktrees" / "agent-x"
        (tree / "src").mkdir(parents=True)
        (tree / "CLAUDE.md").write_text("# Rules\n", encoding="utf-8", newline="\n")
        self.assertAllowed(self.hook("pre-tool", {"cwd": str(self.repo), "tool_input": {"file_path": str(tree / "src" / "a.cpp")}}))
        self.assertEqual((tree / "artifacts").resolve(), (self.repo / "artifacts").resolve())
        self.assertAllowed(self.hook("pre-tool", {"cwd": str(tree), "tool_input": {}}))

    def worktree(self):
        (self.repo / "artifacts" / "oracle" / "states").mkdir(parents=True)
        (self.repo / "artifacts" / "oracle" / "states" / "a.cst").write_text("state\n", encoding="utf-8", newline="\n")
        tree = self.repo / ".claude" / "worktrees" / "w"
        (tree / "src").mkdir(parents=True)
        (tree / "CLAUDE.md").write_text("# Rules\n", encoding="utf-8", newline="\n")
        return tree

    def agent_bash(self, command, tree=None, **extra):
        return self.hook("pre-bash", {"cwd": str(tree or self.repo), "tool_input": {"command": command}, "agent_id": "a1", **extra})

    def test_artifacts_link_is_made_once(self):
        tree = self.worktree()
        make = f"ln -s {self.repo}/artifacts artifacts"
        if sys.platform == "win32":
            self.assertDenied(self.agent_bash(make, tree), "skip this step")
            self.assertEqual((tree / "artifacts").resolve(), (self.repo / "artifacts").resolve())
        else:
            self.assertAllowed(self.agent_bash(make, tree))
            self.assertFalse(os.path.lexists(tree / "artifacts"))
            link_directory(self.repo / "artifacts", tree / "artifacts")
        for again in (make, f"cd {tree} && {make}", f"ln -sf {self.repo}/artifacts {tree}/artifacts", f"echo a; ln -s ../x artifacts"):
            self.assertDenied(self.agent_bash(again, tree), "already exists here")
        for allowed in (f"[ -e artifacts ] || {make}", f"test -e artifacts || {make}", f"ln -sfn {self.repo}/artifacts artifacts",
                        f"ln -s {self.repo}/artifacts other", f"ln -s {self.repo}/artifacts", "grep ln artifacts"):
            self.assertAllowed(self.agent_bash(allowed, tree))
        self.assertFalse(os.path.lexists(self.repo / "artifacts" / "artifacts"))

    def test_agents_never_delete_from_the_main_artifacts(self):
        tree = self.worktree()
        link_directory(self.repo / "artifacts", tree / "artifacts")
        denied = [f"rm {A}/oracle/states/a.cst", f"rm -rf {A}/artifacts", f"unlink {A}/artifacts", f"rmdir {A}/oracle",
                  f"mv {A}/oracle/states/a.cst /tmp/x", f"mv -t /tmp {A}/oracle/states/a.cst", f"find {A}/ -name '*.cst' -delete",
                  f"find {A}/oracle -exec rm {{}} +", "cd artifacts && rm -f x", f"rm {self.repo}/{A}/oracle/states/a.cst",
                  f"rm -rf {self.repo}/{A}", f"ls && rm -r {A}/oracle/states", f"bash -c 'rm {A}/rounds/x'",
                  f"rm -r {A}/oracle/outsider", f"rm {A}/*", f"sudo rm {A}/x"]
        for command in denied:
            self.assertDenied(self.agent_bash(command, tree), f"main checkout's {A}/")
        self.assertDenied(self.agent_bash(f"rm -r {self.repo}/{A}/oracle/states"), "scratch")
        allowed = [f"rm -rf {A}/oracle/20261027-x-A-out*", f"rm -r {A}/oracle/a-out/shots", f"rm -rf {A}/scratch", f"rm {A}/scratch/a.cpp",
                   "rm artifacts", "unlink artifacts", "rm src/a.cpp", "rm -rf build", "mv src/a.cpp src/b.cpp", f"mv /tmp/x {A}/rounds/y",
                   "find src -name '*.o' -delete", f"find {A}/oracle -name '*.png'", "rm -f /tmp/x 2>&1 | tail -2", f"echo rm {A}/x"]
        for command in allowed:
            self.assertAllowed(self.agent_bash(command, tree))
        (self.repo / "tools" / "x").write_text("x\n", encoding="utf-8", newline="\n")
        self.assertAllowed(self.agent_bash("rm tools/x"))
        self.assertAllowed(self.bash(f"rm -rf {A}/oracle/states"))

    def test_scratch_code_is_not_written_into_artifacts(self):
        tree = self.worktree()
        link_directory(self.repo / "artifacts", tree / "artifacts")
        for tool in ("Write", "Edit"):
            for path in (tree / "artifacts" / "probe.cpp", self.repo / "artifacts" / "scratch" / "p.hpp", tree / "artifacts" / "x" / "p.cc",
                         tree / "artifacts" / "p.CXX", tree / "artifacts" / "p.h"):
                self.assertDenied(self.hook("pre-tool", {"cwd": str(tree), "tool_name": tool, "tool_input": {"file_path": str(path)}}),
                                  "scratchpad")
        for tool, path in (("Write", tree / "src" / "a.cpp"), ("Edit", tree / "tests" / "a_test.cpp"), ("Write", tree / "tools" / "p.hpp"),
                           ("Write", tree / "artifacts" / "rounds" / "r.report.md"), ("Write", Path(self.tmp.name) / "scratch" / "p.cpp"),
                           ("Read", tree / "artifacts" / "probe.cpp"), ("Grep", tree / "artifacts" / "probe.cpp")):
            self.assertAllowed(self.hook("pre-tool", {"cwd": str(tree), "tool_name": tool, "tool_input": {"file_path": str(path)}}))

    def test_scratch_code_is_not_written_into_artifacts_from_bash(self):
        tree = self.worktree()
        link_directory(self.repo / "artifacts", tree / "artifacts")
        for command in (f"cat > {A}/probe.cpp <<'EOF'\nint main() {{}}\nEOF", f"echo x >> {A}/scratch/p.hpp", f"printf x | tee {A}/p.cc",
                        f"cp src/a.cpp {A}/b.cpp", f"cd {A} && echo x > p.h", f"g++ a.cpp 2>&1 > {self.repo}/{A}/out.cxx"):
            self.assertDenied(self.agent_bash(command, tree), "scratchpad")
        for command in (f"cat > {A}/notes.md <<'EOF'\nx\nEOF", f"echo x > src/a.cpp", f"g++ -o {A}/probe /tmp/p.cpp 2>&1 | tail -3",
                        f"cat {A}/probe.cpp", f"cp {A}/probe.cpp /tmp/p.cpp", "echo x > /tmp/p.cpp"):
            self.assertAllowed(self.agent_bash(command, tree))

    def stop_fixture(self, prompt_for_agent="r-A", content_blocks=False):
        tree = self.worktree()
        rounds = self.repo / "artifacts" / "rounds" / "r"
        rounds.mkdir(parents=True)
        (rounds / "r-A.md").write_text(f"Brief\n\n3. Write your report to `{A}/rounds/r/r-A.report.md` and give the same text.\n", encoding="utf-8", newline="\n")
        prompt = f"You are worker {prompt_for_agent} of round r. Read your brief {rounds / 'r-A.md'} and follow it completely."
        content = [{"type": "text", "text": prompt}] if content_blocks else prompt
        transcript = Path(self.tmp.name) / "agent.jsonl"
        transcript.write_text(json.dumps({"type": "user", "message": {"role": "user", "content": content}}) + "\n"
                              + json.dumps({"type": "assistant", "message": {"role": "assistant", "content": "ok"}}) + "\n",
                              encoding="utf-8", newline="\n")
        return tree, transcript, rounds / "r-A.report.md"

    def stop(self, tree, transcript, **extra):
        data = {"cwd": str(tree), "hook_event_name": "SubagentStop", "agent_id": "a1", "stop_hook_active": False, **extra}
        if transcript:
            data["agent_transcript_path"] = str(transcript)
        return self.hook("subagent-stop", data)

    def test_worker_stops_only_after_its_report_exists(self):
        for blocks in (False, True):
            tree, transcript, report = self.stop_fixture(content_blocks=blocks)
            result = self.stop(tree, transcript)
            self.assertEqual(result.returncode, 0, result.stderr)
            decision = json.loads(result.stdout)
            self.assertEqual(decision["decision"], "block")
            self.assertIn("r-A.report.md", decision["reason"])
            report.write_text("", encoding="utf-8", newline="\n")
            self.assertEqual(json.loads(self.stop(tree, transcript).stdout)["decision"], "block")
            report.write_text("r-A: DONE\n", encoding="utf-8", newline="\n")
            self.assertAllowed(self.stop(tree, transcript))
            report.unlink()
            shutil.rmtree(self.repo / "artifacts")
            shutil.rmtree(self.repo / ".claude")

    def test_a_second_stop_is_let_through(self):
        tree, transcript, _ = self.stop_fixture()
        result = self.stop(tree, transcript, stop_hook_active=True)
        self.assertEqual((result.returncode, result.stdout), (0, ""))
        self.assertIn("blocked once already", result.stderr)

    def test_unidentifiable_agents_are_never_blocked(self):
        tree, transcript, _ = self.stop_fixture()
        self.assertAllowed(self.stop(self.repo, transcript))
        self.assertAllowed(self.stop(tree, None))
        self.assertAllowed(self.stop(tree, Path(self.tmp.name) / "missing.jsonl"))
        planner = Path(self.tmp.name) / "planner.jsonl"
        planner.write_text(json.dumps({"type": "user", "message": {"role": "user", "content": "Find the callers of X."}}) + "\n", encoding="utf-8", newline="\n")
        self.assertAllowed(self.stop(tree, planner))
        broken = Path(self.tmp.name) / "broken.jsonl"
        broken.write_text("not json\n", encoding="utf-8", newline="\n")
        self.assertAllowed(self.stop(tree, broken))
        (self.repo / "artifacts" / "rounds" / "r" / "r-A.md").write_text("Brief without a report line\n", encoding="utf-8", newline="\n")
        self.assertAllowed(self.stop(tree, transcript))
        (self.repo / "artifacts" / "rounds" / "r" / "r-A.md").unlink()
        self.assertAllowed(self.stop(tree, transcript))
        self.assertAllowed(self.hook("subagent-stop", {"cwd": 5, "agent_transcript_path": 7}))

    def post_edit(self, path):
        return self.hook("post-edit", {"cwd": str(self.repo), "tool_input": {"file_path": str(path)}})

    def test_edit_reports_doc_and_local_data_findings_of_that_file(self):
        self.copy_tools("check_doc_links.py", "user_docs.py", "check_local_data.py")
        features = self.repo / "docs" / "plans" / "features"
        features.mkdir(parents=True)
        (features / "README.md").write_text("# Features\n\n| File | Status |\n|---|---|\n| [f](f.md) | open |\n| [g](g.md) | open |\n", encoding="utf-8", newline="\n")
        (features / "f.md").write_text("# F\n\n- [ ] F-1 — gap. blocks: F-2.\n- [ ] F-2 — gap.\n", encoding="utf-8", newline="\n")
        (features / "g.md").write_text("# G\n\n- [ ] G-1 — gap, see /home/someone/notes.txt.\n", encoding="utf-8", newline="\n")
        result = self.post_edit(features / "g.md")
        self.assertEqual(result.returncode, 2)
        self.assertIn("docs/plans/features/g.md:3:", result.stderr)
        self.assertIn("home folder", result.stderr)
        self.assertNotIn("f.md", result.stderr.replace("g.md", ""))
        result = self.post_edit(features / "f.md")
        self.assertEqual(result.returncode, 2)
        self.assertIn("dependency-pair", result.stderr)
        (features / "g.md").write_text("# G\n\n- [ ] G-1 — gap.\n", encoding="utf-8", newline="\n")
        self.assertEqual(self.post_edit(features / "g.md").returncode, 0)
        ignored = self.repo / "ignored" / "x.md"
        ignored.parent.mkdir()
        ignored.write_text("/home/someone/x\n", encoding="utf-8", newline="\n")
        self.assertEqual(self.post_edit(ignored).returncode, 0)

    def load_hooks(self):
        spec = importlib.util.spec_from_file_location("claude_hooks_under_test", TOOLS / "claude_hooks.py")
        module = importlib.util.module_from_spec(spec)
        with mock.patch.object(sys, "path", [str(TOOLS), *sys.path]):
            spec.loader.exec_module(module)
        return module

    def powershell(self, command, **extra):
        return self.hook("pre-bash", {"cwd": str(self.repo), "tool_name": "PowerShell", "tool_input": {"command": command}, **extra})

    def test_powershell_tool_commands_follow_the_same_rules(self):
        self.assertDenied(self.powershell('git commit -m "Add x"'), "no Co-Authored-By")
        self.assertAllowed(self.powershell(f'git commit -m "Add x" -m "{CO_AUTHOR}"'))
        self.assertDenied(self.powershell("git push origin main", agent_id="a1"), "agents never push")
        self.assertDenied(self.powershell("git config user.name X"), "never change git config")
        (self.repo / "a.txt").write_text("a\n", encoding="utf-8", newline="\n")
        git(self.repo, "add", "a.txt")
        self.assertDenied(self.powershell("git push origin main"), "staged changes")

    def test_commands_never_point_git_elsewhere_through_environment_variables(self):
        denied = ["export GIT_DIR=/x/.git", "export -x GIT_WORK_TREE", "GIT_WORK_TREE=/x git status", "env GIT_INDEX_FILE=/x/index git add .",
                  "cd a && GIT_COMMON_DIR=/x/.git git log", "declare -x GIT_DIR=/x", "$env:GIT_DIR = 'C:\\x'", '$env:GIT_WORK_TREE="x"; git status',
                  "${env:GIT_DIR}='x'", "$env:git_dir='x'", "set GIT_DIR=x", "Set-Item Env:GIT_DIR x", "New-Item -Path Env:GIT_WORK_TREE -Value x",
                  "[Environment]::SetEnvironmentVariable('GIT_DIR', 'x')", "wsl.exe -e bash -c 'export GIT_DIR=/mnt/c/x; git status'",
                  'wsl bash -c "cd x && GIT_WORK_TREE=/y git log"', "wsl -e bash <<'EOF'\nGIT_INDEX_FILE=/y\ngit status\nEOF",
                  "GIT_DIR+=/x git status", "$env:GIT_DIR += 'x'"]
        allowed = ["echo GIT_DIR", "grep -rn GIT_DIR src", 'echo "GIT_WORK_TREE is set"', "echo $GIT_DIR", '[ "$GIT_DIR" = "" ] && echo none',
                   "git --git-dir=/x/.git status", "git -C /x log", "echo MY_GIT_DIR=1", "unset GIT_DIR", "Remove-Item Env:GIT_DIR", "echo $env:GIT_DIR"]
        for command in denied:
            for run in (self.bash, self.powershell):
                result = run(command)
                self.assertDenied(result, "never point git at another checkout")
                self.assertIn("git clone -q --branch", json.loads(result.stdout)["hookSpecificOutput"]["permissionDecisionReason"])
        for command in allowed:
            for run in (self.bash, self.powershell):
                self.assertAllowed(run(command))

    def test_empty_arguments_are_parsed(self):
        for command in ("git log --format=''", "echo '' > x", "printf '' | cat"):
            self.assertAllowed(self.bash(command))

    def test_git_output_is_decoded_as_utf8(self):
        message = Path(self.tmp.name) / "message"
        message.write_bytes("Café — done\n".encode("utf-8") + b"\x81\x8d\n")
        git(self.repo, "commit", "--allow-empty", "-q", "-F", str(message))
        shown = self.load_hooks().git(self.repo, "log", "-1", "--format=%B").stdout
        self.assertIn("Café — done", shown)

    def test_tool_output_is_decoded_as_utf8(self):
        (self.repo / "tools" / "say.py").write_text('print("Caf\\u00e9 \\u2014 done")\n', encoding="utf-8", newline="\n")
        result = self.load_hooks().run_tool(self.repo, dict(os.environ), "say.py")
        self.assertEqual(result.stdout.strip(), "Café — done")

    def test_hook_input_is_read_as_utf8_with_or_without_a_byte_order_mark(self):
        data = {"cwd": str(self.repo), "agent_id": "a1", "tool_input": {"command": 'echo "Café —" && git push'}}
        raw = json.dumps(data, ensure_ascii=False).encode("utf-8")
        for payload in (raw, b"\xef\xbb\xbf" + raw):
            result = subprocess.run([sys.executable, str(self.repo / "tools" / "claude_hooks.py"), "pre-bash"], input=payload, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn("agents never push", json.loads(result.stdout)["hookSpecificOutput"]["permissionDecisionReason"])

    def test_findings_with_characters_outside_the_console_code_page_are_written(self):
        self.copy_tools("check_doc_links.py", "user_docs.py", "check_local_data.py")
        features = self.repo / "docs" / "plans" / "features"
        features.mkdir(parents=True)
        (features / "README.md").write_text("# Features\n\n| File | Status |\n|---|---|\n| [g](g.md) | open |\n", encoding="utf-8", newline="\n")
        (features / "g.md").write_text("# G\n\n- [ ] G-1 → gap 日本, see /home/someone/notes.txt.\n", encoding="utf-8", newline="\n")
        data = {"cwd": str(self.repo), "tool_input": {"file_path": str(features / "g.md")}}
        result = subprocess.run([sys.executable, str(self.repo / "tools" / "claude_hooks.py"), "post-edit"], input=json.dumps(data).encode("utf-8"),
                                capture_output=True)
        self.assertEqual(result.returncode, 2, result.stderr)
        self.assertIn("日本".encode("utf-8"), result.stderr)

    def test_assets_library_is_found_under_each_platform_name(self):
        module = self.load_hooks()
        build = self.repo / "build"
        build.mkdir()
        with mock.patch.dict(os.environ):
            os.environ.pop("CHASE_ASSETS_LIB", None)
            self.assertNotIn("CHASE_ASSETS_LIB", module.assets_env(self.repo))
            for name in ("libchase_assets_c.so", "chase_assets_c.dll", "libchase_assets_c.dll"):
                (build / name).write_bytes(b"")
                self.assertEqual(Path(module.assets_env(self.repo)["CHASE_ASSETS_LIB"]), build / name)
                (build / name).unlink()


class LauncherTest(unittest.TestCase):
    ROOT = Path(__file__).resolve().parent.parent
    BRIEF_MISSING = {"tool_input": {"prompt": "Read your brief artifacts/rounds/20261026-hooks/20261026-hooks-A.md and follow it completely."}}
    PAYLOADS = {"pre-bash": {"tool_name": "PowerShell", "tool_input": {"command": "echo hi"}}, "pre-agent": {"tool_input": {"prompt": "Find X."}},
                "pre-tool": {"tool_input": {}}, "subagent-stop": {}, "post-edit": {"tool_input": {}}}

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.project = Path(self.tmp.name) / "project folder"
        (self.project / ".claude").mkdir(parents=True)
        (self.project / "tools").mkdir()
        shutil.copy(self.ROOT / ".claude" / "run_hook.cmd", self.project / ".claude")
        for sibling in ("claude_hooks.py", "hook_paths.py"):
            if (self.ROOT / "tools" / sibling).is_file():
                shutil.copy(self.ROOT / "tools" / sibling, self.project / "tools")

    def tearDown(self):
        self.tmp.cleanup()

    def settings_commands(self):
        settings = json.loads((self.ROOT / ".claude" / "settings.json").read_text(encoding="utf-8"))
        return [(group.get("matcher"), hook["command"]) for groups in settings["hooks"].values() for group in groups for hook in group["hooks"]]

    def fake_bin(self, name, stub=False, python=False):
        folder = Path(self.tmp.name) / name
        folder.mkdir()
        posix = sys.platform != "win32"

        def write(command, body):
            path = folder / (command + ("" if posix else ".cmd"))
            path.write_text(("#!/bin/sh\n" if posix else "@echo off\r\n") + body, encoding="utf-8", newline="")
            path.chmod(0o755)

        if stub:
            write("python3", "echo Python was not found >&2\nexit 49\n" if posix else "echo Python was not found 1>&2\r\nexit /b 9009\r\n")
        if python:
            write("python", f'exec "{sys.executable}" "$@"\n' if posix else f'"{sys.executable}" %*\r\n')
        return folder

    def path_without_python(self):
        keep = []
        for entry in os.environ["PATH"].split(os.pathsep):
            if not any((Path(entry) / (name + ext)).exists() for name in ("python3", "python", "py") for ext in ("", ".exe")):
                keep.append(entry)
        return keep

    def environment(self, *folders, **extra):
        return dict(os.environ, PATH=os.pathsep.join([*map(str, folders), *self.path_without_python()]), **extra)

    def launch(self, env, subcommand="pre-agent", payload=None):
        launcher = str(self.project / ".claude" / "run_hook.cmd")
        command = [launcher, subcommand] if sys.platform == "win32" else [SH, launcher, subcommand]
        payload = {"cwd": str(self.project), **self.BRIEF_MISSING} if payload is None else payload
        return subprocess.run(command, input=json.dumps(payload), capture_output=True, env=env, **TEXT)

    def assertHookRan(self, result):
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout)["hookSpecificOutput"]["permissionDecision"], "deny")

    def test_a_working_python_is_used_after_a_stub_python3(self):
        self.assertHookRan(self.launch(self.environment(self.fake_bin("a", stub=True, python=True))))

    def test_python_is_used_when_there_is_no_python3(self):
        self.assertHookRan(self.launch(self.environment(self.fake_bin("b", python=True))))

    @unittest.skipUnless(shutil.which("py"), "the py launcher exists only on Windows")
    def test_the_py_launcher_is_the_last_choice(self):
        self.assertHookRan(self.launch(self.environment(self.fake_bin("c", stub=True), Path(shutil.which("py")).parent)))

    def test_a_missing_python_is_reported_without_blocking(self):
        result = self.launch(self.environment(self.fake_bin("d", stub=True)))
        self.assertEqual(result.returncode, 1)
        self.assertIn("no Python 3.11 or newer found", result.stderr)

    def test_the_exit_code_of_the_hook_is_passed_on(self):
        result = self.launch(self.environment(self.fake_bin("e", python=True)), subcommand="bogus", payload={})
        self.assertEqual(result.returncode, 1)
        self.assertIn("usage: claude_hooks.py", result.stderr)

    def test_settings_run_every_hook_through_the_launcher_for_both_shells(self):
        found = self.settings_commands()
        self.assertEqual({command.rsplit(" ", 1)[1] for _, command in found}, set(self.PAYLOADS))
        for _, command in found:
            self.assertTrue(command.startswith('cd "${CLAUDE_PROJECT_DIR}"; ./.claude/run_hook.cmd '), command)
        self.assertEqual({command.rsplit(" ", 1)[1]: matcher for matcher, command in found}["pre-bash"], "Bash|PowerShell")

    def shells(self):
        if sys.platform != "win32":
            return {"sh": [SH, "-c"]}
        shells = {}
        if shutil.which("powershell"):
            shells["powershell"] = [shutil.which("powershell"), "-NoProfile", "-NonInteractive", "-Command"]
        bash = Path(shutil.which("git") or "x").parent.parent / "bin" / "bash.exe"
        if bash.is_file():
            shells["git-bash"] = [str(bash), "-c"]
        return shells

    def run_in_shell(self, shell, runner, command, env, data):
        if shell == "powershell":
            command = command.replace("${CLAUDE_PROJECT_DIR}", "${env:CLAUDE_PROJECT_DIR}") + "; if (-not $?) { if ($LASTEXITCODE -eq 2) { exit 2 } }"
        return subprocess.run([*runner, command], input=json.dumps(data), capture_output=True, env=env, cwd=self.tmp.name, **TEXT)

    def test_the_settings_commands_start_every_hook_under_each_shell_from_another_folder(self):
        shells = self.shells()
        self.assertTrue(shells)
        env = self.environment(self.fake_bin("f", python=True), CLAUDE_PROJECT_DIR=str(self.project))
        for shell, runner in shells.items():
            for _, command in self.settings_commands():
                subcommand = command.rsplit(" ", 1)[1]
                data = {"cwd": str(self.project), **self.PAYLOADS[subcommand]}
                result = self.run_in_shell(shell, runner, command, env, data)
                with self.subTest(shell=shell, subcommand=subcommand):
                    self.assertEqual((result.returncode, result.stdout, result.stderr), (0, "", ""))

    def test_the_settings_commands_pass_a_denial_through_each_shell(self):
        env = self.environment(self.fake_bin("g", python=True), CLAUDE_PROJECT_DIR=str(self.project))
        command = next(command for _, command in self.settings_commands() if command.endswith("pre-agent"))
        for shell, runner in self.shells().items():
            data = {"cwd": str(self.project), **self.BRIEF_MISSING}
            result = self.run_in_shell(shell, runner, command, env, data)
            with self.subTest(shell=shell):
                self.assertHookRan(result)
                self.assertEqual(result.stderr, "")

    def test_the_settings_commands_pass_exit_codes_and_findings_through_each_shell(self):
        env = self.environment(self.fake_bin("h", python=True), CLAUDE_PROJECT_DIR=str(self.project))
        command = next(command for _, command in self.settings_commands() if command.endswith("post-edit"))
        for code in (1, 2):
            stub = self.project / "tools" / "claude_hooks.py"
            stub.write_text(f"import sys\nsys.stderr.write('finding\\n')\nsys.exit({code})\n", encoding="utf-8", newline="\n")
            for shell, runner in self.shells().items():
                result = self.run_in_shell(shell, runner, command, env, {"cwd": str(self.project)})
                with self.subTest(shell=shell, code=code):
                    self.assertEqual((result.returncode, result.stdout, result.stderr.strip()), (code, "", "finding"))

if __name__ == "__main__":
    unittest.main()
