import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
import forbidden_files  # noqa: E402


class ForbiddenFilesTest(unittest.TestCase):
    def test_game_derived_and_build_paths_are_refused_whatever_their_content(self):
        for path in ("game.cci", "dump/LEGO.3DS", "state.cst", "artifacts/romfs/x.txt", "build/a.o", "build-release/x",
                     "graphify-out/graph.json", "oracle/session.json", "shot.PNG", "music.bcstm", "level.blvl"):
            self.assertEqual(forbidden_files.forbidden_reason(path, b"text"), "game-derived or build path", path)
            self.assertEqual(forbidden_files.forbidden_reason(path, None), "game-derived or build path", path)

    def test_text_files_and_similar_names_are_allowed(self):
        for path in ("src/runtime/buildit.cpp", "docs/re/buildit.md", "tests/data/session_notes.json", "tests/data/camera/sweep.csv",
                     "tools/oracle/to-gameplay.txt"):
            self.assertIsNone(forbidden_files.forbidden_reason(path, b"plain text\n"), path)

    def test_binary_content_is_refused_except_bundled_fonts(self):
        self.assertEqual(forbidden_files.forbidden_reason("tests/data/table.inc", b"\x01\x00\x02"), "binary content")
        self.assertEqual(forbidden_files.forbidden_reason("fonts/Font.ttf", b"\x00\x01"), "binary content")
        self.assertEqual(forbidden_files.forbidden_reason("third_party/fonts/sub/Font.ttf", b"\x00\x01"), "binary content")
        self.assertIsNone(forbidden_files.forbidden_reason("third_party/fonts/Font.ttf", b"\x00\x01"))
        self.assertIsNone(forbidden_files.forbidden_reason("tests/data/empty.csv", b""))

    def test_binary_probe_looks_at_the_first_8000_bytes_like_git(self):
        self.assertEqual(forbidden_files.forbidden_reason("a.txt", b"a" * 7999 + b"\x00"), "binary content")
        self.assertIsNone(forbidden_files.forbidden_reason("a.txt", b"a" * 8000 + b"\x00"))

    def test_check_paths_reads_only_what_the_path_rule_allows(self):
        heads = {"a.txt": b"text", "b.raw": b"\x00\x01", "c.png": b"never read"}
        read = []

        def read_head(path):
            read.append(path)
            return heads[path]

        found = forbidden_files.check_paths(["a.txt", "b.raw", "c.png"], read_head)
        self.assertEqual(found, [("b.raw", "binary content"), ("c.png", "game-derived or build path")])
        self.assertEqual(read, ["a.txt", "b.raw"])


if __name__ == "__main__":
    unittest.main()
