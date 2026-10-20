# Plans

The work is split into plans, one per line of work. Each plan has a plan/progress pair, a rule file that every agent working for it reads in full, and its own item ID prefixes. Rules: CLAUDE.md "Plans and order of work". Only the user opens a gate; a new plan is created with [new-plan.md](../../.claude/skills/lead-round/references/new-plan.md).

| Plan | Gate | Plan and progress | Rule file | Platform | ID prefixes |
|---|---|---|---|---|---|
| `linux-port` | open (user, 2026-10-12) | [linux-port.md](linux-port.md), [linux-port.progress.md](linux-port.progress.md) | [port.md](../rules/port.md) | Linux x86_64 | all prefixes in [features/](features/README.md) so far |
| `windows-port` | closed: only when the user says so | not created yet | [windows.md](../rules/windows.md) | Windows (user's Windows partition) | decided when the gate opens |
| `qol` | closed: only after the game is finished for PC | not created yet | [qol.md](../rules/qol.md) | all PC builds | decided when the gate opens |

Shared files: [features/README.md](features/README.md) (feature files of all plans, grouped by area), [priorities.md](priorities.md) (generated ranking, used by the `linux-port` plan), [id-reservations](features/id-reservations.md).
