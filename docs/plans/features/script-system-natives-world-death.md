# Script system: the death of a character

## 1. Scope

Message 1 (destroy) of the character handler 0x18e4c8 for kinds 27, 121 and 242, as DestroyProp and KillObject (and every other caller of the entity destroy 0x23cd80) reach it: the `DisableFallapart` modes, the fall-apart body with its pool, the bricks, the stud drop and the respawn tail. The destroy itself and the other kinds are in [script-system-natives-world.md](script-system-natives-world.md); the player's own death and respawn is PLAYER-11 in [player-movement.md](player-movement.md), dropped studs are STUD-11 in [studs.md](studs.md).

## 2. Status

IN_PROGRESS, updated 2026-10-19T13:23:58+02:00. The handler path, the fall-apart pool and the part motion are read and ported: a killed goon of lvl_02 falls apart into the BrokenHead / Body / Arm / Legs or Hips parts of `levels/charbits.blvl`, which fly, bounce, blink and vanish after 64 updates, with sound 548. The stud drop, the respawn tail and the character's own sound are not ported.

## 3. Verified behaviour

- The mode switch of 0x18e93c (four modes, the minifig test 0x22716c, the brick branch, the respawn tail) is pinned by `character-death` on synthetic inputs. evidence: [script-natives-world-death.md §2026-10-19T13:21:05](../../re/script-natives-world-death.md#ts-20261019-132105).
- The five KillObject targets of lvl_02 (char_ai_prisoner01..03, kind 27, `CharacterEnum` 142) fall apart and stay dead (00_GoonFaller with its instance `AIRespawnIfFriendly` 0 too); the mech 6-9_PlayerMech of lvl_05 (kind 242) vanishes without bricks; the part objects and meshes of `levels/charbits.blvl` and the body part's bounds are pinned by `character-death-real-data`. evidence: same entry.
- The pool (four slots, a fifth start fails), the start placement (head 0.6 below the node, the other parts 0.8 above), the start velocity and spin ranges, the motion without and with a ground hit (rest rule, reflection with 0.5, gravity 0.075) and the lifetime (32 flying updates, blinking on an even clock, free on the 64th update) are pinned by `character-death`. evidence: same entry.
- In `chase-game`, `--call-script 2-4_AutoArrestA01` in lvl_02 logs the fall-apart and plays sound 548 (manual run, 2026-10-19).

## 4. Stand-ins

- SCRIPT-S264 The death of a character in the port: byte +0x12e bit 5 and the global 0x22b0b4 count as clear and the player branch is never taken (the player is no placed character); a character that comes back (friendly respawn) stays hidden; no stud drop, no 0x21e854 (weapons and attachment entities), no character sound 0x21e734, no 0x21e700 / 0x21e218; the sound 548 and the bricks start at the node position (0x23fb78 and the entity box not read). needs: SCRIPT-321, SCRIPT-322, SCRIPT-323. uses: KillObject, DestroyProp. evidence: [script-natives-world-death.md §2026-10-19T13:21:05](../../re/script-natives-world-death.md#ts-20261019-132105).
- SCRIPT-S265 The fall-apart body in the port: the blink clock 0x241098 is the game update count, the line test 0x21ab20 of 0x298274 is the wheel ray of the collision world (as VEH-S104), the parts take the class's `UpperTexture` / `LowerTexture` instead of the texture found on the character's node, the head part shows the character's head model, face and hat with their offset from the character at death (0x22690c / 0x2268f4 not read); the white node colour and the release by owner 0x182270 are not ported. needs: SCRIPT-323. uses: KillObject, DestroyProp. evidence: same entry.

## 5. Open items

- [ ] SCRIPT-321 — The stud drop of a character that stays dead: 0x21e2e4 mode 1 (mode 2 when the random word is a multiple of three) with count 0 and the argument 30 of 0x19913c, mode 0 with count 30 when byte +0x12e bit 5 is set. needs: STUD-11. blocks: SCRIPT-S264. uses: KillObject, DestroyProp. evidence: [script-natives-world-death.md §2026-10-19T13:21:05](../../re/script-natives-world-death.md#ts-20261019-132105).
- [ ] SCRIPT-322 — The respawn tail: the comes-back branch (player, 0x22b0b4 with its writer, `AIFriendly` with `AIRespawnIfFriendly`: enable, node flags 0x23f734, state 11), the AI after-death 0x21e218 (spawn point record +0xac, 0x22471c, 0x21decc), 0x21e700 (the records at 0x5187cc), 0x2b96c0 and 0x26d1f0 for the player, message 8 to a used object of kind 171, and the release by owner 0x182270 (caller 0x1d4c50). blocks: SCRIPT-S264. uses: KillObject, DestroyProp. evidence: same entry.
- [ ] SCRIPT-323 — Unread details: the character sound table `*0x375f7c` behind record +0x2c2 (character table byte +0x21), the unit of the clock 0x241098, the texture copy 0x270ae0 / 0x22ad0c, the head attach 0x22690c / 0x2268f4, the line test filter of 0x298274, the weapons of 0x21e854 (0x22af80 / 0x22ae28), the writer of byte +0x12e bit 5 and kind 159's create. blocks: SCRIPT-S264, SCRIPT-S265. uses: KillObject, DestroyProp. evidence: same entry.

## 6. Findings summary

- The mode switch, the minifig test, the bricks, the respawn tail, the fall-apart pool with its start, update, motion and release, and the shipped targets: [script-natives-world-death.md §2026-10-19T13:21:05](../../re/script-natives-world-death.md#ts-20261019-132105).
- The routine per kind that named the calls first: [script-natives-world.md §2026-10-19T10:02:26](../../re/script-natives-world.md#ts-20261019-100226).

## 7. Code and tests

- `src/runtime/character_death.*` (`characterDeathInput`, `characterDeathSteps`, `characterSmashBurst`), `src/runtime/fall_apart.*` (`FallApartPool`, `fallApartStep`, `fallApartCarried`), glue `src/game_character_death.cpp` (`LevelSession::characterDeathMessage`, called from the destroy hook in `src/game_level_natives.cpp`), `src/game_fall_apart.*` (`LevelFallApart`: part models, textures, head and hat), `LevelCharacters::bodyPose`.
- CTest: `character-death`, `character-death-real-data`.
