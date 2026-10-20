# Script system: use markers

## 1. Scope

The marker disc drawn on the ground at a usable zone (the use markers of the list 0x374640, system 0x172254 / 0x1c015c / 0x1db800 / 0x1b29f0): which entity kinds feed it, its texture, show and hide conditions, placement and draw. The dog cage use spot is in [script-system-missions.md](script-system-missions.md) (SCRIPT-111), the detective spot in [abilities-detective.md](abilities-detective.md).

## 2. Status

IN_PROGRESS, updated 2026-10-20T21:59:12+02:00. Use spots (165) and detective spots (182) show their marker; the texture of every kind of the list is transcribed from 0x292388 (police disc for the detective spot, locked disc when the ability is locked or missing). The detective spot's marker shows in state 0 only and stands where the original puts it (the node of `1-2_Dtv_DogSpot_02`). The other 34 kinds do not feed it yet.

## 3. Verified behaviour

- The detective spot of mission 1-2 shows the police disc with the blue ring at the spot base; the width in the port picture (about 52 px) matches the oracle picture of `footprint-scanner-trigger` (55 px) by eye; the marker goes when the spot enters detective mode (test `game-use-marker-detective-1-2`).
- Kind to ability to texture, the lock rule and the ability tables are pinned by tests `use-marker` and `use-marker-real-data`.

## 4. Stand-ins

- SCRIPT-S247 `kUseMarkerClockUnitsPerSecondStandIn` (60): the clock of the marker spin advanced 2 per frame in the oracle; the multiplier of 0x241098 is not read. needs: SCRIPT-299. evidence: [use-spot.md §2026-10-19T09:25:34](../../re/use-spot.md#ts-20261019-092534).
- SCRIPT-S248 `UseMarkers::step`: the hold of 0.1 s counts down by the update time and then the marker fades out; the fader manager that does it in the original was not found. needs: SCRIPT-299. evidence: [use-spot.md §2026-10-19T09:25:34](../../re/use-spot.md#ts-20261019-092534).
- SCRIPT-S249 `useMarkerGround` and `LevelSession::drawUseMarkers`: the marker stands on the nearest hit of a 5 m segment down from the zone centre (the camera's query), not the ray of 0x1db800 with its object list and step-up; it is drawn with the particle quad state and the render state of 0x1b29f0 is not decoded. needs: SCRIPT-357. evidence: [use-spot.md §2026-10-20T21:58:32](../../re/use-spot.md#ts-20261020-215832).
- SCRIPT-S286 `LevelSession::updateUseMarkers`: only the player character (Chase, id 9) is asked for the ability of a kind; the party list 0x51b248, the case of the player entity of kind 159 and the cheat byte 0x375961 bit 5 of 0x1be6c0 are not modelled. needs: SCRIPT-356. evidence: [use-spot.md §2026-10-20T21:58:32](../../re/use-spot.md#ts-20261020-215832).
- SCRIPT-S287 `LevelSession::updateUseMarkers`: only use spots (165) and detective spots (182) feed the markers; the message 3 answer of the other 34 kinds is not read. needs: SCRIPT-299. evidence: [use-spot.md §2026-10-20T21:58:32](../../re/use-spot.md#ts-20261020-215832).
- SCRIPT-S288 `LevelSession::updateUseMarkers`: every spot of the level counts as listed; 0x1c015c lists an entity only when its position passes the box test 0x2397a0 against the boxes of `*0x376c78`. needs: SCRIPT-356. evidence: [use-spot.md §2026-10-20T21:58:32](../../re/use-spot.md#ts-20261020-215832).

## 5. Open items

- [ ] SCRIPT-299 — Feed the markers of the other 34 entity kinds of the list 0x374640 (carry objects, build-its, the reveal-like and ability props): each kind's message 3 answer, the u32 ability fields of kinds 58 (+0x60) and 185 (+0x50) and 0x1fa330 for kind 201 (their textures are in 0x292388). blocks: SCRIPT-355, SCRIPT-S247, SCRIPT-S248, SCRIPT-S287. uses: prop_simpleuse, ability_detective. evidence: [use-spot.md §2026-10-20T21:58:32](../../re/use-spot.md#ts-20261020-215832).
- [ ] SCRIPT-355 — The special cases of 0x172254 for kinds 58, 150, 183 and 210 and the `HeadMarker` path (ability `HeadMarker + 24`, 0x22d7c0) are not ported. needs: SCRIPT-299. evidence: [use-spot.md §2026-10-19T09:25:34](../../re/use-spot.md#ts-20261019-092534).
- [ ] SCRIPT-356 — The list builder test 0x2397a0 against the boxes of `*0x376c78` and the rebuild when it changes, the party list 0x51b248 and the kind 159 case of 0x292388. blocks: SCRIPT-S286, SCRIPT-S288. evidence: [use-spot.md §2026-10-20T21:58:32](../../re/use-spot.md#ts-20261020-215832).
- [ ] SCRIPT-357 — The placement ray 0x1db800 (objects of the box 0x237f30, dropped kinds, 0x204764 / 0x22a7a4 with the 0.5 m step-up) and the render state 0x1b29f0 hands to 0x25511c. blocks: SCRIPT-S249. evidence: [use-spot.md §2026-10-20T21:58:32](../../re/use-spot.md#ts-20261020-215832).

## 6. Findings summary

- Textures, list, update, fader, placement, draw and the oracle run for use spots: [use-spot.md §2026-10-19T09:25:34](../../re/use-spot.md#ts-20261019-092534).
- Texture function 0x292388, ability tables, the detective spot's answer and the oracle state: [use-spot.md §2026-10-20T21:58:32](../../re/use-spot.md#ts-20261020-215832); state: [oracle-states.md §2026-10-20T21:58:04](../../re/oracle-states.md#ts-20261020-215804).

## 7. Code and tests

- `src/runtime/use_marker.{hpp,cpp}` (`UseMarkers`, kind list, ability and texture tables), `src/runtime/ability_table.{hpp,cpp}`, `src/runtime/use_marker_ground.{hpp,cpp}`, glue `src/game_use_spot.cpp`; tests `use-marker`, `use-marker-real-data`, `use-spot-real-data`, `game-use-spot-1-2`, `game-use-marker-detective-1-2`.
