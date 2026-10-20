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
| Worker, other plans (Windows, QoL) | Sonnet | A package with an approved spec has a known start. **Opus** for a design that spans several systems, such as a first platform layer. **Haiku** for mechanical edits |
| Emulator servant | Sonnet | |
| Code reviewer | Sonnet | **Opus** when the branch transcribes new handlers or constants from the disassembly that must be checked against it, or when a correctness argument spans several systems. **Skip it** when the branch touches no code |
| Combined reviewer | Sonnet | Replaces both reviewers on a small branch (see "Combined review" below). Never Haiku: it judges code |
| Docs reviewer | Sonnet | **Haiku** when the docs change is bookkeeping only: status lines, closed items, a log line, at most one small `docs/re` entry, about 150 changed lines or fewer |
| Recorder | Sonnet | **Haiku** when it only registers a save state the user described completely |
| One-off reader | Haiku | **Sonnet** when the answer needs judgement, not just finding text |

The CLAUDE.md "Model choice" rule for workers is the source of this table. If the two ever disagree, CLAUDE.md wins.

## Combined review

Each reviewer pays for reading `CLAUDE.md`, the brief, the report and the diff, which is about the same on a small branch as on a big one. One combined reviewer saves one of those reads, roughly $0.80 per branch. Use it when **all** of these hold:

- **Small:** at most about 250 changed lines in total (insertions plus deletions), with at most about 80 of them in `src/`, `tests/`, `tools/` and `CMakeLists.txt`.
- **No new reverse engineering:** the report's "Read from the original" line says "none", or names only addresses that `docs/re` already recorded before this round. A branch that transcribes new handlers or constants gets a separate Opus code reviewer.
- **Code is touched:** a docs-only branch needs only the docs reviewer.

If a combined reviewer answers `SPLIT`, launch the two separate reviewers.

## Adjusting a reviewer at review time

The lead sees only `git diff --stat`, `--shortstat` for the code paths and the top-level directories a branch touches:

- **Small branch with no new reverse engineering:** one combined reviewer (above).
- **No `src/`, `tests/`, `tools/` or `CMakeLists.txt`:** no code reviewer.
- **Planner said Sonnet for the code reviewer, but the worker's short report says it read and transcribed new handlers:** Opus.
- **Docs reviewer:** keep the planner's choice unless the docs diff is far larger or smaller than the planner expected.

## Escalation

An agent stuck on code reading, or one whose turns keep repeating the same attempt, is stopped and reported to the user with a proposal for a larger model. A reviewer that cannot judge a reverse engineering claim says so in its verdict ("not verified: …") instead of guessing.
