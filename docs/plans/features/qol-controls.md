# QoL PC controls

## 1. Scope

Escape that is consistent and never closes the game, and the overhaul of the controls so that the game plays like a typical PC game on foot and drives like a typical PC driving game. Plan: [qol.md](../qol.md). A QoL feature with its own "PC controls" toggle on the Quality of life tab (on in Improved, off in Original); with it off the faithful 3DS mapping of [input](input.md) holds. The Escape rules apply in both modes. Design: [qol-controls.md](../../design/qol-controls.md). The settings menu and its Controls tab are [qol-settings-ui](qol-settings-ui.md).

## 2. Status

IN_PROGRESS. The user approved the Escape rules and the controls design on 2026-11-01 (see the design file). Built by 20261101-qolctl-A: the "PC controls" toggle (QOLCTL-8) and the on-foot default set (QOLCTL-2), kept in `config/controls.pc.cfg` (the user's answer of 2026-11-01). Open: Escape rules and the Quit row (worker B), the vehicle layout, the mouse camera.

## 3. Verified behaviour

PC controls toggle and on-foot set pinned by test `qol-pc-controls` against the design entry (original mode first: no file written, `config/controls.cfg` untouched; Original off, Improved on, Custom own mix; the layout key by key; the running input; toggle through the settings menu; rebinds and Reset kept per mapping). Every behaviour is tested first in original mode (all QoL off, the port's tests unchanged), then with the feature on ([qol rules](../../rules/qol.md)).

## 4. Stand-ins

- QOLCTL-S1 — `chase-game` closes on Escape (`pollEvents` in `src/game.cpp`, a development convenience, not a 3DS button; "Press Escape to quit" in `docs/controls.md`) until the Escape rule is built. Replaced by: QOLCTL-1. evidence: [qol-controls.md §2026-11-01T13:50:44](../../design/qol-controls.md#ts-20261101-135044).

- QOLCTL-S2 — The PC on-foot set has no Escape for Start (Start is Keypad Enter only) and no driving keys; the table is `pcControlsDefaults` in `src/qol/pc_controls.cpp`. Replaced by: QOLCTL-1 (Escape, fixed Start binding of worker B) and QOLCTL-3. evidence: [qol-controls.md §2026-11-01T13:50:47](../../design/qol-controls.md#ts-20261101-135047).

## 5. Open items

- [ ] QOLCTL-1 — Escape rules in every mode: Escape never closes `chase-game` (window close still does); in play it opens the pause menu (also bound to Start, Keypad Enter second), in a menu or dialog it goes back or closes it, in movies, cutscenes, logos and the title it acts like the original skip where one exists and does nothing else. `chase-viewer`, the FMV player and `chase-input-lab` keep Escape = quit. Replaces QOLCTL-S1; updates `docs/controls.md` ("Press Escape to quit"). needs: QOLCTL-5. evidence: [qol-controls.md §2026-11-01T13:50:44](../../design/qol-controls.md#ts-20261101-135044).
- [ ] QOLCTL-5 — "Quit game" row with Yes/No on the PC Settings Game tab and the same on the title and save slot screens; the original menus stay unchanged. blocks: QOLCTL-1. evidence: [qol-controls.md §2026-11-01T13:50:44](../../design/qol-controls.md#ts-20261101-135044).
- [ ] QOLCTL-3 — Vehicle layout: W or Up accelerate (A), S or Down brake and reverse (B), A and D steer, Space brake (B), H horn (Y), F leave (X); hand brake and siren stay unbound. needs: QOLCTL-7. blocks: QOLCTL-10. evidence: [qol-controls.md §2026-11-01T13:50:47](../../design/qol-controls.md#ts-20261101-135047).
- [ ] QOLCTL-6 — Mouse X turns the camera: it feeds the existing camera turn (yaw only, analog size, no pitch); the cursor is captured in play and released in menus and on the lower screen; a left click on the lower screen still counts as touch. needs: QOLCTL-8. evidence: [qol-controls.md §2026-11-01T13:50:47](../../design/qol-controls.md#ts-20261101-135047).
- [ ] QOLCTL-7 — Bindings that depend on being seated in a vehicle (the same keys mean different actions on foot and in a vehicle). needs: QOLCTL-8. blocks: QOLCTL-3. evidence: [qol-controls.md §2026-11-01T13:50:47](../../design/qol-controls.md#ts-20261101-135047).
- [ ] QOLCTL-10 — An existing `config/controls.pc.cfg` is not updated when the built-in set grows (driving keys, mouse camera): the file is written once and then belongs to the user. Decide a merge rule when the set changes (question for the user). needs: QOLCTL-3. evidence: [qol-controls.md §2026-11-01T13:50:47](../../design/qol-controls.md#ts-20261101-135047).

## 6. Findings summary

Design: [qol-controls.md](../../design/qol-controls.md); the user's report: [playtests-qol.md §2026-11-01T13:50:42](../../design/playtests-qol.md#ts-20261101-135042).

## 7. Code and tests

PC controls: `src/qol/pc_controls.*` (the built-in on-foot set, the file `controls.pc.cfg` next to the controls file, `activeControls` choosing the file by the toggle), `src/qol/controls_reload.hpp` (`ReloadingControls`: the Controls tab port that rebuilds when the toggle changes), the `PcControls` feature in `src/qol/features.hpp` (`pc_controls` in `config/qol.cfg`) with its row and help in `src/qol/ui_page_qol.cpp`, `MenuStores::commit` (`src/qol/menu_stores.cpp`) reloading the Controls tab and the running input when the toggle changes; port side `src/runtime/qol_pc_controls.*` (SDL key resolver join) and `QolWindow::syncControls` for the start. Test `qol-pc-controls` (`tests/qol/pc_controls_test.cpp`); `qol-settings`, `qol-ui-page-qol` and `qol-menu-texts` pin the second feature. Still today: `src/game.cpp` (`pollEvents`), `src/input.*`, `config/controls.cfg`, `src/qol/controls_tab.*`, `src/qol/ui_page_controls.*`; user doc `docs/controls.md`.
