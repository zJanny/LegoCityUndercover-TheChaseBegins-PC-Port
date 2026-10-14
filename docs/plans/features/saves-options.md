# Saves and options

## 1. Scope

`savegame.dat`: header, CRC, validation, atomic XDG storage, option fields, slot progress accessors and the completion percentage (0x1cd3dc), the level and character tables. The slot screens of the front end are [front-end.md](front-end.md).

## 2. Status

IN_PROGRESS, updated 2026-10-14T15:25:00+0200. `chase_save` reads, validates and writes safely and `chase-savecheck` reports completion; `chase-game --save FILE --slot N` starts the level of the slot (byte +6) at its Player1Start with the progress of the slot (saved level variables, unlock bits, game complete); `--level` and `--start` override. No game program writes a save.

## 3. Verified behaviour

- Header size and version sources, CRC, option defaults (0x2081cc), option fields (sound/music volume, invert camera, subtitles), language order and gift list recovered.
- Slot progress layout (character bits, level blocks), completion formula (divide 0x25c1b8 is unsigned), totals source `TablesSuperCounters` (0x2f63dc) matching the 13 x 7 totals layout, offline completion total 711 (`save-real-data`, `save-core`). No cheat flags in the completion.

- Object records of the saved state decode on the user's save (48 in level 2, 1 in level 1, all payloads 0, end bit set is zero padding); slot byte +6 is the current level (`slot-progress-real-data`).
- Level state block (0x820 bytes): length, StoryProgress, saved flag, state records; the level-data record (default parameters with flag bit 1) decodes exactly on the user's real save; unlock bits at slot +0x51e0; `GetGameComplete` and `IsLevelUnlocked` read the slot, `IsRedBrickExtraActive` reads a runtime mask that a load leaves 0. With the user's slot (mission 1-1 done) the bound enable bytes equal the oracle state `street-after-1-1` except the three SCRIPT-10 bounds (`slot-progress-real-data`).

## 4. Stand-ins

- SAVE-S1 An empty slot wrap is shown as empty by `chase-savecheck`.
- SAVE-S2 (closed 2026-10-14: `--save` starts the level of the slot, see SAVE-9.)
- SAVE-S4 Continue enters every level at its Player1Start (code shows no saved position; measured for level 1 only). needs: SAVE-13.
- SAVE-S3 The 1-byte object records (types 0x15, 0x43, 0x62) are decoded but not applied; the 4-byte ones set TASKS_COMPLETE. All payloads of the only real save are 0, so nothing differs. needs: SAVE-8.

## 5. Open items

- [x] SAVE-1 — closed 2026-10-14: `chase-game --save FILE --slot N` loads a slot through `chase_save_progress` and gives the script runner the saved variables of the level (StoryProgress, ...) and the unlock / complete state; replaces `--call-script 1-1_MissionSkip` for progress. evidence: [save-format.md §2026-10-14T11:53](../../re/save-format.md#ts-20261014-115300).
- [ ] SAVE-2 — Existing slots with time, percent and location, Continue and delete in the front end. blocks: FE-1.
- [ ] SAVE-3 — Compare `chase-savecheck` with the percentage the game shows (possible now with the oracle).
- [ ] SAVE-4 — Options +0 nibbles and +3 bits 2..5 are not understood; the options volume source in a loaded save. evidence: [save-format.md §2026-10-13T08:53](../../re/save-format.md#ts-20261013-0853).
- [x] SAVE-6 — closed 2026-10-14: object records decoded (`objectRecords`), classes with callbacks identified, end bit set shown to be never used (mode word always 2), 4-byte records applied as TASKS_COMPLETE. evidence: [save-format.md §2026-10-14T13:41](../../re/save-format.md#ts-20261014-134146)
- [x] SAVE-7 — closed 2026-10-14: Continue level = slot byte +6; no position field; `which` 1 is never written, so the StoryProgress override 0x192e14 never fires. evidence: [save-format.md §2026-10-14T13:41](../../re/save-format.md#ts-20261014-134146)
- [ ] SAVE-8 — Names and creator of the type 0xfc and 0x62 objects behind the record hashes, and the effect of type 0x15 / 0x43 / 0x62 records with bit 0 set (needs a save with built build-its). blocks: SAVE-S3. evidence: [save-format.md §2026-10-14T13:41](../../re/save-format.md#ts-20261014-134146)
- [x] SAVE-9 — closed 2026-10-14: Continue loads the level of slot byte +6 (0 becomes 2) through 0x2e7ce8, 0x2d5874 and 0x20a4d4 and spawns at Player1Start; `chase-game --save` does the same; oracle level 1 matches. evidence: [save-format.md §2026-10-14T15:19](../../re/save-format.md#ts-20261014-151903).
- [ ] SAVE-10 — No save writer: the save request 0x21cf38(0) (QuickSave, SetRaceWon, AwardRacingCar, GiveReward) and the slot copy to the file are not done; the progress natives change an in-memory slot image only. blocks: SCRIPT-63. evidence: [script-natives-progress.md §2026-10-14T14:41](../../re/script-natives-progress.md#ts-20261014-144155).
- [ ] SAVE-11 — New-game slot defaults (0x1a25cc: level 2 unlocked, characters 9..140, sets) are not reproduced; without `--save` the progress natives start from a zero image. evidence: [save-format.md §2026-10-13T09:13](../../re/save-format.md#ts-20261013-0913).
- [ ] SAVE-12 — Where the language byte 0x376adc comes from (options field) and its wiring into GetLanguage. blocks: SCRIPT-65. evidence: [script-natives-progress.md §2026-10-14T14:41](../../re/script-natives-progress.md#ts-20261014-144155).
- [ ] SAVE-13 — Continue at level 2 or later is not measured (heading, position, level scripts moving the player); needs a user save or state with slot byte +6 above 1. blocks: SAVE-S4. evidence: [save-format.md §2026-10-14T15:19](../../re/save-format.md#ts-20261014-151903).
- [ ] SAVE-5 — Most of what a save game contains: only the completion totals are understood.

## 6. Findings summary

- Layout, CRC, save thread: [save-format.md §2026-10-13T10:30](../../re/save-format.md#ts-20261013-1030).
- Options and defaults: [save-format.md §2026-10-13T12:30](../../re/save-format.md#ts-20261013-1230), [save-format.md §2026-10-13T08:53](../../re/save-format.md#ts-20261013-0853), [save-format.md §2026-10-13T08:56](../../re/save-format.md#ts-20261013-0856).
- Level state blocks, unlock bits, the progress natives and the user's save: [save-format.md §2026-10-14T11:53](../../re/save-format.md#ts-20261014-115300).
- Object records, end bit set, current level, second unlock bit: [save-format.md §2026-10-14T13:41](../../re/save-format.md#ts-20261014-134146).
- Slot bytes the progress natives write (abilities +0x51ee, races +0x5200, racing car +0x520c, red bricks +0x51b8, character group +0x51bb): [script-natives-progress.md §2026-10-14T14:41](../../re/script-natives-progress.md#ts-20261014-144155).
- Where Continue starts (level of byte +6, Player1Start): [save-format.md §2026-10-14T15:19](../../re/save-format.md#ts-20261014-151903).
- Slot progress, totals and completion: [save-format.md §2026-10-13T09:13](../../re/save-format.md#ts-20261013-0913), [save-format.md §2026-10-13T09:25](../../re/save-format.md#ts-20261013-0925), [save-format.md §2026-10-13T09:26](../../re/save-format.md#ts-20261013-092607).

## 7. Code and tests

- `src/save/` (`chase_save`, `chase_save_tables`), `chase-savecheck`, `chase-tabledump`.
- `src/save/continue_start.*` (`continueStart`), `game_save::chooseContinueLevel`, `GameOptions::levelGiven`.
- `src/save/slot_progress.*` (`chase_save_progress`), `src/game_save.hpp`, `ScriptRunner::setProgress` / `setVariable`, `SlotStart::slotBytes` to `ProgressNativeState::setSlotImage`.
- CTest: `game-save-continue`, `game-save-continue-empty-slot` (need the real save), `game-options-core`, `save-core`, `save-real-data`, `slot-progress`, `slot-progress-real-data` (needs a copy of a real save, `-DCHASE_REAL_SAVE`).
