# QoL bottom-screen content

## 1. Scope

The rest of the bottom screen as PC forms: the characters button, character swap and the fast-travel map as full-window panels clicked with the mouse, the scanner, build-it, shop and award viewer pages, and the pause screen as a PC menu. The bottom screen is hidden when these features are on. Plan: [qol.md](../qol.md). The original screens are [ui-menus.md](ui-menus.md) and [level-objects-fast-travel.md](level-objects-fast-travel.md); a panel can only show what the port has recovered.

## 2. Status

NOT_STARTED, opened 2026-10-28T09:21:35+01:00. No code.

## 3. Verified behaviour

None yet. Every behaviour is tested first in original mode (all QoL off, the port's tests unchanged), then with the feature on ([qol rules](../../rules/qol.md)).

## 4. Stand-ins

None.

## 5. Open items

- [ ] QOLBOT-1 — Hide the bottom screen when its features are on; one toggle per panel; hiding the bottom screen requires the minimap on and turns every panel toggle on too; with the features off the layout is unchanged; all panels use the original game's art and fonts. needs: QOLSET-1, QOLFS-1. blocks: QOLBOT-2, QOLBOT-3, QOLBOT-4, QOLBOT-5, QOLBOT-6, QOLBOT-7, QOLBOT-8, QOLBOT-9, QOLBOT-10, QOLBOT-15, QOLBOT-16, QOLBOT-17. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLBOT-2 — The characters button as a full-window panel clicked with the mouse (the original button is UI-1). needs: QOLBOT-1, UI-1. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLBOT-3 — Character swap as a full-window panel (original screen not ported yet). needs: QOLBOT-1. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLBOT-4 — The fast-travel map as a full-window panel (original LEVOBJ-120). needs: QOLBOT-1, LEVOBJ-120. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLBOT-5 — The pause screen as a PC menu with Resume, Options (the original page), Credits, Exit and PC Settings. needs: QOLBOT-1, QOLSET-2. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLBOT-7 — The scanner's bottom screen as a full-window panel clicked with the mouse (original: SCAN-11). needs: QOLBOT-1, SCAN-11. evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).
- [ ] QOLBOT-8 — The build-it bottom screen as a full-window panel clicked with the mouse (original build menu: HUD-85). needs: QOLBOT-1, HUD-85. evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).
- [ ] QOLBOT-9 — The shop as a full-window panel clicked with the mouse (the original shop page is not ported). needs: QOLBOT-1. evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).
- [ ] QOLBOT-10 — The award viewer as a full-window panel clicked with the mouse (original: UI-12). needs: QOLBOT-1, UI-12. evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).
- [ ] QOLBOT-15 — Open question for the user: after the bottom screen is hidden (every panel toggle on), what happens when the player turns one panel toggle off (the bottom screen is shown again as a whole, it is shown for that page only, or the toggle stays locked on while hiding is on). needs: QOLBOT-1. evidence: [qol-screen-layout.md §2026-10-28T09:40:42](../../design/qol-screen-layout.md#ts-20261028-094042).
- [ ] QOLBOT-16 — Open question for the user: what a panel toggle does while the bottom screen is visible (the panel replaces that page's bottom-screen version while the bottom screen stays, the panel toggles act only when hiding is on, or they are shown only when hiding is on). needs: QOLBOT-1. evidence: [qol-screen-layout.md §2026-10-28T09:40:42](../../design/qol-screen-layout.md#ts-20261028-094042).
- [ ] QOLBOT-17 — Open question for the user: whether "hide the bottom screen" is its own toggle in the Quality of life tab (the one that forces every panel on), or only the result of the master button and the panel toggles. needs: QOLBOT-1. evidence: [qol-screen-layout.md §2026-10-28T09:40:42](../../design/qol-screen-layout.md#ts-20261028-094042).

## 6. Findings summary

Design: [qol-screen-layout.md](../../design/qol-screen-layout.md).

## 7. Code and tests

Not written yet. Tests: one set with the feature off (unchanged behaviour) and one with it on, pinned to the design entry.
