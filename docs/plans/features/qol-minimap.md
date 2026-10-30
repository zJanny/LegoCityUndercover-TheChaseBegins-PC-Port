# QoL minimap

## 1. Scope

The map of the bottom screen as a minimap in a corner of the full-screen view. Plan: [qol.md](../qol.md). Switched by the master button only (Improved). The original map is [hud-ui.md](hud-ui.md).

## 2. Status

IN_PROGRESS. The round minimap is drawn from the original map canvas (20261030-qolpanels-B, 2026-10-30T15:39:42+01:00); placement under the counters and hiding in the scanner, dialog boxes, scene change page, fast-travel map and pause menu are built (20261030-qolpanels2-B); the comparison with a gameplay picture remains.

## 3. Verified behaviour

`qol-minimap` pins the geometry (diameter one fifth of the window height, at the right edge and below the studs and counters of the corner HUD, from the same layout source), the round mask and that Original and Custom draw nothing. Every behaviour is tested first in original mode (all QoL off, the port's tests unchanged), then with the feature on ([qol rules](../../rules/qol.md)).

## 4. Stand-ins

- QOLMAP-S1 — The circle shows the 240 px disc around the player of the original 320x240 map canvas (the original zoom, the characters button lies outside the disc) and keeps a margin of 4 original-view units below the counters. The spec gives no margin or disc size ("a small margin"). The flush top-right placement is replaced by QOLMAP-5. evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).

## 5. Open items

- [ ] QOLMAP-7 — Compare the round minimap with a gameplay picture (rotation, icons and objective markers inside the disc) on real data. needs: QOLMAP-1.  evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).

## 6. Findings summary

Design: [qol-screen-layout.md](../../design/qol-screen-layout.md).

## 7. Code and tests

`src/qol/minimap.*` (library `chase_qol_minimap`): `qol::drawRoundMinimap` takes a `MinimapPanels` and crops the disc out of the canvas that the port's `drawMinimap` filled (map, rotation, icons and objective markers are the original's) and draws it through `HudOverlay::drawTop`; hook in `LevelSession::draw`. Tests `qol-minimap` (`tests/qol/minimap_test.cpp`) and `qol-minimap-real-data` (`tests/qol/minimap_real_test.cpp`).
