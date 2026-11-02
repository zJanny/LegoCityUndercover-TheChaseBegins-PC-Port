import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
import check_file_names

TOOL = Path(__file__).resolve().parent.parent / "tools" / "check_file_names.py"

# Names the guard rejected on the tree before the renames of 20261102-naming-B, recorded by 20261102-naming-A on 2026-11-02
# with `python3 tools/check_file_names.py` (22 files; the three CTest names and three targets are pinned below).
OFFENDER_FILES = [
    "src/game_switch_glue.cpp", "src/game_switch_use_glue.cpp", "src/runtime/game_ai_spawner_glue.cpp",
    "src/runtime/game_attach_bone_glue.cpp", "src/runtime/game_attach_children_glue.cpp",
    "src/runtime/game_attack_characters_glue.cpp", "src/runtime/game_attack_facts_glue.cpp",
    "src/runtime/game_character_hit_glue.cpp", "src/runtime/game_character_takedown_glue.cpp",
    "src/runtime/game_generator_natives_glue.cpp", "src/runtime/game_mission13_fixes_glue.cpp",
    "src/runtime/game_mission14_natives_glue.cpp", "src/runtime/game_mission_end_1_3_glue.cpp",
    "src/runtime/game_platform_glue.cpp", "src/runtime/game_platform_riding_glue.cpp", "src/runtime/game_push_cart_glue.cpp",
    "src/runtime/game_rocking_follow_glue.cpp", "src/runtime/mission13_end.cpp", "src/runtime/mission13_end.hpp",
    "tests/mission13_end_test.cpp", "tests/mission14_census_real_test.cpp", "tests/mission14_natives_test.cpp",
]


def banned_words(errors):
    return [error.split('banned word "')[1].split('"')[0] for error in errors]


def file_words(path):
    return banned_words(check_file_names.file_violations([path]))


def cmake_words(text):
    return banned_words(check_file_names.cmake_violations(text, "CMakeLists.txt"))


class CheckFileNamesTest(unittest.TestCase):
    def test_names_by_function_pass(self) -> None:
        for path in ("src/runtime/ai_move_watch.cpp", "tests/script_particles_test.cpp", "tools/check_local_data.py",
                     "src/assets/level_objects.hpp", "tests/fixtures.py", "tests/save_fixture.hpp", "src/render/shaders/sky.glsl",
                     "src/templates.cpp", "tools/tmpl.py", "src/runtime/quickstart.cpp", "src/render/h264_2d.cpp",
                     "tests/sha256_test.cpp"):
            self.assertEqual(check_file_names.file_violations([path]), [], path)

    def test_each_banned_word_is_rejected(self) -> None:
        cases = {
            "mission": "src/runtime/mission_end.cpp", "missions": "src/runtime/missions.cpp", "mission13": "src/mission13_end.cpp",
            "m4": "tests/m4_walk_test.cpp", "round": "src/runtime/round_two.cpp", "agent": "tools/agent_notes.py",
            "session": "src/session_cache.cpp", "fix": "src/runtime/fix_camera.cpp", "fixes": "src/runtime/camera-fixes.cpp",
            "fixed": "tests/fixed_door_test.cpp", "glue": "src/runtime/game_door_glue.cpp", "patch": "src/patch_loader.cpp",
            "temp": "tools/temp_run.py", "tmp": "tools/tmp_run.py", "hack": "src/hack.cpp", "wip": "src/runtime/wip_door.cpp",
            "misc": "src/misc.cpp", "stuff": "src/stuff.cpp", "workaround": "src/runtime/workaround.cpp",
            "standin": "src/runtime/standin_door.cpp", "todo": "src/todo.cpp", "quick": "tools/quick_check.py",
        }
        for word, path in cases.items():
            self.assertEqual(len(check_file_names.file_violations([path])), 1, (word, path))

    def test_stand_in_with_separator_is_rejected(self) -> None:
        self.assertEqual(file_words("src/runtime/stand_in_door.cpp"), ["stand-in"])
        self.assertEqual(file_words("src/runtime/stand-in-door.cpp"), ["stand-in"])

    def test_number_pairs_are_rejected(self) -> None:
        self.assertEqual(file_words("src/runtime/door_1_3.cpp"), ["number pair 1-3"])
        self.assertEqual(file_words("tests/door-1-4.cpp"), ["number pair 1-4"])
        self.assertEqual(file_words("tests/door_1.cpp"), [])
        self.assertEqual(file_words("tests/door_1_x_3.cpp"), [])

    def test_whole_words_only(self) -> None:
        for path in ("src/prefix.cpp", "src/template.cpp", "src/roundabout.cpp", "src/agentless.cpp", "src/mm.cpp", "src/m.cpp",
                     "src/mx4.cpp", "src/patchwork.cpp", "src/stuffing.cpp"):
            self.assertEqual(check_file_names.file_violations([path]), [], path)

    def test_every_path_component_is_checked(self) -> None:
        self.assertEqual(file_words("src/mission_end/door.cpp"), ["mission"])
        self.assertEqual(file_words("tests/glue/door_test.cpp"), ["glue"])

    def test_message_names_file_word_and_rule(self) -> None:
        [message] = check_file_names.file_violations(["src/runtime/game_door_glue.cpp"])
        self.assertIn("src/runtime/game_door_glue.cpp", message)
        self.assertIn('"glue"', message)
        self.assertIn("what the code or tool does", message)

    def test_allow_list_is_narrow(self) -> None:
        self.assertEqual(check_file_names.file_violations(["src/frontend/round_menu.cpp", "src/frontend/round_menu.hpp"]), [])
        self.assertEqual(check_file_names.file_violations(["tools/round_log.py", "tests/test_round_log.py"]), [])
        self.assertEqual(check_file_names.file_violations(["tools/merge_agent.py", "tests/test_merge_agent.py"]), [])
        self.assertEqual(file_words("src/runtime/round_door.cpp"), ["round"])
        self.assertEqual(file_words("tools/round_log_glue.py"), ["round", "glue"])
        self.assertEqual(file_words("tools/agent_notes.py"), ["agent"])
        self.assertEqual(file_words("src/frontend/round_menu_fix.cpp"), ["fix"])

    def test_skipped_and_unchecked_folders(self) -> None:
        for path in ("tools/oracle/mission_1_3_walk.txt", "tests/data/mission_1_3.csv", "tests/qol_separation_fixtures/glue/a.cpp",
                     "tests/gl_only_in_render/mission_fix.cpp", "tests/frame_keep_points/glue.cpp", "docs/re/mission-1-3.md",
                     "README.md", "CMakeLists.txt", "third_party/glue/a.c"):
            self.assertEqual(check_file_names.file_violations([path]), [], path)

    def test_listed_offenders_are_all_rejected(self) -> None:
        self.assertEqual(len(OFFENDER_FILES), 22)
        errors = check_file_names.file_violations(OFFENDER_FILES)
        self.assertEqual({error.split(":")[0] for error in errors}, set(OFFENDER_FILES))

    def test_listed_offender_tests_and_targets_are_rejected(self) -> None:
        text = "\n".join([
            "add_executable(mission14-natives-test tests/mission14_natives_test.cpp)",
            "add_executable(mission14-census-real-test tests/mission14_census_real_test.cpp)",
            "add_executable(mission13-end-test tests/mission13_end_test.cpp)",
            "add_test(NAME mission14-natives COMMAND mission14-natives-test)",
            "add_test(NAME mission-1-4-census-real-data COMMAND mission14-census-real-test)",
            "add_test(NAME mission13-end COMMAND mission13-end-test)",
        ])
        self.assertEqual(len(check_file_names.cmake_violations(text, "CMakeLists.txt")), 7)

    def test_cmake_names_by_function_pass(self) -> None:
        text = "add_executable(ai-move-watch-test tests/a.cpp)\nadd_test(NAME ai-move-watch COMMAND x)\nadd_library(chase_x src/x.cpp)"
        self.assertEqual(cmake_words(text), [])

    def test_cmake_test_executable_and_library_names_are_checked(self) -> None:
        self.assertEqual(cmake_words("add_test(NAME mission13-end COMMAND x)"), ["mission"])
        self.assertEqual(cmake_words("add_executable(mission14-natives-test tests/a.cpp)"), ["mission"])
        self.assertEqual(cmake_words("add_library(chase_glue src/a.cpp)"), ["glue"])
        self.assertEqual(cmake_words("add_test(\n  NAME door-patch-check\n  COMMAND x)"), ["patch"])
        self.assertEqual(cmake_words("add_test(NAME mission-1-4-census-real-data COMMAND x)"), ["mission", "number pair 1-4"])

    def test_cmake_variables_in_names_are_ignored(self) -> None:
        self.assertEqual(cmake_words("add_test(NAME qol-separation-rejects-${fixture} COMMAND x)"), [])

    def test_game_scene_tests_may_name_the_scene(self) -> None:
        self.assertEqual(cmake_words("add_test(NAME game-mission-1-1 COMMAND x)\nadd_test(NAME game-key-use-1-3 COMMAND x)"), [])
        self.assertEqual(cmake_words("add_test(NAME game-door-glue COMMAND x)"), ["glue"])
        self.assertEqual(cmake_words("add_test(NAME game-fix-1-3 COMMAND x)"), ["fix"])
        self.assertEqual(cmake_words("add_executable(game-mission-1-1 tests/a.cpp)"), ["mission", "number pair 1-1"])

    def test_cmake_message_names_target_word_and_rule(self) -> None:
        [message] = check_file_names.cmake_violations("add_test(NAME door-wip COMMAND x)", "CMakeLists.txt")
        self.assertIn("door-wip", message)
        self.assertIn('"wip"', message)
        self.assertIn("what the code or tool does", message)

    def test_tracked_files_of_a_repository_are_checked(self) -> None:
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            subprocess.run(["git", "init", "-q", str(root)], check=True)
            for path in ("src/runtime/ai_move_watch.cpp", "src/runtime/game_door_glue.cpp", "tools/oracle/mission_1_3.txt"):
                (root / path).parent.mkdir(parents=True, exist_ok=True)
                (root / path).write_text("")
            (root / "CMakeLists.txt").write_text("add_test(NAME door-hack COMMAND x)\n")
            subprocess.run(["git", "-C", str(root), "add", "-A"], check=True)
            errors = check_file_names.check(root)
            self.assertEqual(len(errors), 2)
            self.assertTrue(errors[0].startswith("src/runtime/game_door_glue.cpp:"))
            self.assertTrue(errors[1].startswith("CMakeLists.txt:"))
            run = subprocess.run([sys.executable, str(TOOL), "--root", str(root)], capture_output=True, text=True)
            self.assertEqual(run.returncode, 1)
            self.assertIn("game_door_glue.cpp", run.stdout)
            self.assertIn("door-hack", run.stdout)
            (root / "src/runtime/game_door_glue.cpp").unlink()
            (root / "CMakeLists.txt").write_text("add_test(NAME door-ok COMMAND x)\n")
            subprocess.run(["git", "-C", str(root), "add", "-A"], check=True)
            run = subprocess.run([sys.executable, str(TOOL), "--root", str(root)], capture_output=True, text=True)
            self.assertEqual((run.returncode, run.stderr), (0, ""))

    def test_options_are_strict(self) -> None:
        for arguments in (["--bogus"], ["--roo", "."], ["--root"], ["--root", "/nonexistent-folder"], ["extra"]):
            run = subprocess.run([sys.executable, str(TOOL)] + arguments, capture_output=True, text=True)
            self.assertEqual(run.returncode, 2, arguments)
            self.assertIn("check_file_names.py: error:", run.stderr)

    def test_outside_a_git_repository_stops_with_an_error(self) -> None:
        with tempfile.TemporaryDirectory() as folder:
            run = subprocess.run([sys.executable, str(TOOL), "--root", folder], capture_output=True, text=True)
            self.assertNotEqual(run.returncode, 0)
            self.assertIn("cannot list the tracked files", run.stderr)


if __name__ == "__main__":
    unittest.main()
