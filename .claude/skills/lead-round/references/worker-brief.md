# Worker brief template

The planner copies everything below the line into `artifacts/rounds/rN/rN-X.md`, fills in every `{{…}}` and deletes the `[optional: …]` blocks that do not apply. The rules section stays complete: each rule in it is there because an agent once skipped it.

---

You are worker {{rN-X}} of round {{N}} of the PC port of *LEGO City Undercover: The Chase Begins*, working for the plan `{{plan}}`. Main checkout: `{{main}}`. You work in your own git worktree; paths in this brief are relative to your worktree root unless they start with the main checkout. The lead (`main`) relays between you and the user. When you finish, a code reviewer and a docs reviewer check your branch before it is merged.

## Rules (mandatory)

- **Read `CLAUDE.md` and the plan's rule file `{{rule file}}` completely before anything else,** and follow them throughout. Then read:
  - the Status and Next Action of `docs/plans/{{plan}}.progress.md`
  - only your area's section of `docs/plans/features/README.md`
  - the feature files you touch: {{feature files, with sizes; say which are near 16 KB and must be split first}}
  - [optional: linux-port] `docs/re/oracle.md` before any emulator use
  - [optional: other plans] the spec entries this package implements: {{docs/design links}}
- **First command, in your worktree root:** `ln -s {{main}}/artifacts artifacts`
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
  - Record next to each pinned value where it came from: command, state, address or trace.
  - Every new command-line option or script command gets a test for its rejection.
- **No explanatory comments.** No before/after screenshots for the user.
- [optional: emulator, this worker holds the RPC slot]
  **Emulator:** you hold the only `--rpc` slot of this round. Use only `tools/azahar_oracle.py --renderer opengl --display xvfb --rpc --port {{port}} --home {{home}}`.
  - Start from save states, never by booting through menus. Use one session with `--keep` and probe with `--attach`. Use RPC for timing (`sample`); never measure rates from stepped frames.
  - The first load after an emulator start may disagree with later loads; use repeated loads that agree.
  - Never `pkill`, `killall` or `pkill -f`. Stop with `--stop` (your port and home) or `kill <PID>` of a process you started. Report `pgrep -x azahar` and `pgrep -x Xvfb` at the end.
- [optional: emulator, a servant owns RPC]
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
  - Before your final commit, merge main into your branch and run `python3 tools/progress_report.py --update-readme README.md`.

## Task: {{title}}

{{Context: which items, why, what is known and where (links to docs/re entries), what chase-game does today.}}

**Steps:**
1. {{…}}
2. {{…}}

Port as far as you get, in this order: {{ordered goals}}. Anything left becomes an open item.

## End of session

1. Do the CLAUDE.md session end:
   - the dated `docs/re` entry (`linux-port`) or `docs/design` entry (other plans), with unknowns last and `Current state` updated
   - the feature file: status, items, stand-ins, findings summary link
   - ONE progress-log line in `docs/plans/{{plan}}.progress.md`, starting `Agent {{rN-X}} (round {{N}}, …)`
   - `python3 tools/priority_report.py --update docs/plans/priorities.md`
   - `python3 tools/check_doc_links.py --fix`
2. Run the full `ctest -LE slow` and the Python unittests.
3. Write your report to `artifacts/rounds/r{{N}}/{{rN-X}}.report.md` (through your artifacts link, so it lands in the main checkout), and give the same text as your final message, in **at most 25 lines**. Findings belong in `docs/re`, not in the report.

```
{{rN-X}}: DONE | PARTIAL | BLOCKED (<why, one line>)
Branch: <branch>  Worktree: <path>  Commits: <count>, last <sha>
Closed: <IDs>  Opened: <IDs>  Stand-ins: <IDs>  Unused IDs: <IDs>
Tests: ctest -LE slow <passed>/<total>; unittests <ok | failures>
Graphify: <queries run, and what you used from them, one line each>
Read from the original: <addresses or handlers transcribed, or "none">
Emulator: <pgrep output | not used>
Docs: <docs/re or docs/design entry link>; <feature files>
Waiting on the user: <state requests | none>
Notes for reviewers: <at most 5 lines on anything non-obvious>
```

## If review returns your branch

You may be resumed with a list of review findings. Fix each one, commit, update your report file, and reply with one line per finding saying what you changed. If you disagree with a finding, say why in one line instead of changing it.
