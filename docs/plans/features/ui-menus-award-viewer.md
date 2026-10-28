# UI menus: award viewer

## 1. Scope

The award viewer of the police station: its entry through the use spot `Use_AwardViewer` and the script native AwardViewer, its round menu (Overview and the seven levels) with the Back shortcut, and the top screen with the shop picture, the award models and the bronze, silver and gold counts of the save slot. Split from [ui-menus.md](ui-menus.md), which holds the round menu, the pause screen and the other menus; the closed item UI-12 is in [ui-menus.closed.md](ui-menus.closed.md). The census entry is [ui-screens.md](../../re/ui-screens.md). Bottom-screen UI is used by clicking it with the mouse.

## 2. Status

IN_PROGRESS, updated 2026-10-28T23:41:28+01:00. A on the award viewer's use spot in the police station opens the award viewer in `chase-game`: the round menu with Overview and the seven levels and the Back shortcut, the shop picture and the bronze, silver and gold counts of the save slot on the top screen. Start does nothing while it is open; B closes it. The award models and the mode fades are not reproduced, and no oracle picture has been compared yet.

## 3. Verified behaviour

- Award viewer: the use spot `Use_AwardViewer` runs `Use_AwardViewerScript`, AwardViewer opens the screen in the next update; items, texts, pictures, Back only, A stores the selection without a sound, B plays 0x11 and closes after TransitionOff_Round; the counts are 0x1f61f0 over the save's category counters, the table totals, the missions done and finished levels (`award-viewer`, `award-viewer-real-data`, `game-award-viewer`): [ui-screens.md §2026-10-28T22:55](../../re/ui-screens.md#ts-20261028-225513).

## 4. Stand-ins

- UI-S12 The award viewer opens in the update after the native and closes as soon as the round menu is idle: the mode fades (0.5 s each), the gate of the push and the other calls of the mode's enter and leave are not reproduced; a Start press of the update in which the native ran is dropped when the viewer opens (which push wins in the original is not read); the saved selection lives as long as the level session (the original's page object is static). needs: UI-27. evidence: [ui-screens.md §2026-10-28T22:55](../../re/ui-screens.md#ts-20261028-225513).
- UI-S13 The award viewer's top screen draws no award models, and its counts use frontendfont, white, centred on their point (the original's font state is not read). needs: UI-26, UI-27. evidence: [ui-screens.md §2026-10-28T22:55](../../re/ui-screens.md#ts-20261028-225513).

## 5. Open items

- [ ] UI-26 — Award viewer models: the bronze, silver and gold award models with their shine model and its looped animation, seen through the viewer's own camera and lights. blocks: UI-S13. evidence: [ui-screens.md §2026-10-28T22:55](../../re/ui-screens.md#ts-20261028-225513).
- [ ] UI-27 — Award mode push and pop: the 0.5 s fades of the push and pop, the gate of the push, the unread calls of the mode's enter and leave, the second draw branch and the count text's font state. blocks: UI-S12, UI-S13. evidence: [ui-screens.md §2026-10-28T22:55](../../re/ui-screens.md#ts-20261028-225513).
- [ ] UI-28 — Compare the award viewer with an oracle picture (needs a save state at the award viewer spot of the police station, requested from the user). evidence: [ui-screens.md §2026-10-28T22:55](../../re/ui-screens.md#ts-20261028-225513).

## 6. Findings summary

- Award viewer: entry path, mode 0x514030, page 0x514058, counts 0x1f61f0, draw: [ui-screens.md §2026-10-28T22:55](../../re/ui-screens.md#ts-20261028-225513).

## 7. Code and tests

- Award viewer: `runtime/award_viewer.*` (counts 0x1f61f0, the AwardViewer request of `ScriptRunner`), `frontend/award_viewer.*` (`AwardViewer`), glue `src/game_award_viewer.cpp`.
- CTest: `award-viewer`, `award-viewer-real-data`, `game-award-viewer` (slow).
