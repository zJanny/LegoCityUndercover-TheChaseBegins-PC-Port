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
        self.assertEqual(problems("docs/x.md", "/home/someone/.local/share/x"), [1])
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

    def test_output_folder_and_trace_names_are_rejected(self) -> None:
        for text in ("see agentHUD2-out/h2.png", "r40-servant-out/c1_L1", "20261021-audio-servant-out", "run in myfolder-out/x.png", "see myfolder-out/sub/cmp.json",
                     "replays runA.mem", "hitP4.mem update 26"):
            for name in ("tests/vehicle_test.cpp", "docs/re/vehicles.md", "docs/plans/features/x.md"):
                self.assertEqual(problems(name, text), [1], (name, text))

    def test_output_folder_rule_leaves_other_text_alone(self) -> None:
        self.assertEqual(problems("tests/t.cpp", "graphify-out/graph.json and check-out and time-out"), [])
        self.assertEqual(problems("docs/re/hud.md", "fade-out/fade-in, zoom-out/zoom-in and time-out/retry"), [])
        self.assertEqual(problems("docs/rules/port.md", "artifacts/oracle/r{{N}}-servant-out/ and {{round}}-servant-out/"), [])
        self.assertEqual(problems("src/runtime/x.cpp", "// replays runA.mem"), [])
        self.assertEqual(problems("tests/test_azahar_oracle_watch.py", "(directory / 'demo.mem').read_bytes()"), [])
        self.assertEqual(problems("tests/test_merge_agent.py", '"trace.mem", "state.cst"'), [])
        self.assertEqual(problems("docs/re/oracle-measuring.md", "`trace N NAME mem` also writes `NAME.mem`"), [])
        self.assertEqual(problems("docs/re/hud.md", "also writes `NAME.mem`"), [1])

    def test_distro_locations_are_rejected_outside_install_sections(self) -> None:
        for text in ("`/usr/bin/azahar` (Arch package 1:2126.2-1)", "/usr/src/debug/azahar/x", "sudo pacman -S build-essential"):
            self.assertEqual(problems("docs/re/oracle-setup.md", text), [1], text)
            self.assertEqual(problems("CLAUDE.md", text), [1], text)
            self.assertEqual(problems("README.md", text), [], text)
            self.assertEqual(problems("docs/getting-started.md", text), [], text)
            self.assertEqual(problems("docs/building.md", text), [], text)
        self.assertEqual(problems("docs/commands.md", "an Azahar build with the RPC server"), [])

    def test_machine_phrases_and_user_names_only_warn(self) -> None:
        def warned(name, text):
            return [number for number, _, _ in check_local_data.warnings(name, text)]

        self.assertEqual(warned("docs/re/oracle.md", "ok\nMeasured (This machine, software)"), [2])
        self.assertEqual(warned("docs/re/x.md", "the user's own Azahar save"), [1])
        self.assertEqual(warned("docs/commands.md", "this machine"), [])
        self.assertEqual(warned("docs/building.md", "ask jan or zJanny"), [1])
        self.assertEqual(warned("README.md", "clone https://github.com/zJanny/Project.git"), [])
        self.assertEqual(warned("README.md", "written by zJanny"), [1])
        self.assertEqual(warned("LICENSE", "jan"), [])
        self.assertEqual(warned("tools/progress_trend.py", "MONTHS = ('Jan', 'Feb')"), [])
        self.assertEqual(problems("docs/building.md", "ask jan or zJanny"), [])

    def test_loaded_state_must_be_saved_by_a_committed_script(self) -> None:
        def hits(name, text, created=frozenset({"gameplay-start"})):
            return [number for number, _, _ in check_local_data.violations(name, text, created)]

        self.assertEqual(hits("docs/re/x.md", "ok\nloaded with `--state street-after-1-1` here"), [2])
        self.assertEqual(hits("tests/a_test.cpp", "// loadstate some-state"), [1])
        self.assertEqual(hits("src/a.cpp", "// --state a-b-c"), [1])
        self.assertEqual(hits("docs/commands.md", "--state gameplay-start and --state NAME and loadstate the"), [])
        self.assertEqual(hits("docs/commands.md", "`loadstate` refuses a state; after loadstate the frame counter"), [])
        self.assertEqual(hits("tools/oracle/a.txt", "loadstate some-state"), [1])
        self.assertEqual(hits("tests/test_azahar_oracle_launch.py", "oracle.loadstate('some-state')"), [])

    def test_oracle_scripts_may_load_only_roles_and_states_they_save(self) -> None:
        def hits(text, created=frozenset({"made-here"})):
            return [number for number, _, _ in check_local_data.violations("tools/oracle/a.txt", text, created)]

        self.assertEqual(hits("loadstate @PATROL_CAR\nloadstate made-here\nloadstate other\nloadstate x-y"), [3, 4])
        self.assertEqual(hits("# Run with --state @PATROL_CAR on a New 3DS emulator.\n# Run with --state a-b-c here"), [2])
        self.assertEqual(hits("loadstate m1-state  # comment\nstick 0 0  # input survives loadstate. Next"), [1])
        self.assertEqual(hits("  loadstate\tsome-state"), [1])
        self.assertEqual(hits("# input survives loadstate. then frames\n# after loadstate the frame counter"), [])
        self.assertEqual(hits("savestate some-state\nscreenshot some-state"), [])

    def test_created_states_come_from_savestate_lines_of_the_scripts(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            subprocess.run(["git", "init", "-q"], cwd=root, check=True)
            (root / "tools" / "oracle").mkdir(parents=True)
            (root / "tools" / "oracle" / "make.txt").write_text("frames 2\nsavestate made-here\n")
            (root / "tools" / "oracle" / "use.txt").write_text("loadstate made-here\nloadstate @ROLE\nloadstate user-state\n")
            (root / "docs").mkdir()
            (root / "docs" / "a.md").write_text("--state made-here\n--state other-state\n")
            subprocess.run(["git", "add", "tools", "docs"], cwd=root, check=True)
            errors = check_local_data.check(root)
            self.assertEqual(len(errors), 2)
            self.assertTrue(errors[0].startswith("docs/a.md:2: "))
            self.assertTrue(errors[1].startswith("tools/oracle/use.txt:3: "))
            self.assertIn("use @ROLE and --state-param", errors[1])

    def test_warnings_do_not_fail_the_run(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            subprocess.run(["git", "init", "-q"], cwd=root, check=True)
            (root / "docs" / "re").mkdir(parents=True)
            (root / "docs" / "re" / "x.md").write_text("measured on this machine\n")
            subprocess.run(["git", "add", "docs"], cwd=root, check=True)
            self.assertEqual(check_local_data.check(root), [])
            self.assertEqual(len(check_local_data.check_warnings(root)), 1)
            result = subprocess.run([sys.executable, str(Path(check_local_data.__file__)), "--root", str(root)],
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 0)
            self.assertIn("1 warning(s)", result.stdout)

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
