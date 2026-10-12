# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

Early-stage Linux x86_64 reconstruction of *LEGO City Undercover: The Chase Begins* (Nintendo 3DS) from the decrypted `.cci` image in the repo root. Right now there are two independent pieces:

1. **Extraction pipeline** (`tools/extract_game.py`, Python 3.11+ stdlib only). It reads the NCSD/CCI image, then the NCCH partition 0, the exheader and the ExeFS, and checks every SHA256. It decompresses `.code` (backwards LZ) and wraps it in a synthetic ELF32 ARM file (`game.arm.elf`) for disassembly in REA.
2. **Native input laboratory** (`src/`, C++17 + SDL2). It tests configurable keyboard/mouse input that maps onto 3DS buttons and the 320x240 lower touch screen. It is **not** the game runtime. Gameplay, rendering, audio, assets and saves have not been recovered yet.

The plan and its progress log live in `docs/plans/linux-port.md` and `docs/plans/linux-port.progress.md`, with steps S1–S4. Read both before you resume port work. Update the progress file with the commands, results and timestamps from each session. The plan does not permit committing, pushing or merging unless the user asks.

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
build/chase-soundtest music/act_bass_break.bwav   # plays decoded sounds; SDL_AUDIODRIVER=dummy for headless

# Development model viewer (needs libepoxy; not gameplay). Offscreen screenshot for headless checks:
SDL_VIDEODRIVER=offscreen build/chase-viewer --screenshot out.bmp models/lvl_00_policestation/room1.bfnmdl --exclude sky

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
- **The native asset runtime is `chase_assets`** (`src/assets/`, namespace `chase::assets`). It has no SDL dependency. It ports the verified Python tools: FUSE archive + RefPack, the pooled reader with `PoolCursor`, and PICA texture decode. The Python tools in `tools/` are the reference. When you change a format decoder, change both, and keep `assets-real-test`'s checksums (from `convert_textures.py --checksum`) in agreement.
- **`chase_audio` is the mixer core** (`src/audio/`). It has no SDL dependency: voices with linear resampling, mono→stereo, gain and loop points, pulled as float stereo frames by `mix()`. `chase-soundtest` is a thin SDL front end, and `mixer-test` checks the mixing rules.
- `config/controls.cfg` format: `Action key|mouse Code hold|toggle`, with `#` comments. Key names are SDL names with `_` in place of spaces. Mouse codes are `Left`, `Middle`, `Right`, `X1` and `X2`. Action names are 3DS buttons, not recovered gameplay meanings.
- **Extractor safety rules:**
  - Open the image read-only.
  - Bounds-check every offset.
  - Fail on encryption or a hash mismatch.
  - Cap decompressed output at `MAX_CODE`.
  - Never overwrite an existing output directory.

  `manifest.json` records offsets, hashes and ARM segment addresses as evidence. The ELF is a derived analysis wrapper: its entry point is inferred from the text base, and it is not an original binary.
- REA cannot open the `.cci` directly. Analyse `artifacts/<dir>/game.arm.elf` instead, and record REA evidence IDs in the progress file.

## Constraints

- Never modify the `.cci` image. Never stage or commit it, or anything game-derived (`*.cci`, `*.3ds`, `artifacts/`, `build/` are gitignored).
- Record what is unknown instead of inventing game behaviour. Don't size native pointers from ARM addresses. Never describe the lab as a playable port.

### Graphify — Codebase Knowledge Graph

**Initialization**
1. If Graphify is not already configured for this repository, verify that the Graphify CLI and Claude Code integration are installed. Initialize the project, build the initial graph, and verify that it can be queried successfully. Do not silently skip setup if Graphify is unavailable; report the issue.
2. Follow the installed Graphify skill and its documented commands. Do not invent CLI commands or assume generated files exist.
3. Configure appropriate exclusions using `.graphifyignore`, respecting the repository's `.gitignore`.

**Codebase Exploration**
4. For questions about architecture, code flow, dependencies, symbol relationships, or how features work, query Graphify first. Use its graph report and relevant graph queries to identify important nodes and relationships before inspecting source files.
5. Use targeted source-code searches and read the relevant files to verify details, especially when investigating exact implementations, string occurrences, configuration values, or behavior that the graph cannot establish.
6. If Graphify is missing, stale, incomplete, or fails to answer a question, attempt to update or rebuild it as appropriate. If that fails, fall back to direct source-code inspection and clearly report any remaining limitations. Never treat inferred graph relationships as confirmed facts without verification.

**Keeping the Graph Current**
7. After modifying source code, regenerate or incrementally update the graph using the documented Graphify workflow before relying on it for subsequent architectural analysis. Avoid unnecessary full rebuilds when an incremental update is supported.
8. Ensure the graph is up to date before committing changes. Configure Graphify's documented Git hooks where appropriate so commits and branch switches keep the graph synchronized. Update the graph after pulls and merges when required.
9. Verify that graph updates succeed. Do not claim the graph is current if an update failed or has not completed.
10. Follow the repository's existing version-control policy. Commit the shared graph artifacts if the project uses them as team-maintained files; exclude local caches, temporary files, and machine-specific artifacts.

**Graph Scope and Exclusions**
11. Exclude irrelevant directories from the graph, such as `docs/` when documentation is not needed for code exploration, along with dependency directories, build outputs, generated artifacts, and temporary files where appropriate.
12. Do not exclude source code, tests, configuration, migrations, or other files needed to understand application behavior and dependencies without a specific reason.
13. Revisit exclusions when the project structure or investigation requirements change.

**Semantic Extraction — Use the Active Claude Session**
14. Always perform semantic extraction of supported non-code files (documentation, PDFs, images, and other supported content) using the active Claude Code session whenever possible. Use Claude's existing model capabilities and subagents rather than requiring a separate LLM provider API key.
15. Never request, require, or wait for `ANTHROPIC_API_KEY`, `OPENAI_API_KEY`, `GEMINI_API_KEY`, or another provider API key when the active Claude session can perform the extraction itself.
16. Follow Graphify's installed extraction workflow. For semantic extraction, inspect the installed Graphify skill and dispatch the required extraction tasks to Claude subagents when instructed by that workflow. Merge the extracted entities and relationships into Graphify's expected output format and complete the graph-building process.
17. Do not confuse Graphify's deterministic, local code parsing with semantic extraction. Code structure should be extracted locally without an LLM. Use the active Claude session for semantic extraction of non-code content when required.
18. Do not silently skip semantic extraction or produce an AST-only graph when semantic extraction is required and the active Claude session can perform it.
19. If the installed Graphify version or execution environment cannot use the active Claude session for semantic extraction, investigate supported alternatives, such as the documented Claude CLI backend or a local model backend. Do not install an alternative, require credentials, or send repository contents to an external provider without authorization.
20. Verify that semantic extraction completed successfully and that its results were incorporated into the graph. Report any files or extraction steps that could not be processed.

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

