# Closed items of rendering

Done items of [rendering.md](rendering.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] RENDER-4 Closed 2026-10-14: the lobby compared with the oracle (lightmap surfaces match within 1 colour level on the stair wall, floor about 8 percent lighter; props darker = RENDER-1/5; camera differs, see CAMERA-6). evidence: [materials.md §2026-10-14T09:50](../../re/materials.md#ts-20261014-095011).
- [x] RENDER-3 Split: fog is implemented (RENDER-30 to RENDER-32); shadows RENDER-33, material colour pulse RENDER-34, draw sorting RENDER-35. evidence: [rendering-fog.md §2026-10-14T17:35](../../re/rendering-fog.md#ts-20261014-173500).
- [x] RENDER-32 Fog selection loops, other setFog and override callers, OverrideFog reader, projection source: read; what stays open is RENDER-39 and RENDER-40. evidence: [rendering-fog.md §2026-10-14T18:59](../../re/rendering-fog.md#ts-20261014-185910).
- [x] RENDER-36 The clear colour of the top screen is the level parameter `BackgroundColour` (lvl_01 to lvl_07 (183, 199, 221), lvl_00 black); `LevelSession::draw` clears to it. evidence: [rendering-fog.md §2026-10-14T18:59](../../re/rendering-fog.md#ts-20261014-185910).
- [x] RENDER-31 Closed 2026-10-14: the hypothesis that rooms load inside full fog is not confirmed; 24 of 25 port loads and all 4 oracle loads happen with the nearest geometry at 98 to 260 from the camera, rooms far ahead at 60 to 98 % fog, adjacent rooms at 0 to 23 %. evidence: [rendering-fog.md §2026-10-14T20:47:14](../../re/rendering-fog.md#ts-20261014-204714).
- [x] RENDER-41 — closed 2026-10-14: in the oracle the rooms loaded on the street walk are out of view (Room16, Room6) or far ahead in 70 to 100 % fog (Room4, Room10); no abrupt pop-in seen, the port's room adds 1 to 3 % of the far field. Single-update resolution not reached. evidence: [rendering-fog.md §2026-10-14T21:42](../../re/rendering-fog.md#ts-20261014-214220).
