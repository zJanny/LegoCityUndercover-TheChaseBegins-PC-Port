# Rules of the `qol` plan

Every agent that works for the `qol` plan (quality-of-life features) reads this file in full after [CLAUDE.md](../../CLAUDE.md). Plan: [qol.md](../plans/qol.md) and [qol.progress.md](../plans/qol.progress.md); index and gate: [plans/README.md](../plans/README.md).

**Gate: open.** The user opened it on 2026-10-28 with the words "I am opening the QOL of Life Gate.", earlier than the old condition "after the game is finished for PC". Only the user opens a gate; the faithful `linux-port` plan keeps going in parallel and has priority (below).

## The user's three rules

1. **Every QoL feature is switched in the menu.** Every QoL feature has a toggle in the normal settings menu of the game, and that is the only player-facing switch. No feature without a toggle, no command-line-only, file-only or key-only switch, no feature that runs unconditionally. (The user: "Every QOL of Live Feature is switched in the Menu".)
2. **The faithful version stays number one priority.** Everything is tested first in the original port without QoL, and then with the features enabled. (The user: "The faithful Version Stays no 1 Priority, everything is Tested First on the original Port without QOL and then with the Features enabled.")

3. **QoL source code is separate** from the port's code, no mixing (see "Separate source code").

What follows applies these rules.

## Faithful first

- **The bottom screen has no toggle of its own.** By the user's decision (2026-10-28, [qol-screen-layout.md §2026-10-28T09:48:15](../design/qol-screen-layout.md#ts-20261028-094815)) the bottom screen, its panels and the minimap are switched only by the master button: Improved hides the bottom screen and shows the minimap and the panels, Original and Custom show the bottom screen. There are no per-panel toggles and no minimap toggle. This is the one place where "each feature by itself" does not apply.

- **The two approved exceptions.** Both open the PC settings menu, change no game behaviour, and stay shown with every QoL feature off. Reviewers do not flag them; any other visible or behavioural difference with QoL off is a defect.
  - The pause menu has an always shown "PC Settings" entry. The user decided this explicitly ("Title + PC Settings entry", 2026-10-28, [qol-settings.md §2026-10-28T09:32:07](../design/qol-settings.md#ts-20261028-093207)). It is the last entry of the round pause menu. The original Options page stays unchanged.
  - The save slot page of the front end has an always shown "PC Settings" button. A left click inside it opens the menu instead of acting as A. The user allowed both explicitly ("Allow, update rule" and "Yes, acceptable", 2026-10-29, [qol-settings.md §2026-10-29T18:06:35](../design/qol-settings.md#ts-20261029-180635)).
- **Off by default.** With every QoL feature off (master state Original), the game behaves exactly as without the `qol` plan: the port's tests stay unchanged and pass, and no QoL code runs on the default path except the check of its own toggle. A change of this plan never edits a test of the port to make it pass.
- **Original mode first, then the feature.** Every QoL feature is built and tested in this order: (1) the whole test suite passes with all QoL off, before and after the change; (2) the feature's tests with its toggle on. A branch that reports only the second is not finished.
- **The port has priority.** A QoL feature never stands in for unrecovered original behaviour: gaps of the port stay port items. QoL work never blocks, delays or reorders a `linux-port` item; when both want the same code, the port's item goes first.
- **Saves stay faithful.** QoL settings are never written into the game save, and a save made with QoL on loads and plays in original mode.
- **Fix the port, not the toggle.** A bug found in original mode while testing a QoL feature is a port item (feature file of `linux-port`), reported through the lead.

## Separate source code

The user's third rule: "Source Code for These Features is separated from other Code! No mixing". Design: [qol-settings.md §2026-10-28T09:24:08](../design/qol-settings.md#ts-20261028-092408).

- **Where QoL code lives.** Only in `src/qol/`, built as its own CMake library `chase_qol`, with its tests in `tests/qol/` (CTest names start with `qol-`). Menu pages, layouts, panel and minimap logic, the toggle store and the feature code are all there. No QoL logic in `src/game*.cpp`, `src/runtime/`, `src/render/`, `src/frontend/`, `src/assets/` or any other file of the port. QoL code may call the port's public interfaces; it never copies port code.
- **What a port file may contain, and nothing more:** (1) the CMake line that links `chase_qol`; (2) an `#include` of a hook header of `src/qol/`; (3) a hook point: one call (or one `if (qol::enabled(qol::Feature::X)) qol::hookName(...)` line) to a function that `src/qol/` declares, with plain-type arguments and no QoL decision, math or state in the port file. With the toggle off the hook changes nothing. Every hook point is named in the list "Hook points" of [qol-settings-menu.md](../plans/features/qol-settings-menu.md) (file, function, hook, toggle) in the same branch that adds it; a hook not on the list is a defect.
- **A QoL feature never edits port runtime files** beyond these hook points: no changed constants, no moved code, no new branches, no refactoring of port files. A feature that needs more asks the lead; if a port library lacks a generic capability (for example an extra viewport or draw call in `chase_render`, where all OpenGL stays), the capability is added as a port item that changes no default behaviour, never as QoL logic inside the port.
- **The port never depends on QoL for its own tests:** the port's tests and the library build without `chase_qol` being consulted except through the hook calls, which compile to no effect in original mode.
- **Guard.** The CTest `qol-separation` (`tests/qol_separation.cmake`) checks the identifier, hook-count and no-copy rules; reviewers still check by hand that a hook holds no QoL decision, math or state.

## The toggle mechanism

Design: [docs/design/qol-settings.md](../design/qol-settings.md); features: [qol-settings-menu](../plans/features/qol-settings-menu.md).

- **Settings menu.** A normal PC settings menu with the tabs Game (the original options SFX volume, Music volume, Invert camera and Language, mirrored: same values, same save, in sync with the original Options page), Display (windowed and borderless full screen, resolution; an F11 shortcut switches the window mode), Controls (key bindings) and Quality of life (master button and one toggle per QoL feature). The master button cycles Original, Improved, Custom (Custom restores the last own mix kept in `config/qol.cfg` and is skipped while none is set). The Game, Display and Controls tabs are always active, they are not QoL features. Mouse, keyboard and gamepad all work; the menu uses the original game's art and fonts. It is reached from the title screen and from the "PC Settings" entry, the last entry of the round pause menu (mouse, D-pad or keyboard). Do not add entries the user did not choose.
- **Master toggle with three states:** Original (all QoL off), Custom and Improved (all on, including the minimap and the hidden bottom screen). The master state is stored in `config/qol.cfg`; the per-feature switches (so far only the widened view with the corner HUD) apply only in Custom, and Custom restores the last own mix.
- **Storage.** Key bindings stay in `config/controls.cfg`, or in `config/controls.pc.cfg` while the "PC controls" feature is on ([qol-controls.md §2026-11-01T13:50:47](../design/qol-controls.md#ts-20261101-135047)); everything else is in `config/qol.cfg` in the line format of `config/controls.cfg` (one "name value" per line; key names and values: [qol-settings.md §2026-10-28T21:43:23](../design/qol-settings.md#ts-20261028-214323)), parsed strictly (CLAUDE.md "Strict input": unknown or malformed lines stop with an error), never the game save. Default: all off.
- **A new feature adds its toggle in the same branch** as the feature, in the settings menu, with a test that drives the toggle. A feature whose toggle is missing is not mergeable.
- **S1 comes first.** Until the toggle store and the settings menu exist ([qol.md](../plans/qol.md) S1), the planner plans no other QoL package, and a worker that finds the mechanism missing does no QoL work and tells the user through the lead (`SendMessage` to `main`).

## Behaviour

- What a QoL feature does comes only from a spec the user approved, recorded in `docs/design/`. What the spec does not decide is asked of the user and recorded as an open item, never chosen. Points still open are open items in the feature files, marked as questions for the user.
- The bottom screen is hidden only when its features are on (it is hidden exactly when the minimap is on); the widened view and the corner HUD are one toggle; the minimap has no toggle of its own and shows only in Improved, so Custom always shows the bottom screen; every panel uses the original game's art and fonts; with all QoL off the window is today's top over bottom.
- New controls follow the input rules of CLAUDE.md; bottom-screen UI that stays is used by mouse clicks; a bottom-screen content moved to a full-window panel is clicked with the mouse as well.
- Shared code stays generic (CLAUDE.md "Generic over hardcoded"): one rule for all users, not per-object special cases. `src/game.cpp` gets only hook lines (see "Separate source code"); all QoL logic is in `src/qol/`, and OpenGL calls stay in `src/render/` behind plain-type interfaces.

## Tests

- Each QoL feature has tests with the feature off (unchanged behaviour, in original mode) and on (pinned to its spec entry in `docs/design/`, with provenance).
- The test of a toggle drives it through the same code as the settings menu, and the master state test covers Original, Custom and Improved.
- Output and picture checks follow CLAUDE.md "Short tests" and "No local-only data".

## Briefs and review

- **Every QoL brief also names:** this rule file, the spec entries in `docs/design/`, the toggle that switches the feature, the separation rule and the hook points the feature may add, that original mode is tested first, and the stop rule above.
- **Review checks beyond the tests:** the feature is off by default; the default path is unchanged and the port's tests are unchanged; the toggle is in the settings menu and is the user's mechanism; the diff touches port files only at listed hook points and QoL code is only in `src/qol/` and `tests/qol/`; every behaviour is backed by a spec entry; the tests cover original mode first and then on; nothing was done for a plan whose gate is closed (Windows) and no `linux-port` item was delayed.
