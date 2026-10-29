# Contributing

Thank you for your interest in this project. This page is for people who want to change the code or the docs.

## Not affiliated

This is an unofficial fan project. It is not affiliated with, endorsed by or sponsored by Nintendo or any other right holder of *LEGO City Undercover: The Chase Begins*. All names and trademarks belong to their owners.

## License of contributions

- The project is licensed under the [GNU General Public License, version 3 or later](LICENSE) (GPL-3.0-or-later).
- Your contribution is licensed under the same terms.
- Never submit game assets, screenshots, saves, traces or memory dumps from the original game.
- Never submit code copied from another project with an incompatible license.
- Do not commit your game image or anything derived from it. The folders `artifacts/` and `build/` are ignored by git for this reason.

## Set up

- [Getting started](docs/getting-started.md): requirements and the first run.
- [Building the tools](docs/building.md): packages, build options and tests. If you install `ccache`, CMake uses it automatically for faster rebuilds.
- [Extracting game data](docs/extracting-game-data.md): you need your own decrypted image of the game. Some tests skip without the extracted data.
- [Command reference](docs/commands.md): all programs, options and test names.

## Build and test

The exact commands are in [Building the tools](docs/building.md#check-that-everything-works) and in the [command reference](docs/commands.md). In short, you build with CMake, run the CTest tests and run the Python unit tests.

Before you open a pull request:

- The build has no warnings.
- The unit tests and the CTest tests pass.
- Every new command-line option has a test that checks its rejection of bad input.

## Continuous integration

The workflows `.forgejo/workflows/ci.yml` and `.github/workflows/ci.yml` run on every push to `main` and on every pull request.

- They build the project without warnings.
- They run the Python unit tests.
- They run the CTest tests without the labels `slow` and `needs-game-data`.
- Tests that need the game image are left out, because CI has no game data. Run them on your own machine.

## The port must be faithful

- The goal is a port that behaves exactly like the original game.
- Behaviour comes from the original game: from its code and from measurements. Do not invent it, do not "improve" it.
- If you do not know how the original behaves, write it down as an open item. Do not guess.
- Anything that is a guess or a temporary approximation is labelled as a stand-in in the code and in the docs.

## Commit messages

- One commit is one focused change.
- The subject is short, in the imperative mood, in sentence case, without a final period. Aim for 50 characters, 72 at most.
- The subject names the main change. It is not a list of findings, addresses or IDs.
- Use the body for context: what changed and why.
- Review the diff before you commit.

## Documentation

- Document what you change. User-visible behaviour, options, controls and setup go into the top-level files of [docs/](docs/); keep them true to the code.
- Findings about the original game go into [docs/re/](docs/re/README.md).
- Open items, stand-ins and the state of each feature go into the [feature pages](docs/plans/features/README.md).
- After you edit docs, run `python3 tools/check_doc_links.py --fix`. It checks links and the documentation rules, and the CTest test `doc-links` runs it.
- Do not write explanatory comments in the code. Use clear names instead.

## AI coding agents

Instructions for AI coding agents are in [CLAUDE.md](CLAUDE.md).

Claude Code loads the hooks in `.claude/settings.json`; they run `tools/claude_hooks.py` and do the mechanical rule checks for every agent:

- Before a commit: check the commit message, then run `check_doc_links.py --fix` and stage what it changed. On main the hook also regenerates a stale README progress section or `docs/plans/priorities.md`; in a branch it refuses them, because only `merge_agent.py` produces the generated files. Then stop the commit when the docs checks, `check_local_data.py` or staged game-derived or binary files fail. A commit with explicit paths is refused when the index holds other staged files, because git would leave them out.
- After an edit: report the docs and local-data findings of the edited file.
- Always: no push or `git config` change from an agent, `merge_agent.py finish` runs alone, a worker starts only from its brief, and a worktree gets its `artifacts` link.
- Shared `artifacts/` folder: no second `ln -s` that would put a stray link into it, no C++ source files written into it (scratch code goes to the scratchpad), and no deletes, moves or `find -delete` there by an agent except its own scratch folders.
- When a worker stops: a round worker cannot stop before the report file named in its brief exists.

Their tests are in `tests/test_claude_hooks.py`. Contributors without Claude Code are not affected.
