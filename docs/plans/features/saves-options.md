# Saves and options

## 1. Scope

`savegame.dat`: header, CRC, validation, atomic XDG storage, option fields, slot progress accessors and the completion percentage (0x1cd3dc), the level and character tables. The slot screens of the front end are [front-end.md](front-end.md).

## 2. Status

IN_PROGRESS, updated 2026-10-13T21:29:01+0200. `chase_save` reads, validates and writes safely and `chase-savecheck` reports completion; no game program loads or writes a save.

## 3. Verified behaviour

- Header size and version sources, CRC, option defaults (0x2081cc), option fields (sound/music volume, invert camera, subtitles), language order and gift list recovered.
- Slot progress layout (character bits, level blocks), completion formula (divide 0x25c1b8 is unsigned), totals source `TablesSuperCounters` (0x2f63dc) matching the 13 x 7 totals layout, offline completion total 711 (`save-real-data`, `save-core`). No cheat flags in the completion.

## 4. Stand-ins

- SAVE-S1 An empty slot wrap is shown as empty by `chase-savecheck`.

## 5. Open items

- [ ] SAVE-1 — No program uses saves: a save-aware level start and new-game values replaced by loaded ones. blocks: SCRIPT-S2.
- [ ] SAVE-2 — Existing slots with time, percent and location, Continue and delete in the front end. blocks: FE-1.
- [ ] SAVE-3 — Compare `chase-savecheck` with the percentage the game shows (possible now with the oracle).
- [ ] SAVE-4 — Options +0 nibbles and +3 bits 2..5 are not understood; the options volume source in a loaded save. evidence: [save-format.md §2026-10-13T08:53](../../re/save-format.md).
- [ ] SAVE-5 — Most of what a save game contains: only the completion totals are understood.

## 6. Findings summary

- Layout, CRC, save thread: [save-format.md §2026-10-13T10:30](../../re/save-format.md).
- Options and defaults: [save-format.md §2026-10-13T12:30](../../re/save-format.md), [save-format.md §2026-10-13T08:53](../../re/save-format.md), [save-format.md §2026-10-13T08:56](../../re/save-format.md).
- Slot progress, totals and completion: [save-format.md §2026-10-13T09:13](../../re/save-format.md), [save-format.md §2026-10-13T09:25](../../re/save-format.md), [save-format.md §2026-10-13T09:26](../../re/save-format.md).

## 7. Code and tests

- `src/save/` (`chase_save`, `chase_save_tables`), `chase-savecheck`, `chase-tabledump`.
- CTest: `save-core`, `save-real-data`.
