import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
import check_local_data


def problems(name, text):
    return [number for number, _, _ in check_local_data.violations(name, text)]


class CheckLocalDataTest(unittest.TestCase):
    def test_state_path_is_rejected(self) -> None:
        self.assertEqual(problems("docs/re/x.md", "see artifacts/oracle/states/m12.cst"), [1])

    def test_trace_directory_cmake_option_is_rejected(self) -> None:
        self.assertEqual(problems("CMakeLists.txt", 'set(TRACE_DIR "${CMAKE_SOURCE_DIR}/artifacts/oracle/traces")'), [1])

    def test_scratch_path_is_rejected(self) -> None:
        self.assertEqual(problems("tests/t.py", "a\nopen('artifacts/scratch/out.txt')"), [2])

    def test_emulator_home_is_rejected(self) -> None:
        self.assertEqual(problems("docs/x.md", "~/.local/share/azahar-emu/sdmc/x"), [1])
        self.assertEqual(problems("docs/x.md", "/home/jan/.local/share/x"), [1])
        self.assertEqual(problems("docs/x.md", "azahar-emu/states/a"), [1])

    def test_allowed_paths_pass(self) -> None:
        text = "\n".join([
            "artifacts/game/game.arm.elf", "artifacts/romfs/romfs", "artifacts/fib", "artifacts/textures/a",
            "artifacts/audio", "artifacts/progress.json", "artifacts/<dir>", "artifacts/ and build/", "`artifacts/`",
        ])
        self.assertEqual(problems("docs/x.md", text), [])

    def test_exemption_is_narrow(self) -> None:
        self.assertEqual(problems("docs/commands.md", "Screenshots go to `artifacts/oracle/out/`."), [])
        self.assertEqual(problems("docs/commands.md", "Open artifacts/oracle/r40-servant-out/a.png"), [1])
        self.assertEqual(problems("tests/save_test.cpp", 'setenv("HOME", "/home/u/x", 1)'), [])
        self.assertEqual(problems("tests/other.cpp", 'setenv("HOME", "/home/u/x", 1)'), [1])

    def test_concrete_paths_fail_in_every_exempt_file_class(self) -> None:
        concrete = ["artifacts/oracle/r50-A-out/x.mem", "artifacts/oracle/states/m12-foo.cst", "artifacts/rounds/r47/r47-A.md",
                    "artifacts/oracle/r47-servant-out/", "artifacts/scratch/out.txt", "artifacts/rounds/20261021-audio/20261021-audio-A.md",
                    "artifacts/oracle/20261021-audio-servant-out/x.png"]
        for name in ("docs/re/oracle.md", "docs/re/oracle-rpc.md", "docs/rules/port.md", "docs/commands.md", "CLAUDE.md",
                     ".claude/skills/lead-round/SKILL.md", ".claude/skills/lead-round/references/recorder.md",
                     "tools/azahar_oracle.py", "tools/merge_agent.py", "tests/test_merge_agent.py", "tools/round_log.py",
                     ".claude/skills/lead-round/evals/evals.json", "docs/plans/linux-port.progress.md"):
            for path in concrete:
                self.assertEqual(problems(name, "see `%s`." % path), [1], (name, path))

    def test_folder_names_and_placeholders_pass_in_exempt_files(self) -> None:
        rule_text = "\n".join([
            "`artifacts/oracle/`", "artifacts/oracle/states/", "artifacts/oracle/states/NAME.cst", "artifacts/oracle/states/README.md",
            "artifacts/oracle/rN-X-home", "artifacts/oracle/rN-\u2026", "artifacts/oracle/r{{N}}-servant-out/", "artifacts/rounds/",
            "artifacts/rounds/rN/rN-X.md", "artifacts/rounds/r{{N}}/{{rN-X}}.report.md", "artifacts/scratch/.",
            "artifacts/rounds/<round>/", "artifacts/rounds/<round>/<round>-X.md", "artifacts/rounds/{{round}}/{{agent}}.report.md",
            "artifacts/oracle/<round>-X-home", "artifacts/oracle/{{round}}-servant-out/",
        ])
        for name in ("docs/rules/port.md", "CLAUDE.md", ".claude/skills/lead-round/references/worker-brief.md"):
            self.assertEqual(problems(name, rule_text), [], name)
        self.assertEqual(problems("tools/azahar_oracle.py", "artifacts/oracle/out"), [])
        self.assertEqual(problems("docs/re/oracle.md", "artifacts/oracle/<home>/ and azahar-emu/states/<TITLEID>.00.cst"), [])
        self.assertEqual(problems("docs/commands.md", "artifacts/rounds/rN/"), [1])

    def test_check_runs_on_tracked_files(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            subprocess.run(["git", "init", "-q"], cwd=root, check=True)
            (root / "bad.md").write_text("x\nartifacts/oracle/states/a.cst\n")
            (root / "good.md").write_text("artifacts/game\n")
            (root / "untracked.md").write_text("artifacts/scratch/z\n")
            subprocess.run(["git", "add", "bad.md", "good.md"], cwd=root, check=True)
            errors = check_local_data.check(root)
            self.assertEqual(len(errors), 1)
            self.assertTrue(errors[0].startswith("bad.md:2: "))


if __name__ == "__main__":
    unittest.main()
