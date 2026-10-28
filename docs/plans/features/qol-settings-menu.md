# QoL settings menu and toggles

## 1. Scope

The settings menu of the PC port and the toggle store of all QoL features: the master toggle (Original, Custom, Improved), one toggle per feature, window mode and resolution, key bindings, and `config/qol.cfg`. Opened from the title screen and from an always shown "PC Settings" entry of the pause menu, with no extra key; tabs Game, Display, Controls and Quality of life; mouse, keyboard and gamepad. Nothing else of the `qol` plan may start before this exists. Plan: [qol.md](../qol.md).

## 2. Status

NOT_STARTED, opened 2026-10-28T09:21:35+01:00. No code.

## 3. Verified behaviour

None yet. Every behaviour is tested first in original mode (all QoL off, the port's tests unchanged), then with the feature on ([qol rules](../../rules/qol.md)).

## 4. Stand-ins

None.

## 5. Open items

- [ ] QOLSET-1 — The toggle store: one switch per QoL feature, the three-state master (Original, Custom, Improved) derived from them, read and written strictly in `config/qol.cfg` (line format of `config/controls.cfg`, unknown or malformed lines stop with an error), never in the game save; all off by default. blocks: QOLSET-2, QOLFS-1, QOLMAP-1, QOLBOT-1, QOLSET-7. evidence: [qol-settings.md §2026-10-28T09:32:07](../../design/qol-settings.md#ts-20261028-093207).
- [ ] QOLSET-2 — The settings menu with the tabs Game, Display, Controls and Quality of life (master toggle and one toggle per feature), usable with mouse, keyboard and gamepad, opened from the title screen. needs: QOLSET-1. blocks: QOLSET-3, QOLSET-4, QOLSET-8, QOLSET-9, QOLBOT-5. evidence: [qol-settings.md §2026-10-28T09:32:07](../../design/qol-settings.md#ts-20261028-093207).
- [ ] QOLSET-3 — Window mode and resolution in the Display tab. needs: QOLSET-2. evidence: [qol-settings.md §2026-10-28T09:32:07](../../design/qol-settings.md#ts-20261028-093207).
- [ ] QOLSET-4 — Key bindings in the Controls tab (binding format: `docs/controls.md`). needs: QOLSET-2. evidence: [qol-settings.md §2026-10-28T09:32:07](../../design/qol-settings.md#ts-20261028-093207).
- [ ] QOLSET-7 — The CTest `qol-separation`, shaped like `gl-only-in-render` and `game-cpp-wiring` (CMake script plus a rejecting fixture under `tests/`): QoL identifiers (`qol::`, `chase_qol`) appear only in `src/qol/`, `tests/qol/`, CMake link lines and the hook lines of the Hook points list below; the number of hook lines in port files equals the list; `src/qol/` copies no port file. Propose it in the first round, do not build it before S1 is planned. needs: QOLSET-1. blocks: QOLSET-8. evidence: [qol-settings.md §2026-10-28T09:24:08](../../design/qol-settings.md#ts-20261028-092408).
- [ ] QOLSET-8 — The always shown "PC Settings" entry in the pause menu, clicked with the mouse, opens the PC settings menu only and changes no game behaviour (the one approved exception, shown even with every QoL feature off; the original Options page stays unchanged; the entry is a hook point). needs: QOLSET-2, QOLSET-7. evidence: [qol-settings.md §2026-10-28T09:32:07](../../design/qol-settings.md#ts-20261028-093207).
- [ ] QOLSET-9 — The Game tab mirrors SFX volume, Music volume, Invert camera and Language, kept in sync with the original Options page, same values, same save; the original page is not ported yet. needs: QOLSET-2, UI-4, SAVE-4. evidence: [qol-settings.md §2026-10-28T09:32:07](../../design/qol-settings.md#ts-20261028-093207).

## 6. Findings summary

Design: [qol-settings.md](../../design/qol-settings.md).

## 7. Code and tests

Not written yet. Tests: one set with the feature off (unchanged behaviour) and one with it on, pinned to the design entry.

### Hook points

Port files that call into `src/qol/` (rule file "Separate source code"): none yet. One line per hook: file, function, hook, toggle.
