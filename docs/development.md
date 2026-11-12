# Development rules

The rules that every change to the code and the docs follows, and which test checks each of them.

Back to [Contributing](../CONTRIBUTING.md).

This page is the human version of [CLAUDE.md](../CLAUDE.md), the rules file for AI coding agents. Both state the same rules. When you change one, you change the other in the same commit.

## What the project is

- The project is an early, unofficial port of *LEGO City Undercover: The Chase Begins* (Nintendo 3DS). The goal is a port that behaves exactly like the original.
- The code is C++17 with SDL2 for the programs, plus a few Python tools. Linux x86_64 comes first.
- Behaviour comes from the original game, not from your idea of how it should work. If you do not know, write the question down as an open item.
- A guess or a temporary approximation is a stand-in. Label it as one in the code and list it in the feature page.

## Architecture rules

| Rule | Checked by |
|---|---|
| The libraries `chase_input`, `chase_audio`, `chase_save` and `chase_assets` do not use SDL. Their tests link without it. | The build: the tests link without SDL. |
| Every OpenGL call, GL type and `<epoxy/gl.h>` include lives in `src/render/`. Other code uses its plain-type interfaces. | CTest `gl-only-in-render` |
| Shader code lives only in `.glsl` files under `src/render/shaders/`, never in a `.cpp` or `.hpp` file. | CTest `no-inline-shaders` |
| `src/game.cpp` only wires features together. New runtime logic goes into its own file under `src/runtime/` with its own test. | CTest `game-cpp-wiring` |
| Format decoders live in C++ (`src/assets/`). Python tools call them through `tools/chase_native.py`. | Review |
| Quality-of-life code lives only in `src/qol/`, with tests in `tests/qol/`. | Review |

- **Strict input:** every command-line option and development script is parsed strictly. Malformed, unknown or out-of-range input stops with an error that names the option and the bad text. Never fall back to a default. Each new option gets a test for its rejection.
- **Generic over hardcoded:** write one rule for all its users instead of lists, names or special cases per object. A special case needs evidence from the original.
- **Extractor safety:** the extraction tools open the image read-only, check every offset and hash, and never overwrite an output folder.

## Code style

- Write portable C++17 and use the standard library.
- Do not write explanatory comments. Use clear names and simple control flow.
- Keep licence notices, tool directives and comments on non-obvious correctness or unavoidable constraints.
- A provenance comment is allowed where a value comes from the original. Name the `docs/re/` entry and the address, not a round or agent.
- Name files, tests, executables and libraries by what they do. Do not use a mission, level, round, agent or date. Do not use a number pair like `1_3` or a status word such as `fix`, `glue`, `patch` or `temp`. CTest `file-names` checks this.

## Test rules

- Run the tests before you commit: the commands are in [Building the tools](building.md#check-that-everything-works) and the [command reference](commands.md).
- Keep tests short. A test that runs longer than about 5 seconds gets the CTest label `slow`. CTest `test-labels` checks the labels.
- A test that needs the game image or `artifacts/` gets the label `needs-game-data`. Every other test passes on a clean clone. CI has no game data.
- A test pins recorded reference values from the original, not a second live implementation. Next to each value, write where it comes from: the command, the evidence entry or the manual inspection.
- Never put local paths, scratch file names or the details of your machine into a test, a doc or the code. CTest `no-local-data` checks this.
- Never commit game assets, saves, traces or memory dumps. See [Contributing](../CONTRIBUTING.md#legal-and-clean-room-rules).
- The CTest tests `no-game-files` and `tests-data-size` check the files in the repository: the first refuses game-derived and binary files, the second warns about files over 100 KB under `tests/data/`.

## Documentation rules

Documentation lives in four places. Do not copy text between them; link instead.

| What | Where |
|---|---|
| Findings about the original game | `docs/re/` |
| Design decisions of the other plans | `docs/design/` |
| Gaps: open items, stand-ins, the state of a feature | `docs/plans/features/` |
| Work state of a plan | its progress file in `docs/plans/` |

- Run `python3 tools/check_doc_links.py --fix` after every docs edit. CTest `doc-links` runs the same checks.
- **Dated entries in `docs/re/`:** put the anchor line `<a id="ts-YYYYMMDD-HHMMSS"></a>` directly above a heading `## <date -Iseconds> — title`. Take the time from `date -Iseconds`. `--fix` adds a missing anchor.
- **`docs/re/` only grows.** Add new entries at the end. Mark a correction at both ends: the new entry links the old one with link text that starts `Corrects`, and the old entry gets a `Corrected by` line under its heading. `--fix` moves fully invalid entries to a history file.
- A file in `docs/re/` or `docs/design/` is at most 40 KB. Split it by subtopic before it grows past that.
- **Feature files** in `docs/plans/features/` have the sections Scope, Status, Verified behaviour, Stand-ins, Open items, Findings summary, and Code and tests. A file is at most 16 KB.
- **Item IDs** are stable and never reused. Get a new one with `python3 tools/check_doc_links.py --next PREFIX` (`PREFIX-S` for a stand-in). Never invent an ID by hand.
- Items depend on each other with `needs:` and `blocks:` lines. Item fields stay in the order gap, `needs:`, `blocks:`, `uses:`, `evidence:`.
- **A duplicate ID** makes `doc-links` print `duplicate-id: F-1 is already defined at <file>:<line>; IDs are never reused`. If the ID is new in your branch, another pull request merged first and took this ID. Rebase on `main`, run `python3 tools/check_doc_links.py --next PREFIX` again and rename your item.
- User docs (`README.md`, `CONTRIBUTING.md`, the top-level `docs/*.md`) follow the [user docs guide](design/user-docs-guide.md) and stay true to the code. Change them in the same step as the behaviour they describe.
- CTest `doc-links` also checks that every option and program is named in a user doc, and that every code span that looks like a path exists.

## Git and commit rules

- One commit is one focused change. Split unrelated changes.
- The subject is imperative, in sentence case, without a final period. Aim for 50 characters, 72 at most. It names the main change, not a list of findings, addresses or IDs.
- Add a body only when it gives context the subject cannot, such as what changed and why.
- Never commit game-derived files: `*.cci`, `*.3ds`, `artifacts/` and `build/` are ignored by git. Inspect what you stage.
- Never edit the generated files: the progress section of `README.md`, `docs/plans/priorities.md` and `docs/progress-trend.svg`. The maintainer's tools produce them.
- Outside pull requests need a `Signed-off-by:` line on every commit. See [Contributing](../CONTRIBUTING.md#sign-off-dco).

## Which CTest enforces which rule

| Test | Rule |
|---|---|
| `doc-links` | links, anchors, size limits and format of the docs, user docs coverage, the size of `CLAUDE.md` |
| `file-names` | names by function |
| `no-local-data` | no local paths, scratch names or machine details in tracked files |
| `gl-only-in-render` | OpenGL only in `src/render/` |
| `no-inline-shaders` | shaders only in `.glsl` files |
| `game-cpp-wiring` | `src/game.cpp` stays glue |
| `test-labels` | `slow` and `needs-game-data` labels are set |
| `no-game-files` | no game-derived or binary files in the repository |
| `tests-data-size` | a warning for a file over 100 KB under `tests/data/` |

## Plans and gates

- The work is split into plans, one per line of work, listed in the [plan index](plans/README.md). Each has a rule file in `docs/rules/`.
- The faithful Linux port comes first. Windows is refused while its gate is closed. Quality-of-life features are suggested as issues.
- The maintainer opens a gate. A pull request never opens one.
- Where a plan gets its behaviour: the port from the original, every other plan from a design the maintainer approved in `docs/design/`.
- The faithful game stays the default. No other plan may change game behaviour with its options at their defaults.

## Commit hooks

Claude Code hooks in `.claude/settings.json` run `tools/claude_hooks.py` and check the rules above before an agent commits.

- On a branch, `README.md` may be edited, except inside the section between `<!-- progress:start -->` and `<!-- progress:end -->`. Only the maintainer's merge tool writes that section.
- A commit that stages `CLAUDE.md` without `docs/development.md` is refused on every branch, including `main`.

## For the maintainer

- The maintainer runs the work in rounds with the lead-round skill in `.claude/skills/lead-round/`. Agents work in their own branches.
- Branches are merged only with `python3 tools/merge_agent.py` (`prepare`, `check`, `finish`). It builds without warnings, runs all tests and regenerates the generated files.
- Outside pull requests are merged in the GitHub web UI, squashed. Before that, check one with `python3 tools/merge_agent.py check-pr N`. Read the diff first. See [Contributing](../CONTRIBUTING.md#how-pull-requests-are-merged).
- Outside pull requests never touch the progress files and their archives, `docs/plans/features/id-reservations.md`, generated files, plan gates, `docs/design/` or `.claude/`.
