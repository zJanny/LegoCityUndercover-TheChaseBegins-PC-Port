# Rules of the `qol` plan

Every agent that works for the `qol` plan (quality-of-life features) reads this file in full after [CLAUDE.md](../../CLAUDE.md). Plan: [qol.md](../plans/qol.md) and [qol.progress.md](../plans/qol.progress.md); index and gate: [plans/README.md](../plans/README.md).

**Gate: open.** The user opened it on 2026-10-28 with the words "I am opening the QOL of Life Gate.", earlier than the old condition "after the game is finished for PC". Only the user opens a gate; the faithful `linux-port` plan keeps going in parallel and has priority (below).

## The user's two rules

1. **Every QoL feature is switched in the menu.** Every QoL feature has a toggle in the normal settings menu of the game, and that is the only player-facing switch. No feature without a toggle, no command-line-only, file-only or key-only switch, no feature that runs unconditionally. (The user: "Every QOL of Live Feature is switched in the Menu".)
2. **The faithful version stays number one priority.** Everything is tested first in the original port without QoL, and then with the features enabled. (The user: "The faithful Version Stays no 1 Priority, everything is Tested First on the original Port without QOL and then with the Features enabled.")

What follows applies these two rules.

## Faithful first

- **Off by default.** With every QoL feature off (master state Original), the game behaves exactly as without the `qol` plan: the port's tests stay unchanged and pass, and no QoL code runs on the default path except the check of its own toggle. A change of this plan never edits a test of the port to make it pass.
- **Original mode first, then the feature.** Every QoL feature is built and tested in this order: (1) the whole test suite passes with all QoL off, before and after the change; (2) the feature's tests with its toggle on. A branch that reports only the second is not finished.
- **The port has priority.** A QoL feature never stands in for unrecovered original behaviour: gaps of the port stay port items. QoL work never blocks, delays or reorders a `linux-port` item; when both want the same code, the port's item goes first.
- **Saves stay faithful.** QoL settings are never written into the game save, and a save made with QoL on loads and plays in original mode.
- **Fix the port, not the toggle.** A bug found in original mode while testing a QoL feature is a port item (feature file of `linux-port`), reported through the lead.

## The toggle mechanism

Design: [docs/design/qol-settings.md](../design/qol-settings.md); features: [qol-settings-menu](../plans/features/qol-settings-menu.md).

- **Settings menu.** A normal PC settings menu, opened from the Options entry of the pause menu and from the title screen; no extra key. It holds the master toggle, one toggle per QoL feature, window mode and resolution, and key bindings. Volumes were not selected by the user: do not add them.
- **Master toggle with three states:** Original (all features off), Custom (any mix) and Improved (all on), derived from the per-feature switches; choosing Original or Improved sets every switch.
- **Storage.** `config/qol.cfg`, parsed strictly (CLAUDE.md "Strict input"), never the game save. Default: all off.
- **A new feature adds its toggle in the same branch** as the feature, in the settings menu, with a test that drives the toggle. A feature whose toggle is missing is not mergeable.
- **S1 comes first.** Until the toggle store and the settings menu exist ([qol.md](../plans/qol.md) S1), the planner plans no other QoL package, and a worker that finds the mechanism missing does no QoL work and tells the user through the lead (`SendMessage` to `main`).

## Behaviour

- What a QoL feature does comes only from a spec the user approved, recorded in `docs/design/`. What the spec does not decide is asked of the user and recorded as an open item, never chosen. Open items asked of the user so far: QOLSET-5, QOLSET-6, QOLMAP-2, QOLBOT-6.
- The bottom screen is hidden only when its features are on; with all QoL off the window is today's top over bottom.
- New controls follow the input rules of CLAUDE.md; bottom-screen UI that stays is used by mouse clicks; a bottom-screen content moved to a full-window panel is clicked with the mouse as well.
- Shared code stays generic (CLAUDE.md "Generic over hardcoded"): one rule for all users, not per-object special cases. `src/game.cpp` gets at most glue lines; QoL logic goes into its own files under `src/runtime/`, `src/render/` (OpenGL stays there) or a QoL library with its own tests.

## Tests

- Each QoL feature has tests with the feature off (unchanged behaviour, in original mode) and on (pinned to its spec entry in `docs/design/`, with provenance).
- The test of a toggle drives it through the same code as the settings menu, and the master state test covers Original, Custom and Improved.
- Output and picture checks follow CLAUDE.md "Short tests" and "No local-only data".

## Briefs and review

- **Every QoL brief also names:** this rule file, the spec entries in `docs/design/`, the toggle that switches the feature, that original mode is tested first, and the stop rule above.
- **Review checks beyond the tests:** the feature is off by default; the default path is unchanged and the port's tests are unchanged; the toggle is in the settings menu and is the user's mechanism; every behaviour is backed by a spec entry; the tests cover original mode first and then on; nothing was done for a plan whose gate is closed (Windows) and no `linux-port` item was delayed.
