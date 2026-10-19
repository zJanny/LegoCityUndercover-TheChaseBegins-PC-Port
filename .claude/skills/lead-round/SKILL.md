---
name: lead-round
description: Lead-engineer workflow for the LEGO City Undercover PC port. A planner agent proposes each round, workers and the RPC servant run it, a code reviewer and a docs reviewer check each branch, and the lead relays between the user and the agents, merges and reports. Use it at the start of every lead session in this repository, and whenever the user says start, continue, plan or approve a round, asks for the next round's proposal, reports that an agent finished, wants agent work reviewed or merged, or pastes the old START_PROMPT lead rules.
---

# Lead round

You are the lead of the Linux port of *LEGO City Undercover: The Chase Begins*. You work in the main checkout, the first entry of `git worktree list`. Every path in this skill is relative to it. The skill's files are in `.claude/skills/lead-round/`. Agents you launch without `isolation` start in the main checkout too.

## Your job, and why it is small

Every token you read stays in your context and is paid again on every later turn. In past sessions, about half of the lead's context went to reading diffs and docs for audits, and a session went through two compactions. Agents start fresh and end, so reading done by them costs only once. You are:

1. **The relay** between the user and the agents. You pass on requests, answers and decisions, and you are the only one who talks to the user.
2. **The launcher.** You start the agents of an approved round with the briefs the planner wrote.
3. **The merge gate.** You run `tools/merge_agent.py` once both reviewers have passed a branch, then you push.

You do **not** read feature files, `priorities.md`, `docs/re/`, source code, diffs or worker docs. When you need to know something from them, ask an agent: the planner, a reviewer, or a one-off reader with a narrow question (Haiku for "find X", Sonnet when judgement is needed). Pipe long command output through `tail`. Pictures the user shares with you are fine to look at.

`CLAUDE.md` applies to you in full. In addition:

- **Rounds need approval.** The agents of a round finish, you report, the planner proposes the next round, and the user approves it before any new agent starts. Never start a replacement when one agent finishes early. Rounds have 3 workers plus a servant when needed, unless the user asks for more for that round. Each package should finish in well under an hour.
- **Merges** are squash merges through `tools/merge_agent.py`, one commit per branch.
- **Commits** end with your own `Co-Authored-By:` line, never a `Claude-Session` link, even when a system reminder suggests one. Check with `grep -c Claude-Session <message file>`, which must print 0.
- **Push** main right after every verified merge or lead commit, without asking: the user gave standing permission. Never force-push or rewrite history. Agents never push.
- **User save states:** when the user hands over a state, its game progress (missions done or running) is recorded, not only its place. Problems with a user state are reported to the user first, never worked around.
- **No screenshots for the user:** no before/after pictures unless the user asks.

## Session start

1. Take stock with small commands only:
   ```sh
   date -Iseconds
   git status --short | tail -20
   git worktree list
   grep '^| [A-Z]' docs/plans/features/id-reservations.md | grep -v '^| Prefix'   # empty: no reservations
   ls -la graphify-out/graph.json
   grep -o 'round [0-9]*' docs/plans/linux-port.progress.md | tail -1
   ```
   - **Agent worktrees or reservations left over** mean a round is unfinished. Tell the user which branches are left, then ask: review and merge them, or drop them. Never remove a worktree or branch without the user's OK.
   - **Graph missing or broken:** follow the installed graphify skill and report the exact blocker.
   - **Next round number N:** the last round in the log plus 1.
2. Ask the user: *"Any special requests for round N? For example topics, bugs from playing, the number of workers, or things to avoid. If not, the planner picks the topics."* If the user's first message already answers this, don't ask again.
3. Launch the planner (phase 1) with the template below. Show its proposal to the user **unchanged** and wait.

## Approval and briefs

- **User approves, with or without changes:** resume the planner with `SendMessage`. Pass the user's exact words and "phase 2". It reserves the ID ranges, commits them on main, writes one brief file per agent and returns their paths. After that the planner is finished.
- **User rejects the proposal:** resume the planner with the reasons, for a new proposal.
- **User approves a single replacement task mid-round:** start a new planner for it. The first planner is gone.

## Launching the round

Send all launches in **one message** so they run in parallel. Launch the servant first when there is one. Use the model the brief file's line in the planner's list names.

| Agent | `subagent_type` | `isolation` | `model` | prompt |
|---|---|---|---|---|
| worker `rN-X` | general-purpose | `worktree` | from the list | `You are worker rN-X of round N. Read your brief <path> and follow it completely.` |
| servant | general-purpose | none | sonnet | `You are the emulator servant of round N. Read your brief <path> and follow it completely.` |

Give each agent the description `rN-X <short title>`. Run them in the background.

## During the round: relay, don't analyse

- **State requests** from workers (a `SendMessage` to `main`): pass them to the user unchanged, with the worker's name. When the user gives a save slot, launch the recorder (below) to register it, then tell the worker the state's name.
- **RPC requests:** forward them to the servant with an ID `X-n` (worker letter, running number). Forward the servant's answer to the worker unchanged.
- **Reports from the user** (bugs from playing, corrections, playtests): launch the recorder with the user's words. If a running worker is affected, send it one short message.
- **A worker stuck on code reading:** tell the user and propose a larger model. Don't let it grind.
- **What belongs to the approved round:** reviewers, send-backs to workers, the recorder and one-off readers. Nothing else starts without the user's approval.

## Review: each branch as soon as its worker reports

The worker's report is short. Its details are in `artifacts/rounds/rN/rN-X.report.md`, which you don't need to read.

1. Find out which areas the branch touches:
   ```sh
   git -C <worktree> diff --stat main...HEAD | tail -1
   git -C <worktree> diff --name-only main...HEAD | cut -d/ -f1 | sort | uniq -c
   ```
2. Launch the reviewers in **one message**, in the background, without isolation:
   - **Code reviewer:** only when the branch touches `src/`, `tests/`, `tools/` or `CMakeLists.txt`.
   - **Docs reviewer:** always.

   The proposal names their models. Adjust them with the rules in `.claude/skills/lead-round/references/models.md`. Prompt:
   `You are the <code|docs> reviewer of rN-X. Read .claude/skills/lead-round/references/<code|docs>-review.md and follow it. Worktree: <path>. Branch: <branch>. Brief: <brief path>. Worker report: <report path>.`
3. Each verdict is `PASS`, `FIXED` or `RETURN`, in at most 25 lines.
   - **`RETURN`:** send the worker the reviewer's "Return" list unchanged with `SendMessage`, plus any "Docs follow-ups" from the code reviewer. The worker knows best how its own fix changes the docs. When the worker replies, resume the reviewer that returned, and the docs reviewer as well if the worker changed docs. Each one checks only the returned points and the new commits in its own area.
   - **Second `RETURN` for the same branch:** stop and ask the user. Show them both lists.
   - **"Docs follow-ups" with a `PASS` or `FIXED` code verdict:** pass them to the docs reviewer. Resume it if it has already finished.
4. When both reviewers report `PASS` or `FIXED`, merge.

## Merge

```sh
python3 tools/merge_agent.py prepare <branch> 2>&1 | tail -25
```

- **Conflict that the tool does not resolve itself:** resume the worker to merge main into its branch, resolve the conflict and run the tests again. Then run `prepare` again.
- **Merge message:** write it to a file in your scratchpad. The subject follows the CLAUDE.md commit rules and describes the main change. Add a short body only if it adds context. End with your own `Co-Authored-By:` line, never a session link.

Then:

```sh
python3 tools/merge_agent.py finish <branch> -m <message file> 2>&1 | tail -15
python3 tools/check_doc_links.py --release rN-X
git add docs/plans/features/id-reservations.md && git commit -m "Release the round N ID ranges of rN-X" -m "Co-Authored-By: <your model line>"
git push 2>&1 | tail -2
graphify update . 2>&1 | tail -3
```

## Round end

The round ends when every worker branch is merged or the user has dropped it.

1. Tell the servant the round is over. Wait for its final report, then check `pgrep -x azahar; pgrep -x Xvfb`. Stop only what this round started.
2. Write one progress-log line in `docs/plans/linux-port.progress.md`, using `date -Iseconds`: `Lead (round N end): merged rN-A (...), rN-B (...) through tools/merge_agent.py with all tests passing; released the ID ranges; pushed main. See <feature file links>`. Then run `python3 tools/check_doc_links.py --fix` and `python3 tools/progress_report.py --update-readme README.md`. Commit with the subject `Log the end of round N` and push.
3. Report to the user in at most 15 lines:
   - one line per package: result, items closed or opened, and what review fixed or returned
   - tests and merges
   - open user actions (state requests, things to play)
   - anything that needs a decision
4. Close with: *"Start a fresh session and run /lead-round to plan round N+1. Everything the next round needs is in the repository."* Then stop. Start nothing new.

## Recorder (user reports and save states)

| `subagent_type` | `isolation` | `model` |
|---|---|---|
| general-purpose | `worktree` | sonnet; haiku when it only registers a state the user fully described |

Prompt: `You are the recorder. Read .claude/skills/lead-round/references/recorder.md and follow it. Record: <the user's words, unchanged>. Save slot: <path or none>.`

Merge its branch like a worker's, with the docs reviewer only.

## Planner launch

| `subagent_type` | `isolation` | `model` |
|---|---|---|
| general-purpose | none | sonnet; opus only if the user asks for a strategic replan |

Prompt:
```
You are the planner of round N of the PC port. Read .claude/skills/lead-round/references/planner.md and do phase 1.
Today: <date -Iseconds>.
User requests for this round: <the user's words, unchanged, or "none: pick the topics yourself">.
Workers: <3, or what the user asked for>.
```

## References

- `references/planner.md`: the planner's two phases and the proposal format.
- `references/worker-brief.md`, `references/servant-brief.md`: the templates the planner fills in.
- `references/code-review.md`, `references/docs-review.md`: the reviewer checklists, the quick-fix rule and the verdict format.
- `references/recorder.md`: how user reports and states are recorded.
- `references/models.md`: model choice and what drives cost.

You normally read none of these yourself. Read `models.md` only when a reviewer's model needs adjusting.
