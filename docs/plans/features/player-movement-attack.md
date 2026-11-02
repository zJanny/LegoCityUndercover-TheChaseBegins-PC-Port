# Player movement: the attack

## 1. Scope

Chase's attack on foot (Y): the tap, the punch and combo (states 0xa3 / 0xa4), its targets (props and level characters), the fight move a tap starts on a goon, and the messages to the target. Split from [player-movement.md](player-movement.md). Not covered: the hit character's reaction and defeat (the character side), counter (X) and arrest (A).

## 2. Status

IN_PROGRESS, updated 2026-11-02T15:33:47+01:00. Runs in `chase-game`: the punch hits wobbly props; a tap near a goon of mission 1-4 starts a fight move on the nearest one, as in the oracle, which the character side does not answer yet. The grab test sees walls and other characters through the line of sight 0x219990, and the search reads the player's near list.

## 3. Verified behaviour

- Attack on foot: Y tap (release within 9 held updates), state 0xa3 / 0xa4, combo clips 122..125, facing turn to the target, root step 0.07548, hit message 0 (damage 1) in update 5 / 5 / 4 of combo_1..3, message 62 at the entry; target search and hit test of the original: `player-attack-real-data` (oracle the save state in front of the table of mission 1-2), `game-script-attack-table`.
- Character targets: the gate 0x21a2d4, target modes 0x219ba0, weights 0x1b7358, the grab test 0x28265c and the choice of 0x1fdbd4: `attack-targets` (made-up characters, and the oracle's fight state of mission 1-4: the nearer goon in four stick directions, no message 62 or 0), `attack-targets-real-data` (the 15 goons of mission 1-4, the fight moves of `tables/tablescombat.bxls`, equal to the table in memory), `game-script-attack-goon`.

## 4. Stand-ins

- PLAYER-S5 `kPlayerMeleeRangeStandIn` 2.2, `kPlayerAttackAngleStandIn` 90, `kPlayerComboLengthStandIn` 4 — heap values read in the oracle, writers not read. needs: PLAYER-16.
- PLAYER-S6 `kAttackHitTrackStandIn` / `kAttackComboTrackStandIn` — the clip tracks behind the hit and combo-window events, inferred from the measured updates. needs: PLAYER-16.
- PLAYER-S10 The root step of the combo clips is the z translation of the extra joint curve at frame k x framesPerUpdate with linear interpolation between frames; the x and y axes (zero in these clips), the slot weight and the original's interpolation are not read. needs: PLAYER-22. evidence: [player-attack.md §2026-10-19T08:03:14](../../re/player-attack.md#ts-20261019-080314).
- PLAYER-S8 the attack uses libm `acos` for the library acosf 0x2584c8. needs: PLAYER-31. evidence: [player-attack-characters.md §2026-11-01T15:19:31](../../re/player-attack-characters.md#ts-20261101-151931).
- PLAYER-S9 `attackTargetObjects` — of the level objects only `leprop_wobblydestructible` props are attack targets (the other kinds the entity gate accepts are not supplied), and the glue allows the attack only in idle / walk / run / sprint on the ground. needs: PLAYER-15.
- PLAYER-S11 `kAttackSoundOwnerStandIn` 0 — the owner key of the attack sounds (u16 at the player entity +0x14) is not read; the attack sounds are never stopped, so it has no effect. needs: AUDIO-30.
- PLAYER-S12 `characterHitSink` / `characterFightSink` of `LevelSession` only log `attack: hit character ...` and `attack: fight move N on character NAME (state 227)` until the character side takes them. evidence: [player-attack-characters.md §2026-11-01T15:19:31](../../re/player-attack-characters.md#ts-20261101-151931).
- PLAYER-S15 `appendAttackCharacters` — the character facts besides the box and the near list: the `MeshCollision` branch of the box setup (which replaces the box) is not modelled, state 1, on the ground, ctrl+0x240 at the node height, the class row's abilities without the state-dependent changes, target mode 0 unless EnableTargetable 0, and nobody engaged on the target (`PlayerAttackTables::engagedOn` is empty until the character side fills it). needs: PLAYER-30, PLAYER-31. evidence: [player-attack-characters.md §2026-11-02T15:32:44](../../re/player-attack-characters.md#ts-20261102-153244).
- PLAYER-S16 The line of sight sees the world's entries of the port and the level characters the glue supplies, ties go to the target, the query's owner exclusion of two flag bits is not applied, and the near list keeps the candidates' order and uses a box instead of the grid cells (no effect on a search of at most 3.5). needs: PLAYER-32. evidence: [player-attack-characters.md §2026-11-02T15:32:44](../../re/player-attack-characters.md#ts-20261102-153244).
- PLAYER-S14 The fight move plays its clip in the attack states with the punch's motion (facing turn to the target heading, root step); the pair object of both characters, the grab struggle (the tap then does nothing) and the special fight target are not ported, and the move is drawn from the stand-in random generator `breakRandomStandIn`. When no fight move is allowed the port starts nothing, while the original has already entered variant 4 and goes on with move index -1 (cannot happen for Chase, who has ability 46). needs: PLAYER-28. evidence: [player-attack-characters.md §2026-11-01T15:19:31](../../re/player-attack-characters.md#ts-20261101-151931).

## 5. Open items

- [ ] PLAYER-26 — A test that pins the measured tap with goons lying knocked down and Chase idle (a plain punch in the air); the port does the same by reading, but no test checks it. evidence: [player-attack-characters.md §2026-11-02T15:50:34](../../re/player-attack-characters.md#ts-20261102-155034).
- [ ] PLAYER-28 — The fight move's own motion: the enter and update of variant 4, the pair object that places both characters (the goon moves toward Chase from the 12th update of the move), the grab struggle and its two states, the special fight target. blocks: PLAYER-S14. evidence: [player-attack-characters.md §2026-11-01T15:19:31](../../re/player-attack-characters.md#ts-20261101-151931).
- [ ] PLAYER-30 — The engaged table the grab test reads (six entities with their target and next state): fill `PlayerAttackTables::engagedOn` from the character side's aggro and pursuit state. needs: COMBAT-3. blocks: PLAYER-S15. evidence: [player-attack-characters.md §2026-11-02T15:32:44](../../re/player-attack-characters.md#ts-20261102-153244).
- [ ] PLAYER-31 — The character facts not read: the writer of ctrl+0x240, the state-dependent ability changes, the height test of the attack flag 0x40, the kinds 0x43 / 0xcc, the start value of ctrl+0xda, and the library acosf. blocks: PLAYER-S15, PLAYER-S8. evidence: [player-attack-characters.md §2026-11-02T15:32:44](../../re/player-attack-characters.md#ts-20261102-153244).
- [ ] PLAYER-32 — The ray's tie rule, the owner exclusion of two flag bits in its query, composite owners and the other entities of the grid (pedestrians, drivers) in the line of sight and the near list. blocks: PLAYER-S16. evidence: [player-attack-characters.md §2026-11-02T15:32:44](../../re/player-attack-characters.md#ts-20261102-153244).
- [ ] PLAYER-13 — The collision entry of the mesh-kind wobbly props is 0.009 higher than the oracle's entity box (`DonutTable_14` half y 1.52876 against 1.5197); the attack's box test uses the port's value. evidence: [player-attack.md §2026-10-18T19:59:57](../../re/player-attack.md#ts-20261018-195957).
- [ ] PLAYER-15 — Other attack paths: the jump attack (0x1d5254 / 0x1d5308), which other callers of 0x20b984 allow the attack, the weapon-out state 0x9a, the damage-99 flag. blocks: PLAYER-S9. evidence: [player-attack.md §2026-10-18T19:59:57](../../re/player-attack.md#ts-20261018-195957).
- [ ] PLAYER-16 — Sources of the attack values: the writers of ctrl+0x80, *0x372e44 and the combo length, the table from clip track hashes to event types. blocks: PLAYER-S5, PLAYER-S6, PLAYER-19. evidence: [player-attack.md §2026-10-18T19:59:57](../../re/player-attack.md#ts-20261018-195957).
- [ ] PLAYER-18 — Next to the table the oracle's root step slides sideways against the table's entity (contact message 45 of 0x21a73c); the port slides against the collision box and ends 0.3 m away from the oracle's position after one attack. evidence: [player-attack.md §2026-10-18T19:59:57](../../re/player-attack.md#ts-20261018-195957).
- [ ] PLAYER-22 — Trap 0x270778 during a punch (the save state in front of the table of mission 1-2) to confirm that the root sampler runs for the combo slot and what it returns per update. blocks: PLAYER-S10. evidence: [player-attack.md §2026-10-19T08:03:14](../../re/player-attack.md#ts-20261019-080314).
- [ ] PLAYER-23 — What 0x2b80f4 does in the hit event before the hit sound is played (callees 0x2554c0, 0x254250, 0x227790, 0x2408c4, 0x25b2b8, 0x22a9ec unread; a hit effect is likely). evidence: [player-attack.md §2026-10-19T12:54:52](../../re/player-attack.md#ts-20261019-125452).
- [ ] PLAYER-24 — Whether the original sounds anything on a punch (animation sound events of the combo clips, other callers); no audio recording of a punch was compared. needs: AUDIO-34. evidence: [player-attack.md §2026-10-19T12:54:52](../../re/player-attack.md#ts-20261019-125452).
- [ ] PLAYER-21 — Look at the punch in `chase-game` (pose against the clip frames, no foot sliding) and check where the original applies the joint orientation (record +0x88) to 0xfa channels. needs: PLAYER-19. evidence: [skeleton-animation.md §2026-10-18T21:40:55+02:00](../../re/skeleton-animation.md#ts-20261018-214055).

## 6. Findings summary

- Combo clips animate through 0xfa raw-matrix joint curves, decomposed by 0x1324f8: [skeleton-animation.md §2026-10-18T21:40:55+02:00](../../re/skeleton-animation.md#ts-20261018-214055).
- Attack on foot: Y tap, combat action states, target search, hit test and the messages to the target: [player-attack.md §2026-10-18T19:59:57](../../re/player-attack.md#ts-20261018-195957).
- Attack sounds: swing on entering a state and hit on the hit event, from the weapon table row of the character; all ids are 0: [player-attack.md §2026-10-19T12:54:52](../../re/player-attack.md#ts-20261019-125452).
- Line of sight, near list and the box of a character: [player-attack-characters.md §2026-11-02T15:32:44](../../re/player-attack-characters.md#ts-20261102-153244).
- Characters as targets, the grab test and the fight move on a goon: [player-attack-characters.md §2026-11-01T15:19:31](../../re/player-attack-characters.md#ts-20261101-151931); measured in the oracle: [player-attack-characters.md §2026-11-01T16:02:02](../../re/player-attack-characters.md#ts-20261101-160202).

## 7. Code and tests

- `src/runtime/player_attack.*`, `attack_targets.*`, `attack_facts.*`, `attack_sounds.*`; glue `src/game_attack.cpp`, `src/runtime/game_attack_characters_glue.cpp`, `src/runtime/game_attack_facts_glue.cpp`.
- CTest: `player-attack`, `player-attack-real-data`, `attack-targets`, `attack-targets-real-data`, `attack-facts`, `attack-facts-real-data`, `game-script-attack-goon`, `attack-sounds`, `attack-sounds-real-data`, `game-script-attack-table`.
- Scenarios: `tools/oracle/attack-*.txt`.
