# Level objects and characters

## 1. Scope

The objects a level places: how the class file supplies model and parameters, which objects are drawn, level characters (minifig body, head, textures, starting pose) and the level start state as seen by drawing. Not covered: collision of objects ([collision.md](collision.md)), studs ([studs.md](studs.md)), doors ([doors.md](doors.md)), the scripts that change objects ([script-system.md](script-system.md)), character animation playback ([animation-characters.md](animation-characters.md)).

## 2. Status

IN_PROGRESS, updated 2026-10-19T19:27:54+02:00. Every model is drawn with its node matrices (LEVOBJ-113 closed) and the plain models of nested class objects are drawn (the crane ladder's upper half). Objects are drawn standing at their level state; characters stand in place; props with a playing clip move by its root motion (the mission 1-2 dog leaves its cage); a cutscene poses the level prop it names by `CSCharacter` (the dog of `1_2_c`, LEVOBJ-100 closed). The animals of kinds 0, 64 and 67 (dogs, fireman cats, crow) are drawn as props (LEVOBJ-7 narrowed); the fireman cats sit (idle clip of their rescue spot; the rescue needs the fireman ability, LEVOBJ-108). Wobbly props and bouncers: [level-objects-wobbly](level-objects-wobbly.md).

## 3. Verified behaviour

- Class files supply `meshname` and transform where the instance sets none (lookup 0x217080, loader 0x2442b4); lvl_00 has 131 and lvl_01 1198 drawable objects. The slot rule matches the data (0 of 5773 exceptions).
- Minifig characters resolve body, head, textures and animation class from class and instance parameters: lvl_00 32 and lvl_01 99 drawn, with the head attached on the `head` joint. Checked by pictures: lvl_00 start (two correctly dressed characters), lvl_01 (Chase and the donut cop `1-1_AI_DonutCop`).
- Animal rigs of kinds 0, 64 and 67 are placed props (lvl_01: `1-2_Dog`, `1-2_Dog_Prop`, four cats, `1-3_Crow`); the released dog follows the root motion of `dog_cage_open` and ends beside Chase where the oracle shows it (A-2 screenshots, by eye).
- Disabling an entity hides its node and its nested objects whatever the kind (0x2428c0): level characters and wheels follow it; the start-visibility parameters (`DisabledOnStartUp` of studs, `ShowOnObjectScan`, `ShowOnMap`) never hide a model ([objects.md §2026-10-19T10:46:42](../../re/objects.md#ts-20261019-104642)).
- Level start state comes from the `startup` script ([script-system.md](script-system.md)), compared with the oracle grid read for collision only.

## 4. Stand-ins

- LEVOBJ-S1 Characters other than pedestrians run the idle state of `char_ai_actor` (verified for `char_ai_actor`; assumed for the other classes, which were not measured); pedestrians stand in frame 0 of `idle_minifig`. needs: PED-2.
- LEVOBJ-S4 The random generator of the idle choice uses a fixed seed; the original seeds from the clock (0x105d8c). evidence: [npc-animation.md §2026-10-15T09:29](../../re/npc-animation.md#ts-20261015-092900).
- LEVOBJ-S88 The cat rescue spots start their idle clip in the first update of the level, whether or not the cat's room is loaded; when the original creates the object and starts the state change is not read. evidence: [use-spot.md §2026-10-19T12:58:16](../../re/use-spot.md#ts-20261019-125816).
- LEVOBJ-S89 The current state of a cat rescue spot starts at 0 (the record is taken as zeroed by its allocation 0x25c030, not checked), so the first update changes to the requested state 1. needs: LEVOBJ-108. evidence: [use-spot.md §2026-10-19T12:58:16](../../re/use-spot.md#ts-20261019-125816).
- LEVOBJ-S90 A prop clip keeps running during a cutscene; the original swaps the entity's animation layers for an empty array at the setup and restores them at the teardown. needs: LEVOBJ-110. evidence: [cutscene-actors-props.md §2026-10-19T13:47:29](../../re/cutscene-actors-props.md#ts-20261019-134729).
- LEVOBJ-S91 A held prop is drawn only while the port's room streaming shows its room; the original gives an actor the room-activity class 5 (active in every room type). needs: LEVOBJ-111. evidence: [cutscene-actors-props.md §2026-10-19T13:47:29](../../re/cutscene-actors-props.md#ts-20261019-134729).
- LEVOBJ-S86 A prop with a `CSCharacter` that has no scene key of its own (not an animation target) or whose cutscene model has no skeleton keeps its placed prop, and the cutscene draws a model of its own beside it. needs: LEVOBJ-106. evidence: [cutscene-actors-props.md §2026-10-19T12:59:54](../../re/cutscene-actors-props.md#ts-20261019-125954).
- LEVOBJ-S2 Everything else with a model is drawn standing still at its placement; props and level characters are hidden while the start state disables them, and the nested objects the port draws (vehicle wheels, LEVOBJ-S93) follow their owner (LEVOBJ-8).
- LEVOBJ-S92 A `HeadModel` / `HatModel` node hangs on its joint with the joint's skin matrix (head node matrix · skin · body world, as 0x10e028 composes a joint-bound node); the consumer of the attach record 0x242814 is not read. evidence: [models.md §2026-10-19T19:24:18](../../re/models.md#ts-20261019-192418).
- LEVOBJ-S93 Of the objects nested in a class file only the plain models drawableObjects keeps are drawn, standing still at child placement · parent placement; nested objects of classes with a `startup` script, nested minifigs and wheels (vehicle path) are left out. needs: LEVOBJ-8. evidence: [object-kinds.md §2026-10-19T19:25:32](../../re/object-kinds.md#ts-20261019-192532).
- LEVOBJ-S3 `props/map_*` markers (mission waypoints, `Prop_Waypoint_*`) are filtered out by an observation-based class-name rule (five lie within 12 units of the player at the gameplay start and none appears in the oracle reference).
- LEVOBJ-S82 Root motion of props applies all four axes (x, y, z, yaw) to every clip (measured 0xf only for `dog_cage_open`) and ignores the test 0x245f84() != 6. needs: LEVOBJ-101. evidence: [objects.md §2026-10-19T10:10:34](../../re/objects.md#ts-20261019-101034).
- LEVOBJ-S83 The root curve at a fractional frame is interpolated linearly between its two samples (the decoder 0x287124 was not read for the fraction). evidence: [objects.md §2026-10-19T10:10:34](../../re/objects.md#ts-20261019-101034).
- LEVOBJ-S84 An animal prop without a playing clip is drawn with the stored vertices of its skinned model; the original's pose of a skeleton without a clip was not compared (the cage hides the dog). evidence: [objects.md §2026-10-19T09:59:14](../../re/objects.md#ts-20261019-095914).

## 5. Open items

- [ ] LEVOBJ-16 — Other NPC states (pedestrian walk, AIMillAbout stop, patrol, gang classes) and the animation of classes not measured. needs: ANIM-5, PED-2. evidence: [npc-animation.md §2026-10-15T09:29](../../re/npc-animation.md#ts-20261015-092900).
- [ ] LEVOBJ-3 — Objects that move, break or react (platforms, scripted objects, NPC walking, mission flow) are frozen at their level state. needs: SCRIPT-1. evidence: [scripts.md §2026-10-13T20:42](../../re/scripts.md#ts-20261013-204236).
- [ ] LEVOBJ-4 — The 8 custom lineup characters (`CharacterEnum` 1..8 through 0x2372e0) and the ride_pig and shark rigs (LEVOBJ-7) are not drawn; the jetpack `AttachModel` placement (bone name hash unresolved, value `chest`) and `AttachRelative`. evidence: [objects.md §2026-10-13T16:34](../../re/objects.md#ts-20261013-163430).
- [ ] LEVOBJ-5 — The code that copies instance parameters into the slot array `obj + 0x2c` was not found (the rule is inferred); how `xx_particleemitter` and other `xx_*` objects are created; model LOD selection (`LOD_NumberofLods`); whether `Animation` start poses change placed props. evidence: [objects.md §2026-10-13T15:39](../../re/objects.md#ts-20261013-153915).
- [ ] LEVOBJ-6 — The police officer NPC of the oracle reference is not identified.
- [ ] LEVOBJ-7 — Classes without a recovered create are not created (shared with COLL-2): among the animals `chars/ride_pig` (kind 159, 0x23e394) and `props/prop_sharkattack` (kind 239, 0x15eec0); the dogs, cats and crow (kinds 0, 64, 67) and the vehicles are done. evidence: [objects.md §2026-10-19T09:59:14](../../re/objects.md#ts-20261019-095914).
- [ ] LEVOBJ-8 — Objects inside class files (sub-objects such as `SBVehicleDropConsole`, `Barrier01`, `Gem`) are not instantiated as entities (only their plain models are drawn, LEVOBJ-S93), so the class `startup` scripts that enable or disable them have no effect. blocks: SCRIPT-52, SCRIPT-S14, LEVOBJ-S93. evidence: [scene-start.md §2026-10-14T14:27](../../re/scene-start.md#ts-20261014-142739).

- [ ] LEVOBJ-110 — Freeze the clip of a prop that is a cutscene actor from the setup to the teardown (0x2d1524 / 0x2d135c swap the layer array). blocks: LEVOBJ-S90. evidence: [cutscene-actors-props.md §2026-10-19T13:47:29](../../re/cutscene-actors-props.md#ts-20261019-134729).
- [ ] LEVOBJ-111 — Room-activity classes of entities (table `*0x376ad4`, `E+0x18`, read by 0x23c1a8 / 0x1dd590 / 0x1ffbd8): the port has none, so a held actor prop in an unloaded room is hidden. blocks: LEVOBJ-S91. evidence: [cutscene-actors-props.md §2026-10-19T13:47:29](../../re/cutscene-actors-props.md#ts-20261019-134729).
- [ ] LEVOBJ-104 — Other readers of flags bit 2 of a parameter record (the lookup 0x217080 and the record reader 0x208ae0 do not use it). evidence: [objects.md §2026-10-19T10:46:42](../../re/objects.md#ts-20261019-104642).
- [ ] LEVOBJ-108 — The rescue itself: the fireman ability (30, test 0x23a87c in message 3 of 0x18e1b8), the player's reaction to message 0x1a6, the writer of the requested states 4, 6, 7, 8 and of the launch arc (+0x58), and the hang of the cat on the player (0x24c118) are not in the port; the HUD count "%u/%u rescuecats" needs the completion. blocks: LEVOBJ-S89. evidence: [use-spot.md §2026-10-19T12:58:16](../../re/use-spot.md#ts-20261019-125816).
- [ ] LEVOBJ-109 — Message 0x58 of the cat spot (0x225cfc, 0x225ca8(*0x51640e)) disables cat and spot, and record +0x64 (0.65) has no reader found. evidence: [use-spot.md §2026-10-19T12:58:16](../../re/use-spot.md#ts-20261019-125816).
- [ ] LEVOBJ-101 — Where an animation record's root axis word +8 is set (record from 0x1dbf0c) for clips other than `dog_cage_open`, and what 0x245f84() returns 6 for (root motion off). blocks: LEVOBJ-S82. evidence: [objects.md §2026-10-19T10:10:34](../../re/objects.md#ts-20261019-101034).
- [ ] LEVOBJ-102 — The create-time clip start of 0x234004 (`Animation`, `AnimationVelocity`, `AnimationOffset` of kinds 0 and 67) is not ported; no lvl_01 object sets it, six objects of other main levels do. evidence: [objects.md §2026-10-19T09:59:14](../../re/objects.md#ts-20261019-095914).
- [ ] LEVOBJ-106 — Other props with a `CSCharacter` (parrot, squirrel, chalkboards, shuttle, hoop, key, safe, pallet, crane lift) when their cutscene runs in the port: they need a scene key of their own or node instances to be posed. blocks: LEVOBJ-S86. evidence: [cutscene-actors-props.md §2026-10-19T12:59:54](../../re/cutscene-actors-props.md#ts-20261019-125954).
- [ ] LEVOBJ-114 — The model draw's node override list (`0x5300f4`, 0x4c-byte records matched by node name that replace or hide a node's matrix) and the lists `0x5300fc` / `0x530104` have no writer found; the port has none. evidence: [models.md §2026-10-19T19:24:18](../../re/models.md#ts-20261019-192418).

## 6. Findings summary

- Every model node is drawn with its node matrix (0x205160 / 0x10e028), heads on their joint's skin matrix: [models.md §2026-10-19T19:24:18](../../re/models.md#ts-20261019-192418); the crane ladder's nested upper half (RENDER-71): [object-kinds.md §2026-10-19T19:25:32](../../re/object-kinds.md#ts-20261019-192532).
- The cat rescue use spot (kind 124; create 0x1829ac, clips 0x182a5c, states 0x1c6aa8, message 3 0x18e1b8): [use-spot.md §2026-10-19T12:58:16](../../re/use-spot.md#ts-20261019-125816).
- A cutscene actor is the entity with the matching `CSCharacter`, props included (the dog of `1_2_c`): [cutscene-actors-props.md §2026-10-19T12:59:54](../../re/cutscene-actors-props.md#ts-20261019-125954).
- A cutscene enables its disabled actor entities and restores their node matrix at the end; byte +0x18 = 5 is a room-activity class: [cutscene-actors-props.md §2026-10-19T13:47:29](../../re/cutscene-actors-props.md#ts-20261019-134729).
- Animals that are props (kinds 0, 64, 67; creates 0x1d1450, 0x15a4b4, 0x199adc through 0x234004 / 0x2442b4): [objects.md §2026-10-19T09:59:14](../../re/objects.md#ts-20261019-095914); root motion of props (0x16b06c, 0x2327b4): [objects.md §2026-10-19T10:10:34](../../re/objects.md#ts-20261019-101034).
- Hidden props (EnableObject, flag bit 0, node hidden) and the police entrance disc: [objects.md §2026-10-14T15:05](../../re/objects.md#ts-20261014-150541).
- Object census, parameter slots, class files (+0x10/+0x14) and the model lookup: [objects.md §2026-10-13T15:39](../../re/objects.md#ts-20261013-153915).
- Characters: body, head, textures, starting pose, custom characters, animation class: [objects.md §2026-10-13T16:34](../../re/objects.md#ts-20261013-163430).
- Studs are pickup sprites, not models: [objects.md §2026-10-13T17:09](../../re/objects.md#ts-20261013-170941).
- Level format, placement Euler order and handedness: [levels.md §2026-10-12T23:05](../../re/levels.md#ts-20261012-2305), [levels.md §2026-10-13T12:49](../../re/levels.md#ts-20261013-1249).

## 7. Code and tests

- `objectPlacements`, `drawableObjects` (`DrawableObjects::characters`), `LevelParam::flags`, `Level::classParams` in `src/assets/level.cpp`; rig code `src/render/character.*`; `chase-game`, `chase-viewer`.
- `drawnPlacements`, `drawnCharacters`, `nestedObjectDrawn` in `src/runtime/object_visibility.*`, `Scene::setInstanceVisible`, glue `src/game_object_visibility.*`.
- `objectCutsceneCharacter` in `src/assets/object_classes.hpp`, `resolveCastTarget` in `src/runtime/cutscene_cast.*`, `levelProps` and `InEngine::pose` in `src/game_cutscene.*`; CTest `cutscene-cast`, `game-cutscene-dog-prop`.
- `assets::nodeDrawMatrix` (src/assets/model.cpp) with `Scene::addModel` / `Scene::setNodeMatrix`; `runtime::nestedProps` (src/runtime/nested_props.*), glue `src/game_nested_props.cpp`; CTest `node-draw-matrix-real-data`, `nested-props-real-data`.
- `createsModelAsProp` in `src/assets/object_classes.hpp`; `src/runtime/prop_root_motion.*` used by `PropAnimations`, glue `src/game_animals.cpp`.
- `src/runtime/cat_rescue.*` (`CatRescueSet`), glue in `src/game_level.cpp` and `src/game_animals.cpp`.
- CTest: `cat-rescue-real-data`, `animal-props-real-data`, `prop-root-motion`, `assets-core`, `assets-real-data`, `game-real-smoke`, `viewer-real-smoke`, `runtime-core`, `level-start-real-data`, `police-entrance-shot` + `police-entrance-no-disc`.
