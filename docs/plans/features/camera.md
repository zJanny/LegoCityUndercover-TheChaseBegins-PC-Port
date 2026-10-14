# Camera

## 1. Scope

The gameplay camera: yaw word 0x376894, camera manager, the follow camera on the street (OnFoot preset), projection, and obstruction by level geometry and placed objects. Not covered: cutscene cameras ([movies-cutscenes.md](movies-cutscenes.md)), `.cam` record selection beyond what is recovered (data format in [assets-formats.md](assets-formats.md)), other presets.

## 2. Status

IN_PROGRESS, updated 2026-10-14T09:50:11+02:00. The follow camera and its obstruction run in `chase-game`.

## 3. Verified behaviour

- Follow law (look point and eye smoothing 1 - 0.75^(30 dt), 10 m horizontal rope, vertical spring 0.3, yaw (L/R +-2 + stick term) * dt * 0.6, projection 0x23458c): `camera-real-data` replays 9 oracle traces (323 updates, eye error <= 6.1e-4), later all rows of w_walk / w_L / w_east_R (eye error 5.5e-4).
- Obstruction: two thin slabs along the camera-to-look line cut by the level collision (mask 0x01000010), side push 2.5, vertical push 1.25, pull-in: `camera-sweep-real-data` (699 of 700 frames with the recorded segment count, eye error at most 5.5e-4, clearance error at most 7.3e-5).
- Placed objects obstruct the camera through the query layer {all, 0x30} and the box slice 0x2eb5d4.

## 4. Stand-ins

- CAMERA-S1 Mesh objects are gathered through the world box of the transformed tree, boxes through the world box of their corners — stand-ins for 0x2335e4 and the local-space tree query, shared with COLL-1.
- CAMERA-S2 `game-script-camera` pin 8.83 -> 8.77 is the replica's own output, not an oracle value.
- CAMERA-S3 `chase-game` uses the OnFoot follow camera with L/R turning in every level and never selects the record pose camera 0x298cd8 or shows the crossed-out camera icon (see CAMERA-6).

## 5. Open items

- [ ] CAMERA-1 — The value K+0x1c8 holds during the very first gather of a camera; whether the far limit K+0x1cc and other constants are also read stale; the original's segment list order (grid cells), which matters only above 13 recorded segments (`w_L` row 2 of the first gather is the known case). evidence: [camera.md §2026-10-13T17:35](../../re/camera.md#ts-20261013-173530).
- [ ] CAMERA-2 — Mask-4 characters (flag bit 0 rejects them at gameplay-start) and vehicles are not registered, so the camera ignores them. needs: COLL-2. evidence: [camera.md §2026-10-13T17:35](../../re/camera.md#ts-20261013-173530).
- [ ] CAMERA-3 — Pitch clamp, record pose blend rooms (0x298cd8, also the cause of CAMERA-6) and other presets are unknown. blocks: CAMERA-6. evidence: [camera.md §2026-10-13T14:39](../../re/camera.md#ts-20261013-143958).
- [ ] CAMERA-4 — The camera being placed again when it is stuck (not recovered).
- [ ] CAMERA-5 — The chase-game level start differs slightly from the oracle spawn in lvl_01, so the camera reference comparison is framing-only. see PLAYER-8.
- [ ] CAMERA-6 — The police station (lvl_00) locks the camera rotation; `chase-game` does not. Original (user, real 3DS; confirmed in the oracle, 2026-10-14): L/R do not change the yaw and the HUD shows `UI_CameraLocked`. Cause (proven): not `CameraRotFixedSet` (called once in all levels, in lvl_07_downtown) but the active camera command being the record pose 0x298cd8 instead of the follow controller 0x2a8bfc; chosen by the `.cam` records at the player position (default preset update 0x1b4a30 / 0x19b8c8), so it applies wherever the record set is not one Room record. Whether all indoor rooms are covered (user hypothesis) is unknown. Next: implement the record selection and the 0x298cd8 pose (CAMERA-3), then the lock and the icon (0x193aa4, shown when L or R is pressed and the command is not 0x2a8bfc). needs: CAMERA-3. evidence: [camera.md §2026-10-14T09:49](../../re/camera.md#ts-20261014-094953).

## 6. Findings summary

- Writer of 0x376894 is the camera manager update 0x23f174 (atan2 of the camera node's forward axis); the street camera is follow controller 0x2e0bf0: [camera.md §2026-10-13T14:39](../../re/camera.md#ts-20261013-143958).
- 0x3313c0 is a two-slab sweep, not a sphere sweep: [camera.md §2026-10-13T16:19](../../re/camera.md#ts-20261013-161935).
- Placed objects: layer, box slicing, stale near limit: [camera.md §2026-10-13T17:35](../../re/camera.md#ts-20261013-173530).
- Camera lock: record pose camera instead of follow controller, `CameraRotFixedSet` is only used in lvl_07: [camera.md §2026-10-14T09:49](../../re/camera.md#ts-20261014-094953).
- `.cam` selection and weights (spatial, XZ distance, bit 0x400): [data-tables-config.md §2026-10-13T09:32](../../re/data-tables-config.md#ts-20261013-0932).

## 7. Code and tests

- `src/runtime/follow_camera.*`, `camera_sweep.*`; chase-game `--no-camera-collision`.
- CTest: `camera-sweep`, `camera-real-data`, `camera-sweep-real-data`, `game-script-camera`.
- Trace dirs: `CHASE_CAMERA_TRACE_DIR`, `CHASE_CAMERA_WALL_TRACE_DIR`. Scenarios: `tools/oracle/camera-trace.txt`, `camwall-circle.txt`, `camwall-east.txt`; trace export `tools/export_camera_trace.py`.
