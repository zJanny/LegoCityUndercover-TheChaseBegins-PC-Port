import contextlib
import io
import sys
import tempfile
import unittest
from datetime import date
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
import round_log  # noqa: E402

TODAY = date(2026, 10, 21)
LOG = """# Progress

## Session Log

- 2026-10-20T12:45:27+02:00 — Lead (round 44 end): merged r44-A.
- 2026-10-21T10:34:08+02:00 — Lead (round 48 end): merged r48-A.
- 2026-10-21T11:00:01+02:00 — Agent 20261021-audio-A (sfx): done.
"""
ARCHIVE = """# Session log archive

- 2026-10-13T15:22:58+02:00 — Lead (round 12 end): merged.
- 2026-10-20T12:45:27+02:00 — Lead (round 44 end): merged r44-A.
"""


def run(root, *argv):
    out, err = io.StringIO(), io.StringIO()
    with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
        try:
            code = round_log.main(["--root", str(root), *argv])
        except SystemExit as stop:
            code = stop.code
    return code, out.getvalue(), err.getvalue()


class RoundNameTest(unittest.TestCase):
    def test_valid_names(self) -> None:
        for name in ("20261021-audio", "20261021-audio-sfx", "20261021-a2-b3-c4", "20240229-leap"):
            self.assertIsNone(round_log.name_error(name, TODAY), name)

    def test_malformed_names_are_rejected(self) -> None:
        cases = {
            "round-49": "expected YYYYMMDD-slug",
            "49": "expected YYYYMMDD-slug",
            "20261021": "expected YYYYMMDD-slug",
            "20261021-Audio": "expected YYYYMMDD-slug",
            "20261021-audio-A": "expected YYYYMMDD-slug",
            "20261021-a-b-c-d": "expected YYYYMMDD-slug",
            "20261021-9lives": "expected YYYYMMDD-slug",
            "2026-10-21-audio": "expected YYYYMMDD-slug",
            "20261341-audio": "not a calendar date",
            "20230229-audio": "not a calendar date",
            "20261022-audio": "in the future",
            "20261021-audio-servant": "must not be servant or audit",
            "20261021-audit": "must not be servant or audit",
        }
        for name, expected in cases.items():
            self.assertIn(expected, round_log.name_error(name, TODAY) or "", name)

    def test_cli_rejects_a_malformed_name_with_the_text(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            code, _, err = run(tmp, "name", "20261021_audio")
        self.assertEqual(code, 2)
        self.assertIn("name: expected YYYYMMDD-slug", err)
        self.assertIn("'20261021_audio'", err)

    def test_taken_names_are_found_in_logs_reservations_and_round_folders(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            plans = root / "docs" / "plans"
            (plans / "features").mkdir(parents=True)
            (plans / "linux-port.progress.md").write_text(LOG)
            (plans / "features" / "id-reservations.md").write_text(
                "# ID reservations\n\nHeld by agents named like `20261021-docs-A`.\n\n"
                "| F | 1 | 2 | 20261020-cars-B | 2026-10-20T10:00:01+02:00 |\n")
            (root / "artifacts" / "rounds" / "20261019-docs").mkdir(parents=True)
            code, out, _ = run(root, "name", "20261021-audio")
            self.assertEqual(code, 1)
            self.assertIn("round name 20261021-audio is taken: docs/plans/linux-port.progress.md:7", out)
            self.assertIn("add a slug word", out)
            self.assertEqual(run(root, "name", "20261020-cars")[0], 1)
            code, out, _ = run(root, "name", "20261019-docs")
            self.assertEqual(code, 1)
            self.assertIn("artifacts/rounds/20261019-docs (local)", out)
            for free in ("20261021-audio-sfx", "20261021-aud", "20261020-cars-ped", "20261021-docs"):
                code, out, _ = run(root, "name", free)
                self.assertEqual(code, 0, free)
                self.assertIn(f"round name {free} is free", out)

    def test_name_pattern_matches_agents_but_not_longer_names(self) -> None:
        pattern = round_log.name_pattern("20261021-audio")
        for text in ("Lead (20261021-audio end)", "Agent 20261021-audio-A (x)", "| 20261021-audio-servant |", "20261021-audio-audit"):
            self.assertTrue(pattern.search(text), text)
        for text in ("20261021-audio-sfx", "x20261021-audio", "20261021-audio-AB", "120261021-audio"):
            self.assertFalse(pattern.search(text), text)


class AuditTest(unittest.TestCase):
    def make(self, tmp, log=LOG, archive=ARCHIVE):
        plans = Path(tmp) / "docs" / "plans"
        plans.mkdir(parents=True)
        (plans / "linux-port.progress.md").write_text(log)
        (plans / "linux-port.progress-archive.md").write_text(archive)
        return Path(tmp)

    def test_counts_old_and_new_round_ends_once(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = self.make(tmp, LOG + "- 2026-10-21T15:00:01+02:00 — Lead (20261021-audio end): merged.\n")
            self.assertEqual(round_log.audit_state(root), (4, None))
            code, out, _ = run(root, "audit")
            self.assertEqual(code, 0)
            self.assertIn("user docs audit not due: 4 round ends and no docs audit recorded; due after 10", out)

    def test_count_restarts_after_an_audit_done_or_declined(self) -> None:
        for mark in ("done", "declined"):
            with tempfile.TemporaryDirectory() as tmp:
                log = LOG.replace("merged r48-A.", f"merged r48-A; user docs audit: {mark}.")
                log += "- 2026-10-21T15:00:01+02:00 — Lead (20261021-audio end): merged.\n"
                root = self.make(tmp, log)
                self.assertEqual(round_log.audit_state(root), (1, "2026-10-21T10:34:08+02:00"), mark)

    def test_due_after_ten_round_ends(self) -> None:
        lines = "".join(f"- 2026-10-21T12:{n:02d}:01+02:00 — Lead (20261021-r{n} end): merged.\n" for n in range(10))
        with tempfile.TemporaryDirectory() as tmp:
            root = self.make(tmp, LOG.replace("merged r48-A.", "docs audit: done.") + lines)
            code, out, _ = run(root, "audit")
            self.assertIn("user docs audit due: 10 round ends since the last docs audit (2026-10-21T10:34:08+02:00)", out)

    def test_other_lead_lines_are_not_round_ends(self) -> None:
        log = LOG + ("- 2026-10-21T15:00:01+02:00 — Lead (recorded round 40 emulator findings): x.\n"
                     "- 2026-10-21T15:00:02+02:00 — Lead (20261021-audio-A merge): x.\n")
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual(round_log.audit_state(self.make(tmp, log))[0], 3)


if __name__ == "__main__":
    unittest.main()
