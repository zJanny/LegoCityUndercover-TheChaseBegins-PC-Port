# Script system: director camera

## 1. Scope

The director camera natives (StartDCam, StartDCamTrack, WaitForDCam, EndDCam and the commands not recovered), the level paths, the phase machine, busy and end tests and what an active director camera does to the rest of the game. See [script-system-natives.md](script-system-natives.md) for the other natives and [script-system.md](script-system.md) for the runner.

## 2. Status

IN_PROGRESS, updated 2026-10-14T16:48:13+02:00. The director camera runs: StartDCam, StartDCamTrack, WaitForDCam and EndDCam follow the oracle timing of the police entry script.

## 3. Verified behaviour

- StartDCam, StartDCamTrack, WaitForDCam and EndDCam follow the oracle timing of the police entry script (`dcam-real-data`): [dcam.md §2026-10-14T14:36](../../re/dcam.md#ts-20261014-143600).

## 4. Stand-ins

- SCRIPT-S11 `StartDCamTrack` looks at the placement position of the object (the live position for the player), not at its node matrix times the offset at entity +0x60 (0x221a1c, 0x250da8). needs: SCRIPT-48. evidence: [dcam.md §2026-10-14T14:36](../../re/dcam.md#ts-20261014-143600)
- SCRIPT-S12 `ShakeScreen` stores its arguments and draws nothing; `SnapCamera` resets the follow camera with `FollowCamera::reset` (10 m behind, CAMERA-S5), not with 0x2313c8. needs: CAMERA-10.
- SCRIPT-S13 `DCamActive` stores the flag per object and nothing reads it; `Set3DFocusObject` does nothing (stereoscopic focus). needs: SCRIPT-47.

## 5. Open items

- [ ] SCRIPT-40 — Director camera: path and track playback, the busy test, the end and the hand-back run (see SCRIPT-28); open are the cutscene load / play / free natives of SCRIPT-29 (the cutscene part of the busy test 0x2297b0) and the items SCRIPT-46 to SCRIPT-50. blocks: SCRIPT-29, VEH-19. evidence: [dcam.md §2026-10-14T14:36](../../re/dcam.md#ts-20261014-143600)
- [ ] SCRIPT-46 — Director camera commands not recovered: look path alone 0x270ff4, entity alone 0x2879b0, the entity camera 0x2663c4, StartDCamTrackAuto / Dual / NodelessDCamTrack, TiltDCam, SetDCamEase, type 1 paths, CameraFollow, CameraRotFixedSet, VehicleSetDCamPathFollow. blocks: SCRIPT-40. evidence: [dcam.md §2026-10-14T14:36](../../re/dcam.md#ts-20261014-143600)
- [ ] SCRIPT-47 — What an active director camera does to the rest of the game: objects without `DCamActive` (flag +0xd9 bit 1), the player and AI, the HUD (0x244970), bytes 0x56d554 / 0x56d558, callbacks 0x3768c8 / 0x3768cc. blocks: SCRIPT-S13. evidence: [dcam.md §2026-10-14T14:36](../../re/dcam.md#ts-20261014-143600)
- [ ] SCRIPT-48 — The look offset at entity +0x60 and the node matrix of a Track look object; needs a Track DCam in the oracle (lobby scripts such as the chief or red brick cop). blocks: SCRIPT-S11. evidence: [dcam.md §2026-10-14T14:36](../../re/dcam.md#ts-20261014-143600)
- [ ] SCRIPT-49 — Paths with three or more points are checked only against the formula of 0x1c17c0; the oracle comparison covers one and two points. Needs a state where a script with such a path can be started. evidence: [dcam.md §2026-10-14T14:36](../../re/dcam.md#ts-20261014-143600)
- [ ] SCRIPT-50 — Ticks per second of the DCam timer clock (1000 assumed), its quantisation of other dt values, and the cutscene part of the busy test (0x2297b0). needs: SCRIPT-29. evidence: [dcam.md §2026-10-14T14:36](../../re/dcam.md#ts-20261014-143600)

## 6. Findings summary

- Director camera: level paths (section +0x20), StartDCam and the command routine, phase machine, busy test, end test and hand-back, oracle timing: [dcam.md §2026-10-14T14:36](../../re/dcam.md#ts-20261014-143600).

## 7. Code and tests

- `DirectorCamera` (`src/runtime/dcam.*`, tests `dcam`, `dcam-real-data`, scenarios `tools/oracle/dcam-police-*.txt`).
