# Input

## 1. Scope

Mapping the 3DS controls to keyboard and mouse (configurable bindings, hold/toggle, focus reset, touch mapping inside the aspect-fitted 320x240 viewport) and the pad pipeline that feeds the game logic (input slot table, channels, stick to D-pad). Mapping is part of the port, not a QoL feature, and must not change game behaviour. Rule (user, 2026-10-19): the bottom-screen UI (map buttons, menus, every touch control) is operated by clicking it with the mouse, as the touch screen; touch-only controls get no keyboard keys. Buttons the original also reads from the pad keep their pad bindings. Player rules that consume the stick are in [player-movement.md](player-movement.md).

## 2. Status

DONE for the input laboratory (S2) and the keyboard/mouse mapping used by `chase-game`; the pad pipeline replica works for the recovered slots. Updated 2026-10-14T14:37:06+02:00.

## 3. Verified behaviour

- A hold action stays active while any binding is held; a toggle flips only on a fresh press; opposite directions cancel; focus loss resets all held and toggled state; touch maps only inside the viewport (`input-core`, `input-sdl-smoke`, `--smoke-test`).
- Input slot table 0x3769e8 (18 slots to pad channels), A/B pairs, channel 7 unfed by the pad, stick to D-pad 0x2289b8; pad pipeline replica matches the live stick value (`runtime-core`).
- `DebugOverlay` (default F3, toggle; developer tool, no game behaviour) is parsed like every action; a malformed line is still rejected (`input-core`); see [TOOLS-10 in viewer-tools](viewer-tools.md).
- Action names are 3DS buttons, not gameplay meanings; `Walk` (Left Alt) and jump on B (Q or the right mouse button) are bound in `config/controls.cfg`.

## 4. Stand-ins

- INPUT-S1 The `Walk` key (Left Alt, own non-3DS action) stands in for a half-tilted circle pad.

## 5. Open items

- [ ] INPUT-1 — Keep open (user decision 2026-10-13T21:38): the `Walk` key stands in for a half-tilted circle pad; revisit with an analog mapping if controller support is added later (a mapping question, not game behaviour).
- [ ] INPUT-2 — Readers of input slots 16/17. evidence: [arm-executable.md §2026-10-13T09:22](../../re/arm-executable.md#ts-20261013-092253).
- [ ] INPUT-3 — Slots 6/7 are read by gameplay and 4/5 by UI; which UI actions use them and the controller state 255 are unknown. evidence: [arm-executable.md §2026-10-13T09:22](../../re/arm-executable.md#ts-20261013-092253), [oracle.md §2026-10-13T11:50](../../re/oracle.md#ts-20261013-115030).

## 6. Findings summary

- Slot table and channel records: [arm-executable.md §2026-10-13T09:20](../../re/arm-executable.md#ts-20261013-092040); readers and stick to D-pad: [arm-executable.md §2026-10-13T09:22](../../re/arm-executable.md#ts-20261013-092253); HID and main loop: [arm-executable.md §2026-10-13T10:45](../../re/arm-executable.md#ts-20261013-1045).
- Stick to intent: [player-entity-input.md §2026-10-13T10:59](../../re/player-entity-input.md#ts-20261013-105911).

## 7. Code and tests

- `src/input.*` (`chase_input`, no SDL), `src/main.cpp` (`chase-input-lab`), `src/runtime/pad.*`, `config/controls.cfg` (format in CLAUDE.md "Architecture rules"); user docs `docs/controls.md`.
- CTest: `input-core` (includes the `DebugOverlay` line), `input-sdl-smoke`, `runtime-core`.
