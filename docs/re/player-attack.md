# Player attack on foot

Chase's punch on foot: the button, the combat action states, the target search, the hit test and the messages the attack sends to the object it hits. Prop reactions to the hit (wobbly props, the table of mission 1-2) are in [vehicles-physics-props.md](vehicles-physics-props.md) and [debris.md](debris.md).

Subtopics: [player-attack-characters.md](player-attack-characters.md)

## Current state

- Y tap: 0x1bb690 counts the held updates of ctrl+0xc bit 0x20 (Y sets 0x60 in 0x200354). A release after 1 to 9 held updates sets the tap edge ctrl+0xe bit 0x20. A hold of 10 or more updates sets the held bit and no attack follows. The attack starts in the release update. [§2026-10-18T19:59:57](#ts-20261018-195957)
- The ground action handler 0x20b984 ends in 0x1fdbd4, which takes the tap edge (button test 0x210b50 mode 0) and calls 0x214f40. That function requests state 0xa3, or 0xa4 when 0xa3 is already running (`GOCSCombat_CombatActionState` / `...RepeatState`, objects 0x5152c8 / 0x5152dc, vtable 0x363d88: enter 0x307e7c, update 0x3085dc, exit 0x3084f4). Chase also punches without a target. [§2026-10-18T19:59:57](#ts-20261018-195957)
- Target: 0x219c08 (range ctrl+0x80 = 2.2, angle *0x372e44 = 90 degrees, along the stick heading or the facing) picks the lowest score of 0x21a1d8, len x (1.25 - cos) x 2 x weight, among entities that pass 0x21a2d4. The candidate's half width and depth beyond the player's are added to the range. The running attack (state 2 with the stick) uses 1.5 x the range. [§2026-10-18T19:59:57](#ts-20261018-195957)
- Clips (0x18d15c): variant 0 plays 0x7a + combo index (combo_1, combo_2, combo_3, row 125 "combo_1"). The index counts up to the record's combo length 4 and is reset when the attack ends. The running variant 3 plays 0x7e. [§2026-10-18T19:59:57](#ts-20261018-195957)
- Timing (oracle, the save state in front of the table of mission 1-2):
  - combo_1 lasts 14 updates; the facing turns toward the target heading ctrl+0xa with the 0.3 rule of 0x1f3f1c.
  - The root step is 0.07548 per update from the second update on, along ctrl+0xa (track 0xdcd633e2).
  - The hit comes when the clip passes the key of track 0xb684fd50: update 5, 5 and 4 of combo_1, combo_2 and combo_3.
  - A tap after the combo-window key (track 0xc99766ba) starts the next step one update later.
  [§2026-10-18T19:59:57](#ts-20261018-195957)
- Messages to the target, all sent through 0x244fc8, sender the player:
  - 62 in the enter when the target's node is within 2.2.
  - 0 at the hit (0x319ffc -> 0x1f7f28 hit test), record {0, player, 0, damage 1, 0, kind 1, 0}.
  - Message 45 comes from the wall slide 0x21a73c, not from the attack.
  [§2026-10-18T19:59:57](#ts-20261018-195957)
- The joint curves of the combo clips are 0xfa raw matrices; they are decoded now (cause of the missing punch animation): [skeleton-animation.md §2026-10-18T21:40:55+02:00](skeleton-animation.md#ts-20261018-214055)
- The root step of the combo clips is the z translation of the extra joint curve (joint 21: 0.07549 per frame, 1.05684, 0.90586 and 0.67939 at the end of combo_1..3), sampled by 0x2327b4 through 0x270778; the equal sub-animation track 0xdcd633e2 is not its source. [§2026-10-19T08:03:14+02:00](#ts-20261019-080314)
- Sounds: the enter 0x307e7c plays the weapon table (*0x376050) u16 +0xc through 0x23ae84, the hit event 0x319ffc plays +0xa when the hit test passes; the weapon row is byte +0x17 of the character record. Both ids are 0 for every character (id 0 plays nothing, 0x23fa54), so the attack is silent through these hooks. Port: `src/runtime/attack_sounds.*`. [§2026-10-19T12:54:52](#ts-20261019-125452)
- Characters as targets, the grab test and the fight move on a goon: [player-attack-characters.md](player-attack-characters.md).
- Port: `src/runtime/player_attack.*`, glue `src/game_attack.cpp` (stand-ins in [player-movement-attack.md](../plans/features/player-movement-attack.md)). [§2026-10-18T19:59:57](#ts-20261018-195957)
- Open: the track-to-event table, the writers of the range, the angle and the combo length, the line-of-sight test 0x219990, the jump attack 0x1d5308, the weapon-out state 0x9a, and the exact update order of the tap read in the attack states.

<a id="ts-20261018-195957"></a>

## 2026-10-18T19:59:57+02:00 — The attack on foot: Y tap, states 0xa3 / 0xa4, target search, hit test and messages (REA, objdump, Azahar GDB traces of the save state in front of the table of mission 1-2)

Corrected by [player-attack.md §2026-10-19T08:03:14+02:00](#ts-20261019-080314): the root step of the combo clips comes from the extra joint curve (joint 21), not from the sub-animation track 0xdcd633e2; the values are equal.

Corrected by [player-attack.md §2026-10-19T12:54:52](#ts-20261019-125452): the sounds of the attack come from the weapon table row of character byte +0x17 (+0xc on entering, +0xa on the hit), not from "the melee entry".

### Method

- REA (Ghidra 12.1.2) pseudocode: 0x200354 (labelled FUN_0026be04), 0x1bb690, 0x1fdbd4, 0x210b50 with its table *0x377554 (filled at 0x17560c: mode 0 = 0x25d504 edge test of ctrl+0xe, mode 1 0x25d530, mode 2 0x25d58c), 0x2106d8, 0x21a1d0, 0x2108c0, 0x1b7358, 0x219b80, 0x245b00, 0x21e798, 0x214f40 (objdump), 0x307e7c, 0x3084f4, 0x319ffc, 0x3204dc, 0x18d15c, 0x2a39a8, 0x21a2d4, 0x219990, 0x1e740c, 0x2253a8, 0x205fbc, 0x1fa7e0, 0x219c08, 0x1f7f28, 0x21a1d8, 0x23d4c0, 0x22103c, 0x23de70, 0x2507dc, 0x275c00, 0x244fc8, 0x20cd30, 0x2c9770, 0x2b01e4, 0x1f3f1c, 0x1ff030.
- llvm-objdump (`--triple=armv6k --mattr=+vfp2`): 0x3085dc (no Ghidra function), 0x214f40, 0x20b984..0x20bca0, 0x2833a0, 0x32b900, the state object init 0x34c7f0..0x34c934 (names `GOCSCombat_AnimationFinished`, `GOCSCombat_AnimationEvent`, `Combat Move Event`, `GOCSCombat_CombatActionState`, `GOCSCombat_CombatActionRepeatState`), 0x257e78, 0x257ea0, 0x257f14, 0x20e968, 0x21a73c.
- Oracle: the save state in front of the table of mission 1-2. Own emulator: port 24731, own home, OpenGL, no RPC, dt fixed at 1/30. Scenarios (committed):
  - `tools/oracle/attack-tap.txt` (run atap): Y held one update, trace with memory.
  - `attack-tap-ctrace.txt`: ctrace of 0x2414b4 and 0x214f40.
  - `attack-tap-hit.txt` and `attack-combo-hit.txt`: traps of the wobbly handler 0x2a3390.
  - `attack-variants.txt`: memory reads, Y held 3 and 12 updates, two-tap combos.
  - `attack-record.txt`: the combat record, and taps facing away from the table.
- Rows below count updates after the load as in the trace files; the Y press is held in row 6. Trap frame labels are one lower than rows.

### Button

- 0x200354 maps slot 9 (Y) to ctrl+0xc |= 0x60 while held (no edge, no event). At the end it calls 0x1bb690(ctrl). For bit 0x20 (counter 0x374510) and bit 0x40 (0x374511): while held, the counter counts up to 0xff and the bit is cleared while the counter is below 10. On release, a counter of 1..9 sets ctrl+0xe bit 0x20 / 0x40, then the counter is reset.
- Oracle:
  - atap: Y held in row 6, state 0xa3 and ctrl+0xe = 0x60 in row 7.
  - ahold3: Y held in rows 6..8, state 0xa3 in row 9.
  - ahold12: Y held in rows 6..17; ctrl+0xc = 0x60 from row 15 (10th held update) to 17, then state 1 throughout.
- 0x1fdbd4 tests mask 0x20 mode 0 (`ctrl+0xe & 0x20`). Conditions: the character's ability 0x13 (0x23a87c), character table byte +0x17 not 0, nothing carried (ctrl+0x13c 0) and state not 0x101. It is reached from the ground action handler 0x20b984 (tail call at 0x20bca0, after the B jump test). Its fourth argument is the "stick moved" result of the mover (0x1d5650: `r2 = 0x20cd30(...)`). With that argument set and ctrl+0x6c = 2 (run) the attack is the running variant 3.
- For the player, 0x1fdbd4 first searches with 3.0 / 45 degrees (*0x372e50 / *0x372e4c) for a character to grab or arrest. A prop is no character (0x245b00), so the melee search follows.
- Without a target, the weapon flags of the character's melee entry decide between state 0x9a (weapon out) and the attack. Oracle aaway, facing away from the table: no target (ctrl+0x140 = 0), and the tap still enters 0xa3 in the same update. Chase therefore never takes the 0x9a branch here. ctrl+0x2cd goes from 0 to 18 at the first attack (0x22af80) and stayed 18 for the 300 traced updates after it.

### Target search 0x219c08 and score 0x21a1d8

- The direction is the node forward row (+0x20, +0x28), or the stick heading ctrl+6 through the sincos 0x249038 while the player's stick bit is set.
- For the player, a range up to *0x377538 (5.0) uses the near list of 0x23d4c0; a range above *0x372e5c (5.0) walks all rooms.
- Per candidate:
  - 0x21a2d4(candidate, player, 9) must pass: entity+0x10 bit 0x200 (the collision flag) set and bit 0 clear. Non-characters pass only for types 100, 155, 85, 119 (wobbly prop), 214, and 19 with +0x90.
  - Extra range: candidate half x minus the player's (0.6) when larger, plus the same for z (0.52).
  - The candidate's box-centre height is replaced by the player's when the player's node height lies within the candidate's vertical span.
  - The weight is 0x1b7358: 1.0 for a prop.
  - The score is 0x21a1d8(range + extra, angle, weight, player box centre, direction, candidate box centre). v = candidate - player is normalised (0x257ea0: length 0 or less than 2^-23 gives (0, 0, 1)). The candidate must lie within the range and at an angle acos(dot) of at most angle x pi / 180. Its score is len x (1.25 - dot) x 2 x weight.
  - The lowest score wins and is stored in ctrl+0x140.
- Values read from the heap (attack-variants): *0x372e40..0x372e60 = 65, 90, 30, 45, 3.0, 0.1, 1.0, 5.0, 5.0; *0x377534 = 27, *0x377538 = 5.0; player ctrl+0x80 = 2.2; player entity +0x5c = 1.16314, +0x60 = (0, 1.15, 0.08), +0x6c = (0.6, 0.85, 0.52).
- DonutTable_14 (entity 0x08c99320, type byte +0x12 = 0x77, +0x10 = 0x0740): radius 2.6192, box centre (0.0016, 1.5197, 0), half (1.5243, 1.5197, 1.4925). The port's collision entry has the same x / z values but a height of 1.52876 (PLAYER-13).

### States 0xa3 / 0xa4

- 0x214f40(entity, ctrl, variant, flag, arg): stores the variant in combat record byte +8 (record = *ctrl+0xe0, first word), then requests 0xa4 if ctrl+0x6c is 0xa3, else 0xa3.
- Enter 0x307e7c:
  - record +9 = +8; record +0x10 is set to 1 when ctrl+0x314 bit 6 is set and the stick bit is set, else 0. This clears the combo-window bit 3.
  - For variants 0 and 3: 0x22af80 when ctrl+0x2cd bit 1 is clear, and message 39 to the player's own items ctrl+0xf0 / ctrl+0xfc.
  - The player's target heading comes from 0x205fbc: ctrl+0xa = atan2 of the target's box centre (node position without the collision flag) minus the player's node, when the horizontal distance exceeds the player's entity+0x6c (0.6).
  - Message 62 goes to the target when the 3D distance of the node positions is below ctrl+0x80. Then sound table entry +0xc of the melee entry plays, and the clip of 0x18d15c(entity, variant, ...) starts with blend 0.2.
- 0x18d15c, variant 0: clip 0x7a + low nibble of record byte +0xf. The nibble counts up, back to 0 when the high nibble (combo length) is not above it. Variant 3: 0x7d against a target of type 0x43 / 0xcc, else 0x7e.
  - Combat record 0x085698b0 before the attack: bytes +8..+0xf = 00 00 ff ff ff ff 00 40 (combo length 4, index 0); during the attack +0xf = 0x41; after the attack 0x40 again.
  - Exit 0x3084f4 clears the low nibble when the next state is not 0xa3 / 0xa4.
  - tablesanimations sheet 1: 122 combo_1, 123 combo_2, 124 combo_3, 125 combo_1, 126 combo_1 (class_generic). combo_1 has 15 frames at 30 per second (length 14), combo_2 13 frames, combo_3 10 frames.
- Update 0x3085dc:
  - For variants 0, 3, 5, 6, 7 it sends message 39 to the own items.
  - For the player with record +0x10 bit 3 (the combo window) it calls 0x1fdbd4 again, so a tap enters the next state.
  - Then, on the ground (ctrl+0x314 bit 6), with the stick bit set and variant 0: ctrl+0xa = ctrl+6 (the stick steers the attack). It then calls 0x1fa7e0, which runs the mover 0x20cd30 with the stick bit cleared.
  - Inside the mover, 0x2c9770 runs 0x1f3f1c: the facing turns toward ctrl+0xa by sign x min(ctrl+0xa4 or 0x1900, ceil(0.3 |d|)), the locomotion rule. The animation root step is rotated by ctrl+0xa while ctrl+0xa differs from ctrl+8.
- AnimationFinished 0x3204dc:
  - With the window bit set it calls 0x1fdbd4 once more.
  - Otherwise it requests 0x1b0 for variant 11, 5 in the air, 2 with the stick held and clip 0x7d / 0x7e / 0x91, else 1.
- Animation events 0x319ffc (event record: word 0 type, float +0x10 value):
  - Type 7: a value above 0.5 sets record +0x10 bit 3, a value below -0.5 clears it.
  - Type 3 above 0.5 is the strike. For variants 0 and 3:
    - Without a target it searches again (range ctrl+0x80, flags 0xb). With a target, 0x21a2d4(target, player, 0xc) must still pass.
    - A blocked line of sight (0x219990, ray 0x21ab20 between the box centres) clears the target.
    - Then the hit test 0x2a39a8 -> 0x1f7f28(ctrl+0x80, *0x372e44, 0, player, target). When it passes, message 0 goes to the target with the record {0, player, 0, damage, 0, kind, 0}. Damage is 1, or 99 when byte +0x12e of the record ctrl+0xe0 has bit 3 (read 0x04 in the oracle). Kind is 1 for variants 0 and 3, else 0.
    - A target whose +0xc bit 4 is set afterwards is dropped.
  - Variants 1 / 2 send message 62 with a projectile (0x17d9ac); variant 11 sends message 0 with damage 1, kind 3 to an object found by 0x2a39e0. Neither applies to the punch.
- Hit test 0x1f7f28(range, maxAngle, minAngle, attacker, target):
  - The box centres A and T go through their node matrices. Both vertical gaps (T bottom - A top and A bottom - T top) must compare above 0xbe4ccccc as unsigned words, that is be at or below -0.2.
  - |A - T| minus the target's radius must be below the range.
  - The direction from the attacker's node to the target's node, with y = 0 and normalised, must lie within [min, max] x pi / 180 of the attacker's forward row (acos 0x2584c8).

### Timing in the oracle

- atap (one tap at the table):
  - In this run Y is held in row 8 (two more traced updates before the press). State 0xa3 with ctrl+0x2bc = 122 in rows 9..22; state 1 with clip 108 from row 23.
  - Facing ctrl+8 per update: -936 before, then -375, 18, 293, 486, 621, 715, 781, 827, 860, 883, 899, 910, 918, 923; target heading ctrl+0xa = 934 from the entry.
  - Positions move 0.0755 per update along 934, but a contact (ctrl+0x21c / 0x220 = 0.02272 / -0.06486, slide against the table's entity) turns the move sideways in most updates.
- aaway (no target, flat ground):
  - No move in the entry update, then 13 steps of 0.07548 along heading 32451; state 0xa3 for 14 updates.
  - The facing goes 32460 -> 32455 in the entry update: the idle mover turned it once (to 32457) before the tap was handled, then the attack state's mover turned it again. Then 32453, 32452, 32451.
- Hits (traps ataphit, acombohit): message 62 in the entering update (lr 0x3082c4); message 0 from lr 0x31a38c in update 5 of combo_1, update 5 of combo_2 and update 4 of combo_3. The record of message 0 was {0, 0x085526f0 (player), 0, 1, 0, 1, 0}; the words after it are stack contents. Message 45 reaches the table every update of the attack from lr 0x21a93c, which is in the wall slide 0x21a73c: it is the contact message of the mover, not part of the attack.
- Clip events: the tracks of the combo clips (sub-animations of `combo_*_minifig.bfnanm`) include b684fd50 (one key of value 1: combo_1 at frame 5.0, combo_2 at 4.0018, combo_3 at 3.0055) and c99766ba (key of value 1 at 7.017, 6.997, 5.009, staying at 1). The hit updates 5, 5, 4 are the first updates whose clip frame reaches the b684fd50 key (frame k at the end of update k). The threshold of a smooth evaluation would give 5, 4, 3. So the hit is event type 3 at that key (inferred, PLAYER-S6).
  - The combo window opens when the c99766ba key is passed (update 8, 7, 6).
  - Combos measured: acombo10 taps released in rows 7, 16, 25 give 0xa3 / 122 (rows 7..16), 0xa4 / 123 (17..25), 0xa3 / 124 (26..34), state 1 from row 35. acombo3: a second tap in row 9 (update 3 of combo_1) is ignored, state 1 from row 21.
  - A tap in update n of an attack state starts the next state in update n + 1; from locomotion the tap acts in its own update.
- Root step: the sub-animation track 0xdcd633e2 of combo_1 rises linearly to 1.05684 over the clip (handles on the line): 1.05684 / 14 = 0.075489, the measured step (0.07548). combo_2 0.90586 / 12 and combo_3 0.67939 / 9 give the same step. The joint curves of these clips start with header 0xfa (64 bytes per frame) and are not decoded (`decodeChannels` reports `rawMatrix`); they are not plain float matrices. dcd633e0..e2 are taken as the root translation x, y, z (inferred, PLAYER-S7).

### Port

- `src/runtime/player_attack.*`: `AttackTap` (0x1bb690), `makeAttackClip` (key frames of the two tracks, root track per update), `attackScore`, `findAttackTarget`, `attackHitTest`, `attackTargetHeading`, `PlayerAttack` (states 0xa3 / 0xa4, combo index, window, messages 62 and 0), `attackTargetObjects` (wobbly props); `playerRootStepUpdate` in `player_motion.*` moves by the root step with the wall slide.
- Glue `src/game_attack.cpp`:
  - The tap is read from Y (slot 9). An idle, walk, run or sprint role on the ground, with nothing carried and outside detective mode, allows the attack. It is entered after the locomotion step of the same update.
  - The messages of each update go to `LevelSession::attackMessages` (instance, message) for the prop reactions, and are logged as `attack: hit <instance> (message 0, damage 1) at update N`.
- Tests: `player-attack`, `player-attack-real-data` (oracle values above), `game-script-attack-table`.

Unknowns:
- The table from clip track hashes to event types (0x319ffc gets typed events), and the writers of ctrl+0x80, *0x372e44 and the combo length nibble.
- The line-of-sight test 0x219990 (not ported: always clear), the near list 0x23d4c0, the other candidate types and characters (grab, arrest, the team test 0x219ba0), the weight of characters.
- Whether the attack states read the stick bit of the current or the previous update (the port uses the current one), and which other callers of 0x20b984 (0x25f280, 0x2a4f18, 0x1d6fa0) allow the attack.
- The jump attack (0x1d5254 tests ctrl+0xe bit 0x20 at 0x1d5294, 0x214f40 from 0x1d5308), the weapon-out state 0x9a and its counterpart 0xc, the damage 99 flag, and the sounds of the attack (melee entry +0xa / +0xc).
- Why the port's entity box of the mesh-kind wobbly props is 0.009 higher than the oracle's.

<a id="ts-20261019-080314"></a>

## 2026-10-19T08:03:14+02:00 — The root step of the combo clips is the extra joint curve (llvm-objdump of 0x270778 and 0x2327b4, probes over combo_1..3)

[Corrects player-attack.md §2026-10-18T19:59:57](#ts-20261018-195957): its root step came from the sub-animation track 0xdcd633e2 (inferred, PLAYER-S7).

- Data (probe `assets::rootMotion` and `decodeChannels` over `models/char_minifig/class_generic/combo_{1,2,3}_minifig.bfnanm`): the joint table words 0x1007a015, 0x1006a015 and 0x10052015 have bit 28 set and name curve 21, the last of 22, as the extra (root motion) curve. Its translation is (0, 0, z) with z = 0.07549 x frame (rows of the 0xfa matrices decomposed by 0x1324f8, see [skeleton-animation.md §2026-10-18T21:40:55+02:00](skeleton-animation.md#ts-20261018-214055)): 1.05684 at frame 14 of combo_1, 0.90586 at frame 12 of combo_2, 0.67939 at frame 9 of combo_3. The sub-animation track 0xdcd633e2 has the same end values and the same slope; dcd633e0 and dcd633e1 stay 0.
- Code: the root delta of 0x2c9770 comes from 0x2327b4, which samples the root block of each playing slot with 0x270778. 0x270778 reads the animation's joint table word: with bit 28 it indexes the joint curve array with word & 0x1fff (else the root joint's curve), decodes the curve at the frame (0x287124, 0x24a0c8) into a pose entry and, for the extra curve, builds a matrix from it (0x247fb0, 0x2588f4) and stores three floats of it to the caller's vector (0x270830). What exactly those three floats are (the translation row) was not traced. The sub-animation tracks (event tracks with hashes) are not read there. So the curve is the source, as for the jumps ([player-locomotion.md §2026-10-13T17:02:03](player-locomotion.md#ts-20261013-170203)).
- Port: `makeAttackClip` samples `assets::rootMotion` (z of the extra curve) at the frame k x framesPerUpdate of every update; the step per update stays 0.07549 (oracle 0.07548). The stand-in `kAttackRootTrackStandIn` is removed; PLAYER-S7 is replaced by PLAYER-S10, the interpolation between frames and the unused x and y axes. Test `player-attack-real-data` (end values and first step).

Unknowns:
- That 0x270778 runs for the punch slot (not trapped in the oracle; read from the code and the equal values only).
- The interpolation between two frames of the curve and the weighting of the slot (the combo clips run one frame per update at weight 1 in the measured steps).
- x and y of the delta are zero in these clips; the port keeps the z axis only.

<a id="ts-20261019-125452"></a>

## 2026-10-19T12:54:52+02:00 — The sounds of the attack: swing in the enter, hit in the hit event, both from the melee weapon row, which has id 0 for every character (llvm-objdump of 0x307e7c, 0x319ffc, 0x23ae84, 0x23fa54; tablesweapons and tablescharacters dumps)

[Corrects player-attack.md §2026-10-18T19:59:57](#ts-20261018-195957): "sound table entry +0xc of the melee entry" and "melee table 0x23ae84" were loose; 0x23ae84 is the play routine, and the table is the weapon table at *0x376050.

### Method

- llvm-objdump (`--triple=armv6k --mattr=+vfp2`) of `artifacts/game/game.arm.elf`: 0x307e7c..0x3084cc (enter of states 0xa3 / 0xa4), 0x319ffc..0x31a3d8 (animation event handler, hit event), 0x23ae84, 0x23fa54, the pool words 0x3084cc / 0x3084d0 / 0x3084d4 and 0x31a3cc. No REA session was used for this entry.
- Data: `build/chase-tabledump tables/tablesweapons.bxls` and `tables/tablescharacters.bxls`, offsets from the column types (type 8 = 8 bits per stored byte), the same rule as docs/re/save-format.md §2026-10-13T09:25.

### Tables

- Character table `*0x372e10` (tablescharacters sheet 0, stride 0x28), indexed by the byte ctrl+0x2cc of the character (row 9 `police` for Chase): byte +0x17 (column 71) is the weapon row of both attack sounds. The weapon flag test of the enter (0x307ff4..0x308064) picks byte +0x18 for variants 0 and 3, +0x17 for 1 and 4 and +0x19 otherwise. Chase: +0x17 = 13, +0x18 = 8, +0x19 = 0.
- Weapon table `*0x376050` (tablesweapons sheet 0, 17 rows, stride 24; the pool word sits at 0x3084d0 and 0x31a3cc): +0 string, then u16 at +4 (flags; bit 3 is tested at 0x308040), +6, +8, +0xa, +0xc, +0xe, +0x10, +0x12, +0x14, +0x16 (columns 2..11).
- Rows used by the 192 character rows at +0x17: 13 (174 rows, including Chase), 15 pickaxe (14), 16 (2), 0 (2). Their +8, +0xa and +0xc are all 0. Non-zero values in those columns exist only in the gadget rows (row 4 hazardcannon +0xa = 501, row 8 grapplegun +0xa = 501 and +0x10 / +0x12 = 517 / 518, row 7 energygun +0xa = 513, row 9 chickengun +0xa = 162).

### Where the sounds play

- Swing: the enter 0x307e7c, after the notice message 62 (or without it) at 0x3082c4..0x3082f4: `0x23ae84(ldrh [weapon row of byte +0x17] +0xc, entity)`, before the clip starts (0x3082f8). It runs for every entered state (0xa3 and 0xa4), also without a target. The branch at 0x3080e8 (variants 1, 2 and the 0x2106d8 path) uses byte +0x18 and the +8 column; it is not reached by Chase's variants 0 and 3 (jump table at 0x307fb0: variants 0 and 3 go to 0x308180).
- Hit: the animation event handler 0x319ffc, event type 3 (value above 0.5), after the candidate passes 0x2a39a8 (the hit test) and the message record is built: `0x2b80f4(entity, ctrl)`, then `0x23ae84(ldrh [weapon row of byte +0x17] +0xa, player)` at 0x31a378, then message 0 to the target through 0x244fc8. No sound plays when no target is hit.
- Call: 0x23ae84(id, object) = `0x23fb78(object, &pos)` (the node position) and `0x23fa54(id, &pos, u16 object+0x14, 0)`; positional, no volume or pitch arguments (the sound record gives them). 0x23fa54 masks the id with the negated byte at 0x376ce4 (`ands r0, -byte, id`) and returns at once when the result is 0, so id 0 plays nothing.

### Result

Chase's attack plays no sound through these hooks: the swing and the hit id are 0 in his weapon row, and in the row of every character of the table. The audible punch of the original, if any, does not come from them. No audio recording of a punch was compared.

Port: `src/runtime/attack_sounds.*` reads the two ids from the tables (`assets::characterMeleeWeapon`, `assets::weaponSounds`) and turns an attack step into sound calls (swing when a state was entered, hit for each hit message, at the player's position). Test `attack-sounds` / `attack-sounds-real-data`.

Unknowns:
- What 0x2b80f4 (called in the hit event before the sound) does: its callees 0x2554c0, 0x250810, 0x254250, 0x227790, 0x2408c4, 0x25b2b8 and 0x22a9ec were not read (a hit effect is likely).
- The u16 at entity+0x14 (owner of the voice); the port uses 0 (PLAYER-S11).
- Whether the combo clips carry sound events (the clip tracks other than the hit and combo-window keys were not listed), and whether the original sounds anything on a punch.
