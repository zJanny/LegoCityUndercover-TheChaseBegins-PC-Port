# Saves and options

## 1. Scope

`savegame.dat`: header, CRC, validation, atomic XDG storage, option fields, slot progress accessors and the completion percentage (0x1cd3dc), the level and character tables. The slot screens of the front end are [front-end.md](front-end.md).

## 2. Status

IN_PROGRESS, updated 2026-10-14T11:53:08+0200. `chase_save` reads, validates and writes safely and `chase-savecheck` reports completion; `chase-game --save FILE --slot N` starts a level with the progress of a real slot (saved level variables, unlock bits, game complete). No game program writes a save.

## 3. Verified behaviour

- Header size and version sources, CRC, option defaults (0x2081cc), option fields (sound/music volume, invert camera, subtitles), language order and gift list recovered.
- Slot progress layout (character bits, level blocks), completion formula (divide 0x25c1b8 is unsigned), totals source `TablesSuperCounters` (0x2f63dc) matching the 13 x 7 totals layout, offline completion total 711 (`save-real-data`, `save-core`). No cheat flags in the completion.

- Level state block (0x820 bytes): length, StoryProgress, saved flag, state records; the level-data record (default parameters with flag bit 1) decodes exactly on the user's real save; unlock bits at slot +0x51e0; `GetGameComplete` and `IsLevelUnlocked` read the slot, `IsRedBrickExtraActive` reads a runtime mask that a load leaves 0. With the user's slot (mission 1-1 done) the bound enable bytes equal the oracle state `street-after-1-1` except the three SCRIPT-10 bounds (`slot-progress-real-data`).

## 4. Stand-ins

- SAVE-S1 An empty slot wrap is shown as empty by `chase-savecheck`.
- SAVE-S2 `chase-game --save` does not know which level a Continue starts in or where; the caller gives `--level` (and `--start`). The object records of the saved state are not applied (SAVE-6). needs: SAVE-6, SAVE-7.

## 5. Open items

- [x] SAVE-1 — closed 2026-10-14: `chase-game --save FILE --slot N` loads a slot through `chase_save_progress` and gives the script runner the saved variables of the level (StoryProgress, ...) and the unlock / complete state; replaces `--call-script 1-1_MissionSkip` for progress. evidence: [save-format.md §2026-10-14T11:53](../../re/save-format.md#ts-20261014-115300).
- [ ] SAVE-2 — Existing slots with time, percent and location, Continue and delete in the front end. blocks: FE-1.
- [ ] SAVE-3 — Compare `chase-savecheck` with the percentage the game shows (possible now with the oracle).
- [ ] SAVE-4 — Options +0 nibbles and +3 bits 2..5 are not understood; the options volume source in a loaded save. evidence: [save-format.md §2026-10-13T08:53](../../re/save-format.md#ts-20261013-0853).
- [ ] SAVE-6 — Object records of the saved state (class restore callbacks 0x583124 +0x24: doors, pickups, build-its), the end-marker bit set (`scene+0x12f4`) and the hub block are not decoded or applied, so collected items and object states do not come from the save yet. blocks: SAVE-S2. evidence: [save-format.md §2026-10-14T11:53](../../re/save-format.md#ts-20261014-115300).
- [ ] SAVE-7 — Which level and position a Continue starts in (no field found in the slot); the second unlock bit (`which` 1) writer; whether the main-scene StoryProgress override 0x192e14 matters. blocks: SAVE-S2.
- [ ] SAVE-5 — Most of what a save game contains: only the completion totals are understood.

## 6. Findings summary

- Layout, CRC, save thread: [save-format.md §2026-10-13T10:30](../../re/save-format.md#ts-20261013-1030).
- Options and defaults: [save-format.md §2026-10-13T12:30](../../re/save-format.md#ts-20261013-1230), [save-format.md §2026-10-13T08:53](../../re/save-format.md#ts-20261013-0853), [save-format.md §2026-10-13T08:56](../../re/save-format.md#ts-20261013-0856).
- Level state blocks, unlock bits, the progress natives and the user's save: [save-format.md §2026-10-14T11:53](../../re/save-format.md#ts-20261014-115300).
- Slot progress, totals and completion: [save-format.md §2026-10-13T09:13](../../re/save-format.md#ts-20261013-0913), [save-format.md §2026-10-13T09:25](../../re/save-format.md#ts-20261013-0925), [save-format.md §2026-10-13T09:26](../../re/save-format.md#ts-20261013-092607).

## 7. Code and tests

- `src/save/` (`chase_save`, `chase_save_tables`), `chase-savecheck`, `chase-tabledump`.
- `src/save/slot_progress.*` (`chase_save_progress`), `src/game_save.hpp`, `ScriptRunner::setProgress` / `setVariable`.
- CTest: `save-core`, `save-real-data`, `slot-progress`, `slot-progress-real-data` (needs a copy of a real save, `-DCHASE_REAL_SAVE`).
