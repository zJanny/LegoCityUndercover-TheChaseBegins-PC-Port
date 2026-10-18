# Abilities: detective tool

## 1. Scope

The detective tool of mission 1-2 and later missions: the `ability_detective*` object classes, the point that switches the player into detective mode, the scan, the reveal of a detective spot and the `ObjectTriggered` events these classes fire for mission scripts (part of SCRIPT-111 in [script-system-missions.md](script-system-missions.md)). The sound scanner is in [abilities-scanner.md](abilities-scanner.md).

## 2. Status

IN_PROGRESS, updated 2026-10-18T19:30:15+02:00. Detective spot (type 182), reveal spot (238) and reveal prop (74) run in `chase-game` with the oracle's timing; from the second detective point the mission reaches `1-2_SO_Dspot_02_X` (test `game-detective-1-2`). The Doorlock script now passes its wait (DETECT-2 closed), so the first point is enabled in `chase-game`. Chase holds the magnifying glass in detective mode (DETECT-7 closed); the other look differences against the oracle are open items.

## 3. Verified behaviour

- Footprint offsets of `1-2_Dtv_DogSpot_01` equal the oracle record (33 of 33).
- Intervals of the oracle traces: spot state 1 one update after the A press, detect_out 37, X ahead → state 2 and detect_in (19) in one update, use-state event 43 and end 116, X spot state 2 after 32 more, reveal prop launch after 30 and trigger after 18.
- Detective walk step x 1.4, no jump, `detect_*` role clips; the X spots start hidden (RevealSwitch link).
- The glass (`Ability_MagnifyingGlass` on `weaponRight`) first shows 14 updates after the A press, as in the oracle; detective mode changes neither camera nor screen tint.

## 4. Stand-ins

- DETECT-S1 — The X-spot use state's event (43) and end (116) are measured numbers, not derived from its approach phase and the clip's event table. evidence: [abilities-detective.md §2026-10-18T11:26:21](../../re/abilities-detective.md#ts-20261018-112621).
- DETECT-S2 — Fades (FadeObject, RevealObject, the X spot's own fade-out) are a show or hide at the fade's start. evidence: [abilities-detective.md §2026-10-18T11:26:21](../../re/abilities-detective.md#ts-20261018-112621).
- DETECT-S3 — Footprint pool models: FootprintType 0 / 1 use `ability_detective_foot` / `ability_detective_dog`, unrotated. evidence: [abilities-detective.md §2026-10-18T11:26:21](../../re/abilities-detective.md#ts-20261018-112621).
- DETECT-S4 — One sound owner key for all detective objects. evidence: [abilities-detective.md §2026-10-18T11:26:21](../../re/abilities-detective.md#ts-20261018-112621).
- DETECT-S5 — Bounce phases of the reveal prop count down one per update (0x2077a8 not read), which places its second trigger. evidence: [abilities-detective.md §2026-10-18T11:26:21](../../re/abilities-detective.md#ts-20261018-112621).
- DETECT-S6 — Registry order: detective objects get the A press only when no carry object, build-it zone or build is active; the revealed prop appears at its placement without the flight path. evidence: [abilities-detective.md §2026-10-18T11:26:21](../../re/abilities-detective.md#ts-20261018-112621).
- DETECT-S7 — Hand item track: sampled as a step function, the first sample of a clip fires no event, and the rise and fall are not weighted. blocks: DETECT-12. evidence: [abilities-detective.md §2026-10-18T19:30:00](../../re/abilities-detective.md#ts-20261018-193000).
- DETECT-S8 — The glass sits on the bone with the bone's whole bind matrix times the skin matrix (orientation inferred from 0x2ca88c, the per-frame update 0x242814 not read). blocks: DETECT-12. evidence: [abilities-detective.md §2026-10-18T19:30:00](../../re/abilities-detective.md#ts-20261018-193000).

## 5. Open items

- [ ] DETECT-1 — The use state's approach (0x207be4, about 2.3 units in the oracle) and its event table are not read. blocks: DETECT-S1. evidence: [abilities-detective.md §2026-10-18T11:26:21](../../re/abilities-detective.md#ts-20261018-112621).
- [ ] DETECT-3 — Nothing read leaves state 2 of a spot, so `1-2_Dtv_DogSpot_01_Script` never runs (also not in the oracle); confirm in a longer trace. evidence: [abilities-detective.md §2026-10-18T11:26:21](../../re/abilities-detective.md#ts-20261018-112621).
- [ ] DETECT-4 — Not drawn: question particles, footprint rotation, fades, the dust particle and the bone's flight. blocks: DETECT-S2, DETECT-S3, DETECT-14. evidence: [abilities-detective.md §2026-10-18T11:26:21](../../re/abilities-detective.md#ts-20261018-112621).
- [ ] DETECT-5 — Other readers of ext+0x130 bit 2 (camera, prompts, vehicle entry 0x1835d0) and 0x2a62b8 are not ported. evidence: [abilities-detective.md §2026-10-18T11:26:21](../../re/abilities-detective.md#ts-20261018-112621).
- [ ] DETECT-6 — Ability 25 check, the list 0x529c44 and the registry order between object kinds are not modelled. blocks: DETECT-S6. evidence: [abilities-detective.md §2026-10-18T11:26:21](../../re/abilities-detective.md#ts-20261018-112621).
- [ ] DETECT-8 — The same track (0xba1e5b53) shows other items in about 20 clips (hammer, axe, handcuffs, tea cup, fish, blowtorch, wire tap, zipline, whistle); only the glass is drawn (`runtime::HandItemTrack` is generic). blocks: DETECT-S7. evidence: [abilities-detective.md §2026-10-18T19:30:00](../../re/abilities-detective.md#ts-20261018-193000).
- [ ] DETECT-9 — Active mode: clue objects glow faintly blue in the oracle; the cause is not read. evidence: [abilities-detective.md §2026-10-18T19:30:00](../../re/abilities-detective.md#ts-20261018-193000).
- [ ] DETECT-10 — Walking in the mode leaves white dots behind Chase in the oracle; the cause is not read. evidence: [abilities-detective.md §2026-10-18T19:30:00](../../re/abilities-detective.md#ts-20261018-193000).
- [ ] DETECT-11 — Bottom screen: the magnifier button disappears in active mode and a white circle ripples out from the map arrow (about 1.5 s); not read, not drawn. evidence: [abilities-detective.md §2026-10-18T19:30:00](../../re/abilities-detective.md#ts-20261018-193000).
- [ ] DETECT-12 — Hand item events: previous-value handling on a clip start, weighting, the per-frame bone update 0x242814 (glass orientation), the glass during `detect_find` and when the mode ends without `detect_in`. needs: DETECT-S7, DETECT-S8. evidence: [abilities-detective.md §2026-10-18T19:30:00](../../re/abilities-detective.md#ts-20261018-193000).
- [ ] DETECT-13 — Compare the glass pose with the oracle at the same camera and clip frame (the port's camera differs in the shots so far; compared by eye only). blocks: DETECT-S8. evidence: [abilities-detective.md §2026-10-18T19:30:00](../../re/abilities-detective.md#ts-20261018-193000).
- [ ] DETECT-14 — The user sees the marks that detective mode reveals drawn a bit off in `chase-game` (2026-10-18 playtest; position or turn not yet said); the footprints are drawn unrotated (DETECT-S3), not compared with the oracle. needs: DETECT-4. evidence: [abilities-detective.md §2026-10-18T11:26:21](../../re/abilities-detective.md#ts-20261018-112621).

## 6. Findings summary

- Glass and look of detective mode: [abilities-detective.md §2026-10-18T19:30:00](../../re/abilities-detective.md#ts-20261018-193000); classes, footprints, player states and oracle timing: [abilities-detective.md §2026-10-18T11:26:21](../../re/abilities-detective.md#ts-20261018-112621); mission 1-2 gap list: [scripts-missions.md §2026-10-14T21:39](../../re/scripts-missions.md#ts-20261014-213921); oracle states: [oracle-states.md §2026-10-18T10:13:06](../../re/oracle-states.md#ts-20261018-101306).

## 7. Code and tests

- `src/runtime/detective.{hpp,cpp}` (classes, footprints, player states), `src/game_detective.cpp` (LevelSession glue, footprint drawing), `src/game_detective_glass.{hpp,cpp}` (the glass), `src/runtime/hand_item.{hpp,cpp}` (hand item track, `Animation::subAnimationTargets`), RevealSwitch link in `src/runtime/level_start.cpp`, detective walk in `player_locomotion`.
- Tests `hand-item`, `hand-item-real-data`, `detective`, `detective-real-data`, `game-detective-1-2`, `game-doorlock-detective-1-2` (slow), `level-start-real-data`; the idle fix is in `src/runtime/npc_idle.*` (`npc-idle`).
