# Camera

## 1. Scope

The gameplay camera: yaw word 0x376894, camera manager, the follow camera on the street (OnFoot preset), projection, and obstruction by level geometry and placed objects. The vehicle camera (InVehicle preset) is part of it. Not covered: cutscene cameras ([movies-cutscenes.md](movies-cutscenes.md)), other presets (data format of `.cam` in [assets-formats.md](assets-formats.md)).

## 2. Status

IN_PROGRESS, updated 2026-10-15T16:58:09+02:00. The follow camera and its obstruction run in `chase-game`; the record selection, the record pose camera, the camera lock, the fade between camera commands and the pose filter run too; the script director camera (StartDCam) is a command on the stack and hands back to the follow or record command (lobby walks over one and five records verified against the oracle).

## 3. Verified behaviour

- Follow law (look point and eye smoothing 1 - 0.75^(30 dt), 10 m horizontal rope, vertical spring 0.3, yaw (L/R +-2 + stick term) * dt * 0.6, projection 0x23458c): `camera-real-data` replays 9 oracle traces (323 updates, eye error <= 6.1e-4), later all rows of w_walk / w_L / w_east_R (eye error 5.5e-4).
- Obstruction: two thin slabs along the camera-to-look line cut by the level collision (mask 0x01000010), side push 2.5, vertical push 1.25, pull-in: `camera-sweep-real-data` (699 of 700 frames with the recorded segment count, eye error at most 5.5e-4, clearance error at most 7.3e-5).
- Placed objects obstruct the camera through the query layer {all, 0x30} and the box slice 0x2eb5d4.
- Hand-back after the release director camera of mission 1-1: StartDCam clears the stored heading, the reset's ray branch seeds 12.99 m behind the car and the near plane test 0x2e0e28 moves the tracker to the wanted position in the first update; `camera-vehicle-real-data` replays it (seed rows 2.4e-4, track 2.9e-3, near plane 2.4e-4).
- Record selection and pose: police-station lobby selects the record pose with `Camerabound_01` and eye / look / FOV / yaw equal to the oracle (`camera-records-real-data`); the street (street-after-1-1, gameplay-start) selects the follow controller. While the record pose is active L/R do not turn the camera and the crossed-out camera icon fades in 0.3 s, holds 0.5 s after the last press and fades out 0.3 s (`camera-records`, oracle object fields; replaces CAMERA-S3).

## 4. Stand-ins

- CAMERA-S1 Mesh objects are gathered through the world box of the transformed tree, boxes through the world box of their corners — stand-ins for 0x2335e4 and the local-space tree query, shared with COLL-1.
- CAMERA-S2 `game-script-camera` pin 8.83 -> 8.77 is the replica's own output, not an oracle value.
- CAMERA-S4 The records that the spatial structure 0x598300 returns are taken in level order (the original's grid order is unknown); all camera bounds count as enabled, bound matrices as static, the scene node world matrix of a bound as the identity, bytes 0x3770c8 / counter 0x3770e0 as 0, and the conditions 0x21d744 / 0x236008 of the icon as 0.
- CAMERA-S5 The fixed pose of Blocker records (0x58d42c) and the shake 0x1c607c are not applied.
- CAMERA-S6 While a director camera is the newest command the record / follow hand-overs of the selector are ignored; the original may push commands over it. needs: CAMERA-11.

- CAMERA-S31 The reset's segment queries test box entries in their own space with a slab test and read vertical segments like the others; composite owners (flag 0x800) and 0x293188 are not modelled. evidence: [camera-vehicle.md §2026-10-15T15:22](../../re/camera-vehicle.md#ts-20261015-152200)

- CAMERA-S20 `CameraFollow` (0x1bf7c8, a request to switch to the gameplay camera `*0x3770d8`) is a no-op in the script runner (stand-in): the port has one gameplay camera object. In 1-1_VanBreakdown the original's request is also without effect (the cutscene end switched back first). needs: CAMERA-40. evidence: [camera-vehicle.md §2026-10-15T16:58](../../re/camera-vehicle.md#ts-20261015-165809)

## 5. Open items

- [ ] CAMERA-1 — The value K+0x1c8 holds during the very first gather of a camera; whether the far limit K+0x1cc and other constants are also read stale; the original's segment list order (grid cells), which matters only above 13 recorded segments (`w_L` row 2 of the first gather is the known case). evidence: [camera-collision-sweep.md §2026-10-13T17:35](../../re/camera-collision-sweep.md#ts-20261013-173530).
- [ ] CAMERA-2 — Mask-4 characters (flag bit 0 rejects them at gameplay-start) and vehicles are not registered, so the camera ignores them. needs: COLL-2. evidence: [camera-collision-sweep.md §2026-10-13T17:35](../../re/camera-collision-sweep.md#ts-20261013-173530).
- [ ] CAMERA-3 — Record selection, record pose, fade and pose filter are in; open: the multi-record weights and pose against the oracle, Blocker pose, shake, query order and the presets of other player states. evidence: [camera-records-blending.md §2026-10-14T13:39](../../re/camera-records-blending.md#ts-20261014-133954).
- [ ] CAMERA-4 — The camera being placed again when it is stuck (not recovered).
- [ ] CAMERA-5 — The chase-game level start differs slightly from the oracle spawn in lvl_01, so the camera reference comparison is framing-only. see PLAYER-8.
- [ ] CAMERA-7 — The lock scope census treats all camera bounds as enabled; which are switched off by EnableBound and what the moving bound matrices do is not traced (affects CAMERA-S4). `EnableBound` is transcribed in the script runner (`ScriptRunner::boundsEnabled`, 0x1d0ce8); the camera records do not read it yet.
- [ ] CAMERA-51 — In `chase-game` the settled camera after the hand-back is 0.31 m lower than the original's and its swing starts three updates early, because the police car rests 4 cm from the original's resting place (VEH-S6); with the recorded car pose the camera matches. needs: VEH-33. evidence: [camera-vehicle.md §2026-10-15T16:58](../../re/camera-vehicle.md#ts-20261015-165809)
- [ ] CAMERA-50 — The port renders with its own near plane; the original uses the result of the near plane test (`FollowCamera::nearPlane()`, camera +0xc4) and the far plane K+0x14c = 260. evidence: [camera-vehicle.md §2026-10-15T16:45](../../re/camera-vehicle.md#ts-20261015-164500)
- [ ] CAMERA-40 — The enter / leave / update virtuals of the gameplay camera 0x377110 and the cutscene camera 0x3768e8, and CameraFollow calls made while another camera is current (other scripts than 1-1_VanBreakdown). blocks: CAMERA-S20. evidence: [camera-vehicle.md §2026-10-15T16:58](../../re/camera-vehicle.md#ts-20261015-165809)
- [ ] CAMERA-11 — Whether the camera manager pushes selector commands while a director camera runs (the police entry moves the player into the lobby camera bounds). blocks: CAMERA-S6. evidence: [dcam.md §2026-10-14T14:36](../../re/dcam.md#ts-20261014-143600)
- [ ] CAMERA-12 — Pose filter factors at a director camera push (1.0 seen after the follow command; the lobby record filter case is untested) and the pose fields +0x34 (0.035). evidence: [dcam.md §2026-10-14T14:36](../../re/dcam.md#ts-20261014-143600)

## 6. Findings summary

- Writer of 0x376894 is the camera manager update 0x23f174 (atan2 of the camera node's forward axis); the street camera is follow controller 0x2e0bf0: [camera-follow.md §2026-10-13T14:39](../../re/camera-follow.md#ts-20261013-143958).
- 0x3313c0 is a two-slab sweep, not a sphere sweep: [camera-collision-sweep.md §2026-10-13T16:19](../../re/camera-collision-sweep.md#ts-20261013-161935).
- Placed objects: layer, box slicing, stale near limit: [camera-collision-sweep.md §2026-10-13T17:35](../../re/camera-collision-sweep.md#ts-20261013-173530).
- Camera lock: record pose camera instead of follow controller, `CameraRotFixedSet` is only used in lvl_07: [camera-records-blending.md §2026-10-14T09:49](../../re/camera-records-blending.md#ts-20261014-094953).
- Fade between camera commands (0x224080, 0x2ae250, curve 0x2b5950), stack merge 0x2c584c and pose filter 0x2a00dc with its factors: [camera-records-blending.md §2026-10-14T12:07](../../re/camera-records-blending.md#ts-20261014-120746).
- `.cam` selection and weights (spatial, XZ distance, bit 0x400): [data-tables-config.md §2026-10-13T09:32](../../re/data-tables-config.md#ts-20261013-0932); camera records are the `Camera*` level bounds, decision state machine, pose formula, icon timing, per-level lock census: [camera-records-blending.md §2026-10-14T10:44](../../re/camera-records-blending.md#ts-20261014-104400).
- Vehicle camera (preset chooser 0x1597c8, InVehicle block, automatic turn, pitch clamp): [camera-vehicle.md §2026-10-15T12:07](../../re/camera-vehicle.md#ts-20261015-120718).
- Reset obstruction branches (line test, ray test, pull-in, candidates), K+0x178 and the hand-back: [camera-vehicle.md §2026-10-15T15:22](../../re/camera-vehicle.md#ts-20261015-152200).
- Writers of K+0x178, near plane test 0x2e0e28 and the tracker snap, render near plane: [camera-vehicle.md §2026-10-15T16:45](../../re/camera-vehicle.md#ts-20261015-164500).
- Hand-back screenshot pair, `chase-game` against the original, CameraFollow in 1-1_VanBreakdown: [camera-vehicle.md §2026-10-15T16:58](../../re/camera-vehicle.md#ts-20261015-165809).
- Follow take-over (reset seed, near rule, filter ramp): [camera-follow-handover.md §2026-10-15T11:20](../../re/camera-follow-handover.md#ts-20261015-112028).
- Director camera command, end test and hand-back through the command stack: [dcam.md §2026-10-14T14:36](../../re/dcam.md#ts-20261014-143600).

## 7. Code and tests

- `src/runtime/follow_camera.*`, `camera_reset_probe.*` (segment queries of the reset), `camera_near_plane.*` (near plane test), `camera_sweep.*`, `camera_records.*` (selection, pose), `camera_rig.*` (command switch), `camera_lock_icon.*`, `vehicle_camera.*` (vehicle preset and subject), `dcam.*` (director camera, `CameraRig::setDirector`); `tools/camera_lock_census.cpp`; chase-game `--no-camera-collision`.
- CTest: `camera-reset-probe`, `camera-near-plane`, `camera-sweep`, `camera-real-data`, `camera-sweep-real-data`, `camera-records`, `camera-records-real-data`, `camera-vehicle-real-data` (traces `CHASE_CAMERA_VEHICLE_TRACE_DIR`, scenarios `tools/oracle/camera-vehicle-drive.txt`, `camera-vehicle-enter.txt`, `camera-vehicle-handback.txt`, `camera-vehicle-handback-near.txt`, converter `tools/export_camera_vehicle_trace.py`), `camera-blend-real-data` (traces `CHASE_CAMERA_BLEND_TRACE_DIR`, scenarios `tools/oracle/camera-lobby-walk.txt`, `camera-street-teleport.txt`, `camera-street-seed.txt`, `camera-street-near.txt`, converter `tools/export_camera_blend_trace.py`), `game-script-camera`.
- Trace dirs: `CHASE_CAMERA_TRACE_DIR`, `CHASE_CAMERA_WALL_TRACE_DIR`. Scenarios: `tools/oracle/camera-trace.txt`, `camwall-circle.txt`, `camwall-east.txt`; trace export `tools/export_camera_trace.py`.
