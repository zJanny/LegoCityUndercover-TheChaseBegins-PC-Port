# Reverse-engineering findings

Everything recovered about the game's formats and code lives here, one file per topic.
Read this index, then open only the file you need.

> **All addresses here are for one exact build of the game (Europe, title ID 00040000000AD600, v1.0, image SHA-256 078faf7e…59ad). With any other image they are probably wrong.** Check the image first: [game-version.md](game-version.md).

| File | Contents |
|---|---|
| [game-version.md](game-version.md) | The reference build (hashes, title ID, version), how to check an image, the different-version warning |
| [container-and-archive.md](container-and-archive.md) | CCI/NCCH identity, RomFS, FUSE `.fib` archive and RefPack codec |
| [arm-executable.md](arm-executable.md) | Startup chain, SVC stubs, services, paths, HID input, channels, main loop |
| [pooled-resources.md](pooled-resources.md) | Shared layout of all pooled `b*` formats |
| [textures.md](textures.md) | `.btga/.bpng/.bdds`, v orientation, DMPGL notes |
| [audio.md](audio.md) | `.bwav`/CWAV decode, mixer |
| [models.md](models.md) | `.bfnmdl`, vertex layout, materials, shaders, UV scales, lightmaps (collision section: see collision.md) |
| [materials.md](materials.md) | Material record fields, the texture-environment stage generator 0x12e460, blend/alpha test/depth/cull, material census, lighting measurements |
| [rendering-fog.md](rendering-fog.md) | Fog: level parameters (`FogStart`, `FogEnd`, `FogColour`), the per-draw rule, the 128-entry PICA fog table and how it is built, room pop-in |
| [rendering-lighting.md](rendering-lighting.md) | Fragment lighting: light objects and classes, the light list 0x5943b0, per-draw registers of 0x12d884, lighting init and lookup tables |
| [rendering-sky.md](rendering-sky.md) | Sky models: the sky pass 0x180fac (camera orientation, eye at (0, lift, 0)), draw order, depth, the Sea lift |
| [view-edges.md](view-edges.md) | Which draw, spawn, fade, shadow and room decisions depend on the view's width (the widened 16:9 view of the `qol` plan) |
| [rendering-glow.md](rendering-glow.md) | The glow post-process 0x2bf830: copies, blur tables of modes 0xc / 0xd, the add with mode 0x12, callers and strengths |
| [rendering-draw-order.md](rendering-draw-order.md) | The draw lists 0x530634: material key 0x115128, depth key 0x25688c, the library sort, which parts are blended, particles in the list, and the near-camera semi-transparent lists (0x1ddde4) |
| [rendering-reflections.md](rendering-reflections.md) | Reflection cube maps: the reflection flag 0x37796d, the cube subroutine of the vertex programs, `TexTrans`, face registers, census |
| [rendering-shadows.md](rendering-shadows.md) | Shadows: the shadow light, focus and orthographic shadow camera, Shadow_Caster / Shadow_Receiver, the two receiving stages, oracle comparison |
| [levels.md](levels.md) | `.blvl`, placements, Euler order, nav grids (`.bfnnav`) |
| [level-transitions.md](level-transitions.md) | Details moved out of the level-transition feature items (natives without handler, loading page code) |
| [objects.md](objects.md) | Level objects: classes, class files (+0x10/+0x14), parameter slots, how the model is found, drawn classes |
| [object-kinds.md](object-kinds.md) | Kind byte of a placed entity (entity +0x12 = ObjectType): where it is set, the kind of every class of the main levels, what it gates (ToggleUseable group, EnableShadow, EnableSwitch) |
| [buildit.md](buildit.md) | Build-its (`props/prop_buildit`, `leprops/leprop_buildit`): class record, use zone, state machine and timing, completion and the event it fires |
| [buildit-bricks.md](buildit-bricks.md) | Build-it bricks: scatter, flight, built pose, hop of unbuilt bricks, rise of the final prop |
| [super-buildit.md](super-buildit.md) | Super build-its (`props/prop_superbuildit`) and their holders: the helipad of mission 1-3, the "needs N super bricks" prompt page, the confirmation, the movie, the build and the super brick count |
| [fast-travel.md](fast-travel.md) | Fast travel pad (`props/prop_fasttravelzone`, kind 227): use query, the player's states 0x180..0x183, the Helicopter's landing, what opens the fast travel map |
| [abilities-detective.md](abilities-detective.md) | Detective tool: detective spot, reveal spot, reveal prop, footprints, the player's detective states and their timing |
| [hand-items.md](hand-items.md) | Hand item track (hash 0xba1e5b53): every object a clip shows in the hand (glass, scanner, zipline items, hammer, axe, whistle ...), the generic rule and the item list |
| [use-spot.md](use-spot.md) | Use spot (`props/prop_simpleuse`): class record, handler 0x1922e8, one-shot rule, oracle trace, the unfired Door_Lock_Hint_Script object |
| [abilities-scanner.md](abilities-scanner.md) | Sound scanner (`props/prop_scannerzoneaudio`, type 201): use press, player states 0x156..0x158, view, zoom, lock, scan, fired events |
| [abilities-scanner-overlay.md](abilities-scanner-overlay.md) | Sound scanner overlay: layouts, reticle, sound markers, info boxes, frame tint, sonar |
| [playtests.md](playtests.md) | What the user saw playing `chase-game`: confirmed behaviour and the bugs found, each with a feature item |
| [carrying.md](carrying.md) | Carry objects and targets (`props/prop_carry*`): class records, use zones, pickup, drop, placing, fired events |
| [carrying-animation.md](carrying-animation.md) | Carry states (pickup, drop, place): clips, hand attachment, the release, what ends each state, the track event that attaches and releases |
| [skeleton-animation.md](skeleton-animation.md) | `.bfnskl`, `.bfnanm`, curves, pose composition, skinning, bone palettes |
| [cutscenes.md](cutscenes.md) | `csdata` cutscenes: tracks, cameras, actors, soundtrack |
| [cutscene-actors.md](cutscene-actors.md) | Which entities the cutscene cast are (`CSCharacter`, `Player1.Player01`), faces and clothes |
| [cutscene-actors-props.md](cutscene-actors-props.md) | Cutscene actors that are level props or vehicles: the dog of `1_2_c`, setup and teardown, the dog truck |
| [text-localisation-fonts.md](text-localisation-fonts.md) | `.loc`, text escapes, `.tnf` fonts |
| [data-tables-config.md](data-tables-config.md) | `.bxls` tables, brace-block config text |
| [pregen-traffic.md](pregen-traffic.md) | `Pregen/Traffic` and `Pregen/Pedestrians` spawn files: loaders 0x2e02d4 / 0x2ee868, spawn points on level paths, the quadtree of spawn lists and its lookup 0x1eb490, census |
| [save-format.md](save-format.md) | `savegame.dat` layout, CRC, save thread, options, slot progress, level table; later entries: [save-format-state.md](save-format-state.md) |
| [save-format-state.md](save-format-state.md) | Level state blocks, unlock bits, object records, where Continue starts, the save request chain and slot row fields |
| [ui-layouts.md](ui-layouts.md) | `.bxaml` UI layouts and storyboards |
| [ui-screens.md](ui-screens.md) | Census of every UI screen (55 layouts and code-drawn screens: loader, opener, screen, port state); the pause screen: mode 0x527358, page manager, round menu 0x376014, touch |
| [ui-screens-pause.md](ui-screens-pause.md) | The pause screen of a level: mode 0x527358, page manager, round menu 0x376014, page 0, the Options page and the Languages page, touch and close (moved from ui-screens.md) |
| [fmv.md](fmv.md) | Mobiclip `.moflex` video and subtitles |
| [debris.md](debris.md) | Brick debris of vehicle contacts, destroyed props and the car break (dropped pickup records, Debris01..20), the hit flash of a vehicle, its smoke emitter, oracle traces |
| [particles.md](particles.md) | `particledefs.bprt` definitions and the particle runtime: start entry, emitter update, particle init and step, quad draw state, callers, oracle check |
| [scripts.md](scripts.md) | Level scripts (section +0x18), the script executor 0x234204 and its words, the per-update runner, EnableObject / EnableCollision, the `startup` script, the revealer links, the start state of placed objects; runtime entries: [scripts-runtime.md](scripts-runtime.md) |
| [scripts-runtime.md](scripts-runtime.md) | Trigger records and bound switches, the police entrance, WarningDialogBox / PositionObject / FadeScreen, a live script thread of the original |
| [script-text.md](script-text.md) | Tip text, dialogue lines, dialogue background, mission title: handlers of batch 30, the timed objects 0x517ef8 / 0x5189cc, measured timing and layout |
| [script-text-bottom-screen.md](script-text-bottom-screen.md) | Bottom screen panel: dialogue background fade (panel alpha 0x51a2d4), ShowDialogBG byte 0x51a2bd and FadeScreen's screens, BlackoutMap, the InformationBox picture |
| [objective-hud.md](objective-hud.md) | Waypoints, sub-objective records and arrows: handlers of batch 31 (SetActiveWayPoint 0x27bde8, SubObjectiveGoToEnable 0x2adaac, arrow natives), the manager update 0x2ed8d8 / 0x2be934, group map slots, arrow quads |
| [objective-hud-map.md](objective-hud-map.md) | Map screen icons: placement 0x1ee7a8, edge ellipse and arrow, halo, grow, pulse rings (0x2db0dc, 0x2ddd64) |
| [objective-hud-waypoints.md](objective-hud-waypoints.md) | The waypoint column and its badge, the node tween fades, mission 1-1 flow through waypoints |
| [ai-natives.md](ai-natives.md) | The 17 AI natives of batch 34 (AIMove 0x171f7c, AIEnable 0x1881d4, ...): AI component fields, requested state numbers, the AIMove trigger event |
| [ai-navigation.md](ai-navigation.md) | AIMove path finder 0x204284 on the nav grids, grid and cell search, follower 0x27ddb4, arrival and the AIMove event 0x2c630c, state 11 (AISuperJump) 0x2c3bd8 |
| [ai-states.md](ai-states.md) | AISuperJump set-up 0x213300 (frames, +0x308, +0x30c), state 10 face target 0x2c5bc4, state 11 arrival value (ATTR_RunSpeed), state numbers 25 / 27 / 35 / 36 |
| [script-triggers.md](script-triggers.md) | Check functions of every bound trigger type (BoundEntered / Left / Inside / Outside, on-foot and vehicle variants, object bounds), census of lvl_01 and lvl_00, oracle runs of BoundLeft and PlayerOnFootBoundEntered, why a teleport does not fire BoundLeft; the reference-trigger step 0x23211c (records, lists, events, messages, registering classes, callers). |
| [scene-start.md](scene-start.md) | Scene start: class `startup` scripts of placed objects, camera bound switches (0x2ff35c), the trigger rule, `OnSceneEnter<room>` / `OnSceneExit` events (0x1ecc50), `ScriptCheckExtras` (0x2e5638) |
| [script-natives.md](script-natives.md) | Script natives: how the census was made, handler ABI, classes A-D and the work batches SCRIPT-26 to SCRIPT-39 |
| [script-natives-census.md](script-natives-census.md) | Census table of all script natives, kept up to date: handler addresses, argument counts, call counts per level, class, batch and state |
| [script-natives-flags.md](script-natives-flags.md) | Batch 26 handlers (object flags, KillScript, class messages): split from script-natives.md |
| [script-natives-progress.md](script-natives-progress.md) | Batch SCRIPT-27: handlers of the progress, save and stud natives, the slot bytes they use |
| [script-natives-audio.md](script-natives-audio.md) | Music natives OverrideMusic, AutomaticMusic, StopMusic, the music block 0x376b30, EnableTrafficAndPedestrians and the camera manager request (+0x28) |
| [npc-animation.md](npc-animation.md) | Idle state 0x1d5070 of level characters: animation slots and position rule (0x24cbd4), blend weights, the random idle / idle2 choice (0x219614), RNG 0x25b2b8, oracle timeline |
| [pedestrians.md](pedestrians.md) | The pedestrian manager 0x51b33c (slots `Pedestrian%02u`, limit, countdown, update 0x2ee5e8, spawn 0x2ee068), `props/pedestriansystem` and its paths, the pregen loader 0x2ee868; the correction of the swapped traffic and pedestrian managers; the wiring in `chase-game` |
| [pedestrians-character.md](pedestrians-character.md) | The pedestrian character (kind 197): create 0x1913f8, state step 0x1d7e88, walk 0x2b9138, avoidance 0x2edf08, the steady updates of states 3, 4, 5, 7 (0x277afc, 0x277c94), draw test 0x2930e0, type 1 path 0x1b33fc |
| [pedestrians-inputs.md](pedestrians-inputs.md) | The pedestrian's vehicle test 0x1ffcd4, its dodge ray 0x22b108 and the vehicle list 0x5863b4 + 0x305c, the room layer table *0x376ad4 and its cutscene push, the class byte +0x18, room node +0x7c, the mesh node type *0x37674c, the fade record 0x376794, the cutscene update caller 0x164240, the pedestrian room move 0x2e6ce4, priority 128, vehicle speed 0x34163c, player state +0x6c |
| [pedestrians-fade.md](pedestrians-fade.md) | The spawn fade-in 0x275434, the per-frame node alpha step 0x10eb70, the mesh node set-up 0x10d2d4 and the run-time value of the mesh node type *0x37674c |
| [script-natives-animation.md](script-natives-animation.md) | Batch SCRIPT-32: handlers of the 13 animation and attach natives (PlayCharAnim 0x29e5f0, PlayAnimation, LoadAnimation, the WaitForCurrent natives, Attach*), corrects the census row of PlayCharAnim |
| [script-natives-animation-player.md](script-natives-animation-player.md) | Batch SCRIPT-32, Player1: PlayCharAnim and the user-animation state 0x50, root motion of a script clip, the order of a state change, the first pose and the hand-back |
| [script-natives-world.md](script-natives-world.md) | The mission 1-2 natives DestroyProp / KillObject (0x23cd80), EffectFadeObject (tween registry), EnableDistanceFade and the distance fade of mesh nodes (0x10eb70, measured thresholds), AddToSemiTransNearCameraList, SetPartyCharacter (0x245258), StreamRoomIn and the room request list |
| [script-natives-world-death.md](script-natives-world-death.md) | The death of a character (message 1 of kinds 27, 121, 242, 0x18e93c): `DisableFallapart` modes, the fall-apart pool 0x1d3b50 / 0x231a08 / 0x1d27f4, the bricks, the stud drop (count 0, nothing dropped), the state DEAD with its spawn counter 0x225f5c and the after-death 0x21e218 |
| [script-natives-world-reset.md](script-natives-world-reset.md) | ResetObject / EnableSwitch message 8: kinds 64, 132 and the default handler 0x221800, entity +0x44 (skeleton), 0x237c48 rotation order |
| [script-natives-party.md](script-natives-party.md) | Batch SCRIPT-35: handlers of SwapAllegiance, SetWeaponEnabled, CharSetAllowVehicleEntry, SuperFreeplaySwap, the boss hooks (Walnuts, Rex, 1-8), mech, skydive, beam and downtown intro natives |
| [scripts-missions.md](scripts-missions.md) | Mission gap analysis: the natives and trigger events a whole mission needs, oracle trace of the start of mission 1-2 (waypoint class starts the script, call order and counts) |
| [scripts-missions-1-4.md](scripts-missions-1-4.md) | Mission 1-4 (the car showroom): scripts, natives without handler, trigger events, the AI spawner chain, texts, the order of the first part |
| [doors.md](doors.md) | Doors (type 42): record, update 0x1d16b8, state change, messages, easing, hinge and slide pose, timings measured in the oracle |
| [switches.md](switches.md) | The switch base (loader 0x23e84c, state machine 0x21435c / 0x22c6d0, handler 0x23aac0, states 0 to 5), the lever class (kind 58: use zone, message 3, handler 0x17f6f4) and the mission 1-3 key switch |
| [studs.md](studs.md) | Studs (collectible coins): pickup pool records, sprite sheet drawing and spin, pickup radius and skip counter, collect timer, counter values and limits, stand-ins; dropped studs are in [studs-drops.md](studs-drops.md) |
| [studs-drops.md](studs-drops.md) | Dropped studs (0x21e2e4 modes 0..2, 0x19913c, 0x2149ac), their callers (wobble hit, build-it, player hurt and death, hit-sequence slots, ability props) and which classes run the generic state machine 0x22c6d0 |
| [super-bricks.md](super-bricks.md) | Super bricks: the placed pickups (`pickup_sbbrick*`, pickuptype 103), pickup rule, destroy, the count at 0x593430 and its save slot field, the roof brick of mission 1-3 |
| [super-bricks-prompt.md](super-bricks-prompt.md) | Super bricks, subtopic: the two placeholders of the super build-it prompt in the oracle pictures |
| [hud.md](hud.md) | In-game HUD: layouts, portrait, hearts (SetHearts, loop rule, health default), stud bar (state machine, formatting, icon bounce, no roll-up for pickups), flying studs (law, unprojection, sinks), bottom-screen map placement |
| [streaming.md](streaming.md) | World levels and the master level, scene array, room objects and states, room links (`.blvl` +0x34), event bindings (+0x1c), the `SceneChange` chain, `StreamRoomIn`; `pregen/` is not streaming; the observed chain: [streaming-transitions.md](streaming-transitions.md) |
| [streaming-transitions.md](streaming-transitions.md) | The observed transition chain: why the Lower bound did not fire, station entry, lobby exit and the scene change update |
| [streaming-rooms.md](streaming-rooms.md) | The per-update room request rule: current room link list, view-ranked preloads, unload by absent request, room request word layout |
| [oracle.md](oracle.md) | Azahar runtime oracle: entry point, setup, fidelity rules, fast workflow, pointers to the subtopic files |
| [oracle-setup.md](oracle-setup.md) | Oracle launch: Xvfb, render-window crash and launch retry, visible desktop mode |
| [oracle-rpc.md](oracle-rpc.md) | Oracle RPC channel: UDP memory access, screenshots, pad input without breakpoints, timings |
| [oracle-routes.md](oracle-routes.md) | Oracle route scripts to gameplay and scenario pitfalls (`teleport`, input that survives a load) |
| [oracle-measuring.md](oracle-measuring.md) | Oracle measuring methods: watchpoints, execution traps, fixed dt, memory traces, `poke` |
| [camera.md](camera.md) | Gameplay camera: pointers to the three files below |
| [camera-follow.md](camera-follow.md) | Yaw word 0x376894 writer, manager and modes, follow camera law (OnFoot preset), projection |
| [camera-follow-handover.md](camera-follow-handover.md) | Follow controller take-over: reset 0x2313c8 seed, near rule and filter ramp of 0x1b4864 |
| [camera-vehicle.md](camera-vehicle.md) | Vehicle camera: chooser 0x1597c8, InVehicle block and record floats, 15 m behind, automatic turn, pitch clamp 0x2e0a50, enter and leave |
| [camera-collision-sweep.md](camera-collision-sweep.md) | Obstruction by level geometry and placed objects (slab sweep, pull-in, side and vertical push, box slicing) |
| [camera-records-blending.md](camera-records-blending.md) | Camera lock, level-bound records, record pose 0x298cd8, fade between commands, pose filter |
| [player.md](player.md) | Player entity and controller: pointers to the subtopic files below |
| [player-entity-input.md](player-entity-input.md) | Player entity pointer, scene node, controller fields, stick-to-intent, position and character source, head attachment |
| [player-locomotion.md](player-locomotion.md) | Locomotion rules and root motion, replica, ground following, derivation of the speeds |
| [vehicles.md](vehicles.md) | Vehicles: pointers to the three files below |
| [vehicles-system.md](vehicles-system.md) | Vehicle class 183: record, handling data in class files and the vehicle table, update chain, physics body (located), natives of mission 1-1, wheels, police car collision |
| [vehicles-boarding.md](vehicles-boarding.md) | Entering and leaving a vehicle (paths, timing, events) |
| [vehicles-driving.md](vehicles-driving.md) | Driving: input, state machine, measurements |
| [vehicles-waypoints.md](vehicles-waypoints.md) | Waypoints reached in a vehicle, script jump in / out natives |
| [vehicles-physics.md](vehicles-physics.md) | Vehicle physics body: rigid body, world step, wheels, engine, tyres, wheel pose |
| [vehicles-physics-collision.md](vehicles-physics-collision.md) | Vehicle physics collision: the Bullet world, floor collision pass, sleep rule, contacts with the static Bullet boxes |
| [vehicles-damage.md](vehicles-damage.md) | Vehicle contact damage: the contact and damage messages of 0x1797c4, hearts and health points, cool-downs, the pair rule of 0x271764, the break 0x16cccc, the oracle run into the box |
| [vehicles-physics-props.md](vehicles-physics-props.md) | Vehicle physics: the second floor cell search 0x164328 (push-out of points below an overhead layer), the vehicle against placed props, the car breaking into bricks, the sphere sweep of the floor pass 0x1c54f8 (0x1b6ef8, 0x16788c, set-up 0x33a4fc), the wheel ray through the floor graph 0x155888, mass scale and body activation messages |
| [vehicles-prop-smash.md](vehicles-prop-smash.md) | Which props a vehicle contact destroys: mesh-kind grid entries, the reaction of each prop class to message 0, bricks released, timing against the oracle. |
| [traffic.md](traffic.md) | The traffic manager 0x527410 (spawn, removal, countdown, fade bounds, SetTrafficMaxVehicles), its wiring in `chase-game`, the spawn placement, the cars' node and driver values, the LOD table and the removal radius |
| [traffic-ai-driver.md](traffic-ai-driver.md) | The junction routes (route list, EnableTrafficPath, path follower, junction test) and the AI driver (record, input provider 0x26d934, state change 0x1cb148, post step 0x1bc79c, kinematic traffic cars) |
| [player-falling-jumping.md](player-falling-jumping.md) | Falling, landing stun, jumping, step-off slide, slope slow-down, state table |
| [player-attack.md](player-attack.md) | Attack on foot: Y tap, combat action states 0xa3 / 0xa4, combo, target search, hit test, messages 62 and 0 to the target |
| [player-traversal.md](player-traversal.md) | Zipline (type 101: grab test, states 42..44, slide, dismount, oracle ride), survey of the climbing moves |
| [player-traversal-pole.md](player-traversal-pole.md) | Pole and drain pipe (type 187): walk-in and A-press mount, climb, exits, dismount (the save state climbing the drain pipe of mission 1-2) |
| [player-traversal-bricks.md](player-traversal-bricks.md) | Climbing bricks (type 20, states 141..152), the climb wall entry (type 30) (the save state climbing the bricks of mission 1-2) |
| [player-traversal-rails.md](player-traversal-rails.md) | Rails, climb bars (type 29) and the corner rail (type 218), ledge states 23 / 24 (the save state climbing along the railing of mission 1-2) |
| [player-traversal-fence-vault.md](player-traversal-fence-vault.md) | Fence vault (kind 189): the B press gate, the approach to the fence and state 326 (the fence of the mission 1-3 lawn mower rescue) |
| [player-traversal-ladders.md](player-traversal-ladders.md) | Ladders (kind 56): use zone, mount, climb, clamber state 23, exits (the save state in front of the crane ladder of mission 1-3) |
| [props-wobbly-bouncer.md](props-wobbly-bouncer.md) | The wobbly destructible props (hit message 0, health, ObjectHit, destroy chain, TriggerObject of the table to the trampoline build-it) and the bouncer class (message 15, take-off speed rules, the bounce flight), oracle traces of the save state in front of the table of mission 1-2 and the save state at the built trampoline of mission 1-2 |
| [props-rocking.md](props-rocking.md) | The rocking of the kind 67 props (`Rocking`, `WaveBob1/2`, 0x28a758), SetPropRocking 0x1da6a8, WaitForPlatform 0x263b84 and the order of `1-3_Generator_Script` |
| [props-rocking-platforms.md](props-rocking-platforms.md) | The platform class (kind 64): record, trigger messages, travel and node move; AttachObject's node hierarchy that carries attached objects |
| [props-rocking-attach.md](props-rocking-attach.md) | Attached placed objects following the parent's world, the rocking clock's rate and divisor, the generator scene's camera calls |
| [pushables.md](pushables.md) | Pushables (kind 71, `leprops/leprop_pushable`) and their handles (kind 72), the player's handle states 84 to 86, and the push cart of mission 1-3 |
| [front-end.md](front-end.md) | Boot sequencer 0x302080, screen order and triggers from power-on to the first level, prompt state machine 0x313368, save indicator, language, savegameflow pages, loading screen, intro movie bubble and title card |
| [collision.md](collision.md) | Level collision data (model+0x1c section), ground probe by box bisection, wall slide, placed objects (world grid, registration, shapes, query layers), differential test against traces |
| [collision-nested.md](collision-nested.md) | Collision of the objects class files nest, of the super build-it plate (kind 98) and of the mission 1-3 helipad |
| [dcam.md](dcam.md) | Director camera: level paths, StartDCam commands, phase machine, busy test and end of WaitForDCam, hand-back, oracle timing |

Python tools that were later removed from the repository (convert_models, convert_anims, convert_nav, tt_resource, preview_model, the census modes of convert_levels, and the Python decoders behind convert_textures, convert_audio, convert_loc, convert_font and extract_fib) last existed at commit `9d04c91`. Older entries that name them stay as written.

Rules for editing these files are in CLAUDE.md ("Reverse-engineering findings"). Open items and stand-ins per feature are in [../plans/features/README.md](../plans/features/README.md).
