# HUD and UI layouts

## 1. Scope

The in-game HUD (hearts, stud counter, studs flying to the counter) and the `.bxaml` UI layouts and storyboards that draw it, plus later menus. Front-end screens are [front-end.md](front-end.md); the stud rule is [studs.md](studs.md). Fonts and text are in [assets-formats.md](assets-formats.md).

## 2. Status

NOT_STARTED on main, updated 2026-10-13T21:29:01+0200. The layouts decode (`.bxaml` in C++, `src/assets/xaml.cpp`). A HUD branch is being worked on by another agent; its result is not merged yet, so this file records main only.

## 3. Verified behaviour

- `.bxaml` layouts decode; the storyboard runtime layout (0x1dbb80) is recorded. Nothing of the HUD is drawn by `chase-game`; the stud counter exists as a value only (printed by `--script`).

## 4. Stand-ins

- None yet (no HUD code on main).

## 5. Open items

- [ ] HUD-1 — Draw the HUD (hearts, stud counter) from the `.bxaml` layouts. blocks: STUD-2.
- [ ] HUD-2 — Studs flying to the counter (replaces the vanish stand-in STUD-S4).
- [ ] HUD-3 — The `.bxaml` track properties and the storyboard property evaluator (not found, time-boxed). evidence: [ui-layouts.md §2026-10-13T09:12](../../re/ui-layouts.md).
- [ ] HUD-4 — The stud counter's display (0x15b2f0 loads the same sheet for the stud bar) and the per-level stud total. evidence: [studs.md §2026-10-13T17:09](../../re/studs.md).
- [ ] HUD-5 — In-game menus and the main menu are not recovered.

## 6. Findings summary

- `.bxaml` layout format and storyboards: [ui-layouts.md §2026-10-13T11:00](../../re/ui-layouts.md), runtime layout [ui-layouts.md §2026-10-13T09:12](../../re/ui-layouts.md).

## 7. Code and tests

- `src/assets/xaml.cpp`; `chase-game` front-end uses its own CPU canvas (`src/frontend/`).
- CTest: `assets-real-data`.
