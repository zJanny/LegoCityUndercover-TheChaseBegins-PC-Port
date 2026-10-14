# Closed items of viewer-tools

Done items of [viewer-tools.md](viewer-tools.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] TOOLS-5 — Closed 2026-10-14: Forgejo's web view jumps to the explicit `<a id="ts-...">` entry anchors (checked by the user).
- [x] TOOLS-7 — Closed 2026-10-14: `chase-viewer`, `chase-soundtest`, `chase-fmv`, `chase-frontshot`, `chase-hudshot`, `chase-savecheck`, `chase-tabledump` and `chase-input-lab` check every option with `src/strict_parse.hpp` (bad numbers, unknown options and missing values stop with `<tool>: <option>: <reason>: '<text>'`); one rejection test per tool, with no game data needed.
- [x] TOOLS-6 — Closed 2026-10-14: the oracle records the console mode in `session.json` and in every saved `NAME.json` (`"new_3ds"`), and `loadstate` refuses a state from the other mode with a clear message; checked by unit tests and a real New 3DS save and reload. [oracle-states.md §2026-10-14T11:30](../../re/oracle-states.md#ts-20261014-113036).
