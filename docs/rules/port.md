# Rules of the `linux-port` plan

Every agent that works for the `linux-port` plan reads this file in full after [CLAUDE.md](../../CLAUDE.md). Plan: [linux-port.md](../plans/linux-port.md), work state: [linux-port.progress.md](../plans/linux-port.progress.md), plan index: [plans/README.md](../plans/README.md).

## Faithfulness

- **Behaviour comes from the original:** code (REA, objdump) and measurements (oracle). Do not "improve", round off or simplify it. Never invent game behaviour; record unknowns.
- **Stand-ins:** anything not yet taken from the original (guessed asset, measured constant instead of its source, approximation) is labelled as a stand-in in code names or `docs/re/` and listed in the feature file. Gates and conditions an agent chose are stand-ins too. Never present a stand-in as original behaviour.
- Findings go into `docs/re/` (rules: CLAUDE.md "Documentation rules").

## Reverse engineering

- **Script natives in batches:** natives are implemented one batch item at a time (SCRIPT-26..39 in `docs/plans/features/script-system-natives.md`, census table in `docs/re/script-natives-census.md`), never one by one; read each handler before transcribing it and update the census row. The census table is edited in place; a row's note is its state, item IDs and links, never findings (those go into the linked docs/re entry).
- **Game version:** all `docs/re/` addresses are for one build (Europe, `00040000000AD600`, v1.0; hashes in `docs/re/game-version.md`). With a different image, warn the user loudly and stop before applying offsets.
- **REA:** analyse `artifacts/<dir>/game.arm.elf` (REA cannot open the `.cci`) with the installed REA skill; otherwise other disassembly tools. Never claim REA analysis that did not happen. Don't size native pointers from ARM addresses.

## Runtime oracle (Azahar)

The original runs in Azahar as a measuring reference. Read `docs/re/oracle.md` before any emulator use. Fast path only:

- Run the emulator only through `tools/azahar_oracle.py`, never `azahar` directly or with your own config or saves.
- Start from save states (`--state NAME --display xvfb`), never by booting through menus. Create a missing checkpoint once (script ending with `savestate NAME`) and reuse it.
- **Hard-to-reach states:** when a game point needs real play, or about 3 scripted tries or 15 minutes fail, stop and ask the user through the lead (`SendMessage` to `main`) with: state name, why, game progress needed (missions done / not started), where to go and how, where to stand and face, what must not happen first, the save slot. Continue other work meanwhile. The lead copies the user's slot read-only to `artifacts/oracle/states/NAME.cst` with `NAME.json` (`file`, `new_3ds`, `source`) and records its game progress in the local catalogue `artifacts/oracle/states/README.md` (not committed). The user's saves are never modified.
- **Report problems with the user's states first:** when a state the user made does not load, is somewhere else or does not show what was described, stop and report to the lead (`SendMessage` to `main`: state, exact inputs, expected vs seen, screenshot paths); never work around it with teleports, pokes or long retries. Continue other work meanwhile; the lead relays the report to the user.
- **Generated material stays local** (CLAUDE.md "No local-only data in the repo"): states, traces, screenshots and homes go only under `artifacts/oracle/` (`--home`, `--output`, `--states`) and are never committed or cited by path. Write every measured value into `docs/re/` (or pin it in the test with its provenance) before you report the work. A state's game progress is recorded in the local catalogue; a finding that relies on a state says in words how to reach the equivalent game point (missions done, place), so another developer can record it.
- Explore with one session (`--keep`, probe with `--attach`), save a state at each new screen. Navigate with `wait`; use `frames`/`trace` only for per-update data.
- Use `--rpc` for screenshots, memory reads and timing (`sample`); never measure rates from stepped frames. `--rpc-input` for buttons. Only one emulator per machine may use `--rpc`; each agent has its own `--port` and `--home` under `artifacts/oracle/`.
- Stop what you start (`--stop`), check `pgrep -x azahar` / `pgrep -x Xvfb`, never kill processes you did not start.
- **Emulator servant:** whenever any worker needs `--rpc` or an oracle state (no worker holds the slot); requests go through the lead with an ID (worker and number); one scripted try per goal, then a report with what a user state would need; the first load after an emulator start may disagree with later loads (use repeated loads that agree); when its work ends it stops everything it started and reports `pgrep` output.

## Labelling tests that need game data

- A test that needs the original image or `artifacts/` (for example `artifacts/romfs`, `artifacts/game`) carries the CTest label `needs-game-data`, set where it is registered in `CMakeLists.txt`; a test that skips without that data exits with code 77 and carries the label too.
- Every other test builds its data in the test (pinned values with provenance) and must pass on a clean clone.

## Briefs and review

- **Every port brief also names:** the rule "No local-only data in the repo", stand-in labelling (gates and conditions the agent chose are stand-ins too), the emulator `--port` / `--home` and `--renderer opengl` (the software renderer crashes in the sandbox).
- **Review checks beyond the tests:** unlabelled stand-ins; a test that reaches the point under test by teleport or `--call-script` while the user plays the real path: the code reviewer runs the real path once before a blocker is reported fixed.
