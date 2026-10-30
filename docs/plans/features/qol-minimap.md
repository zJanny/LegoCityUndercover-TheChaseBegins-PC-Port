# QoL minimap

## 1. Scope

The map of the bottom screen as a minimap in a corner of the full-screen view. Plan: [qol.md](../qol.md). Switched by the master button only (Improved). The original map is [hud-ui.md](hud-ui.md).

## 2. Status

IN_PROGRESS. The round minimap is drawn from the original map canvas (20261030-qolpanels-B, 2026-10-30T15:39:42+01:00); panels other than the scanner, margin and overlap with the top-right HUD remain.

## 3. Verified behaviour

`qol-minimap` pins the geometry (diameter one fifth of the window height, flush top-right), the round mask and that Original and Custom draw nothing. Every behaviour is tested first in original mode (all QoL off, the port's tests unchanged), then with the feature on ([qol rules](../../rules/qol.md)).

## 4. Stand-ins

- QOLMAP-S1 — The circle touches the window's top and right edges with no margin, and shows the 240 px disc around the player of the original 320x240 map canvas (the original zoom, the characters button lies outside the disc). The spec gives no margin or disc size. evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).

## 5. Open items

- [ ] QOLMAP-5 — at 16:9 the circle covers the top-right corner where the corner HUD puts the studs and counters; the user decided 2026-10-30 "Below the counters": still top-right, moved down so it starts under the studs and counters, with a small margin (replaces stand-in QOLMAP-S1's flush placement); not built. needs: QOLMAP-1.  evidence: [qol-screen-layout.md §2026-10-28T09:37:31](../../design/qol-screen-layout.md#ts-20261028-093731).
- [ ] QOLMAP-6 — Hide the minimap in the other panels (dialog boxes, scene change page, fast-travel map, pause menu); only the scanner overlay and cutscenes hide it now. needs: QOLMAP-1.  evidence: [qol-screen-layout.md §2026-10-28T09:37:31](../../design/qol-screen-layout.md#ts-20261028-093731).
- [ ] QOLMAP-7 — Compare the round minimap with a gameplay picture (rotation, icons and objective markers inside the disc) on real data. needs: QOLMAP-1.  evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).

## 6. Findings summary

Design: [qol-screen-layout.md](../../design/qol-screen-layout.md).

## 7. Code and tests

`src/qol/minimap.*` (library `chase_qol_minimap`): `qol::drawRoundMinimap` crops the disc out of the canvas that the port's `drawMinimap` filled (map, rotation, icons and objective markers are the original's) and draws it through `HudOverlay::drawTop`; hook in `LevelSession::draw`. Test `qol-minimap` (`tests/qol/minimap_test.cpp`).
