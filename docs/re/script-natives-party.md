# Script natives: allegiance, weapons, party, mech and boss hooks (batch 35)

Handlers of the 18 natives of batch SCRIPT-35 (census batch 35): what each reads and writes, what the port does with it. Census and the other batches: [script-natives.md](script-natives.md), table: [script-natives-census.md](script-natives-census.md). Items: [script-system-natives-party.md](../plans/features/script-system-natives-party.md) (SCRIPT-338 to SCRIPT-340, SCRIPT-S277 to SCRIPT-S280).

## Current state

- All 18 handlers are read; every one returns 1 (none blocks). Argument slots are {object word, value pointer} 8 bytes apart (object arguments at +0, +8, value pointers at +4, +0xc, +0x14, +0x1c). [§2026-10-19T15:53:02](#ts-20261019-155302)
- Six of the natives are one-line setters of the requested-state halfword at `[entity + 0x7c] + 4` of a boss component: Walnuts_Hit 5, walnuts_startphase2 2, 1-8_RejoinFight 6, 1-8_runfromarrest 2, Rex_AxeSmash_Done 6, Rex_GrapplePull_Done 8. [§2026-10-19T15:53:02](#ts-20261019-155302)
- SwapAllegiance toggles bit 2 of the object's flag word `+0xc` and moves the object between two 6-slot lists (0x58deb0 with the bit set, 0x58dec8 clear), then re-registers it (0x230e60). SetWeaponEnabled picks hand A (value 2.0) or hand B (other non-zero values), or turns both off; the player only sets the bytes 0x375f04 to 0x375f06. [§2026-10-19T15:53:02](#ts-20261019-155302)
- SetPartyCharacter was read in [script-natives-world.md](script-natives-world.md); UsePet 0x172184 is an empty handler. HACK_PlayDowntownIntro writes 1.0 once when level 8 is unlocked and its StoryProgress is at most 1. [§2026-10-19T15:53:02](#ts-20261019-155302)
- Port: `PartyNativeState` (`src/runtime/natives_party.*`) records the effects; UsePet and HACK_PlayDowntownIntro are transcribed, the other 15 are stand-ins. [§2026-10-19T15:53:02](#ts-20261019-155302)

<a id="ts-20261019-155302"></a>

## 2026-10-19T15:53:02+02:00 — Handlers of the 17 remaining natives of batch 35

Evidence: llvm-objdump (`--triple=armv6k --mattr=+vfp2`) of `artifacts/game/game.arm.elf`, level data (`parseLevel`, call counts equal the census for all 17); no emulator run, no REA session. SetPartyCharacter (28 calls) was read before ([script-natives-world.md](script-natives-world.md)).

| Native | Handler | Reads and writes |
|---|---|---|
| SwapAllegiance | 0x1da88c | object word `obj`; 0x2b6b5c(obj) toggles bit 2 of `[obj+0xc]` and moves `obj` between the 6-slot lists 0x58deb0 (bit set) and 0x58dec8 (clear, first free slot); then 0x230ef4 / 0x230e60(obj, new bit, 1) re-registers it with the grid objects (0x1fce14, 0x1fcc68; list removal 0xa0 count). The initial bit comes from the level object flags (not read). |
| SetWeaponEnabled | 0x26fea4 | entity (0x22dacc), float v. v = 0: 0x22ae28(e, 0, 0) and 0x22af80(e, 0, 0). v != 0 and e is the player (0x3772c0): bytes 0x375f04 = 1, 0x375f05 = 1, 0x375f06 = (2.0 < v), nothing else. v != 0, other entity: v = 2.0 gives 0x22ae28(e, 1, 0), otherwise 0x22af80(e, 1, 0). 0x22ae28 / 0x22af80 equip or unequip the item at component +0x144 / +0x148 (hand A / B: message 8, 0x2428c0, byte +0x2d0 = 1, `[ctrl+0xe0]+0x126` = 0). |
| CharSetAllowVehicleEntry | 0x29ea3c | entity, int flag: bit 2 of byte `[[entity+0x7c]+0xe0]+0x131` = flag != 0. That bit is the "may enter a vehicle" gate of [vehicles-system.md](vehicles-system.md) (0x322a04, prompt 0x1835d0). |
| SuperFreeplaySwap | 0x27c0ac | entity, int id: `[0x5163e8+0xc8]` = entity, u16 `[0x5163e8+0xcc]` = id; when the entity is the player (0x3772b8) the effects 7 and 0xf1 are spawned at its node (0x225248). The reader of the record is not found. |
| RelockChasePlainClothes | 0x2b4bac | 0x23b388(16) takes character 16 out of the party; when byte 4 of the party manager 0x51b248 is 16 the record 0x5163e8+0xc8 = `[0x51b248+0x2c]` and +0xcc = 9 (swap back to Chase). |
| Walnuts_Hit / walnuts_startphase2 | 0x1b2828 / 0x291730 | argument word as entity: halfword `[[e+0x7c]+4]` = 5 (0x16458c) / 2 (0x1c6914). |
| 1-8_RejoinFight / 1-8_runfromarrest | 0x26353c / 0x27bb4c | same halfword = 6 (0x25e0bc) / 2 (0x1fcc40); runfromarrest also sets bits 0 to 2 of byte +0xda of the component of `[[e+0x7c]+0xc]` to 1. |
| Rex_AxeSmash_Done / Rex_GrapplePull_Done | 0x27bdb8 / 0x29ea28 | same halfword = 6 (0x1d4030) / 8 (0x275ac4). |
| getinmech / GetOutMech | 0x194e7c / 0x1a2f04 | getinmech(mech, pilot): 0x1577c4 stores the pilot at `[mech+0x7c]+0x348`, attaches its node (0x245048, 0x245008), clears bit 9 of its halfword +0x10 and installs a state with 0x2414b4(pilot, ctrl+0x50, 1, 0). GetOutMech(mech): 0x15a458 clears `+0x348` and detaches. The rest of both is not read. |
| CharSetSkydiveMode | 0x1da6e4 | entity, int flag: 0x2414b4(e, `[e+0x7c]+0x50`, 129 or 1, 0), then the float at 0x376910 = 0.202 (0x3e4ed917) for flag != 0, else 0.707 (0x3f350108). 0x376910 is `kFloorNormalMin` of the collision (0.6088 at level start, [collision.md](collision.md)). |
| FireBeamWeapon | 0x1d9cb4 | object word, int id (`& 0xff`), two floats: 0x1b72f4(obj, id, 1, f1, f2) builds a beam from the object's matrix (0x212980, 0x23b92c); the rest is not read. |
| HACK_PlayDowntownIntro | 0x2ad87c | output variable: 1.0 when 0x21d450(8, 0) (level 8 unlocked, [save-format-state.md](save-format-state.md)), the u16 `StoryProgress` of the level block of level 8 (0x203464(8) + 0x1c) is at most 1 and the byte `0x5163e8 + 0xbd` is 0; the byte is then set to 1; otherwise 0.0. The cheat flag 0x375961 bit 2 also unlocks the level. |
| UsePet | 0x172184 | `mov r0, #1; bx lr`: no effect. |

Slot image offsets: the level block of level 8 is at slot offset 0x30c8, so `StoryProgress` is the u16 at 0x30e4 (formula `0x8 + (L - 2) * 0x820` of [save-format.md](save-format.md)).

Unknowns:
- Who reads the allegiance lists 0x58deb0 / 0x58dec8, the record 0x5163e8+0xc8, the bytes 0x375f04 to 0x375f06 and the byte 0x5163e8+0xbd.
- The initial bit 2 of the object flag word and the state numbers 129 / 1 as player states.
- The boss components (Walnuts, Rex, 1-8) and what their state numbers 2, 5, 6, 8 do.
- The remaining steps of getinmech and FireBeamWeapon.
