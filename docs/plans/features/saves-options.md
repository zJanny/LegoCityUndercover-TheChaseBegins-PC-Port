# Saves and options

## 1. Scope

`savegame.dat`: header, CRC, validation, atomic XDG storage, option fields, slot progress accessors and the completion percentage (0x1cd3dc), the level and character tables. The slot screens of the front end are [front-end.md](front-end.md).

## 2. Status

IN_PROGRESS, updated 2026-10-14T16:35:00+0200. `chase_save` reads, validates and writes safely and `chase-savecheck` reports completion; `chase-game --save FILE --slot N` starts the level of the slot (byte +6) at its Player1Start with the progress of the slot (saved level variables, unlock bits, game complete); `--level` and `--start` override. `chase-game --save FILE --save-out OUT` writes the slot to OUT on every save request of the progress natives (`SaveRequestWriter`, never to the `--save` file); without `--save-out` nothing is written. The slot row fields (play time, percent, location) decode from the slot.

## 3. Verified behaviour

- Header size and version sources, CRC, option defaults (0x2081cc), option fields (sound/music volume, invert camera, subtitles), language order and gift list recovered.
- Slot progress layout (character bits, level blocks), completion formula (divide 0x25c1b8 is unsigned), totals source `TablesSuperCounters` (0x2f63dc) matching the 13 x 7 totals layout, offline completion total 711 (`save-real-data`, `save-core`). No cheat flags in the completion.

- The save request chain 0x21cf38 and the commit of the live slot into the file image are transcribed; committing the user's slot 0 bytes reproduces the file byte for byte; the slot row fields (ticks at +0, per mille at +4, level byte +6) and their texts follow 0x2cc298 (`save-writer`, `save-writer-real-data`).
- Object records of the saved state decode on the user's save (48 in level 2, 1 in level 1, all payloads 0, end bit set is zero padding); slot byte +6 is the current level (`slot-progress-real-data`).
- Level state block (0x820 bytes): length, StoryProgress, saved flag, state records; the level-data record (default parameters with flag bit 1) decodes exactly on the user's real save; unlock bits at slot +0x51e0; `GetGameComplete` and `IsLevelUnlocked` read the slot, `IsRedBrickExtraActive` reads a runtime mask that a load leaves 0. With the user's slot (mission 1-1 done) the bound enable bytes equal the oracle state `street-after-1-1` except the three SCRIPT-10 bounds (`slot-progress-real-data`).

## 4. Stand-ins

- SAVE-S1 An empty slot wrap is shown as empty by `chase-savecheck`.
- SAVE-S2 (closed 2026-10-14: `--save` starts the level of the slot, see SAVE-9.)
- SAVE-S4 Continue enters every level at its Player1Start (code shows no saved position; measured for level 1 only). needs: SAVE-13.
- SAVE-S5 A save request writes only to the file given with `--save-out`; the summary of 0x17caf8 takes studs, stud total and the display byte from the slot itself, not from live state; the level state block is not refreshed; the play-time counter at slot +0 is not incremented. needs: SAVE-14, SAVE-15. evidence: [save-format-state.md §2026-10-14T16:34](../../re/save-format-state.md#ts-20261014-163403).
- SAVE-S3 The 1-byte object records (types 0x15, 0x43, 0x62) are decoded but not applied; the 4-byte ones set TASKS_COMPLETE. All payloads of the only real save are 0, so nothing differs. needs: SAVE-8.

## 5. Open items

- [ ] SAVE-2 — Data side done (`slotRow`, `formatPlayTime`, `formatPercent`, `locationTextHash`); the slot list page with the rows, Continue and delete is FE-11. blocks: FE-1. evidence: [save-format-state.md §2026-10-14T16:34](../../re/save-format-state.md#ts-20261014-163403).
- [ ] SAVE-3 — Compare `chase-savecheck` with the percentage the game shows (possible now with the oracle).
- [ ] SAVE-4 — Options +0 nibbles and +3 bits 2..5 are not understood; the options volume source in a loaded save. evidence: [save-format.md §2026-10-13T08:53](../../re/save-format.md#ts-20261013-0853).
- [ ] SAVE-8 — Names and creator of the type 0xfc and 0x62 objects behind the record hashes, and the effect of type 0x15 / 0x43 / 0x62 records with bit 0 set (needs a save with built build-its). blocks: SAVE-S3. evidence: [save-format-state.md §2026-10-14T13:41](../../re/save-format-state.md#ts-20261014-134146)
- [ ] SAVE-11 — New-game slot defaults (0x1a25cc: level 2 unlocked, characters 9..140, sets) are not reproduced; without `--save` the progress natives start from a zero image. evidence: [save-format.md §2026-10-13T09:13](../../re/save-format.md#ts-20261013-0913).
- [ ] SAVE-12 — Where the language byte 0x376adc comes from (options field) and its wiring into GetLanguage. blocks: SCRIPT-65. evidence: [script-natives-progress.md §2026-10-14T14:41](../../re/script-natives-progress.md#ts-20261014-144155).
- [ ] SAVE-13 — Continue at level 2 or later is not measured (heading, position, level scripts moving the player); needs a user save or state with slot byte +6 above 1. blocks: SAVE-S4. evidence: [save-format-state.md §2026-10-14T15:19](../../re/save-format-state.md#ts-20261014-151903).
- [ ] SAVE-14 — The level state refresh 0x192f60 (object serialiser 0x1c26dc into block +0x1f, length +0x1a, saved flag, StoryProgress 0x2451d0) and 0x229740 / 0x2baddc are not ported, so a save keeps the old level block. blocks: SAVE-S5. evidence: [save-format-state.md §2026-10-14T16:34](../../re/save-format-state.md#ts-20261014-163403).
- [ ] SAVE-15 — Live inputs of the summary (0x593418 +0 / +0x18, byte 0x519760+0x40), the per-update play-time tick (`addPlayTick`, four update loops), the write to the default XDG path instead of `--save-out`, and the request queue behaviour 0x2400a0 are not wired. blocks: SAVE-S5. evidence: [save-format-state.md §2026-10-14T16:34](../../re/save-format-state.md#ts-20261014-163403).
- [ ] SAVE-5 — Most of what a save game contains: only the completion totals are understood.

## 6. Findings summary

- Layout, CRC, save thread: [save-format.md §2026-10-13T10:30](../../re/save-format.md#ts-20261013-1030).
- Options and defaults: [save-format.md §2026-10-13T12:30](../../re/save-format.md#ts-20261013-1230), [save-format.md §2026-10-13T08:53](../../re/save-format.md#ts-20261013-0853), [save-format.md §2026-10-13T08:56](../../re/save-format.md#ts-20261013-0856).
- Level state blocks, unlock bits, the progress natives and the user's save: [save-format-state.md §2026-10-14T11:53](../../re/save-format-state.md#ts-20261014-115300).
- Object records, end bit set, current level, second unlock bit: [save-format-state.md §2026-10-14T13:41](../../re/save-format-state.md#ts-20261014-134146).
- Slot bytes the progress natives write (abilities +0x51ee, races +0x5200, racing car +0x520c, red bricks +0x51b8, character group +0x51bb): [script-natives-progress.md §2026-10-14T14:41](../../re/script-natives-progress.md#ts-20261014-144155).
- Save request chain, slot commit, slot row fields: [save-format-state.md §2026-10-14T16:34](../../re/save-format-state.md#ts-20261014-163403).
- Where Continue starts (level of byte +6, Player1Start): [save-format-state.md §2026-10-14T15:19](../../re/save-format-state.md#ts-20261014-151903).
- Slot progress, totals and completion: [save-format.md §2026-10-13T09:13](../../re/save-format.md#ts-20261013-0913), [save-format.md §2026-10-13T09:25](../../re/save-format.md#ts-20261013-0925), [save-format.md §2026-10-13T09:26](../../re/save-format.md#ts-20261013-092607).

## 7. Code and tests

- `src/save/` (`chase_save`, `chase_save_tables`), `chase-savecheck`, `chase-tabledump`.
- `src/save/save_writer.*` (`commitSlot`, `writeSlotSummary`, `SaveRequestWriter`, `slotRow`, `formatPlayTime`, `formatPercent`, `locationTextHash`), glue `game_save::requestWriter`, `--save-out`.
- `src/save/continue_start.*` (`continueStart`), `game_save::chooseContinueLevel`, `GameOptions::levelGiven`.
- `src/save/slot_progress.*` (`chase_save_progress`), `src/game_save.hpp`, `ScriptRunner::setProgress` / `setVariable`, `SlotStart::slotBytes` to `ProgressNativeState::setSlotImage`.
- CTest: `save-writer`, `save-writer-real-data` (needs the real save copy), `game-save-continue`, `game-save-continue-empty-slot` (need the real save), `game-options-core`, `save-core`, `save-real-data`, `slot-progress`, `slot-progress-real-data` (needs a copy of a real save, `-DCHASE_REAL_SAVE`).
