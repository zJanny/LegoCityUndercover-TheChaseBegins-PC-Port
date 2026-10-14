# Player locomotion

Walking and running measured in the oracle, the locomotion replica, ground following and the derivation of the speeds.

<a id="ts-20261013-134220"></a>

## 2026-10-13T13:42:20+02:00 — Player locomotion measured in the oracle (worker; Azahar 2126.2 old 3DS, REA Ghidra 12.1.2 for the code)

Method: state `gameplay-start`, `dt 0.033 0.033` (fixes the scheduler step), `stick` injection, `trace ... mem` (entity, controller and node bytes per frame). Scenarios: `tools/oracle/loco-*.txt`. A trace row is the state after the previous update; positions are `node+0x8c` (x, z are the ground plane, y is up). Speeds are world units (u) per second of game dt. CSVs and `.mem` files stay local (`artifacts/oracle/loco-out/`).

**Measured facts**

Speed against stick magnitude (stick (0, M); `loco-magnitude.txt`, `loco-threshold.txt`, `loco-threshold-low.txt`; 75 updates per magnitude):

| raw M | v after the pad pipeline | ctrl+0xc | speed (u/s) | ctrl+0x94 |
|---|---|---|---|---|
| 45, 50..59 | below the dead zone | 0 | 0 (intent 0, no movement) | 1.0 |
| 60 (first active value) | 0.1005 | 9 | 2.301 | 1.0 |
| 61..117 (measured 60, 61, 70, 74, 76, 90, 110, 112..117) | up to 0.703 | 9 | 2.301, constant | 1.0 |
| 118..156 (measured 118..123, 130, 145, 156) | 0.714 and up | 1 | 6.900 for 30 updates, then 10.349 | 1.0, then 1.5 |

- Walk/run switch is exactly at raw 117 / 118. This matches `v = ((M-40)/105 - 0.1)/0.9` (circle clamp and radial dead zone of src/runtime/pad.cpp) and the flag rule `v^2 < 0.5` (117: 0.495, 118: 0.510). Raw 59 is idle and 60 moves: activation is `v > 0.1` (59: 0.0899, 60: 0.1005).
- Within a regime the speed does not depend on M (stepped, not continuous). Values: walk 2.3005..2.3010, run phase 1 6.9000..6.9004, run phase 2 10.3490..10.3504.
- Acceleration and deceleration: none. The first update with the stick active moves at full regime speed (mag156: first step 0.2275 u at dt 0.033 = 6.9 u/s). The first update after release has zero displacement.
- Run phase 2: `ctrl+0x24c` (run timer, f32) gains dt per update in the run state (0.033 after the first update, 1.023 after the 31st). In the update where it exceeds 1.0, `ctrl+0x94` (f32 animation speed scale) becomes 1.5; the displacement rises one update later (30 updates at 6.9, then 10.349 = 1.5 x 6.9). Sequences (`loco-restart.txt`): release for 1 update then push again restarts at 6.9 with timer 0.033 (also after 2 updates); during walk the timer keeps its value (0.66 held for 20 updates in case D) and run re-entry restarts it at 0.033; a walk-to-run change starts the 1 s count when run begins.
- Speed is per second of dt: at dt 0.066 the run speeds are 6.9002 and 10.3503 u/s, at dt 0.0163 (requested 0.0165, quantised) 6.9019 u/s.
- Heading convention: the node forward vector (node+0x7c, +0x84) is (sin h, cos h) for (x, z) with h = s16 * 2*pi/65536 (32768 = pi), and `atan2(fx, fz)` equals ctrl+8 / 10430.378 on all rows (e.g. -24512 -> -2.3501). The displacement direction `atan2(dx, dz)` equals ctrl+0xa / 10430.378 to 4 decimals on every update of `turn_run_r90`: movement follows the intent heading immediately, not the facing. Stick x > 0 adds +16384 (90 degrees) to the camera yaw heading.
- Controller fields (s16 unless noted): ctrl+6 = intent heading of this update (0 when the stick is inactive), ctrl+8 = facing (what the node forward vector shows), ctrl+0xa = last non-zero intent (kept after release), ctrl+0xc held flags, ctrl+0x94 f32 animation speed scale, ctrl+0x24c f32 run timer.
- Facing turn (`loco-turn.txt`, `loco-turn-dt.txt`): per update `facing += sign(d) * min(6400, ceil(f32(0.3) * |d|))` with `d = wrap_s16(ctrl+0xa - facing)` (float32 product, rounded up in magnitude). This is exact on 292 of 295 steps (right/left 90 degrees from rest, 90 degrees while running and walking, 180 degrees from rest and while running, at dt 0.033, 0.066 and 0.0165). The 3 misses are updates in which nothing changed (row 59 of the three rest cases). The cap of 6400 (0.6136 rad, 35.2 degrees) was active in two consecutive updates of the 180 degree case. The turn is per update, not per second: identical step sequences at three dt values. After release the facing keeps converging to the last intent.
- Intent includes the camera yaw (0x376894), and the camera moves: with the stick held sideways the yaw advanced about 0.025..0.045 rad per update (different at dt 0.066 and 0.0165), which rotates the movement direction. The camera law was not recovered.

**Code (REA Ghidra decompile of artifacts/game/game.arm.elf, objdump, execution traps in the oracle)**

- Locomotion is animation root motion. The character mover `0x20cd30(entity, ctrl, flags, deltaMatrix)` runs once per update for the player (trap `loco-trap-mover.txt`: lr 0x1d5168 in the first update, then 0x1c7474; r2 = r3 = 0). Inside it:
  - the delta vector starts as zero (`0x25b390`); `0x2c9770(entity, ctrl, flags, &delta)` adds the animation root delta of this update: `0x2327b4` samples the root track of the playing animations (weights from `0x115ecc`), it is rotated by the node matrix (or by the angle ctrl+0xa when ctrl+8 != ctrl+0xa, scale 2*pi/65536 = 9.58738e-5) and added with `0x23d39c`;
  - after the collision query/slide (`0x21a73c`) the world translation of the node (`0x250810(node) + 0x30`, i.e. node+0x8c) gets x and z added and `0x24c118(node, world)` stores the transform: it derives the local matrix (node+0x1c..0x5c, translation +0x4c) from the world matrix and the parent link and marks the world matrix valid (node word 0 bit 0x1000).
  - Write watchpoints on node+0x4c (`loco-watch.txt`) stopped at 0x20cf60, 0x21b6d8, 0x20b6e0, 0x24c164. With the dynarmic JIT the stop pc is not the writing instruction (block granularity), so these only located the neighbourhood; the call chain was confirmed with execution traps (`loco-trap-move.txt`: 0x20b48c is called from 0x20d59c inside 0x20cd30).
- Speed control is the playback speed of the locomotion animation. `0x1abe60(entity, ctrl)`: `role = 0x219614(entity, 2)` when role is 2, 0x83 or 0x6f and byte 0x51640e != 1: `ctrl+0x94 = 1.0`; on ground (ctrl+0x314 bit 6) a slope term can lower it (`1 - 0.55 * clamp(-2*dot, ..2) * (1 - t)`, dot from `0x221920(node+0x20, ctrl+0x1ec)`, t from ctrl+0x1f0 and *0x376910; not exercised on flat ground); if `0x236b6c(entity) == 0` then `ctrl+0x24c += dt` (0x23602c) and above 1.0 it switches to role 12 (`0x219614(entity, 12)`), calls `0x2744d0(2.5)` and multiplies ctrl+0x94 by 1.5 (1.3 when `0x219b80(entity)` and `0x2cd540()` are both non-zero). It starts the animation with `0x21941c(0.1, 1.0, entity, id, 1, 0)` (0.1 = blend time). Only direct caller found: 0x1b81f8.
- Animation roles: `0x219614` looks the role up in the table at `*0x374294` (heap, 0x08370f80 in the checkpoint): 14-byte records of 7 s16 `[default, v1..v6]`, variant chosen by `0x2106d8(ctrl, 0..2)` and bit 4 of `*(ctrl+0xe0)+0x130`; 0x271 (625) means none. Record 2 = (2, 111, 131, 151, 162, 552, 272), record 12 = (281, 282, 283, 625, ...). Names (`tables/tablesanimations.bxls` sheet 1, row = id): 1 "walk", 2 "run", 111/131/162 "run" (class variants), 151 "itemleft_run", 272 "detect_run", 281/282/283 "sprint", 552 "supercarry_walk". So the regimes are: "walk" (id 1) 2.301 u/s, "run" (id 2) 6.9 u/s, "sprint" (id 281, scale 1.5) 10.349 u/s. That walk is role 1 in this walk branch was not traced (the walk path of 0x1abe60 caller was not read).

**Fitted model (for the native runtime; fit quality in brackets)**

```
v        = ((|stick| - 40) / 105 - 0.1) / 0.9          pad.cpp pipeline; activation and walk/run thresholds reproduced exactly
active   = per-axis v > 0.1 (player_intent, unchanged)
walk     = v^2 < 0.5
h_target = (s16) intent heading incl. camera yaw        (matched on every row)
speed    = walk ? 2.301 : (run_timer <= 1.0 ? 6.900 : 10.349)   u/s       (flat within +-0.001 u/s)
run_timer: += dt each run update (0 at run re-entry), frozen while walking, reset at release
pos     += speed * dt * (sin h_target, cos h_target) on (x, z)           (direction error < 1e-4 rad)
facing  += sign(d) * min(6400, ceil(f32(0.3) * |d|)),  d = wrap_s16(h_target - facing), once per update   (292/295 exact)
```
Run phase 2 applies from the update after the one in which the timer exceeds 1.0 (timer 1.023 at the 31st update, speed 10.349 from the 32nd). The speeds are measured at the output; the derivation from the animation root tracks is open.

Evidence IDs: 0x20cd30 ev_08be8f79c89f4d1984d08dabdcfbd908726d71b671c760d83b853e5ff2cd099a; 0x2c9770 ev_b79fe7bc332011fda70ecd8cec9943a5a92fe0b02a19f678fc95ec1adfd799e0; 0x2327b4 ev_683e74f616e2832d7722dde5b10a033f9e333c58c401d19dbb67eea5328138de; 0x1abe60 ev_15b67eaf9c47398f51e5dab31750331acf7a38d55d39f60f8763bb7b656a6f7a; 0x219614 ev_0689721821644b94e66cfb4f60ac76ec44997215859c15bff10b270aa3425906; 0x1b81f8 ev_fd8a216034ea7e62e0104115dc94c4f5d943b0e1382a1e80f972d6fa960dccee; 0x2b01e4 ev_221c2f32… (target-following branch, not used by the stick path).

Unknowns:
- Root speed of the walk, run and sprint animations from the `.bfnanm` data and how it relates to 2.301 / 6.9 / 10.349 (measured at the output, not derived).
- The code that applies the 0.3 / 6400 facing step and the code that resets the run timer on release (writers of ctrl+8 and ctrl+0x24c were not located; a watch on them was not run).
- Why no update occurred at row 59 in the three rest turn cases (always the same row after the stick start; not seen in straight pushes).
- Camera yaw law (writer of 0x376894), real slope behaviour of ctrl+0x94, the 1.3 variant, other characters and costume variants, vehicles, collision slides at obstacles, ground following (y).
- Whether free-run dt (about 0.0333, ticks of 1 ms) changes any of the numbers: all measurements used a fixed dt in stepped mode.

<a id="ts-20261013-135827"></a>

## 2026-10-13T13:58:27+02:00 — Locomotion replica, root motion check, differential test (worker; recorded traces in artifacts/oracle/loco-out, REA Ghidra 12.1.2 for 0x2327b4)

Corrected by [player-locomotion.md §2026-10-13T14:46](player-locomotion.md#ts-20261013-144645): the height steps of the ramp are the ground probe at the kerb
Corrected by [player-locomotion.md §2026-10-13T17:02](player-locomotion.md#ts-20261013-170203): the stick movement is not animation root motion

Replica: `locomotionUpdate`, `turnFacing`, `kMeasuredRootMotionSpeeds` in src/runtime/player_locomotion.{hpp,cpp}; used by chase-game (src/game.cpp). Tests: runtime-core (synthetic thresholds, timer, facing rule), `locomotion-real-data` (tests/locomotion_real_test.cpp, differential against 33 recorded traces).

**Update order of one step (fits all traces)**
1. Stick intent (`stickIntent`) is evaluated with the camera yaw from before the camera moves in the same update: with the yaw of the previous trace row the heading equals ctrl+6 to within 1 unit on 338 active rows; with the yaw of the row itself (the camera turns 0.03..0.045 rad per update in the traces) it is off by up to 1280 units.
2. If the stick is active: lastIntent (ctrl+0xa) = heading. Facing (ctrl+8) takes one `turnFacing` step toward lastIntent, also while idle.
3. Regime: idle (stick inactive), walk (flag 8 set), run. Movement = speed(role at the start of the update) * dt * (sin h, cos h), h = lastIntent. Walk 2.3008, run 6.9002, sprint 6.9002 * 1.5 u/s.
4. Run timer: reset to 0 when the previous update was not a run update, then += dt; above 1.0 the sprint role is active from the next update (scale 1.5). Walk and idle clear the sprint role (scale back to 1.0 at once, also on release from sprint) and leave the timer value untouched.
- Confirmed by cross-file transitions: idle to run moves at full run speed in the first update (6.9, restartA_2 -> A_3), run to walk at walk speed in the first update (2.301, D_1 -> D_2), walk to run at run speed (C_1 -> C_2), release from sprint: zero movement and scale 1.0 in the first update (mag156_push -> mag156_release).

**Differential test numbers** (`locomotion-real-data`, 1114 updates over 33 traces: pushes at raw 60/70/110/130/156, releases, the four restart scenarios, the turn scenarios at dt 0.033/0.066/0.0165)
- Per-update movement: max error 7.8e-5 u (tolerance 2e-4: float32 positions at 870..1110 have an ulp of 6e-5..1.2e-4; the measured speeds are known to +-0.0003 u/s, 1e-5 u per update at dt 0.033).
- Accumulated position over a whole trace (45..75 updates, state not resynchronised): max drift 1.5e-3 u (limit 0.01).
- Run timer (restart traces, the only ones with ctrl+0x24c): identical float values (error 0).
- Facing: exact on every compared update; three updates (row 25 of turn_rest_r90, _l90, _180) are resynchronised, see below.
- Animation scale: 1.0 / 1.5 within 2e-3 (the game shows 0.99999994 and 1.4999 on some rows).
- Stick to heading with the previous row's yaw: max 1 unit over 338 rows.

**Findings from the traces**
- Updates with no facing step: in each of the three rest turns (row 25, frame 59) the player moves along the old direction but ctrl+8 does not change; in r90 and l90 the latched ctrl+0xa also still holds the old intent while ctrl+6 already shows the new one. A sampling artefact or a skipped controller step; the same frame number every time suggests a periodic event. The test resynchronises the facing there (at most 3 allowed).
- Slope: in turn_run_r90 the player climbs a ramp (y 16.58 -> 16.78 at rows 26..30). ctrl+0x94 drops to about 1.33 and the speed to about 9.15 u/s for 5 updates, then returns to 1.5 (confirms the slope term of 0x1abe60 described above). Six rows are skipped by the test; ground height and slope are not modelled in the native runtime. The scale deviates by 1e-4..1.5e-4 on several rows (0.9999999 from row 9 of the rest turns, 1.4999 after the ramp) without a visible speed change.

**Root motion from animation data: does not match, measured speeds are used**
- 0x2327b4 (decompiled again): per active slot (0x58 bytes at instance+0x2c) it samples a root track block of the animation (`0x253ea0(anim+0xc, 1)`, evaluator 0x270778, channel mask `*(anim+8) & 0xf`), subtracts the part belonging to a wrapped loop and adds the weighted per-frame delta. A second path (slot flag 0x1000, or the 0x376750 branch via 0x1fa1e4) adds `slot bytes +0x28..+0x2b / 63 * dt * weight` as a velocity. Which path the walk/run/sprint animations take was not traced.
- Data (char_minifig class_police walk 12 frames, run 12 frames, sprint 20 frames; checked in the real-data test): the pelvis joint curve (joint 0, the long curve with frames + 1 samples) has no net x/z travel over the loop (x -0.0206 .. 0.0244, z constant 0.075 for walk; z 0.0381 run; z -0.2 sprint, first = last frame). The two sub-animations hold a footstep pulse track (hash 0x68cc6917, 5..6 keys, value 1 twice per cycle at normalised times 0.329/0.832 walk, 0.458/0.957 run, 0.095/0.596 sprint) and eight constant tracks (dcd633e0..e2 = 0, 4f44e168 = 0, 7e190104..06 = 1, 301764d5 = 1). So no stride length or speed can be derived from these files, and 6.9 u/s, 2.301 u/s (ratio 3.0003) and 1.5 x run cannot be checked against them.
- Consequence: `kMeasuredRootMotionSpeeds` (walk 2.3008, run 6.9002, sprint scale 1.5) is PROVISIONAL; it is measured at the output only.

**Native runtime**
- chase-game drives the player with the step inside the scheduler update, plays idle/walk/run/sprint (class_police files) at 30 frames/s times the role scale (rate assumed, phase kept across switches, no blending), keeps the head attachment, and places the world matrix from (x, spawn height, z) and the facing (rows (c, 0, -s), (0, 1, 0), (s, 0, c), translation). The camera is a follow placeholder whose yaw feeds the stick intent. Height stays at the spawn height (the nav-grid height of the 2026-10-12T23:55 entry is data-fitted within 1..6 percent and sits 0.1 under the floor).
- `--script "X Y COUNT;..."` (raw stick, dt 0.033): `0 150 40` moves the player 10.133 u (31 run updates 0.2277 u each, 9 sprint updates 0.3416 u), `0 90 40` moves him 3.037 u; both are ctest cases.

Unknowns:
- Source of the speeds in the original (data or script value, root track path versus the byte-velocity path of 0x2327b4, role table scale); stride speed of other characters and costume classes.
- Whether ctrl+0xa is updated for an active stick with heading 0 or only for a non-zero heading (the implementation uses "active").
- Initial value of ctrl+0xa/ctrl+8 at spawn (the implementation takes the Player1Start yaw), and behaviour of run timer and sprint role when walking from sprint (the implementation clears the sprint role at once; only release from sprint was measured).
- The cause of the skipped facing step at frame 59, the slope law (clamp and t terms of 0x1abe60), ground following in y, collision slides.
- Camera yaw law (writer of 0x376894) and the animation playback rate (frames per second) and blend of the real game.

<a id="ts-20261013-144645"></a>

## 2026-10-13T14:46:45+02:00 — Ground following and collision slides recovered (see collision.md)

- The unknowns "ground following in y" and "collision slides at obstacles" of the locomotion entries are answered in [collision.md](collision.md): after the horizontal move (0x21a73c slides it along wall triangles) 0x20b48c bisects the height of the floor under the player's box and keeps the old height within 0.01.
- [Corrects player-locomotion.md §2026-10-13T13:58:27](player-locomotion.md#ts-20261013-135827), "slope" finding above: the height steps of the turn_run_r90 ramp (16.577, 16.672, 16.794, 16.783) are this probe at the kerb, not a modelled slope; the speed slow-down on that ramp (ctrl+0x94 1.33) is the separate slope term of 0x1abe60 and is still not modelled.
- ctrl+0x21c/+0x220 hold the x and z the slide changed, ctrl+0x224 the movement before the slide (type 27 only), ctrl+0x240/+0x244 the last ground height, ctrl+0x1e4/+0x1e8 the ground object and triangle, ctrl+0x1ec the ground normal.

<a id="ts-20261013-170203"></a>

## 2026-10-13T17:02:03+02:00 — Derivation of the walk/run speeds, jump scales, 0.8 factor and landing stun (worker; REA Ghidra 12.1.2 xrefs, objdump of artifacts/game/game.arm.elf, Azahar oracle traps without RPC)

Replica: `kCharacterSpeeds` (src/runtime/player_locomotion.hpp), `makeJumpProfile`, `landingDurations`, `kJumpHeight` (src/runtime/player_vertical.*), `rootMotion`, `animationFramesPerSecond`, `animationDuration` (chase_assets). Tests: `runtime-core`, `assets-core` (synthetic extra curve), `locomotion-real-data`, `fall-real-data`, `jump-real-data`. **Corrections** of the 13:42:20 and 16:29:20 entries are marked below.

**Walk and run speed (found).**
- **[Corrects player-locomotion.md §2026-10-13T13:58:27](player-locomotion.md#ts-20261013-135827):** the stick movement is not animation root motion. In `0x20cd30` the call `0x2c9770` returns 0 for the walk, run and sprint animations (trap run: `0x270778`, the root sampler, was never reached in walk or run; in `0x2327b4` the slot flag 0x1000 is clear (0x2400 in the slot dump) and `[animObj+8] & 0xf`, the root channel mask, is 0, so the loop at 0x232a20 skips the slot and the return value stays 0). `0x20cd30` then calls the fallback `0x203d18(entity, ctrl, &delta)` (0x20ce00, `bics` with ctrl+0xc bit 0) when the stick is held.
- `0x203d18` (types outside 128..135, the player is 27): `step[0] = dt * 30.0 * (ctrl+0x8c * ctrl+0x94)`, `step[1] = dt * 30.0 * (ctrl+0x90 * ctrl+0x94)` (0x203d8c..0x203dc4, literal 30.0 at 0x204014, dt from 0x23602c). The index is `(ctrl+0xc >> 3) & 1`, forced to 1 by bit 3 of `*(ctrl+0xe0)+0x130`; `0x225754` returns 2 only for an entity carrying another one (ctrl+0x13c of type 0x7a), then step[1] is multiplied by `*0x374368`. The vector (sin h, cos h) of the heading ctrl+0xa (scale 2*pi/65536, literal 0x38c90fdb) is multiplied by the step and added to the delta (0x203ea4..0x203ecc).
- ctrl+0x8c = `ATTR_RunSpeed` read by the character set-up `0x236d74` (0x236ec0: `0x241280(entity, "ATTR_RunSpeed", 0, default)`; `0x241280` hashes the name and calls lookup 0x217080, returning the default when the class file does not list the name). The default is the float at `0x37450c`, stored by the one-line initialiser `0x343de4` (literal 0x343df4 = 0x3e6b851f = **0.23**). `chars/char_player.blvl` lists no `ATTR_RunSpeed` (hash 0x412e0b01 is absent from its +0x10 and +0x14 tables, probe over `parseLevel`). ctrl+0x90 = ctrl+0x8c * 0x3eaaaaab (**1/3**, literal 0x2371b0), ctrl+0x94 = 1.0 (literal 0x2371b4). Oracle: ctrl+0x8c = 0.23, ctrl+0x90 = 0.076667, ctrl+0x94 = 1.0 in every locomotion trace.
- Result: run 0.23 * 30 = 6.9 u/s, walk 6.9 / 3 = 2.3 u/s, sprint x 1.5 (literal 0x3fc00000 at 0x1ac154; the 1.3 variant is the literal 0x3fa66666 at 0x1ac150). The measured 6.9002 / 2.3008 u/s were float noise of the positions. `locomotion-real-data` with the derived speeds: max movement error per update 6.1e-5 u (7.8e-5 before), accumulated drift 0 u over 45..75 updates (1.5e-3 before).
- The 0x1abe60 slope term and the other `0x203d18` states (16/17 strafing, 8, 68, 73 and 75 with ctrl+0x308) are not replicated.

**Animation playback rate (found; [Corrects skeleton-animation.md §2026-10-12T22:10](skeleton-animation.md#ts-20261012-2210): "rate unknown").** Slot +0xc is the clock rate returned by `0x108190` (entry +0x10 of the type table at 0x52ce8c, built at run time), slot +0x30 = rate * the speed argument; both seen as 15 (0x41700000) in the walk and jump slots. `0x108190` takes bits 13..14 of the first word of the joint table as an index into the u16 table `{15, 30, 60, 0}` at 0x376764 (static image, `0f 00 1e 00 3c 00 00 00`); without a joint table it uses header u16 +4 >> 14. Police data: all walk, run, jump, land, fall, idle animations 15 fps, sprint 30 fps (word 0x200a2015).

**Jump height scales (found).**
- Entering state 4, `0x200cc4` stores ctrl+0x88 = `*0x3742a0` = **2.1** (set at start-up by `0x3468c8`, which stores 0.75, 0.1, 2.1, 1.5, 0.65, 0.25, 0.4, 0.3, 2.0, 1.0 at 0x374298; 2.1 is the literal 0x346908; oracle ctrl+0x88 = 2.1 in the jump traces). After the animation starts, `0x200e70..0x200ecc` (when `[animObj+8] & 2`) calls `0x2147e4(slot, &min, &max)` and stores `slot+0x1c = ctrl+0x88 / (max.y - min.y)`; slot+0x18..0x24 is the root-motion scale vector (1.0 from `0x245b40`) that `0x2327b4` multiplies into the root delta.
- `0x2147e4` -> `0x292ef8` samples the **extra joint curve** (joint table word bit 28 set: curve index = word & 0x1fff, the last curve, e.g. joint 21 of the 22 curves of jump_minifig) for frames 1..n (n = bits 15..27 of the word plus bit 29, the index clamped to n-1), subtracts the frame 0 translation (`0x227790`, vector subtraction) and keeps the componentwise minimum and maximum, both starting at 0 (`0x23a758` zeroes them). If max.y is 0 it is replaced by 1.0.
- Data: jump_minifig max.y 2.4325, min.y -0.006875, scale 2.1 / 2.439375 = **0.8608763**; jump_run_minifig max 2.524375, min -0.01125, scale **0.8281981**. Oracle: trap `0x115ecc` during a standing jump shows slot+0x1c of the jump slot (0x0856a510) = 0x3f5c6263 = 0.8608762622, equal to the derived value to the last digit. The fitted 0.860869 / 0.828197 are replaced.
- **[Corrects player-falling-jumping.md §2026-10-13T16:29:20](player-falling-jumping.md#ts-20261013-162920):** the jump height profile is the extra joint curve, not the sub-animation track 0xdcd633e1. The track is a Bezier approximation of it, which explains the residual of 3.7e-4 noted at 16:29:20; with the joint curve (linear between frames, 15 fps, (frames - 1) intervals) the worst airborne height error over 264 updates is 1.7e-4. The apex is 2.0941 (jump) and 2.0907 (jump_run), not 2.1, because the extent includes the small negative minimum.
- The state is written by the watch at `0x0856a52c` from the block that ends in the call at `0x200f10` (dynarmic reports the block start; the code path `0x200cc4..0x200edc` is the only writer found by reading).

**The 0.8 horizontal factor after take-off (found; [Corrects player-falling-jumping.md §2026-10-13T16:29:20](player-falling-jumping.md#ts-20261013-162920): "ctrl+0xa0").** It is the literal 0x3f4ccccd at 0x204028 in `0x203d18` for the states 4 (jump) and 68 (0x203e84..0x203ff8: speed * (sin * 0.8)). That ctrl+0xa0 also holds 0.8 is a coincidence of values; ctrl+0xa0 was not traced and is not the source.

**Landing stun (derived, partly inferred).** The landing animations land (14 frames) and fallland (19 frames) at 15 fps last (frames - 1) / 15 = 13/15 s = 26/30 s and 18/15 s = 36/30 s, exactly the measured stun lengths (bounds (0.864, 0.880] and (1.188, 1.221] from 27/14/54 and 37 updates). `landingDurations` uses `animationDuration`. Which function ends states 6 and 7 (it should be the "animation finished" test of 0x24cbd4 for the clamped slot) and whether state 6 plays "land" and state 7 "fallland" were not read; the match of both numbers is the evidence. This remains an inferred rule, listed as a stand-in at the time; replaced by the original rule in the 2026-10-13T21:15:52 entry ([features/player-movement.md](../plans/features/player-movement.md)).

Unknowns:
- The code that ends the landing states 6 and 7 (see above); the soft landing animation root motion (the drag of 0.1 / 0.88 m).
- The root joint of an animation without the extra curve (the byte read through `0x24b8bc(skeleton, 0)` in `0x292ef8`); `rootMotion` returns nothing for those, which the jump animations do not need.
- Which writes of slot+0x1c other than 0x200e70 exist (other characters, jump_double 215, costume classes); ctrl+0xe0+0x130 bit 3, ctrl+0x308 and the states 8, 16, 17, 73, 75 of `0x203d18`.
- Whether `ATTR_RunSpeed` is listed in other character classes (the 192 characters of tablescharacters; only char_player was probed) and what `0x225754` mode 2 (carrying) does to the speed.
