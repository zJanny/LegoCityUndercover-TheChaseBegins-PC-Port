# Script system: the death of a character

## 1. Scope

Message 1 (destroy) of the character handler 0x18e4c8 for kinds 27, 121 and 242, as DestroyProp and KillObject (and every other caller of the entity destroy 0x23cd80) reach it: the `DisableFallapart` modes, the fall-apart body with its pool, the bricks, the stud drop and the respawn tail. The destroy itself and the other kinds are in [script-system-natives-world.md](script-system-natives-world.md); the player's own death and respawn is PLAYER-11 in [player-movement.md](player-movement.md), dropped studs are STUD-11 in [studs.md](studs.md).

## 2. Status

IN_PROGRESS, updated 2026-10-19T14:06+02:00. The handler path, the fall-apart pool and the part motion are read and ported: a killed goon of lvl_02 falls apart into the BrokenHead / Body / Arm / Legs or Hips parts of `levels/charbits.blvl`, which fly, bounce, blink and vanish after 64 updates, with sound 548. The stud drop of a dead character is ported (it drops nothing: count 0), and so are the after-death 0x21e218 choice and the come-back timeline of the state DEAD; `chase-game` does not show a character that comes back (SCRIPT-326). The character's own sound is not ported.

## 3. Verified behaviour

- The mode switch of 0x18e93c (four modes, the minifig test 0x22716c, the brick branch, the respawn tail) is pinned by `character-death` on synthetic inputs. evidence: [script-natives-world-death.md §2026-10-19T13:21:05](../../re/script-natives-world-death.md#ts-20261019-132105).
- The five KillObject targets of lvl_02 (char_ai_prisoner01..03, kind 27, `CharacterEnum` 142) fall apart and stay dead (00_GoonFaller with its instance `AIRespawnIfFriendly` 0 too); the mech 6-9_PlayerMech of lvl_05 (kind 242) vanishes without bricks; the part objects and meshes of `levels/charbits.blvl` and the body part's bounds are pinned by `character-death-real-data`. evidence: same entry.
- The pool (four slots, a fifth start fails), the start placement (head 0.6 below the node, the other parts 0.8 above), the start velocity and spin ranges, the motion without and with a ground hit (rest rule, reflection with 0.5, gravity 0.075) and the lifetime (32 flying updates, blinking on an even clock, free on the 64th update) are pinned by `character-death`. evidence: same entry.
- The stud drop request (mode 1, count 0, skip 30; mode 0, count 30 with bit 5), the after-death choice of 0x21e218 (00_GoonFaller back to its placement, the 2-4 goons out of the engaged table) and the spawn counter with DEAD leaving at 65, the node shown from 66 and the colour pulse are pinned by `character-death` / `character-death-real-data`; that count 0 drops nothing but uses two draws by `stud-drop`. evidence: [script-natives-world-death.md §2026-10-19T14:04:25](../../re/script-natives-world-death.md#ts-20261019-140425).
- In `chase-game`, `--call-script 2-4_AutoArrestA01` in lvl_02 logs the fall-apart, plays sound 548 and logs `stud drop mode 0 count 0` (manual runs, 2026-10-19).

## 4. Stand-ins

- SCRIPT-S264 The death of a character in the port: byte +0x12e bit 5 and the global 0x22b0b4 count as clear and the player branch is never taken (the player is no placed character); a character that comes back stays hidden; no 0x21e854 (weapons and attachment entities), no character sound 0x21e734, no 0x21e700; 0x21decc and 0x22471c are only logged; the sound 548, the bricks and the stud drop start at the node position (0x23fb78 and the entity box not read). needs: SCRIPT-323, SCRIPT-326, SCRIPT-327, SCRIPT-328. uses: KillObject, DestroyProp. evidence: [script-natives-world-death.md §2026-10-19T14:04:25](../../re/script-natives-world-death.md#ts-20261019-140425).
- SCRIPT-S265 The fall-apart body in the port: the blink clock 0x241098 is the game update count, the line test 0x21ab20 of 0x298274 is the wheel ray of the collision world (as VEH-S104), the parts take the class's `UpperTexture` / `LowerTexture` instead of the texture found on the character's node, the head part shows the character's head model, face and hat with their offset from the character at death (0x22690c / 0x2268f4 not read); the white node colour and the release by owner 0x182270 are not ported. needs: SCRIPT-323. uses: KillObject, DestroyProp. evidence: same entry.
- SCRIPT-S268 The respawn tail in the port: the global bytes 0x3771d7 & 0x377540 count as clear; in the come-back timeline 0x225f5c runs before the state update (IDLE and the event at counter 65, maybe 66); a friendly character that stays dead takes its placement matrix through `LevelCharacters::place` (the controller part of SCRIPT-S237 left out). needs: SCRIPT-328. uses: KillObject, DestroyProp. evidence: [script-natives-world-death.md §2026-10-19T14:04:25](../../re/script-natives-world-death.md#ts-20261019-140425).

## 5. Open items

- [ ] SCRIPT-326 — `chase-game` does not show a character that comes back: enabled with the node hidden, DEAD's enter 0x21e218, IDLE after the counter passes 64, shown from 66 with the colour pulse 0x18b0c8 (`characterComeBackStep` exists; no shipped KillObject / DestroyProp target reaches it but the player's mech). blocks: SCRIPT-S264. uses: KillObject, DestroyProp. evidence: [script-natives-world-death.md §2026-10-19T14:04:25](../../re/script-natives-world-death.md#ts-20261019-140425).
- [ ] SCRIPT-327 — The player's branch of the tail: 0x2b96c0, 0x26d1f0 for the camera focus, DEAD's update for the focus (0x19c574, 0x18226c), 0x21e700 with the records at 0x5187cc. blocks: SCRIPT-S264, STUD-26. uses: KillObject, DestroyProp. evidence: same entry.
- [ ] SCRIPT-328 — Unread helpers of the tail: 0x22471c (`AIMillAbout`), 0x22f03c, 0x2246c0, 0x215988, 0x22ac74, 0x2c35b0, the event *0x3775ec, the engaged tables 0x58deb0 / 0x58dec8 (no port counterpart), the writers of 0x3771d7 / 0x377540, the order of 0x225f5c in 0x1835d0, message 8 to a used object of kind 171, the release by owner 0x182270. blocks: SCRIPT-S264, SCRIPT-S268. uses: KillObject, DestroyProp. evidence: same entry.
- [ ] SCRIPT-323 — Unread details: the character sound table `*0x375f7c` behind record +0x2c2 (character table byte +0x21), the unit of the clock 0x241098, the texture copy 0x270ae0 / 0x22ad0c, the head attach 0x22690c / 0x2268f4, the line test filter of 0x298274, the weapons of 0x21e854 (0x22af80 / 0x22ae28), the writer of byte +0x12e bit 5 and kind 159's create. blocks: SCRIPT-S264, SCRIPT-S265. uses: KillObject, DestroyProp. evidence: same entry.

## 6. Findings summary

- The stud drop with count 0, the state DEAD and its spawn counter, the after-death 0x21e218: [script-natives-world-death.md §2026-10-19T14:04:25](../../re/script-natives-world-death.md#ts-20261019-140425).
- The mode switch, the minifig test, the bricks, the respawn tail, the fall-apart pool with its start, update, motion and release, and the shipped targets: [script-natives-world-death.md §2026-10-19T13:21:05](../../re/script-natives-world-death.md#ts-20261019-132105).
- The routine per kind that named the calls first: [script-natives-world.md §2026-10-19T10:02:26](../../re/script-natives-world.md#ts-20261019-100226).

## 7. Code and tests

- `src/runtime/character_death.*` (`characterDeathInput`, `characterDeathSteps`, `characterSmashBurst`, `characterStudDropRequest`), `src/runtime/character_respawn.*` (`characterAfterDeath`, `characterComeBackStep`), the drop through `game_stud_drops::LevelStudDrops`, `src/runtime/fall_apart.*` (`FallApartPool`, `fallApartStep`, `fallApartCarried`), glue `src/game_character_death.cpp` (`LevelSession::characterDeathMessage`, called from the destroy hook in `src/game_level_natives.cpp`), `src/game_fall_apart.*` (`LevelFallApart`: part models, textures, head and hat), `LevelCharacters::bodyPose`.
- CTest: `character-death`, `character-death-real-data`, `stud-drop`.
