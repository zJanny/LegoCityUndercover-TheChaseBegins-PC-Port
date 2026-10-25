# Player movement

## 1. Scope

How the main character moves on foot: stick to heading, walk/run/sprint, facing, ground following, jumping, falling, landing stun, slope slow-down, the slide down steep slopes and the attack on foot (punch and combo). Not covered: fighting characters (grab, arrest, counters), climbing and ziplines ([player-traversal.md](player-traversal.md)), vehicles, other characters' abilities. Level collision itself is [collision.md](collision.md); the camera is [camera.md](camera.md).

## 2. Status

IN_PROGRESS, updated 2026-10-19T12:55:42+0200. Runs in `chase-game` (`--gameplay`, `--start`, `--script`); the attack on foot (Y) hits wobbly props.

## 3. Verified behaviour

- Stick to intent heading, dead zones, atan2 of the game library (bit-exact, qemu-arm differential on 300441 pairs): CTest `runtime-core`.
- Walk 2.3 / run 6.9 u/s with the stepped sprint speed, facing step, no acceleration: derived from `0x203d18` (ATTR_RunSpeed 0.23, walk one third, 30 steps per second) and replayed by `locomotion-real-data` (1114 updates).
- Falling (gravity 0.05*30*dt per update, terminal 0.8, hard landing at fall distance 12): `fall-real-data` (18 traces, 536 updates, 0 wrong).
- Jumping (B, root-motion height curve of jump_minifig / jump_run_minifig, forward speed 0.8 of ground speed, no double jump): `jump-real-data` (12 jumps, 0 wrong).
- Landing stun ends by the animation-finished test (ticks * rate / 3000 >= frames - 1); step-off state 8 is a slide down a steep slope (`slideStep`, `slide-real-data` over 5 recordings, 4 slide exits skipped, PLAYER-2); the ground probe's triangle choice agrees with 133 of 133 recorded normals (`fall-real-data`, candidate order of 0x2092d8 flag 8); slope slow-down of 0x1abe60 (`slopeFactor`, `slope-real-data`, error 0 on 91 kept run rows of 405 recorded).
- Exact library sin/cos (`gameSin`/`gameCos`).
- Attack on foot: Y tap (release within 9 held updates), state 0xa3 / 0xa4, combo clips 122..125, facing turn to the target, root step 0.07548, hit message 0 (damage 1) in update 5 / 5 / 4 of combo_1..3, message 62 at the entry; target search and hit test of the original: `player-attack-real-data` (oracle the save state in front of the table of mission 1-2), `game-script-attack-table`.

## 4. Stand-ins

- PLAYER-S1 `kSlideLandingStunStandIn` — the soft landing after a slide lasts one update — rule not recovered, see PLAYER-1.
- PLAYER-S2 `gameSin`/`gameCos` use libm for arguments of 12868 or more — the library slow path 0x132ec8 is not replicated.
- PLAYER-S5 `kPlayerMeleeRangeStandIn` 2.2, `kPlayerAttackAngleStandIn` 90, `kPlayerComboLengthStandIn` 4 — heap values read in the oracle, writers not read. needs: PLAYER-16.
- PLAYER-S6 `kAttackHitTrackStandIn` / `kAttackComboTrackStandIn` — the clip tracks behind the hit and combo-window events, inferred from the measured updates. needs: PLAYER-16.
- PLAYER-S10 The root step of the combo clips is the z translation of the extra joint curve at frame k x framesPerUpdate with linear interpolation between frames; the x and y axes (zero in these clips), the slot weight and the original's interpolation are not read. needs: PLAYER-22. evidence: [player-attack.md §2026-10-19T08:03:14](../../re/player-attack.md#ts-20261019-080314).
- PLAYER-S8 the attack uses libm `acos` for the library acosf 0x2584c8 and treats the line of sight 0x219990 as clear. needs: PLAYER-14.
- PLAYER-S9 `attackTargetObjects` — only `leprop_wobblydestructible` props are attack targets, and the glue allows the attack only in idle / walk / run / sprint on the ground. needs: PLAYER-14, PLAYER-15.
- PLAYER-S11 `kAttackSoundOwnerStandIn` 0 — the owner key of the attack sounds (u16 at the player entity +0x14) is not read; the attack sounds are never stopped, so it has no effect. needs: AUDIO-30.
- PLAYER-S4 keyboard `Walk` key (Left Alt, own non-3DS action in `config/controls.cfg`) stands in for a half-tilted circle pad — part of the input mapping, see INPUT-1.

## 5. Open items

- [ ] PLAYER-25 — The first locomotion step after a placement: bit 4 of ctrl+0x315 clear takes 0x20b48c(..., flag 16, 0.0) instead of 0x289b80 / 0x203b34 (0x20d3bc), and the bits 2 and 3 test at 0x20cf20; what those branches do. blocks: SCRIPT-S275. evidence: [script-natives-flags.md §2026-10-19T15:12:32](../../re/script-natives-flags.md#ts-20261019-151232).
- [ ] PLAYER-1 — Why the soft landing after a slide lasts one update (`kSlideLandingStunStandIn`); the other-animation branch on ext+0x130 bit 5 of 0x1abc5c is ruled out (byte is 0 in the recordings), the landing slot's mode (0x22f560) after a slide is unread. evidence: [player-falling-jumping.md §2026-10-16T10:30:18](../../re/player-falling-jumping.md#ts-20261016-103018).
- [ ] PLAYER-2 — Slide exit: the original keeps the stick step and adds a constant push of (-0.1035, -0.0393) in the update where state 8 ends, and its probe then finds only the flat floor; the replica removes the stick step (corner rule) and still finds the slope. Cause of the push not found; `slide-real-data` skips the 4 exits. evidence: [player-falling-jumping.md §2026-10-16T10:30:18](../../re/player-falling-jumping.md#ts-20261016-103018).
- [ ] PLAYER-3 — Second ground probe of the slide branch (0x22a7a4 path), surface byte 6 (slippery ground counter, constant 0x23ca4c), moving ground (0x23d3d0), the flag 255 passed to 0x21a73c for surface 20, the sprint scale in state 8, and the 5 s timeout of state 8 (read: removes the character through 0x23cd80, the replica has no death or respawn; needs: PLAYER-11). evidence: [player-falling-jumping.md §2026-10-16T10:30:18](../../re/player-falling-jumping.md#ts-20261016-103018).
- [ ] PLAYER-10 — The ground candidate list caps (0x100 triangles per item, 300 entries, 100 grid entries) and the duplicate-record removal (0x209a98 flag 0x40) of 0x1f9884 are not replicated; the entry order across items follows the replica's registration order, not the grid cells. evidence: [player-falling-jumping.md §2026-10-16T10:30:18](../../re/player-falling-jumping.md#ts-20261016-103018).
- [ ] PLAYER-11 — Player death / respawn (the state 8 timeout, falling out of the world) is not modelled. blocks: PLAYER-3. evidence: [player-falling-jumping.md §2026-10-16T10:30:18](../../re/player-falling-jumping.md#ts-20261016-103018).
- [ ] PLAYER-5 — The landing animation's root motion drag; `rootMotion` handles only animations with the extra curve. evidence: [player-falling-jumping.md §2026-10-13T21:15](../../re/player-falling-jumping.md#ts-20261013-211552).
- [ ] PLAYER-6 — The other `0x203d18` states (strafing 16/17, 73, 75, 128..135) and `ATTR_RunSpeed` of other character classes are not replicated. evidence: [player-falling-jumping.md §2026-10-13T21:15](../../re/player-falling-jumping.md#ts-20261013-211552).
- [ ] PLAYER-7 — Player completeness beyond walking: fighting, vehicles, abilities; the climbing moves are in [player-traversal.md](player-traversal.md) (zipline ported). needs: TRAV-1, TRAV-2, TRAV-3, TRAV-4.
- [ ] PLAYER-8 — The chase-game level start differs slightly from the oracle spawn in lvl_01 (camera reference comparison is framing-only).
- [ ] PLAYER-13 — The collision entry of the mesh-kind wobbly props is 0.009 higher than the oracle's entity box (`DonutTable_14` half y 1.52876 against 1.5197); the attack's box test uses the port's value. evidence: [player-attack.md §2026-10-18T19:59:57](../../re/player-attack.md#ts-20261018-195957).
- [ ] PLAYER-14 — The attack's other targets are not ported: characters (grab and arrest search, team test 0x219ba0, weights), the other entity types of 0x21a2d4, and the line-of-sight test 0x219990. blocks: PLAYER-S8, PLAYER-S9. evidence: [player-attack.md §2026-10-18T19:59:57](../../re/player-attack.md#ts-20261018-195957).
- [ ] PLAYER-15 — Other attack paths: the jump attack (0x1d5254 / 0x1d5308), which other callers of 0x20b984 allow the attack, the weapon-out state 0x9a, the damage-99 flag. blocks: PLAYER-S9. evidence: [player-attack.md §2026-10-18T19:59:57](../../re/player-attack.md#ts-20261018-195957).
- [ ] PLAYER-16 — Sources of the attack values: the writers of ctrl+0x80, *0x372e44 and the combo length, the table from clip track hashes to event types. blocks: PLAYER-S5, PLAYER-S6, PLAYER-19. evidence: [player-attack.md §2026-10-18T19:59:57](../../re/player-attack.md#ts-20261018-195957).
- [ ] PLAYER-18 — Next to the table the oracle's root step slides sideways against the table's entity (contact message 45 of 0x21a73c); the port slides against the collision box and ends 0.3 m away from the oracle's position after one attack. evidence: [player-attack.md §2026-10-18T19:59:57](../../re/player-attack.md#ts-20261018-195957).
- [ ] PLAYER-22 — Trap 0x270778 during a punch (the save state in front of the table of mission 1-2) to confirm that the root sampler runs for the combo slot and what it returns per update. blocks: PLAYER-S10. evidence: [player-attack.md §2026-10-19T08:03:14](../../re/player-attack.md#ts-20261019-080314).
- [ ] PLAYER-23 — What 0x2b80f4 does in the hit event before the hit sound is played (callees 0x2554c0, 0x254250, 0x227790, 0x2408c4, 0x25b2b8, 0x22a9ec unread; a hit effect is likely). evidence: [player-attack.md §2026-10-19T12:54:52](../../re/player-attack.md#ts-20261019-125452).
- [ ] PLAYER-24 — Whether the original sounds anything on a punch (animation sound events of the combo clips, other callers); no audio recording of a punch was compared. needs: AUDIO-34. evidence: [player-attack.md §2026-10-19T12:54:52](../../re/player-attack.md#ts-20261019-125452).
- [ ] PLAYER-21 — Look at the punch in `chase-game` (pose against the clip frames, no foot sliding) and check where the original applies the joint orientation (record +0x88) to 0xfa channels. needs: PLAYER-19. evidence: [skeleton-animation.md §2026-10-18T21:40:55+02:00](../../re/skeleton-animation.md#ts-20261018-214055).

## 6. Findings summary

- Combo clips animate through 0xfa raw-matrix joint curves, decomposed by 0x1324f8: [skeleton-animation.md §2026-10-18T21:40:55+02:00](../../re/skeleton-animation.md#ts-20261018-214055).
- Heading comes from the two stick slots (14/15) through `stickIntent`; the heading is 0 on every frame without active stick: [player-entity-input.md §2026-10-13T10:59](../../re/player-entity-input.md#ts-20261013-105911).
- Speed regimes step by stick magnitude (thresholds raw 60, 117/118); movement follows the intent heading; animation root motion goes through 0x20cd30 / 0x1abe60: [player-locomotion.md §2026-10-13T13:42](../../re/player-locomotion.md#ts-20261013-134220), [player-locomotion.md §2026-10-13T13:58](../../re/player-locomotion.md#ts-20261013-135827).
- Speeds and jump scales derive from code and data, no fitted constants: [player-locomotion.md §2026-10-13T17:02](../../re/player-locomotion.md#ts-20261013-170203).
- Fall and jump rules: [player-falling-jumping.md §2026-10-13T16:05](../../re/player-falling-jumping.md#ts-20261013-160500), [player-falling-jumping.md §2026-10-13T16:29](../../re/player-falling-jumping.md#ts-20261013-162920).
- Landing stun, slide, slope slow-down: [player-falling-jumping.md §2026-10-13T21:15](../../re/player-falling-jumping.md#ts-20261013-211552).
- Ground triangle order, slide-exit push, state 8 timeout: [player-falling-jumping.md §2026-10-16T10:30:18](../../re/player-falling-jumping.md#ts-20261016-103018).
- Attack on foot: Y tap, combat action states, target search, hit test and the messages to the target: [player-attack.md §2026-10-18T19:59:57](../../re/player-attack.md#ts-20261018-195957).
- Attack sounds: swing on entering a state and hit on the hit event, from the weapon table row of the character; all ids are 0: [player-attack.md §2026-10-19T12:54:52](../../re/player-attack.md#ts-20261019-125452).
- Player entity pointer 0x3772b8, outfit and appearance rule: [player-entity-input.md §2026-10-13T10:42](../../re/player-entity-input.md#ts-20261013-104227), [player-entity-input.md §2026-10-13T16:34](../../re/player-entity-input.md#ts-20261013-163430).

## 7. Code and tests

- `src/runtime/player_intent.*`, `player_locomotion.*`, `player_vertical.*`, `player_motion.*`, `player_attack.*`, `attack_sounds.*` (glue `src/game_attack.cpp`); `src/game_player.{hpp,cpp}` (rig, placement, per-update step), `src/game_level_update.cpp` (the update order), `src/game.cpp` (wiring).
- CTest: `player-attack`, `player-attack-real-data`, `attack-sounds`, `attack-sounds-real-data`, `game-script-attack-table`, `runtime-core`, `locomotion-real-data`, `fall-real-data` (also pins the ground normals to 1e-5), `jump-real-data`, `slide-real-data`, `slope-real-data`, `game-script-run`, `game-script-walk`, `game-script-ledge`, `game-script-jump`.
- Recorded rows: `tests/data/locomotion_traces.inc`, `fall_traces.inc`, `jump_traces.inc`, `slide_traces.inc`, `slope_traces.inc` (provenance in their headers). Scenarios: `tools/oracle/attack-*.txt`, `tools/oracle/loco-*.txt`, `fall-*.txt`, `jump-*.txt`, `slide-ledge.txt`, `slope-run.txt`.
