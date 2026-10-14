# Camera

## 1. Scope

The gameplay camera: yaw word 0x376894, camera manager, the follow camera on the street (OnFoot preset), projection, and obstruction by level geometry and placed objects. Not covered: cutscene cameras ([movies-cutscenes.md](movies-cutscenes.md)), other presets (data format of `.cam` in [assets-formats.md](assets-formats.md)).

## 2. Status

IN_PROGRESS, updated 2026-10-14T12:10:00+02:00. The follow camera and its obstruction run in `chase-game`; the record selection, the record pose camera, the camera lock, the fade between camera commands and the pose filter run too (lobby walks over one and five records verified against the oracle).

## 3. Verified behaviour

- Follow law (look point and eye smoothing 1 - 0.75^(30 dt), 10 m horizontal rope, vertical spring 0.3, yaw (L/R +-2 + stick term) * dt * 0.6, projection 0x23458c): `camera-real-data` replays 9 oracle traces (323 updates, eye error <= 6.1e-4), later all rows of w_walk / w_L / w_east_R (eye error 5.5e-4).
- Obstruction: two thin slabs along the camera-to-look line cut by the level collision (mask 0x01000010), side push 2.5, vertical push 1.25, pull-in: `camera-sweep-real-data` (699 of 700 frames with the recorded segment count, eye error at most 5.5e-4, clearance error at most 7.3e-5).
- Placed objects obstruct the camera through the query layer {all, 0x30} and the box slice 0x2eb5d4.
- Record selection and pose: police-station lobby selects the record pose with `Camerabound_01` and eye / look / FOV / yaw equal to the oracle (`camera-records-real-data`); the street (street-after-1-1, gameplay-start) selects the follow controller. While the record pose is active L/R do not turn the camera and the crossed-out camera icon fades in 0.3 s, holds 0.5 s after the last press and fades out 0.3 s (`camera-records`, oracle object fields; replaces CAMERA-S3).

## 4. Stand-ins

- CAMERA-S1 Mesh objects are gathered through the world box of the transformed tree, boxes through the world box of their corners — stand-ins for 0x2335e4 and the local-space tree query, shared with COLL-1.
- CAMERA-S2 `game-script-camera` pin 8.83 -> 8.77 is the replica's own output, not an oracle value.
- CAMERA-S4 The records that the spatial structure 0x598300 returns are taken in level order (the original's grid order is unknown); all camera bounds count as enabled, bound matrices as static, the scene node world matrix of a bound as the identity, bytes 0x3770c8 / counter 0x3770e0 as 0, and the conditions 0x21d744 / 0x236008 of the icon as 0.
- CAMERA-S5 The fixed pose of Blocker records (0x58d42c) and the shake 0x1c607c are not applied; the follow controller is seeded by `FollowCamera::reset` 10 m behind the player when it takes over from a record pose (the original starts about 4.6 m behind, 0x1b4864 / 0x2313c8), and the 0.1 factor ramp of 0x1b4864 is not modelled (factors 1.0 at once).

## 5. Open items

- [ ] CAMERA-1 — The value K+0x1c8 holds during the very first gather of a camera; whether the far limit K+0x1cc and other constants are also read stale; the original's segment list order (grid cells), which matters only above 13 recorded segments (`w_L` row 2 of the first gather is the known case). evidence: [camera.md §2026-10-13T17:35](../../re/camera.md#ts-20261013-173530).
- [ ] CAMERA-2 — Mask-4 characters (flag bit 0 rejects them at gameplay-start) and vehicles are not registered, so the camera ignores them. needs: COLL-2. evidence: [camera.md §2026-10-13T17:35](../../re/camera.md#ts-20261013-173530).
- [ ] CAMERA-3 — Record selection, record pose, fade and pose filter are in; open: the multi-record weights and pose against the oracle (the lobby has no overlapping records; needs a place with two overlapping records and a free player), Blocker pose, shake, query order, pitch clamp and other presets. evidence: [camera.md §2026-10-14T10:44](../../re/camera.md#ts-20261014-104400), [camera.md §2026-10-13T14:39](../../re/camera.md#ts-20261013-143958).
- [ ] CAMERA-4 — The camera being placed again when it is stuck (not recovered).
- [ ] CAMERA-5 — The chase-game level start differs slightly from the oracle spawn in lvl_01, so the camera reference comparison is framing-only. see PLAYER-8.
- [x] CAMERA-6 — Camera lock and UI_CameraLocked icon in the police station. Done 2026-10-14: the record pose is the active command wherever a `Camera*` bound contains the player; L/R do not turn it and the icon shows (CAMERA-S3 removed). Scope: not "all indoor rooms" but every place under a `Camera*` volume, list in [camera.md §2026-10-14T10:44](../../re/camera.md#ts-20261014-104400).
- [ ] CAMERA-7 — The lock scope census treats all camera bounds as enabled; which are switched off by EnableBound and what the moving bound matrices do is not traced (affects CAMERA-S4). `EnableBound` is transcribed in the script runner (`ScriptRunner::boundsEnabled`, 0x1d0ce8); the camera records do not read it yet.
- [x] CAMERA-8 — closed 2026-10-14: the record pose camera snapped in the police station (user report). Now the command stack fades between poses (smoothstep over the records' BlendTime) and the pose goes through the per-update filter; the lobby replays match to 6e-6. evidence: [camera.md §2026-10-14T12:07](../../re/camera.md#ts-20261014-120746).
- [ ] CAMERA-9 — The follow controller in a fade: its seed when it takes over from a record pose (original about 4.6 m behind the player, y player + 2.0), its reaction to a large jump of the player and the factor ramp 0.1 of 0x1b4864 are not reproduced, so the poses of the street hand-over replay only match late (CAMERA-S5). Needs the follow controller with collision in the test and a walked (not teleported) hand-over. evidence: [camera.md §2026-10-14T12:07](../../re/camera.md#ts-20261014-120746).

## 6. Findings summary

- Writer of 0x376894 is the camera manager update 0x23f174 (atan2 of the camera node's forward axis); the street camera is follow controller 0x2e0bf0: [camera.md §2026-10-13T14:39](../../re/camera.md#ts-20261013-143958).
- 0x3313c0 is a two-slab sweep, not a sphere sweep: [camera.md §2026-10-13T16:19](../../re/camera.md#ts-20261013-161935).
- Placed objects: layer, box slicing, stale near limit: [camera.md §2026-10-13T17:35](../../re/camera.md#ts-20261013-173530).
- Camera lock: record pose camera instead of follow controller, `CameraRotFixedSet` is only used in lvl_07: [camera.md §2026-10-14T09:49](../../re/camera.md#ts-20261014-094953).
- Fade between camera commands (0x224080, 0x2ae250, curve 0x2b5950), stack merge 0x2c584c and pose filter 0x2a00dc with its factors: [camera.md §2026-10-14T12:07](../../re/camera.md#ts-20261014-120746).
- `.cam` selection and weights (spatial, XZ distance, bit 0x400): [data-tables-config.md §2026-10-13T09:32](../../re/data-tables-config.md#ts-20261013-0932); camera records are the `Camera*` level bounds, decision state machine, pose formula, icon timing, per-level lock census: [camera.md §2026-10-14T10:44](../../re/camera.md#ts-20261014-104400).

## 7. Code and tests

- `src/runtime/follow_camera.*`, `camera_sweep.*`, `camera_records.*` (selection, pose), `camera_rig.*` (command switch), `camera_lock_icon.*`; `tools/camera_lock_census.cpp`; chase-game `--no-camera-collision`.
- CTest: `camera-sweep`, `camera-real-data`, `camera-sweep-real-data`, `camera-records`, `camera-records-real-data`, `camera-blend-real-data` (traces `CHASE_CAMERA_BLEND_TRACE_DIR`, scenarios `tools/oracle/camera-lobby-walk.txt`, `camera-street-teleport.txt`, converter `tools/export_camera_blend_trace.py`), `game-script-camera`.
- Trace dirs: `CHASE_CAMERA_TRACE_DIR`, `CHASE_CAMERA_WALL_TRACE_DIR`. Scenarios: `tools/oracle/camera-trace.txt`, `camwall-circle.txt`, `camwall-east.txt`; trace export `tools/export_camera_trace.py`.
