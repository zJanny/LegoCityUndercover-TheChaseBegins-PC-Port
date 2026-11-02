# The attack on characters

Subtopic of [player-attack.md](player-attack.md): which characters the player's attack picks, the grab test and the fight move a Y tap starts on a goon.

## Current state

- Characters: the gate 0x21a2d4 keeps the characters of the other side (entity +0xc bit 2 = `AIBadGuy`) whose target mode ctrl+0xda & 7 allows the player; a bad guy weighs 0.4 and is searched all around (180 degrees). [§2026-11-01T15:19:31](#ts-20261101-151931)
- A Y tap first searches 3.0 / 45 degrees for a character to grab (0x28265c); a goon found there gets a fight move (0x1d3980: variant 4, a random `grab_putdown*` clip of `tables/tablescombat.bxls` sheet 1, target state 0xe3), or the grab struggle 0x19bfc0 for abilities 0x32 / 0x35. The punch's melee search drops characters for Chase's weapon row 13. Not measured yet. [§2026-11-01T15:19:31](#ts-20261101-151931)
- Measured (20261101-mission-D-1, -2): a tap next to two goons takes the nearer one (1.0 m) in every stick direction into the fight move (state 163 with variant 4, move clip `grab_putdown_04` or `_07`, goon state 227 one update later); no message 62 or 0 is sent; *0x372e28 equals `tables/tablescombat.bxls` sheet 1. [§2026-11-01T16:02:02](#ts-20261101-160202)
- Measured (20261102-mission14-servant): a tap with two goons lying knocked down (state 309) and Chase idle is a plain punch in the air: the search gate passes the lying goons, no fight move starts and no message 0 or 62 goes to them. [§2026-11-02T15:50:34](#ts-20261102-155034)
- Port: `src/runtime/attack_targets.*`, `choosePlayerAttack` in `src/runtime/player_attack.*`, glue `src/runtime/game_attack_characters_glue.cpp`. [§2026-11-01T15:19:31](#ts-20261101-151931)
- The line of sight 0x219990 is the ray 0x21ab20 between the box centres with the query words include 5 / exclude 0x10, clear when the nearest entry is the target's; a character's box is its class's `CollisionBound` (the player's box (0, 1.15, 0.08) / (0.6, 0.85, 0.52) in every level character class that has the record); the player's near list is the mask 4 / 6 entries in a box of half 5.0 around his node. Port: `src/runtime/attack_facts.*`, glue `src/runtime/game_attack_facts_glue.cpp`. [§2026-11-02T15:32:44](#ts-20261102-153244)

<a id="ts-20261101-151931"></a>

## 2026-11-01T15:19:31+01:00 — Characters as attack targets: the entity gate, target modes, weights, and the Y tap that grabs a goon into a fight move (REA with Ghidra, llvm-objdump, table and level data probes)

### Method

- REA (Ghidra 12.1.2) pseudocode, 20261101-mission-D: 0x21a2d4, 0x219ba0, 0x1b7358, 0x219990, 0x219c08, 0x245b00, 0x23de70, 0x22103c, 0x21aae0, 0x2108c0, 0x219b80, 0x21a1d0, 0x1fdbd4, 0x236b6c, 0x28265c, 0x23d610, 0x1d3980, 0x19bfc0, 0x23a70c, 0x21e798, 0x231d20, 0x319ffc, 0x219a8c, 0x23ad70, 0x2ce640, 0x23d4c0, 0x18d15c, 0x235498, 0x1d4dfc, 0x23a87c, 0x2a39a8, 0x206888, 0x210e38.
- llvm-objdump: 0x21a2d4..0x21a420, 0x182e40..0x182f30 (the character create), 0x1d3980..0x1d3a28, 0x31a210..0x31a2b0 (the arguments of the search at the strike).
- Data: `chase-tabledump` of `tables/tablescombat.bxls` and `tables/tablesanimations.bxls`, and a probe with the C++ loaders (sheet layouts, the lvl_01 characters of mission 1-4 with their parameters and table rows).
- No oracle measurement in this entry (requests for the fight states of mission 1-4 are open, PLAYER-26).

### Entity gate 0x21a2d4(candidate, attacker, flags)

- Null, the attacker itself, entity +0x10 bit 0, or no +0x10 bit 0x200 (collision): dropped. Flag 8: dropped when 0x23de70 is true (node or its parent hidden).
- Characters (0x245b00: kind +0x12 27 or 149, a kind of the list 0x3743ec, or +0xc bit 14):
  - Dropped in the states ctrl+0x6c 0xb, 0x52, 0x143, 0x144, and in state 0x13 with flag 0x20.
  - Sides: entity +0xc bit 2. With flag 4 a character on the attacker's side is dropped unless the attacker is the player (*0x3772c0). Without flag 4, flag 1 drops a character on the same side and flag 2 one on the other side.
  - Then 0x219ba0(candidate, attacker, 1) on ctrl+0xda & 7: 0 anyone, 2 only ctrl+0xc4, 3 only the player, 4 nobody (it returns the argument xor 1), 1, 5, 6 and 7 nobody. EnableTargetable writes bits 0 and 3 of this byte ([script-natives-flags.md](script-natives-flags.md)), so a character switched off is in mode 1.
- Kinds 0x43 and 0xcc (0x22103c): 0x23cff4(*0x3775e0, candidate), else the record +0xde bit 0 and +0xcc in {1, 7, 11}. Not read further.
- Other kinds: 100, 155, 85, 119 and 214 pass, 19 with entity +0x90, everything else (158 explicitly) fails.
- Flag 0x40 compares heights through 0x22e1cc (not read). The player's Y tap passes flags 9. In the strike 0x319ffc the check of a held target always passes 0xc (0x31a0b0); the search without a target passes 0xb only for the player's variants 0 and 3 with a weapon row other than 16, otherwise 0xc (0x31a224..0x31a258).
- Entity +0xc bit 2 is `AIBadGuy`: the character create (0x182ef4..0x182f14) ors 4 into +0xc when the class integer is not 0. SwapAllegiance toggles it ([script-natives-party.md](script-natives-party.md)).

### Search 0x219c08 and weight 0x1b7358

- Arguments: s0 range, s1 angle, r0 attacker, r1 "all around", r2 / r3 an exclusion list, then on the stack "characters only", the 0x21a2d4 flags, the weight function and "prefer characters". The Y tap calls (range, angle, player, 1, 0, 0, 0, 9, 0x1b7358, 0); the strike calls (ctrl+0x80, *0x372e44, player, 6, 0, 0, 0, 0xb or 0xc, 0x1b7358, 0).
- The angle is 180 degrees (0x43340000) for a candidate with +0xc bit 2 when "all around" is set: a bad guy is found in any direction.
- Weight 0x1b7358: 1.0 for a hidden node, 0.3 for +0xc bit 14, 0.25 for the player, 0.4 for kind 100, 1.0 for a non-character, 1.0 for a character in state 0x13, with +0xc bit 4 or not a bad guy, otherwise 0.4.
- With "prefer characters" a character with any finite score replaces a non-character target; no call of the attack sets it.
- Near list 0x23d4c0: for the player and a range up to *0x377538 it returns the list 0x593284 with the count *0x377534 (built elsewhere, not read); for other entities a sphere query 0x237f30 into 0x58dee4 (at most 50), cached by entity and range.

### The Y tap of 0x1fdbd4 for the player

1. Grab search (*0x372e50 = 3.0, *0x372e4c = 45 degrees, flags 9). A character target is kept when 0x28265c(target, player) passes, or when it is a boss (+0xc bit 14); then 0x23d610 refuses the target states 0x122..0x12f, 0x134, 0x135, 0x139..0x13e, 0x143 and 0x144, which clears it and leaves the punch without a target (no melee search).
2. Otherwise the melee search (ctrl+0x80, 1.5 x for the running attack, *0x372e44 = 90 degrees, flags 9). A character found there is dropped unless the player's weapon row (character table byte +0x17) is 16. Chase's row is 13, so the punch never targets a character.
3. A Y tap with a grab target (0x219b80 false): 0x19bfc0 when the target has ability 0x32 or 0x35 (0x23a70c) or is a boss: player state 0xfe, the pair object 0x231d20(0.02, 1.77, player, target, 1) with +0x478 = -1, target state 0xff. Otherwise 0x1d3980: the fight move. The special target *(0x3746f4 + 8) (0x219b80) goes through 0x2ce640 to 0x25d8e4 or 0x1d3abc instead.
- 0x28265c(target, player): the target is a character, 0x219ba0 passes, 0x23ad70(target ctrl, 0) is 0 (character table row byte +0x1c bit 0, or state 0xdb with ability 0x2b), both have ctrl+0x314 bit 6 (on the ground), the ctrl+0x240 values differ by at most 0.5 (float bits compared below 0x3f000001), the target has no +0xc bit 14, none of the six engaged entities of 0x58deb0 has the target as ctrl+0x140 with next state ctrl+0x6e 0x132, the player has any of the eight fight move abilities (0x219a8c on the player's set), and the line of sight 0x219990(player, target) is clear.
- Abilities: 0x206888 copies the bytes +0xf..+0x16 of the character table row (`AbilityTable`, [use-spot.md](use-spot.md)); 0x1d4dfc then masks the set for a party member, clears ability 5 in state 0xda, sets ability 7 in state 0xdb with ability 43, sets ability 52 for a held object of kind 248 (0x1c6660) and ability 40 for the player while 0x2a5dcc holds (none of these is modelled).

### Fight move 0x1d3980(player, target)

- ctrl+0x140 = target, 0x214f40(player, ctrl, 4, 0): state 0xa3 (or 0xa4) with variant 4.
- Move choice: the moves i = 0..7 other than the last one (combat record byte +0xb, 0xff before the first) whose ability byte +0xc the player has (0x23a70c); 0x235498(count) = next random word (0x25b2b8) modulo count picks one. With none, the last move stays when the player still has its ability, else +0xb = 0xff (the table is then read at index -1).
- 0x231d20(move +0, move +4, player, target, 1) builds the pair object (headings ctrl+8 of both, the direction between the nodes, the height gaps), stored at +0x14 of both combat records with +0x478 = the move; then the target is asked for state 0xe3.
- The clip (0x18d15c variant 4) is the u16 +8 of the move. The table `*0x372e28` is `tables/tablescombat.bxls` sheet 1 (stride 20, as 0x1d3980 indexes it; not yet confirmed by a memory read): rows 0..7 hold the floats (-0.06, 1.58), (-0.06, 1.58), (0.12, 1.58), (0.09, 1.45), three times (-0.06, 1.58), (-0.03, 1.58), the clips 261, 437, 453, 467, 469, 471, 473, 475 (`grab_putdown`, `grab_putdown_02` .. `_08`, class_generic) and the ability 46 in byte +0xc of every row (byte +0xb holds 45..52).
- `grab_putdown_minifig.bfnanm` (19 frames at 15 per second) has no key of track 0xb684fd50, the track the punch's hit comes from (PLAYER-S6). `grabbed_putdown` (clip 262, 33 frames) is the matching clip of the grabbed character.
- In the strike 0x319ffc a variant other than 0, 3, 1, 2 and 11 still sends message 0 at an event of type 3 when the hit test passes, with kind 0 (kind 1 only for the variants 0 and 3).

### Mission 1-4

- lvl_01 has 15 `1-4_AI_Goon_*` characters (classes `chars/char_ai_cherrytreehillsgang1..4`), all with `AIBadGuy` 1, `AIFriendly` 0 and `CharacterEnum` 142. Row 142 has the ability bytes 31 4e 02 00 01 00 40 00 (no 0x32, no 0x35) and row byte +0x1c = 0x60 (bit 0 clear). `1-4_AI_Goon_01` stands at (552.138, 38.946, 980.847).
- Chase (row 9) has the ability bytes 77 ce 1a 02 03 c0 03 20: ability 46 is set, so all eight fight moves are his; weapon row 13.
- So by the code a Y tap with a goon within 3.0 (in any direction) starts a fight move on it (`grab_putdown*`), the goon is asked for state 0xe3, and the punch with its message 0 is not used on goons.

### Port

- `src/runtime/attack_targets.*`: `attackCandidatePasses` (0x21a2d4), `targetModeAllows` (0x219ba0), `isAttackCharacter` (0x245b00), `attackWeight` (0x1b7358), `attackSearchAngle`, `grabStateAllows` (0x23d610), `fightRefused` (0x23ad70), `fightTargetPasses` (0x28265c), `fightMoves`, `chooseFightMove` (0x1d3980), and the sink types `CharacterHit` (message 0) and `CharacterFightStart` (state 0xe3 or 0xff).
- `src/runtime/player_attack.*`: `findAttackTarget` applies the gate, the weight and the angle per candidate; `choosePlayerAttack` (0x1fdbd4); the fight move enters the attack state with variant 4 and the move's clip; the strike searches with flags 0xb for the punch (0xc otherwise and for the held target) and sends kind 0 for variant 4.
- Glue `src/runtime/game_attack_characters_glue.cpp`: the level characters (kind 27, `AIBadGuy`, `Boss_IsBoss`, the abilities and flags of their `CharacterEnum` row, EnableTargetable) become candidates; the sinks log `attack: fight move N on character NAME (state 227)` and `attack: hit character NAME (...)` until the character side is connected.
- Tests: `attack-targets`, `attack-targets-real-data`.

Unknowns:
- Everything measured: which goon the tap picks in the fight states, the messages the goon gets and when, whether its position changes (PLAYER-26).
- The line of sight 0x219990: the ray 0x21ab20 between the box centres with the descriptor 0x362298 and the words 5, 0x10, 0x1800, 0x200, ignoring the player; blocked when the first hit is not the target.
- The player's list 0x593284 of 0x23d4c0, the characters' entity boxes (+0x60 / +0x6c), ctrl+0x240, the initial ctrl+0xda of level characters, the engaged list 0x58deb0, and the height test of flag 0x40.
- The variant 4 enter and update (0x307e7c, 0x3085dc: heading, message 62, motion), the pair object 0x231d20 and how it moves the two characters, the struggle states 0xfe / 0xff, 0x25d8e4 and 0x1d3abc, and the target's state 0xe3.

<a id="ts-20261101-160202"></a>

## 2026-11-01T16:02:02+01:00 — The Y tap next to a goon in the oracle: the fight move on the nearest goon, no message 62 or 0 (Azahar RPC, 20261101-mission-D-1 to -3)

Measures what [player-attack.md §2026-11-01T15:19:31](#ts-20261101-151931) read from the code; nothing there is contradicted.

### Method

- Emulator servant, New 3DS, OpenGL, RPC, frame-stepped with dt fixed at 1/30 after the load. Recorded from save states of mission 1-4 (the first fight phase in the showroom hall: a goon in front of Chase, two hearts; and a moment later, a goon just knocked down). Update u counts the fixed steps after the load. Each load ran 3 idle updates first.
- Traps: 0x244fc8 (messages; id 0x80000001 left out), 0x1d3980, 0x19bfc0, 0x214f40, 0x25d8e4, 0x231d20, 0x21a2d4 (entry and return value). Memory reads of the player, the goons and the tables.
- D-1: Y held for one update (u1), no stick, 22 updates. D-2: the same with the stick held fully left, down or right the whole time. D-3: the second state, Y tap at u1.

### Before the tap (D-1)

- Chase: state 1, facing ctrl+8 = ctrl+0xa = -5232, position (577.761, 27.6005, 967.0345).
- ctrl+0x140 already holds goon A in the idle state, before any tap.
- Goon A: kind 27, +0x10 = 0x1240, +0xc = 0x00020004 (bit 2 set: `AIBadGuy`), ctrl+0x6c 1, ctrl+0xda 0, ctrl+0x2bc 109, its ctrl+0x140 the player, position (577.19, 27.566, 968.139), 1.0 m from Chase.
- Goon B: the same flags, ctrl+0xda 0, state 111, clip 204, position (575.258, 27.566, 967.710), 2.6 m away, ctrl+0x140 0.
- Other characters within 40 m at that height: two in state 0 at (555.63, 27.77, 980.57) and (589.75, 27.57, 941.39), one in state 80 with ctrl+0xda 9 at (558.80, 27.77, 968.82).
- Tables: *0x372e28 points to 8 entries of 0x14 bytes, byte for byte the rows 0..7 of `tables/tablescombat.bxls` sheet 1: the floats (-0.06, 1.58) .. (-0.03, 1.58), the u16 clips 261, 437, 453, 467, 469, 471, 473, 475 at +8, bytes +0xa = 0, 7..13, +0xb = 45..52, +0xc = 46 in every entry, +0x10 = 254 (255 in entry 7). The count at 0x3743ec is 1 and the list 0x515a94 holds kind 159.

### The tap

- u1: Chase is still in state 1 (state 2, walking, with the stick). In u2, 0x21a2d4 runs once per character near the player (15 calls, flags 9, lr 0x219d88: the near-list path of 0x219c08); 3 calls return 1.
- u2: 0x1d3980(player, goon A) from lr 0x1fe5d8, 0x214f40(player, ctrl, 4) from lr 0x1d39b4, 0x231d20(player, goon A, 1) from lr 0x1d3a50. Chase enters state 163 (0xa3). The combat record bytes +8..+0x14 go from `00 00 ff ff ff ff 00 40 08 00 00 00 00` to `04 04 ff 03 ff ff 00 40 00 00 00 00 00`: variant bytes +8 / +9 = 4, move byte +0xb = 3, and clip 467 (`grab_putdown_04`) plays.
- u3 to u21: goon A is in state 227 (0xe3) with clip 468 (`grabbed_putdown_04`). Its +0xc gains bit 21 (0x00220004) and its ctrl+0xda becomes 1 (mode 1: no longer a target). Goon B stays in state 111.
- No message 0 and no message 62 goes anywhere in 22 updates. Message 39 goes to goon A in every update from u1 on (lr 0x1d6328), and message 36 to the player from u3 on (lr 0x20b6b0).
- Goon A stays at its place until u12, then moves toward Chase: u13 (577.21, 968.09), u16 (577.34, 967.79), u20 (577.69, 967.07), u21 (577.79, 966.87). Chase stays in state 163 with clip 467 to u21 (the clip runs 36 updates at 15 frames per second).
- 0x19bfc0 and 0x25d8e4 are never called.
- Screens: before the tap, Chase stands 1 m from a goon in a red and yellow suit, and the lower screen hint reads "Enemies are displayed as red icons on your map." About 10 updates after the tap both stand close together with an X button prompt above them, and the hint reads "Press X to counter attacks."

### Stick directions (D-2)

- Left: in u1 Chase walks by (0, -0.2275) (heading -32768); in u2 the fight move takes goon A with clip 473.
- Down: in u1 he walks 0.23 m away from goon A; in u2 the fight move takes goon A with clip 473.
- Right: in u1 he walks by (0.076, 0.0805); in u2 the fight move takes goon A with clip 467.
- So goon A (1.0 m) is the target in all four cases and goon B (2.6 m) never is. This matches the all-around search of bad guys with the score of 0x21a1d8.

### The second state (D-3)

- At the load Chase is already in state 163 (clip 487, ctrl+0x140 = goon B), holding goon B (state 230, clip 488, +0xc 0x00220004, ctrl+0xda 1), which rises and moves along +x.
- Goon A lies 5.7 m away (state 309, clip 306, +0xc 0x00220004).
- The tap changes nothing in 16 updates: no 0x21a2d4 call, no 0x1d3980. Message 38 goes to goon B in u3 to u8 (lr 0x20b724).
- A tap on a downed goon needs the throw waited out first.

### Port

- The enter of the fight move no longer sends message 62 (it stays for the punch). Test `attack-targets` (`oracleFightStates`) pins the positions, flags and states of D-1 with the four stick headings: the fight move takes goon A, with no message 0 or 62 over 21 updates. `game-script-attack-goon` fails on a message 62 to the goon.
- The move index matches the original only by chance: the random generator is a stand-in (PLAYER-S14). The goon's motion, its +0xc bit 21 and its mode 1 belong to the character side (state 0xe3).

Unknowns:
- A tap with a goon knocked down and Chase idle, which needs the second state waited out (about 150 updates).
- What messages 39 (lr 0x1d6328, before and during the fight), 36 (to the player) and 38 do, and who keeps ctrl+0x140 set in the idle state.
- Which 3 of the 15 near-list candidates passed 0x21a2d4.

<a id="ts-20261102-153244"></a>

## 2026-11-02T15:32:44+01:00 — The attack's line of sight, the player's near list and the box of a character (llvm-objdump; no emulator; 20261102-mission14-D)

Reads what [§2026-11-01T15:19:31](#ts-20261101-151931) listed under its unknowns. Nothing there is contradicted.

### Method

- llvm-objdump (`--triple=armv6k-none-eabi --mattr=+vfp2`) of `artifacts/game/game.arm.elf`: 0x219990, 0x21ab20, 0x27cab4, 0x237f30, 0x23d4c0, 0x219c08 (the loop over the near list), 0x2e5330..0x2e5418 and 0x2f6af4..0x2f6b54 (the gathers of the player's list), 0x28265c, 0x23aff8, 0x236d74, 0x242df4, 0x254250. REA was not used, so no REA result is claimed here.
- A probe with the C++ loaders over the class files of the characters placed in lvl_01 (`Level::bounds`).

### Line of sight 0x219990(attacker, target)

- Both entities' box centres (entity +0x60) go through the node matrix (0x250810 of entity +0x3c, the point transform 0x254250) and become the ray from the attacker's centre to the target's centre.
- The query descriptor is built on the stack (0x2199f4..0x219a38): vtable 0x362298, include mask 5, exclude mask 0x10, kinds -1 / 0, owner +0xc include 0 and exclude 0x1800, owner +0x10 include 0x200 and exclude 3, the excluded owner = the attacker, mode byte 1, and no excluded surface (words 0). Masks 1 (static level), 4 (characters) and 6 (dynamic props) pass; 2, 16 and 17 (vehicle boxes) do not.
- 0x21ab20 gathers up to 100 entries (0x237f30) in the box around the segment (centre = midpoint, half extents = half the absolute difference), then 0x27cab4 keeps the nearest hit and stores the hit entry in the out record. Box entries (type 0) are cut in the box's own space by 0x2040e8 (owners with flag 0x800 loop over sub-boxes of the model's records), mesh entries (type 1) by 0x22f978; the entry matrix is the owner's node matrix (0x21bb78).
- Result: no hit gives 1; with a hit, 1 when the entry's owner (entry +0x10) is the target and 0 otherwise. So a wall, a prop or another character between the two centres blocks the grab; the attacker itself is never hit.

### The box of a character

- The character create 0x182e40 calls 0x236d74, which calls `0x23aff8(entity, 0, 0, 0, autoBox 0, margin 0)` (0x236f28): with the class's `CollisionBound` record the box is its centre (+0x10) and half extents (+0x1c) in entity +0x60 / +0x6c, and owner flag +0x10 bit 9 (0x200) is set; the parameter `BoundCollision` 0 clears it again. The dynamic argument is 0, so the grid entry has mask 4 (no flag 0x100), and the last argument 0 ors 0x20000 into entity +0xc (the oracle's goons read +0xc = 0x00020004).
- Probe: the character classes of lvl_01 that carry `CollisionBound` (`chars/char_ai_*`, `char_journalist`, `char_boosterstrange`, `char_racecardriver`, the classes of the 15 goons of mission 1-4 among them) all have centre (0, 1.15, 0.08) and half extents (0.6, 0.85, 0.52): the player's box that the port used for every character. `chars/pedestrian` has no record, so that entity gets no box and no flag 0x200 (the gate 0x21a2d4 then drops it).

### The player's near list

- Per update (inside the frame function, 0x2e5330..0x2e5418) the player's list is rebuilt: 0x237f30 over the box with the centre at the player's node position (translation of 0x250810) and half extents *0x377538 (5.0) on all axes, the descriptor words include 4, exclude 0, kinds -1 / 0, owner +0xc 0 / 0, owner +0x10 include 0x200 and exclude 3, no excluded owner, mode 0, at most 100 entries, into the list 0x593284 with the count *0x377534. The level start builds it once the same way (0x2f6af4..0x2f6b54, with owner +0xc exclude 0x10).
- Include mask 4 takes the entries of masks 4 and 6: characters and dynamic props, not static level (1) or vehicle boxes (17). Mode 0 does no per-entry box test, so the list holds every such entry of the grid cells the box touches. 0x237f30 also drops an entry whose flag byte +5 has bit 0 or 1 set (bit 1 is the mark it sets on every entry it adds) and an owner whose node word has bit 5 set. 0x23d4c0 hands this list to 0x219c08 for the player when the range is at most 5.0; for other entities (or a longer range) it gathers its own list with mode 1, owner +0xc exclude 0x210 and the searcher excluded.
- The attack never reaches beyond 3.5 (grab range 3.0, melee range 2.2 x 1.5 = 3.3, plus the target's half width), so an entry in the list's cells but outside the box cannot be picked.

### Not read further

- ctrl+0x240 is the height the grab test compares (named so for the player in [rendering-shadows.md](rendering-shadows.md)). The controller code that writes it is at 0x30a330 (clears the block at +0x23c..+0x248) and 0x30ba10 (adds a delta to +0x240); the port keeps the node height.
- The engaged table: 0x28265c walks the six pointers at 0x58deb0 and refuses the target when an entity other than the target has it as ctrl+0x140 with ctrl+0x6e equal to the pool word 0x132 (0x2827e4). Who fills the table is not read here; [combat-characters.md §2026-11-01T15:23:21](combat-characters.md#ts-20261101-152321) names it for the takedown.

### Port

- `src/runtime/attack_facts.*`: `attackLineClear` (the nearest of the world's entries accepted by mask 5 / exclude 0x10 and the character boxes of the candidates; ties go to the target), `markAttackNearList` (sets `listed`; the search skips the others, so indices stay stable), `characterEntityBox` (the class record and `BoundCollision`). `PlayerAttackTables::lineOfSight` feeds `choosePlayerAttack`; `PlayerAttackTables::engagedOn` is the hook for the engaged table. Glue: `src/runtime/game_attack_facts_glue.cpp` (installs the sight with the level's collision world), `game_attack_characters_glue.cpp` (box and owner flag from the class), `src/game_attack.cpp` (one call to mark the near list).
- Tests: `attack-facts` (hand-calculated walls, character boxes, masks, near list, the blocked grab), `attack-facts-real-data` (the class boxes of lvl_01, and the line from the recorded position of Chase to goon A of the fight state).

Unknowns:
- The `MeshCollision` branch of 0x23aff8 (at 0x23b1c4), which takes the box from the model and replaces the class's `CollisionBound` box: not modelled (stand-in PLAYER-S15); which classes use it is not read.
- The tie rule of 0x27cab4 (equal distances), the meaning of the owner +0xc exclusion 0x1800 of the ray, composite owners (flag 0x800) and which other entities (pedestrians, drivers) the grid holds besides the level characters.
- The writers of the engaged table, of ctrl+0x240 and the start value of ctrl+0xda; the height test of flag 0x40 (0x22e1cc), the kinds 0x43 / 0xcc (0x22103c), the state-dependent ability changes of 0x1d4dfc and the library acosf 0x2584c8.
- Whether the node-bit-5 drop of the gather is the hidden node of 0x23de70.

<a id="ts-20261102-155034"></a>

## 2026-11-02T15:50:34+01:00 — A Y tap while two goons lie knocked down and Chase is idle: a plain punch in the air (Azahar RPC, 20261102-mission14-servant, idle job 1)

Measures the open point of [§2026-11-01T16:02:02](#ts-20261101-160202) (PLAYER-26). Recorded from a save state of the mission 1-4 fight a moment after Chase knocked a goon down; New 3DS, OpenGL, RPC, dt 1/30; two loads agree on every trap line and every sampled state of u1 to u8. Y held for one update at u1, no stick.

- Start: Chase idle (state 1) at (578.704, 27.621, 967.358), facing ctrl+8 = ctrl+0xa = 13556, ctrl+0x140 = 0. Goon B lies in state 309 (clip 308) 2.70 m ahead (about 5 degrees off his facing), goon A lies in state 309 (clip 306) 6.63 m away, a third bad guy stands in state 80 at 19.9 m. Both lying goons have entity +0xc = 0x00220004, ctrl+0xda 0 and ctrl+0x140 = Chase.
- Result: no fight move. In u1 0x214f40(player, ctrl, 0) runs from lr 0x1fe50c (the plain punch); 0x1d3980, 0x19bfc0, 0x25d8e4 and 0x231d20 are never called. Chase is in state 163 at u2, walks forward 0.945 m over u3 to u15 and is back in state 1 at u16 (14 updates, as combo_1).
- The search 0x219c08 in u1 runs 0x21a2d4 15 times with flags 9; 3 return 1: goon B (2.70 m), goon A (6.63 m) and a kind-119 prop. At u5 (the hit window) 0x21a2d4 runs 15 times with flags 0xb and every call returns 0: no target, no message 0, 62 or 46 reaches a goon in u1 to u26. Neither lying goon changes state in 24 updates.
- Reading with the port's rules (not measured): state 309 is 0x135, which the refusal 0x23d610 lists, so the grab target is cleared and the punch has no target; flags 0xb drop characters of both sides for the strike. The port's `choosePlayerAttack` does the same; no test pins this yet.
- Side data (no input, stepped, first load, +-1 update between loads): goon B, held in the throw until u29, lands in state 309 at u30, lies 120 updates, gets up in state 313 (clip 309) at u151 to u181 and is idle (state 1) at u182; goon A lies in 309 until u99, gets up in 313 at u100 to u126 and is idle at u127. Chase dies (state 11) at u236 at one heart if left alone, because a goon attacks.
