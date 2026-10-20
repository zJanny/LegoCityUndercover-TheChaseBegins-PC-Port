# Rules of the `windows-port` plan

Every agent that works for the `windows-port` plan reads this file in full after [CLAUDE.md](../../CLAUDE.md). Plan index and gate: [plans/README.md](../plans/README.md).

**Gate: closed.** Only the user opens it ("only when the user says so", CLAUDE.md "Plans and order of work"). Until then no Windows work of any kind: no builds, infrastructure, abstractions or preparation, in no plan. When the user opens it, the plan is created with [new-plan.md](../../.claude/skills/lead-round/references/new-plan.md), which also fills in the "To be decided" section below with the user.

## Where the work happens

- Windows work runs on the user's Windows partition, in its own checkout with its own Claude Code session (lead and agents). The Linux checkout stays the place of the port.
- Builds and tests for Windows run on that machine. A change counts as verified for Windows only when it was built and tested there; say "not verified on Windows" otherwise.
- The Graphify graph is per checkout: on Windows it is built and queried in the Windows main checkout.

## Behaviour

- The Windows build behaves exactly like the Linux build. It has no game behaviour of its own; the port decides what the game does (`docs/rules/port.md`). A difference between the two builds is a Windows bug, unless the Linux build is wrong compared with the original, which is a port item.
- Design decisions (toolchain, platform interfaces, packaging) come from specs the user approved in `docs/design/`. What no spec decides is asked of the user, never chosen.
- The same tests give the same results on Windows. A test that cannot run on Windows is skipped with a reason in its CTest properties and listed in the feature file.

## Code

- Platform code lives in files of its own per platform behind a plain interface, as `chase_render` keeps OpenGL; no operating-system checks spread through game or runtime code.
- The architecture rules of CLAUDE.md apply unchanged (no SDL in the core libraries, strict input, C++ as the source of truth).

## Linux stays green

- A Windows branch that touches shared code (anything outside the platform files of Windows) also passes the Linux build and all Linux tests before it is merged.
- The Linux check runs in WSL on the Windows machine, called from the agent's shell through `wsl.exe`, in the same checkout, into its own build folder `build-wsl` (never the Windows build folder): configure, build warning-free, `ctest --test-dir build-wsl --output-on-failure` and the Python unittests, as in the CLAUDE.md "Commands".
- The checkout keeps LF line endings (`core.autocrlf=false`), so both builds read the same files.
- Report the result as `Linux green (WSL)`, and say whether real-data tests were skipped because the extracted game data was not reachable from WSL.
- **WSL missing or wrong: warn and stop.** When `wsl.exe` is missing, has no Linux distribution, lacks a build dependency (cmake, compiler, SDL2, epoxy, Python 3.11+), cannot reach the checkout, or the check fails to start for any other reason than a failing test, stop and warn the user through the lead with the exact command and its output. Never work around it: no other machine, no skipped tests, no merge without the check, no installing or reconfiguring WSL without the user's OK.

## Briefs and review

- **Every Windows brief also names:** the spec entries in `docs/design/` it implements, and that the worker verifies on Windows.
- **Review checks beyond the tests:** game behaviour unchanged against Linux; platform code only in its own files; `Linux green (WSL)` reported for shared code; every choice backed by a spec entry.

## To be decided when the gate opens

Toolchain and compiler, how dependencies (SDL2, epoxy) are installed, the Windows build and test commands (then also in `docs/commands.md` and `docs/building.md`), packaging, and the ID prefixes. Until decided, these are open questions, not defaults.
