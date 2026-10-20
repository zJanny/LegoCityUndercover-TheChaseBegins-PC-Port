# CLAUDE.md

Guidance for Claude Code (claude.ai/code) in this repository. Keep this file to rules every agent needs, whatever plan it works for (at most 20 KiB, checked by `doc-links`); the rules of one plan go into its rule file under `docs/rules/`, details of one subsystem into its feature file, `docs/re/` or `docs/design/`.

## Project

PC port of *LEGO City Undercover: The Chase Begins* (Nintendo 3DS), built from the user's decrypted `.cci` image in the repo root. Goal: a faithful, complete port that behaves exactly like the original and is playable from start to finish, natively on Linux x86_64 first. The project is early and **not playable yet**; never describe it as a playable game or a finished port.

1. **Extraction pipeline** (`tools/extract_game.py`, Python 3.11+ stdlib): reads the image, checks every SHA256, decompresses `.code` and wraps it in `game.arm.elf` for disassembly.
2. **Native libraries and tools** (`src/`, C++17 + SDL2): asset decoders, mixer, input lab, viewer, FMV player, and `chase-game`, the development runtime that reproduces measured original behaviour. Missions, enemies, most scripts and rendering states are not recovered yet.

Before resuming work read the plan index `docs/plans/README.md` and, for your plan, its rule file in full, the progress file's Status and Next Action (not the whole log), the feature index `docs/plans/features/README.md` (only your area) and the one feature file you work on.

## Plans and order of work

- **Plans:** the work is split into plans, one per line of work: `linux-port` (the faithful port), `windows-port`, `qol` (quality-of-life features). Each has a plan/progress pair `docs/plans/<plan>.md` / `.progress.md`, a rule file `docs/rules/<plan>.md` and its own item ID prefixes; the index `docs/plans/README.md` lists them with their gate. Every round, branch and agent works for one plan and reads its rule file.
- **Order of work (gates):** (1) the faithful Linux version; (2) only when the user says so, Windows; (3) only after the game is finished for PC, quality-of-life features. Only the user opens a gate. While a plan's gate is closed, no work of any kind for it (builds, infrastructure, abstractions, preparation, features or deviations), in no plan.
- **New plan:** only after the user opened its gate, with `.claude/skills/lead-round/references/new-plan.md`. Its templates set the minimum sections of the pair; a pair written with another planning tool counts if it has them. Never create per-feature plan/progress pairs; a new feature gets a feature file from the template and an index line.
- **Where behaviour comes from:** in `linux-port`, from the original (`docs/rules/port.md`). In every other plan, from a spec the user approved, recorded in `docs/design/`. Never invent behaviour: what neither decides is asked of the user and recorded as an open item.
- **The faithful game stays the default:** no change of another plan may alter game behaviour with its options at their defaults; the port's tests stay unchanged and pass.
- **Input mapping** (3DS controls to keyboard/mouse, stand-ins like the `Walk` key) is part of the port, not QoL, and must not change game behaviour. **Bottom-screen menus and UI are used by clicking them with the mouse** (the touch screen), not with extra keyboard keys. Input rules and their tests: `docs/plans/features/input.md`; binding format: `docs/controls.md`.

## Commands

Linux commands; a plan for another platform names its own in its rule file and in `docs/commands.md`.

```sh
python3 -m unittest discover -s tests -p 'test_*.py' -v
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo && cmake --build build -j"$(($(nproc) - 1))"
ctest --test-dir build -j"$(($(nproc) - 1))" -LE slow --output-on-failure   # while developing; -R <name> for one test
ctest --test-dir build -j"$(($(nproc) - 1))" --output-on-failure            # all tests, once before the final commit
python3 tools/extract_game.py "<image>.cci" --output artifacts/game   # output dir must NOT exist
```

All other commands and test names: `docs/commands.md`.

## Architecture rules

- No SDL in `chase_input` (`src/input.*`), `chase_audio` (`src/audio/`), `chase_save` (`src/save/`) or `chase_assets` (`src/assets/`), so their tests link without it. `readBindings` takes a key-name resolver callback.
- `chase_assets` is the source of truth for every format decoder. `chase_assets_c` (`src/assets/c_api.cpp`) is the C ABI for Python via `tools/chase_native.py`; add an entry point when a Python tool needs a decoder. Tests that use it skip when it is not built.
- **`src/game.cpp` only wires features together** (options, the order of the update, calls). New runtime logic goes into its own file under `src/runtime/` (or the matching library) with its own test; a feature adds at most a few lines of glue to `game.cpp`. CTest `game-cpp-wiring` guards its size.
- **OpenGL and shaders:** OpenGL 4.6 core. Every OpenGL call, GL type and `<epoxy/gl.h>` include lives in `src/render/` (`chase_render`); other code uses its plain-type interfaces (CTest `gl-only-in-render`). Shader code lives only in `.glsl` files under `src/render/shaders/`, embedded at build time; never in a `.cpp` or `.hpp` file (CTest `no-inline-shaders`).
- **Strict input:** every command-line option and development script (`--script`, `--front-script`, `--start`, oracle scripts) is parsed strictly. Malformed, unknown or out-of-range input stops with an error naming the option and the bad text; never fall back to a default or skip silently. Each new option gets a test for its rejection.
- **Extractor safety:** open the image read-only, bounds-check every offset, fail on encryption or hash mismatch, cap decompressed output at `MAX_CODE`, never overwrite an output directory. The ELF is a derived analysis wrapper, not an original binary.
- Portable C++17 and the standard library. Linux x86_64 first; other platforms only through their plan's rule file.

## C++ is the source of truth

1. Every format decoder and piece of runtime logic lives in C++. Fix bugs and add features there first.
2. Python is for exploration and CLI tools. Never write a second decoder for a format C++ implements; call C++ through the binding. A Python prototype may stay as an exploration aid but is not kept in sync or used by tests.
3. The converters (`tools/convert_*.py`, `extract_fib.py`) are thin tools over the binding with no decoding logic. `extract_game.py` and `extract_romfs.py` read the image and have no C++ counterpart.
4. Tests pin recorded reference values (from the original, or from the approved spec in other plans), not a live second implementation; next to each value record its provenance (command, evidence ID, oracle trace, spec entry or manual inspection).
5. Fix violations in a focused commit; if too large, add an open item in the feature file.

## Documentation rules

Four places, never copied between them, only linked: **findings** about the original in `docs/re/`, **design decisions** of the other plans (specs the user approved) in `docs/design/`, **gaps** (open items, stand-ins, feature state) in `docs/plans/features/`, **work state** in the plan's progress file. CTest `doc-links` (`tools/check_doc_links.py`) checks the mechanical rules below; run `python3 tools/check_doc_links.py --fix` after any docs edit.

**Findings and design decisions (`docs/re/<topic>.md`, `docs/design/<topic>.md`, each with a `README.md` index)** follow the same rules:
- Read the index, then the topic's `## Current state` section, then only the entry a feature file links, not whole files.
- Append dated entries at the end: anchor line `<a id="ts-YYYYMMDD-HHMMSS"></a>` directly above `## <date -Iseconds> — title`. Links to entries are `file.md#ts-...` with `§<timestamp>` in the link text; inside the same file only `#ts-...` (Forgejo ignores same-page links that repeat the file name). Never rewrite a heading timestamp. Unknowns go last in the entry.
- **Corrections are marked at both ends:** the new entry links the old one with link text starting `Corrects`, and the old entry gets a line `Corrected by [file.md §<ts>](file.md#ts-...): <what changed>` directly under its heading. Otherwise old entries stay unchanged.
- **A fully invalid entry gets a line `Superseded by [file.md §<ts>](file.md#ts-...): <why>` directly under its heading** (`Corrected by` is for partial corrections); `--fix` then moves it, with its anchor, to `<topic>.history.md` next to it and rewrites all links. Do not delete entries.
- **Every topic file has a `## Current state` section** (after the title and intro, before the first dated entry; the checker fails without it): a short summary of what holds now, with links to the entries behind it. A session that adds or changes entries of a topic creates the section if it is missing and updates it in place; dated entries stay append-only.
- **Split a file by subtopic when it passes 40 KB:** move whole entries with their anchors into `<topic>-<subtopic>.md`, leave a pointer line `Subtopics: [a.md](a.md), ...` in the old file, update the index and all links. Split the `Current state` too: each new file gets the bullets of its moved entries, the old file keeps its own bullets plus one pointer bullet per subtopic file (before the commit, the checker warns on a changed file whose `Current state` links an entry of a file on its `Subtopics:` line).
- A design entry records what the user approved (their words, date) and what is still open; never a choice the agent made alone.
- For developers; no game assets.

**Feature files (`docs/plans/features/<feature>.md`, index `README.md` there)**
- Sections: Scope, Status, Verified behaviour, Stand-ins, Open items, Findings summary, Code and tests. The index groups one row per file under `## <Area>` headings: read only the section of your area, not the whole index. Features of a plan other than `linux-port` sit under that plan's area headings.
- Items have stable IDs (`DOOR-1`, stand-ins `DOOR-S1`), never reused. Dependencies on both sides: `needs: <ID>` on the blocked item, `blocks: <ID>` on the blocking one.
- **A feature file is at most 16 KB (16384 bytes).** An item or stand-in states the gap, needs/blocks and one evidence link; addresses, lists and details go into the linked `docs/re/` or `docs/design/` entry. A larger feature is split by subtopic as `<feature>-<subtopic>.md` (standard sections in each, one index row each). `--fix` moves closed items (`- [x] ID`) to `<feature>.closed.md` (exempt from the limit, not in the index); an ID defined twice in a feature and its closed file is an error. **New IDs come from `python3 tools/check_doc_links.py --next PREFIX [COUNT]` (`PREFIX-S` for stand-ins), never by hand; parallel agents use only the range the lead gave them with `--reserve PREFIX COUNT WHO`, in order from its first ID (`--next` skips reserved ranges; the lead releases them with `--release WHO` at the merge; a new prefix needs `--new-prefix`). Item fields stay in the order gap, `needs:`, `blocks:`, `uses:`, `evidence:`.** `uses: Name, Name` (script natives or object classes) is optional and sets which game usage the item is ranked by. `docs/plans/priorities.md` is generated by `tools/priority_report.py` and regenerated at merges; never edit it.
- **Timestamps come from `date -Iseconds`, never typed or extrapolated;** the checker fails on a future time (any line of `docs/re`, `docs/design`, feature files and progress-log lines) and warns on a round `:00` second in a new heading or log line.
- End of every session: update the feature file (status, items, findings summary link), add the dated `docs/re/` or `docs/design/` entry, then ONE progress-log line in your plan's progress file (timestamp, what, how far, link to the feature file); `--fix` keeps the newest 20 log lines and moves older ones unchanged to `docs/plans/<plan>.progress-archive.md`. No findings, stand-ins or items in the log or in Next Action.

**User docs (`docs/` except `re/`, `design/`, `rules/` and `plans/`)**
- Plain language, short sentences, lists. Update them in the same step as any change to user-visible behaviour, setup, controls or workflows. Document the current version, not its history; link instead of duplicating.
- **The top-level `docs/*.md` stay true to the code,** each updated by the change that makes it wrong, in the same step: options, programs and test names in `commands.md`; keys and mouse in `controls.md`; what the player can and cannot do yet in `status.md` ("What works", "Not recovered yet"; short sentences, details stay in the linked doc); runtime behaviour in `game-runtime.md`; setup in `getting-started.md`, `building.md`, `extracting-game-data.md`. CTest `doc-links` (`tools/user_docs.py`) fails when an option string of `src/` or a `chase-*` program is named in no user doc, or when a code span or fence names an option, program or `tools/`, `src/`, `tests/`, `docs/` path that does not exist. The docs reviewer checks the rest; the lead has every tenth round audited ([lead-round](.claude/skills/lead-round/SKILL.md)).
- **`README.md` is the player's entry point:** it keeps the shortest verified path from a fresh checkout to starting the game (honest state line, requirements, numbered copy-paste steps with what to expect, basic controls, common problems), then the generated progress section and links. Technical and development details live in `docs/` and are linked from it, never deleted. Any change to setup, build, extraction, start commands or controls updates that path in the same step, and you check it by following it.

## Constraints and legal

- Never modify the `.cci`. Never stage or commit it or anything game-derived (`*.cci`, `*.3ds`, `artifacts/`, `build/` are gitignored), and no original game assets or screenshots. Inspect staged changes before every commit.
- If such content is already in history, report it and propose a remediation; never rewrite shared history or delete user data without authorization.
- **Data from another machine:** docs, logs and tests may cite local data recorded on another PC (save states, traces, `artifacts/` paths, the image, emulator homes) that does not exist here. When it is missing, stop and tell the user what is missing and where it was cited, and suggest how to get it (copy it over, record it again, or name the replacement); never recreate, fake or work around it yourself.

## Graphify

- At the start of work check that Graphify is installed and the graph exists; if broken, follow the installed skill and report the exact blocker.
- For architecture, dependency and code-flow questions query the graph first, then verify against source. Rebuild after meaningful source changes.
- The graph lives only in the main checkout (`graphify-out/` is gitignored and not in worktrees). An agent in a worktree queries it there: `graphify query "<question>" --graph <main checkout>/graphify-out/graph.json`. A missing `graphify-out/` in the worktree is no reason to skip the graph; every agent brief names this path.
- `.graphifyignore` excludes `docs/`, everything in `.gitignore`, build outputs, `artifacts/` and temp files. Send no repository content to an external provider without authorization.

## Workflows

**Git**
- Commit small, focused, verified units; review the diff first. Each agent ends its own commit messages with its own `Co-Authored-By:` line; no session links.
- Merge agent branches into main only with `tools/merge_agent.py` (`prepare`, `check`, `finish`): it squashes, refuses game-derived and binary files, regenerates the README section, builds warning-free and runs all tests. The reviewer agents review the diff and findings in between; the script never pushes.
- Never push without explicit user authorization; permission does not carry over between requests.
- Run `python3 tools/progress_report.py --update-readme README.md` before the final commit of a branch and after every merge (CTest `progress-readme`); on a conflict in that section regenerate it. Measures: `docs/progress.md`.
- Keep `.gitignore` current; never ignore files to hide unintended changes.
- Write concise, human-readable commit subjects describing the primary change, not a list of findings, addresses, IDs, or implementation details. Use imperative mood, present tense, and sentence case; omit the trailing period. Aim for 50 characters or fewer and use 72 as a soft maximum, never a reason to make the subject vague. Put supporting details in the body when useful: explain what changed, why, and any important reverse-engineering evidence. Move exhaustive findings, addresses, feature IDs, and status updates to their authoritative documentation. Use a body only when it adds context that the subject cannot convey. Each commit covers one coherent change. Before committing, read the subject alone and make sure it clearly describes the change.
- Do not use an entire session summary as a commit subject. If a commit covers multiple unrelated changes, split it into focused commits rather than listing them all in the title.

**Code and files**
- **Generic over hardcoded:** where possible, implement one general rule for all its users (as the original's shared draw, table or name lookup does), not fixed lists, names or per-object special cases; a special case needs evidence from the original or the approved spec.
- No explanatory comments; clear names and simple control flow. Keep license notices, tool directives and comments on non-obvious correctness or unavoidable constraints.
- Prefer the native Read/Edit/Write tools; scripts only for bulk or structured edits. Review the diff afterwards.
- **Short tests:** no test or development script walks or drives long distances or runs for minutes, and nobody tunes routes. Start close to the point under test (`at X Y Z`, a level data position, a replay of recorded data). A test above about 5 seconds gets the label `slow` (list at the end of `CMakeLists.txt`).
- **Log lines and output checks:** chase-game prints a line per event a test or developer checks, never per entity or per update for routine work (a spawn, a removal, a step). A `PASS_REGULAR_EXPRESSION` pins the order of lines, not their adjacency: join them with `\n.*`; a bare `\n` only where the same code prints both lines together. Most output checks are `slow`, so a worker runs the full `ctest` before its report.

**Subagents**
- Cheapest model that can do the task; narrow tasks with only the needed files and rules. The lead runs rounds with the `lead-round` skill (`.claude/skills/lead-round/`): a planner agent proposes, code and docs reviewer agents (one combined reviewer on a small branch) audit each branch; never let a worker verify its own change.
- **Model choice:** a mid-size model (such as Sonnet) for work with a known starting point: a bug with a known symptom, a class whose handlers are already named, a feature shaped like one already ported, a feature with an approved spec, and the emulator servant. The largest model (such as Opus) for open-ended work: reverse engineering where the driving class or system is unknown or behaviour is spread over many functions, or a design that spans several systems. The smallest model (such as Haiku) only for mechanical edits without reverse engineering or design. An agent stuck on code reading is reported to the user with a proposal for a larger model, not left to grind.
- **Every brief names:** CLAUDE.md, the plan and its rule file, the Graphify queries to start from (the report lists the queries run; skipping Graphify is a review finding), the reserved ID ranges, the rule file's own brief items (for the port: stand-in labelling, emulator `--port` / `--home` and `--renderer opengl`), and, when parallel workers touch the same function, a separate glue file each.
- **Review checks beyond the tests:** the rule file's review checks, and that the branch does nothing for a plan whose gate is closed.
- An agent in a git worktree first runs `ln -s <main checkout>/artifacts artifacts` in the worktree root (gitignored); without it real-data tests skip and the README update refuses to run. Every agent brief says so.

Always use current system time (`date -Iseconds`) when putting a time into a file!
