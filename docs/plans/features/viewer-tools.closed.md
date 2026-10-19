# Closed items of viewer-tools

Done items of [viewer-tools.md](viewer-tools.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] TOOLS-5 — Closed 2026-10-14: Forgejo's web view jumps to the explicit `<a id="ts-...">` entry anchors (checked by the user).
- [x] TOOLS-7 — Closed 2026-10-14: `chase-viewer`, `chase-soundtest`, `chase-fmv`, `chase-frontshot`, `chase-hudshot`, `chase-savecheck`, `chase-tabledump` and `chase-input-lab` check every option with `src/strict_parse.hpp` (bad numbers, unknown options and missing values stop with `<tool>: <option>: <reason>: '<text>'`); one rejection test per tool, with no game data needed.
- [x] TOOLS-6 — Closed 2026-10-14: the oracle records the console mode in `session.json` and in every saved `NAME.json` (`"new_3ds"`), and `loadstate` refuses a state from the other mode with a clear message; checked by unit tests and a real New 3DS save and reload. [oracle-states.md §2026-10-14T11:30](../../re/oracle-states.md#ts-20261014-113036).
- [x] TOOLS-10 — The debug overlay shows GPU time (timer query) and draw calls (`src/render/frame_stats.*`); tests `debug-metrics-core`. evidence: [debug-overlay.md](../../debug-overlay.md).
- [x] TOOLS-11 — The debug overlay is drawn on the front-end screens too (`src/game_front.hpp`, tests `front-overlay-*`). evidence: [debug-overlay.md](../../debug-overlay.md).
- [x] TOOLS-13 — CTest `debug-overlay-area` flaked once in a full parallel run; `debug-overlay-on/off` now use a fixed dt (`--script "0 0 2"`, commit 3df6c03) and five full `ctest -j 4` runs on 2026-10-14 (126 tests, loaded machine) passed it every time. evidence: commit 3df6c03.
- [x] TOOLS-14 — A virtual motion source for the oracle emulator (Cemuhook DSU server `tools/oracle_motion.py`, `--motion-port`, script step `tilt`); the scanner view now runs in the oracle. evidence: [oracle-setup.md §2026-10-18T22:03:32](../../re/oracle-setup.md#ts-20261018-220332).
- [x] TOOLS-16 — Item links in `priorities.md` open the source view of the feature file at the item line (`?display=source#L<n>`); `check_doc_links.py` checks the line and `--fix` follows moved lines. evidence: tests/test_check_doc_links.py.
