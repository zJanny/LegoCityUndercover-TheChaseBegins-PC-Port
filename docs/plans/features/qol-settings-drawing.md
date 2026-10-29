# QoL settings menu drawing

## 1. Scope

Drawing of the PC settings menu ([qol-settings-menu](qol-settings-menu.md)) with the original Options-page art: the page scene of the round menu layout, the scale into the window, the hit areas and the mapping of keys, mouse and wheel onto the menu model's events. Plan: [qol.md](../qol.md). Not a QoL feature of its own: the menu is the toggle mechanism.

## 2. Status

IN_PROGRESS, opened 2026-10-29T16:00:26+01:00 (split from qol-settings-menu). The round form of the Game, Display and Quality of life tabs, the scale rule, the hit areas, the pointer and wheel mapping and a real-data draw test exist (QOLSET-33 part 1); the list form, the notices and the question of the Controls tab (QOLSET-40) and the window hook with the input wiring (QOLSET-41) do not.

## 3. Verified behaviour

None against a running game yet: the page is drawn in tests only, nothing calls it in `chase-game`. With every QoL feature off nothing of it runs.

## 4. Stand-ins

- QOLSET-S1 The Controls tab draws the page frame (title, tab name, arrows, page number, Back and Select) without rows and takes only the frame's hit areas (`standInListForm`) until the list form builders are joined. needs: QOLSET-40. evidence: [qol-settings-drawing.md §2026-10-29T12:11:55](../../design/qol-settings-drawing.md#ts-20261029-121155).

- QOLSET-S2 — Controls list form (`menu_list_page.cpp`, 20261029-qolmenu2-B): bar pitch below the subtitle bar, highlight and `Menu_List` offsets, dark text of the selected row on `TEXT_Icon`, "Press a key or button for X", English "Yes"/"No", notice line pitch 27 and `PageNumber` at the top right are chosen, not read. evidence: [ui-layouts.md §2026-10-29T15:54:31](../../re/ui-layouts.md#ts-20261029-155431).

## 5. Open items

- [ ] QOLSET-33 — Drawing code of the settings menu from the decided spec. Part 1 done (`menu_page`, `menu_hit`, tests `qol-menu-hit`, `qol-menu-page-real-data`): round form scene of the Game, Display and Quality of life tabs (title, tab name, arrows, page number, five pictures with the selected row in the centre, name plate, Back and Select), the scale into the window with black bars, the hit areas and the key, pointer and wheel mapping. Left: the list form, notices and question (QOLSET-40), the window hook and input wiring (QOLSET-41), the text fit (closed QOLSET-35: the page draws row text in `TEXT_Icon`, 256 pixels wide, as the port Options page does, not in `TEXT_ItemName`, 182). needs: QOLSET-39, QOLSET-40, QOLSET-41. blocks: QOLSET-2. evidence: [qol-settings-drawing.md §2026-10-29T12:11:55](../../design/qol-settings-drawing.md#ts-20261029-121155).
- [ ] QOLSET-43 — Controls bars, the user's answer "Two lines per bar" (action on top, the two slots smaller underneath, fewer rows per page): not built, the port cannot draw a layout text smaller; the list form still shows one line per bar. needs: QOLSET-40, HUD-96. evidence: [qol-settings-drawing.md §2026-10-29T12:11:55](../../design/qol-settings-drawing.md#ts-20261029-121155).
- [ ] QOLSET-44 — The English "Yes" and "No" of the Reset all question become the gametext words (the model has no gametext); the notice and question builders are joined in QOLSET-40. needs: QOLSET-40. evidence: [ui-layouts.md §2026-10-29T15:54:31](../../re/ui-layouts.md#ts-20261029-155431).
- [ ] QOLSET-40 — Join the list form builders of `src/qol/menu_list_page.hpp` (`listPageScene`, `listHitAreas`) into `ListForm` in place of `standInListForm`, and draw the notice and Reset all question scenes (`noticeScene`, `questionScene`) over the page while the Controls tab waits or asks. blocks: QOLSET-S1, QOLSET-33, QOLSET-43, QOLSET-44. evidence: [qol-settings-drawing.md §2026-10-29T12:11:55](../../design/qol-settings-drawing.md#ts-20261029-121155).
- [ ] QOLSET-41 — Window hook: draw the page with `drawMenuPage` into a window-sized canvas while the menu is open (title-screen and pause hooks), feed mouse motion and clicks through `pagePoint` and `MenuPointer`, the wheel through `eventForWheel` and the keys through `eventForKey` into `MenuModel`, and pass the live `GameOptions` so the Game rows show their values. blocks: QOLSET-33. evidence: [qol-settings-drawing.md §2026-10-29T12:11:55](../../design/qol-settings-drawing.md#ts-20261029-121155).

## 6. Findings summary

Spec: [qol-settings-drawing.md §2026-10-29T12:11:55](../../design/qol-settings-drawing.md#ts-20261029-121155) (with the user's answers of 2026-10-29 on the tap circle, hover, name plate and Game tab labels). Layout pieces: [ui-layouts.md §2026-10-29T11:50:58](../../re/ui-layouts.md#ts-20261029-115058). Port capabilities used: HUD-95 in [hud-ui](hud-ui.md).

## 7. Code and tests

- `src/qol/menu_list_page.hpp`/`.cpp` (library `chase_qol`): Controls list, notice and question scenes and their hit areas; tests `qol-menu-list-page`, `qol-menu-list-page-real-data`.
- `src/qol/menu_hit.hpp`/`.cpp` (library `chase_qol`): `menuPageRect` and `pagePoint` (scale min(w/320, h/240), page centred), `HitArea` (box, round flag, click and hover events; shared with the list form), `roundHitAreas` and `frameHitAreas` (tap circles of the three visible slots, Back, Select, the `Pageslots` arrows, the subtitle bar hover `tabMove`), `MenuPointer` (hover only on entering an area), `eventForWheel`. Test `qol-menu-hit` (`tests/qol/menu_hit_test.cpp`): the spec's three window sizes, a tall window, the areas, hover on entering, the key and wheel mapping.
- `src/qol/menu_page.hpp`/`.cpp` (library `chase_qol_menu`, over `chase_frontend`): `menuPageSetup` (round timings and tap circles of the pause setup, Options-page labels, gametext `ON`/`OFF`, element boxes from the layouts), `rowPlateText`, `menuPageScene` (through the port's `RoundMenu`, opened at the selected row with its storyboards placed at their end), `menuFrameScene`, `menuShortcutsScene`, `ListForm`, `pageScene`, `pageHitAreas`, `drawMenuPage` (page canvas at window resolution, text through the node scale, blitted centred on black). Test `qol-menu-page-real-data` (`tests/qol/menu_page_real_test.cpp`, label `needs-game-data`).
