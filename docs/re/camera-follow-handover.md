# Gameplay camera: follow controller take-over

How the follow controller (0x2a8bfc, camera system object K at 0x589a70) takes over from another camera command: the seed of 0x2313c8, the near rule and the filter ramp of 0x1b4864. Entries are dated; new entries go at the end.

## Current state

- Take-over 0x1b4864 returns at once when the follow controller is already on top, calls the reset 0x2313c8, applies the near rule, pushes the follow command and sets both pose filter slots. [§2026-10-15T11:20:28](#ts-20261015-112028)
- The reset places the camera at the first of eight candidate angles (180, 0, 90, 270, 45, 135, 225, 315), with distance and height scaled by the stale clearance S+4 of the last sweep. [§2026-10-15T11:20:28](#ts-20261015-112028)
- Near rule (only after a record pose): if the look point is closer than 2 x 10.0 = 20 m to the camera node, the seed is the current camera node position and the filter rate is 0.1, else 1.0. [§2026-10-15T11:20:28](#ts-20261015-112028)
- Filter factors ramp as `f += (1 - f) * 0.1` per update; the hand-over frame runs the follow update twice. [§2026-10-15T11:20:28](#ts-20261015-112028)
- The apparent reaction to a 33 m jump is the level collision; the follow law itself is unchanged. [§2026-10-15T11:20:28](#ts-20261015-112028)
- Port: `FollowCamera::takeOver`, `FilterRamp`, double update in `CameraRig`; test `camera-blend-real-data`. [§2026-10-15T11:20:28](#ts-20261015-112028)
- Open: later candidate angles (line test 0x22b108 has no counterpart), the argument 0 branch and director hand-back wiring, the gameplay start, and the pitch clamp 0x2e0a50.

<a id="ts-20261015-112028"></a>

## 2026-10-15T11:20:28+02:00 — Follow take-over: seed, near rule, filter ramp (CAMERA-9; llvm-objdump and REA Ghidra 12.1.2 pseudo code, Azahar GDB memtrace and trap on the save state outside the police station after mission 1-1, no pokes, no RPC)

Evidence IDs: reset 0x2313c8 ev_a5c17128…, follow update 0x2e0bf0 ev_30040f4a…, pitch clamp 0x2e0a50 ev_ff0bd884…, line test 0x22b108 ev_d373d25b…, strategy update 0x3310d0 ev_7c08f084…. Resolves the unknowns of [camera-records-blending.md §2026-10-14T12:07](camera-records-blending.md#ts-20261014-120746) (seed, reaction to a jump, factor ramp). Scenarios: `tools/oracle/camera-street-seed.txt` and `camera-street-near.txt` (the teleports are measuring aids), converter `tools/export_camera_blend_trace.py --layout seed`; test `camera-blend-real-data` (street_out, street_seed_far, street_near_n / e / ne).

**The "reaction to a 33 m jump" is the level collision.** The follow controller law is unchanged: it matches the recorded follow stack entry (entry +0x00 look, +0x24 eye, routine 0x2a8bfc at entry +0x78) to 1e-3 over the whole teleport-in trace once it runs with the level collision (the earlier blend test ran it without).

**1b4864 (take-over, called from 0x19b8c8 with argument 0).**
1. Returns at once when the top command is already the follow controller.
2. Calls the reset 0x2313c8(K, 0x168). SnapCamera 0x1a30fc calls it with 0, the periodic re-snap of 0x2e0bf0 with 0x168.
3. Near rule, only while the top command is a record pose (0x2a8c4c compares the top command's routine with 0x298cd8; after a director camera the rule is skipped, [camera-vehicle.md §2026-10-15T12:07](camera-vehicle.md#ts-20261015-120718)): when the distance from K+0x64 (wanted look point) to the camera node's world position is below 2 x the float at [K+0x160]+4 (K+0x160 = 0x376f6c, the OnFoot preset block, +4 = 10.0, so 20 m), the wanted camera position A (K+4) becomes the current camera node position (node + 0x8c) and 0x211d98 copies it to the tracker K+0x34. The filter rate is then 0.1, else 1.0.
4. Pushes the follow command (header from 0x224094, routine 0x2a8bfc, fade flag 1, time 0.5 s, curve 5; pushing calls the routine once).
5. 0x21a538 sets both pose filter slots to target 1.0 with that rate, mode 0.

**Reset 0x2313c8.** Zeroes A, B, C and their trackers, calls 0x2e0800 and 0x2e06f4 (strategy inputs; D and H land in the input block at +0x58 and +0x5c), then the strategy's vtable method +8 (0x3310b0: S+4 = 1.0, S+8 = 1.0, S+0xc and S+0x10 from in+0xa8). D and H were computed before that call, so they are **scaled by the clearance S+4 left by the last sweep**. The camera is placed at the first of eight candidate angles 180, 0, 90, 270, 45, 135, 225, 315 (plus the stored heading K+0x178, 360 after any reset): A = B + (sin a, 0, cos a) x D rotated by the player matrix, y + H. With the stored heading 360 the candidate is kept when the line test 0x22b108(B, A) finds nothing; otherwise (argument 0) the ray test 0x21ab20 is used and the position is pulled in with 0x23b92c. At the end 0x211d98 copies A, B and C into their trackers and into K+0x10 / K+0x70 (so the "moving" flag of the next update is false), and K+0x130 = 2.0.

**Oracle (GDB trap at 0x2313c8, 0x2e0bf0, 0x3313c0, memtrace of K).** Teleport out of CameraBound_PolicePK_03 to (905, 16.6, 1110) after 100 updates inside: before the reset A = tracker = 1.57 m from the player (S+4 = 0.157, wall behind the camera), K+0x1c8 = 8.651. The first call of 0x2e0bf0 after the reset sees A = tracker = player + (1.571, 2.017, -0.008): D x 0.157 and y = B.y + H x 0.157 (1.75 + 0.267). S+4 is 1.0 at the first 0x3313c0 and 0.8768, 0.8152, 0.7844, 0.7689, 0.7612 after the next updates (halving towards 0.7536). The hand-over frame runs the follow update **twice** (push call and manager call; K+0x130 drops by 2 dt): A = 8.89 after the first, 8.27 after the second, tracker 4.594 (recorded stack entry 4.594, 2.001, -0.023). The follow camera object K is not updated while its command is off the stack (K frozen over six sampled updates). Landing 11 to 12 m from the camera (player at (933, 1131), (941, 1124), (941, 1131)): the seed is the camera node position (recorded pose eye 11.147 / 2.735 / 11.147 from the old player position); the filter factors read 0.2166 / 0.2742 after the first update (eye / look, from 0.1296 / 0.1936), then 0.295, 0.3655: f += (1 - f) x 0.1 per update. 0x1cf0b8 moves a ramping slot by (target - f) x rate each update (mode 0), sets the target when within 1e-4, and 0x21a538 sets it at once for rate 1.0.

**Port.** `FollowCamera::takeOver` (seed, clearance reset, near rule), `FilterRamp` and the double update in `CameraRig`. `camera-blend-real-data` pins eye and follow entry error below 3e-3 (measured: street_out 1e-3 eye, 1.8e-3 follow entry; street_seed_far 3.7e-4 and 1.8e-3; street_near_n 2.2e-4; street_near_e and street_near_ne bit-exact) and the filter factors below 1e-5. The facing used for the seed is the player's (-1.5655 in these states), not the camera yaw.

Unknowns:
- After a director camera (top command 0x2665e4) the reset seeds the camera behind the target at the candidate 180 degrees, scaled by the stale clearance (oracle: vehicle camera, [camera-vehicle.md §2026-10-15T12:07](camera-vehicle.md#ts-20261015-120718)); the port does the same.
- Candidate angles after the first are not implemented (the line test 0x22b108 has no counterpart in the collision layer), so a seed with an obstructed first candidate is open. The argument 0 branch (0x21ab20, 0x23b92c) of SnapCamera and the director hand-back are not wired to `takeOver`.
- What the gameplay start does before its first sample: the port keeps `FollowCamera::reset` there.
- The pitch clamp 0x2e0a50 acts only when in+0x48 is 0 and in+0x60 > 0.99; no trace reached it.
