# Feature index

One file per feature: scope, status, verified behaviour, stand-ins, open items (stable IDs), a findings summary that links into [docs/re/](../../re/README.md), and code and tests. Read only the section of this index for your area, then only the file for your feature, then only the docs/re entry it links. Rules: CLAUDE.md "Feature files". Work state (status, steps, log) is in the progress file of each plan ([plan index](../README.md)); features of a plan other than `linux-port` sit under that plan's own `## <Area>` headings. Limits: a feature file at most 16 KB (split a larger one as `<feature>-<subtopic>.md`, one row each). `python3 tools/check_doc_links.py --fix` moves closed items (`- [x]`) to `<feature>.closed.md`; closed files are not listed here. Item format: `- [ ] ID — gap. needs: ID. blocks: ID. uses: Name, Name. evidence: link`; `uses:` is optional and lists script natives or object classes of the level data that the item affects (it replaces the automatic match used by [priorities.md](../priorities.md), the generated ranking by reach in the main levels). `needs:` and `blocks:` are written on both sides. Stand-ins use `PREFIX-S<n>`. New feature: copy the section list of any file here, add it to this table. Section list: 1 Scope, 2 Status, 3 Verified behaviour, 4 Stand-ins, 5 Open items, 6 Findings summary, 7 Code and tests. Missions and enemies have no feature file yet (NOT_STARTED): mission flow needs SCRIPT-1; enemies and fighting are not recovered.

## Player and camera

| Feature | Status | Top open items |
|---|---|---|
| [player-movement](player-movement.md) | IN_PROGRESS | PLAYER-1 slide landing stun rule; PLAYER-2 slide exit; PLAYER-3 slide probe and timeout |
| [player-movement-attack](player-movement-attack.md) | IN_PROGRESS | PLAYER-26 oracle check against goons; PLAYER-28 fight move motion; PLAYER-30..32 engaged table and unread character facts (punch on props, fight move on goons, line of sight and near list ported) |
| [player-traversal](player-traversal.md) | IN_PROGRESS | TRAV-66 other users of the B press list; TRAV-34 pole A mount; TRAV-19 brick side jumps; TRAV-2 climb wall (bricks and fence vault ported) |
| [player-traversal-zipline](player-traversal-zipline.md) | IN_PROGRESS | TRAV-9 tightrope; TRAV-55 solid poles; TRAV-8 use press measured; TRAV-63 handcuffs on the rope (zipline ported) |
| [player-traversal-pole](player-traversal-pole.md) | IN_PROGRESS | TRAV-11 entry from the air; TRAV-13 walk-in touch; TRAV-15 sounds (pole ported) |
| [player-traversal-rails](player-traversal-rails.md) | IN_PROGRESS | TRAV-27 bar variants; TRAV-28 hang offset; TRAV-31 other corner direction; TRAV-42..45 acrobat pole leftovers (rails and flag poles ported) |
| [player-traversal-ladders](player-traversal-ladders.md) | IN_PROGRESS | TRAV-58 entry from the air, furled ladders; TRAV-59 exit collision; TRAV-60 clip restart, hidden weapons, camera; TRAV-61 oracle check of the sounds (ladders ported) |
| [camera](camera.md) | IN_PROGRESS | CAMERA-55 police car after PositionObject at the hand-back; CAMERA-3 multi-record weights; CAMERA-2 characters and vehicles not registered |
| [collision](collision.md) | IN_PROGRESS | COLL-1 broad phase stand-in; COLL-2 constructors missing (about 40 kinds, characters); COLL-23 transcribe them; COLL-24 nested start state |
| [combat-characters](combat-characters.md) | IN_PROGRESS | COMBAT-10 counter takedown and grab; COMBAT-8 takedown motion; COMBAT-15 strike of an AI character; COMBAT-12 arrest against the oracle (hit receiver, takedown and arrest by A ported) |
| [animation-characters](animation-characters.md) | IN_PROGRESS | ANIM-3 blending; ANIM-10 slot bookkeeping; ANIM-11 model folders |
| [input](input.md) | DONE (lab, mapping) | INPUT-2 readers of input slots 16/17 |
| [abilities-detective](abilities-detective.md) | IN_PROGRESS | DETECT-2 first point never enabled (Doorlock script waits); DETECT-1 use-state approach; DETECT-4 particles, fades, flight |
| [abilities-scanner](abilities-scanner.md) | IN_PROGRESS | SCAN-2 other zone modes; SCAN-10 sonar segments; SCAN-19 tip pages during the scanner view; SCAN-16 and SCAN-17 info box comparison and icon writer |
| [disguise](disguise.md) | IN_PROGRESS | Party manager ported; DISG-9 swap of the player entity; DISG-1 class look of the HUD; DISG-3 party in the save and at level entry; DISG-5 D-pad swap |

## Level objects

| Feature | Status | Top open items |
|---|---|---|
| [level-objects](level-objects.md) | IN_PROGRESS | LEVOBJ-3 objects frozen (needs SCRIPT-1); LEVOBJ-16 other NPC states |
| [level-objects-buildit](level-objects-buildit.md) | IN_PROGRESS | LEVOBJ-76 studs of the build; LEVOBJ-77 two-target camera; LEVOBJ-43 built state not saved (needs SAVE-8) |
| [level-objects-buildit-super](level-objects-buildit-super.md) | IN_PROGRESS | LEVOBJ-117 reference step, map icon, progress counters, NewPlayerLocation; LEVOBJ-129 sender of message 6 |
| [level-objects-fast-travel](level-objects-fast-travel.md) | IN_PROGRESS | LEVOBJ-120 the question and trip of the map; LEVOBJ-130 map page drawing; LEVOBJ-121 pad camera and particles; LEVOBJ-123 arrival untestable before the trip |
| [level-objects-carry](level-objects-carry.md) | IN_PROGRESS | LEVOBJ-75 release of the box; LEVOBJ-30 attach update; LEVOBJ-31 locomotion while carrying; LEVOBJ-32 sounds and effects |
| [level-objects-wobbly](level-objects-wobbly.md) | IN_PROGRESS | LEVOBJ-87 wobble of a hit; LEVOBJ-89 bounce states beyond the motion; LEVOBJ-90 the bouncer object |
| [doors](doors.md) | IN_PROGRESS | DOOR-1 script-requested doors stay closed (needs SCRIPT-7); DOOR-3 open-door box |
| [studs](studs.md) | IN_PROGRESS | STUD-1 magnet and multiplier; STUD-4 groups (needs SCRIPT-1); STUD-15, STUD-25, STUD-26, STUD-27 callers of the stud drop |
| [studs-super-bricks](studs-super-bricks.md) | IN_PROGRESS | STUD-28 camera shift of the flight; STUD-16 placed brick leftovers; STUD-24 flight target height |
| [particles](particles.md) | IN_PROGRESS | PART-2 script natives and most callers start no emitters; PART-3 `particles` level object; PART-5 update trace |

## Vehicles

| Feature | Status | Top open items |
|---|---|---|
| [vehicles](vehicles.md) | IN_PROGRESS | VEH-7 driving collision; VEH-21 driving details; VEH-10 sounds |
| [vehicles-damage](vehicles-damage.md) | IN_PROGRESS | VEH-125 debris line tests; VEH-140 rest of the debris pickup; VEH-121 rest of the break effects; VEH-120 rest of the return |
| [vehicles-physics](vehicles-physics.md) | IN_PROGRESS | VEH-42 inertia; VEH-50 air control; VEH-54 jump-out braking |
| [vehicles-physics-collision](vehicles-physics-collision.md) | IN_PROGRESS | VEH-102 sphere contact at a wall; VEH-70 Bullet contact details; VEH-141 activation rule for parked vehicles |
| [vehicles-prop-smash](vehicles-prop-smash.md) | IN_PROGRESS | VEH-131 oracle check of a kind 67 smash; VEH-134 hit sequence and lights modes; VEH-130 lamp post one update early |
| [vehicles-boarding](vehicles-boarding.md) | IN_PROGRESS | VEH-19 boarding gates and exit probes; VEH-20 boarding state reactions; VEH-80 head icons |

## Scripts and missions

| Feature | Status | Top open items |
|---|---|---|
| [script-system](script-system.md) | IN_PROGRESS | SCRIPT-1 mission flow (needs SCRIPT-7 natives, SCRIPT-75 / 76 trigger events); runner, scene start, class startups |
| [script-system-triggers](script-system-triggers.md) | IN_PROGRESS | SCRIPT-75 / 76 AI and vehicle trigger events; SCRIPT-77 / 78 object bound checks |
| [script-system-missions](script-system-missions.md) | IN_PROGRESS | SCRIPT-285 door lock hint object trigger; SCRIPT-190 carry gaps |
| [script-system-missions-reference](script-system-missions-reference.md) | IN_PROGRESS | SCRIPT-365 other switch kinds; SCRIPT-359 other callers; SCRIPT-360 receivers of the step; SCRIPT-366 lever details |
| [script-system-missions-1-3](script-system-missions-1-3.md) | IN_PROGRESS | SCRIPT-363 parts crate; SCRIPT-364 generator against the oracle; SCRIPT-383 attached props; SCRIPT-384 rocking clock |
| [script-system-platforms](script-system-platforms.md) | IN_PROGRESS | SCRIPT-377 orient and type 7 platforms, shape curves, lift timers; SCRIPT-385 oracle check of the path platforms; SCRIPT-388 the platform's push and squash; SCRIPT-389 the lift starts one update late |
| [script-system-missions-1-3-generator](script-system-missions-1-3-generator.md) | IN_PROGRESS | SCRIPT-364 generator scene against the oracle |
| [script-system-missions-1-3-end](script-system-missions-1-3-end.md) | IN_PROGRESS | SCRIPT-382 push cart and mission end against the oracle; SCRIPT-390 the lift's switch at the top |
| [script-system-missions-1-4](script-system-missions-1-4.md) | IN_PROGRESS | SCRIPT-435 the time before the arrival cutscene |
| [script-system-missions-1-4-chase](script-system-missions-1-4-chase.md) | IN_PROGRESS | SCRIPT-415 the chase; SCRIPT-462 draw the target display; SCRIPT-447 the hold of a VehicleSetAI car |
| [script-system-missions-1-4-fight](script-system-missions-1-4-fight.md) | IN_PROGRESS | SCRIPT-419 spawner details; SCRIPT-423 the StartState entries not ported |
| [script-system-missions-1-4-conduct](script-system-missions-1-4-conduct.md) | IN_PROGRESS | SCRIPT-428 rest of the idle decision; SCRIPT-441 inputs of the state 14 walk; SCRIPT-433 sleep clips |
| [script-system-missions-1-4-roof](script-system-missions-1-4-roof.md) | IN_PROGRESS | SCRIPT-414 rest of the roof scene; SCRIPT-430 thrown goon in play; SCRIPT-465 pair motion; SCRIPT-467 camera shot; SCRIPT-438 callers and time scale readers; SCRIPT-439 slow motion length |
| [script-system-use-markers](script-system-use-markers.md) | IN_PROGRESS | SCRIPT-299 markers of the other 34 kinds; SCRIPT-355 special cases and HeadMarker; SCRIPT-356 list builder; SCRIPT-357 placement ray |
| [script-system-natives](script-system-natives.md) | IN_PROGRESS | SCRIPT-7 natives in batches SCRIPT-29 to SCRIPT-39, SCRIPT-57 to SCRIPT-65 |
| [script-system-natives-world](script-system-natives-world.md) | IN_PROGRESS | SCRIPT-280 destroy message 1 per class; SCRIPT-281 effect fade rest; SCRIPT-282 distance fade gate; SCRIPT-283 AddToSemiTransList |
| [script-system-natives-world-death](script-system-natives-world-death.md) | IN_PROGRESS | SCRIPT-326 a character that comes back; SCRIPT-327 the player's branch; SCRIPT-328 AI helpers; SCRIPT-323 sound table, clock, textures |
| [script-system-natives-animation](script-system-natives-animation.md) | IN_PROGRESS | SCRIPT-270 player animations; SCRIPT-272 attach node changes; SCRIPT-273 / 274 states, loaded set |
| [script-system-natives-party](script-system-natives-party.md) | IN_PROGRESS | SCRIPT-338 character records read by nothing; SCRIPT-339 boss components; SCRIPT-340 mech piloting |
| [script-system-natives-state](script-system-natives-state.md) | IN_PROGRESS | SCRIPT-56 / 59 / 335 / 336 readers of placement and flags; SCRIPT-57 kind gates; SCRIPT-61 / 62 / 64 / 65 progress natives |
| [script-system-ai-navigation](script-system-ai-navigation.md) | IN_PROGRESS | SCRIPT-79 AI movement; SCRIPT-94 active nav grid set; SCRIPT-103 AISuperJump flight, SCRIPT-458..460 run-away, stalk and pursuit states, SCRIPT-461 rest of the fight decisions |
| [script-system-dcam](script-system-dcam.md) | IN_PROGRESS | SCRIPT-46 director camera commands, SCRIPT-48 / 49 / 50 / 181 / 182 details |
| [script-system-hud-natives](script-system-hud-natives.md) | IN_PROGRESS | SCRIPT-66 to SCRIPT-70 text screens; SCRIPT-73 map and scanner natives; SCRIPT-329 to SCRIPT-331 arrow hide, oracle check, enemy and carry groups; SCRIPT-354 vehicle waypoint steering |

## World and levels

| Feature | Status | Top open items |
|---|---|---|
| [world-streaming](world-streaming.md) | IN_PROGRESS | `chase-game` streams rooms (street level); WSTREAM-12 room position code; WSTREAM-30 loader cost; WSTREAM-4 visibility inside rooms |
| [pedestrians-traffic](pedestrians-traffic.md) | IN_PROGRESS | PREGEN-3 traffic floor cell; derived path, tangent and room ported; pedestrian file used in chase-game |
| [pedestrians-traffic-pedestrians](pedestrians-traffic-pedestrians.md) | IN_PROGRESS | run in chase-game with room moves; PED-30 pedestrian sounds; PED-27 fade point cameras |
| [pedestrians-traffic-pedestrians-fade](pedestrians-traffic-pedestrians-fade.md) | IN_PROGRESS | PED-21 faded-out pedestrians not removed; PED-22 node radius source; PED-23 node draw pass |
| [pedestrians-traffic-pedestrians-vehicles](pedestrians-traffic-pedestrians-vehicles.md) | IN_PROGRESS | vehicle test and set-up layer table run in chase-game; PED-18 cutscene layer table; PED-19 vehicle list filter |
| [pedestrians-traffic-cars-driver](pedestrians-traffic-cars-driver.md) | IN_PROGRESS | AI driver runs on the cars with states 4 to 8 connected, brake lights and culling; the chased car of mission 1-4 drives; TRAFFIC-42 thrown objects, TRAFFIC-43 contact gather |
| [pedestrians-traffic-cars-push](pedestrians-traffic-cars-push.md) | IN_PROGRESS | push vector with its shape tests, squash and events 17 / 18 ported; a traffic car sends message 45 to people in chase-game; TRAFFIC-66 their reaction |
| [pedestrians-traffic-cars-whistle](pedestrians-traffic-cars-whistle.md) | IN_PROGRESS | cop whistle connected (extras bit 19, D-pad Down, clip 709, clock); TRAFFIC-59 clock unit; TRAFFIC-60 character row byte |
| [pedestrians-traffic-cars-gather](pedestrians-traffic-cars-gather.md) | IN_PROGRESS | contact gather, grid build, registration pass and cell lists ported, chase-game feeds its characters to it; TRAFFIC-63 scene test, TRAFFIC-65 rebuild point |
| [pedestrians-traffic-cars-driver-body](pedestrians-traffic-cars-driver-body.md) | IN_PROGRESS | kinematic vehicle body per traffic car, follows the node; the player's car collides with it; open: TRAFFIC-50 engine value, TRAFFIC-51 hit measurement |
| [pedestrians-traffic-cars](pedestrians-traffic-cars.md) | IN_PROGRESS | cars spawn, drive and leave in chase-game on the junction routes; TRAFFIC-14 path record index; TRAFFIC-12 other levels |
| [pedestrians-traffic-cars-movement](pedestrians-traffic-cars-movement.md) | IN_PROGRESS | route mover, stop test, junction choice and queue run on the cars; TRAFFIC-29 ground cast order; TRAFFIC-30 colour of the probe |
| [pedestrians-traffic-cars-junction](pedestrians-traffic-cars-junction.md) | IN_PROGRESS | junction records, light cycle and traffic light looks ported and wired (TRAFFIC-25 done); TRAFFIC-31 room type of the junction rooms, TRAFFIC-26 light prop update |
| [level-transitions](level-transitions.md) | IN_PROGRESS | station exit and entry switch at run time; TRANS-3 master level (needs SCRIPT-1), TRANS-8 loading page code and look; TRANS-15 same-level entry timing, TRANS-16 oracle check of lift entries |

## Front end, HUD, saves

| Feature | Status | Top open items |
|---|---|---|
| [front-end](front-end.md) | IN_PROGRESS | FE-1 existing slots and Continue (needs SAVE-2); FE-30 delete confirmation; FE-3 loading screen 3D badge |
| [hud-ui](hud-ui.md) | IN_PROGRESS | HUD-6 hearts never change; HUD-11 map mission icons, HUD-90 hub player arrow, HUD-60 other map icon types; HUD-8 other counters |
| [hud-ui-text](hud-ui-text.md) | IN_PROGRESS | HUD-22 remaining text screens; HUD-30 character intro and box line counts; HUD-33 tip after the release camera |
| [ui-menus](ui-menus.md) | IN_PROGRESS | UI-20 characters panel; UI-21 map button actions; UI-22 map button gates; UI-11 shop and item grid pages (map buttons ported) |
| [ui-menus-pause](ui-menus-pause.md) | IN_PROGRESS | UI-2 pause fades and gates; UI-3 pause Exit; UI-31 item grid oracle check; UI-30 language change loads; UI-25 options save task (pause screen, Options and Languages pages ported) |
| [ui-menus-award-viewer](ui-menus-award-viewer.md) | IN_PROGRESS | UI-26 award models; UI-27 award mode fades and gates; UI-28 oracle picture (award viewer ported) |
| [hud-ui-super-bricks](hud-ui-super-bricks.md) | IN_PROGRESS | HUD-79 icon through the HUD camera; HUD-80 number font; HUD-85 build-menu placeholders; HUD-84 update count origin |
| [audio](audio.md) | IN_PROGRESS | AUDIO-20 duck record; AUDIO-11 action music; AUDIO-3 world sounds |
| [movies-cutscenes](movies-cutscenes.md) | IN_PROGRESS | MOVIE-36 end from the camera clip; MOVIE-3 short actor clips; MOVIE-41 pose words; MOVIE-1 layout values |
| [saves-options](saves-options.md) | IN_PROGRESS | SAVE-14 level state refresh of a save request, SAVE-18 HUD studs after a load, SAVE-13 Continue at level 2 and later, SAVE-2 slot display (blocks FE-1) |

## Rendering and assets

| Feature | Status | Top open items |
|---|---|---|
| [assets-formats](assets-formats.md) | IN_PROGRESS | ASSET-2 streaming |
| [rendering](rendering.md) | IN_PROGRESS | RENDER-68 glow post-process; RENDER-30 fog oracle comparison |
| [rendering-draw-order](rendering-draw-order.md) | IN_PROGRESS | RENDER-66 distance fade; RENDER-65 particles in the sorted list; RENDER-78 second draw of +0xde bit 2; RENDER-79 other cameras of the near-camera rule |
| [rendering-reflections](rendering-reflections.md) | IN_PROGRESS | RENDER-75 oracle comparison of the cube maps (blocks RENDER-S69) |
| [rendering-shadows](rendering-shadows.md) | IN_PROGRESS | RENDER-72 soft edges of the shadow pass; RENDER-73 street people and traffic cast none; RENDER-74 focus camera inputs |

## Platform and tools

| Feature | Status | Top open items |
|---|---|---|
| [executable-platform](executable-platform.md) | IN_PROGRESS | EXEC-3 update rate 30/60 undecided; EXEC-4 game frame function |
| [viewer-tools](viewer-tools.md) | IN_PROGRESS | TOOLS-1 oracle controller state 255 |

## Quality of life (`qol` plan)

| Feature | Status | Top open items |
|---|---|---|
| [qol-settings-menu](qol-settings-menu.md) | NOT_STARTED | QOLSET-1 toggle store and master (first); QOLSET-2 menu with tabs; QOLSET-7 `qol-separation` guard; QOLSET-8 PC Settings entry; QOLSET-9 mirrored options |
| [qol-settings-menu-hooks](qol-settings-menu-hooks.md) | IN_PROGRESS | The list of hook points that the `qol-separation` guard reads |
| [qol-settings-tabs](qol-settings-tabs.md) | IN_PROGRESS | QOLSET-3 Display tab; QOLSET-4 Controls tab |
| [qol-settings-drawing](qol-settings-drawing.md) | IN_PROGRESS | QOLSET-53 slot page bar; stores behind the open menu; stand-ins S4 and S6 |
| [qol-settings-ui](qol-settings-ui.md) | DONE | Modern PC settings menu: font, layout, painter, wiring and the picture behind the menu |
| [qol-settings-ui-pages](qol-settings-ui-pages.md) | DONE | Pages and dialogs of the modern menu (models done) |
| [qol-controls](qol-controls.md) | IN_PROGRESS | QOLCTL-3 vehicle layout; QOLCTL-7 seated bindings; QOLCTL-11 Quit game before the slot page |
| [qol-fullscreen](qol-fullscreen.md) | IN_PROGRESS | QOLFS-11 load qol.cfg into the active store; QOLFS-1 widened view (wired above the bottom screen); QOLFS-2 corner-anchored HUD; QOLFS-3 pop-in at the edges; QOLFS-20 load rooms only the wide view reaches |
| [qol-view-distance](qol-view-distance.md) | IN_PROGRESS | QOLVD-6 traffic distances (hook on the traffic step); QOLVD-8 extra rooms at the level start |
| [qol-minimap](qol-minimap.md) | IN_PROGRESS | QOLMAP-5 minimap and HUD overlap (question); QOLMAP-6 hide in more panels |
| [qol-bottom-screen](qol-bottom-screen.md) | IN_PROGRESS | QOLBOT-21 front-end pages; QOLBOT-2..4 panels; QOLBOT-5 pause PC menu; QOLBOT-7..10 scanner, build-it, shop, award viewer |

## Windows port (`windows-port` plan)

| Feature | Status | Top open items |
|---|---|---|
| [windows-build](windows-build.md) | IN_PROGRESS | WINBUILD-3 GitHub Windows job; WINBUILD-5 MSYS2 folder choice of the merge tool |
| [windows-platform](windows-platform.md) | IN_PROGRESS | WINPLAT-7 Python tools and non-ANSI paths; WINPLAT-8 Windows 10 before 1903; WINPLAT-5 non-ANSI save path |
| [windows-tests](windows-tests.md) | IN_PROGRESS | WINTEST-13 offscreen OpenGL tests; WINTEST-2 floating-point equality of the scene tests |
| [windows-hook-paths](windows-hook-paths.md) | IN_PROGRESS | WINTEST-6 MSYS root paths in hook commands |
| [windows-hooks](windows-hooks.md) | IN_PROGRESS | WINTEST-3 real Claude Code run on a clean Windows |
| [windows-line-endings](windows-line-endings.md) | IN_PROGRESS | WINTEST-11 camera CSVs stored with CRLF; WINTEST-12 CI check with autocrlf=true |
| [windows-python-tools](windows-python-tools.md) | IN_PROGRESS | WINTEST-10 Azahar oracle is Linux-only; WINTEST-17 guard against removing a worktree with a junction |
