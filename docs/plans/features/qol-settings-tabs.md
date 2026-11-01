# QoL settings menu: Display and Controls tabs

## 1. Scope

The Display tab (window mode, resolution, F11) and the Controls tab (key bindings) of the PC settings menu, split from [qol-settings-menu](qol-settings-menu.md). Plan: [qol.md](../qol.md).

## 2. Status

IN_PROGRESS. The Display and Controls tab models, stores and joins exist; the drawing is in [qol-settings-drawing](qol-settings-drawing.md).

## 3. Verified behaviour

None yet. Every behaviour is tested first in original mode (all QoL off, the port's tests unchanged), then with the feature on ([qol rules](../../rules/qol.md)).

## 4. Stand-ins

None.

## 5. Open items

- [ ] QOLSET-3 — Storage, row model and menu-model tab done (`window_mode`, `resolution` in `settings`, `display_tab`, Display page `ui_page_display`, tests `qol-display-tab`, `qol-ui-page-display`); the window follows the Display tab (QOLSET-19). Windowed and borderless full screen in the Display tab, plus resolution, and an F11 shortcut that switches the window mode (the key is a binding in `config/controls.cfg`, format in `docs/controls.md`); window mode and resolution are stored in `config/qol.cfg` as `window_mode` (`windowed` or `fullscreen`) and `resolution` (`WIDTHxHEIGHT`, for example `1280x720`); `window_mode` is written in every mode, `resolution` only once the player has picked a size (values: [qol-settings.md §2026-10-28T22:20:41](../../design/qol-settings.md#ts-20261028-222041)). needs: QOLSET-2. blocks: QOLSET-18. evidence: [qol-settings.md §2026-10-28T22:20:41](../../design/qol-settings.md#ts-20261028-222041).
- [ ] QOLSET-4 — Model, adapter, writer, `pad` source, SDL resolver and namer and the join done (`controls_tab`, `controls_file`, `QolControlsBindings`; tests `qol-controls-tab`, `qol-controls-file`, `qol-controls-bindings`, `input`: refuse with message, Reset all with confirmation, Escape cancel, key, mouse and pad inputs, two slots with a mode each, clearing, a write after each change or confirmed reset); the Controls tab rows in the menu model are done (QOLSET-39); left: the new widgets (QOLSET-75). Key bindings in the Controls tab, stored in `config/controls.cfg` (format: `docs/controls.md`); the Game, Display and Controls tabs work with every QoL feature off, their values other than the bindings are stored in `config/qol.cfg`. needs: QOLSET-2, QOLSET-32. blocks: QOLSET-38. evidence: [qol-settings.md §2026-10-29T11:06:51](../../design/qol-settings.md#ts-20261029-110651).

## 6. Findings summary

Design: [qol-settings.md](../../design/qol-settings.md).

## 7. Code and tests

Display tab: `display_tab.hpp`/`display_tab.cpp` is the pure row model. Test `qol-display-tab` (`tests/qol/display_tab_test.cpp`) covers the rows, toggling, list cycling, the clamp to the largest mode and rejections. The Display tab of the model takes the display modes and desktop size as plain types (`DisplayPage`). Controls tab model: `controls_tab.hpp`/`controls_tab.cpp` (plain `BindingEntry` list and validator callbacks, no `input.hpp` dependency), test `qol-controls-tab` (`tests/qol/controls_tab_test.cpp`). File adapter and writer: `controls_file.hpp`/`controls_file.cpp` (over `chase::readBindings`, key resolver and namer callbacks), test `qol-controls-file` (`tests/qol/controls_file_test.cpp`). Controls join: `src/runtime/qol_controls_bindings.hpp`/`.cpp` (`QolControlsBindings`, SDL key resolver and namer), test `qol-controls-bindings` (`tests/qol/controls_bindings_test.cpp`). Controls rows: `controls_port.hpp` (`ControlsPort`, `ControlsTabPort`), `ControlsPage` over `ControlsPort`, covered by `qol-ui-page-controls`.
