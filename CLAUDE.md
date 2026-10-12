# CLAUDE.md

Guidance for Claude Code (claude.ai/code) in this repository.

## Project

Early-stage Linux x86_64 reconstruction of *LEGO City Undercover: The Chase Begins* (Nintendo 3DS) from the decrypted `.cci` image in the repo root. Two independent pieces exist:

1. **Extraction pipeline** (`tools/extract_game.py`, Python 3.11+ stdlib only). Reads the NCSD/CCI image, NCCH partition 0, exheader and ExeFS, checks every SHA256, decompresses `.code` (backwards LZ) and wraps it in a synthetic ELF32 ARM file (`game.arm.elf`) for disassembly in REA.
2. **Native libraries and tools** (`src/`, C++17 + SDL2): asset decoders, mixer, input laboratory, model/level viewer, FMV player. This is **not** the game runtime. Gameplay, rendering states, scripts and saves are not recovered. Never describe it as a playable port.

Plan and progress: `docs/plans/linux-port.md` and `docs/plans/linux-port.progress.md` (steps S1–S4). Before resuming port work, read the plan and the progress file's Status and Next Action sections, not the whole log. Record each session's commands, results and timestamps in the progress file. Findings do not go there (see "Reverse-engineering findings").

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
- **REA:** for the ARM executable, use the installed REA skill/integration and its documented workflow. If unavailable, use other disassembly tools. Record reproducible findings and evidence IDs in the matching `docs/re/` file. Never claim REA analysis that did not happen.
- **Platform:** Linux x86_64 only. Prefer portable C++17 and the standard library, but add no Windows builds, infrastructure or abstractions.

## C++ is the source of truth

1. Every format decoder and piece of runtime game logic lives in C++ (`chase_assets`, `chase_audio`, `chase_input`, or a new library under `src/`). Fix bugs and add features there first.
2. Python is for exploration and CLI tools (hash brute force, statistics, censuses). Never write a second full decoder for a format C++ implements; call C++ through the binding.
3. New formats get one implementation, in C++. A Python prototype may stay as an exploration aid but is not kept in sync or used by tests.
4. Existing Python decoders (`tools/convert_*.py`, `extract_*.py`, `tt_resource.py`, `preview_model.py`) are frozen legacy. When one needs a behaviour change that C++ also implements, replace its decoding logic with a binding call in the same commit. Never fix both.
5. Tests pin recorded reference values, not a live second implementation. Next to each value record its provenance (Python command, game-code evidence ID, or manual inspection). `assets-real-test` uses real data; `assets-test` covers synthetic edge cases.
6. Fix violations in a focused commit and note it in the progress file. If too large, add it as an open item there.

## Reverse-engineering findings (docs/re/)

- Findings (addresses, struct/record layouts, format specs, codec rules, evidence IDs, hypotheses, unknowns) live in `docs/re/<topic>.md`, indexed in `docs/re/README.md`. Read the index, then only the file for your topic.
- Append under a dated `##` heading. Mark corrections of earlier entries explicitly. Put unknowns at the end of the entry. Create a new topic file and index line when none fits.
- The progress file holds only work state: status, steps, decisions, blockers, next actions, a short session log. A log entry that produced findings gets one line linking to the `docs/re/` file.
- `docs/re/` is for developers (the plain-language rule does not apply) and contains no game assets.

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
