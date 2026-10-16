# Script natives: progress, saves and studs (batch SCRIPT-27)

Handlers of the 21 natives of batch SCRIPT-27 and what they touch. Census and batches: [script-natives.md](script-natives.md). Items: [script-system-natives.md](../plans/features/script-system-natives.md) (SCRIPT-27, SCRIPT-61 to SCRIPT-65).

## Current state

- All 21 handlers of batch SCRIPT-27 are read; 5 run exactly (CountRacesWon, IsRacingCarAwarded, UnlockLevel, GetCharacterUnlocked, IsRedbrickBought), 16 as stand-ins. [§2026-10-14T14:41:55](#ts-20261014-144155)
- CountRacesWon is not blocking, and EnableStudsByGroup's second argument is not an enable flag (this corrects script-natives.md). [§2026-10-14T14:41:55](#ts-20261014-144155)
- Arguments sit at `[r1 + 8 i + 4]` (object at `[r1]`); a raw integer is read by `ldr`, a float by `vldr` + `vcvt`; all handlers return 1. [§2026-10-14T14:41:55](#ts-20261014-144155)
- The live save slot is at 0x51b6b0; abilities, levels, races, racing car, red bricks and character unlock bits sit at the slot offsets in the entry's table (+0x51e0, +0x51ee, +0x5200, +0x520c, +0x51b8, +0x51bb, +0x51bc, +0x51e3). [§2026-10-14T14:41:55](#ts-20261014-144155)
- The port state is `ProgressNativeState` (src/runtime/natives_progress.*); cheat flags are not modelled. [§2026-10-14T14:41:55](#ts-20261014-144155)
- Open: character table columns (+4, +0x22, +0x23 of 0x372e10), new-game slot defaults 0x1a25cc, the reward switch 0x210480 and the readers of ability, scanner, super brick, parachute and race state.


<a id="ts-20261014-144155"></a>

## 2026-10-14T14:41:55+02:00 — Batch 27 (SCRIPT-27): the 21 handlers read, 5 run exactly, 16 as stand-ins (llvm-objdump on game.arm.elf with `--triple=armv7a --mattr=+vfp3`, C++ runner tests; no emulator, no REA session)

[Corrects script-natives.md §2026-10-14T13:33](script-natives.md#ts-20261014-133339): CountRacesWon is not blocking (class A, no loop on a repeat call); EnableStudsByGroup's second argument is not an enable flag (the levels pass 6 or 8). Every handler address was checked against its registration: the handler word sits directly before the name string at 0x160d54 (EnableStudsByGroup), 0x161b44 (GiveReward), 0x161ae4 (UnlockAbility), 0x161464 (UnlockCharacter), 0x16272c (SetRaceInProgress), 0x16286c (AddStuds), 0x162144 (QuickSave), 0x16280c (AwardRacingCar), 0x162858 (CountRacesWon), 0x162820 (IsRacingCarAwarded), 0x162838 (SetRaceWon), 0x162348 (UnlockLevel), 0x162920 (DisableSuperBuiltItSave), 0x161ac4 (SetParachuteDeployHeight), 0x161af8 (LockAbility), 0x161b68 (EnableSuperBricks), 0x162a1c (GetCharacterUnlocked), 0x1629b4 (GetLanguage), 0x160558 (GetPlayerNumber), 0x16231c (IsRedbrickBought), 0x1627b4 (UnlockScanner).

### Argument array and conversion

As in batch 26: an argument is at `[r1 + 8 i + 4]` (value pointer), so a single value is at `[r1 + 4]`, a second at `[r1 + 0xc]`; an object argument is at `[r1]`. A handler that does `ldr r0,[r0]` reads the raw integer word (UnlockAbility, LockAbility, SetRaceWon, SetRaceInProgress, GetCharacterUnlocked, IsRedbrickBought, UnlockCharacter); `vldr` + `vcvt.s32.f32` / `vcvt.u32.f32` reads a float (EnableStudsByGroup, GiveReward, AddStuds unsigned, EnableSuperBricks, DisableSuperBuiltItSave). Both give the truncated number of a script value in the port. All handlers return 1.

### Slot bytes (live slot at 0x51b6b0, file offsets as in [save-format.md](save-format.md))

| Offset | Pool word | Used by |
|---|---|---|
| +0x51e0 | 0x520890 | UnlockLevel: 0x1b2098(level sxtb, 0) sets bit 2*level (unsigned index, no bound) |
| +0x51ee | 0x52089e | UnlockAbility sets bit n, LockAbility clears it (index `n >> 3` unsigned, bit `n & 7`); the old note "64-bit field" of save-format.md is this bit set |
| +0x5200 | 0x51b6b0 + 0x5200 | SetRaceWon n: bit n-2; if it was clear, set it and call 0x21cf38(0). CountRacesWon: float sum of bits 0..6 of the first byte |
| +0x520c | 0x5206b0 + 0x20c | AwardRacingCar stores the byte 1; IsRacingCarAwarded tests bit 0 |
| +0x51b8 | 0x51b6b0 + 0x5000 + 0x1b8 | IsRedbrickBought n: bit n (cheat flag 0x375961 bit 3 gives true). Same set as the second 21-bit set of the completion |
| +0x51bb | 0x52086b | UnlockCharacter: group bits of ids 9, 19, 29, 39, 49, 59, 69 (bits 0..6) |
| +0x51bc | 0x52086c | characters, 2 bits per id; GetCharacterUnlocked via 0x213444 |
| +0x51e3 | 0x520893 | id set, written by 0x205d5c(41, 0) from AwardRacingCar: if bit 41 is clear, set bit 41 of the set 64 bytes after (+0x5223); then set bit 2*41-2 = 80 |

0x213444(id): ids 1..8 first: 8 is true, 1..7 become the bytes at 0x3741b4.. = 9, 19, 29, 39, 49, 59, 69 and the test goes on; the cheat flags 0x375961 / 0x375962 bit 0 give true; else id < 141 and bit 2*id of +0x51bc.

### Handlers

| Native | Handler | Effect |
|---|---|---|
| UnlockCharacter | 0x263918 | n = raw integer. Group byte as above. For n in {9, 16, 19, 29, 39, 49, 59, 69} and not unlocked (0x213444): 0x231fc0(n & 255) = 0x232028(n, 0) (unlock bit; the first time for an id but 9 also bit n of the "new" set at +0x5211), plus bit 2n+1 when character table field +4 is 0, then recursion into the ids of table bytes +0x22 and +0x23 (table pointer at 0x372e10, stride 0x28, filled at run time); then a loop over 20 halfwords at 0x5163e8+0x11a: for entries equal to n, the halfword counter at 0x518024+0x30 is incremented and 0x205c18(n, 1, 0, 4.0) shows the unlock; then, unless n == 16, a search of the 10 bytes at 0x51b24c for n, a name `Player1.Player0%u` built from the index (0x2501bc), its object lookup (0x23b350) and the message 0x22dbcc(entity, n, 0). Not ported: SCRIPT-61. |
| GiveReward | 0x1a2f18 | 0x217b48(n & 255) = 0x210480(n, 0) (switch over rewards 3..16, stores n in the byte 0x51527c+0x18); for n 1 or 2: 0x27bae8 (a `bx lr`; the two preceding words at 0x1a2f64 / 0x1a2f6c / 0x1a2f70 are `nop`) and 0x1d9720: byte +0x81f (collected) of the current level block +1, capped by byte +0x40 of the level table row; then 0x21cf38(0). Not ported: SCRIPT-62. |
| QuickSave | 0x194f64 | 0x21cf38(0): byte 0x5163e8+0x34 = 0, 0x17caf8, for a level id <= 8 0x229740 and 0x192f60, then the request 0x2400a0(0x1a5248, 0x261a1c, 0). Not ported: SCRIPT-63. |
| AddStuds | 0x18838c | 0x1584e0(0x519760, 1, count u32): byte +0x40 = 1, bits 2..4 of 0x5206b0+0x20b, the 64-bit count at +0x50 += count (no clamp, unlike 0x28a128), +0x44 = 0x241098(), and when 0x3757a4 bit 2 is set a call 0x240fc0(0.0). |
| EnableStudsByGroup | 0x285ed4 | 0x29693c(group & 255, value & 255, 0, 0): entry 0 of the table at 0x58f710 (16 bytes each); when its byte 0 is 0 it stores group, 0, value (+2), 0 (+3) and the index of the first stud record of that group (+8). The per-frame sequencer 0x28a2cc enables the studs. Not ported: STUD-4. |
| SetRaceInProgress | 0x27bff4 | 0x2ae0e0(arg != 0): calls 0x278d48(entry, flag) for each of the `[0x529c18+0x12c]` registered entries, then byte 0x529c18+0x130 = flag. |
| SetParachuteDeployHeight | 0x286040 | float at 0x3773b8 + 0x4c = argument. |
| EnableSuperBricks | 0x27bc08 | byte 0x3743f0+2 = (int != 0). |
| DisableSuperBuiltItSave | 0x2b4b78 | 0x26d530(obj, int & 255): component (obj+0x7c) byte +0x92 bit 3 = bit 0 of the argument. |
| UnlockScanner | 0x1cdeb0 | 0x1b1f78: byte 0x519e24+0x476 = 1, then UI calls 0x24d08c, 0x2134c8 and 0x245b40 (button visuals). |
| GetLanguage | 0x1b241c | 0x22b650 (the language byte at 0x376adc, 1 English, 2 French, 3 Italian, 4 German, 5 Spanish, 6 Dutch, 7 Danish, 8 Portuguese, 9 Russian, 10 Polish, 11 Japanese, 12 Korean) selects `Language_<name>` via 0x29e2ec; 0x233b80 reads that level parameter (default -1, also for language 0 and above 12); the result is converted as a signed byte and written as a float. |
| GetPlayerNumber | 0x263618 | 1.0 when the words at 0x3772c0 and 0x3772b8 are equal, else 0.0 (both are run-time globals, zero in the file). |

### Port

`ProgressNativeState` (src/runtime/natives_progress.*) holds the slot as a byte image (loaded from `--save` through `SlotStart::slotBytes`, zero without a save) and runs the table above; `ScriptRunner::progressNative` reads the arguments, writes the output variable and keeps `levelUnlocked` in step with UnlockLevel. `StudField::addToCounter` takes AddStuds. Exact (class Implemented): CountRacesWon, IsRacingCarAwarded, UnlockLevel, GetCharacterUnlocked, IsRedbrickBought. Cheat flags are not modelled (always clear). Tests: `natives-progress` (values above), `natives-progress-real-data` (lvl_01 scripts found by their calls: abilities 46..49, UnlockLevel 3 with GiveReward 1, GetCharacterUnlocked / UnlockCharacter 16, SetRaceWon 2 with CountRacesWon, EnableStudsByGroup 2 8, UnlockScanner, DisableSuperBuiltItSave, GetLanguage with Language_Spanish = 2, GetPlayerNumber).

Unknowns: the character table columns behind fields +4, +0x22, +0x23 of 0x372e10; the slot defaults 0x1a25cc (new-game image); what 0x3772c0 and 0x3772b8 hold; the reward switch of 0x210480; the readers of the ability, scanner, super brick, parachute and race-in-progress state.
