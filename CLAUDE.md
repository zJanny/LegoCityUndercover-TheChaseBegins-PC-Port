# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Early-stage Linux x86_64 reconstruction of *LEGO City Undercover: The Chase Begins* (Nintendo 3DS) from the decrypted `.cci` image in the repo root. Right now there are two independent pieces:

1. **Extraction pipeline** (`tools/extract_game.py`, Python 3.11+ stdlib only). It reads the NCSD/CCI image, then the NCCH partition 0, the exheader and the ExeFS, and checks every SHA256. It decompresses `.code` (backwards LZ) and wraps it in a synthetic ELF32 ARM file (`game.arm.elf`) for disassembly in REA.
2. **Native input laboratory** (`src/`, C++17 + SDL2). It tests configurable keyboard/mouse input that maps onto 3DS buttons and the 320x240 lower touch screen. It is **not** the game runtime. Gameplay, rendering, audio, assets and saves have not been recovered yet.

The plan and its progress log live in `docs/plans/linux-port.md` and `docs/plans/linux-port.progress.md`, with steps S1–S4. Read both before you resume port work. Update the progress file with the commands, results and timestamps from each session. Reverse-engineering findings do not go there: see "Reverse-engineering findings" below.

## Commands

```sh
# Python extractor tests (synthetic fixtures; no game image needed)
python3 -m unittest discover -s tests -p 'test_*.py' -v
# single Python test
python3 -m unittest discover -s tests -p 'test_*.py' -k test_hash_corruption

# Extract the real image (output dir must NOT already exist)
python3 tools/extract_game.py "LEGO City Undercover - The Chase Begins (Europe) (En,Fr,De,Es,It,Nl,Pt,Da,Ru).cci" --output artifacts/game
# Extract RomFS assets (verifies superblock + IVFC tree; same no-overwrite rule)
python3 tools/extract_romfs.py "LEGO City Undercover - The Chase Begins (Europe) (En,Fr,De,Es,It,Nl,Pt,Da,Ru).cci" --output artifacts/romfs
# Unpack the FUSE asset archive (RefPack-variant codec recovered from 0x12efe0)
python3 tools/extract_fib.py artifacts/romfs/romfs/lego_city3ds.fib --names artifacts/romfs/romfs/lego_city3ds.csv --output artifacts/fib
# Textures (.btga/.bpng/.bdds, PICA200 formats) -> PNG; --census validates headers only
python3 tools/convert_textures.py artifacts/fib/files --output artifacts/textures
# Sounds (.bwav = wrapped CWAV, DSP-ADPCM) -> WAV; --census parses headers only
python3 tools/convert_audio.py artifacts/fib/files --output artifacts/audio
# Pooled b* resource layout check; model parse/stride census; model preview render
python3 tools/tt_resource.py --census artifacts/fib/files
python3 tools/convert_models.py --census artifacts/fib/files
python3 tools/preview_model.py MODEL.bfnmdl --assets artifacts/fib/files --output out.png

# Native build + tests
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
ctest --test-dir build -R input-core          # pure input-core unit test
ctest --test-dir build -R input-sdl-smoke     # SDL integration, runs with SDL_VIDEODRIVER=dummy
ctest --test-dir build -R assets-core         # native asset runtime, synthetic fixtures
ctest --test-dir build -R assets-real-data    # opt-in: needs artifacts/romfs (skips with code 77 otherwise)
ctest --test-dir build -R viewer-real-smoke   # registered only if artifacts/romfs exists at configure time
ctest --test-dir build -R audio-mixer         # mixer core (no SDL)
ctest --test-dir build -R fmv-real-smoke      # registered only if FFmpeg and artifacts/romfs exist at configure time
build/chase-tabledump tables/tableslevels.bxls      # data tables as TSV (columns by index; no names in the file)
build/chase-soundtest music/act_bass_break.bwav   # plays decoded sounds; SDL_AUDIODRIVER=dummy for headless
# FMV player (needs FFmpeg libs): Mobiclip .moflex from RomFS, subtitles from the fib in the game font
SDL_VIDEODRIVER=offscreen build/chase-fmv --lang us --screenshot out.bmp --time 3 artifacts/romfs/romfs/cutscenes/01_gameintro.moflex

# Development model viewer (needs libepoxy; not gameplay). Offscreen screenshot for headless checks:
SDL_VIDEODRIVER=offscreen build/chase-viewer --screenshot out.bmp models/lvl_00_policestation/room1.bfnmdl --exclude sky
# Skinned animation playback (first model is posed; --frame N for a fixed frame)
SDL_VIDEODRIVER=offscreen build/chase-viewer --screenshot out.bmp models/char_bear/char_bear.bfnmdl --skeleton models/char_bear/char_bear.bfnskl --anim models/char_bear/walk.bfnanm --frame 12
# In-engine cutscene: cast from csdata/models/<id>/manifest.bstxt, camera from its ExportCam tracks
SDL_VIDEODRIVER=offscreen build/chase-viewer --screenshot out.bmp --cutscene csdata/models/1_1_a --level levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl --exclude sky --frame 200

# Run the lab (looks for config/controls.cfg relative to cwd; CMake copies it into build/)
cd build && ./chase-input-lab [--config PATH] [--smoke-test]
```

## Architecture notes

- **The input core has no SDL dependency.** `chase_input` (`src/input.{hpp,cpp}`) takes plain `(Source, code, down, repeat)` events. `readBindings` takes a key-name resolver callback, and `main.cpp` passes in `SDL_GetKeyFromName`. Keep SDL out of `input.cpp` so that `input-test` links without SDL.
- Input rules, which `tests/input_test.cpp` and the `--smoke-test` path in `main.cpp` both check:
  - A hold action stays active while any of its bindings is held.
  - A toggle action flips only on a fresh press, and key repeats are ignored.
  - Opposite directions cancel each other in `horizontal()`/`vertical()`.
  - Focus loss resets all held and toggled state.
  - Touch maps only inside the aspect-fitted 320x240 viewport.
  
  When you change behaviour, update both tests.
- **The native asset runtime is `chase_assets`** (`src/assets/`, namespace `chase::assets`). It has no SDL dependency. It is the source of truth for every format decoder (see "C++ source of truth" below): FUSE archive + RefPack, the pooled reader with `PoolCursor`, PICA texture decode, models, levels, navigation grids, skeletons and animation, sound, localisation and fonts.
- `chase_assets` decodes skeletons and animation curves (`anim.cpp`) and resolves skin joints through each part's bone palette. Pose math replicates the game's routines (0x24c734, 0x248870, 0x24c5a0, 0x252cb0).
- **`chase_assets_c` is the C ABI for Python** (`src/assets/c_api.cpp`, built as `build/libchase_assets_c.so`). `tools/chase_native.py` wraps it with ctypes (`skin_matrices`). Add an entry point there when a Python tool needs a C++ decoder. Tests that use it skip when the library is not built.
- **`chase_audio` is the mixer core** (`src/audio/`). It has no SDL dependency: voices with linear resampling, mono→stereo, gain and loop points, pulled as float stereo frames by `mix()`. `chase-soundtest` is a thin SDL front end, and `mixer-test` checks the mixing rules.
- `config/controls.cfg` format: `Action key|mouse Code hold|toggle`, with `#` comments. Key names are SDL names with `_` in place of spaces. Mouse codes are `Left`, `Middle`, `Right`, `X1` and `X2`. Action names are 3DS buttons, not recovered gameplay meanings.
- **Extractor safety rules:**
  - Open the image read-only.
  - Bounds-check every offset.
  - Fail on encryption or a hash mismatch.
  - Cap decompressed output at `MAX_CODE`.
  - Never overwrite an existing output directory.

  `manifest.json` records offsets, hashes and ARM segment addresses as evidence. The ELF is a derived analysis wrapper: its entry point is inferred from the text base, and it is not an original binary.
- REA cannot open the `.cci` directly. Analyse `artifacts/<dir>/game.arm.elf` instead, and record REA evidence IDs in the matching `docs/re/` file.

- **REA reverse engineering:** When investigating the ARM executable, use the installed REA skill or integration if one is available and relevant. Follow its documented workflow rather than inventing commands. If unavailable, use the available disassembly and analysis tools directly, and record reproducible findings and evidence IDs in the matching `docs/re/` file. Do not claim REA analysis was performed unless it actually was.

- **Future platform compatibility:** Linux x86_64 is the only current development target. A Windows port may be considered at a later stage, but this must not slow down or complicate current Linux development. Do not implement Windows builds, add Windows-specific infrastructure, or introduce abstractions solely for hypothetical Windows support. When making architectural decisions, prefer portable C++17 and standard-library facilities where practical, and be mindful of OS-specific APIs, filesystem assumptions, path handling, process behavior, and build configuration. Keep SDL2 and other existing cross-platform dependencies in mind, but prioritize a simple, maintainable Linux implementation over speculative portability work.

## C++ source of truth (no parallel implementations)

1. **C++ is the source of truth.** Every format decoder and every piece of game logic that the runtime uses lives in C++ (`chase_assets`, `chase_audio`, `chase_input`, or a new library under `src/`). Fix bugs and add features there first.
2. **Python is for exploration and command-line tools.** Use Python freely for throwaway reverse-engineering work: hash brute force, statistics, data fitting, censuses. Do not write a second full decoder in Python for a format that C++ implements. When a Python tool needs a decoder that C++ has, call the C++ code through a binding instead of re-implementing it.
3. **New formats get one implementation.** Implement a newly recovered format in C++ only. A Python prototype written while investigating may stay as an exploration aid, but it must not be kept in sync as a reference or be relied on by tests.
4. **Existing Python decoders are frozen legacy.** The decoders that already exist in `tools/` (`convert_*.py`, `extract_*.py`, `tt_resource.py`, `preview_model.py`) may stay while they work. When one needs a behaviour change that C++ also implements, do not edit the Python decoder: replace it with a call into the C++ code through the binding, and delete the Python decoding logic in the same commit. Never fix both.
5. **Tests pin frozen reference values instead of a live second implementation.** Real-data tests (`assets-real-test`) compare against recorded checksums and totals. Next to each value, record how it was obtained: a Python command, a game-code evidence ID, or a manual inspection. Synthetic tests (`assets-test`) cover the edge cases.
6. **Fix violations when you find them.** If you come across anything that breaks rules 1–5, fix it in a focused commit of its own and note the fix in the progress file. Examples: logic that exists only in Python although the runtime needs it, a new format implemented twice, Python and C++ giving different results, a test that depends on running a Python decoder, or a reference value without its provenance. If the fix is too large for the current task, record it as an open item in `docs/plans/linux-port.progress.md`.

## Reverse-engineering findings (docs/re/)

Keep findings out of the progress file so agents load only what they need.

1. **Findings live in `docs/re/<topic>.md`**, one file per topic, indexed in `docs/re/README.md`. Findings are addresses, struct and record layouts, format specs, codec rules, evidence IDs, hypotheses and unknowns.
2. **The progress file holds only work state**: status, steps, decisions, blockers, next actions, and a short session log (date, what was done, commands, test results, commits). A log entry that produced findings gets one line that links to the `docs/re/` file. Never paste findings into the progress file.
3. **Read narrowly.** Open `docs/re/README.md`, then only the file for the topic you work on. Do not read all of `docs/re/`.
4. **Append to the matching file** under a dated `##` heading. Mark corrections of earlier entries explicitly in the new entry. Create a new topic file and an index line when no file fits.
5. **Record unknowns** at the end of the entry, next to the finding they belong to.
6. `docs/re/` is written for developers, so the plain-language rule for user docs does not apply. It contains no game assets, only findings.

## Constraints

- Never modify the `.cci` image. Never stage or commit it, or anything game-derived (`*.cci`, `*.3ds`, `artifacts/`, `build/` are gitignored).
- Record what is unknown instead of inventing game behaviour. Don't size native pointers from ARM addresses. Never describe the lab as a playable port.

### Graphify — Codebase Knowledge Graph

**Initialization**
1. At the start of work, check whether Graphify is installed, integrated with Claude Code, and initialized and usable for this repository. Reuse existing working configuration; do not repeat completed setup unnecessarily.
2. If Graphify is missing or not configured, follow the installed Graphify skill or official documentation to install and configure it. If it is installed but not initialized for this repository, initialize it, build the initial graph, and verify that graph queries work.
3. If installation, configuration, or initialization fails, investigate the cause and report the specific blocker. Do not invent commands, assume setup succeeded, or claim Graphify is working when it is not.
4. Follow the documented Graphify workflow and configure `.graphifyignore` to respect `.gitignore` and exclude irrelevant generated files.
5. For routine tasks, skip Graphify queries when they offer no meaningful benefit. Maintain or refresh the graph as required by the documented workflow.

**Usage and Maintenance**
6. For architectural questions, dependency tracing, code-flow analysis, and unfamiliar code paths, query Graphify first, then verify relevant findings against source code.
7. If the graph is stale or incomplete, update or rebuild it using the documented workflow. Use direct source inspection as a fallback when Graphify cannot answer the question.
8. After meaningful source changes, update the graph as required by the installed workflow. Verify success before claiming it is current.
9. Configure documented Git hooks when appropriate, and keep shared graph artifacts under version control only if the project maintains them as team files.
10. Use the active Claude Code session for semantic extraction of supported non-code content when the installed workflow supports it. Do not require external API credentials or transmit repository content to an external provider without authorization.
11. Follow Graphify's documented semantic-extraction workflow, verify that extracted information was incorporated successfully, and report any files or steps that could not be processed.
12. Never confuse successful local code parsing with successful semantic extraction when both are required.

**Graph Scope and Exclusions**

1. Exclude `docs/` entirely from Graphify indexing and semantic extraction by default. Documentation is not part of the code knowledge graph unless a specific task requires it.
2. Configure `.graphifyignore` according to the installed Graphify documentation. Verify that the exclusion actually works; do not assume that adding a pattern guarantees every Graphify pipeline respects it.
3. Exclude build outputs, generated artifacts, extracted game assets, temporary files, dependency directories, and other irrelevant content. Respect existing `.gitignore` rules.
4. Include source code, tests, build configuration, and relevant project configuration files needed to understand code behavior and dependencies.
5. Only inspect or semantically extract files under `docs/` when the task explicitly requires their contents or the user asks for documentation work. Read only the relevant files rather than scanning the whole directory.
6. Do not run semantic extraction over excluded documentation merely to complete graph initialization. If Graphify requires a separate extraction workflow, configure its scope to follow these exclusions.
7. When a task requires documentation context, inspect the specific relevant files directly without automatically adding the entire directory to the graph.
8. Report any Graphify limitation that prevents the requested exclusions from being enforced.

## User-Specified Workflows

### Git and Commits

1. **Commit at logical steps.** Create small, focused commits when a meaningful unit of work is complete and verified. Do not combine unrelated changes into one commit. Before committing, review the diff and ensure no unintended files are included.
2. **Never push without explicit user authorization.** Local commits are allowed, but pushing to any remote requires the user's explicit permission. Do not assume permission to push from previous requests.
3. **Keep `.gitignore` up to date.** Add appropriate ignore rules whenever new generated files, local configuration, temporary files, build outputs, or other non-versioned artifacts are introduced. Never ignore files merely to conceal unintended changes.

### Game Assets and Legal Compliance

4. **Never commit original game assets or proprietary game content.** Extract or inspect assets only from a game copy supplied or explicitly authorized by the user. Keep extracted assets outside version control unless their inclusion is explicitly authorized and legally appropriate.
   - Inspect staged changes before every commit to ensure original game assets, proprietary content, and unintended extracted files are not included.
   - Add appropriate patterns to `.gitignore` to prevent accidental inclusion of these assets.
   - If prohibited game assets are found in the working tree or staged changes, remove them from the pending commit and working tree as appropriate.
   - If such assets already exist in repository history, report the issue and propose a safe remediation. Do not rewrite shared Git history or delete potentially important user data without authorization.
   - Prefer committing scripts, metadata, tools, and other original project code that operate on the user's supplied game copy rather than redistributing the game content itself.

### User-Facing Documentation

5. **Keep `docs/` accurate and up to date.** Documentation in `docs/` is written for non-technical users. Whenever a change affects user-visible behavior, installation, setup, configuration, controls, workflows, or troubleshooting, update the relevant documentation in the same logical step.
   - Write in plain language with short sentences and short paragraphs.
   - Prefer concise steps and lists over long blocks of text.
   - Document how to use the current version of the software, not its development history.
   - Remove obsolete instructions and superseded information when they are no longer relevant.
   - Do not delete useful information that is still needed to understand or use the current software.
   - Avoid duplicating the same instructions across multiple documents. Link to the relevant documentation instead.

### Code Comments

6. **Do not add explanatory code comments.** Write self-explanatory code using clear names, straightforward control flow, and appropriate abstractions. Put user-facing usage instructions in `docs/`.
   - Do not remove license notices, copyright notices, required attribution, tool directives, compiler or linter directives, type-checker directives, or other comments required for correct operation or legal compliance.
   - Preserve comments that document non-obvious correctness constraints, security requirements, or unavoidable implementation details when removing them would make the code less safe or maintainable.
   - When encountering an ordinary explanatory comment, prefer improving the code itself rather than retaining the comment.

### README

7. **Keep `README.md` minimal.** The README should provide only a concise project overview, essential setup instructions, and links to relevant documentation under `docs/`.
   - Move detailed user guides, feature instructions, troubleshooting, and other extended explanations into appropriate files under `docs/`.
   - Remove redundant or overly detailed README content once the equivalent documentation exists and the relevant links are in place.
   - Ensure the README retains the information necessary to identify the project and get started.

### File Editing Tools

8. **Prefer native file tools for editing.** Use the available file-reading and file-editing tools for inspecting and modifying source files, configuration, documentation, and other text files.
   - Do not use Python merely as a substitute for straightforward file reads, writes, or text edits.
   - Use Python or other scripting tools when they provide a meaningful advantage for structured transformations, bulk operations, data processing, testing, or file formats that native tools cannot handle efficiently.
   - Review the resulting diff after edits and verify that the changes are correct.
   
   
### Subagent Model Selection and Cost Control

1. Use the least expensive available model that can reliably complete each delegated task. Prefer Haiku-class models for bounded, low-risk work such as documentation summarization, non-code semantic extraction, routine repository exploration, and structured reporting.
2. Reserve the main session's stronger model for complex reasoning, reverse engineering, implementation design, security-sensitive changes, difficult debugging, and final review when deeper reasoning is needed.
3. Keep subagent tasks narrow and independent. Provide only the relevant files, required output format, and applicable constraints. Avoid repeatedly sending large repository context to multiple agents.
4. For Graphify semantic extraction, prefer supported Claude subagents using a cost-effective model. Require structured outputs with source references, and validate merged results against Graphify's documented format.
5. Do not delegate correctness-critical verification entirely to a cheaper model. Use deterministic checks, existing tests, checksums, and direct source inspection wherever possible.
6. Do not spawn subagents when the overhead, duplicated context, or likely rework outweighs the benefit. Prefer local scripts for deterministic parsing and transformations.
7. If the preferred model is unavailable, use another model supported by the active Claude Code session. Do not require external provider credentials or transmit repository contents to an external service without authorization.
8. Follow the installed Claude Code and Graphify documentation for model selection and subagent configuration. Do not assume unsupported model aliases or configuration options.

