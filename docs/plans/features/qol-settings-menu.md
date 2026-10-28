# QoL settings menu and toggles

## 1. Scope

The settings menu of the PC port and the toggle store of all QoL features: the master toggle (Original, Custom, Improved), one toggle per feature, window mode and resolution, key bindings, and `config/qol.cfg`. Opened from the Options entry of the pause menu and from the title screen, with no extra key. Nothing else of the `qol` plan may start before this exists. Plan: [qol.md](../qol.md).

## 2. Status

NOT_STARTED, opened 2026-10-28T09:21:35+01:00. No code.

## 3. Verified behaviour

None yet. Every behaviour is tested first in original mode (all QoL off, the port's tests unchanged), then with the feature on ([qol rules](../../rules/qol.md)).

## 4. Stand-ins

None.

## 5. Open items

- [ ] QOLSET-1 — The toggle store: one switch per QoL feature, the three-state master (Original, Custom, Improved) derived from them, read and written strictly in `config/qol.cfg`, never in the game save; all off by default. blocks: QOLSET-2, QOLFS-1, QOLMAP-1, QOLBOT-1, QOLSET-7. evidence: [qol-settings.md §2026-10-28T09:21:31](../../design/qol-settings.md#ts-20261028-092131).
- [ ] QOLSET-2 — The settings menu with the master toggle and one toggle per feature, opened from the title screen and from the Options entry of the pause menu, usable with the mouse and keyboard or pad. needs: QOLSET-1. blocks: QOLSET-3, QOLSET-4, QOLBOT-5, QOLSET-5, QOLSET-6. evidence: [qol-settings.md §2026-10-28T09:21:31](../../design/qol-settings.md#ts-20261028-092131).
- [ ] QOLSET-3 — Window mode and resolution in the settings menu. needs: QOLSET-2. evidence: [qol-settings.md §2026-10-28T09:21:31](../../design/qol-settings.md#ts-20261028-092131).
- [ ] QOLSET-4 — Key bindings in the settings menu (binding format: `docs/controls.md`). needs: QOLSET-2. evidence: [qol-settings.md §2026-10-28T09:21:31](../../design/qol-settings.md#ts-20261028-092131).
- [ ] QOLSET-5 — Open question for the user: with all QoL off the Options entry of the pause menu must stay the original's (UI-4, today it does nothing), yet the settings menu has to be reachable to switch QoL on; how it is reached then (title screen only, or another mouse-clicked entry) is not decided. needs: QOLSET-2. evidence: [qol-settings.md §2026-10-28T09:21:31](../../design/qol-settings.md#ts-20261028-092131).
- [ ] QOLSET-6 — Open question for the user: whether the original Options entries (SFX, Music, Invert camera, Language) are shown in the settings menu; volumes were not selected. needs: QOLSET-2. evidence: [qol-settings.md §2026-10-28T09:21:31](../../design/qol-settings.md#ts-20261028-092131).
- [ ] QOLSET-7 — The CTest `qol-separation`, shaped like `gl-only-in-render` and `game-cpp-wiring` (CMake script plus a rejecting fixture under `tests/`): QoL identifiers (`qol::`, `chase_qol`) appear only in `src/qol/`, `tests/qol/`, CMake link lines and the hook lines of the Hook points list below; the number of hook lines in port files equals the list; `src/qol/` copies no port file. Propose it in the first round, do not build it before S1 is planned. needs: QOLSET-1. evidence: [qol-settings.md §2026-10-28T09:24:08](../../design/qol-settings.md#ts-20261028-092408).

## 6. Findings summary

Design: [qol-settings.md](../../design/qol-settings.md).

## 7. Code and tests

Not written yet. Tests: one set with the feature off (unchanged behaviour) and one with it on, pinned to the design entry.

### Hook points

Port files that call into `src/qol/` (rule file "Separate source code"): none yet. One line per hook: file, function, hook, toggle.
