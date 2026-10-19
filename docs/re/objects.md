# Level objects

How the .blvl objects (section +0x2c) find their model and transform. Format of the level file: [levels.md](levels.md).

## Current state

- A class `N` is the file `levels/N.blvl`; its two parameter tables (12-byte records, hash/flags/type/value) are read by the lookup 0x217080, and an instance parameter the class does not list does not exist. [§2026-10-13T15:39](#ts-20261013-153915)
- The model path is `models/<dir>/<leaf>.bfnmdl` from `meshname` (loader 0x2442b4); `initialposition` and `initialorientation` (degrees, matrix 0x237c48) place it. [§2026-10-13T15:39](#ts-20261013-153915)
- Minifig characters are drawn with body, head joint, textures replacing the `*_TEST.tga` names; the pose is frame 0 of `class_generic/idle_minifig` (stand-in). [§2026-10-13T16:34](#ts-20261013-163430)
- Studs are pickup sprites, not models; they are not counted as model-less. [§2026-10-13T17:09](#ts-20261013-170941), [studs.md §2026-10-13T17:09](studs.md#ts-20261013-170941)
- Custom characters (`CharacterEnum` 1..8, 0x2372e0) take their look from the saved custom character, not from level data; they are not drawn. [§2026-10-13T16:34](#ts-20261013-163430)
- The start state (which objects are disabled) comes from the `startup` script and the reveal links, not from a flag in the .blvl. [§2026-10-13T21:17](#ts-20261013-211741)
- EnableObject sets entity flag bit 0 and hides the node (0x23f734, flags 0x60); a disabled object is not drawn (127 of 1198 hidden at lvl_01 start, e.g. `Police_Plaque_Floor`). [§2026-10-14T15:05](#ts-20261014-150541)
- Animal rigs of kinds 0, 64 and 67 (the mission 1-2 dogs `1-2_Dog` / `1-2_Dog_Prop`, the fireman cats, the crow) are props: their create builds the model with 0x2442b4 and starts only an `Animation` parameter (0x234004), which none of them has; ride_pig (159) and the sharks (239) have creates of their own. [§2026-10-19T09:59:14](#ts-20261019-095914)
- A prop with a playing clip moves by the clip's root motion (prop update 0x16b06c with 0x2327b4); the released dog leaves the cage and ends where the oracle trap has its node (915.2977 64.3605 663.6295, yaw -87.64). [§2026-10-19T10:10:34](#ts-20261019-101034)
- Open: per-class start visibility (`DisabledOnStartUp`, `ShowOnObjectScan`, flags bit 2), hidden characters still drawn, NPC start animations, the pose of a skinned model without a clip, the root-motion axis bits of a clip, the ride_pig and shark creates, the scene code that skips nodes with 0x60.

<a id="ts-20261013-153915"></a>

## 2026-10-13T15:39:15+02:00 — Object census, parameter slots and the model lookup

Corrected by [objects.md §2026-10-13T16:34](#ts-20261013-163430): minifig characters are drawn
Corrected by [objects.md §2026-10-13T17:09](#ts-20261013-170941): studs are pickup sprites, not models
Corrected by [studs.md §2026-10-13T17:09](studs.md#ts-20261013-170941): the stud classes are pickup objects, never loaded as models

**Census (C++ parser, parameter names from code.bin strings, local only).** Classes with counts, `levels/levels/lvl_00_policestation/lvl_00_policestation_main.blvl` (350 objects) and `levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl` (2967 objects; the gameplay-start state of the oracle stands in this level, player at (886.8, 16.6, 1111.2)).

| Class | lvl_00 | lvl_01 | Model |
|---|---|---|---|
| leprops/leprop_wobblydestructible | 96 | 674 | instance `meshname` (95 of 96 in lvl_00); else class default `Prop_Mech_CrateDefault` |
| leprops/leprop_simpleindestructible, simpledestructible, prop_respawnabledestructible, prop_buildit, prop_door, prop_platform, dummy/dummymesh, ability_*, parkour_* | 51 | see drawable count below | instance `meshname`, else class default |
| vehicles/vehicle_* | 0 | 27 | instance `meshname` (also class default `Veh_*`) |
| pickups/pickup_stud{silver,gold,blue,purple}, lepickups | 97 | 1040 | class default `xx_silverstud`, `xx_goldstud`, ... : no model in the archive |
| chars/char_ai_*, pedestrian, ride_pig, props/ability_freeplay_astronaut_alien01 | 40 | 100 | class default `char_minifig` / `char_pig` (skeletal) |
| props/map_*waypoint* | 0 | 44 | class default `Prop_Waypoint_*` (mission markers) |
| scriptobject, point, dummy, dummyrunto, scenechange, lighting/*, soundobject, particles, xx_skipto, ... | rest | rest | no `meshname` parameter, or `xx_particleemitter` (not in the archive) |

Per-class parameter lists come from a scratch tool over the C++ parser (not committed); `tools/convert_levels.py --param-names artifacts/game/code.bin <level>` lists the instances. Parameters used per class include `initialposition` (float3), `initialorientation` (float3, degrees), `meshname` (string) plus class-specific ones (`TriggerObject`, `DebrisColour`, `Respawn`, `SpawnDir`, `HeadModel`, `GrappleTarget`, ...).

**Class files.** An object's class name N is the file `levels/N.blvl` (parsed by 0x1cfc50, which loads `<name>.blvl`). Its header sections +0x10 and +0x14 are parameter tables of 12-byte records `{u32 name hash, u16 flags, u16 type, u32 value}` (the flags at +4 were previously skipped; `LevelParam::flags`). Over all 17368 objects of the 856 `levels/levels/*.blvl`:
- every object whose instance has a `meshname` parameter has one in its class tables, with flags bit 1 set (instance count 5773, 0 exceptions);
- 7496 objects have no instance `meshname` and take the class value;
- all class `meshname` records have flags 2 (+0x10 table) or 6 (+0x14 table).

**Lookup 0x217080** (obj, name hash, ...), ev_94b33d89…c06c475 (REA, Ghidra pseudo-code, `procedure_pseudo_code 0x217080`):
- the object's class data is `*(obj + 0x24)`; it walks the +0x10 table (count u16 at class+2), then the +0x14 table (count at class+4);
- on a hash match: flags bit 1 returns `*(obj + 0x2c) + n * 4` (the +0x14 table uses `obj + 0x30`), where n counts the preceding flag-bit-1 entries of the same table, i.e. the per-instance value slot; flags bit 0 returns the inline record value (+8);
- a name that no table lists returns 0 (not found). So an instance parameter the class does not list does not exist for the object.

**Model loader 0x2442b4** (ev_431e9702…a015, REA pseudo-code): `meshname` is read with 0x217080 (hash from 0x24da28 of the lower-cased string). If the string is empty or absent, nothing is loaded. Path rule: `"models/" + meshname`; a meshname without `/` gets `/` and its own name appended, then the leaf after the last `/` or `\` plus ".fnmdl" is added (the archive stores `.bfnmdl`): `models/<dir>/<leaf>.bfnmdl`, which is `meshModelPath`. The model is loaded by 0x2123cc (flag 2 when object bit 0x10 is set, otherwise 0) and stored at obj+0x3c; the texture directory comes from 0x212420 with the path cut at the last `/`. Attachments follow: `AttachModel01..04` with `AttachBone%02d`, `AttachRelative%02d`, then `AttachModel`, `HeadModel`, `HatModel` (a second model on a head or hat joint) and `lightexcludemask`.
- The prop class constructor 0x1a6900 (ev_9d1d7c2b…25ba5) calls 0x241624(obj, "meshname") and, when set, 0x234004, which calls 0x2442b4 and then plays an `Animation` parameter on the model; with no meshname it creates a non-model node (0x3c).

**Transform.** `initialposition` and `initialorientation` use the same lookup (levels.md: orientation degrees × 0.0174533, matrix 0x237c48). When the class lists them (all listed ones have flags 2 or 6) the value is the instance's, else the class default (zero in the data seen). When the class does not list them they are absent, i.e. identity.

**Implemented.** `chase::assets::objectPlacements(level, classOf)` applies this lookup; `drawableObjects(archive, level)` keeps objects whose model exists in the archive. Both chase-game and chase-viewer use it instead of `placements()` (instance `meshname` only).
- lvl_00: 131 drawable (130 old + 1: `Portrait_Box01` takes the class default `Prop_Mech_CrateDefault`), 98 without model (46+48+3 studs, 1 emitter), 40 skeletal, 0 markers.
- lvl_01: 1198 drawable, 1051 without model, 115 skeletal, 44 markers. About 110 are new (class-default models only): ziplines (24), freeplay ATMs (8), drill holes (7), catapults (4), gems (4), repair spots, safe panels, ladders, fires.
- All 856 levels: 7657 drawable, 5258 without model, 886 skeletal, 405 markers. Tests: `assets-core` (synthetic class/instance cases) and `assets-real-data`.

**Left out and why**
- `char_*` meshes (NPCs, animals, `ride_pig`, minifig props): skeletal models that need rigs, head/face/hat attachments and animation; drawn in bind pose they look wrong (an earlier version drew a few, e.g. `char_Crow`, `char_cat`).
- Studs (`xx_*stud`), `xx_particleemitter`, `xx_HintMesh*`: the archive has no models of those names; the game must create them differently (not traced).
- `props/map_*` classes (mission waypoints, beacon meshes `Prop_Waypoint_*`): at the gameplay-start position five of them lie within 12 units of the player, and none appears in `artifacts/oracle/ref/gameplay-start.png`. They are hidden at the start; what shows them (mission script) is not traced. The class-name rule is an observation-based approximation.
- Everything else with a model is drawn standing still at its placement, including objects that may be hidden or inactive in the original until a script or mode shows them (freeplay `FP_*`, ability props such as ziplines). Whether each is visible at the start is not verified.

**Visual check.** `chase-game --level lvl_01 --frames 2 --screenshot`: before this change the lvl_01 start view had no waypoint beacons; with markers included, a teal pillar and green boxes appeared in front of the player and were not in the reference; after the marker filter the view is identical to before. A freeplay ATM (`FP_Task_ROBBER_CTH_06`, 801.86, 17.03, 1053.80) appears on the pavement in `chase-viewer --level ... --eye 815,19,1060 --yaw 245`.

**Unknowns**
- The code that copies instance parameters into the slot array `obj + 0x2c` was not found (the rule is inferred from lookup 0x217080 and from the data: 0 of 5773 exceptions).
- Per-class visibility at start (hidden until triggered, `DisabledOnStartUp`, `ShowOnObjectScan`, `ShowOnMap`) and the effect of flags bit 2 (value 6 in the +0x14 table).
- How studs, particle emitters and other `xx_*` objects are created; model LOD selection (`LOD_NumberofLods`); whether `Animation` start poses change placed props.
- Orientation parameters of other classes were assumed to use 0x237c48 as well (see levels.md).

<a id="ts-20261013-163430"></a>

## 2026-10-13T16:34:30+02:00 — Characters in the level: body, head, textures and starting pose (worker; REA Ghidra 12.1.2 pseudo-code, objdump, class and instance dumps through parseLevel)

Corrected by [objects.md §2026-10-13T21:17](#ts-20261013-211741): the start state comes from the startup script and reveal links
Corrected by [objects.md §2026-10-19T09:59:14](#ts-20261019-095914): the dog, cat and crow rigs of lvl_01 are props of kinds 0, 64 and 67 and are drawn

Replica: `drawableObjects` / `CharacterObject` in src/assets/level.cpp, `addLevelCharacters` in src/render/character.cpp (shared with the player through `addCharacter` / `poseBody` / `placeCharacter`; chase-game and `chase-viewer --characters`). Tests: `assets-real-data` (counts and three resolved rigs).

**[Corrects objects.md §2026-10-13T15:39:15](#ts-20261013-153915):** the "Left out and why" entry on `char_*` meshes of the earlier section no longer holds for minifig characters (drawn now, see below); animals and the custom characters stay out.

**Census (all objects whose resolved meshname is a `char_*` model).** lvl_00: 40 `chars/char_ai_actor`, all `char_minifig`. lvl_01: 115 = 99 `char_minifig` (char_ai_actor 25, char_ai_cherrytreehillsgang1..4 46, char_ai_goon_fists 3, char_ai_tonyknucklesmcgee 2, char_boosterstrange 2, deputydunby 1, journalist 1, racecardriver 1, `chars/pedestrian` 14, `props/ability_freeplay_astronaut_alien01` 4) + 16 other rigs (`char_pig` 5 for `chars/ride_pig`, `char_Crow`, `char_cat` 4, `char_dog` 2, `char_shark` 4). All 856 levels: 886 = 708 minifig characters drawn + 8 custom + 170 other rigs.

**Appearance parameters (confirmed from data, resolved with the existing lookup rule).** The body is `meshname` (`char_minifig` for every minifig class). `HeadModel`, `FaceTexture`, `UpperTexture` and `LowerTexture` are slot parameters (flags 2) of the character classes with the TEST placeholders in `chars/char_ai_actor` (`Char_Head_TEST`, `head_TEST.tga`, `upper_TEST.tga`, `lower_TEST.tga`) and real values in specific classes (`char_police`, `char_ai_robber`, `char_ai_cherrytreehillsgang1` ...). An instance overrides them (lvl_00 Lobby_Cop02: `Char_Head_PoliceHat`, `head_MalePolice01DIFF64_c4x4.tga`, `lower_Lego_BlackDIFF64.tga`, `upper_CopJacketDIFF128.tga`; a class value is taken when the instance sets none, e.g. lvl_01 `1-3_Dumby`). `chars/pedestrian` keeps all of them only in the +0x14 table (flags 6), so the instances override per pedestrian. The shared body model carries `lower_TEST.tga`/`upper_TEST.tga` and the head `head_TEST.tga`, which these names replace (as for the player, player.md).
- The head is a standalone model attached on the `head` joint, hats are mostly part of the head model (`Char_Head_HatFireman_Gray`, `Char_Head_HatKnitted_Black`, ...). 0x2442b4 attaches `HatModel` on the `hat` joint, but no class file under levels/chars and levels/props (604 files) and no instance in lvl_00 and lvl_01 lists a `HatModel` parameter (name hash from code.bin strings); the other levels' instances were not scanned. `AttachModel` (+ one unnamed string parameter holding the bone, value `chest`) occurs in 5 of those classes (jetpacks: char_ai_evil_astronaut*, char_evilastronaut, char_ai_flashhotshotjohnson*); none of the objects of lvl_00 and lvl_01 uses them, and they are not drawn. `AttachModel01..04` / `AttachBone%02d` / `AttachRelative%02d` have no resolvable names (built with a format string), so they were not looked for.
- Head and hat placement: translate(joint bind position) · skin(joint) · object matrix (`jointAttachment`, player.md). The object matrix is `placementMatrix` (rows Rz·Ry·Rx, translation = initialposition); no change of the placement height was needed (the objects stand with their feet at the listed y, matching the player start convention; checked by eye in lvl_00 and lvl_01 screenshots).

**Custom characters (confirmed in code).** 0x2372e0 (ev_1248d66a132a0847623994851c25a593af5716d3aacb13854128039cf851f397) reads `CharacterEnum` and handles values 1..8 only (`7 < enum - 1` returns): it copies a 6-byte default record (table at 0x520800), optionally `CustomChar.bin`, and writes `MeshName` (`char_minifig`, `_small`, `_skirt`, `_small_skirt`, `_pegleg`, `_small_pegleg` from record bits), `HeadModel`, `FaceTexture`, `UpperTexture` and `LowerTexture` into the object's parameter slots. So the eight `Lineup_0N_*` actors of lvl_00 (CharacterEnum 1..8, class `chars/char_ai_actor`, no textures in the instance) get their look from the saved custom character, not from the level data. They are not drawn. The other body variants (`char_minifig_small`, `_tall`, `_pegleg`, `_skirt`) appear only in this path and in 0x2270bc / 0x237950; no level object uses them as `meshname`.

**Animation class (partly confirmed).** The pedestrian constructor 0x1913f8 (ev_67e5a4890e56176dfab1f1d86400f3985a9d4f80d1eefd5852ef1de89637051c) builds the model with 0x2442b4 and then pushes the directory `models/char_minifig/<AnimationClass>/` and loads `walk_minifig`, `dodge_*`, `bump*`, `splat*` from it; the class value is `CLASS_Generic` (the folder `class_generic` in the archive). The character-select display 0x27a2bc (ev_08172dbe982eb008db597d1059d884ec911e59b7a780ca3fe31fcbcd24153abf) builds body, `head` joint attachment, upper/lower/face textures replacing the `*_TEST.tga` names (the same replacement rule) and plays `CLASS_Generic/idle_minifig` from frame 0. The implementation draws every minifig character in frame 0 of `models/char_minifig/<animationClass>/idle_minifig.bfnanm`, where animationClass is the `AnimationClass` parameter or `class_generic`. Counts: lvl_00 32 drawn + 8 custom skipped; lvl_01 99 drawn + 16 other rigs skipped (`DrawableObjects::characters` / `customCharacters` / `skeletal`).

**Checked.** chase-game lvl_00 start: two characters visible (blond hair at the reception desk, orange hair at a desk), correctly dressed; lvl_01: Chase and the donut cop (`1-1_AI_DonutCop`, 883.3 / 16.58 / 1123.2) at his stand with head and shirt textures (viewer close-up). The police officer NPC of the oracle reference is not identified.

Unknowns:
- Which animation an AI character actually plays at level start. The AI classes (char_ai_*) read many AI parameters (AIMillAbout, AISimplePatroller, ...) and their states start the animations through the role table (tablesanimations); `class_generic/idle_minifig` frame 0 is a stand-in taken from the pedestrian and character-display code. Which `class_*` folder (police, robber, construction, farmer, fire, space, miner, female) a class selects was not traced (the CharacterEnum row of tablescharacters has columns that may encode it). The 3DS reference NPCs may stand in other poses.
- Whether objects are hidden at the start (script-activated actors such as `1-3_*` cutscene actors in room7, `DisabledOnStartUp`); all resolved objects are drawn.
- The appearance of the custom-character lineup, the rigs of animals (`char_pig`, `char_cat`, `char_dog`, `char_shark`, `char_Crow`), the jetpack `AttachModel` placement (bone parameter name unresolved: its hash is not among the code.bin strings; value `chest`), and `AttachRelative`.
- Playback of NPC animations (not implemented; the player's playback rate is a stand-in too).

<a id="ts-20261013-170941"></a>

## 2026-10-13T17:09:41+02:00 — Studs split out of the model-less objects (worker)

**[Corrects objects.md §2026-10-13T15:39:15](#ts-20261013-153915)** ("Left out and why" entry on studs and the counts above): studs are pickup objects (object type 0x3d, pickuptype 0..3) drawn as sprites, not models (details, evidence and stand-ins in [studs.md](studs.md)). `drawableObjects` now returns them in `DrawableObjects::studs` and no longer counts them in `missingModel`: lvl_00 1 without model (the particle emitter), lvl_01 11, all 856 levels 423 (not itemised); studs over all levels 4835.

Unknowns: what the remaining 423 model-less objects are (hearts, `xx_particleemitter`, hint meshes) and how each is drawn.

Collision of placed objects (which objects register collision, box or mesh, node matrix, query layers): [collision.md](collision.md) 2026-10-13T17:34:00.

<a id="ts-20261013-211741"></a>

## 2026-10-13T21:17:41+02:00 — Start state of placed objects and doors (pointer entry)

- Which placed objects are disabled at level start comes from the level's own `startup` script and from the build-it / reveal links of other objects, not from a per-object flag in the .blvl: [scripts.md](scripts.md). [Corrects objects.md §2026-10-13T16:34:30](#ts-20261013-163430), replacing the earlier "hidden at the start" approximations in this file for collision; the drawing code (`drawableObjects`) still draws every object with a model, including the ones the start state disables.
- Doors: state machine, timing, motion and sounds in [doors.md](doors.md). Trigger chain recovered: a door opens by itself when `AutoOpen` is set (instance parameter, the lvl_00 lift and double doors) and the player is inside its zone, or when it receives message 0xff (from `TriggerObject` / the platform link). The senders of that message in lvl_01 (mission scripts for the front doors) are not traced; the program does not run them.

Unknowns
- Which scripts send the open request to `Police_Door_01/02` and the other lvl_01 doors.
- Whether `drawableObjects` should skip the objects the start state disables (visibility at start; not compared with a screenshot).

<a id="ts-20261014-133957"></a>

## 2026-10-14T13:39:57+02:00 — Details moved from feature items LEVOBJ-1

Moved from the feature files so that the items stay short (CLAUDE.md, "Documentation rules"). The text below is the former wording of each item.

- **LEVOBJ-1** (level-objects.md): Whether `drawableObjects` should skip the objects the start state disables (visibility at start; not compared with a screenshot); per-class visibility at start (`DisabledOnStartUp`, `ShowOnObjectScan`, `ShowOnMap`) and the effect of flags bit 2 (value 6 in the +0x14 table). evidence: [objects.md §2026-10-13T21:17](#ts-20261013-211741), [objects.md §2026-10-13T15:39](#ts-20261013-153915).

<a id="ts-20261014-150541"></a>

## 2026-10-14T15:05:41+02:00 — The pale disc at the police entrance is the hidden floor plaque (LEVOBJ-11)

- **Object.** Listing the drawn placements within 6 units of the player spawn (904.2, 1112.4) gave `GrappleGun` (leprops/leprop_simpleindestructible, Weap_GrappleGun), `Police_Door_01/02`, `Police_Plaque` (the wall emblem, y 21.84) and `Police_Plaque_Floor` (props/prop_respawnabledestructible, model `Prop_PolicePlaque`, position 904.476 16.677 1114.502, orientation 89.854 0 90, room7). The disc is `Police_Plaque_Floor`: the same round emblem model as the wall plaque, turned to lie on the ground. Drawing with that object skipped removes the disc (chase-game picture at update 66 of `Police_ExtToInt`, pale pixel count 11129 before, 71 after).
- **Why the original does not show it.** It is disabled at the level start. The grid read at gameplay-start gave entity flag bit 0 for `Police_Plaque_Floor` ([collision.md §2026-10-13T17:34](collision.md#ts-20261013-173400) lists it, the test `level-start-real-data` pins it with 84 other names, `GrappleGun` among them); the port's startup script run and the reveal links already produce the same state (`PlacedObjectState::disabled`). EnableObject with 0 (0x1ddd00 -> 0x2428c0, [scripts-runtime.md §2026-10-14T10:06](scripts-runtime.md#ts-20261014-100640)) sets entity flag bit 0, flags the sub-objects, and calls `0x23f734(node = entity+0x3c, 0)`; 0x23f734 is `flags = visible ? flags & ~0x60 : flags | 0x60` on the first word of the scene node (disassembly 0x23f734..0x23f748, llvm-objdump; 69 call sites: it is the engine's generic node hide/show). The port drew every placement regardless of that state (the disabled set only reached collision). Mission 1-7 and 1-8 scripts reference `Police_Plaque_Floor` and `GrappleGun` (strings of the level file), so the plaque is a mission marker that scripts show later.
- **Rule applied (LEVOBJ-1, first part).** `drawnPlacements` (`src/runtime/object_visibility.*`) says a placed object is drawn while it is not disabled; `LevelSession::draw` sets `Scene::setInstanceVisible` from the live state before each draw, so EnableObject 1 shows the object again. In lvl_01, 127 of the 1198 drawable objects are hidden at the start (the 249 disabled names also cover characters, studs and objects without a model). No pinned screenshot compares pixels except the new `police-entrance-no-disc`; the other chase-game screenshot tests only check their log lines and still pass; the visible change is that the hidden objects (build-it targets, `FP_*` props, `1-1_DonutVan*`, `Race01*Banner`, `GrappleGun`, and others of the 127) are no longer drawn in the starting picture.
- Comparison picture (not committed): `artifacts/oracle/agentD2-out/compare/dcam_police_oracle_left_port_right.png`.

Unknowns:
- Which code in the scene traversal skips nodes with bits 0x60 was not located (no `tst #0x60` pattern in the executable: the test may be split into two single-bit tests or read through a getter). The rule rests on 0x2428c0 hiding the node, the grid read of flag bit 0 and the oracle picture without the disc.
- The scene instances of disabled characters (and sub-objects of classes) are still drawn: only placed props are filtered (LEVOBJ-12).
- Per-class start visibility (`DisabledOnStartUp`, `ShowOnObjectScan`, `ShowOnMap`) and flags bit 2 of the +0x14 table are not applied (LEVOBJ-13); the shadow (blob or projected) under the player in the oracle picture is not drawn by the port (RENDER-9).

<a id="ts-20261019-095914"></a>

## 2026-10-19T09:59:14+02:00 — The mission 1-2 dog and the other animals of lvl_01 are placed props (REA Ghidra 12.1.2 pseudo-code and llvm-objdump on `artifacts/game/game.arm.elf`, class files through the C++ loader, RPC screenshots A-1 by the emulator servant)

[Corrects objects.md §2026-10-13T16:34:30](#ts-20261013-163430): the animal rigs of lvl_01 are not characters with a constructor of their own; the dog, the cats and the crow are props of kinds 0, 64 and 67.

**The objects.** lvl_01 places the dog twice, both with `meshname` `char_dog` in room32 at the roof cage: `1-2_Dog_Prop` (class `props/prop_dogwaggles`, kind 0, 918.664 64.763 663.005, facing -65.738; `ShowOnObjectScan` 1, `TriggerScanned` `1-2_Scanned_Pringles`) is the caged dog the scanner finds, `1-2_Dog` (class `props/prop_prop`, kind 67, 918.717 64.668 662.731, facing -54.984, `CSCharacter` `dog`, `Immovable` 2) is the dog the cage key script releases. The class `chars/char_dog` (kind 158) is not placed in lvl_01. The other animal rigs of lvl_01: the four `props/ability_freeplay_fireman_cat` (`char_cat`, kind 0), `1-3_Crow` (`leprops/leprop_platform`, `char_Crow`, kind 64), five `chars/ride_pig` (`char_pig`, kind 159) and four `props/prop_sharkattack` (`char_shark`, kind 239).

**Creates (ev_e2f1a97e, ev_9c4b0291, ev_154f92cd, ev_e00aef2d, ev_aa23d9c8, ev_c462629d).**
- Kind 0 is registered by 0x17f2f4 (`mov r0, r4` with r4 = 0 before `bl 0x246240` at 0x17f33c): create 0x1d1450, post-link 0x1c3ee0, +0x1c 0x1d1690. 0x1d1450 copies the 0x88-byte info record, calls 0x234004(entity, 0), builds a `dummy` node when there is no model, and reads `CollisionBound` (a box only when the class lists one; `prop_dogwaggles` and the cat class do not).
- Kind 67 (`props/prop_prop`, registered by 0x1564b8): create 0x15a4b4 reads `Destructable` and calls the prop create 0x16a268, which calls 0x234004(entity, 0) first; the rest sets up destruction, sounds, `ControlledAnim`, judder, rotation and stud spawns (none of these is set for `1-2_Dog`).
- Kind 64 (platforms, registered by 0x180c98): create 0x199adc calls the model loader 0x2442b4 directly, then the platform setup.
- 0x234004 = model loader 0x2442b4(entity, 0), then, when the entity has a skeleton (+0x44) and its `Animation` parameter is not empty, starts that clip (0x23e6e4, 0x23afc0 with `AnimationVelocity`, offset `AnimationOffset`). `prop_prop` has `Animation` "" and `prop_dogwaggles` and the cat class have none; no object or class of lvl_01 sets one. So **no clip runs on the dogs, the cats or the crow at creation**; they stand in the pose of a skeleton without animation until a script plays a clip. In all eight main levels six objects set `Animation` (`Sea_Waves`, `4-6_PiggyTractor_Crashed`, `5-8_Chopper_Circling`, three `7-9_Goon_Chopper*`), none of them an animal.
- 0x2442b4 loads `models/<dir>/<leaf>.fnmdl` into the node (+0x3c) and the skeleton `<dir>.fnskl` of the same folder into +0x40/+0x44 (0x212420), then the attachments, head, hat and the upper/lower/face/cape texture replacement (0x226750). Nothing in it is specific to animals.
- Kinds 159 (`chars/ride_pig`, created through 0x23e394) and 239 (sharks: create 0x15eec0 makes only the first shark of the level and returns 0 for the others, with `AttackFrame01..03`) have creates of their own; they were not read further.

**Why the port showed no dog.** `drawableObjects` left out every model whose name starts with `char_` other than the minifig body, a rule of the port, not of the original. Now an object with such a model is a placed prop when its kind is 0, 64 or 67 (`createsModelAsProp`, src/assets/level.cpp): lvl_01 draws 1205 placed objects (7 more) and 9 other rigs stay out (pigs and sharks); over all levels 78 such objects are placed props and 92 rigs stay out.

**The release (script `1-2_Cage_Key_Target_Script`).** `EnableObject 1-2_Dog_Prop 0`, `EnableObject 1-2_Dog 1`, `EnableCollision 1-2_Dog 0`, `PlayAnimation 1-2_Dog "dog_cage_open"` (`models/char_dog/dog_cage_open.bfnanm`, 45 frames, played once), `Wait 3`, `FadeScreen 0`, `PositionObject 1-2_Dog 1-2_Dog_Position_End` (730.328 31.599 645.549, at the dog house), the cutscene `1_2_c`, whose actor folder `dog` matches the `CSCharacter` of `1-2_Dog` (so the original's cutscene drives that entity, [cutscene-actors.md §2026-10-15T07:54:38](cutscene-actors.md#ts-20261015-075438)), then `EnableObject 1-2_Dog 0`. The path out of the cage is the clip's root motion ([§2026-10-19T10:10:34](#ts-20261019-101034)). In `chase-game` the dog plays the clip from the use spot; `PositionObject` of a prop is not applied (SCRIPT-288) and the cutscene draws a dog of its own.

**Start state.** The level start (`1-2_MissionHide`, `1-3_MissionHide`) disables both dogs and the crow; the cats are drawn (CTest `level-start-real-data`). `1-2_Waypoint_02_Script` enables `1-2_Dog_Prop`.

**Oracle (A-1).** `m12-13-cage-trigger`, no input, screenshots at 0, 60 and 150 updates (`artifacts/oracle/r28-servant-out/a1/`): the cage is closed and its barred side shows a dark interior, so the dog's pose cannot be seen; the pictures do not change.

Port: `createsModelAsProp` in src/assets/object_classes.hpp; CTest `animal-props-real-data` (instances, classes, kinds, positions, models, the release clip), `assets-real-data`, `level-start-real-data`.

Unknowns:
- The pose the original draws for a skinned model without a clip (the port draws the stored vertices); not compared, the cage hides the dog.
- What the cat rescue use spot (kind 124, create 0x1829ac, post-link 0x176e04 reads `TriggerObject` and the cat link, update 0x182b38 -> 0x1c6aa8) does with its cat (clips `idle_sat`, `rescuecat_*`).
- The creates of `ride_pig` (0x23e394) and of the sharks beyond the first lines; the crow's platform motion (`LerpShape`, `ObjectTo`, `Speed`).
- Whether the cutscene binds `1-2_Dog` while it is moved (the binding code 0x2d1ddc was not read for props).

<a id="ts-20261019-101034"></a>

## 2026-10-19T10:10:34+02:00 — Props move by the root motion of their clip: the dog leaves the cage (REA Ghidra 12.1.2 pseudo-code of 0x15aa70, 0x16b06c, 0x2327b4, 0x270778, 0x23d6c4; llvm-objdump of 0x16b2bc..0x16b3a8; C++ curve dump; RPC screenshots A-2 by the emulator servant; Azahar GDB trap, port 24781, no RPC)

**Oracle (A-2).** `m12-13-cage-trigger`, A pressed 30 updates after the load, screenshots 10, 30, 50 and 80 updates after the press (`artifacts/oracle/r28-servant-out/a2/`, ticks in `a2_shots.csv`): the camera stays; at +10 the cage is closed, at +30 its door is up and a shadow is in the air, at +50 the dog is out in front of Chase on the left, at +80 the door is down again and the dog stands against Chase beside the fence. `chase-game` before this entry: the dog reared up at the cage and came back to it.

**Data.** `dog_cage_open` (joint table word 0x10168013) has bit 28 set: curve 19, the last of 20 (the skeleton has 19 joints), is the extra root curve. It stays 0 up to frame 11, then moves to translation (-1.2263, -0.3075, 3.3163) and yaw -0.570 rad by frame 18 and holds (C++ dump with `decodeChannels` / `localMatrix`, scratch `artifacts/scratch/r28-A/tools/rootdump.cpp`). The joint curves alone bring the body back to the cage by frame 40.

**Code.** Kind 67's update 0x15aa70 ends with 0x16b06c(entity, 0). At 0x16b2bc..0x16b3a8: when the entity's current animation (0x23ad24(entity + 0x40)) has any of the bits 0xf of its word +8 and 0x245f84() is not 6, it calls 0x23602c and 0x2327b4(skeleton +0x44, out); on a non-zero return it builds R = Ry(out.yaw) with translation out.xyz (0x24d434, 0x237bc4, 0x2554c0 into the translation row), R = R x node (0x258a10), copies R's rotation into the node matrix (0x232788) and stores it (0x24c118), then moves the entity by R.translation - node.translation (0x249030 with the negated position, 0x23d39c adds) through 0x23d6c4(entity, move, 0, ...), which with the collision flag 0x200 clear (EnableCollision 0) and flags 0 only adds the move to the node position. 0x2327b4 per playing slot: 0x270778 samples the extra curve (joint table bit 28) at the slot's frame, a matrix 0x247fb0 / 0x2588f4, translation and yaw = 0x2473ec of the third row; components whose bit (1 x, 2 y, 4 z, 8 yaw) in the animation's word +8 is clear are zeroed; with flag 0x8000 of the slot (start) or weight 0 the sample is only stored; else the output is B x inverse(A) of the stored sample A and the new one B (translation in A's frame, yaw difference), weighted per slot by its weight and the factors +0x18..+0x24; wraps of a looping clip subtract the clip's whole motion (0x29f97c). So the node turns by the yaw difference and moves by the translation difference taken through its rotation before the turn.

**Oracle (GDB trap).** `m12-13-cage-trigger`, scenario `artifacts/scratch/r28-A/release_root.txt` (30 frames, A held for 1 frame, `trap 0x16b2d4,0x16b358 60`; output `artifacts/oracle/r28A-out/release_trap.txt`): at 0x16b2d4 r1 (the animation's word +8) is 0x0000000f in every stop, so all four axes are on for `dog_cage_open`. At 0x16b358 the matrix r1 points to is the dog's node: it stands at the placement (918.7173 64.6680 662.7309, yaw -54.98 degrees) until stepped frame 45, moves over frames 46..54 (for example frame 49: 917.5319 65.0376 663.2573, yaw -85.09) and stays at **915.2977 64.3605 663.6295, yaw -87.64 degrees** to frame 76. The stepped frames do not run at the real rate, so the timing of the move was not compared.

**Port.** `src/runtime/prop_root_motion.{hpp,cpp}` (`rootSample`, `rootDelta`, `applyRootDelta`), used by `PropAnimations::update` for every animated prop with a node from its placement; glue `src/game_animals.cpp` sets the scene transform of a moved prop. The dog ends at 915.298 64.361 663.629 facing -87.6 degrees (placement 918.717 64.668 662.731, -54.98 degrees), the trap's end pose to the fourth decimal. `chase-game` from the spot (`--start "916.702 64.431 663.724" --facing 0 --call-script 1-2_Pringles_Cage_Locked --script "0 0 5;0 0 1 A;0 0 N"`, N = 10, 30, 49, 79): at +49 and +79 the dog stands left of the cage and turned towards Chase where the A-2 pictures have it (`artifacts/scratch/r28-A/p3_grid.png`: port above, oracle below). Chase himself does not move in the port (his `dog_cage_open_minifig` has root motion of its own; PlayCharAnim on the player is not ported), and the cage is hidden in that port run because the test path skips the script that shows it. Tests `prop-root-motion` (synthetic), `animal-props-real-data` (end position).

Unknowns:
- Where the word +8 of an animation record is set (0x1dbf0c allocates the record; `dog_cage_open` has 0xf, other clips were not read; the port applies all four axes to every clip); the test 0x245f84() != 6 (not applied); how 0x287124 interpolates a fractional frame (linear in the port); the timing of the move in real updates.
- The weights and per-axis factors (+0x18..+0x24) of 0x2327b4 for more than one slot; 0x23602c.
