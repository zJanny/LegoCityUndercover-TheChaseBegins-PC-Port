# Closed items of camera

Done items of [camera.md](camera.md), moved here by tools/check_doc_links.py --fix in the order they were closed. IDs are never reused.

- [x] CAMERA-6 — Camera lock and UI_CameraLocked icon in the police station. Done 2026-10-14: the record pose is active wherever a `Camera*` bound contains the player (not all indoor rooms); L/R do not turn it and the icon shows (CAMERA-S3 removed). List: [camera-records-blending.md §2026-10-14T10:44](../../re/camera-records-blending.md#ts-20261014-104400).
- [x] CAMERA-8 — closed 2026-10-14: the record pose camera snapped in the police station (user report). Now the command stack fades between poses (smoothstep over the records' BlendTime) and the pose goes through the per-update filter; the lobby replays match to 6e-6. evidence: [camera-records-blending.md §2026-10-14T12:07](../../re/camera-records-blending.md#ts-20261014-120746).
