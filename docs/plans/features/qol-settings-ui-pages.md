# QoL settings menu: pages and dialogs

## 1. Scope

The pages (Game, Display, Controls, Quality of life) and the dialogs (binding conflict swap, keep settings) of the modern PC settings menu, split from [qol-settings-ui](qol-settings-ui.md). Plan: [qol.md](../qol.md).

## 2. Status

DONE for the models; wired and used in `chase-game` by QOLSET-79 (see [qol-settings-ui](qol-settings-ui.md)). The help texts of the Game, Display and Controls rows are approved (QOLSET-97).

## 3. Verified behaviour

None yet (the menu is not part of the original).

## 4. Stand-ins

None.

## 5. Open items

None; the open items of the menu are in [qol-settings-ui](qol-settings-ui.md).

## 6. Findings summary

Design: [qol-settings-drawing.md §2026-10-30T21:56:04](../../design/qol-settings-drawing.md#ts-20261030-215604).

## 7. Code and tests

Done (20261101-qolui3-A): `UiNav` takes `promptInput(source, code)` events (key name with underscores, mouse button name, pad button name) that become `PromptInput` actions only while a Prompt modal is open, and `Key::Delete`, which becomes `PromptClear` in a prompt and does nothing elsewhere; clicks in a prompt still arrive as `ModalClick` (button 1 left, 3 right). `RowSpec::cells` (additive in `ui_layout.*`) splits the control of a Button row into equal cells with hit areas (`RowLayout::cells`, `Part::Cell`, `Hit::cell`); `UiNav` moves `Screen::focusCell` with Left/Right and sends `Activate` with `item` = cell (answer 15 of the design entry). `src/qol/ui_page_controls.*` (`ControlsPage`, no SDL, not wired): rows Action, Slot 1, Slot 2, Mode over a `ControlsPort`, outcomes `PromptOpen`, `PromptClosed`, `Changed`, `Rejected`, `NeedsConfirm`, `NeedsSwapDialog(otherAction)`; menu keys (Esc, Q, E, PageUp, PageDown, arrows, Enter, Delete) refused, pad buttons only after `setPadConnected(true)`, both slots follow the Mode, `reset()` after the confirm. Tests `qol-ui-page-controls` (`tests/qol/ui_page_controls_test.cpp`, with the nav at 720p, 1080p and 4K), cell cases in `qol-ui-nav` and `qol-ui-layout`.
Done: `src/qol/ui_page_game.*` and `ui_page_display.*` (QOLSET-73, no SDL, no GL, not wired): `GamePage` (SFX and Music sliders 0 to 100 in steps of 10, Invert camera toggle, Language selector over nine names with wrap that only keeps the chosen value and reports `languageChosen`, Reset to defaults with `needsConfirm` and `reset()` restoring the start language, answer 22) and `DisplayPage` (Window mode selector of Windowed and Fullscreen that wraps, answer 23, Resolution dropdown with `dropdown(row)` and `dropdownItems(row)`, disabled with "Desktop resolution" in full screen, `needsKeepSettings{previous, current}` on a mode or size change but not for the resolution already in use, answer 24; also for the reset). Both take the existing stores by reference and answer a `UiNav` `Action` with a small result struct; `Settings::clearResolution` was added for the reset. Tests `qol-ui-page-game` and `qol-ui-page-display` (`tests/qol/ui_page_game_test.cpp`, `ui_page_display_test.cpp`), driven through `UiNav` at 720p, 1080p and 4K.
Done: `src/qol/ui_page_qol.*` (QOLSET-74, in `chase_qol`, no SDL, no GL, not wired): `QolPage` over `Settings&` with the page interface (`rows`, `dropdown`, `handle`, `reset`) and `help(row)` returning `PageHelp`. Rows: preset selector, one toggle per feature (table `featureHelp` with its help text and default, `static` size tied to `featureCount`), "Reset to defaults" button. A hand change of a feature copies the effective values into the custom switches, flips the one and sets Custom; the preset steps over Original, Improved and Custom (only once an own mix exists) and wraps; the confirmed reset sets Original and keeps the own mix. Test `qol-ui-page-qol` (`tests/qol/ui_page_qol_test.cpp`). Conventions as the user approved them (design entry answers 25 to 28): any value change on a toggle row flips it, the preset selector wraps at both ends and skips Custom without an own mix, the confirmed reset keeps the own mix, and the help texts.
Done: `src/qol/controls_swap.*` and an additive part of `src/qol/controls_tab.*` (QOLSET-76, no SDL, not wired): after `assignInput` reports a conflict with another slot, `ControlsTab` keeps a pending swap (`conflictPending`, `pendingSource`, `pendingCode`); `confirmSwap` exchanges source and code of the two slots (modes stay with their actions; an empty slot makes the other action lose the key and the asking action append it as a `hold` slot) and `cancelConflict` clears the conflict and leaves the prompt open (answer 17). The old refuse result and message are unchanged. `swapDialog(tab)` returns the `SwapDialog` (title, body naming the key and the other action, buttons Swap and Cancel, default focus Cancel) and `resolveSwap` applies a `SwapButton`. A key bound to the slot's own binding is a conflict without a swap (answer 18); a key on the action's other slot swaps the two slots (answer 19). Test `qol-controls-swap` (`tests/qol/controls_swap_test.cpp`, includes the written `controls.cfg`).
Done: `src/qol/keep_settings.*` (QOLSET-77, no SDL, not wired): `KeepSettings(Settings&, desktop, applier)` with `begin(previous, current, now)`, `tick(now)`, `confirm`, `cancel`, `choose`, `secondsLeft` (rounded up from 15), `text` ("Keep these settings? Reverting in N seconds") and `dialog()` (`KeepDialog`, default focus Keep). The clock is the seconds argument. Revert restores window mode and resolution in `Settings` (new `Settings::clearResolution`) and calls the applier once; a second `begin` while open restarts the 15 seconds and keeps the first previous values; equal previous and current values open no dialog (answer 21); "1 second" is singular (answer 20). Answers 17 to 21: [qol-settings-drawing.md §2026-10-30T21:56:04](../../design/qol-settings-drawing.md#ts-20261030-215604). Test `qol-keep-settings` (`tests/qol/keep_settings_test.cpp`).
