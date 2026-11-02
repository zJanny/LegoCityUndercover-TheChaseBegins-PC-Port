# Combat: a character takes a hit

What a level character does with message 0 (the hit): the hit receiver 0x1b8570, the damage 0x15afb4, the reaction message 46, the hit states PUNCHED, KNOCKED DOWN and GET UP, the arrest health and the takedown moves that lower it. The attacker's side (Chase's punch) is in [player-attack.md](player-attack.md), the death of a character (message 1) in [script-natives-world-death.md](script-natives-world-death.md). Items: [combat-characters.md](../plans/features/combat-characters.md). For developers; no game assets.

## Current state

- Message 0 of the character handler 0x18e4c8 goes to the hit receiver 0x1b8570 (0x24255c forwards it to ctrl+0x130 for kinds 284 and 411). A punch (kind 1 with a sender) on any character but the player takes no health: 0x1b8570 jumps past the damage and only sends message 46 to the character itself. [§2026-11-01T15:23:21](#ts-20261101-152321)
- Message 46 (0x18ee2c) requests PUNCHED (190), or PUNCHED REPEAT (191) when the character is in 190, for a kind 1 hit with a sender; KNOCK BACK (188) on the ground or FALL (5) in the air for other hits; 189 or 224 for kinds 4 and 6 with some weapons. None while a request is pending, ctrl+0xd8 bit 6 is set, or 0x25ed90 refuses (an attack window, states 0xf1 / 0xf2, the takedown states). [§2026-11-01T15:23:21](#ts-20261101-152321)
- Other hits take their damage from ctrl+0x70 through 0x15afb4; at 0 the entity is destroyed through 0x23cd80, which is the death path of message 1. The health is `Health` (default 4); the mission 1-4 goons have 2. [§2026-11-01T15:23:21](#ts-20261101-152321)
- PUNCHED plays clip 180 `punched01` and turns to the attacker, then hands back to IDLE when the clip finished (update 0x2a47b0, the USER ANIM update). KNOCKED DOWN (19) plays 177 `react_knockd`, waits 60 updates after the clip, then GET UP (18) plays 178 `react_knockd_getup` and hands back to IDLE. 19 is requested by messages 27 and 32 only. [§2026-11-01T15:23:21](#ts-20261101-152321)
- The arrest health sub +0x122 is `ArrestHealth` + 1 (goons of mission 1-4: 2). Only the enter 0x28cbf8 of the takedown states 0x122..0x13e lowers it (0x2b900c, by byte +10 of the move row of table *0x372e34); at 0 the character goes into the eight-slot list 0x5187cc (0x192fd4) and the event *0x376004 fires. The takedown starts from the player's 0x100 button against an engaged goon in state 0x130 / 0x131 (0x1fdbd4 -> 0x282498 -> 0x2106ec). [§2026-11-01T15:23:21](#ts-20261101-152321)
- Port: `src/runtime/character_hit.*`, glue `src/runtime/game_character_hit_glue.cpp`, clips through `LevelCharacters::startStateClip`. [§2026-11-01T15:23:21](#ts-20261101-152321)
- Worker 20261101-mission-D found that a Y tap near a goon starts a fight move (player state 0xe3), not message 0; the servant's run D-1 saw no message 0 or 62 reach a goon on a tap ([player-attack.md](player-attack.md)). So the punch path of the receiver may not be what the goons of mission 1-4 meet; COMBAT-1. [§2026-11-01T15:23:21](#ts-20261101-152321)
- The fight move's target enters MELEE PUTDOWN (227, enter 0x1d6d04: clip of `tables/tablescombat.bxls` sheet 2 by the move's sheet 1 byte +0xa, target mode 1); at its clip end 0x22e884 (kind 16) requests PUTDOWN STUN 309. The takedown states 0x122..0x13e share the enter 0x28cbf8 (stun clip, a time of sheet 5 by `HitTimingStun` / `HitTimingArrestable`, the arrest health drop by sheet 2 byte +10), the hook 0x29ad90 (timer, then GENERIC GETUP 313) and the exit 0x283a0c. At arrest health 0 the character goes into the eight-slot list 0x5187cc and CharacterArrestable is raised (*0x376004), or with a CharacterAvoidArrest trigger AVOID ARREST 314 follows. A mission 1-4 goon needs two throws by the code. [§2026-11-02T15:37:50](#ts-20261102-153750)
- Measured: a Y-tap throw drops the arrest health of a mission 1-4 goon by 1 (2 -> 1: 309 for 10 updates, then 313 and state 1); a counter throw (rows 18 / 21) drops it to 0, which lists the goon and raises CharacterArrestable in its landing update and shows the hint "Press A to arrest a criminal." with a ring over the goon; it lies about 4 s. In one update the state update's request is entered at once and the hook runs after the enter. [§2026-11-02T15:56:52](#ts-20261102-155652)
- The aggro 0x1d1170 (receiver 0x1b8770): same side nothing; not engaged (ctrl+0xd8 bit 5) the pursuit call 0x21102c (AI state 2); engaged the target switch 0x2080a0 beyond ctrl+0x80 and the counter ctrl+0x72 = 90. [§2026-11-02T15:37:50](#ts-20261102-153750)
- Port: `src/runtime/character_takedown.*`, glue `src/runtime/game_character_takedown_glue.cpp`. [§2026-11-02T15:37:50](#ts-20261102-153750)
- The popups 600 / 700 of a fight are 10 x the running sum of the stud counts (sheet 2 byte +11) of the throws, requested by 0x1b9878 for the HUD object 0x5197c8 at the strike of each throw, next to the stud drop 0x21e2e4. [§2026-11-02T16:13:53](#ts-20261102-161353)
- Open: the arrestable lie of about 124 updates against 120 by the timer, the owner and reset of the popup sum, the counter takedown and the arrest itself, the pair motion, the AI pursuit, the KNOCK BACK states, the countdown of the hit list 0x593178.
- Renamed files: src/runtime/game_character_hit_glue.cpp -> src/runtime/game_character_hit.cpp, src/runtime/game_character_takedown_glue.cpp -> src/runtime/game_character_takedown.cpp.

<a id="ts-20261101-152321"></a>

## 2026-11-01T15:23:21+01:00 — The hit receiver 0x1b8570, message 46, the hit states and the arrest health (REA and llvm-objdump of artifacts/game/game.arm.elf, 20261101-mission-E; no emulator)

### Method

- REA (Ghidra 12.1.2) pseudocode: 0x1b8570, 0x1d6c84, 0x1d1170, 0x22e884, 0x23df6c, 0x25ed90, 0x15afb4, 0x22c60c, 0x244fc8, 0x2414b4, 0x23d610, 0x205fbc, 0x23deb4, 0x245b00, 0x219ba0, 0x23a70c, 0x283c38, 0x2a4814, 0x25f0ec, 0x2a47b0, 0x23ad70, 0x23de70, 0x19b544, 0x21941c, 0x307e7c, 0x2b900c, 0x32e5a8, 0x28cbf8, 0x192fd4, 0x2106ec, 0x282498, 0x2107e0, 0x19bd98, 0x22ac74, 0x2f0fd0.
- llvm-objdump (`--triple=armv6k --mattr=+vfp2`): the message switch of 0x18e4c8 (0x18e4c8..0x18e74c, 0x18e594..0x18e614, 0x18ee2c..0x18ef88), 0x1b8980..0x1b8c78 (the damage branch, to check Ghidra's reuse of one variable), 0x1b8478 (GET UP enter, no Ghidra function), 0x1c7430, the character create 0x236d00..0x2372b0 (class parameters), 0x1fdfd0..0x1fe108 (the takedown button), 0x200490..0x20052c.
- The locomotion state descriptors: 28-byte records {id, update, enter, exit, fourth hook, name, 0} at 0x357538..0x357a40 (46 states) and 0x357b74..0x357be4 (311, 322..324), read from the image with a throwaway script.
- Class values of the goons: a throwaway C++ program over the loader (lvl_01, `chars/char_ai_cherrytreehillsgang1..4`); tablesanimations sheet 1 rows read the same way.

### Message 0 and the hit record

- 0x18e4c8 with message 0 (0x18e70c): when 0x24255c(entity) is non-zero (ctrl+0x6c is 284 or 411: ctrl+0x130) the message goes on to that entity, else `0x1b8570(entity, record)`.
- The record (28 bytes): +0 a move record (non-zero only for the takedown moves), +4 the sender, +0xc the damage, +0x10 a float force, byte +0x14 the kind, byte +0x17 the weapon type. Chase's punch sends {0, player, 0, 1, 0, 1, 0} ([player-attack.md](player-attack.md)).
- 0x244fc8 calls only the class handler of the target's kind (table 0x583138, stride 0x28); no state or AI component sees the message.

### The hit receiver 0x1b8570 (entity, record)

In order; "return" ends without damage or reaction.
1. Entity +0x10 bit 0 (disabled): return. ctrl+0x316 & 0xc0: 0x2253a8. Kind 7: return.
2. The player in states 0x156..0x158: return. A kind 19 hit with a sender requests 0x1b1 when ctrl+0x6c is 0x1b0 / 0x1b1, then returns.
3. Not the player, kind 4 (or kind 0 with a move record) and ability 0x2b (0x23a70c, the bit set of the character): `0x2345dc() < ctrl+0xa0` (1.0 with sub +0x12f bit 2) or state 0xe8 dodge through 0x210c64 and return.
4. A sender that is a character (0x245b00: kinds 27 and 149, the list 0x3743ec, or entity +0xc bit 14): from the player the target's ctrl+0xda & 7 must take it (0x219ba0: modes 0 and 3 yes, 2 only from ctrl+0xc4, 4 and others no); a non-player target then calls 0x1d1170 (it turns on the sender: 0x21102c pursuit when it is not engaged, ctrl+0x72 = 0x23ca4c() x 3).
5. The player: more state and global checks (0x37689c / 0x3768c4 with damage below 200, states 0x43, 0x1a6..0x1a9), kind 3 forwarded to a used object of kind 0xb3. Others: an entity named `Player1.Player01..10` (0x23df6c) returns.
6. Clear sub +0x12f bit 7. Damage below 100 returns (kind 3 with a positive force still pushes, 0x215634) when ctrl+0x2ce is below 136 (the come-back counter), the entity is in the hit list 0x593178 (0x19b544), ctrl+0x6c is 14..17, 225 or 322..324, or a character sender and 0x1758d8.
7. 0x23ad70(ctrl, weapon): bit `weapon` of byte +0x1c of the character table row (*0x372e10 + CharacterEnum x 0x28), state 0xdb with ability 0x2b, or the player in a combat window. Not set, kind 5 and weapon 10: health 1, state 233, return.
8. Cooldown r9 = 20 for the player, else 10, or 5 for kind 1. Damage r7 = the record's damage when 0x23ad70 is clear (0 for weapon 5 with 0x23ad70(ctrl, 0), and 0 for a hidden node 0x23de70 without ctrl+0x314 bit 7 on a non-player); a kind 1 hit with ability 7 on the target but not on the sender clears the reaction flag r8. When 0x23ad70 is set: damage and cooldown 0, no reaction, except kind 6 with weapon 6 or 4, or a character sender of kind 1 with ability 7 (reaction), the target's ability 7 on the ground (message 46 to the sender: the counter), or a sender of kind 64 on the ground (message 32 to itself, below).
9. `0x22e884(entity, ctrl, record)`: non-zero turns the damage to 0. First (0x22e890..0x22e8b0) a target of kind 159 (`chars/ride_pig`) returns 1 for every hit kind but 4. Then it returns 1 for a boss (kind 15 requests 188), and with a move record it dispatches on the move's byte +0x66 (0x22e8f4): 1..6 request 0x122, 7 0x124, 11 0x126, 12 0x128, 13 0x12a and return 1; 0, 8, 9, 10 and 14 or more reach 0x22e9dc and return 0. Without a move a target in a takedown state returns 1, and kinds 13 to 16 and 18 request states 300, 0x12e, 0x133, 0x135 / 0x137, 0x13d and return 1; otherwise 0.
10. **A sender, kind 1 and a target that is not the player: damage 0 and straight to step 13** (0x1b8b88..0x1b8bc4: `ldr r0,[r4,#4]; cmp; ldrb [r4,#0x14]; cmp #1; ... movne r7,#0; bne 0x1b8e88`). Ghidra's output hides this behind a reused variable; the instructions settle it.
11. Damage below 0 heals ctrl+0x70 up to sub +0x128 and ends (no reaction). Damage 0 goes to step 13. Above 0: a boss with 2 or more takes 1 (unreachable, as 0x22e884 already returned 1 for the same entity bit), ctrl+0x315 bit 5 makes it 100; the force is clamped to 1.0; 0x2253b8 for a character; then 0x15afb4.
12. **0x15afb4(force, entity, &ctrl+0x70, move, damage)**: nothing with entity +0xc bit 4, ctrl+0xd8 bit 6 or health 0. With a move record the character is pushed (0x249030 .. 0x23d218). Damage below the health subtracts; else (not a boss with the trigger *0x377610) health 0 and **0x23cd80(entity, 0)**, the entity destroy that sends message 1 ([script-natives-world-death.md](script-natives-world-death.md)). Then kind 3 with a force pushes (0x215634; with a sender of kind 0xb7 the push vector ctrl+0x2f8 = ±0.15, 0, force / 30); kind 17 from kind 0xf2 on a minifig pushes by 2.0; a character with health left and damage above 0 calls 0x27f6f0 and clears sub +0x129; at health 1 (0x1b8dec..0x1b8e48) a sender of kind 27 with its sub +0x12e bit 5 clears the reaction flag r8 unless the target is the player of 0x3772b8, has its own bit 5 or is a boss (bit 5 has no writer, so it never does). Unless entity +0xc bit 4 or ctrl+0xd8 bit 6, a cooldown above 0 enters the hit list (0x22c60c). A destroyed character goes on the same way: the destroy set entity +0xc bit 4 (the destroyed bit), so it gets no hit list entry, and step 13 still sends message 46, whose request 0x2414b4 refuses (that call takes only DEAD, 11, on an entity with the destroyed bit).
13. Message 32 to itself with the kind-64 flag of step 8; otherwise, unless the reaction flag is clear: kind 3 with a force pushes (0x215634), else a damage below 16000 sends **message 46 with the record to itself**. The player at health 0: 0x210c38; the player entity of 0x3772b8: the hearts 0x2316b4; 0x23cee8(*0x3775e0, entity, entity, 255) last.

The hit list 0x593178 (count 0x3774e0, 12-byte entries {entity, u16 duration, byte, flags}): 0x22c60c adds or refreshes an entry (flags bits 2..3 from argument 3, bit 5 from entity +0x10 bit 12, which it clears), 0x19b544 tests membership, 0x22ac74 removes one and 0x2f0fd0 all (restoring the node colour 0x23e110 or the hidden flag). The writer that counts the duration down was not found.

### Message 46 (0x18ee2c)

- Gates: ctrl+0x6e equals ctrl+0x6c (no request pending), ctrl+0xd8 bit 6 clear, 0x25ed90(entity): 0 in 0xa3 / 0xa4 when the attack record (*(sub+0)) +9 is 0 or 3 with +0x10 bit 4, 0 in 0xf1 / 0xf2, else 0x23d610 (0 in the takedown states 0x122..0x12f, 0x134, 0x135, 0x139..0x13e, 0x143, 0x144).
- Kind 4 with weapon 2, or kind 6 with weapon 6 or 4: 0x29b4b4(entity, weapon), then by weapon: 6..9 and 11 request 224 (unless in 224), else 189 (unless in 189).
- Kind 1 with a sender: 190, or 191 when ctrl+0x6c is 190 (0xbe / 0xbf at 0x18eeec).
- Otherwise, unless in 188: 188 with ctrl+0x314 bit 6 (on the ground), else 5.
- ctrl+0x140 = the sender (0x18ef80).
- Messages 27 and 32 (0x18e5f8): 32 only with no request pending; with a sender the heading ctrl+6/8/10 turns to it (0x23df44 x 10430.378, 0x23deb4); then 0x2414b4(entity, ctrl+0x50, 19, 0), KNOCKED DOWN. No `mov r1, #27` / `#32` before a call of 0x244fc8 was found apart from step 13, so the senders of 27 are unknown.

### The hit states

Descriptors {id, update, enter, exit, hook, name}: 18 GET UP {0x25f0ec, 0x1b8478, -, -}, 19 KNOCKED DOWN {0x2a4814, 0x283c38, -, -}, 188 KNOCK BACK {0x2a47b0, 0x26b52c, -, 0x25f45c}, 189 KNOCK BACK CONT {0x2b246c, 0x29b00c, -, 0x25f45c}, 190 PUNCHED and 191 PUNCHED REPEAT {0x2a47b0, 0x1d6c84, -, 0x1c7430 (bx lr)}, 87 THROWN BACK {0x29bccc, 0x2776a8}, 111 TAUNT, 112 MOVE TO SPAWN POINT, 113 LOST TARGET, 311 AUTO ARRESTED, 322 ARRESTING, 323 ARREST, 324 ARRESTED.
- 0x21941c(blend, speed, entity, row, mode, ...): maps the tablesanimations row through the entity's animation set (0x2288c0) and starts every clip of the set in a new slot (0x245b40 / 0x23afc0), stores the row in ctrl+0x2bc and the speed in ctrl+0x310.
- PUNCHED enter 0x1d6c84: 0x21941c(0.1, 1.0, entity, 180 `punched01`, mode 0); ctrl+0x2b8 = 0; 0x205fbc turns ctrl+0xa to the attacker ctrl+0x140 when it is farther than the float entity +0x6c (atan2 of the horizontal difference x 10430.378); ctrl+8 = ctrl+0xa and 0x23deb4 turns the node. The update 0x2a47b0 requests 1 once slot kind (0x22f560) is 6, runs 0x215804 and copies ctrl+8 to ctrl+0xa.
- KNOCKED DOWN enter 0x283c38: clip 177 `react_knockd`; ctrl+0x2c0 = 60 (2 for a boss); entity +0x10 |= 0x100, +0xc |= 0x20000; ctrl+0xc4 (the target) = 0. Update 0x2a4814: while slot kind is 6, ctrl+0x2c0 counts down and at 0 requests 18; ctrl+0xc bit 0 cleared, ctrl+0xa = ctrl+8, the mover 0x20cd30.
- GET UP enter 0x1b8478: clip 178 `react_knockd_getup` (blend 0.1, speed 1.0). Update 0x25f0ec: slot kind 6 requests 1; the rest as 0x2a4814.
- The clips: tablesanimations sheet 1 rows 177 `react_knockd`, 178 `react_knockd_getup`, 180 `punched01`; each exists as `models/char_minifig/class_generic/<name>_minifig.bfnanm`.

### Health and arrest health (the create 0x236d00..0x2372a0)

- `Health` (0x2451d0, default 4) into sub +0x128 (the player gets one more with 0x2369a8(17)); the DEAD enter hook 0x19d3b0 and 0x218b88 copy it into ctrl+0x70. `ArrestHealth` (default 0) + 1, at most 255, into sub +0x122 and +0x123; `HitTimingStun` (default 0) into +0x12c, `HitTimingArrestable` (default 1) into +0x12d. ctrl+0x2ce = 255.
- The 15 goons `1-4_AI_Goon_01..17` of lvl_01 (without 04 and 05; `chars/char_ai_cherrytreehillsgang1..4`, kind 27, CharacterEnum 142, AIFriendly 0): Health 2.0, ArrestHealth 1.0, HitTimingStun 0, HitTimingArrestable 1. The balcony and door goons of mission 1-5 have ArrestHealth 3.0.
- 0x2b900c(entity, ctrl, delta): with a negative delta and +0x122 not 0 it starts effect 265 (0x23de24) 0.5 above the joint found by name; +0x122 += delta, at least 0; when the trigger *0x377614 holds a threshold that +0x122 crossed, it fires.
- Its only caller is the enter 0x28cbf8 of the takedown states (299, 0x123, 0x125, 0x127, 0x129, 0x12d, 0x12f, 0x134, 0x135, 0x13c): the clip of the move row (table *0x372e34, 12-byte rows, by byte +0xe of the attack record), the timing ctrl+0x24c from table *0x372e68 (60-byte rows by sub +0x12c or +0x12d), then the delta -byte +10 of the move row; at 0 (and not the trigger *0x376000) 0x192fd4 puts the entity into the first free of eight 0x40-byte slots at 0x5187cc (entity at +0x2c, a timer started with -1.0 through 0x23cc00), restores ctrl+0xda and fires *0x376004.
- A strike event of a takedown move (the loop at 0x1d6b20..0x1d6c58 inside the body Ghidra merged into the character handler) drops studs while +0x122 is not 0 and sub +0x131 & 0xc0 is clear: 0x21e2e4 with the count byte +0xb of the move row and skip 30 at the strike point, then 0x1b9878(goon, move). A candidate for the score popups (COMBAT-5), not checked.
- 0x2107e0 (the player's attack input) tests a target for the arrest: arrest health 0, not *0x376000, in one of those takedown states with a non-zero arrest damage.
- The takedown: in 0x1fdbd4 the player's ctrl+0xe bit 0x100 (0x210b50 mode 0) with 0x282498 finding an entity of the engaged table 0x58deb0 (6 entries) whose ctrl+0x140 is the player and whose ctrl+0x6e is 0x130 / 0x131 (without abilities 0x33 / 0x35) calls 0x2106ec(player, goon): the player's combat action variant 7, a random move index 1..8 other than the last, a move object 0x1cb808 from table *0x372e30, and state 230 on the goon. 0x19bd98 is the reach test of the grab.

### Unknowns

- Which button sets ctrl+0xe bit 0x100 (slot index from *0x200740) and the player's side of the takedown (D's area, [player-attack.md](player-attack.md)).
- What knocks the mission 1-4 goons down and in which state one is arrestable (the hint "Press A to arrest a criminal."); the score popups 600 / 700 (no string or table found). The oracle requests 20261101-mission-E-1..3 were sent to the servant and are queued, not answered; nothing in this entry is measured.
- The writer that counts the hit list 0x593178 down, the effect of 0x27f6f0, 0x2253a8 / 0x2253b8, the abilities 7 / 0x2b / 0x33 / 0x35, 0x1758d8, the character table byte +0x1c, the float entity +0x6c of 0x205fbc, the start value of ctrl+0xda & 7.
- The KNOCK BACK states 188 / 189, FALL as a reaction, 224 and the pushes 0x215634 / 0x249030.

<a id="ts-20261102-153750"></a>

## 2026-11-02T15:37:50+01:00 — The takedown states, the arrest health drop, the arrestable list and the aggro (llvm-objdump of artifacts/game/game.arm.elf, 20261102-mission14-B; no emulator)

Adds to [§2026-11-01T15:23:21](#ts-20261101-152321); nothing there is contradicted. For the takedown states the four functions of a state record are update, enter, per-update hook and exit (below).

### Method

- llvm-objdump (`--triple=armv6k --mattr=+vfp2`) with the literal pools resolved by a throwaway script: 0x1d6d04, 0x27726c, 0x22e884, 0x28cbf8, 0x283b10, 0x29ad90, 0x283a0c, 0x2b900c, 0x192fd4, 0x21e700, 0x1d1170, 0x21102c, 0x23ca4c, 0x23cff4, 0x2345dc, 0x24615c, the registrations 0x279da0..0x27a110, the table loader 0x2f60e8..0x2f61b8 and the event registration 0x15b888..0x15b940. REA was not used: its Ghidra provider did not start in this session.
- Data: `build/chase-tabledump tables/tablescombat.bxls` and `tables/tablesanimations.bxls`; the lvl_01 goons and triggers through the C++ loaders (test `character-takedown-real-data`).
- Nothing in this entry is measured; the oracle request 20261102-mission14-B-1 (COMBAT-1) was sent through the lead.

### Tables (loader 0x2f60e8: `TablesCombat.xls` sheets 0..5 into *0x372e24, *0x372e28, *0x372e34, *0x372e2c, *0x372e30, *0x372e68)

- Sheet 1 (*0x372e28, the fight moves, 20-byte rows): byte +0xa (column 4) is the takedown row of the target (0, 7..13 for moves 0..7), +0xb (column 5) the row for a target with ability 51 (45..52).
- Sheet 2 (*0x372e34, 12-byte rows, 57 rows): five u16 clips of tablesanimations sheet 1 (first +0 grabbed, second +2 stun, third +4 get up, +6, +8), byte +10 the arrest damage, byte +11 the stud count. Row 9 (move 3, `grab_putdown_04`): 468 `grabbed_putdown_04`, 308 `react_knockd_stunb`, 309 `react_knockd_stunb_up`, 315, 316, damage 1, studs 20.
- Sheet 5 (*0x372e68, 60-byte rows: twelve floats, two ints, a float): the times of the stun states in seconds, row by `HitTimingStun` (sub +0x12c) or `HitTimingArrestable` (+0x12d). Row 0: 0.35 0.35 0.35 0.5 1.0 3.0 2.5 4.0 1.0 2.5 1.0 3.0; row 1: 4.0 4.0 4.0 1.0 2.0 6.0 5.0 8.0 10.0 5.0 1.0 3.0; row 2: 3.0 3.0 3.0 0.5 1.0 3.0 2.5 4.0 5.0 2.5 1.0 3.0.

### The states

- 0x24615c(id, update, enter, hook, exit on the stack, name, ...) fills the state table *0x3742c8. 0x279da0 registers 0x122..0x12f, 0x133..0x136 and 0x139..0x13e with update 0x283b10, enter 0x28cbf8, hook 0x29ad90 and exit 0x283a0c: ENERGYGUN / WATERGUN / FREEZEGUN / GRAPPLEGUN / EGGGUN / JUMPSLAM (two pairs) HIT and STUN 0x122..0x12f, THROWN HIT / STUN 0x133 / 0x134, PUTDOWN STUN 0x135 (309), FALL ARREST HIT 0x136, GENERIC GETUP 0x139 (313), AVOID ARREST 0x13a (314), BOSS HIT / STUN 0x13b / 0x13c, RUN OVER HIT / STUN 0x13d / 0x13e. 227 MELEE PUTDOWN, 228 GRAB PUTDOWN, 229 TACKLE PUTDOWN, 230 MELEE PARRY, 231 GRAB COUNTER THROW and 325 TACKLE ARREST share update 0x27726c and enter 0x1d6d04.
- **MELEE PUTDOWN enter 0x1d6d04**: the pair object (sub +0x14) gives the move index +0x478; for 227 the row is sheet 1 byte +0xa of the move (+0xb when 0x23a70c(ctrl, 51)), stored in the combat record (*(sub+0)) byte +0xe; 228 reads sheet 3 byte +2, 229 / 325 sheet 1 byte +0xa, 230 sheet 4 byte +0xa, 231 sheet 0 u16 +34. It plays the first clip of that sheet 2 row (0x21941c blend 0.1, speed 1.0, mode 0), places the pair (0x205f08, 0x205e24), sets the target mode ctrl+0xda & 7 to 1 and entity +0xc bit 21, calls 0x21e854 and clears sub +0x131 bits 4..5. This fits the oracle (20261101-mission-D-1): the goon's ctrl+0xda becomes 1, +0xc gains bit 21, clip 468.
- **MELEE PUTDOWN update 0x27726c**: while the clip runs, 227 / 229 / 325 call 0x1fda20, 228 / 231 0x1fd9cc, 230 0x26e284 (the pair motion, not read). When the clip is done (slot kind 6) and the entity is not destroyed (+0xc bit 4): the target mode is restored from ctrl+0xda bits 3..5 and bit 21 cleared; 325 requests 324 ARRESTED; 227 with ability 51 goes idle (0x2246c0, request 1); otherwise a record {no move, no sender, damage 1, kind 16} goes to 0x22e884. The part at 0x27745c handles a fall from height (sub +0x131 bit 4, 0x1fdad0).
- **0x22e884 without a move record**: in the states 0x122..0x12f, 0x134, 0x135, 0x139..0x13e, 0x143, 0x144 it returns 1 (jump table 0x22ea24, the same list as 0x23d610); otherwise by kind: 13 requests 300 with record +0xe = 1, 14 302 with +0xe = 2, 15 0x133, **16 PUTDOWN STUN 0x135, or AUTO ARRESTED 0x137 when sub +0x131 & 0xc0**, 18 reads the sender (0x22eb9c, not read), 17 returns 0.
- **Takedown enter 0x28cbf8** (jump table 0x28cc38): ctrl+0x2b8 = 0. The gun HIT states 0x122 / 0x124 / 0x126 / 0x128 / 0x12a play the first clip (mode 0) and fall when in the air (r7); 0x12c, 0x12e, 0x133, 0x136, 0x13b and 0x13d play the first clip. The STUN states play the second clip with mode 1 and take a time from the timing rows at offset 0x123 +12, 0x125 +20, 0x127 +32, 0x129 +16, 0x12b +24, 0x12d +36, 0x12f +28, 0x134 +0, **0x135 between +4 and +8** (a + (b - a) x 0x2345dc(), one draw for the stun row, then one for the arrestable row); 0x13c plays the second clip with time 0; 0x13e plays the second clip and sets ctrl+0x24c = 2.0; 0x139 and 0x13a play the third clip (mode 0). Then for every state: ctrl+0xda & 7 = 1 and entity +0xc bit 21.
- **The arrest health drop** (0x28d2a0, in 0x123, 0x125, 0x127, 0x129, 0x12b, 0x12d, 0x12f, 0x134, 0x135 and 0x13c): ctrl+0x24c = the stun time; when the row's arrest damage is not 0: 0x2b900c(entity, ctrl, -damage); then when sub +0x122 is 0 and 0x23cff4(*0x376000, entity) finds no CharacterAvoidArrest trigger of the character: sub +0x9c and ctrl+0x24c = the arrestable time, **0x192fd4 lists it**, ctrl+0xda & 7 is restored from bits 3..5 (it can be targeted again) and **0x23cee8(*0x376004, entity, entity, 255, 0) raises CharacterArrestable**. Last: sub +0x131 bit 5 cleared, a gun HIT state in the air starts the fall (0x2554c0, 0x1fdad0, bit 5), bit 4 cleared.
- 0x2b900c(entity, ctrl, delta): with a negative delta and +0x122 not 0, effect 265 (0x23de24) 0.5 above the joint named by the string at 0x2b9114; +0x122 += delta, at least 0; then 0x23cff4(*0x377614, entity) looks for an `OnHealthValue` trigger of the character (event slot 0x6c, registered at 0x16bc98) and raises it (0x23cee8, 255) when the old value was above the trigger's float +20 (as an integer) and the new one is at or below it.
- **Per-update hook 0x29ad90**: the HIT states 0x122..0x12e (even), 0x13b and 0x13d request state + 1 when the clip is done; 0x133 and 0x136 likewise, or 0x137 with sub +0x131 & 0xc0; the STUN states with the drop count ctrl+0x24c down by the real dt (0x23602c) and request GENERIC GETUP 0x139 at or below 0, except with arrest health 0 and a CharacterAvoidArrest trigger, which requests AVOID ARREST 0x13a; BOSS STUN does nothing; RUN OVER STUN resets the timer to 2.0 while 0x278b04 holds (not read) and counts down the same way; GENERIC GETUP and AVOID ARREST request IDLE when the clip is done.
- **Exit 0x283a0c**: ctrl+0xda & 7 restored, bit 21 cleared; 0x123, 0x125, 0x127, 0x12b, 0x12d, 0x12f, 0x134, 0x135 and 0x129 (which also calls 0x2428c0 unless 0x139 follows) remove the character from the list (0x21e700); AVOID ARREST raises CharacterAvoidArrest (0x23cee8(*0x376000, entity, entity, 255, 0), after 0x1fcc40 when ctrl+0x2cc is 163).
- The update 0x283b10 of the takedown states is the mover 0x215804 with the fall from height; it requests nothing.

### The arrestable list and the events

- 0x192fd4: the first of the eight 0x40-byte slots at 0x5187cc with +0x2c 0 takes the entity; 0x23cc00(slot, 0, -1.0) starts its timer. No free slot: nothing. 0x21e700 clears the slot of an entity through 0x2459d8. Other readers of 0x5187cc: 0x1cc4d0, 0x2ceda8, 0x2cf69c (not read; an arrest marker is likely).
- The event types registered at 0x15b888..0x15b93c (base 0x375fe0): +0x18 `CharacterArrested`, +0x1c `CharacterGrabbed`, **+0x20 (*0x376000) `CharacterAvoidArrest`**, **+0x24 (*0x376004) `CharacterArrestable`**, +0x28 `PursuitEnded`.
- lvl_01 has CharacterArrestable triggers on two goons of mission 1-4 (03 and 06); its only CharacterAvoidArrest trigger is on `1-8_AI_Tony_Knuckles`.

### What this gives for a goon of mission 1-4 (by the code, not measured)

- `ArrestHealth` 1.0 gives sub +0x122 = 2, `HitTimingStun` 0 and `HitTimingArrestable` 1. A fight move (move 3) puts it in 227 with clip 468; at the clip end it enters PUTDOWN STUN with clip 308, the arrest health drops to 1 and the timer is 0.35 s; then GENERIC GETUP (clip 309) and IDLE. A second fight move takes the arrest health to 0: the goon is listed, CharacterArrestable is raised (for goons 03 and 06 that starts their trigger script) and it lies for 4.0 s before GENERIC GETUP.
- The oracle saw goon A lying in state 309 with clip 306 and +0xc 0x00220004 (bit 21) after a throw ([player-attack-characters.md §2026-11-01T16:02:02](player-attack-characters.md#ts-20261101-160202)), which fits PUTDOWN STUN of a move row whose second clip is 306.

### The aggro 0x1d1170(target, sender)

Called by the receiver at 0x1b8770 for a character sender the target took (after the target-mode test of a player sender), unless the target is the player.
- Same side (entity +0xc bit 2 equal): nothing.
- ctrl+0xd8 bit 5 clear: 0x21102c(target, sender), and nothing more unless that set bit 5. 0x21102c: nothing in state 82; ctrl+0xdc bits 3..4 = 2; no sender: idle 0x2246c0; else a log call 0x22dac8, the path record 0x206d18 and, when ctrl+0xd8 bit 5 is set, AI request 2 (ctrl+0x15, the pursuit) and ctrl+0xc4 = sender.
- Bit 5 set: nothing with ctrl+0xd9 bit 0 (AIIgnorePlayer). When ctrl+0xc4 is the sender or ctrl+0x72 is not 0: ctrl+0x72 = 3 x 0x23ca4c() (the update rate, 30 by default: 90) only when ctrl+0xc4 is the sender and the sender is the player. Otherwise the distance to the current target (FLT_MAX unless ctrl+0xc4 is the player; 0 for a boss hit by someone other than the player; else the node distance 0x2507dc): farther than ctrl+0x80 switches the target to the sender (0x2080a0) and sets ctrl+0x72 = 90.

### Port

- `src/runtime/character_takedown.*`: `takedownTables`, `takedownTimingOf`, `takedownRandom` (0x2345dc), `CharacterTakedownStates` (MELEE PUTDOWN 227 and the registered takedown states: enter, per-update hook, exit), `putdownRequest` (0x22e884 kind 16), `ArrestableList`, `aggroOnHit`; `receiveHit` sets `CharacterHitResult::aggro`. Glue `src/runtime/game_character_takedown_glue.cpp`: the attack's fight sink starts MELEE PUTDOWN on the goon, entered one update after the fight move as the oracle's goon did (20261101-mission-D-1: 0x1d3980 in u2, state 227 from u3). Log lines `character NAME: state S NAME at update N`, `character NAME: arrest health A -> B, timer T`, `character NAME: arrestable (...)`, `character NAME: event CharacterArrestable at update N` (raised through `ScriptRunner::fireObjectEvent`; the CharacterAvoidArrest query 0x23cff4 reads the enabled triggers). Tests `character-takedown`, `character-takedown-real-data`, `game-script-takedown-goon`.

### Unknowns

- Measured: which state and update a goon reaches after one and after two throws, and whether the arrest hint appears after the second (request 20261102-mission14-B-1, COMBAT-1).
- The pair motion of 227 (0x1fda20, 0x205e24), the mover and fall of the takedown states (0x283b10), 0x21e854, effect 265, 0x278b04 of RUN OVER STUN, 0x2428c0, 0x1fcc40, sub +0x131 bits 6..7 (AUTO ARRESTED) and their writers, the readers of the arrestable list and its slot timer, the stud drop of the strike loop.
- The player's side: the counter takedown 0x2106ec (move records through message 0, byte +0x66), the arrest itself (0x2107e0, states 322..324, CharacterArrested) and the other target states 228..231 and 325.
- The meaning of ctrl+0xd8 bit 5 and who sets it; the AI pursuit state 2.

<a id="ts-20261102-155652"></a>

## 2026-11-02T15:56:52+01:00 — The takedown of a mission 1-4 goon in the oracle (Azahar RPC, 20261102-mission14-B-1, emulator servant 20261102-mission14-servant)

Measures what [§2026-11-02T15:37:50](#ts-20261102-153750) read from the code; the rules there hold. New: the order of a goon's update and the moves that drop the arrest health by 2.

### Method

- Emulator servant, New 3DS, OpenGL, RPC, frame-stepped with dt 1/30 after the load, 3 idle updates first; Y held for one update at u1. Recorded from save states of mission 1-4: the first fight in the showroom hall (goon A 1.24 m in front of Chase, goon B 2.59 m away taunting), and a moment where Chase has just thrown a goon (a second goon lies arrestable). Two loads agreed on every update, a third too.
- Traps on 0x2414b4, 0x22e884, 0x2b900c, 0x192fd4, 0x21e700, 0x23cee8 (the two event types only); memory reads per update of ctrl+0x6c / +0x6e, sub +0x122 / +0x123, ctrl+0x24c, ctrl+0xda, the combat record byte +0xe and the clip ctrl+0x2bc. A trapped call is given with the update it happened in; a state read at uN is the state after update N - 1.

### One throw (goon A, arrest health 2 -> 1)

- u1 0x2414b4(A, 227) from lr 0x1fe5d8 (the fight move); read u2: request 227; read u3: state 227, clip 474 (`grabbed_putdown_07`), ctrl+0xda 1, record +0xe 12, sub +0x122 / +0x123 2 / 2, entity +0xc 0x00220004; ctrl+0x24c stays 1.455 in 227. Chase is in 163 to u43 and idle at u44.
- u93 (the clip done): 0x22e884 (lr 0x277410), 0x2414b4(A, 309) (lr 0x22eb90) and 0x2b900c(A, ctrl, -1) (lr 0x28d33c) in the same update. Read u94: state 309, clip 308, arrest health 1 / 2, timer 0.317.
- u103: 0x2414b4(A, 313) (lr 0x18365c); read u104: timer -0.017, request 313; u104: 0x21e700 from the exit (lr 0x283ab8); read u105: state 313, clip 309. So 309 lasted 10 updates (0.35 s).
- u134: 0x2414b4(A, 1); read u136: state 1, clip 109, ctrl+0xda 0, entity +0xc 0x00020004; arrest health stays 1 / 2. No 0x192fd4, no CharacterArrestable or CharacterAvoidArrest event, no arrest prompt in 300 updates.
- After waking, goon A keeps ctrl+0x140 = Chase and attacks with the states 304 / 305 / 306 (clips 266..268 `leap_punch_intro` / `_idle` / `leap_punch`; requests of 0x130..0x132 from lr 0x1fe7f0, 0x2c825c, 0x2c4c50), which knocked Chase back (188) twice in 300 updates. A second tap at u157 took the nearer goon B (227 with clip 438 `grabbed_putdown_02` from u159 to u216, 0x2b900c(B, -1) at u217, 309 with clip 306, 313 at u229 with clip 307, state 1 at u256): no second throw on A was measured.
- So 227 lasts 58 updates (clip 438) to 91 updates (clip 474), by the clip of the move.

### A throw that makes the goon arrestable

- In the second save state the thrown goon had record +0xe 21 (sheet 2 row 21, arrest damage 2): at u26 0x22e884, 0x2414b4(goon, 309), 0x2b900c(goon, ctrl, -2) (arrest health 2 -> 0), **0x192fd4** (lr 0x28d374) and **0x23cee8(*0x376004, goon, goon, 255)** (lr 0x28d3a8), all in the landing update. Read u27: state 309, clip 308, ctrl+0xda 0, timer 3.967. It went to 313 at about u150 and to state 1 at u182 (313 for 31 updates).
- The other goon there (record +0xe 18, arrest health 0 / 2) lay in 309 with timer 3.15 at u1, 313 from u100 to u126, state 1 at u127.
- Rows 18 and 21 are sheet 4 byte +0xa (rows 14..22), which MELEE PARRY 230 reads: these throws came from the counter, not from the Y tap.
- Screens: the lower-screen hint "Press A to arrest a criminal." is up from the first update of that state; an A-button ring is drawn over the lying goon with arrest health 0, a handcuff icon over the other; a 700 popup floats over the throw in flight. The update in which the hint appears could not be separated from the event (it was already up).

### The update order

- 227 -> 309: the request of the 227 update 0x27726c, the enter of 309 (with the drop) and one countdown of its timer happen in one update (u93; timer 0.35 - 1/30 = 0.317 after it).
- 309 -> 313: the hook's request (u103) is entered in the next update (u104), together with the exit of 309.
- So a goon's update runs its state update, then takes a pending request (exit, enter), then its per-update hook; a request made by the hook waits for the next update. The fight move's request from the player's update (u1) is entered in the goon's next update (u2).

### Port

- `CharacterTakedownStates::update` runs in that order (the update of 227, the request, the hook, which sees a clip started in the same update as not finished); test `character-takedown` (`oracleTiming`) pins the timer 0.317 after the enter update, the request of 313 ten updates later, and the arrest health 2 -> 0 with the listing and the event in the landing update for row 21.

### Unknowns

- An arrestable goon lay in 309 for about 124 updates (u26 to about u150) where the timer of 4.0 s gives 120; not explained.
- The time from the first arrest prompt to its disappearance, and the arrest itself (A next to a lying goon): not run.
- The goons' attack states 304..306 (LEAP PUNCH) and Chase's knock-back 188.

<a id="ts-20261102-161353"></a>

## 2026-11-02T16:13:53+01:00 — The popups 600 / 700 of a fight are the fight's stud total, requested at the strike of a throw (Azahar RPC and llvm-objdump, emulator servant 20261102-mission14-servant)

Answers the open point of the score popups in [§2026-11-01T15:23:21](#ts-20261101-152321): they are not a goon score.

### Method

- Emulator servant 20261102-mission14-servant, 2026-11-02, New 3DS, OpenGL, RPC, dt 1/30, traps by value; recorded from the save state of the first showroom fight of mission 1-4 (throws by Y tap; the trace was run twice with the same sequence) and from two later fight states that show the popups 700 and 600. llvm-objdump of 0x1b9878..0x1b9940 by the servant.

### Findings

- The popup is a second HUD number object at 0x5197c8, next to the stud bar 0x519760 (the u64 stud counter 0x593418, shown only when studs are collected; +0x44 / +0x48 hold the counter).
- Request site: in the update of the strike frame of Chase's fight move (about 68 updates after the tap, before the goon lands), the hit-sequence loop 0x1d6b20..0x1d6c58 of the character message dispatcher (inside 0x1d5adc) calls the stud drop 0x21e2e4 with the count of byte +0xb of the takedown move row (*0x372e34, 20 for the rows measured; lr 0x1d6c30) and right after it 0x1b9878(goon, move row index) (lr 0x1d6c3c).
- 0x1b9878: for every combat object of its list with halfword +2 == 2 whose slot holds the goon it adds byte +0xb of the move row (row index x 12 at *0x372e34) to the u16 at object +0x34 and calls 0x22ddf4(0x5197c8, byte [0x5197c8 + 0x40], 10 x that u16 total, sign, 1) at 0x1b9938. The popup shows 10 x the running sum of the stud counts of the throws since that combat object started.
- Measured: the first throw (move row 12) gave 200, the second (row 7) 400 (200 + 200; byte +0xb is 20 in both rows and the stud drop count was 20 both times). The 700 and 600 the user saw are sums of 70 and 60 over several throws, shown by the same call. The object's words at the end: +0x40 type 0, +0x48 0x190.
- The studs themselves (20 per throw, value 200) are collected later through 0x28a128 (adds the value, stores the counter 0x593418 at 0x28a23c, lr 0x180aec), the hook 0x29c090 and 0x22ddf4(0x519760, 1, new total) (lr 0x29c0f4): the counter rose 6400 -> 6500 -> 6600 at u127 and to 6700 and 6800 at u245; that bar shows the 6,6xx totals, not the popup.

### Unknowns

- Which list or owner holds the combat object with the u16 at +0x34 and when it is reset; the widget that draws object 0x5197c8 (the 200 is shown at the top left next to a silver stud icon for about 2 s).
