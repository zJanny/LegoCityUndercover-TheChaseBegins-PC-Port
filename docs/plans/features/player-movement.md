# Player movement

## 1. Scope

How the main character moves on foot: stick to heading, walk/run/sprint, facing, ground following, jumping, falling, landing stun, slope slow-down and the slide down steep slopes. Not covered: fighting, climbing, vehicles, other characters' abilities. Level collision itself is [collision.md](collision.md); the camera is [camera.md](camera.md).

## 2. Status

IN_PROGRESS, updated 2026-10-13T21:29:01+0200. Runs in `chase-game` (`--gameplay`, `--start`, `--script`).

## 3. Verified behaviour

- Stick to intent heading, dead zones, atan2 of the game library (bit-exact, qemu-arm differential on 300441 pairs): CTest `runtime-core`.
- Walk 2.3 / run 6.9 u/s with the stepped sprint speed, facing step, no acceleration: derived from `0x203d18` (ATTR_RunSpeed 0.23, walk one third, 30 steps per second) and replayed by `locomotion-real-data` (1114 updates).
- Falling (gravity 0.05*30*dt per update, terminal 0.8, hard landing at fall distance 12): `fall-real-data` (18 traces, 536 updates, 0 wrong).
- Jumping (B, root-motion height curve of jump_minifig / jump_run_minifig, forward speed 0.8 of ground speed, no double jump): `jump-real-data` (12 jumps, 0 wrong).
- Landing stun ends by the animation-finished test (ticks * rate / 3000 >= frames - 1); step-off state 8 is a slide down a steep slope (`slideStep`, `slide-real-data` over 7 recordings, 4 slide exits skipped); slope slow-down of 0x1abe60 (`slopeFactor`, `slope-real-data`, error 0 on 405 run rows).
- Exact library sin/cos (`gameSin`/`gameCos`).

## 4. Stand-ins

- PLAYER-S1 `kSlideLandingStunStandIn` — the soft landing after a slide lasts one update — rule not recovered, see PLAYER-1.
- PLAYER-S2 `gameSin`/`gameCos` use libm for arguments of 12868 or more — the library slow path 0x132ec8 is not replicated.
- PLAYER-S3 chase-viewer plays animations at an assumed 30 fps (`--fps`) although each animation stores its rate (docs/model-viewer.md) — see ANIM-1.
- PLAYER-S4 keyboard `Walk` key (Left Alt, own non-3DS action in `config/controls.cfg`) stands in for a half-tilted circle pad — part of the input mapping, see INPUT-1.

## 5. Open items

- [ ] PLAYER-1 — Why the soft landing after a slide lasts one update (`kSlideLandingStunStandIn`). evidence: [player.md §2026-10-13T21:15](../../re/player.md).
- [ ] PLAYER-2 — Wall response while the box penetrates the slope: the 4 slide exits in the recordings are not reproduced, `slide-real-data` skips them. evidence: [player.md §2026-10-13T21:15](../../re/player.md).
- [ ] PLAYER-3 — Second ground probe of the slide branch (0x22a7a4 path), the 5 s timeout of state 8, surface byte 6 (slippery ground counter, constant 0x23ca4c), moving ground (0x23d3d0), the flag 255 passed to 0x21a73c for surface 20, the sprint scale in state 8. evidence: [player.md §2026-10-13T21:15](../../re/player.md).
- [ ] PLAYER-4 — Choice among overlapping near-flat triangles for the ground normal (14 of 133 rows pick the other one). evidence: [player.md §2026-10-13T21:15](../../re/player.md).
- [ ] PLAYER-5 — The landing animation's root motion drag; `rootMotion` handles only animations with the extra curve. evidence: [player.md §2026-10-13T21:15](../../re/player.md).
- [ ] PLAYER-6 — The other `0x203d18` states (strafing 16/17, 73, 75, 128..135) and `ATTR_RunSpeed` of other character classes are not replicated. evidence: [player.md §2026-10-13T21:15](../../re/player.md).
- [ ] PLAYER-7 — Player completeness beyond walking: fighting, climbing, vehicles, abilities. Not started.
- [ ] PLAYER-8 — The chase-game level start differs slightly from the oracle spawn in lvl_01 (camera reference comparison is framing-only).
- [x] PLAYER-9 — The keyboard walk modifier has its own non-3DS action `Walk` (Left Alt) in `config/controls.cfg` (done 2026-10-13).

## 6. Findings summary

- Heading comes from the two stick slots (14/15) through `stickIntent`; the heading is 0 on every frame without active stick: [player.md §2026-10-13T10:59](../../re/player.md).
- Speed regimes step by stick magnitude (thresholds raw 60, 117/118); movement follows the intent heading; animation root motion goes through 0x20cd30 / 0x1abe60: [player.md §2026-10-13T13:42](../../re/player.md), [player.md §2026-10-13T13:58](../../re/player.md).
- Speeds and jump scales derive from code and data, no fitted constants: [player.md §2026-10-13T17:02](../../re/player.md).
- Fall and jump rules: [player.md §2026-10-13T16:05](../../re/player.md), [player.md §2026-10-13T16:29](../../re/player.md).
- Landing stun, slide, slope slow-down: [player.md §2026-10-13T21:15](../../re/player.md).
- Player entity pointer 0x3772b8, outfit and appearance rule: [player.md §2026-10-13T10:42](../../re/player.md), [player.md §2026-10-13T16:34](../../re/player.md).

## 7. Code and tests

- `src/runtime/player_intent.*`, `player_locomotion.*`, `player_vertical.*`, `player_motion.*`; `src/game.cpp`.
- CTest: `runtime-core`, `locomotion-real-data`, `fall-real-data`, `jump-real-data`, `slide-real-data`, `slope-real-data`, `game-script-run`, `game-script-walk`, `game-script-ledge`, `game-script-jump`.
- Trace dirs: `CHASE_LOCO_TRACE_DIR`, `CHASE_FALL_TRACE_DIR`, `CHASE_JUMP_TRACE_DIR` (defaults under `artifacts/oracle/`). Scenarios: `tools/oracle/loco-*.txt`, `fall-*.txt`, `jump-*.txt`, `slide-ledge.txt`, `slope-run.txt`.
