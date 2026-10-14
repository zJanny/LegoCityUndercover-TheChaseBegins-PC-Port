# Closed items of rendering

Done items of [rendering.md](rendering.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] RENDER-4 Closed 2026-10-14: the lobby compared with the oracle (lightmap surfaces match within 1 colour level on the stair wall, floor about 8 percent lighter; props darker = RENDER-1/5; camera differs, see CAMERA-6). evidence: [materials.md §2026-10-14T09:50](../../re/materials.md#ts-20261014-095011).
- [x] RENDER-3 Split: fog is implemented (RENDER-30 to RENDER-32); shadows RENDER-33, material colour pulse RENDER-34, draw sorting RENDER-35. evidence: [rendering-fog.md §2026-10-14T17:35](../../re/rendering-fog.md#ts-20261014-173500).
- [x] RENDER-32 Fog selection loops, other setFog and override callers, OverrideFog reader, projection source: read; what stays open is RENDER-39 and RENDER-40. evidence: [rendering-fog.md §2026-10-14T18:59](../../re/rendering-fog.md#ts-20261014-185910).
- [x] RENDER-36 The clear colour of the top screen is the level parameter `BackgroundColour` (lvl_01 to lvl_07 (183, 199, 221), lvl_00 black); `LevelSession::draw` clears to it. evidence: [rendering-fog.md §2026-10-14T18:59](../../re/rendering-fog.md#ts-20261014-185910).
