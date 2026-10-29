# QoL bottom-screen content

## 1. Scope

The rest of the bottom screen as PC forms: the characters button, character swap and the fast-travel map as full-window panels clicked with the mouse, the scanner, build-it, shop and award viewer pages, and the pause screen as a PC menu. The bottom screen is hidden when these features are on. Plan: [qol.md](../qol.md). The original screens are [ui-menus.md](ui-menus.md) and [level-objects-fast-travel.md](level-objects-fast-travel.md); a panel can only show what the port has recovered.

## 2. Status

IN_PROGRESS, opened 2026-10-28T09:21:35+01:00. QOLBOT-1 is built: `qol::hiddenScreens` (`src/qol/bottom_hook.*`) hides the bottom screen with `master Improved` (user: "QOLBOT-1 first", "Hide anyway", "5:3 centred", 2026-10-29); the panels do not exist yet.

## 3. Verified behaviour

None yet. Every behaviour is tested first in original mode (all QoL off, the port's tests unchanged), then with the feature on ([qol rules](../../rules/qol.md)).

## 4. Stand-ins

- QOLBOT-S1 — With Improved the bottom screen is hidden while its pages (pause menu, characters button, map, scanner, build-it, award viewer) still live only on the bottom canvas, so they have no input or picture until the panels QOLBOT-2 to QOLBOT-10 exist. Approved by the user on 2026-10-29 ("Hide anyway"). Replaced by: QOLBOT-2 to QOLBOT-10. evidence: [qol-screen-layout.md §2026-10-29T11:42:22](../../design/qol-screen-layout.md#ts-20261029-114222).

## 5. Open items

- [ ] QOLBOT-19 — The front end screens (title, menus) draw their own two-screen layout and still show the bottom screen with Improved; decide with the user whether they follow the hiding. needs: QOLBOT-1. evidence: `src/game_front.hpp` `presenter.present`.
- [ ] QOLBOT-2 — The characters button as a full-window panel clicked with the mouse (the original button is UI-1). needs: QOLBOT-1, UI-1. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLBOT-3 — Character swap as a full-window panel (original screen not ported yet). needs: QOLBOT-1. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLBOT-4 — The fast-travel map as a full-window panel (original LEVOBJ-120). needs: QOLBOT-1, LEVOBJ-120. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLBOT-5 — The pause screen as a PC menu with Resume, Options (the original page), Credits, Exit and PC Settings. needs: QOLBOT-1, QOLSET-2. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLBOT-7 — The scanner's bottom screen as a full-window panel clicked with the mouse (original: SCAN-11). needs: QOLBOT-1, SCAN-11. evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).
- [ ] QOLBOT-8 — The build-it bottom screen as a full-window panel clicked with the mouse (original build menu: HUD-85). needs: QOLBOT-1, HUD-85. evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).
- [ ] QOLBOT-9 — The shop as a full-window panel clicked with the mouse (the original shop page is not ported). needs: QOLBOT-1. evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).
- [ ] QOLBOT-10 — The award viewer as a full-window panel clicked with the mouse (original: UI-12). needs: QOLBOT-1, UI-12. evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).

## 6. Findings summary

Design: [qol-screen-layout.md](../../design/qol-screen-layout.md).

## 7. Code and tests

`src/qol/bottom_hook.hpp`, `src/qol/bottom_hook.cpp` (in `chase_qol`): `bottomScreenHidden`, `minimapOn`, `hiddenScreens`, `hiddenScreensFor`, `hiddenTopArea`; hook `game_hud::screensFor` (the touch tracker reads the same rectangle). Test `qol-bottom-hook` (`tests/qol/bottom_hook_test.cpp`). Panels: not written yet. Tests: one set with the feature off (unchanged behaviour) and one with it on, pinned to the design entry.
