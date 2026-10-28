# Feature index

One file per feature: scope, status, verified behaviour, stand-ins, open items (stable IDs), a findings summary that links into [docs/re/](../../re/README.md), and code and tests. Read only the section of this index for your area, then only the file for your feature, then only the docs/re entry it links. Rules: CLAUDE.md "Feature files". Work state (status, steps, log) is in the progress file of each plan ([plan index](../README.md)); features of a plan other than `linux-port` sit under that plan's own `## <Area>` headings. Limits: a feature file at most 16 KB (split a larger one as `<feature>-<subtopic>.md`, one row each). `python3 tools/check_doc_links.py --fix` moves closed items (`- [x]`) to `<feature>.closed.md`; closed files are not listed here. Item format: `- [ ] ID — gap. needs: ID. blocks: ID. uses: Name, Name. evidence: link`; `uses:` is optional and lists script natives or object classes of the level data that the item affects (it replaces the automatic match used by [priorities.md](../priorities.md), the generated ranking by reach in the main levels). `needs:` and `blocks:` are written on both sides. Stand-ins use `PREFIX-S<n>`. New feature: copy the section list of any file here, add it to this table. Section list: 1 Scope, 2 Status, 3 Verified behaviour, 4 Stand-ins, 5 Open items, 6 Findings summary, 7 Code and tests. Missions and enemies have no feature file yet (NOT_STARTED): mission flow needs SCRIPT-1; enemies and fighting are not recovered.

## Player and camera

| Feature | Status | Top open items |
|---|---|---|
| [player-movement](player-movement.md) | IN_PROGRESS | PLAYER-1 slide landing stun rule; PLAYER-2 slide exit; PLAYER-14 attack targets beyond props (attack on foot ported) |
| [player-traversal](player-traversal.md) | IN_PROGRESS | TRAV-34 pole A mount; TRAV-19 brick side jumps; TRAV-7 zipline rope; TRAV-2 climb wall (zipline and bricks ported) |
| [player-traversal-pole](player-traversal-pole.md) | IN_PROGRESS | TRAV-11 entry from the air; TRAV-13 walk-in touch; TRAV-15 sounds (pole ported) |
| [player-traversal-rails](player-traversal-rails.md) | IN_PROGRESS | TRAV-27 bar variants; TRAV-28 hang offset; TRAV-31 other corner direction; TRAV-42..45 acrobat pole leftovers (rails and flag poles ported) |
| [player-traversal-ladders](player-traversal-ladders.md) | IN_PROGRESS | TRAV-58 entry from the air, furled ladders; TRAV-59 exit collision; TRAV-60 clip restart, hidden weapons, camera; TRAV-61 oracle check of the sounds (ladders ported) |
| [camera](camera.md) | IN_PROGRESS | CAMERA-55 police car after PositionObject at the hand-back; CAMERA-3 multi-record weights; CAMERA-2 characters and vehicles not registered |
| [collision](collision.md) | IN_PROGRESS | COLL-1 broad phase stand-in; COLL-2 constructors missing (about 40 kinds, characters); COLL-23 transcribe them; COLL-24 nested start state |
| [animation-characters](animation-characters.md) | IN_PROGRESS | ANIM-3 blending; ANIM-10 slot bookkeeping; ANIM-11 model folders |
| [input](input.md) | DONE (lab, mapping) | INPUT-2 readers of input slots 16/17 |
| [abilities-detective](abilities-detective.md) | IN_PROGRESS | DETECT-2 first point never enabled (Doorlock script waits); DETECT-1 use-state approach; DETECT-4 particles, fades, flight |
| [abilities-scanner](abilities-scanner.md) | IN_PROGRESS | SCAN-2 other zone modes; SCAN-10 sonar segments; SCAN-19 tip pages during the scanner view; SCAN-16 and SCAN-17 info box comparison and icon writer |

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
| [vehicles-physics-collision](vehicles-physics-collision.md) | IN_PROGRESS | VEH-102 sphere contact at a wall; VEH-70 Bullet contact details; VEH-91 vehicle activation |
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
| [script-system-use-markers](script-system-use-markers.md) | IN_PROGRESS | SCRIPT-299 markers of the other 34 kinds; SCRIPT-355 special cases and HeadMarker; SCRIPT-356 list builder; SCRIPT-357 placement ray |
| [script-system-natives](script-system-natives.md) | IN_PROGRESS | SCRIPT-7 natives in batches SCRIPT-29 to SCRIPT-39, SCRIPT-57 to SCRIPT-65 |
| [script-system-natives-world](script-system-natives-world.md) | IN_PROGRESS | SCRIPT-279 party swap; SCRIPT-280 destroy message 1 per class; SCRIPT-281 effect fade rest; SCRIPT-282 distance fade gate; SCRIPT-283 AddToSemiTransList |
| [script-system-natives-world-death](script-system-natives-world-death.md) | IN_PROGRESS | SCRIPT-326 a character that comes back; SCRIPT-327 the player's branch; SCRIPT-328 AI helpers; SCRIPT-323 sound table, clock, textures |
| [script-system-natives-animation](script-system-natives-animation.md) | IN_PROGRESS | SCRIPT-270 player animations; SCRIPT-272 attach node changes; SCRIPT-273 / 274 states, loaded set |
| [script-system-natives-party](script-system-natives-party.md) | IN_PROGRESS | SCRIPT-338 character records read by nothing; SCRIPT-339 boss components; SCRIPT-340 mech piloting |
| [script-system-natives-state](script-system-natives-state.md) | IN_PROGRESS | SCRIPT-56 / 59 / 335 / 336 readers of placement and flags; SCRIPT-57 kind gates; SCRIPT-61 / 62 / 64 / 65 progress natives |
| [script-system-ai-navigation](script-system-ai-navigation.md) | IN_PROGRESS | SCRIPT-79 AI movement; SCRIPT-94 active nav grid set; SCRIPT-103 AISuperJump flight, SCRIPT-104 pursuit / stalk / run-away states |
| [script-system-dcam](script-system-dcam.md) | IN_PROGRESS | SCRIPT-46 director camera commands, SCRIPT-48 / 49 / 50 / 181 / 182 details |
| [script-system-hud-natives](script-system-hud-natives.md) | IN_PROGRESS | SCRIPT-66 to SCRIPT-70 text screens; SCRIPT-73 map and scanner natives; SCRIPT-329 to SCRIPT-331 arrow hide, oracle check, enemy and carry groups; SCRIPT-354 vehicle waypoint steering |

## World and levels

| Feature | Status | Top open items |
|---|---|---|
| [world-streaming](world-streaming.md) | IN_PROGRESS | `chase-game` streams rooms (street level); WSTREAM-12 room position code; WSTREAM-30 loader cost; WSTREAM-4 visibility inside rooms |
| [pedestrians-traffic](pedestrians-traffic.md) | IN_PROGRESS | PREGEN-3 traffic floor cell; derived path, tangent and room ported; pedestrian file used in chase-game |
| [pedestrians-traffic-pedestrians](pedestrians-traffic-pedestrians.md) | IN_PROGRESS | run in chase-game with room moves; PED-5 message slot; PED-27 fade point cameras |
| [pedestrians-traffic-pedestrians-fade](pedestrians-traffic-pedestrians-fade.md) | IN_PROGRESS | PED-21 faded-out pedestrians not removed; PED-22 node radius source; PED-23 node draw pass |
| [pedestrians-traffic-pedestrians-vehicles](pedestrians-traffic-pedestrians-vehicles.md) | IN_PROGRESS | vehicle test and set-up layer table run in chase-game; PED-18 cutscene layer table; PED-19 vehicle list filter |
| [pedestrians-traffic-cars](pedestrians-traffic-cars.md) | IN_PROGRESS | cars spawn and leave in chase-game, standing still; TRAFFIC-2 connect the AI driver; TRAFFIC-7 route mover; TRAFFIC-10 route numbers; TRAFFIC-12 other levels |
| [level-transitions](level-transitions.md) | IN_PROGRESS | station exit and entry switch at run time; TRANS-3 master level (needs SCRIPT-1), TRANS-8 loading page code and look; TRANS-15 same-level entry timing, TRANS-16 oracle check of lift entries |

## Front end, HUD, saves

| Feature | Status | Top open items |
|---|---|---|
| [front-end](front-end.md) | IN_PROGRESS | FE-1 existing slots and Continue (needs SAVE-2); FE-30 delete confirmation; FE-3 loading screen 3D badge |
| [hud-ui](hud-ui.md) | IN_PROGRESS | HUD-6 hearts never change; HUD-11 map levels and mission icons, HUD-60 other map icon types; HUD-8 other counters |
| [hud-ui-text](hud-ui-text.md) | IN_PROGRESS | HUD-22 remaining text screens; HUD-30 character intro and box line counts; HUD-33 tip after the release camera |
| [ui-menus](ui-menus.md) | IN_PROGRESS | UI-20 characters panel; UI-21 map button actions; UI-2 pause fades and gates; UI-3 pause Exit; UI-23 Languages page; UI-24 red brick extras page; UI-25 options save task (pause screen, its Options page and map buttons ported) |
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
| [qol-fullscreen](qol-fullscreen.md) | NOT_STARTED | QOLFS-1 widened full-window view; QOLFS-2 corner-anchored HUD; QOLFS-3 pop-in at the edges |
| [qol-minimap](qol-minimap.md) | NOT_STARTED | QOLMAP-1 round top-right minimap |
| [qol-bottom-screen](qol-bottom-screen.md) | NOT_STARTED | QOLBOT-1 hide the bottom screen; QOLBOT-2..4 panels; QOLBOT-5 pause PC menu; QOLBOT-7..10 scanner, build-it, shop, award viewer |
