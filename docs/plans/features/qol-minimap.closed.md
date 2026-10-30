# Closed items of qol-minimap

Done items of [qol-minimap.md](qol-minimap.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] QOLMAP-2 — Decided: top-right, round, rotates, about 1/5 of the screen height, zoom like the original. evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).
- [x] QOLMAP-3 — Decided: original content, fixed zoom, hidden in cutscenes and panels (QOLMAP-1). evidence: [qol-screen-layout.md §2026-10-28T09:37:31](../../design/qol-screen-layout.md#ts-20261028-093731)
- [x] QOLMAP-4 — Decided: "Master only": the minimap shows only in Improved, together with the hidden bottom screen (QOLMAP-1). evidence: [qol-screen-layout.md §2026-10-28T09:48:15](../../design/qol-screen-layout.md#ts-20261028-094815).
- [x] QOLMAP-1 — The minimap, drawn from the same map data as the original bottom-screen map: round, top-right corner, about one fifth of the screen height, rotating with the camera, fixed zoom like the original map, the same icons and objective markers as the original map, hidden in cutscenes and panels (done for cutscenes and the scanner overlay; the other panels are QOLMAP-6, the top-right overlap is QOLMAP-5); no toggle of its own: it shows only when the master button is on Improved, together with the hidden bottom screen. needs: QOLSET-1, QOLFS-1. blocks: QOLMAP-5, QOLMAP-6, QOLMAP-7.  evidence: [qol-screen-layout.md §2026-10-28T09:31:02](../../design/qol-screen-layout.md#ts-20261028-093102).
