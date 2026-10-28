# QoL minimap

## 1. Scope

The map of the bottom screen as a minimap in a corner of the full-screen view. Plan: [qol.md](../qol.md). Toggle: own switch in the settings menu. The original map is [hud-ui.md](hud-ui.md).

## 2. Status

NOT_STARTED, opened 2026-10-28T09:21:35+01:00. No code.

## 3. Verified behaviour

None yet. Every behaviour is tested first in original mode (all QoL off, the port's tests unchanged), then with the feature on ([qol rules](../../rules/qol.md)).

## 4. Stand-ins

None.

## 5. Open items

- [ ] QOLMAP-1 — The minimap, drawn from the same map data as the original bottom-screen map: round, top-right corner, about one fifth of the screen height, rotating with the camera, fixed zoom like the original map, the same icons and objective markers as the original map, hidden in cutscenes and panels; own toggle. needs: QOLSET-1, QOLFS-1. blocks: QOLMAP-4. evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).
- [ ] QOLMAP-4 — Open question for the user: whether the minimap stays its own toggle in the Quality of life tab, or follows the master button only like the bottom screen (and so shows only in Improved). needs: QOLMAP-1. evidence: [qol-screen-layout.md §2026-10-28T09:48:15](../../design/qol-screen-layout.md#ts-20261028-094815).

## 6. Findings summary

Design: [qol-screen-layout.md](../../design/qol-screen-layout.md).

## 7. Code and tests

Not written yet. Tests: one set with the feature off (unchanged behaviour) and one with it on, pinned to the design entry.
