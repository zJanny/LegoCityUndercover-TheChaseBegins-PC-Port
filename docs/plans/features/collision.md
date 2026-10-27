# Collision

## 1. Scope

Level collision data (model +0x1c section), the ground probe by box bisection, wall sliding, and placed-object collision (world grid, registration, shapes, query layers) for the player and the camera. Not covered: the player rules that use the result ([player-movement.md](player-movement.md)), door motion ([doors.md](doors.md)), the start state of objects ([script-system.md](script-system.md)).

## 2. Status

IN_PROGRESS, updated 2026-10-27T19:14:39+01:00. Level collision, placed objects and the objects class files nest (the helipad of mission 1-3: plate and built prop) block the player and the camera in `chase-game`.

## 3. Verified behaviour

- Ground height and wall slide: `collision-real-data` replays 104 traces (4104 updates, 249 height changes, 0 wrong) and 35 wall traces (98 updates decided by objects, 0 wrong; 2 millimetre-contact differences in the first 24).
- Placed objects: world grid DAT_00578dfc, registration (0x30e0d8 / 0x2ba3f4 / 0x23aff8), query layers (player {3, 0x90}, camera {all, 0x30}).
- Start state from the level scripts replaces the 85-name list: lvl_01 824 entries, 414 meshes; 83 of the 85 names follow from the rule.
- The helipad of mission 1-3: the plate (kind 98) and the built prop (kind 67, nested) register as the oracle grid has them (box 6.096 x 0.106, item box 7.622 x 1.906, owner flags before and after the build); the holder and the fast travel pad register nothing: [collision-nested.md §2026-10-27T19:14:39](../../re/collision-nested.md#ts-20261027-191439).
- Automatic boxes use the node matrices (bounds routine 0x242eb0): 405 of 406 auto and class boxes agree with the grid read within 2e-3.

## 4. Stand-ins

- COLL-S1 Broad phase by world boxes instead of 0x2335e4 / the local tree query (shared with CAMERA-S1).
- COLL-S2 The start state takes new-game values and treats waits as elapsed — owned by [script-system.md](script-system.md) (SCRIPT-S1, SCRIPT-S2).
- COLL-S3 An open door keeps the closed box (see DOOR-3).
- COLL-S4 Doors of disabled objects are not updated; whether the original updates a disabled entity is not read. needs: COLL-21. evidence: [collision.md §2026-10-15T07:58](../../re/collision.md#ts-20261015-075843).
- COLL-S5 The children of a class with a `startup` script and the hand items nested in the player (kind 65) get no collision entry (their start state is not applied, as for the drawn nested props LEVOBJ-S93). needs: LEVOBJ-8, COLL-24. evidence: [collision-nested.md §2026-10-27T19:14:39](../../re/collision-nested.md#ts-20261027-191439).
- COLL-S6 A nested object counts as disabled while any ancestor is; the original's rule for the descendants of a disabled object is not read. needs: COLL-24. evidence: [collision-nested.md §2026-10-27T19:14:39](../../re/collision-nested.md#ts-20261027-191439).
- COLL-S7 The nesting depth of the nested-object registration is capped at 8 by the port; the original's bound was not read. evidence: [collision-nested.md §2026-10-27T19:14:39](../../re/collision-nested.md#ts-20261027-191439).
- The former stand-ins `kStateDisabledStandIn` and `kDoorCentreXStandIn` are gone (start state from the level scripts and revealer links; box corners by the node matrices).

## 5. Open items

- [ ] COLL-1 — Broad phase by world boxes instead of 0x2335e4 / local tree query. evidence: [collision.md §2026-10-13T17:34](../../re/collision.md#ts-20261013-173400).
- [ ] COLL-2 — About 40 other constructor kinds register with their own arguments and are not transcribed (census of lvl_01 in the entry), and characters (mask 4) and vehicles' other rules are missing; `FP_Sky_Dive_02` of props/parkour_skydive has another box (centre z 1.8271 against 1.7145). blocks: CAMERA-2, COLL-23. evidence: [collision-nested.md §2026-10-27T19:14:39](../../re/collision-nested.md#ts-20261027-191439).
- [ ] COLL-3 — The 100 grid entries without a level object (for example 24 boxes 0.21 x 1.0 x 0.18 with mask 6) and the boxes with mask 38. blocks: COLL-25. evidence: [collision.md §2026-10-13T17:34](../../re/collision.md#ts-20261013-173400).
- [ ] COLL-4 — What sets owner flag bit 0 and whether it changes while playing; why the master model has 153 items but 147 + 7 static entries; the box of Police_Door_02 and of the girders; the real surface byte of the box triangles (0 assumed). blocks: DOOR-3. evidence: [collision.md §2026-10-13T17:34](../../re/collision.md#ts-20261013-173400).
- [ ] COLL-5 — Whether the ground probe stands on box tops as the replica does (no trace of a character on a crate). evidence: [collision.md §2026-10-13T17:34](../../re/collision.md#ts-20261013-173400).
- [ ] COLL-6 — Unexplained: `door_back` rows 125/128/130 (static contact near (901.8, 1096)), 153 master-model items vs 147 + 7 static entries.
- [ ] COLL-7 — Collision with parked cars and people, and objects that break or move, is not replicated (cars and people fall under COLL-2); objects that appear or change are covered only by the owner flags (the helipad build is done).
- [ ] COLL-23 — Transcribe the collision registration arguments and the follow-up calls of each other constructor kind (list in COLL-2) and add the kinds the main levels place, kinds 64 and 228 first (42 of 56 objects missing in lvl_01). needs: COLL-2. evidence: [collision-nested.md §2026-10-27T19:14:39](../../re/collision-nested.md#ts-20261027-191439).
- [ ] COLL-24 — Read the enable and disable recursion over the descendants and the start state of nested objects (startup scripts, `DisabledOnStartUp`-like parameters, the hand items of the player) from the oracle grid, then drop COLL-S5 and COLL-S6. blocks: COLL-S5, COLL-S6. evidence: [collision-nested.md §2026-10-27T19:14:39](../../re/collision-nested.md#ts-20261027-191439).
- [ ] COLL-25 — Compare the nested entries of lvl_01 (44) with the 100 grid entries without a level object of COLL-3 and read the kind 1 entry's item root node from the oracle. needs: COLL-3. evidence: [collision-nested.md §2026-10-27T19:14:39](../../re/collision-nested.md#ts-20261027-191439).
- [ ] COLL-21 — Pin the player-into-police-car stop as an oracle trace with the port camera at the same yaw (the 3.06 / 3.22 unit stop agrees only roughly), and read 0x242b08 to settle COLL-S4. blocks: COLL-S4. evidence: [collision.md §2026-10-15T07:58](../../re/collision.md#ts-20261015-075843); screenshots: [collision.md §2026-10-15T10:50](../../re/collision.md#ts-20261015-105024).

## 6. Findings summary

- EnableObject / EnableCollision during play only change owner flags, the grid is filled once: [collision.md §2026-10-15T07:58](../../re/collision.md#ts-20261015-075843).
- Collision data layout, ground probe (0x20b48c / 0x1f9884 / 0x2023e4) and wall slide (0x21a73c / 0x208d84 / 0x2092d8): [collision.md §2026-10-13T14:45](../../re/collision.md#ts-20261013-144500).
- Helipad plate, built prop and nested objects: [collision-nested.md §2026-10-27T19:14:39](../../re/collision-nested.md#ts-20261027-191439).
- Placed-object grid, shapes and layers: [collision.md §2026-10-13T17:34](../../re/collision.md#ts-20261013-173400).
- Corrections that replaced the stand-ins: [collision.md §2026-10-13T20:43](../../re/collision.md#ts-20261013-204340), [collision.md §2026-10-13T20:53](../../re/collision.md#ts-20261013-205344).
- The model +0x1c section is collision geometry: [models.md §2026-10-13T14:46](../../re/models.md#ts-20261013-144637).

## 7. Code and tests

- `src/runtime/collision.*`, `src/assets/object_collision.*`, `CollisionWorld::addObject`, `objectCollisions` (chase_assets).
- `src/runtime/collision_enable.*` (owner flags per update), `objectCollisionCandidates`, `nestedObjectCollisionCandidates` (the objects class files nest; kind 98 rule).
- CTest: `collision-enable`, `collision-real-data`, `level-start-real-data`, `nested-object-collision-real-data`, `game-helipad-collision` (slow).
- Recorded rows: `tests/data/collision_ground_samples.inc`, `tests/data/collision_wall_samples.inc` (provenance in their headers). Scenarios: `tools/oracle/coll-wall*.txt`, `objcoll-walk.txt`, `objcoll-door.txt`, `objcoll-barrier.txt`.
