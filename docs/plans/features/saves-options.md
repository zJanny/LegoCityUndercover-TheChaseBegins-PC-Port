# Saves and options

## 1. Scope

`savegame.dat`: header, CRC, validation, atomic XDG storage, option fields, slot progress accessors and the completion percentage (0x1cd3dc), the level and character tables. The slot screens of the front end are [front-end.md](front-end.md).

## 2. Status

IN_PROGRESS, updated 2026-10-28T22:52:58+01:00. A run keeps the live options block and volume bytes: each level load takes the volumes of the slot at options +2, the pause Options page changes them and saves the options, a save request copies the live options into the file, and the follow camera reads the Invert camera bit. A `chase-game` run keeps the live slot, the stud counter and the super brick count across its level sessions. A new game starts from the original's default slot (0x228274, 0x1a25cc), `--save FILE --slot N` from that slot; both go through the level entry 0x20a4d4 (studs and super bricks from the slot). Every update (also of the open pause menu) ticks the play time; a level load shows the loaded studs on the HUD; each save request writes the summary with the live counters into the slot and stores `savegame.dat` at `--save-out OUT` or the default XDG path; such a file starts again with `--save`. A save request also writes the level state block with the level variables and StoryProgress; object records come only from an earlier saved state (SAVE-14). Earlier: `chase_save` reads, validates and writes safely, `chase-savecheck` reports completion, `--save` starts the level of slot byte +6 at its Player1Start, the slot row fields decode.

## 3. Verified behaviour

- Header size and version sources, CRC, option defaults (0x2081cc), option fields (sound/music volume, invert camera, subtitles), language order and gift list recovered.
- The loading-screen enter 0x303a40 sets the live volumes from the slot at options +2; on the oracle's state (slot 1: sound effects 5, music 5) the live bytes and gains are 5 and 0.5 (`options-page`, `game-options-reload`); options +0 and +3 bits 2..5 have no reader: [save-format.md §2026-10-28T22:51:25](../../re/save-format.md#ts-20261028-225125).
- Slot progress layout (character bits, level blocks), completion formula (divide 0x25c1b8 is unsigned), totals source `TablesSuperCounters` (0x2f63dc) matching the 13 x 7 totals layout, offline completion total 711 (`save-real-data`, `save-core`). No cheat flags in the completion.

- The save request chain 0x21cf38 and the commit of the live slot into the file image are transcribed; committing the user's slot 0 bytes reproduces the file byte for byte; the slot row fields (ticks at +0, per mille at +4, level byte +6) and their texts follow 0x2cc298 (`save-writer`, `save-writer-real-data`).
- Object records of the saved state decode on the user's save (48 in level 2, 1 in level 1, all payloads 0, end bit set is zero padding); slot byte +6 is the current level (`slot-progress-real-data`).
- Level state block (0x820 bytes): length, StoryProgress, saved flag, state records; the level-data record (default parameters with flag bit 1) decodes exactly on the user's real save; unlock bits at slot +0x51e0; `GetGameComplete` and `IsLevelUnlocked` read the slot, `IsRedBrickExtraActive` reads a runtime mask that a load leaves 0. With the user's slot (mission 1-1 done) the bound enable bytes equal the oracle save state outside the police station after mission 1-1 except the three SCRIPT-10 bounds (`slot-progress-real-data`).
- The new-game slot after the level entry equals the live slot of the oracle state `gameplay-start` in every byte the slot callback and the level entry write (`new-game-slot-real-data`); a new game's save request writes a valid file to the default path and a `--save` start from it has the 1500 super bricks again (`game-save-new-game`, `game-save-reload`); studs and the count of the user's save copy load as 1130 and 52.
- Re-serialising the saved level blocks 1 and 2 of the user's save copy gives the same bytes and StoryProgress (`level-state-real-data`); a new game's save request writes StoryProgress 1 like the oracle's `gameplay-start` slot.

## 4. Stand-ins

- SAVE-S1 An empty slot wrap is shown as empty by `chase-savecheck`.
- SAVE-S2 (closed 2026-10-14: `--save` starts the level of the slot, see SAVE-9.)
- SAVE-S4 Continue enters every level at its Player1Start (code shows no saved position; measured for level 1 only). needs: SAVE-13.
- SAVE-S5 The level state of a save request: object records the port does not produce are the slot's own saved records written back (a 4-byte one with the live TASKS_COMPLETE), the end bit set keeps that state's length (none without one), string and vector parameters are the table defaults, and 0x229740 is not called. needs: SAVE-14. evidence: [save-format-state.md §2026-10-19T20:24:11](../../re/save-format-state.md#ts-20261019-202411).
- SAVE-S6 The part indices of the eight customisation records of a new game (slot +0x5150, bytes 0..3 of each) are the oracle's `gameplay-start` bytes (`kNewGameCustomisationPartsStandIn`). needs: SAVE-16. evidence: [save-format-state.md §2026-10-19T20:24:11](../../re/save-format-state.md#ts-20261019-202411).
- [x] SAVE-S7 — Closed 2026-10-20: no code of the executable sets the cheat bits, so clear is the original state. evidence: [save-format-state.md §2026-10-20T10:29:41](../../re/save-format-state.md#ts-20261020-102941).
- SAVE-S8 The file is stored in the update of the save request; the save thread's I/O time and the one pending request of the queue 0x2400a0 are not modelled. needs: SAVE-20. evidence: [save-format-state.md §2026-10-19T20:24:11](../../re/save-format-state.md#ts-20261019-202411).
- [x] SAVE-S9 — Closed 2026-10-20: a level load (0x2e5fd8) reads the type from the slot bits into the HUD byte and a counter of 0 resets it, as `hudStudCounterAtLoad` does. evidence: [save-format-state.md §2026-10-20T10:29:41](../../re/save-format-state.md#ts-20261020-102941).
- SAVE-S10 Save requests go to `--save-out` or the default XDG path, also in a run started with `--save FILE` (the original has one file); a new game without the front end uses `--slot` (default 0). evidence: [save-format-state.md §2026-10-19T20:24:11](../../re/save-format-state.md#ts-20261019-202411).
- SAVE-S11 The play time at slot +0 ticks once in every update of a level session and of the open pause menu (0x300444); the conditions of the level update loop (mode word 3, level resource loaded) and of the loops 0x2f26e8 and 0x2d8a30 are not ported, and whether the update that opens the pause ticks is not known. needs: SAVE-22. evidence: [save-format-state.md §2026-10-20T10:29:41](../../re/save-format-state.md#ts-20261020-102941).
- SAVE-S3 The 1-byte object records (types 0x15, 0x43, 0x62) are decoded but not applied; the 4-byte ones set TASKS_COMPLETE. All payloads of the only real save are 0, so nothing differs. needs: SAVE-8.

## 5. Open items

- [ ] SAVE-2 — Data side done (`slotRow`, `formatPlayTime`, `formatPercent`, `locationTextHash`); the slot list page with the rows, Continue and delete is FE-11. blocks: FE-1. evidence: [save-format-state.md §2026-10-14T16:34](../../re/save-format-state.md#ts-20261014-163403).
- [ ] SAVE-3 — Compare `chase-savecheck` with the percentage the game shows (possible now with the oracle).
- [ ] SAVE-23 — Options +0x20b bit 5 and +3 bit 0 written by a second options screen of the code (rows 4 and 6): which screen it is and what +0x20b bit 5 does. evidence: [save-format.md §2026-10-28T22:51:25](../../re/save-format.md#ts-20261028-225125).
- [ ] SAVE-8 — Names and creator of the type 0xfc and 0x62 objects behind the record hashes, and the effect of type 0x15 / 0x43 / 0x62 records with bit 0 set (needs a save with built build-its). blocks: SAVE-S3, LEVOBJ-43, LEVOBJ-34. evidence: [save-format-state.md §2026-10-14T13:41](../../re/save-format-state.md#ts-20261014-134146)
- [ ] SAVE-12 — Where the language byte 0x376adc comes from (options field) and its wiring into GetLanguage. blocks: SCRIPT-65, SCRIPT-S280. evidence: [script-natives-progress.md §2026-10-14T14:41](../../re/script-natives-progress.md#ts-20261014-144155).
- [ ] SAVE-13 — Continue at level 2 or later is not measured (heading, position, level scripts moving the player); needs a user save or state with slot byte +6 above 1. blocks: SAVE-S4. evidence: [save-format-state.md §2026-10-14T15:19](../../re/save-format-state.md#ts-20261014-151903).
- [ ] SAVE-14 — The level state refresh writes the level-data record and StoryProgress (`levelStateData`); open: the save callbacks of table 0x583144 for build-its, props, super build-its and TASKS_COMPLETE (needs SAVE-8), the count at `scene + 0x12f4`, and 0x229740 / 0x2baddc. blocks: SAVE-S5. evidence: [save-format-state.md §2026-10-19T20:24:11](../../re/save-format-state.md#ts-20261019-202411).
- [ ] SAVE-16 — The four 0xcf30-byte customisation tables of 0x2cd154 (stream 0x5145cc) are not found, so the part indices of the new-game records are not computed. blocks: SAVE-S6. evidence: [save-format-state.md §2026-10-19T20:24:11](../../re/save-format-state.md#ts-20261019-202411).
- [ ] SAVE-21 — The meaning of the bits at slot +0x5180 (writers 0x285c08 from the reward table case 4 of 0x322fec and from 0x1bf9dc, reader 0x27baec) and their port; the callers of 0x322fec and 0x1bf9dc. evidence: [save-format-state.md §2026-10-20T10:29:41](../../re/save-format-state.md#ts-20261020-102941).
- [ ] SAVE-22 — Which game states own the modes with the vtables 0x362b40 (update 0x2f26c8) and 0x361cc0 (update 0x2d8984), whose loops tick the play time. blocks: SAVE-S11. evidence: [save-format-state.md §2026-10-20T10:29:41](../../re/save-format-state.md#ts-20261020-102941).
- [ ] SAVE-20 — The save thread's I/O time and the pending request of 0x2400a0 / 0x1f05e8, and when the save indicator shows. blocks: SAVE-S8. evidence: [save-format-state.md §2026-10-19T20:24:11](../../re/save-format-state.md#ts-20261019-202411).
- [ ] SAVE-5 — Most of what a save game contains: only the completion totals are understood.

## 6. Findings summary

- Layout, CRC, save thread: [save-format.md §2026-10-13T10:30](../../re/save-format.md#ts-20261013-1030).
- Options and defaults: [save-format.md §2026-10-13T12:30](../../re/save-format.md#ts-20261013-1230), [save-format.md §2026-10-13T08:53](../../re/save-format.md#ts-20261013-0853), [save-format.md §2026-10-13T08:56](../../re/save-format.md#ts-20261013-0856).
- Live volumes, loading-screen volume source, unread option bits, movie volume: [save-format.md §2026-10-28T22:51:25](../../re/save-format.md#ts-20261028-225125).
- Level state blocks, unlock bits, the progress natives and the user's save: [save-format-state.md §2026-10-14T11:53](../../re/save-format-state.md#ts-20261014-115300).
- Object records, end bit set, current level, second unlock bit: [save-format-state.md §2026-10-14T13:41](../../re/save-format-state.md#ts-20261014-134146).
- Slot bytes the progress natives write (abilities +0x51ee, races +0x5200, racing car +0x520c, red bricks +0x51b8, character group +0x51bb): [script-natives-progress.md §2026-10-14T14:41](../../re/script-natives-progress.md#ts-20261014-144155).
- Save request chain, slot commit, slot row fields: [save-format-state.md §2026-10-14T16:34](../../re/save-format-state.md#ts-20261014-163403).
- New-game slot, level entry, summary inputs, play time, save queue: [save-format-state.md §2026-10-19T20:24:11](../../re/save-format-state.md#ts-20261019-202411).
- Cheat bytes, slot +0x5180, the HUD counter at a load, pause-mode play time: [save-format-state.md §2026-10-20T10:29:41](../../re/save-format-state.md#ts-20261020-102941).
- Where Continue starts (level of byte +6, Player1Start): [save-format-state.md §2026-10-14T15:19](../../re/save-format-state.md#ts-20261014-151903).
- Slot progress, totals and completion: [save-format.md §2026-10-13T09:13](../../re/save-format.md#ts-20261013-0913), [save-format.md §2026-10-13T09:25](../../re/save-format.md#ts-20261013-0925), [save-format.md §2026-10-13T09:26](../../re/save-format.md#ts-20261013-092607).

## 7. Code and tests

- `src/save/` (`chase_save`, `chase_save_tables`), `chase-savecheck`, `chase-tabledump`.
- `src/save/save_writer.*` (`commitSlot`, `writeSlotSummary`, `SaveRequestWriter::request` with the live options, `SaveRequestWriter::storeOptions`, `setSlotStudType`, `slotRow`, `formatPlayTime`, `formatPercent`, `locationTextHash`), `--save-out`.
- `src/save/new_game.*` (`newGameTables`, `newGameSlot`, `unlockCharacterChain`, `resetSlotOptions`, `enterLevel`); `src/save/level_state.*` (`levelStateData`, `writeLevelState`, `storyProgressOf`), `game_save::refreshLevelState`.
- `src/runtime/options_page.*` (`loadingScreenVolumes`, `restoreSoundVolume`), `game_save::loadingScreen`, `Progress::options` / `volumes`; CTest `options-page`, `game-options-reload`.
- `src/runtime/save_hud_counter.*` (`hudStudCounterAtLoad`), `game_save::playTick`; CTest `save-hud-counter`, `game-save-hud-studs` (use the fixture save of `test-save-build`).
- `src/save/continue_start.*` (`continueStart`), `game_save::chooseContinueLevel`, `GameOptions::levelGiven`.
- `src/save/slot_progress.*` (`chase_save_progress`), glue `src/game_save.*` (`Progress` of a run, `start`, `sceneEntered`, `slotStartFor`, `apply`, `update`), `LevelSession::handOverProgress`, `ScriptRunner::setProgress` / `setVariable`, `ProgressNativeState::slotImage`.
- CTest: `save-writer`, `save-writer-real-data` (save built by `tests/save_fixture.hpp`), `new-game-slot`, `new-game-slot-real-data`, `level-state`, `level-state-real-data`, `game-save-new-game`, `game-save-reload`, `game-save-continue`, `game-save-continue-empty-slot` (use the fixture save of `test-save-build`), `game-options-core`, `save-core`, `save-real-data`, `slot-progress`, `slot-progress-real-data` (save built by `tests/save_fixture.hpp` from the named values of the user's save). Every CTest runs with its own `XDG_DATA_HOME` under `build/test-xdg/`.
