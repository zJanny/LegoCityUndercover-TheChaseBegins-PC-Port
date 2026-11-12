# Disguises

## 1. Scope

Chase's disguises: the eight character classes (police, robber, construction, farmer, astronaut, miner, fireman, civilian) with their abilities, the party of player characters, the swap between them (D-pad, lower screen, scripts, swap pad), how a disguise is given and saved, and the class look of the HUD. The abilities themselves (scanner, detective tool, grapple and so on) have their own feature files; the script natives are in [script-system-natives-world.md](script-system-natives-world.md), [script-system-natives-state.md](script-system-natives-state.md) and [script-system-natives-party.md](script-system-natives-party.md).

## 2. Status

IN_PROGRESS, updated 2026-11-12T11:23:21+01:00. The party manager is ported (`party_manager.*`): chase-game sets the party up at each level load from the slot and the level's TablesLevels row, and SetPartyCharacter, UnlockCharacter and LockCharacter go through it. A swap to another member changes the party's player character but not the player entity, which stays Chase (character 9) in the police class (DISG-S1). No mission up to 1-4 swaps the character.

## 3. Verified behaviour

- The party manager's rules are transcribed from the code and pinned in CTest `party-manager` (not compared with the oracle): the list of the level set-up with the row's extras and the slot's own party, the add, removal, lookup, cycle, the swap's bookkeeping (effect gate of 15 updates, stand-for ids), SetPartyCharacter (a removed member is swapped to), UnlockCharacter's add and LockCharacter's removal.
- A story level of a new game starts with eight slots (9, 19, 29, 39, 49, 59, 69, 80), only Chase usable, walk count 8; chase-game prints the party at the level start.

## 4. Stand-ins

- DISG-S1 A swap the party manager carries out (SetPartyCharacter to another member, the cycle of a removal) changes its player character, member and character byte and prints `party swap to slot ...: player entity swap not ported`; the player entity is not swapped (model, abilities, state and carried thing stay Chase's), effects 7 and 241 are not spawned and the member scripts are not run. needs: DISG-9. evidence: [disguise.md §2026-11-12T11:17](../../re/disguise.md#ts-20261112-111720)
- DISG-S2 Global bytes whose writers are not read are taken as clear: the "party kept" byte (the party is set up at every level load) and the byte that lets the cycle take its start slot; no `Player2` members exist; where the cycle reads a byte outside the manager (the player's character has no slot) the port reads 0. needs: DISG-10. evidence: [disguise.md §2026-11-12T11:17](../../re/disguise.md#ts-20261112-111720)
- Related stand-ins of other features: SCRIPT-S234 (SetPartyCharacter) in [script-system-natives-world.md](script-system-natives-world.md), SCRIPT-S21 (UnlockCharacter) in [script-system-natives-state.md](script-system-natives-state.md).

## 5. Open items

Ranked by use in the missions (first the ones the early missions touch).

- [ ] DISG-1 — Class look of the HUD and the lower screen: the heart background, portrait and portrait border follow the class of the player character (one loaded image per class); the port shows the police look fixed, which is right up to mission 1-4. The reader that picks the entry and the locked portrait are not read. uses: ShowCharacterIntro. evidence: [disguise.md §2026-11-12T09:58](../../re/disguise.md#ts-20261112-095806)
- [ ] DISG-9 — Swap of the player entity: the member `Player1.Player<n>` becomes the player entity with heading, state and carried thing handed over, effects 7 and 241, the member's `startup` / `OnSceneEnter<n>` scripts, and chase-game showing and using the new character (model, class abilities, HUD look DISG-1). blocks: DISG-S1, SCRIPT-S234, DISG-3, DISG-5, DISG-7, DISG-8. uses: SetPartyCharacter, RelockChasePlainClothes. evidence: [disguise.md §2026-11-12T11:17](../../re/disguise.md#ts-20261112-111720)
- [ ] DISG-10 — Read the writers of the two global bytes of DISG-S2 and whether `Player2.Player%02d` members exist (a second instance of `levels/players.blvl`). blocks: DISG-S2. evidence: [disguise.md §2026-11-12T11:17](../../re/disguise.md#ts-20261112-111720)
- [ ] DISG-3 — The party in the save and at level entry: UnlockCharacter's add and the set-up from the slot's own party are ported; not ported are the two writers that copy the party ids into the slot and mark it as the slot's own party, and the level entry swap to the character stored in the session (its writer not read). needs: DISG-9. uses: UnlockCharacter, LockCharacter. evidence: [disguise.md §2026-11-12T11:17](../../re/disguise.md#ts-20261112-111720)
- [ ] DISG-5 — Swap by D-pad: right next, left previous, with a request word and a 5-update countdown, the cycle with its skip rules and the swap gate; the gate's state tests, the third pad channel and the per-slot bytes it reads are not read. Needs a key binding for D-pad left and right on foot (input mapping, see the input feature file). needs: DISG-9. evidence: [disguise.md §2026-11-12T09:58](../../re/disguise.md#ts-20261112-095806)
- [ ] DISG-6 — Class abilities: name each ability number of the class sets (listed per class in the entry) and link it to its object classes (`props/ability_*`, 78 classes) and player states; sheet 2 bytes +4 and +7. evidence: [disguise.md §2026-11-12T09:58](../../re/disguise.md#ts-20261112-095806)
- [ ] DISG-7 — Swap on the lower screen: the portrait code (touch areas, storyboards, borders, `Locked.png`) is not read beyond its swap call. Used by clicking with the mouse. needs: DISG-9. evidence: [disguise.md §2026-11-12T09:58](../../re/disguise.md#ts-20261112-095806)
- [ ] DISG-8 — Other ways to change character: the character swap pad (object kind 126, player states 234 to 236), the freeplay swap (pending record, rebuild of the entity from `levels/chars/char_<name>.lvl`), the character shop (CharacterShop, `Use_CharShop`) and the character tokens (106 pickups); class names, handlers and effects not read; plain clothes (character 16) reach the party only through the freeplay swap. needs: DISG-9. uses: SuperFreeplaySwap, CharacterShop, pickups/pickup_charactertoken. evidence: [disguise.md §2026-11-12T09:58](../../re/disguise.md#ts-20261112-095806)

## 6. Findings summary

- Classes and ability sets, party manager, swap, D-pad and lower-screen requests, UnlockCharacter and the save byte, census of the eight main levels, what the missions up to 1-4 need: [disguise.md §2026-11-12T09:58](../../re/disguise.md#ts-20261112-095806).
- Party set-up at level start, the manager functions as ported, the disguise calls per mission: [disguise.md §2026-11-12T11:17](../../re/disguise.md#ts-20261112-111720).
- Read before: SetPartyCharacter ([script-natives-world.md §2026-10-18T11:01](../../re/script-natives-world.md#ts-20261018-110140)), the party and freeplay natives ([script-natives-party.md §2026-10-19T15:53](../../re/script-natives-party.md#ts-20261019-155302)), ability test, lock set and use markers by class ([use-spot.md §2026-10-20T21:58](../../re/use-spot.md#ts-20261020-215832)).

## 7. Code and tests

- `PartyManager` (`src/runtime/party_manager.*`), CTest `party-manager`; owned by `PartyNativeState` (`src/runtime/natives_party.*`), which the runner's SetPartyCharacter (`src/runtime/script_runner_natives.cpp`), UnlockCharacter and LockCharacter (`src/runtime/script_runner.cpp`) call. Set-up at level load: `game_save::setUpParty` (`src/game_save.cpp`) with the row's extras from `NewGameTables` (`src/save/new_game.*`); log lines in `src/game_level_natives.cpp`. Outcomes also in CTest `natives-world`.
- Related code: the HUD's fixed police look (`src/frontend/hud.cpp`).
