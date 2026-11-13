# Contributing

This page is for people who want to change the code or the docs of this project. It covers setup, tests, continuous integration, commit messages and documentation. Start with the [README](README.md) if you only want to run the game.

## Not affiliated

This is an unofficial fan project. It is not affiliated with, endorsed by or sponsored by Nintendo or any other right holder of *LEGO City Undercover: The Chase Begins*. All names and trademarks belong to their owners.

## Legal and clean-room rules

### License of contributions

- The project is licensed under the [GNU General Public License, version 3 or later](LICENSE) (GPL-3.0-or-later).
- Your contribution is licensed under the same terms.
- Never submit game assets, screenshots, saves, traces or memory dumps from the original game.
- Never submit code copied from another project with an incompatible license.
- Third-party files live under `third_party/` with their licence text next to them and an entry in `REUSE.toml` (source and commit). Only GPL-compatible licences are accepted.
- Do not commit your game image or anything derived from it. The folders `artifacts/` and `build/` are ignored by git for this reason.

### Clean room

The project rebuilds behaviour from your own game image and your own measurements. Contribute only work that comes from there.

- Do not use leaked source code or leaked SDKs, and do not paste text from them.
- Do not use code or documents from Nintendo or the game's developer.
- Do not paste large decompiler dumps. A short excerpt in `docs/re/` is fine. Describe findings in your own words with the address.
- Do not add game assets: no textures, models, sounds, screenshots or saves.

### Sign-off (DCO)

Every commit of an outside pull request carries a `Signed-off-by:` line. It certifies the [Developer Certificate of Origin 1.1](https://developercertificate.org/): you wrote the change or have the right to submit it under the project's licence.

1. Commit with `git commit -s`. Git adds the line from your configured name and email.
2. A GitHub noreply email address is fine.
3. The workflow `.github/workflows/dco.yml` (workflow `DCO`, job `dco`) fails a pull request when a commit has no such line.
4. To fix a branch of yours, run `git rebase --signoff main` or, for the last commit, `git commit --amend -s`. Then force-push your own branch.

The maintainer's own commits and the commits of agents the maintainer runs do not need it.

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
- Give every command-line option a test that checks its rejection of bad input.
- Name files, tests, executables and libraries by what they do.
- Never use a mission, round, agent or date in a name.
- Never use a number pair like `1_3` or a status word such as `fix`, `glue` or `temp`.
- The CTest test `file-names` (`tools/check_file_names.py`) checks the names.

## Working on Windows

### The `artifacts` link of a worktree

Each worktree needs an `artifacts` link to the main checkout. Without Developer Mode, `ln -s` in Git Bash copies the folder and Python's `os.symlink` fails. A directory junction needs no privilege. In the worktree root, in PowerShell:

```powershell
New-Item -ItemType Junction -Path artifacts -Target <main checkout>\artifacts
```

With Developer Mode on (Settings, System, For developers), a symbolic link works too. Claude Code hooks create it for agents: [Commit hooks](docs/development.md#commit-hooks).

### Graphify on PATH

Install Graphify with `pip install --user graphifyy`. Its `Scripts` folder is not on `PATH`. Add the printed folder to `PATH` and open a new shell:

```powershell
python -c "import sysconfig; print(sysconfig.get_path('scripts', 'nt_user'))"
graphify --help
```

`graphify --help` prints a list of commands. Set `PYTHONIOENCODING=utf-8`. Build the graph once in the checkout with `graphify update .`.

### Checking Linux in WSL

Install WSL with `wsl --install` as administrator. Commit first, then run the checks on a clone inside WSL. Checks: [Linux stays green](docs/rules/windows.md#linux-stays-green).

```sh
git clone --branch <your branch> /mnt/c/<path to the main checkout> ~/wslcheck/<name>
```

- Never set `GIT_*` variables (such as `GIT_DIR`) in WSL. The tests run git and would change it.
- The clone has no `artifacts` folder: game-data tests skip.
- WSL needs a Linux distribution with CMake, a C++17 compiler, SDL2, libepoxy, FFmpeg development libraries, pkg-config, git and Python 3.11+.

## Your first pull request

1. Open an issue or a [discussion](https://github.com/zJanny/LegoCityUndercover-TheChaseBegins-PC-Port/discussions) first for anything bigger than a small fix. Ask questions in the discussions.
2. Fork the repository and create a branch.
3. Set up and build as above. Read the [development rules](docs/development.md).
4. Make one focused change. Sign off every commit (see "Sign-off (DCO)").
5. Run `python3 tools/check_doc_links.py --fix`, the unit tests and the CTest tests.
6. Open a pull request on [GitHub](https://github.com/zJanny/LegoCityUndercover-TheChaseBegins-PC-Port) and fill in the template.

The first pull request of a new contributor needs the maintainer's approval before CI runs. That is a GitHub setting, not a judgement.

### What the maintainer accepts

- You may move the port forward. Prove that your implementation matches the original: link the `docs/re/` evidence, or list it as a stand-in.
- Quality-of-life ideas come as issues with the quality-of-life template, not as pull requests. They are not guaranteed to be accepted.
- Windows changes follow the [Windows rules](docs/rules/windows.md) too: Linux stays green.
- Never change these files in a pull request: the progress files and their archives, `docs/plans/features/id-reservations.md`, the generated files (the progress section of `README.md`, `docs/plans/priorities.md`, `docs/progress-trend.svg`), plan gates, `docs/design/` and `.claude/`.

### Duplicate item IDs

When `doc-links` prints `duplicate-id: F-1 is already defined at <file>:<line>; IDs are never reused`, and the ID is new in your branch, another pull request merged first and took this ID. Rebase on `main`, run `python3 tools/check_doc_links.py --next PREFIX` again and rename your item.

### How pull requests are merged

The maintainer reads the diff, checks the pull request locally with `python3 tools/merge_agent.py check-pr N`, and merges it with a squash in the GitHub web UI. The squash keeps your `Signed-off-by:` lines.

For the maintainer:

- `check-pr N` fetches `refs/pull/N/head` from the GitHub remote and checks the full tree with the rules of `tools/forbidden_files.py` before any code of the pull request runs. Then it builds, runs all tests and prints `check-pr N: PASS` or `check-pr N: FAIL`. It never pushes or merges. Details: [command reference](docs/commands.md).
- After the file check, the build and tests of the pull request run with your user rights, can read `artifacts/` through the link and can reach the network. Read the diff of `tools/`, `cmake/`, `CMakeLists.txt`, `tests/*.py`, `.claude/`, `.github/` and `.forgejo/` first (`check-pr` lists them), or run it in a VM or container without keys.
- After a push to `main`, the workflow `.github/workflows/generated.yml` (workflow `Generated files`, job `regenerate`) rewrites the generated files when they differ.

### Which test enforces which rule

The table in [Development rules](docs/development.md#which-ctest-enforces-which-rule) lists every check. Pull requests also run the `dco` job, and the tests `no-game-files` and `tests-data-size` check the files you add.

## Reporting problems

- Bugs and ideas: [issues](https://github.com/zJanny/LegoCityUndercover-TheChaseBegins-PC-Port/issues) with the templates.
- Security problems and requests to remove game material: see [SECURITY.md](SECURITY.md).
- Behaviour in the community follows the [Code of Conduct](CODE_OF_CONDUCT.md).

## Continuous integration

The workflows `.forgejo/workflows/ci.yml` and `.github/workflows/ci.yml` run on every push to `main` and on every pull request.

- They build the project without warnings.
- They run the Python unit tests.
- They run the CTest tests without the labels `slow` and `needs-game-data`.
- Tests that need the game image are left out, because CI has no game data. Run them on your own machine.
- On GitHub, `.github/workflows/ci.yml` also builds and tests on Windows (job `windows-build-and-test`, MSYS2 UCRT64 with Ninja). It first checks that a clone with `core.autocrlf=true` still has LF line endings. Forgejo has no Windows job. See [Building the tools](docs/building.md#continuous-integration).

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

The full rules are in [Development rules](docs/development.md).

- Document what you change. User-visible behaviour, options, controls and setup go into the top-level files of [docs/](docs/); keep them true to the code.
- Write user docs by the [user docs guide](docs/design/user-docs-guide.md): short sentences, current state only, copy-paste commands.
- Findings about the original game go into [docs/re/](docs/re/README.md).
- Open items, stand-ins and the state of each feature go into the [feature pages](docs/plans/features/README.md).
- After you edit docs, run `python3 tools/check_doc_links.py --fix`. It checks links and the documentation rules, and the CTest test `doc-links` runs it.
- Do not write explanatory comments in the code. Use clear names instead.

## AI coding agents

Instructions for AI coding agents are in [CLAUDE.md](CLAUDE.md).

Claude Code loads the hooks in `.claude/settings.json`; they run `tools/claude_hooks.py` and do the mechanical rule checks for every agent:

- Before a commit: check the commit message, then run `check_doc_links.py --fix` and stage what it changed. On main the hook also regenerates a stale README progress section or `docs/plans/priorities.md`; in a branch it refuses them, because only `merge_agent.py` produces the generated files. Then stop the commit when the docs checks, `check_local_data.py` or staged game-derived or binary files fail (bundled fonts under `third_party/fonts/` are the one allowed binary). A commit with explicit paths is refused when the index holds other staged files, because git would leave them out.
- After an edit: report the docs and local-data findings of the edited file.
- Always: no push or `git config` change from an agent, `merge_agent.py finish` runs alone, a worker starts only from its brief, and a worktree gets its `artifacts` link.
- Shared `artifacts/` folder: no second `ln -s` that would put a stray link into it.
- Shared `artifacts/` folder: no C++ source files in it (scratch code goes to the scratchpad).
- Shared `artifacts/` folder: an agent deletes or moves nothing there, and runs no `find -delete`, except in its own scratch folders.
- When a worker stops: a round worker cannot stop before the report file named in its brief exists.

Their tests are in `tests/test_claude_hooks.py`. Contributors without Claude Code are not affected.
