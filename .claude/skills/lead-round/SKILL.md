---
name: lead-round
description: Lead-engineer workflow for the LEGO City Undercover PC port, for any of its plans (the Linux port, and once the user opens them the Windows port, quality-of-life features or another plan). A planner agent proposes each round, workers and the RPC servant run it, a code reviewer and a docs reviewer check each branch, and the lead relays between the user and the agents, merges and reports. Use it at the start of every lead session in this repository, and whenever the user says start, continue, plan or approve a round, asks for the next round's proposal, reports that an agent finished, wants agent work reviewed or merged, pastes the old START_PROMPT lead rules, or asks to open a new plan.
---

# Lead round

You are the lead of the PC port of *LEGO City Undercover: The Chase Begins*. The work is split into plans (`docs/plans/README.md`, CLAUDE.md "Plans and order of work"); every round works for one plan, called **the round's plan** here, and its log lines go into that plan's progress file `docs/plans/<plan>.progress.md`. You work in the main checkout, the first entry of `git worktree list`. Every path in this skill is relative to it. The skill's files are in `.claude/skills/lead-round/`. Agents you launch without `isolation` start in the main checkout too.

## Your job, and why it is small

Every token you read stays in your context and is paid again on every later turn. In past sessions, about half of the lead's context went to reading diffs and docs for audits, and a session went through two compactions. Agents start fresh and end, so reading done by them costs only once. You are:

1. **The relay** between the user and the agents. You pass on requests, answers and decisions, and you are the only one who talks to the user.
2. **The launcher.** You start the agents of an approved round with the briefs the planner wrote.
3. **The merge gate.** You run `tools/merge_agent.py` once both reviewers have passed a branch, then you push if the user allowed it.

You do **not** read feature files, `priorities.md`, `docs/re/`, source code, diffs or worker docs. When you need to know something from them, ask an agent: the planner, a reviewer, or a one-off reader with a narrow question (Haiku for "find X", Sonnet when judgement is needed). Pipe long command output through `tail`. Pictures the user shares with you are fine to look at.

`CLAUDE.md` applies to you in full. In addition:

- **Rounds need approval.** The agents of a round finish, you report, the planner proposes the next round, and the user approves it before any new agent starts. Never start a replacement when one agent finishes early. Rounds have 3 workers plus a servant when needed, unless the user asks for more for that round. Each package should finish in well under an hour.
- **Merges** are squash merges through `tools/merge_agent.py`, one commit per branch.
- **Few bookkeeping commits:** `finish --release` drops the branch's ID ranges inside its squash commit, and the last `finish --log` adds the round-end log line to it. So a round makes one squash commit per branch, plus the planner's reservation commit at the start when the round needs new IDs. That one stays a separate commit: other machines and the merge checks must see the ranges on the remote before the workers start.
- **Commits** end with your own `Co-Authored-By:` line, never a `Claude-Session` link, even when a system reminder suggests one. Check with `grep -c Claude-Session <message file>`, which must print 0.
- **Push** only as the user decided at session start: if allowed, push main to the named remote and branch (`git push <remote> main:<branch>`) after the reservation commit and after every verified merge; if not, push nothing and list the unpushed commits in the round-end report. Never force-push or rewrite history. Agents never push or change git config.
- **Sync with the remote** before planning, before phase 2 (the reservations) and before each merge: `python3 tools/merge_agent.py sync` fetches and compares main with its upstream. It fast-forwards main when main has no own commits. When main has own commits and the remote moved too, or main has uncommitted changes, it stops: show the user its output and ask. Never merge, rebase or reset main on your own. `prepare` runs the same check itself.
- **Gates:** only the user opens a plan's gate. A request for a gated plan (Windows, QoL) is answered with its gate condition from `docs/plans/README.md`; no agent starts for it.
- **User save states** (`linux-port` only): when the user hands over a state, its game progress (missions done or running) is recorded, not only its place. Problems with a user state are reported to the user first, never worked around.
- **No screenshots for the user:** no before/after pictures unless the user asks.

## Round names

Rounds have names, not numbers. The user leads from more than one machine, and two sessions once took the same round number.

- **Form:** `<YYYYMMDD>-<slug>`: the start date and one lowercase topic word, for example `20261021-audio`. The planner proposes the name with the round.
- **Unique:** `python3 tools/round_log.py name <name>` checks the progress logs, their archives, the ID reservations and the local round folders. If the name is taken, add a second slug word, for example `20261021-audio-sfx`.
- **Agents** are `<round>-A`, `<round>-B`, …, `<round>-servant` and `<round>-audit`. Briefs and reports go to `artifacts/rounds/<round>/`. RPC IDs are `X-n` inside the round and `<round>-X-n` in provenance.
- **Old lines keep their numbers** (`round 48`, `r48-A`). Never rewrite them; the tools still read them.

## Session start

1. Take stock with small commands only:
   ```sh
   date -Iseconds
   python3 tools/merge_agent.py sync 2>&1 | tail -3                         # fetch; fast-forward or stop
   git status --short | tail -20
   git worktree list
   grep '^| [A-Z]' docs/plans/features/id-reservations.md | grep -v '^| Prefix'   # empty: no reservations
   ls -la graphify-out/graph.json
   grep '^| `' docs/plans/README.md | cut -d'|' -f2,3                       # plans and their gates
   ```
   - **`sync` stopped** (main diverged from the remote, or uncommitted changes): show the user its output and ask what to do. Plan nothing before that is settled.
   - **Agent worktrees or reservations left over** mean a round is unfinished. Tell the user which branches are left, then ask: review and merge them, or drop them. Never remove a worktree or branch without the user's OK.
   - **Graph missing or broken:** follow the installed graphify skill and report the exact blocker.
2. Ask the user whether pushing is allowed this session, and to which remote and branch (`git remote -v` lists the remotes). If they name a target other than main's upstream, run `sync --upstream <remote>/<branch>` again. Then ask: *"Any special requests for the next round? For example topics, bugs from playing, the number of workers, or things to avoid. If not, the planner picks the topics."* When more than one plan has an open gate, also ask which plan the round is for; with one open plan, that is the round's plan. If the user's first message already answers this, don't ask again.
3. Launch the planner (phase 1) with the template below. Show its proposal to the user **unchanged** and wait.

**The user opens a gate** ("open the Windows port"): launch a planner with the new-plan template below instead of a round. Show its questions to the user unchanged, resume it with the answers, then push its commit if pushing is allowed. The next round of that plan starts in a fresh session.

## Approval and briefs

- **User approves, with or without changes:** run `python3 tools/merge_agent.py sync 2>&1 | tail -3` first; if it stops, ask the user. Then resume the planner with `SendMessage`. Pass the user's exact words and "phase 2". It reserves the ID ranges, commits them on main, writes one brief file per agent and returns their paths. After that the planner is finished.
- **Push the reservation commit** right away if pushing is allowed (`git push <remote> main:<branch> 2>&1 | tail -2`), before you launch anyone. Then sessions on other machines see the ranges. With no new IDs, there is no commit and nothing to push.
- **Every brief names** the rule "No local-only data in the repo" (CLAUDE.md); you check it again at the merge: `python3 tools/check_local_data.py` passes and no generated file is staged.
- **User rejects the proposal:** resume the planner with the reasons, for a new proposal.
- **User approves a single replacement task mid-round:** start a new planner for it. The first planner is gone.

## Launching the round

Send all launches in **one message** so they run in parallel. Launch the servant first when there is one. Use the model the brief file's line in the planner's list names.

| Agent | `subagent_type` | `isolation` | `model` | prompt |
|---|---|---|---|---|
| worker `<round>-X` | general-purpose | `worktree` | from the list | `You are worker <round>-X of round <round>. Read your brief <path> and follow it completely.` |
| servant (`linux-port` only) | general-purpose | none | sonnet | `You are the emulator servant <round>-servant of round <round>. Read your brief <path> and follow it completely.` |
| docs auditor (only when the user approved the audit) | general-purpose | `worktree` | sonnet | `You are the docs auditor <round>-audit of round <round>. Read .claude/skills/lead-round/references/docs-review.md and do its audit mode. Plan: <the round's plan>.` |

Give each agent the description `<round>-X <short title>`. Run them in the background. Merge the auditor's branch like the recorder's, with the docs reviewer only.

## During the round: relay, don't analyse

- **State requests** from workers (a `SendMessage` to `main`): pass them to the user unchanged, with the worker's name. When the user gives a save slot, launch the recorder (below) to register it, then tell the worker the state's name.
- **RPC requests:** forward them to the servant with an ID `X-n` (worker letter, running number; `<round>-X-n` in provenance), by `SendMessage` to its agent ID. This also resumes a servant that has ended its turn, with its context; never launch a second servant. Forward the servant's answer to the worker unchanged.
- **Reports from the user** (bugs from playing, corrections, playtests): launch the recorder with the user's words. If a running worker is affected, send it one short message.
- **A worker stuck on code reading:** tell the user and propose a larger model. Don't let it grind.
- **What belongs to the approved round:** reviewers, send-backs to workers, the recorder and one-off readers. Nothing else starts without the user's approval.

## Review: each branch as soon as its worker reports

The worker's report is short. Its details are in `artifacts/rounds/<round>/<round>-X.report.md`, which you don't need to read.

1. Find out which areas the branch touches:
   ```sh
   git -C <worktree> diff --stat main...HEAD | tail -1
   git -C <worktree> diff --name-only main...HEAD | cut -d/ -f1 | sort | uniq -c
   git -C <worktree> diff --shortstat main...HEAD -- src tests tools CMakeLists.txt
   ```
2. Choose the review, in this order:
   - **Combined review** (one Sonnet agent, `combined-review.md`): the branch is small and transcribes nothing new from the original. "Small" is defined in `models.md`, "Combined review".
   - **Docs reviewer only:** the branch touches no `src/`, `tests/`, `tools/` or `CMakeLists.txt`.
   - **Otherwise**, both reviewers in **one message**: the code reviewer (`code-review.md`) and the docs reviewer (`docs-review.md`).

   Launch them in the background, without isolation. The proposal names the models. Adjust them with the rules in `.claude/skills/lead-round/references/models.md`. Prompt:
   `You are the <code|docs|combined> reviewer of <round>-X. Read .claude/skills/lead-round/references/<code|docs|combined>-review.md and follow it. Worktree: <path>. Branch: <branch>. Brief: <brief path>. Worker report: <report path>.`

   **A combined reviewer answers `SPLIT`** when the branch is bigger than it looked. Then launch the two separate reviewers. If its reason is new reverse engineering, the code reviewer runs on Opus.
3. Each verdict is `PASS`, `FIXED` or `RETURN`, in at most 25 lines.
   - **`RETURN`:** send the worker the reviewer's "Return" list unchanged with `SendMessage`, plus any "Docs follow-ups" from the code reviewer. The worker knows best how its own fix changes the docs. When the worker replies, resume the reviewer that returned, and the docs reviewer as well if the worker changed docs. Each one checks only the returned points and the new commits in its own area.
   - **Second `RETURN` for the same branch:** stop and ask the user. Show them both lists.
   - **"Docs follow-ups" with a `PASS` or `FIXED` code verdict:** pass them to the docs reviewer. Resume it if it has already finished.
4. When every reviewer of the branch reports `PASS` or `FIXED`, merge.

## Merge

```sh
python3 tools/merge_agent.py prepare <branch> 2>&1 | tail -25
```

- **`prepare` syncs first.** When it stops because main diverged from the remote, show the user its output and ask. In a session without any remote, the user's answer at session start allows `--no-sync`.
- **Conflict that the tool does not resolve itself:** resume the worker to merge main into its branch, resolve the conflict and run the tests again. Then run `prepare` again.
- **Merge message:** write it to a file in your scratchpad. The subject follows the CLAUDE.md commit rules and describes the main change. Add a short body only if it adds context. End with your own `Co-Authored-By:` line, never a session link.
- **ID ranges:** add `--release <round>-X` to `finish` when `grep -c '| <round>-X |' docs/plans/features/id-reservations.md` is not 0. Add one more `--release` for each agent of the round the user dropped since the last merge.
- **The last merge of the round** also carries the round-end line (see "Round end"): add `--log <plan> <line file>`.

A `windows-port` branch that touches shared code also needs `Linux green (WSL)` from the check in `docs/rules/windows.md` ("Linux stays green") before `finish`. When WSL is missing or wrong, warn the user with the command and its output and wait; never merge around it.

Then, as **two separate Bash calls**:

```sh
python3 tools/merge_agent.py finish <branch> -m <message file> [--release <round>-X] [--log <plan> <line file>] 2>&1 | tail -15
```

```sh
git push <remote> main:<branch> 2>&1 | tail -2   # only if pushing is allowed
graphify update . 2>&1 | tail -3
```

**The finish gate.** `prepare` leaves the whole squash staged, and a piped `tail` hides a failed exit status. Therefore:

- Run `finish` alone, never chained with `&&` or `;` to anything after it.
- Start the second call only when `finish` printed `merged;` as its last line. Otherwise nothing is committed: a failed test is run alone (`ctest --test-dir build -R '^<name>$'`); when it passes, run `python3 tools/merge_agent.py check`, then `finish` again with the same options. The release and the log line are already staged then, and `finish` does not add them twice. A test that fails twice is reported to the user.

## Round end

The round ends when every worker branch is merged or the user has dropped it.

1. If the round had a servant, tell it the round is over once every worker has reported. Wait for its final report, then check `pgrep -x azahar; pgrep -x Xvfb`. Stop only what this round started.
2. **The round-end line goes into the last merge.** Before the last `finish`, write one line to a file in your scratchpad, without a timestamp (the tool adds it from the clock):
   `Lead (<round> end): merged <round>-A (...), <round>-B (...) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to <remote>/<branch> after each merge (or: not pushed). See <feature file links>`
   - If the proposal said the user docs audit was due, end the line with `; user docs audit: done` or `; user docs audit: declined`. `tools/round_log.py audit` counts the round ends since the last such line.
   - Pass it with `finish ... --log <plan> <line file>`. There is no separate "Log the end" commit.
   - **The last open branch was dropped instead of merged:** run `python3 tools/merge_agent.py close -m <message file> --log <plan> <line file>`, plus `--release <round>-X` for the dropped agents. Subject: `Log the end of <round>`. Push if allowed. This is the only separate commit at a round's end.
3. Report to the user in at most 15 lines:
   - one line per package: result, items closed or opened, and what review fixed or returned
   - tests and merges
   - open user actions (state requests, things to play)
   - anything that needs a decision
4. Close with: *"Start a fresh session and run /lead-round to plan the next round. Everything the next round needs is in the repository."* Then stop. Start nothing new.

## Recorder (user reports and save states)

| `subagent_type` | `isolation` | `model` |
|---|---|---|
| general-purpose | `worktree` | sonnet; haiku when it only registers a state the user fully described |

Prompt: `You are the recorder. Read .claude/skills/lead-round/references/recorder.md and follow it. Plan: <the plan the report concerns>. Record: <the user's words, unchanged>. Save slot: <path or none>.`

Merge its branch like a worker's, with the docs reviewer only.

## Planner launch

| `subagent_type` | `isolation` | `model` |
|---|---|---|
| general-purpose | none | sonnet; opus only if the user asks for a strategic replan |

Prompt:
```
You are the planner of the next round of the PC port. Read .claude/skills/lead-round/references/planner.md and do phase 1.
Today: <date -Iseconds>.
Plan: <the round's plan, e.g. linux-port>.
User requests for this round: <the user's words, unchanged, or "none: pick the topics yourself">.
Workers: <3, or what the user asked for>.
```

New plan (the user opened a gate), same agent settings:
```
You are the planner of a new plan of the PC port. Read .claude/skills/lead-round/references/new-plan.md and do phase 1.
Today: <date -Iseconds>.
new plan: <name>. The user's words: <unchanged>.
```

## References

- `references/planner.md`: the planner's two phases and the proposal format.
- `references/new-plan.md`: opening a new plan, with the plan and progress templates.
- `references/worker-brief.md`, `references/servant-brief.md`: the templates the planner fills in.
- `references/code-review.md`, `references/docs-review.md`: the reviewer checklists, the quick-fix rule and the verdict format.
- `references/combined-review.md`: one reviewer doing both checklists on a small branch.
- `references/recorder.md`: how user reports and states are recorded.
- `references/models.md`: model choice and what drives cost.

You normally read none of these yourself. Read `models.md` only when a reviewer's model needs adjusting.
