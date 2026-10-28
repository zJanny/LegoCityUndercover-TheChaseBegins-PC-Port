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

- [ ] QOLMAP-1 — The minimap, drawn from the same map data as the original bottom-screen map: round, top-right corner, about one fifth of the screen height, rotating with the camera, zoom like the original map; own toggle. needs: QOLSET-1, QOLFS-1. evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).

## 6. Findings summary

Design: [qol-screen-layout.md](../../design/qol-screen-layout.md).

## 7. Code and tests

Not written yet. Tests: one set with the feature off (unchanged behaviour) and one with it on, pinned to the design entry.
