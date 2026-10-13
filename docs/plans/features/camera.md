# Camera

## 1. Scope

The gameplay camera: yaw word 0x376894, camera manager, the follow camera on the street (OnFoot preset), projection, and obstruction by level geometry and placed objects. Not covered: cutscene cameras ([movies-cutscenes.md](movies-cutscenes.md)), `.cam` record selection beyond what is recovered (data format in [assets-formats.md](assets-formats.md)), other presets.

## 2. Status

IN_PROGRESS, updated 2026-10-13T21:29:01+0200. The follow camera and its obstruction run in `chase-game`.

## 3. Verified behaviour

- Follow law (look point and eye smoothing 1 - 0.75^(30 dt), 10 m horizontal rope, vertical spring 0.3, yaw (L/R +-2 + stick term) * dt * 0.6, projection 0x23458c): `camera-real-data` replays 9 oracle traces (323 updates, eye error <= 6.1e-4), later all rows of w_walk / w_L / w_east_R (eye error 5.5e-4).
- Obstruction: two thin slabs along the camera-to-look line cut by the level collision (mask 0x01000010), side push 2.5, vertical push 1.25, pull-in: `camera-sweep-real-data` (699 of 700 frames with the recorded segment count, eye error at most 5.5e-4, clearance error at most 7.3e-5).
- Placed objects obstruct the camera through the query layer {all, 0x30} and the box slice 0x2eb5d4.

## 4. Stand-ins

- CAMERA-S1 Mesh objects are gathered through the world box of the transformed tree, boxes through the world box of their corners — stand-ins for 0x2335e4 and the local-space tree query, shared with COLL-1.
- CAMERA-S2 `game-script-camera` pin 8.83 -> 8.77 is the replica's own output, not an oracle value.
- CAMERA-S3 `chase-game` uses the OnFoot follow camera (L/R and stick turning) in every level, also inside the police station (lvl_00); the original uses other camera rules there (see CAMERA-6).

## 5. Open items

- [ ] CAMERA-1 — The value K+0x1c8 holds during the very first gather of a camera; whether the far limit K+0x1cc and other constants are also read stale; the original's segment list order (grid cells), which matters only above 13 recorded segments (`w_L` row 2 of the first gather is the known case). evidence: [camera.md §2026-10-13T17:35](../../re/camera.md).
- [ ] CAMERA-2 — Mask-4 characters (flag bit 0 rejects them at gameplay-start) and vehicles are not registered, so the camera ignores them. needs: COLL-2. evidence: [camera.md §2026-10-13T17:35](../../re/camera.md).
- [ ] CAMERA-3 — Pitch clamp, record pose blend rooms and other presets are unknown. evidence: [camera.md §2026-10-13T14:39](../../re/camera.md).
- [ ] CAMERA-4 — The camera being placed again when it is stuck (not recovered).
- [ ] CAMERA-5 — The chase-game level start differs slightly from the oracle spawn in lvl_01, so the camera reference comparison is framing-only. see PLAYER-8.
- [ ] CAMERA-6 — (User observation on a real 3DS, 2026-10-13) Inside the police station the camera cannot be turned, so the view behind the start position is never visible; `chase-game` lets the player turn it (CAMERA-S3). Recover the indoor camera rules: room/bound camera records and the record pose blend 0x298cd8 (CAMERA-3), the pose flag bit 0x40000000 "no rotation" (C+0x4c+0x10), and which camera mode lvl_00 starts with; check against the original in the police station (RENDER-4 state). needs: RENDER-4. evidence: [camera.md §2026-10-13T14:39](../../re/camera.md).

## 6. Findings summary

- Writer of 0x376894 is the camera manager update 0x23f174 (atan2 of the camera node's forward axis); the street camera is follow controller 0x2e0bf0: [camera.md §2026-10-13T14:39](../../re/camera.md).
- 0x3313c0 is a two-slab sweep, not a sphere sweep: [camera.md §2026-10-13T16:19](../../re/camera.md).
- Placed objects: layer, box slicing, stale near limit: [camera.md §2026-10-13T17:35](../../re/camera.md).
- `.cam` selection and weights (spatial, XZ distance, bit 0x400): [data-tables-config.md §2026-10-13T09:32](../../re/data-tables-config.md).

## 7. Code and tests

- `src/runtime/follow_camera.*`, `camera_sweep.*`; chase-game `--no-camera-collision`.
- CTest: `camera-sweep`, `camera-real-data`, `camera-sweep-real-data`, `game-script-camera`.
- Trace dirs: `CHASE_CAMERA_TRACE_DIR`, `CHASE_CAMERA_WALL_TRACE_DIR`. Scenarios: `tools/oracle/camera-trace.txt`, `camwall-circle.txt`, `camwall-east.txt`; trace export `tools/export_camera_trace.py`.
