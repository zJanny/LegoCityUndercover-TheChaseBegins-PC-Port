# CLAUDE.md

Guidance for Claude Code (claude.ai/code) in this repository. Keep this file to rules every agent needs (under about 12 KB); details of one subsystem go into its feature file or `docs/re/`.

## Project

PC port of *LEGO City Undercover: The Chase Begins* (Nintendo 3DS), built from the user's decrypted `.cci` image in the repo root. Goal: a faithful, complete port that behaves exactly like the original and is playable from start to finish, natively on Linux x86_64 first. The project is early and **not playable yet**; never describe it as a playable game or a finished port.

1. **Extraction pipeline** (`tools/extract_game.py`, Python 3.11+ stdlib): reads the image, checks every SHA256, decompresses `.code` and wraps it in `game.arm.elf` for disassembly.
2. **Native libraries and tools** (`src/`, C++17 + SDL2): asset decoders, mixer, input lab, viewer, FMV player, and `chase-game`, the development runtime that reproduces measured original behaviour. Missions, enemies, most scripts and rendering states are not recovered yet.

Plan and progress: `docs/plans/linux-port.md` and `docs/plans/linux-port.progress.md`. Before resuming work read the progress file's Status and Next Action (not the whole log), the feature index `docs/plans/features/README.md` and the one feature file you work on.

## Goal and faithfulness

- **Order of work:** (1) the faithful Linux version; (2) only when the user says so, Windows; (3) only after the game is finished for PC, quality-of-life features. Until then no Windows work of any kind (builds, infrastructure, abstractions, preparation) and no QoL features or deviations.
- **Behaviour comes from the original:** code (REA, objdump) and measurements (oracle). Do not "improve", round off or simplify it. Never invent game behaviour; record unknowns.
- **Stand-ins:** anything not yet taken from the original (guessed asset, measured constant instead of its source, approximation) is labelled as a stand-in in code names or `docs/re/` and listed in the feature file. Never present a stand-in as original behaviour.
- **Input mapping** (3DS controls to keyboard/mouse, stand-ins like the `Walk` key) is part of the port, not QoL, and must not change game behaviour. Input rules and their tests: `docs/plans/features/input.md`; binding format: `docs/controls.md`.

## Commands

```sh
python3 -m unittest discover -s tests -p 'test_*.py' -v
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build
ctest --test-dir build --output-on-failure                # -R <name> for one test
python3 tools/extract_game.py "<image>.cci" --output artifacts/game   # output dir must NOT exist
```

All other commands and test names: `docs/commands.md`.

## Architecture rules

- No SDL in `chase_input` (`src/input.*`), `chase_audio` (`src/audio/`), `chase_save` (`src/save/`) or `chase_assets` (`src/assets/`), so their tests link without it. `readBindings` takes a key-name resolver callback.
- `chase_assets` is the source of truth for every format decoder. `chase_assets_c` (`src/assets/c_api.cpp`) is the C ABI for Python via `tools/chase_native.py`; add an entry point when a Python tool needs a decoder. Tests that use it skip when it is not built.
- **`src/game.cpp` only wires features together** (options, the order of the update, calls). New runtime logic goes into its own file under `src/runtime/` (or the matching library) with its own test; a feature adds at most a few lines of glue to `game.cpp`. CTest `game-cpp-wiring` guards its size.
- **Shaders:** OpenGL 4.6 core. Shader code lives only in `.glsl` files under `src/render/shaders/`, embedded at build time; never in a `.cpp` or `.hpp` file.
- **Strict input:** every command-line option and development script (`--script`, `--front-script`, `--start`, oracle scripts) is parsed strictly. Malformed, unknown or out-of-range input stops with an error naming the option and the bad text; never fall back to a default or skip silently. Each new option gets a test for its rejection.
- **Script natives in batches:** natives are implemented one batch item at a time (SCRIPT-26..39 in `docs/plans/features/script-system.md`, census in `docs/re/script-natives.md`), never one by one; read each handler before transcribing it and update the census row.
- **Extractor safety:** open the image read-only, bounds-check every offset, fail on encryption or hash mismatch, cap decompressed output at `MAX_CODE`, never overwrite an output directory. The ELF is a derived analysis wrapper, not an original binary.
- **Game version:** all `docs/re/` addresses are for one build (Europe, `00040000000AD600`, v1.0; hashes in `docs/re/game-version.md`). With a different image, warn the user loudly and stop before applying offsets.
- **REA:** analyse `artifacts/<dir>/game.arm.elf` (REA cannot open the `.cci`) with the installed REA skill; otherwise other disassembly tools. Never claim REA analysis that did not happen. Don't size native pointers from ARM addresses.
- Linux x86_64 first; portable C++17 and the standard library.

## C++ is the source of truth

1. Every format decoder and piece of runtime logic lives in C++. Fix bugs and add features there first.
2. Python is for exploration and CLI tools. Never write a second decoder for a format C++ implements; call C++ through the binding. A Python prototype may stay as an exploration aid but is not kept in sync or used by tests.
3. The converters (`tools/convert_*.py`, `extract_fib.py`) are thin tools over the binding with no decoding logic. `extract_game.py` and `extract_romfs.py` read the image and have no C++ counterpart.
4. Tests pin recorded reference values, not a live second implementation; next to each value record its provenance (command, evidence ID, oracle trace or manual inspection).
5. Fix violations in a focused commit; if too large, add an open item in the feature file.

## Documentation rules

Three places, never copied between them, only linked: **findings** in `docs/re/`, **gaps** (open items, stand-ins, feature state) in `docs/plans/features/`, **work state** in the progress file. CTest `doc-links` (`tools/check_doc_links.py`) checks the mechanical rules below; run `python3 tools/check_doc_links.py --fix` after any docs edit.

**Findings (`docs/re/<topic>.md`, index `docs/re/README.md`)**
- Read the index, then only the entry a feature file links, not whole files.
- Append dated entries at the end: anchor line `<a id="ts-YYYYMMDD-HHMMSS"></a>` directly above `## <date -Iseconds> — title`. Links to entries are `file.md#ts-...` with `§<timestamp>` in the link text; inside the same file only `#ts-...` (Forgejo ignores same-page links that repeat the file name). Never rewrite a heading timestamp. Unknowns go last in the entry.
- **Corrections are marked at both ends:** the new entry links the old one with link text starting `Corrects`, and the old entry gets a line `Corrected by [file.md §<ts>](file.md#ts-...): <what changed>` directly under its heading. Otherwise old entries stay unchanged.
- **Split a file by subtopic when it passes 40 KB:** move whole entries with their anchors into `docs/re/<topic>-<subtopic>.md`, leave a short pointer list in the old file, update the index and all links.
- For developers; no game assets.

**Feature files (`docs/plans/features/<feature>.md`, index `README.md` there)**
- Sections: Scope, Status, Verified behaviour, Stand-ins, Open items, Findings summary, Code and tests. Index at most about 40 lines, a feature file at most 150 lines.
- Items have stable IDs (`DOOR-1`, stand-ins `DOOR-S1`), never reused. Dependencies on both sides: `needs: <ID>` on the blocked item, `blocks: <ID>` on the blocking one.
- **An item or stand-in is at most 400 characters:** the gap, needs/blocks, one evidence link. Addresses, lists and details go into the linked `docs/re/` entry.
- End of every session: update the feature file (status, items, findings summary link), add the dated `docs/re/` entry, then ONE progress-log line (timestamp, what, how far, link to the feature file). No findings, stand-ins or items in the log or in Next Action.
- Never create per-feature plan/progress pairs; the `feature-development` workflow applies only to `linux-port.md` / `.progress.md`. A new feature gets a file from the template and an index line.

**User docs (`docs/` except `re/` and `plans/`)**
- Plain language, short sentences, lists. Update them in the same step as any change to user-visible behaviour, setup, controls or workflows. Document the current version, not its history; link instead of duplicating. `README.md` stays minimal.

## Runtime oracle (Azahar)

The original runs in Azahar as a measuring reference. Read `docs/re/oracle.md` before any emulator use. Fast path only:

- Run the emulator only through `tools/azahar_oracle.py`, never `azahar` directly or with your own config or saves.
- Start from save states (`--state NAME --display xvfb`), never by booting through menus. Create a missing checkpoint once (script ending with `savestate NAME`) and reuse it.
- **Hard-to-reach states:** when a game point needs real play, or about 3 scripted tries or 15 minutes fail, stop and ask the user through the lead (`SendMessage` to `main`) with: state name, why, game progress needed (missions done / not started), where to go and how, where to stand and face, what must not happen first, the save slot. Continue other work meanwhile. The lead copies the user's slot read-only to `artifacts/oracle/states/NAME.cst` with `NAME.json` (`file`, `new_3ds`, `source`) and records its game progress in `docs/re/oracle.md`. The user's saves are never modified.
- Explore with one session (`--keep`, probe with `--attach`), save a state at each new screen. Navigate with `wait`; use `frames`/`trace` only for per-update data.
- Use `--rpc` for screenshots, memory reads and timing (`sample`); never measure rates from stepped frames. `--rpc-input` for buttons. Only one emulator per machine may use `--rpc`; each agent has its own `--port` and `--home` under `artifacts/oracle/`.
- Stop what you start (`--stop`), check `pgrep -x azahar` / `pgrep -x Xvfb`, never kill processes you did not start.

## Constraints and legal

- Never modify the `.cci`. Never stage or commit it or anything game-derived (`*.cci`, `*.3ds`, `artifacts/`, `build/` are gitignored), and no original game assets or screenshots. Inspect staged changes before every commit.
- If such content is already in history, report it and propose a remediation; never rewrite shared history or delete user data without authorization.

## Graphify

- At the start of work check that Graphify is installed and the graph exists; if broken, follow the installed skill and report the exact blocker.
- For architecture, dependency and code-flow questions query the graph first, then verify against source. Rebuild after meaningful source changes.
- `.graphifyignore` excludes `docs/`, everything in `.gitignore`, build outputs, `artifacts/` and temp files. Send no repository content to an external provider without authorization.

## Workflows

**Git**
- Commit small, focused, verified units; review the diff first. Each agent ends its own commit messages with its own `Co-Authored-By:` line; no session links.
- Merge agent branches into main only with `tools/merge_agent.py` (`prepare`, `check`, `finish`): it squashes, refuses game-derived and binary files, regenerates the README section, builds warning-free and runs all tests. The lead reviews the diff and findings in between; the script never pushes.
- Never push without explicit user authorization; permission does not carry over between requests.
- Run `python3 tools/progress_report.py --update-readme README.md` before the final commit of a branch and after every merge (CTest `progress-readme`); on a conflict in that section regenerate it. Measures: `docs/progress.md`.
- Keep `.gitignore` current; never ignore files to hide unintended changes.

**Code and files**
- No explanatory comments; clear names and simple control flow. Keep license notices, tool directives and comments on non-obvious correctness or unavoidable constraints.
- Prefer the native Read/Edit/Write tools; scripts only for bulk or structured edits. Review the diff afterwards.

**Subagents**
- Cheapest model that can do the task; narrow tasks with only the needed files and rules. Do not delegate correctness-critical verification.
- An agent in a git worktree first runs `ln -s <main checkout>/artifacts artifacts` in the worktree root (gitignored); without it real-data tests skip and the README update refuses to run. Every agent brief says so.

Always use current system time (`date -Iseconds`) when putting a time into a file!
