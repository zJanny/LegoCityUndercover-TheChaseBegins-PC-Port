# QoL PC controls

## 1. Scope

Escape that is consistent and never closes the game, and the overhaul of the controls so that the game plays like a typical PC game on foot and drives like a typical PC driving game. Plan: [qol.md](../qol.md). A QoL feature with its own "PC controls" toggle on the Quality of life tab (on in Improved, off in Original); with it off the faithful 3DS mapping of [input](input.md) holds. The Escape rules apply in both modes. Design: [qol-controls.md](../../design/qol-controls.md). The settings menu and its Controls tab are [qol-settings-ui](qol-settings-ui.md).

## 2. Status

NOT_STARTED, recorded 2026-11-01T13:50:47+01:00 from the user's playtest of the new settings menu. The user approved the Escape rules and the controls design on 2026-11-01 (see the design file); nothing is built.

## 3. Verified behaviour

None yet. Every behaviour is tested first in original mode (all QoL off, the port's tests unchanged), then with the feature on ([qol rules](../../rules/qol.md)).

## 4. Stand-ins

- QOLCTL-S1 — `chase-game` closes on Escape (`pollEvents` in `src/game.cpp`, a development convenience, not a 3DS button; "Press Escape to quit" in `docs/controls.md`) until the Escape rule is built. Replaced by: QOLCTL-1. evidence: [qol-controls.md §2026-11-01T13:50:44](../../design/qol-controls.md#ts-20261101-135044).

## 5. Open items

- [ ] QOLCTL-1 — Escape rules in every mode: Escape never closes `chase-game` (window close still does); in play it opens the pause menu (also bound to Start, Return second), in a menu or dialog it goes back or closes it, in movies, cutscenes, logos and the title it acts like the original skip where one exists and does nothing else. `chase-viewer`, the FMV player and `chase-input-lab` keep Escape = quit. Replaces QOLCTL-S1; updates `docs/controls.md` ("Press Escape to quit"). needs: QOLCTL-5. evidence: [qol-controls.md §2026-11-01T13:50:44](../../design/qol-controls.md#ts-20261101-135044).
- [ ] QOLCTL-5 — "Quit game" row with Yes/No on the PC Settings Game tab and the same on the title and save slot screens; the original menus stay unchanged. blocks: QOLCTL-1. evidence: [qol-controls.md §2026-11-01T13:50:44](../../design/qol-controls.md#ts-20261101-135044).
- [ ] QOLCTL-2 — On-foot PC layout as the default set of "PC controls": W/A/S/D and arrows move, Left Ctrl held walks (Left Alt second), Space jumps (B), E interacts and confirms (A, Enter second), left mouse punches (Y, R second), F enters or leaves a vehicle (X), Z and C turn the camera, middle mouse re-centres (Z and C together second), Tab is Select, right mouse unused; lower-screen menus clicked. needs: QOLCTL-8. evidence: [qol-controls.md §2026-11-01T13:50:47](../../design/qol-controls.md#ts-20261101-135047).
- [ ] QOLCTL-3 — Vehicle layout: W or Up accelerate (A), S or Down brake and reverse (B), A and D steer, Space brake (B), H horn (Y), F leave (X); hand brake and siren stay unbound. needs: QOLCTL-7. evidence: [qol-controls.md §2026-11-01T13:50:47](../../design/qol-controls.md#ts-20261101-135047).
- [ ] QOLCTL-6 — Mouse X turns the camera: it feeds the existing camera turn (yaw only, analog size, no pitch); the cursor is captured in play and released in menus and on the lower screen; a left click on the lower screen still counts as touch. needs: QOLCTL-8. evidence: [qol-controls.md §2026-11-01T13:50:47](../../design/qol-controls.md#ts-20261101-135047).
- [ ] QOLCTL-7 — Bindings that depend on being seated in a vehicle (the same keys mean different actions on foot and in a vehicle). needs: QOLCTL-8. blocks: QOLCTL-3. evidence: [qol-controls.md §2026-11-01T13:50:47](../../design/qol-controls.md#ts-20261101-135047).
- [ ] QOLCTL-8 — The "PC controls" toggle on the Quality of life tab (on in Improved, off in Original) and its default set: the Controls tab rebinds on top of it and "Reset to defaults" restores it; the 3DS button icons stay in the game, key names only in the Controls tab. needs: QOLSET-2. blocks: QOLCTL-2, QOLCTL-6, QOLCTL-7. evidence: [qol-controls.md §2026-11-01T13:50:47](../../design/qol-controls.md#ts-20261101-135047).

## 6. Findings summary

Design: [qol-controls.md](../../design/qol-controls.md); the user's report: [playtests-qol.md §2026-11-01T13:50:42](../../design/playtests-qol.md#ts-20261101-135042).

## 7. Code and tests

None yet. Today: `src/game.cpp` (`pollEvents`), `src/input.*`, `config/controls.cfg`, `src/qol/controls_tab.*`, `src/qol/ui_page_controls.*`; user doc `docs/controls.md`.
