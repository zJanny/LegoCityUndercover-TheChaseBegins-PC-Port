# Combat: a character takes a hit

## 1. Scope

The character side of a hit: message 0 to a level character (the hit receiver 0x1b8570, damage, the reaction message 46), the hit states PUNCHED, KNOCKED DOWN and GET UP, the takedown of the fight move's target (MELEE PUTDOWN and the takedown states), the arrest health, the list of arrestable characters and its events, and the aggro of a hit character. The player's punch and the fight move on a goon are in [player-movement-attack.md](player-movement-attack.md) (open items PLAYER-26..29); the death of a character (message 1) is [script-system-natives-world-death.md](script-system-natives-world-death.md); the goons' attack on the player (AIPursuit) is not in this file.

## 2. Status

IN_PROGRESS, updated 2026-11-02T15:58:08+01:00. The hit receiver and message 46 are ported for every level character: a punch takes no health and requests PUNCHED; other hits take health and destroy the character at 0. The fight move a Y tap starts on a goon now takes the goon through MELEE PUTDOWN, PUTDOWN STUN (the arrest health drops) and GENERIC GETUP back to IDLE; at arrest health 0 it is listed as arrestable and lies for its arrestable time (4 s), and CharacterArrestable is raised for the level scripts. The oracle confirms it (one Y-tap throw takes a goon's arrest health from 2 to 1; the counter throws that take it to 0 are not ported). The goon does not move during the takedown and the player cannot arrest it yet (COMBAT-7). The aggro of a character hit by a character is decided and logged; the AI pursuit it starts is not ported (COMBAT-9).

## 3. Verified behaviour

- The receiver's guards, the punch without damage, the damage and destroy, the heal, the hit list durations 20 / 10 / 5 and the reaction states 190 / 191 / 188 / 5 / 189 / 224 are pinned by `character-hit` on made-up characters; the PUNCHED, KNOCKED DOWN (wait 60, boss 2) and GET UP hand-overs too. evidence: [combat-characters.md §2026-11-01T15:23:21](../../re/combat-characters.md#ts-20261101-152321).
- The 15 goons of mission 1-4 in lvl_01 have Health 2 and arrest health 2 and react to the punch with PUNCHED; the clips 180, 177 and 178 are `punched01`, `react_knockd` and `react_knockd_getup` of `class_generic` (`character-hit-real-data`). evidence: same entry.
- The takedown read from the code and measured in the oracle (the update order, the arrest health drop of a Y-tap and a counter throw, the stun and arrestable timers, the listing and the event; [combat-characters.md §2026-11-02T15:56:52](../../re/combat-characters.md#ts-20261102-155652)): MELEE PUTDOWN, the takedown states' enter, per-update hook and exit, the arrest health drop, the arrestable list and its events, the tables of `tables/tablescombat.bxls` sheets 1, 2 and 5, `HitTimingStun` 0 / `HitTimingArrestable` 1 of the goons and their CharacterArrestable triggers (`character-takedown`, `character-takedown-real-data`, `game-script-takedown-goon`); the aggro 0x1d1170 (`character-takedown`). evidence: [combat-characters.md §2026-11-02T15:37:50](../../re/combat-characters.md#ts-20261102-153750).

## 4. Stand-ins

- COMBAT-S1 What the receiver reads but the port does not track is fixed: the target select mode takes the player's hit, the come-back counter is 255, no weapon immunity, no abilities, no attack window, no hidden node, no character-sender refusal; hit list entries are never counted down, so a character that took a damaging hit would ignore every later hit below 100 for good (not reachable yet: the punch takes no damage). needs: COMBAT-6. evidence: [combat-characters.md §2026-11-01T15:23:21](../../re/combat-characters.md#ts-20261101-152321).
- COMBAT-S2 The PUNCHED enter turns the character to the player whatever the distance (the distance limit of the turn is not read); the hit states run before the animation clock step of the update. needs: COMBAT-6. evidence: same entry.
- COMBAT-S3 The reaction states KNOCK BACK (188), KNOCK BACK CONT (189), FALL (5) and 224 are logged as not ported and the character stays in its state. needs: COMBAT-4. evidence: same entry.
- COMBAT-S4 The takedown runs on its own machine beside the hit states (a hit during a takedown sees the hit machine's state); the character stays where it stands (no pair motion, mover or fall); the stun clips (mode 1) play once and hold; sub +0x131 bits 6..7 are clear; the random words come from the attack's stand-in generator; the takedown runs before the animation clock step, and GENERIC GETUP ends one update later than in the oracle (32 against 31 updates). needs: COMBAT-8. evidence: [combat-characters.md §2026-11-02T15:37:50](../../re/combat-characters.md#ts-20261102-153750).
- COMBAT-S5 `characterAggro` knows only the side bit of the target: ctrl+0xd8 bit 5, AIIgnorePlayer, ctrl+0xc4, ctrl+0x72 and ctrl+0x80 are taken as clear or 0, and the result is only logged. needs: COMBAT-9. evidence: same entry.

## 5. Open items

- [ ] COMBAT-7 — The player's side of the takedown and the arrest: the counter takedown (move records through message 0), the arrest by A (states 322..324, CharacterArrested), the struggle and the other target states 228..231 and 325. An arrested goon must also count as destroyed for its AI spawner (`objectState`, SCRIPT-410), not only in the hit target. blocks: SCRIPT-411. evidence: [combat-characters.md §2026-11-02T15:37:50](../../re/combat-characters.md#ts-20261102-153750).
- [ ] COMBAT-8 — The rest of the takedown: the pair motion of MELEE PUTDOWN, the mover and fall of the takedown states, effect 265 and the OnHealthValue trigger of the arrest health drop, the writers of the AUTO ARRESTED flags, the readers of the arrestable list (an arrest marker is likely). blocks: COMBAT-S4. evidence: same entry.
- [ ] COMBAT-9 — The AI pursuit the aggro starts (AI state 2), ctrl+0xd8 bit 5 and its writers, and the target and counter fields the aggro reads. blocks: COMBAT-S5, PLAYER-30. uses: AIPursuit. evidence: same entry.
- [ ] COMBAT-4 — The reaction states KNOCK BACK (188), KNOCK BACK CONT (189), FALL as a reaction and 224, and the pushes of a hit with a force or a move, are not ported. blocks: COMBAT-S3. evidence: same entry.
- [ ] COMBAT-5 — The popup of a fight's stud total (the 600 / 700 seen) and the stud drop at the strike of a throw are not ported; the owner and reset of the sum are not read. evidence: [combat-characters.md §2026-11-02T16:13:53](../../re/combat-characters.md#ts-20261102-161353).
- [ ] COMBAT-6 — Parts of the receiver are not read yet (the hit list countdown, the hurt call, abilities, weapon immunity, the hidden-node test, the character-sender refusal, the limit of the PUNCHED turn); the entry lists them under its unknowns. blocks: COMBAT-S1, COMBAT-S2. evidence: same entry.

## 6. Findings summary

- The receiver, message 46, the hit states, the class values and the arrest health: [combat-characters.md §2026-11-01T15:23:21](../../re/combat-characters.md#ts-20261101-152321).
- The takedown states, the tables, the arrest health drop, the arrestable list, its events and the aggro: [combat-characters.md §2026-11-02T15:37:50](../../re/combat-characters.md#ts-20261102-153750).

## 7. Code and tests

- `src/runtime/character_hit.*` (`receiveHit`, `hitReactionState`, `CharacterHitStates`, `characterHitTargetOf`, `characterHitFromPlayer`, `changeArrestHealth`), glue `src/runtime/game_character_hit.cpp` (`LevelSession::characterHit`, `LevelSession::updateCharacterHits`, log lines `character NAME: hit (kind K, damage D) at update N, ...` and `character NAME: state S NAME at update N`), `LevelCharacters::startStateClip`, `holdIdle`, `clipFinished`.
- `src/runtime/character_takedown.*` (`CharacterTakedownStates`, `takedownTables`, `takedownTimingOf`, `putdownRequest`, `ArrestableList`, `aggroOnHit`), glue `src/runtime/game_character_takedown.cpp` (`LevelSession::characterTakedownStart`, `updateCharacterTakedowns`, `characterTakedownTargetMode`, `characterAggro`, `characterArrestEventSink`).
- CTest: `character-hit`, `character-hit-real-data`, `character-takedown`, `character-takedown-real-data`, `game-script-takedown-goon`.
