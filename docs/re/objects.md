# Level objects

How the .blvl objects (section +0x2c) find their model and transform. Format of the level file: [levels.md](levels.md).

## 2026-10-13T15:39:15+02:00 — Object census, parameter slots and the model lookup

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

## 2026-10-13T16:34:30+02:00 — Characters in the level: body, head, textures and starting pose (worker; REA Ghidra 12.1.2 pseudo-code, objdump, class and instance dumps through parseLevel)

Replica: `drawableObjects` / `CharacterObject` in src/assets/level.cpp, `addLevelCharacters` in src/render/character.cpp (shared with the player through `addCharacter` / `poseBody` / `placeCharacter`; chase-game and `chase-viewer --characters`). Tests: `assets-real-data` (counts and three resolved rigs).

**Correction:** the "Left out and why" entry on `char_*` meshes of the earlier section no longer holds for minifig characters (drawn now, see below); animals and the custom characters stay out.

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

## 2026-10-13T17:09:41+02:00 — Studs split out of the model-less objects (worker)

**Correction of the "Left out and why" entry on studs and of the counts above:** studs are pickup objects (object type 0x3d, pickuptype 0..3) drawn as sprites, not models (details, evidence and stand-ins in [studs.md](studs.md)). `drawableObjects` now returns them in `DrawableObjects::studs` and no longer counts them in `missingModel`: lvl_00 1 without model (the particle emitter), lvl_01 11, all 856 levels 423 (not itemised); studs over all levels 4835.

Unknowns: what the remaining 423 model-less objects are (hearts, `xx_particleemitter`, hint meshes) and how each is drawn.
