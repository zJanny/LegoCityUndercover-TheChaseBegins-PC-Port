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

- [ ] QOLBOT-1 — Hide the bottom screen when its features are on; with them off the layout is unchanged. needs: QOLSET-1, QOLFS-1. blocks: QOLBOT-2, QOLBOT-3, QOLBOT-4, QOLBOT-5, QOLBOT-6, QOLBOT-7, QOLBOT-8, QOLBOT-9, QOLBOT-10, QOLBOT-11, QOLBOT-13. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLBOT-2 — The characters button as a full-window panel clicked with the mouse (the original button is UI-1). needs: QOLBOT-1, UI-1. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLBOT-3 — Character swap as a full-window panel (original screen not ported yet). needs: QOLBOT-1. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLBOT-4 — The fast-travel map as a full-window panel (original LEVOBJ-120). needs: QOLBOT-1, LEVOBJ-120. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLBOT-5 — The pause screen and Options as PC menus. needs: QOLBOT-1, QOLSET-2. blocks: QOLBOT-12. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLBOT-7 — The scanner's bottom screen as a full-window panel clicked with the mouse (original: SCAN-11). needs: QOLBOT-1, SCAN-11. evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).
- [ ] QOLBOT-8 — The build-it bottom screen as a full-window panel clicked with the mouse (original build menu: HUD-85). needs: QOLBOT-1, HUD-85. evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).
- [ ] QOLBOT-9 — The shop as a full-window panel clicked with the mouse (the original shop page is not ported). needs: QOLBOT-1. evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).
- [ ] QOLBOT-10 — The award viewer as a full-window panel clicked with the mouse (original: UI-12). needs: QOLBOT-1, UI-12. evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).
- [ ] QOLBOT-11 — Open question for the user: how the bottom-screen features are switched (one toggle that hides the bottom screen and turns every panel on, or one toggle per panel) and what happens when the minimap is off but the bottom screen is hidden. needs: QOLBOT-1. evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).
- [ ] QOLBOT-12 — Open question for the user: what the pause screen as a PC menu holds (Resume, Options, Credits, Exit, PC Settings) and whether its Options entry keeps opening the original page. needs: QOLBOT-5. evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).
- [ ] QOLBOT-13 — Open question for the user: the look of the full-window panels (the original game's art and fonts, or a plain PC style). needs: QOLBOT-1. evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).

## 6. Findings summary

Design: [qol-screen-layout.md](../../design/qol-screen-layout.md).

## 7. Code and tests

Not written yet. Tests: one set with the feature off (unchanged behaviour) and one with it on, pinned to the design entry.
