# QoL bottom-screen content

## 1. Scope

The rest of the bottom screen as PC forms: the characters button, character swap and the fast-travel map as full-window panels clicked with the mouse, the scanner, build-it, shop and award viewer pages, and the pause screen as a PC menu. The bottom screen is hidden when these features are on. Plan: [qol.md](../qol.md). The original screens are [ui-menus.md](ui-menus.md) and [level-objects-fast-travel.md](level-objects-fast-travel.md); a panel can only show what the port has recovered.

## 2. Status

IN_PROGRESS, opened 2026-10-28T09:21:35+01:00. QOLBOT-1 is built: `qol::hiddenScreens` (`src/qol/bottom_hook.*`) hides the bottom screen with `master Improved` (user: "QOLBOT-1 first", "Hide anyway", "5:3 centred", 2026-10-29); the front end panel (QOLBOT-19) and its row and button clicks (QOLBOT-22) exist; movies show no panel and draw their subtitle bubble over the picture (QOLBOT-28, QOLBOT-29); every front-end page shows or hides its panel by a page rule table (QOLBOT-21); the other panels do not exist yet.

## 3. Verified behaviour

None yet. Every behaviour is tested first in original mode (all QoL off, the port's tests unchanged), then with the feature on ([qol rules](../../rules/qol.md)).

## 4. Stand-ins

- QOLBOT-S1 — With Improved the bottom screen is hidden while its pages (pause menu, characters button, map, scanner, build-it, award viewer) still live only on the bottom canvas, so they have no input or picture until the panels QOLBOT-2 to QOLBOT-10 exist. Approved by the user on 2026-10-29 ("Hide anyway"). Replaced by: QOLBOT-2 to QOLBOT-10. evidence: [qol-screen-layout.md §2026-10-29T11:42:22](../../design/qol-screen-layout.md#ts-20261029-114222).

## 5. Open items

- [ ] QOLBOT-19 — Done: the front end presenter in Improved (top picture fitted 5:3, darkened, the 320x240 bottom page as a centred 4:3 panel), the window-to-canvas mapping and the slot page "PC Settings" bar click through the panel; original mode unchanged. The row and button clicks are QOLBOT-22. needs: QOLBOT-1. blocks: QOLBOT-21, QOLBOT-28. evidence: [qol-screen-layout.md §2026-10-30T14:07:44](../../design/qol-screen-layout.md#ts-20261030-140744); `src/qol/front_panel.hpp`.
- [ ] QOLBOT-25 — The Y picture of a used slot is clickable and presses Y, but the front end has no delete page yet, so the press does nothing. needs: FE-30. evidence: [front-end.md §2026-11-02T15:04:23](../../re/front-end.md#ts-20261102-150423).
- [ ] QOLBOT-32 — A left click skips the fusion movie and the mission card only where a button already skips (not on a first run); the port pages expose no skip target, so the click is not handled by the panel: it passes on as the bound mouse button (A by default), which skips the fusion movie (not on a first run) and the mission card like the key does. needs: QOLBOT-21. evidence: [qol-screen-layout.md §2026-10-30T14:07:44](../../design/qol-screen-layout.md#ts-20261030-140744).
- [ ] QOLBOT-2 — The characters button as a full-window panel clicked with the mouse (the original button is UI-1). needs: QOLBOT-1, UI-1. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLBOT-3 — Character swap as a full-window panel (original screen not ported yet). needs: QOLBOT-1. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLBOT-4 — The fast-travel map as a full-window panel (original LEVOBJ-120). needs: QOLBOT-1, LEVOBJ-120. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLBOT-5 — The pause screen as a PC menu with Resume, Options (the original page), Credits, Exit and PC Settings. needs: QOLBOT-1, QOLSET-2. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLBOT-7 — The scanner's bottom screen as a full-window panel clicked with the mouse (original: SCAN-11). needs: QOLBOT-1, SCAN-11. evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).
- [ ] QOLBOT-8 — The build-it bottom screen as a full-window panel clicked with the mouse (original build menu: HUD-85). needs: QOLBOT-1, HUD-85. evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).
- [ ] QOLBOT-9 — The shop as a full-window panel clicked with the mouse (the original shop page is not ported). needs: QOLBOT-1. evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).
- [ ] QOLBOT-10 — The award viewer as a full-window panel clicked with the mouse (original: UI-12). needs: QOLBOT-1, UI-12. evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).

## 6. Findings summary

Design: [qol-screen-layout.md](../../design/qol-screen-layout.md). Page geometry of the front end: [front-end.md §2026-11-02T15:04:23](../../re/front-end.md#ts-20261102-150423).

## 7. Code and tests

`src/qol/bottom_hook.hpp`, `src/qol/bottom_hook.cpp` (in `chase_qol`): `bottomScreenHidden`, `minimapOn`, `hiddenScreens`, `hiddenScreensFor`, `hiddenTopArea`; hook `game_hud::screensFor` (the touch tracker reads the same rectangle). Test `qol-bottom-hook` (`tests/qol/bottom_hook_test.cpp`). Front end panel (QOLBOT-19): `front_panel.hpp`/`front_panel.cpp` (`frontPanels`, `darken`, `classifyClick`; panel rectangle is `menuPageRect`), `front_panel_present.hpp`/`front_panel_present.cpp` (`presentFrontPanels`, over the port's `FrontPresenter::presentBoxes`), `frontBottomScreen`/`frontBottomPoint` in `menu_window_sdl.cpp` follow the panel; tests `qol-front-panel` and `qol-front-panel-point` (`tests/qol/`). Row and button clicks (QOLBOT-22): `front_panel_clicks.hpp`/`.cpp` (`panelClick`, `FrontClickQueue`), `front_panel_clicks_sdl.hpp`/`.cpp` (`FrontClicks`, `clickTargets`); the port side is `src/frontend/page_targets.*` (`alertPageTargets`, `languageListTargets`, `slotPageTargets`, `FrontEnd::targets()`) and `FrontInput::selectRow`; tests `qol-front-panel-clicks`, `qol-front-panel-clicks-real-data` and `frontend-select-row`. Movies (QOLBOT-28, QOLBOT-29): `movie_subtitle_overlay.hpp`/`.cpp` (`movieBubbleOffset`, `frontMoviePanelsFor`, `drawMovieSubtitles`; library `chase_qol_hud`), called by `presentFrontPanels` and the hook in `LevelSession::draw`; the movie camera kind is `CameraKind::Movie` and the minimap hides with `MinimapPanels::movie`; tests `qol-movie-subtitle-overlay`, `qol-movie-subtitle-overlay-real-data`, `qol-view-hook`, `qol-minimap`. Page rules (QOLBOT-21): `front_page_rules.hpp`/`.cpp` (`frontPageView`, `frontPagePanelsFor`, `loadingSpinnerFor`), `front_loading_spinner.hpp`/`.cpp` (`drawFrontLoadingSpinner`, over the port's `drawSaveSpinnerAt`; QOLBOT-31); test `qol-front-page-rules`. Other panels: not written yet. Tests: one set with the feature off (unchanged behaviour) and one with it on, pinned to the design entry.
