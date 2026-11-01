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
- Open: what knocks a goon down in mission 1-4 and when it is arrestable (oracle requests 20261101-mission-E-1..3), the score popup, the takedown moves and the counter, the aggro 0x1d1170, the KNOCK BACK states, the countdown of the hit list 0x593178.

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
