# Rules of the `qol` plan

Every agent that works for the `qol` plan (quality-of-life features) reads this file in full after [CLAUDE.md](../../CLAUDE.md). Plan index and gate: [plans/README.md](../plans/README.md).

**Gate: closed.** It opens only after the game is finished for PC, and only the user opens it (CLAUDE.md "Plans and order of work"). Until then no QoL features or deviations of any kind, in no plan. When the user opens it, the plan is created with [new-plan.md](../../.claude/skills/lead-round/references/new-plan.md).

## Every QoL feature is optional

- **Off by default.** With every QoL feature off, the game behaves exactly as without the `qol` plan: the port's tests stay unchanged and pass, and no QoL code runs on the default path except the check of its own toggle.
- **The toggle mechanism is the user's.** Each QoL feature is switched by the toggle mechanism the user defined (recorded in `docs/design/`). Never invent a toggle mechanism, a settings file or a menu entry for it.
- **No mechanism yet: stop.** If no user-defined toggle mechanism exists, the planner plans no QoL package, and a worker that finds none stops and tells the user through the lead (`SendMessage` to `main`) that the toggle mechanism has to be defined and implemented first. It does no QoL work meanwhile.

## Behaviour

- What a QoL feature does comes only from a spec the user approved, recorded in `docs/design/`. What the spec does not decide is asked of the user, never chosen.
- A QoL feature never stands in for unrecovered original behaviour: gaps of the port stay port items.
- New controls follow the input rules of CLAUDE.md; bottom-screen UI is used by mouse clicks.

## Tests

- Each QoL feature has tests with the feature on (pinned to its spec entry) and off (unchanged behaviour).

## Briefs and review

- **Every QoL brief also names:** the spec entries in `docs/design/`, the toggle that switches the feature, and the stop rule above.
- **Review checks beyond the tests:** the feature is off by default; the default path is unchanged; the toggle is the user's mechanism; every behaviour is backed by a spec entry; tests cover on and off.
