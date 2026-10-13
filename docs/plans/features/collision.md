# Collision

## 1. Scope

Level collision data (model +0x1c section), the ground probe by box bisection, wall sliding, and placed-object collision (world grid, registration, shapes, query layers) for the player and the camera. Not covered: the player rules that use the result ([player-movement.md](player-movement.md)), door motion ([doors.md](doors.md)), the start state of objects ([script-system.md](script-system.md)).

## 2. Status

IN_PROGRESS, updated 2026-10-13T21:29:01+0200. Level collision and placed objects block the player and the camera in `chase-game`.

## 3. Verified behaviour

- Ground height and wall slide: `collision-real-data` replays 104 traces (4104 updates, 249 height changes, 0 wrong) and 35 wall traces (98 updates decided by objects, 0 wrong; 2 millimetre-contact differences in the first 24).
- Placed objects: world grid DAT_00578dfc, registration (0x30e0d8 / 0x2ba3f4 / 0x23aff8), query layers (player {3, 0x90}, camera {all, 0x30}).
- Start state from the level scripts replaces the 85-name list: lvl_01 824 entries, 414 meshes; 83 of the 85 names follow from the rule.
- Automatic boxes use the node matrices (bounds routine 0x242eb0): 405 of 406 auto and class boxes agree with the grid read within 2e-3.

## 4. Stand-ins

- COLL-S1 Broad phase by world boxes instead of 0x2335e4 / the local tree query (shared with CAMERA-S1).
- COLL-S2 The start state takes new-game values and treats waits as elapsed — owned by [script-system.md](script-system.md) (SCRIPT-S1, SCRIPT-S2).
- COLL-S3 An open door keeps the closed box (see DOOR-3).
- The former stand-ins `kStateDisabledStandIn` and `kDoorCentreXStandIn` are gone (start state from the level scripts and revealer links; box corners by the node matrices).

## 5. Open items

- [ ] COLL-1 — Broad phase by world boxes instead of 0x2335e4 / local tree query. evidence: [collision.md §2026-10-13T17:34](../../re/collision.md).
- [ ] COLL-2 — Only the classes with a recovered constructor rule register: vehicles (mask 6, flags 0x1341 / 0x1340), characters (mask 4), prop_prop and about 50 other constructors are missing; `FP_Sky_Dive_02` of props/parkour_skydive has another box (centre z 1.8271 against 1.7145). blocks: CAMERA-2. evidence: [collision.md §2026-10-13T20:53](../../re/collision.md).
- [ ] COLL-3 — The 100 grid entries without a level object (for example 24 boxes 0.21 x 1.0 x 0.18 with mask 6) and the boxes with mask 38. evidence: [collision.md §2026-10-13T17:34](../../re/collision.md).
- [ ] COLL-4 — What sets owner flag bit 0 and whether it changes while playing; why the master model has 153 items but 147 + 7 static entries; the box of Police_Door_02 and of the girders; the real surface byte of the box triangles (0 assumed). blocks: DOOR-3. evidence: [collision.md §2026-10-13T17:34](../../re/collision.md).
- [ ] COLL-5 — Whether the ground probe stands on box tops as the replica does (no trace of a character on a crate). evidence: [collision.md §2026-10-13T17:34](../../re/collision.md).
- [ ] COLL-6 — Unexplained: `door_back` rows 125/128/130 (static contact near (901.8, 1096)), 153 master-model items vs 147 + 7 static entries.
- [ ] COLL-7 — Collision with parked cars and people, and objects that break or move, is not replicated (cars and people fall under COLL-2).

## 6. Findings summary

- Collision data layout, ground probe (0x20b48c / 0x1f9884 / 0x2023e4) and wall slide (0x21a73c / 0x208d84 / 0x2092d8): [collision.md §2026-10-13T14:45](../../re/collision.md).
- Placed-object grid, shapes and layers: [collision.md §2026-10-13T17:34](../../re/collision.md).
- Corrections that replaced the stand-ins: [collision.md §2026-10-13T20:43](../../re/collision.md), [collision.md §2026-10-13T20:53](../../re/collision.md).
- The model +0x1c section is collision geometry: [models.md §2026-10-13T14:46](../../re/models.md).

## 7. Code and tests

- `src/runtime/collision.*`, `src/assets/object_collision.*`, `CollisionWorld::addObject`, `objectCollisions` (chase_assets).
- CTest: `collision-real-data`, `level-start-real-data`.
- Trace dirs: `CHASE_COLLISION_TRACE_DIR`, `CHASE_OBJECT_COLLISION_TRACE_DIR`. Scenarios: `tools/oracle/coll-wall*.txt`, `objcoll-walk.txt`, `objcoll-door.txt`, `objcoll-barrier.txt`.
