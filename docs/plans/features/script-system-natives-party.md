# Script system: allegiance, weapon, boss and mech natives

## 1. Scope

The 18 natives of batch SCRIPT-35: SwapAllegiance, SetWeaponEnabled, SetPartyCharacter ([script-system-natives-world.md](script-system-natives-world.md)), CharSetAllowVehicleEntry, SuperFreeplaySwap, RelockChasePlainClothes, CharSetSkydiveMode, FireBeamWeapon, getinmech, GetOutMech, the boss hooks of the Walnuts, Rex and 1-8 missions, HACK_PlayDowntownIntro and UsePet. The other natives: [script-system-natives.md](script-system-natives.md).

## 2. Status

IN_PROGRESS, updated 2026-10-19T15:53:02+02:00. All 18 handlers are read. UsePet (empty handler) and HACK_PlayDowntownIntro run; the other 15 record their effect in `PartyNativeState` (`src/runtime/natives_party.*`) because the systems that read the written fields are not ported (AI core, weapons, mechs, boss components). The transition natives CharSetAllowVehicleEntry and RelockChasePlainClothes are among them (TRANS-5).

## 3. Verified behaviour

- Call counts of the 17 natives in the shipped levels equal the census (66, 36, 4, 4, 3, 3, 2, 2, 1, ...) and every call has the arguments the handler reads. evidence: [script-natives-party.md §2026-10-19T15:53:02](../../re/script-natives-party.md#ts-20261019-155302).
- HACK_PlayDowntownIntro writes 1.0 once when level 8 is unlocked and its StoryProgress is at most 1; SetWeaponEnabled's hand choice and the player bytes, the requested states of the boss hooks, the swap record of RelockChasePlainClothes follow the handlers. CTest `natives-party`, `natives-party-real-data`.

## 4. Stand-ins

- SCRIPT-S277 SwapAllegiance (toggle count only; the initial bit 2 of the object flags, the lists 0x58deb0 / 0x58dec8 and the grid registration 0x230e60 are missing), SetWeaponEnabled (hand A / B and the player bytes recorded), CharSetAllowVehicleEntry (flag recorded, the boarding gate stays open), SuperFreeplaySwap and RelockChasePlainClothes (swap record, no swap, no effects), CharSetSkydiveMode (state 129 / 1 and the value for 0x376910 recorded; `kFloorNormalMin` stays a constant) and FireBeamWeapon (arguments recorded) only record. needs: SCRIPT-338. uses: SwapAllegiance, SetWeaponEnabled, CharSetAllowVehicleEntry, SuperFreeplaySwap, RelockChasePlainClothes, CharSetSkydiveMode. evidence: [script-natives-party.md §2026-10-19T15:53:02](../../re/script-natives-party.md#ts-20261019-155302).
- SCRIPT-S278 Walnuts_Hit, walnuts_startphase2, 1-8_RejoinFight, 1-8_runfromarrest, Rex_AxeSmash_Done and Rex_GrapplePull_Done store the requested state of the boss component (5, 2, 6, 2, 6, 8) per object name; the components and their states do not exist. needs: SCRIPT-339. uses: Walnuts_Hit, walnuts_startphase2, 1-8_RejoinFight, 1-8_runfromarrest, Rex_AxeSmash_Done, Rex_GrapplePull_Done. evidence: [script-natives-party.md §2026-10-19T15:53:02](../../re/script-natives-party.md#ts-20261019-155302).
- SCRIPT-S279 getinmech and GetOutMech store and clear the pilot of a mech by name; no node attach, state or visibility change. needs: SCRIPT-340. uses: getinmech, GetOutMech. evidence: [script-natives-party.md §2026-10-19T15:53:02](../../re/script-natives-party.md#ts-20261019-155302).
- SCRIPT-S280 HACK_PlayDowntownIntro keeps the played byte (original `0x5163e8 + 0xbd`, global to the session) in the runner, so a new level start plays it again; the cheat flag 0x375961 bit 2 that also unlocks level 8 does not exist. needs: SAVE-12. uses: HACK_PlayDowntownIntro. evidence: [script-natives-party.md §2026-10-19T15:53:02](../../re/script-natives-party.md#ts-20261019-155302).

## 5. Open items

- [ ] SCRIPT-338 — Readers of the character records: the allegiance bit and lists (0x2b6b5c, 0x230e60), the weapon hands 0x22ae28 / 0x22af80 and bytes 0x375f04 to 0x375f06, the vehicle-entry bit (ctrl+0xe0+0x131 bit 2, boarding gate VEH-S10), the swap record 0x5163e8+0xc8 with effects 7 and 0xf1, skydive state 129 and the floor normal 0x376910, the beam 0x1b72f4. needs: SCRIPT-42, VEH-19. blocks: SCRIPT-S277. evidence: [script-natives-party.md §2026-10-19T15:53:02](../../re/script-natives-party.md#ts-20261019-155302).
- [ ] SCRIPT-339 — Boss components of Walnuts (1-8), Rex and the 1-8 arrest scene: their state machines for the requested states 2, 5, 6, 8 and the bits set by 1-8_runfromarrest (+0xda). blocks: SCRIPT-S278. evidence: [script-natives-party.md §2026-10-19T15:53:02](../../re/script-natives-party.md#ts-20261019-155302).
- [ ] SCRIPT-340 — Mech piloting: the rest of 0x1577c4 / 0x15a458 (node attach 0x245048 / 0x245008, state install 0x2414b4, player hand-over) and the mech vehicle. needs: VEH-5. blocks: SCRIPT-S279. evidence: [script-natives-party.md §2026-10-19T15:53:02](../../re/script-natives-party.md#ts-20261019-155302).

## 6. Findings summary

- Handlers of batch 35, argument slot layout, level block of level 8 for StoryProgress: [script-natives-party.md §2026-10-19T15:53:02](../../re/script-natives-party.md#ts-20261019-155302).

## 7. Code and tests

- `PartyNativeState` (`src/runtime/natives_party.*`), `ScriptRunner::partyNative` (`src/runtime/script_runner_natives.cpp`). CTest: `natives-party` (synthetic, provenance in the header), `natives-party-real-data` (argument counts and call totals in the shipped levels).
