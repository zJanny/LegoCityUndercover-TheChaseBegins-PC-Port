# QoL PC controls

## 1. Scope

Escape that is consistent and never closes the game, and the overhaul of the controls so that the game plays like a typical PC game on foot and drives like a typical PC driving game. Plan: [qol.md](../qol.md). A QoL feature with its own "PC controls" toggle on the Quality of life tab (on in Improved, off in Original); with it off the faithful 3DS mapping of [input](input.md) holds. The Escape rules apply in both modes. Design: [qol-controls.md](../../design/qol-controls.md). The settings menu and its Controls tab are [qol-settings-ui](qol-settings-ui.md).

## 2. Status

IN_PROGRESS. The user approved the Escape rules and the controls design on 2026-11-01 (see the design file). Built by 20261101-qolctl-A: the "PC controls" toggle (QOLCTL-8) and the on-foot default set (QOLCTL-2), kept in `config/controls.pc.cfg` (the user's answer of 2026-11-01). Built by 20261101-qolctl-B: the Quit game row and the Escape rules (QOLCTL-5, QOLCTL-1). Open: the vehicle layout, the mouse camera.

## 3. Verified behaviour

PC controls toggle and on-foot set pinned by test `qol-pc-controls` against the design entry (original mode first: no file written, `config/controls.cfg` untouched; Original off, Improved on, Custom own mix; the layout key by key; the running input; toggle through the settings menu; rebinds and Reset kept per mapping). Escape (QOLCTL-1) and Quit game (QOLCTL-5), tests `escape-binding`, `qol-ui-page-game`, `qol-ui-menu`, `qol-quit-event`, `qol-pause-entry-real-data`: Escape is a fixed second binding of Start in `chase-game` (`InputState::fix`), never a quit; Quit game asks "Quit the game?" with Yes and No (focus on No), Yes pushes `SDL_QUIT`, No, Escape, pad B and a click on No cancel. Every behaviour is tested first in original mode (all QoL off, the port's tests unchanged), then with the feature on ([qol rules](../../rules/qol.md)).

## 4. Stand-ins

- QOLCTL-S3 — Escape acts as Start in the pause screen's Options and Languages pages: it closes the whole pause screen at once instead of going back one page like B (the page of the pause screen is not readable from `pollEvents`). Replaced by: QOLCTL-12. evidence: [qol-controls.md §2026-11-01T13:50:44](../../design/qol-controls.md#ts-20261101-135044).

- QOLCTL-S2 — The PC on-foot set has no driving keys; the table is `pcControlsDefaults` in `src/qol/pc_controls.cpp`. Replaced by: QOLCTL-3. (Its missing Escape for Start is covered: Escape is a fixed Start binding in every mode, QOLCTL-1.) evidence: [qol-controls.md §2026-11-01T13:50:47](../../design/qol-controls.md#ts-20261101-135047).

## 5. Open items

- [ ] QOLCTL-11 — The screens before the slot page (logos, movie, language list, prompt pages) have no Quit game; only closing the window quits there (the user chose "Only via PC Settings", 2026-11-01). Question for the user: is that enough? evidence: [qol-controls.md §2026-11-01T13:50:44](../../design/qol-controls.md#ts-20261101-135044).
- [ ] QOLCTL-12 — Escape on the pause screen's Options and Languages pages should go back one page (B) instead of closing the pause screen (Start): the pause page must be readable from `pollEvents`, a query on `LevelSession` in `src/game_level.hpp` (a port file). Replaces stand-in QOLCTL-S3. evidence: [qol-controls.md §2026-11-01T13:50:44](../../design/qol-controls.md#ts-20261101-135044).
- [ ] QOLCTL-3 — Vehicle layout: W or Up accelerate (A), S or Down brake and reverse (B), A and D steer, Space brake (B), H horn (Y), F leave (X); hand brake and siren stay unbound. needs: QOLCTL-7. blocks: QOLCTL-10. evidence: [qol-controls.md §2026-11-01T13:50:47](../../design/qol-controls.md#ts-20261101-135047).
- [ ] QOLCTL-6 — Mouse X turns the camera: it feeds the existing camera turn (yaw only, analog size, no pitch); the cursor is captured in play and released in menus and on the lower screen; a left click on the lower screen still counts as touch. needs: QOLCTL-8. evidence: [qol-controls.md §2026-11-01T13:50:47](../../design/qol-controls.md#ts-20261101-135047).
- [ ] QOLCTL-7 — Bindings that depend on being seated in a vehicle (the same keys mean different actions on foot and in a vehicle). needs: QOLCTL-8. blocks: QOLCTL-3. evidence: [qol-controls.md §2026-11-01T13:50:47](../../design/qol-controls.md#ts-20261101-135047).
- [ ] QOLCTL-10 — An existing `config/controls.pc.cfg` is not updated when the built-in set grows (driving keys, mouse camera): the file is written once and then belongs to the user. Decide a merge rule when the set changes (question for the user). needs: QOLCTL-3. evidence: [qol-controls.md §2026-11-01T13:50:47](../../design/qol-controls.md#ts-20261101-135047).

## 6. Findings summary

Design: [qol-controls.md](../../design/qol-controls.md); the user's report: [playtests-qol.md §2026-11-01T13:50:42](../../design/playtests-qol.md#ts-20261101-135042).

## 7. Code and tests

Escape: `src/runtime/qol_escape.hpp` (`bindEscape`), `InputState::fix` in `src/input.*`, test `escape-binding` (`tests/escape_test.cpp`). Quit game: `src/qol/ui_page_game.*` (`GamePage`, quit-only page), `src/qol/ui_menu.*` (`MenuEffects::quitGame`), `src/qol/ui_menu_sdl.cpp` (pushes `SDL_QUIT`), `StoresPorts::quitTab` set by `src/qol/pause_entry.cpp`, tests `qol-ui-page-game`, `qol-ui-menu`, `qol-quit-event` (`tests/qol/quit_event_test.cpp`), `qol-pause-entry-real-data`. PC controls: `src/qol/pc_controls.*` (the built-in on-foot set, the file `controls.pc.cfg` next to the controls file, `activeControls` choosing the file by the toggle), `src/qol/controls_reload.hpp` (`ReloadingControls`: the Controls tab port that rebuilds when the toggle changes), the `PcControls` feature in `src/qol/features.hpp` (`pc_controls` in `config/qol.cfg`) with its row and help in `src/qol/ui_page_qol.cpp`, `MenuStores::commit` (`src/qol/menu_stores.cpp`) reloading the Controls tab and the running input when the toggle changes; port side `src/runtime/qol_pc_controls.*` (SDL key resolver join) and `QolWindow::syncControls` for the start. Test `qol-pc-controls` (`tests/qol/pc_controls_test.cpp`); `qol-settings`, `qol-ui-page-qol` and `qol-menu-texts` pin the second feature. Still today: `src/game.cpp` (`pollEvents`), `src/input.*`, `config/controls.cfg`, `src/qol/controls_tab.*`, `src/qol/ui_page_controls.*`; user doc `docs/controls.md`.
