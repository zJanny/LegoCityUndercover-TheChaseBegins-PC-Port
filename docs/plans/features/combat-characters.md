# Combat: a character takes a hit

## 1. Scope

The character side of a hit: message 0 to a level character (the hit receiver 0x1b8570, damage, the reaction message 46), the hit states PUNCHED, KNOCKED DOWN and GET UP, the arrest health and the state in which a goon can be arrested. The player's punch and the fight move on a goon are in [player-movement-attack.md](player-movement-attack.md) (open items PLAYER-26..29); the death of a character (message 1) is [script-system-natives-world-death.md](script-system-natives-world-death.md); the goons' attack on the player (AIPursuit) and the arrest action are not in this file.

## 2. Status

IN_PROGRESS, updated 2026-11-01T15:56:17+01:00. The hit receiver and message 46 are ported for every level character: a punch takes no health and requests PUNCHED, which plays `punched01` facing Chase and hands back to IDLE; other hits take health and destroy the character at 0 through the death path. KNOCKED DOWN and GET UP are ported as states but nothing in the port requests them yet. The punch reaches the receiver once the lead wires the attack's hit sink (worker D of round 20261101-mission) to `LevelSession::characterHit`.

## 3. Verified behaviour

- The receiver's guards, the punch without damage, the damage and destroy, the heal, the hit list durations 20 / 10 / 5 and the reaction states 190 / 191 / 188 / 5 / 189 / 224 are pinned by `character-hit` on made-up characters; the PUNCHED, KNOCKED DOWN (wait 60, boss 2) and GET UP hand-overs too. evidence: [combat-characters.md §2026-11-01T15:23:21](../../re/combat-characters.md#ts-20261101-152321).
- The 15 goons of mission 1-4 in lvl_01 have Health 2 and arrest health 2 and react to the punch with PUNCHED; the clips 180, 177 and 178 are `punched01`, `react_knockd` and `react_knockd_getup` of `class_generic` (`character-hit-real-data`). evidence: same entry.

## 4. Stand-ins

- COMBAT-S1 What the receiver reads but the port does not track is fixed: the target select mode takes the player's hit, the come-back counter is 255, no weapon immunity, no abilities, no attack window, no hidden node, no character-sender refusal; hit list entries are never counted down, so a character that took a damaging hit would ignore every later hit below 100 for good (not reachable yet: the punch takes no damage). needs: COMBAT-6. evidence: [combat-characters.md §2026-11-01T15:23:21](../../re/combat-characters.md#ts-20261101-152321).
- COMBAT-S2 The PUNCHED enter turns the character to the player whatever the distance (the distance limit of the turn is not read); the hit states run before the animation clock step of the update. needs: COMBAT-6. evidence: same entry.
- COMBAT-S3 The reaction states KNOCK BACK (188), KNOCK BACK CONT (189), FALL (5) and 224 are logged as not ported and the character stays in its state. needs: COMBAT-4. evidence: same entry.

## 5. Open items

- [ ] COMBAT-1 — What knocks a mission 1-4 goon down, in which state it can be arrested (the hint "Press A to arrest a criminal."), how many hits it takes and the clip timing: unmeasured, the oracle requests 20261101-mission-E-1..3 are queued. A Y tap near a goon starts a fight move rather than message 0 (worker D); once both branches are merged this depends on the attack's hit and fight sinks (stand-in PLAYER-S12 of worker D). needs: COMBAT-2. evidence: [combat-characters.md §2026-11-01T15:23:21](../../re/combat-characters.md#ts-20261101-152321).
- [ ] COMBAT-2 — The takedown moves are not ported: the move records of the hit, the takedown states with their move and timing tables, the arrest health drop, the list of arrestable characters, its events and the counter start from the player's button. blocks: COMBAT-1. uses: AIPursuit. evidence: same entry.
- [ ] COMBAT-3 — A punched character does not turn on the sender: the aggro call of the receiver, the pursuit it starts and the engaged table. uses: AIPursuit. evidence: same entry.
- [ ] COMBAT-4 — The reaction states KNOCK BACK (188), KNOCK BACK CONT (189), FALL as a reaction and 224, and the pushes of a hit with a force or a move, are not ported. blocks: COMBAT-S3. evidence: same entry.
- [ ] COMBAT-5 — The score popups 600 / 700 of a defeated goon: no request site found (no string, no table). evidence: same entry.
- [ ] COMBAT-6 — Parts of the receiver are not read yet (the hit list countdown, the hurt call, abilities, weapon immunity, the hidden-node test, the character-sender refusal, the limit of the PUNCHED turn); the entry lists them under its unknowns. blocks: COMBAT-S1, COMBAT-S2. evidence: same entry.

## 6. Findings summary

- The receiver, message 46, the hit states, the class values and the arrest health: [combat-characters.md §2026-11-01T15:23:21](../../re/combat-characters.md#ts-20261101-152321).

## 7. Code and tests

- `src/runtime/character_hit.*` (`receiveHit`, `hitReactionState`, `CharacterHitStates`, `characterHitTargetOf`, `characterHitFromPlayer`, `changeArrestHealth`), glue `src/runtime/game_character_hit_glue.cpp` (`LevelSession::characterHit`, `LevelSession::updateCharacterHits`, log lines `character NAME: hit (kind K, damage D) at update N, ...` and `character NAME: state S NAME at update N`), `LevelCharacters::startStateClip`, `holdIdle`, `clipFinished`.
- CTest: `character-hit`, `character-hit-real-data`.
