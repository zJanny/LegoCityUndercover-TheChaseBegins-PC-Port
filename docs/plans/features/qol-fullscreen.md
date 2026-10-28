# QoL true full screen

## 1. Scope

The top screen as the whole window: the 3D view widened up to 16:9 (same vertical field of view, more world left and right; wider windows get black side bars), the HUD anchored to the corners, the edges checked for pop-in. Plan: [qol.md](../qol.md). Toggle: own switch in the settings menu ([qol-settings-menu.md](qol-settings-menu.md)).

## 2. Status

NOT_STARTED, opened 2026-10-28T09:21:35+01:00. No code.

## 3. Verified behaviour

None yet. Every behaviour is tested first in original mode (all QoL off, the port's tests unchanged), then with the feature on ([qol rules](../../rules/qol.md)).

## 4. Stand-ins

None.

## 5. Open items

- [ ] QOLFS-1 — The widened full-window view up to 16:9, black side bars beyond with the same vertical field of view, switched by its own toggle; with the toggle off the window is today's top over bottom. needs: QOLSET-1. blocks: QOLFS-2, QOLFS-3, QOLMAP-1, QOLBOT-1, QOLFS-4. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLFS-2 — HUD parts anchored to the window corners. needs: QOLFS-1. blocks: QOLFS-5, QOLFS-6. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLFS-3 — Check the widened edges for pop-in (rooms, objects, pedestrians, shadows, fades) at the widest supported aspect, 16:9. needs: QOLFS-1. evidence: [qol-screen-layout.md §2026-10-28T09:21:33](../../design/qol-screen-layout.md#ts-20261028-092133).
- [ ] QOLFS-4 — Open question for the user: how the view behaves in windows narrower than 5:3 (keep the original 5:3 view with bars above and below, or show the window's aspect). needs: QOLFS-1. evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).
- [ ] QOLFS-5 — Open question for the user: where each HUD part goes in the corners (hearts, character picture, studs, ability and counters) and whether the HUD can be scaled. needs: QOLFS-2. evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).
- [ ] QOLFS-6 — Open question for the user: whether the widened view and the corner HUD are one toggle or two. needs: QOLFS-2. evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).

## 6. Findings summary

Design: [qol-screen-layout.md](../../design/qol-screen-layout.md).

## 7. Code and tests

Not written yet. Tests: one set with the feature off (unchanged behaviour) and one with it on, pinned to the design entry.
