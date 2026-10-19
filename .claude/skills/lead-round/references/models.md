# Model choice

Pick the cheapest model that can finish the task without grinding. Write each choice and a short reason into the proposal, so the user can overrule it. Never use `fable` unless the user asks for it.

## What drives cost

Prices from the claude-api skill, cached 2026-10-06, in dollars per million tokens:

| Model | Input | Cache read | Output |
|---|---|---|---|
| Opus 5.5 | 4.00 | 0.20 | 20.00 |
| Sonnet 5.5 | 2.00 | 0.20 | 10.00 |
| Haiku 5.5 | 0.10 | about 0.01 | 0.50 |

- **Most of what an agent costs is cache reads:** every turn reads the whole context again. Opus and Sonnet cost the same per cached token. Opus costs twice as much only on output and on new context. So **the number of turns and the context size** drive cost more than Opus versus Sonnet.
- **What this means in practice:** narrow packages, a known starting point, reading only what is needed and short reports save more than downgrading from Opus to Sonnet. An Opus worker that solves an open-ended reverse engineering problem in fewer turns can cost less than a Sonnet worker that grinds.
- **Haiku is the real saving,** roughly 20 times cheaper, but only for work that needs no judgement about reverse engineering or project rules beyond a checklist.

## Rules

| Role | Default | Use instead when |
|---|---|---|
| Planner | Sonnet | Opus only when the user asks for a strategic replan, such as reordering the whole critical path |
| Worker, mechanical | Haiku | Edits with no reverse engineering: renames, splitting a feature file, regenerating docs, a census row from data already recorded |
| Worker, known start | Sonnet | A bug with a known symptom, a class whose handlers are named, a feature shaped like one already ported, a batch of natives whose handlers are listed |
| Worker, open-ended reverse engineering | Opus | The driving class or system is unknown, or the behaviour is spread over many functions |
| Emulator servant | Sonnet | |
| Code reviewer | Sonnet | **Opus** when the branch transcribes new handlers or constants from the disassembly that must be checked against it, or when a correctness argument spans several systems. **Skip it** when the branch touches no code |
| Docs reviewer | Sonnet | **Haiku** when the docs change is bookkeeping only: status lines, closed items, a log line, at most one small `docs/re` entry, about 150 changed lines or fewer |
| Recorder | Sonnet | **Haiku** when it only registers a save state the user described completely |
| One-off reader | Haiku | **Sonnet** when the answer needs judgement, not just finding text |

The CLAUDE.md "Model choice" rule for workers is the source of this table. If the two ever disagree, CLAUDE.md wins.

## Adjusting a reviewer at review time

The lead sees only `git diff --stat` and the top-level directories a branch touches:

- **No `src/`, `tests/`, `tools/` or `CMakeLists.txt`:** no code reviewer.
- **Planner said Sonnet for the code reviewer, but the worker's short report says it read and transcribed new handlers:** Opus.
- **Docs reviewer:** keep the planner's choice unless the docs diff is far larger or smaller than the planner expected.

## Escalation

An agent stuck on code reading, or one whose turns keep repeating the same attempt, is stopped and reported to the user with a proposal for a larger model. A reviewer that cannot judge a reverse engineering claim says so in its verdict ("not verified: …") instead of guessing.
