# Level objects and characters

## 1. Scope

The objects a level places: how the class file supplies model and parameters, which objects are drawn, level characters (minifig body, head, textures, starting pose) and the level start state as seen by drawing. Not covered: collision of objects ([collision.md](collision.md)), studs ([studs.md](studs.md)), doors ([doors.md](doors.md)), the scripts that change objects ([script-system.md](script-system.md)), character animation playback ([animation-characters.md](animation-characters.md)).

## 2. Status

IN_PROGRESS, updated 2026-10-13T21:29:01+0200. Objects are drawn standing at their level state; characters stand in place; nothing moves or reacts.

## 3. Verified behaviour

- Class files supply `meshname` and transform where the instance sets none (lookup 0x217080, loader 0x2442b4); lvl_00 has 131 and lvl_01 1198 drawable objects. The slot rule matches the data (0 of 5773 exceptions).
- Minifig characters resolve body, head, textures and animation class from class and instance parameters: lvl_00 32 and lvl_01 99 drawn, with the head attached on the `head` joint. Checked by pictures: lvl_00 start (two correctly dressed characters), lvl_01 (Chase and the donut cop `1-1_AI_DonutCop`).
- Level start state comes from the `startup` script ([script-system.md](script-system.md)), compared with the oracle grid read for collision only.

## 4. Stand-ins

- LEVOBJ-S1 Characters show frame 0 of `class_generic/idle_minifig`, taken from the pedestrian and character-display code, not the animation the AI state would start.
- LEVOBJ-S2 Everything else with a model is drawn standing still at its placement, including objects that may be hidden or inactive in the original (freeplay `FP_*`, ability props such as ziplines).
- LEVOBJ-S3 `props/map_*` markers (mission waypoints, `Prop_Waypoint_*`) are filtered out by an observation-based class-name rule (five lie within 12 units of the player at the gameplay start and none appears in the oracle reference).

## 5. Open items

- [ ] LEVOBJ-1 — Whether `drawableObjects` should skip the objects the start state disables (visibility at start; not compared with a screenshot); per-class visibility at start (`DisabledOnStartUp`, `ShowOnObjectScan`, `ShowOnMap`) and the effect of flags bit 2 (value 6 in the +0x14 table). evidence: [objects.md §2026-10-13T21:17](../../re/objects.md), [objects.md §2026-10-13T15:39](../../re/objects.md).
- [ ] LEVOBJ-2 — Which animation an AI character plays at level start (the AI classes read many parameters and start animations through the role table `tablesanimations`); which `class_*` folder (police, robber, construction, farmer, fire, space, miner, female) a class selects was not traced. needs: ANIM-2. evidence: [objects.md §2026-10-13T16:34](../../re/objects.md).
- [ ] LEVOBJ-3 — Objects that move, break or react (platforms, scripted objects, NPC walking, mission flow) are frozen at their level state. needs: SCRIPT-1. evidence: [scripts.md §2026-10-13T20:42](../../re/scripts.md).
- [ ] LEVOBJ-4 — The 8 custom lineup characters (`CharacterEnum` 1..8 through 0x2372e0) and the 16 animals (`char_pig`, `char_cat`, `char_dog`, `char_shark`, `char_Crow`, `ride_pig`) are not drawn; the jetpack `AttachModel` placement (bone name hash unresolved, value `chest`) and `AttachRelative`. evidence: [objects.md §2026-10-13T16:34](../../re/objects.md).
- [ ] LEVOBJ-5 — The code that copies instance parameters into the slot array `obj + 0x2c` was not found (the rule is inferred); how `xx_particleemitter` and other `xx_*` objects are created; model LOD selection (`LOD_NumberofLods`); whether `Animation` start poses change placed props. evidence: [objects.md §2026-10-13T15:39](../../re/objects.md).
- [ ] LEVOBJ-6 — The police officer NPC of the oracle reference is not identified.
- [ ] LEVOBJ-7 — Vehicles, animals and the other classes without a recovered constructor are not created (shared with COLL-2).

## 6. Findings summary

- Object census, parameter slots, class files (+0x10/+0x14) and the model lookup: [objects.md §2026-10-13T15:39](../../re/objects.md).
- Characters: body, head, textures, starting pose, custom characters, animation class: [objects.md §2026-10-13T16:34](../../re/objects.md).
- Studs are pickup sprites, not models: [objects.md §2026-10-13T17:09](../../re/objects.md).
- Level format, placement Euler order and handedness: [levels.md §2026-10-12T23:05](../../re/levels.md), [levels.md §2026-10-13T12:49](../../re/levels.md).

## 7. Code and tests

- `objectPlacements`, `drawableObjects` (`DrawableObjects::characters`), `LevelParam::flags`, `Level::classParams` in `src/assets/level.cpp`; rig code `src/render/character.*`; `chase-game`, `chase-viewer`.
- CTest: `assets-core`, `assets-real-data`, `game-real-smoke`, `viewer-real-smoke`.
