# Level objects: wobbly props and bouncers

## 1. Scope

Wobbly destructible props (`leprops/leprop_wobblydestructible`: hit message, health, wobble, destroy chain) and bouncers (trampolines: message 15, take-off speed, the bounce flight, the wall side test of the built frame). Split from [level-objects](level-objects.md).

## 2. Status

IN_PROGRESS, updated 2026-10-19T13:12:46+02:00. The wobbly destructible props take hits from any sender (the punch of Chase, health, bricks, destroy, `TriggerObject`) and the trampolines bounce Chase (LEVOBJ-86 closed, gaps LEVOBJ-87..92).

## 3. Verified behaviour

- Wobbly props take the hit message from any sender, lose health and run the destroy chain with bricks; CTest `prop-wobbly`, `prop-wobbly-real-data`, `game-script-wobbly-table`.
- Bouncers take message 15 and launch Chase with the take-off speed of the pad; CTest `prop-bouncer`, `prop-bouncer-real-data`, `game-script-bounce`.
- The built trampoline frame blocks a walk from outside only (wall side test of placed objects); CTest `prop-wall-side`, `prop-wall-side-real-data`.

## 4. Stand-ins

- LEVOBJ-S77 The hit of a sender other than the player is never dropped as "already wobbling" (0x1c54ac) and a wobbling prop shows no wobble: the spring table is not ported. needs: LEVOBJ-87. evidence: [props-wobbly-bouncer.md §2026-10-18T20:25:42](../../re/props-wobbly-bouncer.md#ts-20261018-202542).
- LEVOBJ-S81 The wall side test of the candidate filter 0x209a98 (a triangle only blocks while the player's box centre is in front of its plane) is applied to the triangles of placed objects (every pooled entry) and not to static level triangles, whose entries were not read. needs: LEVOBJ-96. evidence: [collision.md §2026-10-19T07:58:53](../../re/collision.md#ts-20261019-075853).
- LEVOBJ-S79 The bounce plays only the motion: no `bounce_up` clip, no bouncer animation, no pitch from `FrequencyRange`, no particle; `ATTR_SFX` plays at the player's position at the take-off. needs: LEVOBJ-89. evidence: [props-wobbly-bouncer.md §2026-10-18T20:25:42](../../re/props-wobbly-bouncer.md#ts-20261018-202542).
- LEVOBJ-S80 The take-off memory (ctrl+0x2dc) is cleared when the player lands on a floor that is not a bouncer, and a landing from a jump passes fall speed 0 to the first-bounce formula. needs: LEVOBJ-92. evidence: [props-wobbly-bouncer.md §2026-10-18T20:25:42](../../re/props-wobbly-bouncer.md#ts-20261018-202542).

## 5. Open items

- [ ] LEVOBJ-87 — The wobble of a hit that leaves health: the spring entry (0x16b958, 0x2f5af8, table 0x593440), what frees its slot and so the "already wobbling" drop of other senders (0x1c54ac), and the node motion it drives. blocks: LEVOBJ-S77. evidence: [props-wobbly-bouncer.md §2026-10-18T20:25:42](../../re/props-wobbly-bouncer.md#ts-20261018-202542).
- [ ] LEVOBJ-112 — The studs of a wobbling hit leave from the placement node, not the node the spring moved. needs: LEVOBJ-87. blocks: STUD-S7. evidence: [studs.md §2026-10-19T14:36:25](../../re/studs.md#ts-20261019-143625).
- [ ] LEVOBJ-89 — The bounce states 25 "Bounce Take Off" and 26 "Bounce Up" beyond the motion: the clip `bounce_up`, the done and fail events (more than 300 ticks after the enter; bouncer disabled), the update 0x2f5134 of state 26 (B, ctrl+0x317 bit 3), trick bounces (`TrickBounce`, speed above 0.65), the sound pitch from `FrequencyRange`, the particle. blocks: LEVOBJ-S79. evidence: [props-wobbly-bouncer.md §2026-10-18T20:25:42](../../re/props-wobbly-bouncer.md#ts-20261018-202542).
- [ ] LEVOBJ-90 — The bouncer object itself: its `bounce` animation on a landing, the `hit` animation and `ATTR_SFXHit` of its message 0, `AutoBounce`. blocks: LEVOBJ-S79. evidence: [props-wobbly-bouncer.md §2026-10-18T20:25:42](../../re/props-wobbly-bouncer.md#ts-20261018-202542).
- [ ] LEVOBJ-91 — The rest of the wobbly destroy: the `WobbleFallApartAnim` branch (0x2a34b8), the second Dead event for the room object (0x23ce08), `Respawn`, and the entity box of a prop with `MeshCollision` (the effects use the model bounds). evidence: [props-wobbly-bouncer.md §2026-10-18T20:25:42](../../re/props-wobbly-bouncer.md#ts-20261018-202542).
- [ ] LEVOBJ-92 — When ctrl+0x2dc (the last take-off speed) is cleared after leaving a pad, and the ctrl+0x2b8 a landing from a jump passes to the first-bounce formula (the jump has no fall speed in the port). blocks: LEVOBJ-S80. evidence: [props-wobbly-bouncer.md §2026-10-18T20:25:42](../../re/props-wobbly-bouncer.md#ts-20261018-202542).
- [ ] LEVOBJ-96 — Whether static level triangles pass the same side test: read the entries 0x20c850 returns for the master model and the owner field that 0x209a98 tests (owner+0x14). blocks: LEVOBJ-S81. evidence: [collision.md §2026-10-19T07:58:53](../../re/collision.md#ts-20261019-075853).
- [ ] LEVOBJ-97 — The stop at the trampoline frame beyond the circle: the oracle rises 0.012 m in y in the stopping update and its slide decays to 0.001 m per update, the port keeps y and slides at a constant 0.016 m; compare update by update. evidence: [props-wobbly-bouncer.md §2026-10-19T07:58:55](../../re/props-wobbly-bouncer.md#ts-20261019-075855).

## 6. Findings summary

- Wobbly props (hit message, health, destroy chain, the table's script to the trampoline build-it) and bouncers (message 15, take-off speed, flight): [props-wobbly-bouncer.md §2026-10-18T20:25:42](../../re/props-wobbly-bouncer.md#ts-20261018-202542).
- The built trampoline frame blocks a walk from outside only (wall side test of placed objects): [props-wobbly-bouncer.md §2026-10-19T07:58:55](../../re/props-wobbly-bouncer.md#ts-20261019-075855), [collision.md §2026-10-19T07:58:53](../../re/collision.md#ts-20261019-075853).

## 7. Code and tests

- `src/runtime/prop_wobbly_destructible.*`, `src/runtime/prop_bouncer.*`, the bounce flight in `src/runtime/player_vertical.cpp`, glue `src/game_props_destructible.cpp`.
- CTest: `prop-wobbly`, `prop-wobbly-real-data`, `prop-bouncer`, `prop-bouncer-real-data`, `prop-wall-side`, `prop-wall-side-real-data`, `game-script-wobbly-table`, `game-script-bounce`.
