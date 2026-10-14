# CLAUDE.md

Guidance for Claude Code (claude.ai/code) in this repository.

## Project

PC port of *LEGO City Undercover: The Chase Begins* (Nintendo 3DS), built from the user's decrypted `.cci` image in the repo root. The goal is a faithful, complete port: everything works and behaves exactly like the original 3DS game, playable from start to finish, natively on Linux x86_64 first. The project is in an early stage and is **not playable yet**. Never describe it as a playable game or a finished port. Two independent pieces exist:

1. **Extraction pipeline** (`tools/extract_game.py`, Python 3.11+ stdlib only). Reads the NCSD/CCI image, NCCH partition 0, exheader and ExeFS, checks every SHA256, decompresses `.code` (backwards LZ) and wraps it in a synthetic ELF32 ARM file (`game.arm.elf`) for disassembly in REA.
2. **Native libraries and tools** (`src/`, C++17 + SDL2): asset decoders, mixer, input laboratory, model/level viewer, FMV player, and `chase-game`, a development runtime that reproduces measured original behaviour (locomotion, collision, camera, falling, jumping, level objects and characters). Missions, enemies, menus, scripts, most rendering states and saves are not recovered yet. `chase-game` is the start of the game runtime, not the finished game.

Plan and progress: `docs/plans/linux-port.md` and `docs/plans/linux-port.progress.md` (steps S1–S4). Before resuming port work, read the plan and the progress file's Status and Next Action sections, not the whole log, then the feature index `docs/plans/features/README.md` and the one feature file you work on (see "Feature files"). Record each session as one log line in the progress file. Findings, open items and stand-ins do not go there (see "Feature files" and "Reverse-engineering findings").

## Goal and faithfulness

- **Goal:** a PC port that works and behaves exactly like the original 3DS game, playable from start to finish. Linux x86_64 comes first.
- **Order of work:** (1) finish the faithful Linux version; (2) only after that, and only when the user says so, the Windows port; (3) only after the game is finished for PC, quality-of-life features. Until then: no Windows work of any kind and no QoL features or deviations from the original.
- **Behaviour comes from the original:** code (REA, objdump) and measurements (oracle). Do not "improve" it, round it off or simplify it.
- **Stand-ins:** anything not yet taken from the original (a guessed asset, a measured constant used instead of its source, an approximation) must be labelled as a stand-in in the code's names or the matching `docs/re/` file and listed as a stand-in in the feature file (see "Feature files"). Never present a stand-in as original behaviour.
- **Input mapping:** mapping the 3DS controls to keyboard and mouse (configurable bindings, and stand-ins such as the `Walk` key for a half-tilted circle pad) is part of the port, not a QoL feature. It must not change how the game behaves.
- Never invent game behaviour. Record unknowns.

## Commands

```sh
python3 -m unittest discover -s tests -p 'test_*.py' -v   # extractor tests, synthetic fixtures
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build
ctest --test-dir build --output-on-failure                # add -R <name> for one test
python3 tools/extract_game.py "<image>.cci" --output artifacts/game   # output dir must NOT exist
```

All other commands (extractors, converters, viewer, FMV, sound test, input lab, single-test names) are in `docs/commands.md`.

## Architecture rules

- `src/input.{hpp,cpp}` (`chase_input`) and `src/audio/` (`chase_audio`) have no SDL dependency. `readBindings` takes a key-name resolver callback and `main.cpp` passes `SDL_GetKeyFromName`. Keep SDL out of both so their tests link without it.
- `src/save/` (`chase_save`) has no SDL dependency. It treats the options and slot blocks as opaque bytes.
- `src/assets/` (`chase_assets`, namespace `chase::assets`) has no SDL dependency and is the source of truth for every format decoder. Pose math replicates game routines 0x24c734, 0x248870, 0x24c5a0, 0x252cb0.
- `chase_assets_c` (`src/assets/c_api.cpp` → `build/libchase_assets_c.so`) is the C ABI for Python, wrapped by `tools/chase_native.py` (ctypes). Add an entry point there when a Python tool needs a C++ decoder. Tests that use it skip when the library is not built.
- Input rules, checked by `tests/input_test.cpp` and `--smoke-test` in `main.cpp` (update both when behaviour changes):
  - A hold action stays active while any binding is held.
  - A toggle flips only on a fresh press; key repeats are ignored.
  - Opposite directions cancel in `horizontal()`/`vertical()`.
  - Focus loss resets all held and toggled state.
  - Touch maps only inside the aspect-fitted 320x240 viewport.
- `config/controls.cfg` format: `Action key|mouse Code hold|toggle`, `#` comments. Key names are SDL names with `_` for spaces. Mouse codes: `Left`, `Middle`, `Right`, `X1`, `X2`. Action names are 3DS buttons, not recovered gameplay meanings.
- **Extractor safety:** open the image read-only; bounds-check every offset; fail on encryption or hash mismatch; cap decompressed output at `MAX_CODE`; never overwrite an existing output directory. `manifest.json` records offsets, hashes and ARM segment addresses as evidence. The ELF is a derived analysis wrapper, not an original binary.
- REA cannot open the `.cci`. Analyse `artifacts/<dir>/game.arm.elf`.
- **Game version:** all `docs/re/` addresses are for one exact build (Europe, `00040000000AD600`, v1.0). `docs/re/game-version.md` lists its hashes and how to check an image. With a different image, warn the user loudly that the documented offsets are probably wrong, and stop before applying them.
- **REA:** for the ARM executable, use the installed REA skill/integration and its documented workflow. If unavailable, use other disassembly tools. Record reproducible findings and evidence IDs in the matching `docs/re/` file. Never claim REA analysis that did not happen.
- **Platform:** Linux x86_64 first. Windows comes after the Linux version is finished. Until the user says so, do no Windows work of any kind: no builds, infrastructure, abstractions or preparation. Prefer portable C++17 and the standard library.

## C++ is the source of truth

1. Every format decoder and piece of runtime game logic lives in C++ (`chase_assets`, `chase_audio`, `chase_input`, or a new library under `src/`). Fix bugs and add features there first.
2. Python is for exploration and CLI tools (hash brute force, statistics, censuses). Never write a second full decoder for a format C++ implements; call C++ through the binding.
3. New formats get one implementation, in C++. A Python prototype may stay as an exploration aid but is not kept in sync or used by tests.
4. The Python converters (`tools/convert_*.py`, `extract_fib.py`) are thin command-line tools over the binding and hold no decoding logic. Keep it that way: a format change goes into C++ and, when a tool needs it, a C ABI entry point. `extract_game.py` and `extract_romfs.py` read the NCSD/NCCH image and have no C++ counterpart.
5. Tests pin recorded reference values, not a live second implementation. Next to each value record its provenance (Python command, game-code evidence ID, or manual inspection). `assets-real-test` uses real data; `assets-test` covers synthetic edge cases.
6. Fix violations in a focused commit and note it in the progress file. If too large, add it as an open item in the feature file.

## Reverse-engineering findings (docs/re/)

- Findings (addresses, struct/record layouts, format specs, codec rules, evidence IDs, hypotheses, unknowns) live in `docs/re/<topic>.md`, indexed in `docs/re/README.md`. Read the index, then only the file for your topic.
- Append under a dated `##` heading. Mark corrections of earlier entries explicitly. Put unknowns at the end of the entry. Create a new topic file and index line when none fits.
- Every dated entry has an anchor line `<a id="ts-YYYYMMDD-HHMMSS"></a>` (the heading's time digits; `HHMM` when it has no seconds) directly above its `##` heading. Every link to a dated entry is `file.md#ts-...` with `§<timestamp>` in the link text. Never rewrite a heading timestamp. After any docs edit run `python3 tools/check_doc_links.py --fix`; CTest `doc-links` fails on a broken link or missing anchor.
- The progress file holds only work state: status, steps, decisions, blockers, next actions, a short session log. A log entry that produced findings gets one line linking to the feature file.
- `docs/re/` is for developers (the plain-language rule does not apply) and contains no game assets.

## Feature files (docs/plans/features/)

- Known gaps live in one place per feature: `docs/plans/features/<feature>.md`, indexed in `docs/plans/features/README.md` (one line per feature: status and top open items). To find or record a gap: CLAUDE.md, then the index, then one feature file, then only the `docs/re/` entry it links. Keep the index under about 40 lines and a feature file under about 150 lines; shorten the findings summary and link to `docs/re/` instead of growing it.
- Fixed sections per feature: Scope, Status, Verified behaviour, Stand-ins, Open items, Findings summary (links into `docs/re/`), Code and tests. Open items and stand-ins have stable IDs (`DOOR-1`, `DOOR-S1`), never reused. Dependencies between features are written on the item: `needs: <ID>` on the blocked side and `blocks: <ID>` on the blocking side; keep both sides in sync.
- Open items, stand-ins and the current state of a feature live only there. Evidence (addresses, layouts, evidence IDs, unknowns) lives only in `docs/re/`. The progress file holds only work state. Never copy content between the three; link.
- End of every session: update the feature file (status, stand-ins and open items added or removed, findings summary with a link to the new `docs/re/` entry), add the dated `docs/re/` entry, then add ONE progress-file log line: timestamp, what was worked on, how far it got, link to the feature file. Findings, stand-in lists and open items in the log or in Next Action are not allowed.
- The `feature-development` workflow (plan file plus progress file) applies only to the umbrella pair `docs/plans/linux-port.md` and `docs/plans/linux-port.progress.md`. Work on a subsystem updates its `docs/plans/features/<feature>.md` and adds one progress-file log line. Never create a new per-feature `docs/plans/<feature>.md` plus `.progress.md` pair.
- A new feature gets a new file from the template and an index line. Keep Next Action in the progress file to priorities plus links.
- `docs/plans/` is for developers (the plain-language rule does not apply).

## Runtime oracle (Azahar)

The original game runs in Azahar as a measuring reference. Read `docs/re/oracle.md` before any emulator use. Every agent must use the fast path:

- Run the emulator only through `tools/azahar_oracle.py`. Never start `azahar` directly or point it at your own config or saves.
- Start experiments from save states (`--state NAME`, e.g. `gameplay-start`; needs `--display xvfb`), never by booting and replaying menus. If a checkpoint is missing, create it once (e.g. `tools/oracle/to-gameplay.txt`, ending with `savestate NAME`) and reuse it.
- **Ask the user for hard-to-reach states.** When reaching a game point needs playing (missions, fights, driving, long routes), or an agent's scripted attempts fail (about 3 tries or 15 minutes), the agent stops trying and asks the user through the lead. Agents send the request to the lead (`SendMessage` to `main`); the lead passes it to the user. The request states:
  - the state name;
  - why it is needed;
  - the game progress it needs (which missions are done, which must not be started);
  - where to go and how to get there;
  - where to stand and which way to face;
  - what must not happen first (triggers, cutscenes);
  - the save slot.
  Meanwhile the agent continues with work that does not need the state. The user saves in their own Azahar. The lead copies the slot read-only to `artifacts/oracle/states/NAME.cst` and writes `NAME.json` (`file`, `new_3ds`, `source` with slot, time, position and game progress). The user's saves are unmodified reference states; record their game progress in `docs/re/oracle.md`.
- For exploration, keep one session (`--keep`) and probe with `--attach`; save a state at each new screen. Never reboot per probe.
- Navigate with `wait` (free run), not long `frames` runs. Use `frames`/`trace` only where per-update data is needed.
- Use `--rpc` for screenshots, memory reads and timing (`sample`). Never measure rates or fps from stepped frames (they inflate the game clock about 30 %). Use `--rpc-input` for buttons.
- Only one emulator per machine can use `--rpc` (fixed UDP port). Give each agent its own `--port` and `--home` under `artifacts/oracle/`.
- Stop what you start (`--stop`), check `pgrep -x azahar` and `pgrep -x Xvfb`, and never kill processes you did not start.

## Constraints

- Never modify the `.cci` image. Never stage or commit it or anything game-derived (`*.cci`, `*.3ds`, `artifacts/`, `build/` are gitignored).
- Record what is unknown instead of inventing game behaviour. Don't size native pointers from ARM addresses.

## Graphify

- At the start of work check that Graphify is installed and the graph exists; reuse working setup. If missing or broken, follow the installed skill/docs, and report the exact blocker rather than claiming success.
- For architecture, dependency and code-flow questions, query the graph first, then verify against source. Rebuild or update it after meaningful source changes and verify before calling it current.
- `.graphifyignore` must exclude `docs/`, everything in `.gitignore`, build outputs, `artifacts/`, extracted assets and temp files. Include source, tests and build config. Verify the exclusions work and report any Graphify limitation that prevents them.
- Read `docs/` files directly, only when the task needs them. Use the Claude Code session for semantic extraction; send no repository content to an external provider without authorization.

## Workflows

**Git**
- Commit small, focused, verified units. Review the diff first; include no unrelated or unintended files.
- Never push without explicit user authorization; permission does not carry over between requests.
- Keep the README progress section current: run `python3 tools/progress_report.py --update-readme README.md` before the final commit of a branch and after every merge to main, and commit the README change with that work. CTest `progress-readme` fails while the section is stale. On a merge conflict in that section, regenerate it instead of editing it by hand. Measures and their meaning: `docs/progress.md`.
- Keep `.gitignore` current for new generated, local or build files. Never ignore files merely to hide unintended changes.

**Game assets and legal**
- Never commit original game assets or proprietary content. Inspect staged changes before every commit and remove prohibited files from the commit and working tree as appropriate.
- If such assets are already in history, report it and propose a remediation. Do not rewrite shared history or delete user data without authorization.
- Commit scripts, metadata and tools that operate on the user's own game copy, not the content.

**Documentation (`docs/`)**
- `docs/` (except `docs/re/` and `docs/plans/`) is for non-technical users: plain language, short sentences, concise steps and lists.
- Update it in the same logical step as any change to user-visible behaviour, setup, configuration, controls or workflows.
- Document the current version, not its history. Remove obsolete text, keep what is still needed, and link instead of duplicating.
- `README.md` stays minimal: project overview, essential setup, links to `docs/`.

**Code comments**
- Don't add explanatory comments; use clear names and simple control flow.
- Keep license and attribution notices, tool/compiler/linter/type-checker directives, and comments documenting non-obvious correctness, security or unavoidable implementation constraints.
- Prefer improving the code over keeping an ordinary explanatory comment.

**File editing**
- Prefer the native Read/Edit/Write tools. Use Python or scripts only for structured transformations, bulk operations, data processing or formats the tools can't handle. Review the diff afterwards.

**Subagents**
- Use the cheapest model that can do the task (Haiku-class for exploration, summarisation, Graphify extraction). Keep tasks narrow, with only the needed files and constraints.
- Do not delegate correctness-critical verification; use tests, checksums and source inspection.
- Do not spawn subagents when a local script or the overhead outweighs the benefit.

Always use current system time when putting a time into a file!
