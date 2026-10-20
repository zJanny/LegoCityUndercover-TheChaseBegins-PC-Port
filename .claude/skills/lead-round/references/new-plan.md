# New plan

How a plan is opened: the Windows port, quality-of-life features, or another line of work the user names. A planner agent does it instead of a round proposal, when the lead's prompt says `new plan: <name>`. Rules: CLAUDE.md "Plans and order of work"; the index is `docs/plans/README.md`.

The templates below set the minimum sections. A plan the user writes with another planning tool counts if it has at least these sections; then only check it, fill in what is missing, and do steps 4 and 5.

## Steps

1. **Gate.** Read `CLAUDE.md` in full, `docs/plans/README.md` and the plan's rule file `docs/rules/<plan>.md` if it exists. The gate opens only with the user's own words in the lead's prompt ("open the Windows port"). Without them, stop and answer `GATE CLOSED: <plan>, <its condition>`.
2. **Questions (phase 1).** Read only what the plan needs: the rule file, its "To be decided" section, `git log --oneline -15`, and Graphify for the code it touches (`graphify query "<q>" --graph graphify-out/graph.json`). Then end your turn with at most 30 lines:
   ```
   New plan <name> (planner, <date -Iseconds>)
   Gate: <the user's words>
   Goal: <one line>
   Questions for the user:
   1. <question> — recommended: <answer, one line why>
   ...
   Proposed first steps: <two to four lines>
   ```
   Ask about every decision the plan needs that no rule decides: for Windows the toolchain, dependencies, build and test commands, packaging; for QoL the toggle mechanism (`docs/rules/qol.md`), the first features and their spec. Never decide them yourself.
3. **Write (phase 2, after the lead resumes you with the answers).** Run `date -Iseconds` and use that time.
   - `docs/plans/<plan>.md` and `docs/plans/<plan>.progress.md` from the templates.
   - One `docs/design/<topic>.md` per decided topic, a dated entry with the user's answers in their words, and a row in `docs/design/README.md`. Open questions go last in the entry.
   - The rule file: fill in its "To be decided" section from the answers (link the design entries), and set the gate line to open.
   - The row in `docs/plans/README.md`: gate open with the date and the user's words, links to the pair, the ID prefixes.
   - New ID prefixes: `python3 tools/check_doc_links.py --next PREFIX --new-prefix` for each.
   - Feature files for the first steps from the feature template (the section list in `docs/plans/features/README.md`), under a new `## <Area>` heading of the plan in that index.
   - User docs, if setup or commands change (`docs/commands.md`, `docs/building.md`, README path).
4. Run `python3 tools/check_doc_links.py --fix`, then the checker without `--fix`; it must pass.
5. Commit on main only these files with the subject `Open the <plan> plan` and your own `Co-Authored-By:` line. Do not push. Final message, at most 8 lines: the commit, the files, the open questions left.

## Plan template (`docs/plans/<plan>.md`)

```markdown
# Plan: <title>

## 1. Summary
<What the plan delivers, in two to four sentences. What it does not do.>

## 2. Gate
Opened by the user on <date -Iseconds>: "<their words>".

## 3. Requirements
- R1: <requirement>; accept: <how it is checked>.

## 4. Scope and non-goals
<In scope; explicitly out of scope; what stays the port's job.>

## 5. Design decisions
- <topic>: [docs/design/<topic>.md §<ts>](../design/<topic>.md#ts-...)

## 6. Steps
- S1 (R1): <step>; needs: <step or none>; verify: <command or check>.

## 7. Verification
<Target platform and machine, build and test commands, how "verified" is reached, and that the port's tests stay unchanged.>

## 8. Rules
Rule file: [docs/rules/<plan>.md](../rules/<plan>.md). Features: under `## <Area>` in [features/README.md](features/README.md).
```

## Progress template (`docs/plans/<plan>.progress.md`)

The checker and the lead-round skill read `## Status`, `## Next Action` and `## Session Log`; keep these headings exactly.

```markdown
# Progress — <title>

## Status

- Overall: IN_PROGRESS
- Current Step: S1
- Last Updated: <date -Iseconds>
- Plan: docs/plans/<plan>.md

## Decisions

### Decision: <title> (<date -Iseconds>)
<The user's decision and a link to its design entry.>

## Blockers

None.

## Next Action

<Goal of the next rounds, the critical path, user actions needed. No findings, stand-ins or items.>

## Session Log

One line per session, newest last; `--fix` keeps the newest 20 and moves older ones to `<plan>.progress-archive.md`.

- <date -Iseconds> — Planner: opened the <plan> plan. See [<feature>](features/<feature>.md).
```
