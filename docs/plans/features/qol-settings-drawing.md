# QoL settings menu drawing

## 1. Scope

The entry points and stores of the PC settings menu ([qol-settings-menu](qol-settings-menu.md)): the "PC Settings" bar of the slot page and the pause entry with the game art, and the stores behind the open menu. The menu itself is drawn by [qol-settings-ui](qol-settings-ui.md). Plan: [qol.md](../qol.md). Not a QoL feature of its own: the menu is the toggle mechanism.

## 2. Status

IN_PROGRESS, opened 2026-10-29T16:00:26+01:00 (split from qol-settings-menu). The art-based drawing (round form, list form, page scale, hit areas, redraw cache) was removed on 2026-11-01 by QOLSET-79; the menu is now drawn by the redesign in [qol-settings-ui](qol-settings-ui.md). What stays here: the "PC Settings" bar of the slot page and the pause entry, which keep the game art (answer 5 of the design entry), the stores behind the open menu and the stand-ins S4 and S6.

## 3. Verified behaviour

None against the original (the menu is not part of it). In `chase-game` the slot list shows the "PC Settings" button, a left click on it opens the menu over the whole window and the front end stops (checked by 20261029-qolhook-B under Xvfb, and by `qol-front-menu-real-data`); on the other front-end screens and in levels nothing of it runs or draws.

## 4. Stand-ins

- QOLSET-S4 — While the menu is open over the front end, UI sounds that were already playing finish: the audio device is not paused, only the front end's update and new sounds stop ("movies and sounds hold"). evidence: [qol-settings.md §2026-10-29T18:06:35](../../design/qol-settings.md#ts-20261029-180635).

- QOLSET-S7 — The Language row of the open menu is the arrow selector of the spec (answer 4), not the item grid of flags of the pause screen; the chosen language goes through the port's `LanguagesPage` when the menu closes (options and save), and the texts and pictures of the other languages are not loaded (one terminal line says so, as on the pause screen). evidence: [ui-screens-pause.md §2026-10-30T14:16:01](../../re/ui-screens-pause.md#ts-20261030-141601).

## 5. Open items


## 6. Findings summary

Spec: [qol-settings-drawing.md §2026-10-30T21:56:04](../../design/qol-settings-drawing.md#ts-20261030-215604) (with the user's answers of 2026-10-29 on the tap circle, hover, name plate and Game tab labels). Layout pieces: [ui-layouts.md §2026-10-29T11:50:58](../../re/ui-layouts.md#ts-20261029-115058). Port capabilities used: HUD-95 in [hud-ui](hud-ui.md). Playtest of the menu opened from the slot page: [playtests-qol.md §2026-10-29T19:31:01](../../design/playtests-qol.md#ts-20261029-193101).

## 7. Code and tests

- The menu itself (controller, pages, painter, presenter): [qol-settings-ui](qol-settings-ui.md).
- Checked by `qol-front-menu-real-data` against the real slot page (20261103-large-F): the bar draws rows 167..215 and columns 48..273; only its glow lies over the bottom of slot 2; the shortcut row and every pixel outside the art are unchanged; clicks inside the body open the menu, clicks above or below it go to the front end.
- `src/qol/menu_stores.hpp`/`.cpp` (library `chase_qol`): `StoresPorts` (settings path, display modes, desktop size, window, controls and controls-changed callbacks, Game port factory), `MenuEffects` and `MenuStores` (copy of the active store for the pages, `open` makes the controls and Game ports once, `commit` of the effects of each event: `config/qol.cfg` and the active store, the window, the controls callback, the Game port's gains, Languages request and save on close); `WindowController::adopt`; test `qol-menu-stores` (no game data).
- `src/qol/menu_window_sdl.hpp`/`.cpp` (library `chase_qol_window`, over SDL and `chase_render`): `FrontMenu` with the slot list button (`frontButtonScene`, `opensMenu`, `frontButtonBox`), the bottom-screen mapping of the front presenter (`frontBottomScreen`), the gametext language names, and the open menu through `UiMenuWindow`; hook forwards `frontMenuConsumes`, `frontMenuPresented`, `drawFrontMenuButton`. Test `qol-front-menu-real-data` (label `needs-game-data`).
- `src/qol/slot_page_entry.hpp`/`.cpp` (library `chase_qol`): `SlotPageEntry`, the focus rules of the bar; `FrontMenu::steer` and `entryFocused` in `menu_window_sdl.*`; the port capability `FrontEnd::setSlotsUnselected` (no slot highlighted, default off); tests `qol-slot-page-entry` and `qol-slot-page-entry-real-data` (`needs-game-data`).
- `src/qol/front_panel.hpp`/`.cpp` (library `chase_qol`): `menuPageRect` and `pagePoint` (scale min(w/320, h/240), page centred; used by the front panel of Improved), `Box`; test `qol-front-panel`.
