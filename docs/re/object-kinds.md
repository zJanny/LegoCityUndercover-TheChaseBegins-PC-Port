# Object kinds: the kind byte of a placed entity (entity +0x12)

Where the kind byte of an entity comes from, which kind each class file of the main levels has, and which natives it gates. The class descriptors are named by their kind in the other topics ([doors.md](doors.md), [buildit.md](buildit.md), [props-wobbly-bouncer.md](props-wobbly-bouncer.md), [vehicles-system.md](vehicles-system.md)). Items: SCRIPT-57 in [script-system-natives.md](../plans/features/script-system-natives.md).

## Current state

- The kind byte (entity +0x12) is the low byte of the class file's `ObjectType` parameter (hash 0x06fef256, type 7), looked up with the class defaults, then the class table; a class without it has kind 0. The port reads it with `assets::objectKind`. [§2026-10-18T21:36:13](#ts-20261018-213613)
- The byte indexes the class descriptor table 0x583124 (0x28 bytes per kind: create, post-link, update, message handler at +0x14, ...). Every non-zero kind of the 434 classes the eight main levels place (535 with the objects nested in class files) has a descriptor registered by `mov r0,#kind; bl 0x246240`; the table below gives the registration address. [§2026-10-18T21:36:13](#ts-20261018-213613)
- Objects nested in a class file (`Level::objects` of a class file) are created after their parent by the same loader; the parent's halfword +0x16 counts its descendants and the entity's halfword +0x14 is its index in the scene's entity list. Scripts name a nested object `Parent.Child`. `leprops/leprop_pushable` (kind 71) nests four `leprops/leprop_pushhandle` (kind 72), the only kind 71 / 72 pair of the main levels (12 pushables). [§2026-10-18T21:36:13](#ts-20261018-213613)
- ToggleUseable on a kind 71 object writes the use flag of each descendant of kind 72 and then of the object itself; 11 of its 342 calls in the main levels name a pushable. EnableShadow acts only on kinds 27 and 149 (and a kind list and a flag the port does not have); its 8 calls all name characters (kind 27). EnableSwitch writes the state 3 only for kinds 94, 139, 100 and 58; its 9 calls name kinds 185 (7), 190 (1) and 58 (1). [§2026-10-18T21:36:13](#ts-20261018-213613)

<a id="ts-20261018-213613"></a>

## 2026-10-18T21:36:13+02:00 — The kind byte of every placed class, and the group case of ToggleUseable (llvm-objdump of artifacts/game/game.arm.elf; C++ class-file census; no emulator)

[Extends script-natives-flags.md §2026-10-14T14:16](script-natives-flags.md#ts-20261014-141621) (Unknowns: the kind byte of each placed class).

**Where the byte is set.** The object loader (function 0x1a4130, called from the object pass at 0x1a4aa4 for every placed object and, recursively, for the objects nested in the class file) fills a 0x88-byte info record on its stack: at 0x1a4270..0x1a42b0 it looks up `ObjectType` (string at 0x1a4574, flags word 0x07000012 at 0x1a4580) with lookup 0x217080 and stores the result with `strb r0, [sp, #0x1a]`, which is info +0x12. At 0x1a46c0 it loads that byte and calls `table[kind * 40]` at 0x583124 (the descriptor's create) with the info record. Every class create copies the record into its new entity: the door create 0x169494 and the pushable create 0x19a7c8 both allocate 0x88 bytes and copy the info record into them (0x25bc34). So entity +0x12 = low byte of the effective ObjectType. The registrar 0x246240 copies the 0x28 bytes it is given to `0x583124 + kind * 40`. Oracle readings in other topics agree: the player entity has +0x12 = 27, a donut table 0x77 (wobbly class, ObjectType 0x77), the pole 0xbb ([player-traversal-pole.md](player-traversal-pole.md)).

**Port.** `assets::objectKind(object, classFile)` (src/assets/level.cpp, declared in object_classes.hpp) is `effectiveParam` of `ObjectType` (the existing model of lookup 0x217080: the class defaults, then the class table; an instance value only for a slot parameter) truncated to a byte. Test `object-kinds` (synthetic rules and 15 real class files with their provenance).

**Census.** Kind per class file of the eight `*_main.blvl` levels (lvl_00..lvl_07), objects nested in class files included (`(child)`), with the instance count over all eight levels. The registration column is the address of the `bl 0x246240` that registers the kind (found for every kind except 0, whose entities use table entry 0). Kinds and counts come from the C++ loader (objectKind per placed object and per nested object); the registration addresses from scanning the disassembly for `mov r0,#kind` before `bl 0x246240`.

| Kind | Registration | Classes (instances) |
|---|---|---|
| 0 (0x0) | none | `charbits` 8, `dummy/dummy` 683, `dummy/dummy (child)` 221, `dummy/dummycollision` 236, `dummy/dummycollision (child)` 177, `dummy/dummymesh` 155, `dummy/dummymesh (child)` 998, `dummy/dummyrunto` 92, `dummymesh` 25, `dummymesh (child)` 586, `ledummy/ledummy` 88, `ledummy/ledummy (child)` 8, `ledummy/ledummycollision` 35, `ledummy/ledummycollision (child)` 38, `ledummy/ledummymesh` 35, `ledummy/ledummymesh (child)` 719, `ledummy/ledummyrunto` 18, `props/ability_freeplay_fireman_cat` 28, `props/ability_grapple_attach` 48, `props/ledummymesh_scannable_walkietalkie` 1, `props/prop_dogwaggles` 1, `vehicles/vehicledrop_vehicle (child)` 7, `vehicles/vehicledrop_vehicle02 (child)` 4, `weapons/weap_custom1 (child)` 10, `weapons/weapon_baseballbat (child)` 13, `weapons/weapon_dynamite (child)` 3, `weapons/weapon_grapplegun (child)` 2, `weapons/weapon_potplant (child)` 4, `weapons/weapon_raygunfreeze (child)` 5, `weapons/weapon_rock (child)` 1 |
| 1 (0x1) | 0x18b70c | `leparticles` 28, `particles` 281, `particles (child)` 182 |
| 4 (0x4) | 0x168dc4 | `fx/fx_forcefield_bank` 1, `fx/fx_mech_explode` 1, `fx/fx_satellite_beam_activate (child)` 9, `fx/fx_satellite_beam_down` 2, `fx/fx_satellite_beam_down (child)` 9, `fx/fx_satellite_beam_fire (child)` 9, `fx/fx_teleport_in (child)` 58, `fx/fx_teleport_out (child)` 58, `fx/fx_water_fountain_tall` 12 |
| 6 (0x6) | 0x17f348 | `camera` 1 |
| 8 (0x8) | 0x2d82a4 | `lelighting/lelightpoint (child)` 5, `lighting/lightambient` 15, `lighting/lightdirectional` 18, `lighting/lightpoint` 2, `lighting/lightspecular` 8 |
| 9 (0x9) | 0x17f35c | `lescriptobject` 151, `scriptobject` 736, `scriptobject (child)` 68 |
| 10 (0xa) | 0x159384 | `lesoundobject` 87, `lesoundobject (child)` 33, `soundobject` 2 |
| 11 (0xb) | 0x163538 | `lepoint` 52, `lepoint/lepoint` 20, `lepoint/lepointrunto` 3, `point` 137, `xx_teleport1` 1 |
| 12 (0xc) | 0x197018 | `animcamera` 13 |
| 14 (0xe) | 0x18ab34 | `leaispawners/leaispawner_default` 2, `leaispawners/leaispawner_superjump` 1, `leprops/leprop_aispawner` 13, `props/prop_aispawner` 93 |
| 17 (0x11) | 0x1a62c0 | `props/parkour_balancebeam` 16, `props/parkour_balancebeam_2x14` 12 |
| 19 (0x13) | 0x1748ec | `leprops/leprop_bouncer` 17 |
| 20 (0x14) | 0x18b070 | `props/parkour_brickgrab_new` 142 |
| 21 (0x15) | 0x15d2d4 | `leprops/leprop_buildit` 27, `props/prop_buildit` 79 |
| 27 (0x1b) | 0x16d8c4 | `chars/char_ai_actor` 120, `chars/char_ai_actor (child)` 2, `chars/char_ai_ashby` 1, `chars/char_ai_baker` 3, `chars/char_ai_bankteller_01` 1, `chars/char_ai_bankteller_02` 1, `chars/char_ai_bankteller_03` 1, `chars/char_ai_bankteller_04` 1, `chars/char_ai_birdmangang1` 3, `chars/char_ai_birdmangang1_bad` 1, `chars/char_ai_birdmangang2` 1, `chars/char_ai_birdmangang2_bad` 2, `chars/char_ai_birdmangang3` 2, `chars/char_ai_birdmangang3_bad` 1, `chars/char_ai_birdmangang4` 1, `chars/char_ai_birdmangang4_bad` 2, `chars/char_ai_burtbirdmanwellington` 2, `chars/char_ai_burtbirdmanwellingtonboss` 1, `chars/char_ai_carlwalnuts` 3, `chars/char_ai_cherrytreehillsgang1` 23, `chars/char_ai_cherrytreehillsgang2` 11, `chars/char_ai_cherrytreehillsgang3` 6, `chars/char_ai_cherrytreehillsgang4` 6, `chars/char_ai_constructionforeman` 9, `chars/char_ai_constructionworker01` 7, `chars/char_ai_constructionworker02` 7, `chars/char_ai_constructionworker03` 3, `chars/char_ai_constructionworker04` 1, `chars/char_ai_deputydunby` 3, `chars/char_ai_evil_astronaut_fists` 43, `chars/char_ai_farmerswife` 3, `chars/char_ai_firechief` 3, `chars/char_ai_flashhotshotjohnson` 5, `chars/char_ai_flashhotshotjohnson_bossfists` 2, `chars/char_ai_goon_carlwalnuts` 2, `chars/char_ai_goon_farmergoon1` 12, `chars/char_ai_goon_farmergoon2` 12, `chars/char_ai_goon_farmergoon3` 13, `chars/char_ai_goon_farmergoon4` 7, `chars/char_ai_goon_fists` 12, `chars/char_ai_goon_fists_heavy` 1, `chars/char_ai_goon_miniboss` 4, `chars/char_ai_goon_rexgang_01` 7, `chars/char_ai_goon_rexgang_02` 6, `chars/char_ai_goon_rexgang_03` 6, `chars/char_ai_goon_rexgang_04` 7, `chars/char_ai_goon_walnutsgang1` 11, `chars/char_ai_goon_walnutsgang2` 11, `chars/char_ai_goon_walnutsgang3` 7, `chars/char_ai_goon_walnutsgang4` 8, `chars/char_ai_goon_walnutsgang5` 4, `chars/char_ai_goon_walnutsgang6` 5, `chars/char_ai_goon_walnutsgang7` 3, `chars/char_ai_lennyslicksantos` 1, `chars/char_ai_lennyslicksantosdefeated` 1, `chars/char_ai_maninblack1 (child)` 8, `chars/char_ai_maninblack2 (child)` 8, `chars/char_ai_nataliakowalski` 1, `chars/char_ai_prisoner01` 3, `chars/char_ai_prisoner02` 4, `chars/char_ai_prisoner03` 3, `chars/char_ai_prisonwarden` 2, `chars/char_ai_rexfury` 2, `chars/char_ai_smokey` 2, `chars/char_ai_smokeyescobar` 3, `chars/char_ai_smokeyescobargoon1` 11, `chars/char_ai_smokeyescobargoon2` 8, `chars/char_ai_smokeyescobargoon3` 9, `chars/char_ai_smokeyescobargoon4` 5, `chars/char_ai_smokeyescobargoon5` 8, `chars/char_ai_tonyknucklesmcgee` 2, `chars/char_ai_waters` 2, `chars/char_apolloisland_astronaut` 5, `chars/char_apolloisland_evilastronaut` 2, `chars/char_boosterstrange` 2, `chars/char_constructionworker01` 1, `chars/char_constructionworker02` 1, `chars/char_constructionworker03` 1, `chars/char_freeplay_crimerobber (child)` 28, `chars/char_freeplay_crimevictim (child)` 5, `chars/char_freeplay_crimevictim_albatross01 (child)` 1, `chars/char_freeplay_crimevictim_apollo01 (child)` 2, `chars/char_freeplay_crimevictim_apollo02 (child)` 2, `chars/char_freeplay_crimevictim_auburn01 (child)` 2, `chars/char_freeplay_crimevictim_auburn02 (child)` 2, `chars/char_freeplay_crimevictim_blubell01 (child)` 1, `chars/char_freeplay_crimevictim_blubell02 (child)` 1, `chars/char_freeplay_crimevictim_blubell03 (child)` 2, `chars/char_freeplay_crimevictim_cherrytree02 (child)` 3, `chars/char_freeplay_crimevictim_downtown01 (child)` 2, `chars/char_freeplay_crimevictim_downtown02 (child)` 1, `chars/char_freeplay_crimevictim_fortmeadows01 (child)` 2, `chars/char_freeplay_crimevictim_fortmeadows02 (child)` 2, `chars/char_henrikkowalski` 1, `chars/char_journalist` 2, `chars/char_npc_citizenfemale01` 1, `chars/char_npc_citizenfemale07` 1, `chars/char_npc_citizenfemale08` 1, `chars/char_npc_citizenmale04` 1, `chars/char_npc_citizenmale08` 1, `chars/char_player (child)` 208, `chars/char_racecardriver` 7, `chars/char_scruffythejanitor` 1, `chars/char_vonkruchman_disguise` 4, `props/ability_freeplay_astronaut_alien01` 28 |
| 29 (0x1d) | 0x17f23c | `props/parkour_flagpole` 32, `props/parkour_railgrab` 40, `props/parkour_railgrab_4` 20, `props/parkour_railgrab_6` 5, `props/parkour_railgrab_8` 13, `props/parkour_railgrab_phils` 2, `props/parkour_railgrab_truck` 2, `props/prop_climbbar (child)` 5 |
| 30 (0x1e) | 0x18b260 | `props/parkour_wallclimb` 21, `props/parkour_wallclimbvines` 16 |
| 31 (0x1f) | 0x1d1430 | `ledummy/ledummychildcollisionbound (child)` 81 |
| 38 (0x26) | 0x1c3eb8 | `leprops/leprop_lightonoff` 4, `props/prop_lightonoff` 13 |
| 42 (0x2a) | 0x15c290 | `leprops/leprop_door` 10, `props/prop_door` 28 |
| 43 (0x2b) | 0x18b2fc | `props/parkour_brickgrab_hintbound` 17, `props/parkour_brickgrab_hintbound (child)` 5, `props/prop_cannon_missedhint (child)` 6, `props/prop_cannon_readyhint (child)` 6 |
| 45 (0x2d) | 0x15c310 | `props/prop_faller` 6 |
| 46 (0x2e) | 0x198f9c | `props/prop_fallertrap` 1 |
| 47 (0x2f) | 0x18b374 | `leprops/leprop_fanblower` 3, `props/prop_fanblower_truck` 1, `props/prop_fanlift` 19 |
| 55 (0x37) | 0x18b430 | `hurtbound` 38, `hurtbound (child)` 10 |
| 56 (0x38) | 0x16a10c | `leprops/leprop_ladderlarge` 2, `leprops/leprop_laddermedium` 2, `leprops/leprop_ladderxlarge` 3, `leprops/leprop_ladderxxlarge` 1, `props/prop_albatross_ladder` 1, `props/prop_crane_ladderlarge` 2, `props/prop_foldingladder` 4, `props/prop_ladder_hospital` 5, `props/prop_ladderlarge` 7, `props/prop_ladderlarge (child)` 2, `props/prop_ladderlarge_v2` 4, `props/prop_laddermedium` 15, `props/prop_laddershowroom` 1, `props/prop_laddersmall` 2, `props/prop_laddersmallwater` 3 |
| 58 (0x3a) | 0x163c08 | `leprops/leprop_switchlever` 4, `props/ability_detective_x_spot` 3, `props/ability_flowerplatform01` 5, `props/ability_flowerplatform02` 3, `props/ability_flowerplatform03` 1, `props/ability_phil_m_test` 3, `props/ability_sunflowerclimb01` 7, `props/ability_sunflowerclimb02` 5, `props/ability_valve_vertical` 9, `props/ability_vineclimb_base` 3, `props/prop_classlocker_astronaut` 1, `props/prop_classlocker_construction` 1, `props/prop_classlocker_default` 1, `props/prop_classlocker_farmer` 1, `props/prop_classlocker_fireman` 1, `props/prop_classlocker_miner` 1, `props/prop_classlocker_robber` 1, `props/prop_decorativeplant` 1, `props/prop_keypad_green` 6, `props/prop_keypad_red` 8, `props/prop_switchkey_vertical` 10, `props/prop_switchlever` 2, `props/prop_switchlever (child)` 18, `props/prop_switchpull` 31 |
| 61 (0x3d) | 0x2ed0f8 | `lepickups/lepickup_studgold` 16, `lepickups/lepickup_studsilver` 15, `pickups/pickup_studblue` 198, `pickups/pickup_studblue (child)` 32, `pickups/pickup_studgold` 2061, `pickups/pickup_studgold (child)` 128, `pickups/pickup_studpurple` 13, `pickups/pickup_studsilver` 2464, `pickups/pickup_studsilver (child)` 232 |
| 63 (0x3f) | 0x1902d0 | `lepickups/lepickupspawner` 12 |
| 64 (0x40) | 0x180cf0 | `leprops/leprop_platform` 58, `props/prop_albatross_rcactor` 3, `props/prop_apolloisland_tubeplatform` 2, `props/prop_platform` 125, `props/prop_platform_albatrossparrot` 3, `props/prop_platform_apolloisland` 1 |
| 65 (0x41) | 0x16a254 | `players` 13 |
| 66 (0x42) | 0x306e40 | `projectiles/projectile_bullet (child)` 8, `projectiles/projectile_dynamite (child)` 8, `projectiles/projectile_egg (child)` 8, `projectiles/projectile_energybolt (child)` 8, `projectiles/projectile_energyboltblue (child)` 8, `projectiles/projectile_energyboltgreen (child)` 8, `projectiles/projectile_energyboltred (child)` 8, `projectiles/projectile_energyboltsilver (child)` 8, `projectiles/projectile_energyboltyellow (child)` 8, `projectiles/projectile_freezebullet (child)` 8, `projectiles/projectile_grapplebolt (child)` 8, `projectiles/projectile_potplant (child)` 8, `projectiles/projectile_rock (child)` 8, `projectiles/projectile_waterbolt (child)` 8 |
| 67 (0x43) | 0x156518 | `leprops/leprop_prop` 45, `props/ability_miner_rocksmash` 13, `props/ability_miner_rocksmash_large` 8, `props/ability_rocksmash` 2, `props/prop_apolloisland_testtube` 1, `props/prop_centrifuge` 5, `props/prop_pig_scanable` 2, `props/prop_prop` 180, `props/prop_prop (child)` 70, `props/prop_rex_panel_grapplepull` 1, `props/prop_sea` 6, `props/prop_squirrel` 4, `props/prop_switchfloor_column (child)` 4, `vehicles_vehicleselect/vehicle_industrial_foundation (child)` 7, `vehicles_vehicleselect/vehicle_industrial_foundation02 (child)` 4 |
| 71 (0x47) | 0x180e74 | `leprops/leprop_pushable` 12 |
| 72 (0x48) | 0x1d1ff8 | `leprops/leprop_pushhandle (child)` 48 |
| 74 (0x4a) | 0x1a9814 | `props/ability_detective_reveal_cabinet` 1, `props/ability_detective_reveal_dirtmound` 6, `props/ability_detective_reveal_sideboard` 2, `props/ability_detective_reveal_trapdoor` 4, `props/ability_detective_reveal_trapdoor_small` 3, `props/prop_revealhatchnomesh` 1 |
| 78 (0x4e) | 0x18cf08 | `scenechange` 25 |
| 79 (0x4f) | 0x1a9890 | `leprops/leprop_scrollingbg` 1 |
| 81 (0x51) | 0x17f398 | `lelighting/leshadowcaster` 1 |
| 85 (0x55) | 0x2819c4 | `leprops/leprop_simpledestructible` 11 |
| 86 (0x56) | 0x297a20 | `leprops/leprop_simpleindestructible` 406, `leprops/leprop_simpleindestructible (child)` 107, `props/prop_albatross_opendoor01` 4, `props/prop_apollo_shuttle` 1 |
| 89 (0x59) | 0x16b694 | `xx_skipto` 54 |
| 90 (0x5a) | 0x16b6fc | `props/prop_skybox` 8 |
| 91 (0x5b) | 0x1b6414 | `props/parkour_skydive` 3 |
| 98 (0x62) | 0x1924d4 | `props/prop_superbuildit` 12, `props/prop_superbuildit (child)` 29 |
| 100 (0x64) | 0x16b780 | `props/ability_energyswitch_green` 3, `props/ability_energyswitch_red` 3 |
| 101 (0x65) | 0x18c0a8 | `props/parkour_tightrope` 22, `props/parkour_zipline` 66 |
| 102 (0x66) | 0x16b800 | `leprops/leprop_topple` 23, `props/ability_grapplepull_topple` 3, `props/prop_topple` 3 |
| 110 (0x6e) | 0x181c28 | `props/ability_acrobatwalljump` 2, `props/parkour_walljump` 1 |
| 111 (0x6f) | 0x1def8c | `props/ability_acrobatwall` 6 |
| 112 (0x70) | 0x274c68 | `props/parkour_horizontalwallrun` 7 |
| 113 (0x71) | 0x1deff4 | `props/parkour_verticalwallrun` 13, `props/parkour_verticalwallrunclimb` 5 |
| 114 (0x72) | 0x19b47c | `props/parkour_wallhug_onoff` 19 |
| 115 (0x73) | 0x1df098 | `controllers/controller_water` 27 |
| 119 (0x77) | 0x281c8c | `leprops/leprop_wobblydestructible` 2513, `leprops/leprop_wobblydestructible (child)` 22 |
| 122 (0x7a) | 0x15d420 | `props/prop_albatross_parrotcarry` 1, `props/prop_apolloisland_pc_carry` 5, `props/prop_carry` 38, `props/prop_carry_walkie_talkie` 1 |
| 123 (0x7b) | 0x182944 | `props/prop_albatross_carrytargetnomesh` 2, `props/prop_carrytarget` 13, `props/prop_carrytargetnomesh` 17, `props/prop_keyhole` 9 |
| 124 (0x7c) | 0x16d700 | `props/ability_freeplay_fireman_cat_usespot` 28 |
| 127 (0x7f) | 0x1b9698 | `props/ability_chickenglidepad` 12 |
| 130 (0x82) | 0x16db5c | `props/prop_copypanel (child)` 9 |
| 132 (0x84) | 0x190290 | `pickups/pickup_blueprints` 2, `pickups/pickup_charactertoken` 106, `pickups/pickup_redbrick` 21, `pickups/pickup_sbbrick1x1 (child)` 80, `pickups/pickup_sbbrick2x2` 30, `pickups/pickup_sbbrick2x4` 24, `pickups/pickup_sbplate1x1 (child)` 80, `pickups/pickup_vehicletoken` 35 |
| 133 (0x85) | 0x16f288 | `props/ability_drillspot_hole` 20 |
| 134 (0x86) | 0x277a6c | `props/ability_dynamitedestructible` 12 |
| 138 (0x8a) | 0x156394 | `props/prop_fire_1x4` 14, `props/prop_fire_2x4` 19, `props/prop_fire_4x4` 40, `props/prop_fire_6x6` 16, `props/prop_fire_camp` 3, `props/prop_megafire_2x4` 28, `props/prop_megafire_4x4` 2, `props/prop_megafire_6x6` 18 |
| 139 (0x8b) | 0x1848b8 | `props/prop_switchfloor` 4 |
| 142 (0x8e) | 0x16f4a0 | `props/prop_flyingpig (child)` 6 |
| 144 (0x90) | 0x19e37c | `props/ability_grapplepad_ascend` 56 |
| 145 (0x91) | 0x184e54 | `props/ability_grapplepad_pull` 17 |
| 146 (0x92) | 0x1909e0 | `props/ability_grapplepad_swing` 11 |
| 150 (0x96) | 0x1782cc | `props/ability_jetpackpad` 40, `props/ability_jetpackpad_ai` 1 |
| 159 (0x9f) | 0x16547c | `chars/ride_pig` 37 |
| 160 (0xa0) | 0x165648 | `props/prop_ropenode` 22 |
| 161 (0xa1) | 0x1aed44 | `props/ability_securitycamera` 2, `props/prop_hazard_sentrygun_laser` 4 |
| 165 (0xa5) | 0x17033c | `props/prop_simpleuse` 18 |
| 182 (0xb6) | 0x16f188 | `props/ability_detective` 17 |
| 183 (0xb7) | 0x155bb8 | `vehicles/vehicle_civic_epona` 1, `vehicles/vehicle_civic_hazard` 2, `vehicles/vehicle_civic_sweepy` 1, `vehicles/vehicle_civic_taxicab` 4, `vehicles/vehicle_civic_trasher` 3, `vehicles/vehicle_farm_armadillo` 5, `vehicles/vehicle_farm_armadillo_pig` 1, `vehicles/vehicle_fire_douser` 7, `vehicles/vehicle_fire_douser (child)` 7, `vehicles/vehicle_fire_douser02 (child)` 4, `vehicles/vehicle_fire_inferno` 1, `vehicles/vehicle_hospital_panacea` 1, `vehicles/vehicle_industrial_brawn` 2, `vehicles/vehicle_industrial_crater` 2, `vehicles/vehicle_industrial_foundation` 2, `vehicles/vehicle_industrial_minibrawn` 3, `vehicles/vehicle_industrial_steadfast` 7, `vehicles/vehicle_industrial_steadfast_chasespecial` 2, `vehicles/vehicle_industrial_tugger` 2, `vehicles/vehicle_industrial_worker` 3, `vehicles/vehicle_musclecars_falchiongt` 9, `vehicles/vehicle_musclecars_tigerella` 7, `vehicles/vehicle_police_barker` 1, `vehicles/vehicle_police_hero` 2, `vehicles/vehicle_police_justice` 1, `vehicles/vehicle_police_trooper` 9, `vehicles/vehicle_rc_explosive` 4, `vehicles/vehicle_rc_laundry` 1, `vehicles/vehicle_rc_laundrywithwarden` 1, `vehicles/vehicle_rc_test` 3, `vehicles/vehicle_salooncars_ancestor` 2, `vehicles/vehicle_salooncars_caravel` 7, `vehicles/vehicle_salooncars_indulga` 1, `vehicles/vehicle_smallcars_narym` 8, `vehicles/vehicle_smallcars_talos` 5, `vehicles/vehicle_smallcars_trev` 4, `vehicles/vehicle_space_cocoon` 2, `vehicles/vehicle_space_lerv` 10, `vehicles/vehicle_sportcars_drakonas` 5, `vehicles/vehicle_sportcars_lantos` 4, `vehicles/vehicle_suv_ironback` 3, `vehicles/vehicle_suv_transter` 9, `vehicles/vehicle_transitioncar` 5, `vehicles/vehicle_van_donut` 4, `vehicles/vehicle_van_dulmen` 6, `vehicles/vehicle_van_gotland` 7, `vehicles/vehicle_van_oldster` 1, `vehicles/vehicledrop_actor (child)` 7, `vehicles/vehicledrop_actor02 (child)` 4 |
| 184 (0xb8) | 0x158464 | `vehicles/wheel (child)` 986 |
| 185 (0xb9) | 0x176094 | `props/ability_axe_door` 18, `props/ability_axe_panel` 1, `props/ability_breakandenter_cargodoor` 1, `props/ability_breakandenter_door1` 15, `props/ability_breakandenter_door_epic` 3, `props/ability_breakandenter_doorbars01` 7, `props/ability_breakandenter_locker01` 3, `props/ability_breakandenter_manholecover01` 1, `props/ability_breakandenter_skylight01` 8, `props/ability_defuse` 4, `props/ability_electricrepair` 29, `props/ability_electricrepair_nocover` 4, `props/ability_freeplay_construction_repair01` 24, `props/ability_freeplay_construction_repair02` 4, `props/ability_freeplay_miner_gem01` 28, `props/ability_freeplay_robber_atm` 24, `props/prop_albatross_punchbag` 7 |
| 186 (0xba) | 0x1c7754 | `props/prop_switchcolourswapper` 5 |
| 187 (0xbb) | 0x16f958 | `props/ability_sunflowerclimb01_bound (child)` 7, `props/ability_sunflowerclimb02_bound (child)` 5, `props/parkour_drainpipe` 24, `props/parkour_drainpipe_small` 2 |
| 188 (0xbc) | 0x164850 | `props/parkour_catapult` 20 |
| 189 (0xbd) | 0x177f7c | `props/parkour_fencevault` 39 |
| 190 (0xbe) | 0x164b5c | `props/prop_dynamitecrate` 9, `props/prop_dynamitecrate_rock` 6 |
| 191 (0xbf) | 0x1701bc | `props/ability_safecrack` 6, `props/ability_safecrack_door` 1, `props/ability_safecrack_panel` 5 |
| 192 (0xc0) | 0x1bc36c | `props/traffic_junction` 47 |
| 194 (0xc2) | 0x179518 | `props/parkour_slideunder` 20 |
| 195 (0xc3) | 0x15f3b0 | `props/ability_wiretap` 8 |
| 197 (0xc5) | 0x1786dc | `chars/pedestrian` 58 |
| 198 (0xc6) | 0x1c8650 | `props/pedestriansystem` 6 |
| 199 (0xc7) | 0x165e80 | `props/ability_warpgate` 1, `props/prop_albatross_warpgate01` 8, `props/prop_downtown_warpgate01` 2 |
| 200 (0xc8) | 0x186918 | `props/ability_teleport_pad` 42 |
| 201 (0xc9) | 0x1863e0 | `props/prop_scannerzoneaudio` 5, `props/prop_scannerzonephoto` 28, `props/prop_scannerzonesatellite` 1 |
| 202 (0xca) | 0x16f3cc | `dummy/dummyanimation (child)` 13 |
| 203 (0xcb) | 0x165fb8 | `props/map_anothertilewaypoint` 5, `props/map_getintovehiclewaypoint` 6, `props/map_poiwaypoint (child)` 7, `props/map_raceminifigwaypoint` 7, `props/map_racevehiclewaypoint` 142, `props/map_useobjectwaypoint` 54, `props/map_vehiclewaypoint` 23, `props/map_wardenwaypoint (child)` 1, `props/map_waypoint` 159, `props/map_waypoint (child)` 8 |
| 204 (0xcc) | 0x192894 | `props/prop_trafficlight` 90 |
| 207 (0xcf) | 0x1ae9fc | `props/ability_satelliteuplink` 9 |
| 208 (0xd0) | 0x1afd1c | `projectiles/vehthrow_object (child)` 24 |
| 209 (0xd1) | 0x16f790 | `props/ability_freeplay_farmer_cannon` 6 |
| 210 (0xd2) | 0x277d50 | `props/ability_freeplay_farmer_pigspot (child)` 6 |
| 211 (0xd3) | 0x179430 | `props/prop_scannerpoiaudio` 2, `props/prop_scannerpoiaudiowarden` 1 |
| 213 (0xd5) | 0x179448 | `props/prop_scannerpoisatellite` 5 |
| 214 (0xd6) | 0x29c7e4 | `props/prop_respawnabledestructible` 175 |
| 218 (0xda) | 0x177550 | `props/parkour_cornerrail` 15 |
| 219 (0xdb) | 0x1839e0 | `pickups/pickup_blue_chickenring` 4, `pickups/pickup_gold_chickenring` 16, `pickups/pickup_silver_chickenring` 29 |
| 221 (0xdd) | 0x18ffac | `props/parkour_cornershimmy` 12 |
| 222 (0xde) | 0x190810 | `props/prop_switchfreezetarget` 5 |
| 223 (0xdf) | 0x179454 | `props/prop_scannerpoiphoto` 28 |
| 225 (0xe1) | 0x15d8f4 | `props/prop_critter` 3, `props/prop_critter_chicken` 20 |
| 226 (0xe2) | 0x183f88 | `props/prop_crittercage` 5 |
| 227 (0xe3) | 0x1ad2b0 | `props/prop_fasttravelzone (child)` 7 |
| 228 (0xe4) | 0x1c8054 | `dummymeshbouncer (child)` 262 |
| 230 (0xe6) | 0x185620 | `dummy/dummypursuitnode` 94 |
| 231 (0xe7) | 0x15ad1c | `props/ability_rc_point` 4 |
| 232 (0xe8) | 0x1bcb7c | `props/prop_vehicledropzone (child)` 11 |
| 233 (0xe9) | 0x186cec | `props/ability_watercannon` 3 |
| 234 (0xea) | 0x1644f4 | `bossais/ai_smokey` 1 |
| 236 (0xec) | 0x277db8 | `roomstreamingcontrol` 15 |
| 237 (0xed) | 0x284f38 | `roomstreamingdestination` 14 |
| 238 (0xee) | 0x1ad204 | `props/ability_detective_reveal_spot` 15 |
| 239 (0xef) | 0x1579d4 | `props/prop_sharkattack` 14 |
| 240 (0xf0) | 0x182520 | `bossais/ai_tonymcgee` 1 |
| 241 (0xf1) | 0x19da94 | `combatwatcher` 65 |
| 242 (0xf2) | 0x156414 | `chars/char_ai_goon_bossmech` 1, `chars/char_mech` 1 |
| 243 (0xf3) | 0x155f80 | `vehicles/vehicle_aircraft_ufo` 5, `vehicles/vehicle_aircraft_ufo (child)` 7, `vehicles/vehicle_aircraft_ufo02 (child)` 4 |
| 244 (0xf4) | 0x16c460 | `bossais/ai_walnuts` 1 |
| 245 (0xf5) | 0x15ad94 | `props/prop_rcvent` 4 |
| 246 (0xf6) | 0x19fc5c | `props/prop_sbsuperjump` 11 |
| 247 (0xf7) | 0x16490c | `props/ability_freeplay_copcrime` 2, `props/ability_freeplay_copcrime_albatross01` 1, `props/ability_freeplay_copcrime_apolloisland01` 2, `props/ability_freeplay_copcrime_apolloisland02` 2, `props/ability_freeplay_copcrime_auburn01` 2, `props/ability_freeplay_copcrime_auburn02` 2, `props/ability_freeplay_copcrime_blubell01` 1, `props/ability_freeplay_copcrime_blubell02` 1, `props/ability_freeplay_copcrime_blubell03` 2, `props/ability_freeplay_copcrime_cherrytree01` 3, `props/ability_freeplay_copcrime_cherrytree02` 3, `props/ability_freeplay_copcrime_downtown01` 2, `props/ability_freeplay_copcrime_downtown02` 1, `props/ability_freeplay_copcrime_fortmeadows01` 2, `props/ability_freeplay_copcrime_fortmeadows02` 2 |
| 248 (0xf8) | 0x16c2b0 | `bossais/ai_rexfurytruck` 1 |
| 249 (0xf9) | 0x19dd88 | `dcamexclusion` 82, `dcamexclusion (child)` 73 |
| 250 (0xfa) | 0x26c800 | `props/prop_rex_panel_grapplebolts` 1 |
| 251 (0xfb) | 0x1d8c5c | `vehicledeathbound` 3 |
| 252 (0xfc) | 0x192788 | `taskcomplete (child)` 255 |
| 253 (0xfd) | 0x28540c | `props/prop_sbhelipad` 5, `props/prop_sbhelipad_cherrytree` 1, `props/prop_sbhelipadflat` 1, `props/prop_sbvehicledrop` 7, `props/prop_sbvehicledrop02` 4 |
| 254 (0xfe) | 0x28b784 | `aiactionmusicexclusion` 2 |

**ToggleUseable 0x1cdcf8.** The object argument r4 is the entity (null: nothing). If `r4[0x12] == 71` (0x47): for the list indices `r4.hword[0x14] + 1` up to `r4.hword[0x14] + r4.hword[0x16]` of the array at `[[r4 + 0x20] + 0x24]` (bounded by the count at `[[r4 + 0x20] + 0x1c]`), each entity with `kind == 72` (0x48) gets `0x236624(child, v)` and `0x2219bc(child, v)`; then the object itself gets the same two calls. `v` is argument > 0. Both helpers do nothing for an entity that is not in their registry: 0x236624 searches the list of the entity's room (`[[entity + 0x20] + 0x10]` added to the pointer at `[[*0x377434] + 0x1c]`, entries of two words, entity and record) and sets bit 4 of the record byte +4 to v and bit 6 to !v; 0x2219bc searches the 12-byte records (entity first) of the registry at 0x58e9a4 and stores v in the byte +0xa. The records are the use-zone registrations of 0x23e130 ([carrying.md](carrying.md#ts-20261015-001729), [use-spot.md](use-spot.md)); which classes register one is not settled (56 call sites).

**Main-level reach.** The 342 ToggleUseable calls of the main levels name objects of kind 58 (98), 185 (53), 200 (28), 122 (17), 21 (13), 195 (12), 201 (12), 71 (11), 20 (10), 133 (10), 231 (10), 56 (9), 159 (9), 119 (6) and 16 further kinds. 11 calls name a nested object by `Parent.Child` (`prop_switchlever` 9, `prop_fasttravelzone` 2). Only the 11 calls on kind 71 reach children.

**EnableShadow 0x1bf84c.** Resolves the object with 0x22dacc, then 0x21aae0: true for kind 27 or 149, for a kind in the list at 0x3743ec (a count byte, the kind bytes at 0x515a94; one entry, kind 159, in the oracle; who fills it is not known) or when flag bit 14 of the word `obj + 0xc` is set. Then `comp[0x316]` bit 2 = argument > 0. The 8 calls in the main levels name `char_ai_actor` (2), `char_ai_cherrytreehillsgang1` (2) and `char_boosterstrange` (4), all kind 27.

**EnableSwitch 0x1bf8a4.** Argument > 0 (a NaN counts as not greater): the class message 8 to the object (0x244fc8), then 0x236624(object, 1). Otherwise: for kinds 94, 139, 100 and 58 the halfword at `comp[4]` becomes 3; then 0x236624(object, 0). The 9 calls in the main levels name `ability_breakandenter_door1` (4), `ability_breakandenter_skylight01` (3), `prop_dynamitecrate` (1; kind 190) and `prop_switchpull` (1; kind 58).

**enableobjectfade 0x26fac4** (read again): sends message 24 with the argument pointer; a handler that returns non-zero ends it, otherwise the argument 0 calls 0x2428c0 and any other value 0x242cf0 (the node calls EnableObject also uses). The 41 calls in the main levels name classes of kinds 29 (10), 17 (6), 20 (6), 0 (6), 47 (2), 56 (2), 86 (2), 138 (2), 1, 67, 113, 127 and 145. Which of these handlers answer message 24 was not read (kind 86 has no message handler).

**DistanceFadeMid** (3 calls, `ability_flowerplatform01..03`, kind 58) is gated by the node kind test, not by the entity kind: SCRIPT-282.

Unknowns:
- Which classes register a use zone with 0x23e130 (the registries of 0x236624 and 0x2219bc), so which ToggleUseable calls change anything.
- The contents of the kind list at 0x3743ec (who fills it) and flag bit 14 of `obj + 0xc` (the other parts of the EnableShadow gate).
- The handlers of the 13 kinds named by enableobjectfade for message 24.
- A class without `ObjectType` getting kind 0 follows from the lookup's default (r3 = 0 at 0x1a4298) and from the census (`dummy/dummy` and others place entities that never run class code); the +0x12 of a kind 0 object was not read in the oracle.
