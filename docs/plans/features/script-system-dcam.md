# Script system: director camera

## 1. Scope

The director camera natives (StartDCam, StartDCamTrack, WaitForDCam, EndDCam and the commands not recovered), the level paths, the phase machine, busy and end tests and what an active director camera does to the rest of the game. See [script-system-natives.md](script-system-natives.md) for the other natives and [script-system.md](script-system.md) for the runner.

## 2. Status

IN_PROGRESS, updated 2026-10-15T09:38:42+02:00. The director camera runs: StartDCam, StartDCamTrack, WaitForDCam and EndDCam follow the oracle timing of the police entry script, and an active director camera holds the player and the characters like the original.

## 3. Verified behaviour

- StartDCam, StartDCamTrack, WaitForDCam and EndDCam follow the oracle timing of the police entry script (`dcam-real-data`): [dcam.md §2026-10-14T14:36](../../re/dcam.md#ts-20261014-143600).

- While the director camera is current the player not under AI control gets no stick and no buttons, and a character under AI control (the player included) runs its AI only with `DCamActive 1`; the hold ends with the camera. Oracle the save state in front of the police car of mission 1-1: X was ignored for the 273 updates of `1-1_Release_Car_Script` (`game-director-hold`, `game-director-release`, `director-hold`; hold from the update after StartDCam to the update after the end test, `tools/oracle/dcam-hold-order.txt`): [dcam.md §2026-10-15T09:37](../../re/dcam.md#ts-20261015-093712).

## 4. Stand-ins

- SCRIPT-S11 `StartDCamTrack` looks at the placement position of the object (the live position for the player), not at its node matrix times the offset at entity +0x60 (0x221a1c, 0x250da8). needs: SCRIPT-48. evidence: [dcam.md §2026-10-14T14:36](../../re/dcam.md#ts-20261014-143600)
- SCRIPT-S12 `SnapCamera` runs the reset 0x2313c8 with argument 0 through `FollowCamera::snap` (ray branch, CAMERA-10). The camera-bound pose of CAMERA-S5 is not applied.
- SCRIPT-S271 `ShakeScreen` draws its random numbers from `breakRandomStandIn`, the port's stand-in stream; the seed and the other users of the original's one generator are not known. evidence: [dcam.md §2026-10-19T15:09:12](../../re/dcam.md#ts-20261019-150912)
- SCRIPT-S13 `Set3DFocusObject` does nothing (stereoscopic focus, no mono effect expected). evidence: [dcam.md §2026-10-14T14:36](../../re/dcam.md#ts-20261014-143600)
- SCRIPT-S142 A held character only loses its AI move (its other updates, the animation included, still run); the original's control routine 0x29bf30 zeroes held / press / heading and runs nothing else of the controller. needs: SCRIPT-182. evidence: [dcam.md §2026-10-15T09:37](../../re/dcam.md#ts-20261015-093712)

## 5. Open items

- [ ] SCRIPT-40 — Director camera: path and track playback, the busy test, the end and the hand-back run (see SCRIPT-28); open are the cutscene load / play / free natives of SCRIPT-29 (the cutscene part of the busy test 0x2297b0) and the items SCRIPT-46 and SCRIPT-48 to SCRIPT-50, SCRIPT-181 and SCRIPT-182. needs: SCRIPT-46. blocks: SCRIPT-29. evidence: [dcam.md §2026-10-14T14:36](../../re/dcam.md#ts-20261014-143600)
- [ ] SCRIPT-46 — Director camera commands not recovered: look path alone 0x270ff4, entity alone 0x2879b0, the entity camera 0x2663c4, StartDCamTrackAuto / Dual / NodelessDCamTrack, TiltDCam, SetDCamEase, type 1 paths, CameraFollow, CameraRotFixedSet, VehicleSetDCamPathFollow. blocks: SCRIPT-40, SCRIPT-7. evidence: [dcam.md §2026-10-14T14:36](../../re/dcam.md#ts-20261014-143600)
- [ ] SCRIPT-181 — The stopwatch `*(D+0x1c)` (0x5165f0) that the director camera pauses at the start and resumes at the end (0x244970), the callbacks 0x3768c8 / 0x3768cc and the FOV hook 0x3768dc: which timers use that clock and who sets the callbacks. evidence: [dcam.md §2026-10-15T09:37](../../re/dcam.md#ts-20261015-093712)
- [ ] SCRIPT-182 — Other effects of the busy test and the camera pointers: the HUD update 0x2d9f84, the predicates 0x20ac88 / 0x235358, 0x27c298, 0x245258, the character routines 0x26d934 / 0x29cd9c, the byte 0x246128 and the words around 0x56d550. blocks: SCRIPT-S142. evidence: [dcam.md §2026-10-15T09:37](../../re/dcam.md#ts-20261015-093712)
- [ ] SCRIPT-48 — The look offset at entity +0x60 and the node matrix of a Track look object; needs a Track DCam in the oracle (lobby scripts such as the chief or red brick cop). blocks: SCRIPT-S11. evidence: [dcam.md §2026-10-14T14:36](../../re/dcam.md#ts-20261014-143600)
- [ ] SCRIPT-49 — Paths with three or more points are checked only against the formula of 0x1c17c0; the oracle comparison covers one and two points. Needs a state where a script with such a path can be started. evidence: [dcam.md §2026-10-14T14:36](../../re/dcam.md#ts-20261014-143600)
- [ ] SCRIPT-50 — Ticks per second of the DCam timer clock (1000 assumed), its quantisation of other dt values, and the cutscene part of the busy test (0x2297b0). needs: SCRIPT-29. evidence: [dcam.md §2026-10-14T14:36](../../re/dcam.md#ts-20261014-143600)

- [ ] SCRIPT-332 — Other callers of the shake start 0x22c500 (the build-it end calls (4, 4, 4, 0) at 0x1ab07c) pass the lock byte and are not wired; the lock byte of the running shake is implemented. needs: CAMERA-54. evidence: [dcam.md §2026-10-19T15:09:12](../../re/dcam.md#ts-20261019-150912)

## 6. Findings summary

- ShakeScreen: arguments, start 0x22c500, application in the camera manager update 0x23f174: [dcam.md §2026-10-19T15:09:12](../../re/dcam.md#ts-20261019-150912).
- Hold of the player and the characters during a director camera, the paused stopwatch, the other readers of the camera pointers: [dcam.md §2026-10-15T09:37](../../re/dcam.md#ts-20261015-093712).
- Director camera: level paths (section +0x20), StartDCam and the command routine, phase machine, busy test, end test and hand-back, oracle timing: [dcam.md §2026-10-14T14:36](../../re/dcam.md#ts-20261014-143600).

## 7. Code and tests

- `DirectorCamera` (`src/runtime/dcam.*`, tests `dcam`, `dcam-real-data`, scenarios `tools/oracle/dcam-police-*.txt`).
- ShakeScreen: `DirectorCamera::takeShake`, `ScreenShake` (`src/runtime/screen_shake.*`, test `screen-shake`), glue `src/game_screen_shake.cpp`.
- Hold: `characterControl` (`src/runtime/director_hold.*`, test `director-hold`), `AiCore::setUpdateGate`, `ScriptRunner::playerControlTaken`; real data: `game-director-hold`, `game-director-release`, scenarios `tools/oracle/dcam-hold-car.txt`, `dcam-hold-order.txt`; the option `--set-variable` (`game-options`) starts the lobby after its camera tour.
