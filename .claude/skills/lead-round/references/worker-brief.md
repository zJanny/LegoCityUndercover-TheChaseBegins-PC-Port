# Worker brief template

The planner copies everything below the line into `artifacts/rounds/<round>/<round>-X.md`, fills in every `{{…}}` (`{{round}}` is the round name, such as `20261021-audio`; `{{agent}}` is the worker, such as `20261021-audio-A`) and deletes the `[optional: …]` blocks that do not apply. The rules section stays complete: each rule in it is there because an agent once skipped it.

---

You are worker {{agent}} of round {{round}} of the PC port of *LEGO City Undercover: The Chase Begins*, working for the plan `{{plan}}`. Main checkout: `{{main}}`. You work in your own git worktree; paths in this brief are relative to your worktree root unless they start with the main checkout. The lead (`main`) relays between you and the user. When you finish, a code reviewer and a docs reviewer check your branch before it is merged.

## Rules (mandatory)

- **Read `CLAUDE.md` and the plan's rule file `{{rule file}}` completely before anything else,** and follow them throughout. Then read:
  - the Status and Next Action of `docs/plans/{{plan}}.progress.md`
  - only your area's section of `docs/plans/features/README.md`
  - the feature files you touch: {{feature files, with sizes; say which are near 16 KB and must be split first}}
  - [optional: linux-port] `docs/re/oracle.md` before any emulator use
  - [optional: other plans] the spec entries this package implements: {{docs/design links}}
- **First command, in your worktree root:** `[ -e artifacts ] || ln -s {{main}}/artifacts artifacts` (the guard matters: a pre-tool hook may have made the link already, and a second `ln -s` would create a stray link inside the user's `artifacts/`)
- **Scratch code goes to the scratchpad directory of your system prompt, never to `artifacts/`** (it is shared with the user and the lead; a hook denies `.cpp`/`.hpp` writes there). Never delete anything in the main checkout's `artifacts/` except your own `artifacts/oracle/{{agent}}-out` folders and `artifacts/scratch/`; for anything else ask `main` with `SendMessage`.
- **Graphify is your first action after reading.** The graph is only in the main checkout. Start with these queries, list every query you ran in your report, and check each answer against the source:
  {{2 to 4 lines: `graphify query "<q>" --graph {{main}}/graphify-out/graph.json`}}
- [optional: linux-port] **Faithfulness:**
  - Never invent game behaviour. Take it from the original's code (REA via the installed skill, or llvm-objdump on `artifacts/<dir>/game.arm.elf`) and from oracle measurements.
  - Anything not taken from the original is a stand-in, including gates and conditions you chose. Label it in code names or `docs/re`, and list it in the feature file.
  - Never claim REA work that did not happen.
- [optional: other plans] **Spec only:**
  - Behaviour comes only from the approved spec entries above. What they do not decide: ask `main` with `SendMessage` and continue other work; never choose it yourself.
  - With every option at its default, game behaviour stays unchanged: the port's tests stay as they are and pass.
  - [optional: qol] The feature is off by default and switched only by the user's toggle mechanism ({{toggle, with its design entry}}). If that mechanism does not exist, stop: tell `main` that the user has to define and implement it first, and do no QoL work.
  - [optional: windows-port] Build and test on the Windows machine; say "not verified on Windows" otherwise. Platform code only in its own files. For shared code, run the WSL Linux check of `docs/rules/windows.md` ("Linux stays green") and report `Linux green (WSL)`; if WSL is missing or wrong, stop and warn `main` with the command and its output, never work around it.
- **Generic over hardcoded:** one rule for all users, as the original (or the spec) does it. A special case needs evidence from the original or the spec.
- **Reserved IDs:** {{ranges}}. Use them in order from the first one, and report the unused ones. Take no other IDs.
- **Long commands:** run builds and tests in the foreground or with `run_in_background`, which reports when they exit. Never wait in a `pgrep -f` loop: the pattern matches your own shell command, so the loop never ends.
- **Code placement:** new runtime logic goes into its own file under `src/runtime/` (or the matching library), with its own CTest. `game.cpp` gets at most a few lines of glue. Your glue file: {{file}}.
  - Parallel workers: {{who edits what}}. Stay out of their files. If you need their result, read it through a small interface and mark a labelled stand-in until the merge.
- **Tests:**
  - Short only. Start at the point under test (`at X Y Z`, a level data position, recorded data). No long walks or drives, no route tuning. A test over about 5 s gets the label `slow`.
  - Record next to each pinned value where it came from: command, state, address or trace, and who measured it (`{{agent}}`, or the RPC ID `{{agent}}-n`). Rounds have names, not numbers.
  - Every new command-line option or script command gets a test for its rejection.
  - A test that needs the game image or `artifacts/` carries the label `needs-game-data` (when it skips without the data it exits with code 77 and keeps the label).
- **License:** contributions are GPL-3.0-or-later. Copy no code under another license; no game assets.
- **Git:** never change git config; never push.
- **No local-only data** (CLAUDE.md "No local-only data in the repo"): outputs go only under `artifacts/oracle/{{agent}}-out`, your emulator home or the scratchpad. Every value you use goes into the repo (test with provenance or `docs/re` entry) before the report. No local path in any committed file; tests read no local data. Provenance in comments, tests and docs names round/agent, date and command or oracle step and says "recorded from a save state"; never an output folder or scratch file name (`agent...-out`, `*.mem`, `*.png`) or the name of a save state, its slot or how it was reached (the game situation is described in game terms; only states a committed script saves itself are named). Docs name no distro, package, user name, machine timing or "this machine". Run `python3 tools/check_local_data.py` and fix what it reports for your files.
- **No explanatory comments.** No before/after screenshots for the user.
- [optional: emulator, the package needs RPC or a state]
  **Emulator:** the servant owns the only `--rpc` session.
  - Send RPC requests to `main` with `SendMessage`, saying what to load, do and read. The lead forwards them and sends you the answer.
  - Never start an emulator with `--rpc`. Without RPC you may use `tools/azahar_oracle.py --renderer opengl --display xvfb --port {{port}} --home {{home}}` as CLAUDE.md allows.
  - Never `pkill`, `killall` or `pkill -f`.
- [optional: linux-port] **Save states:** start from {{state name (game progress)}}.
  - If a user state does not load, is somewhere else or does not show what was described: stop that part and `SendMessage` to `main` with the state, the exact inputs, expected vs seen, and screenshot paths. Never work around it with teleports, pokes or long retries. Continue other work meanwhile.
  - If a game point needs real play, or about 3 scripted tries or 15 minutes fail, send `main` a state request: name, why, game progress, where to go and how, where to stand and face, what must not happen first, the save slot. Continue other work meanwhile.
- **Git:**
  - Commit small, verified units in your worktree. Subjects follow the CLAUDE.md commit rules.
  - End every message with `Co-Authored-By: {{your model line, e.g. Claude Sonnet 5.5 <noreply@anthropic.com>}}`, never a session link.
  - Never push. Never commit game-derived files or screenshots.
  - Before your final commit, merge main into your branch. Never commit the README progress section, `docs/plans/priorities.md` or `docs/progress-trend.svg`: the merge script regenerates them and the hook refuses them.

## Task: {{title}}

{{Context: which items, why, what is known and where (links to docs/re entries), what chase-game does today.}}

**Steps:**
1. {{…}}
2. {{…}}

Port as far as you get, in this order: {{ordered goals}}. Anything left becomes an open item.

## End of session

1. Do the CLAUDE.md session end:
   - `linux-port`: the dated `docs/re` entry, with unknowns last and `Current state` updated; other plans: the `docs/design` entry, a changed decision edited in place with the old version deleted
   - the feature file: status, items, stand-ins, findings summary link
   - the top-level user docs your change makes wrong, as listed in CLAUDE.md "User docs" (`docs/commands.md` for every new or changed option, program or test, `docs/controls.md`, `docs/status.md`, `docs/game-runtime.md`)
   - ONE progress-log line in `docs/plans/{{plan}}.progress.md`, starting `Agent {{agent}} (<short topic>)`
   - `python3 tools/check_doc_links.py --fix`
2. Run the full `ctest`, slow tests included (most checks of chase-game output are `slow`), and the Python unittests.
3. Write your report to `artifacts/rounds/{{round}}/{{agent}}.report.md` (through your artifacts link, so it lands in the main checkout; a hook keeps you from stopping before it exists), and give the same text as your final message, in **at most 25 lines**. Findings belong in `docs/re`, not in the report.

```
{{agent}}: DONE | PARTIAL | BLOCKED (<why, one line>)
Branch: <branch>  Worktree: <path>  Commits: <count>, last <sha>
Closed: <IDs>  Opened: <IDs>  Stand-ins: <IDs>  Unused IDs: <IDs>
Tests: ctest <passed>/<total>; unittests <ok | failures>
Graphify: <queries run, and what you used from them, one line each>
Read from the original: <addresses or handlers transcribed, or "none">
Emulator: <pgrep output | not used>
Docs: <docs/re or docs/design entry link>; <feature files>
Waiting on the user: <state requests | none>
Notes for reviewers: <at most 5 lines on anything non-obvious>
```

## If review returns your branch

You may be resumed with a list of review findings. Fix each one, commit, update your report file, and reply with one line per finding saying what you changed. If you disagree with a finding, say why in one line instead of changing it.
