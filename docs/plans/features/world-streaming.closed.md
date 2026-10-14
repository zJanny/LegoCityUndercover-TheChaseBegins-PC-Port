# Closed items of world-streaming

Done items of [world-streaming.md](world-streaming.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] WSTREAM-1 — closed 2026-10-14: rule recovered (request chain 0x197794, 0x2af6b0, 0x279834, apply 0x189c90). evidence: [streaming-rooms.md §2026-10-14T14:00](../../re/streaming-rooms.md#ts-20261014-140018).
- [x] WSTREAM-2 — closed 2026-10-14: unloading is the absence of a request, driven by the same link lists (0x173f94); the `root` object of each scene is always requested; the `lvl_05` records' effect is open as WSTREAM-11. evidence: same entry.
- [x] WSTREAM-3 — closed 2026-10-14: `RoomStreamer` applies `planRoomStreaming` every update in `chase-game` (street level), room models and placed objects are shown only while loaded, the script bound rule uses the loaded rooms (SCRIPT-S7). Remaining: WSTREAM-13, WSTREAM-14. evidence: [streaming-rooms.md §2026-10-14T15:07:48](../../re/streaming-rooms.md#ts-20261014-150748).
- [x] WSTREAM-7 — Closed 2026-10-13: the reported grey area behind the police-station start is a view the original never shows; its cause is the missing camera lock, tracked as CAMERA-6.
- [x] WSTREAM-8 — closed 2026-10-14: the node position is the centre of the vertex box of meshes 1 and up (43 of 43 rooms); the code itself stays open as WSTREAM-12. evidence: [streaming-rooms.md §2026-10-14T15:07:48](../../re/streaming-rooms.md#ts-20261014-150748).
