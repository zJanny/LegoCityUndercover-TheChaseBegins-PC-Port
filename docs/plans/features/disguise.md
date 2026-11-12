# Disguises

## 1. Scope

Chase's disguises: the eight character classes (police, robber, construction, farmer, astronaut, miner, fireman, civilian) with their abilities, the party of player characters, the swap between them (D-pad, lower screen, scripts, swap pad), how a disguise is given and saved, and the class look of the HUD. The abilities themselves (scanner, detective tool, grapple and so on) have their own feature files; the script natives are in [script-system-natives-world.md](script-system-natives-world.md), [script-system-natives-state.md](script-system-natives-state.md) and [script-system-natives-party.md](script-system-natives-party.md).

## 2. Status

NOT_STARTED, updated 2026-11-12T10:01:08+01:00. First static map of the original only; no disguise code in the port. The player is always character 9 (Chase in the police class); SetPartyCharacter with another character and UnlockCharacter are stand-ins of the script natives. No mission up to 1-4 swaps the character.

## 3. Verified behaviour

- None in the port. Read from the original (not run): the class table, the party manager, the swap and the ways to ask for it, see the findings summary.

## 4. Stand-ins

- None of this feature. Related stand-ins of other features: SCRIPT-S234 (SetPartyCharacter) in [script-system-natives-world.md](script-system-natives-world.md), SCRIPT-S21 (UnlockCharacter) in [script-system-natives-state.md](script-system-natives-state.md).

## 5. Open items

Ranked by use in the missions (first the ones the early missions touch).

- [ ] DISG-1 — Class look of the HUD and the lower screen: the heart background, portrait and portrait border follow the class of the player character (one loaded image per class); the port shows the police look fixed, which is right up to mission 1-4. The reader that picks the entry and the locked portrait are not read. uses: ShowCharacterIntro. evidence: [disguise.md §2026-11-12T09:58](../../re/disguise.md#ts-20261112-095806)
- [ ] DISG-2 — Party manager and swap: the member list (ids, skip flags, entities `Player01` to `Player16` of `levels/players.blvl`), adding and removing a member and the swap of the player entity (effects 7 and 241, member scripts, state and carried thing handed over). Not ported; how the members get their characters at level start is not read. blocks: DISG-3, DISG-5, DISG-7, DISG-8. uses: SetPartyCharacter, RelockChasePlainClothes. evidence: [disguise.md §2026-11-12T09:58](../../re/disguise.md#ts-20261112-095806)
- [ ] DISG-3 — Giving a disguise and the save: UnlockCharacter for the first row of a class sets the class bit of a save byte and adds the row to the party; the party ids of the save and the character restored at level entry are not ported, their writers not read. needs: DISG-2. uses: UnlockCharacter, LockCharacter. evidence: [disguise.md §2026-11-12T09:58](../../re/disguise.md#ts-20261112-095806)
- [ ] DISG-4 — Which mission gives and uses which disguise: decode the script calls with their arguments (SetPartyCharacter, UnlockCharacter, UnlockAbility, SuperFreeplaySwap) per mission; the Python binding has no script decoder with arguments. First by object name: plain clothes in mission 1-9, robber in mission 2-2. uses: SetPartyCharacter, UnlockCharacter, UnlockAbility. evidence: [disguise.md §2026-11-12T09:58](../../re/disguise.md#ts-20261112-095806)
- [ ] DISG-5 — Swap by D-pad: right next, left previous, with a request word and a 5-update countdown, the cycle with its skip rules and the swap gate; the gate's state tests, the third pad channel and the per-slot bytes it reads are not read. Needs a key binding for D-pad left and right on foot (input mapping, see the input feature file). needs: DISG-2. evidence: [disguise.md §2026-11-12T09:58](../../re/disguise.md#ts-20261112-095806)
- [ ] DISG-6 — Class abilities: name each ability number of the class sets (listed per class in the entry) and link it to its object classes (`props/ability_*`, 78 classes) and player states; sheet 2 bytes +4 and +7. evidence: [disguise.md §2026-11-12T09:58](../../re/disguise.md#ts-20261112-095806)
- [ ] DISG-7 — Swap on the lower screen: the portrait code (touch areas, storyboards, borders, `Locked.png`) is not read beyond its swap call. Used by clicking with the mouse. needs: DISG-2. evidence: [disguise.md §2026-11-12T09:58](../../re/disguise.md#ts-20261112-095806)
- [ ] DISG-8 — Other ways to change character: the character swap pad (object kind 126, player states 234 to 236), the freeplay swap (pending record, rebuild of the entity from `levels/chars/char_<name>.lvl`), the character shop (CharacterShop, `Use_CharShop`) and the character tokens (106 pickups); class names, handlers and effects not read. needs: DISG-2. uses: SuperFreeplaySwap, CharacterShop, pickups/pickup_charactertoken. evidence: [disguise.md §2026-11-12T09:58](../../re/disguise.md#ts-20261112-095806)

## 6. Findings summary

- Classes and ability sets, party manager, swap, D-pad and lower-screen requests, UnlockCharacter and the save byte, census of the eight main levels, what the missions up to 1-4 need: [disguise.md §2026-11-12T09:58](../../re/disguise.md#ts-20261112-095806).
- Read before: SetPartyCharacter ([script-natives-world.md §2026-10-18T11:01](../../re/script-natives-world.md#ts-20261018-110140)), the party and freeplay natives ([script-natives-party.md §2026-10-19T15:53](../../re/script-natives-party.md#ts-20261019-155302)), ability test, lock set and use markers by class ([use-spot.md §2026-10-20T21:58](../../re/use-spot.md#ts-20261020-215832)).

## 7. Code and tests

- None yet. Related code: `PartyNativeState` (`src/runtime/natives_party.*`), `ScriptRunner::takePartyRequests` (`src/runtime/natives_world.cpp`), the HUD's fixed police look (`src/frontend/hud.cpp`).
