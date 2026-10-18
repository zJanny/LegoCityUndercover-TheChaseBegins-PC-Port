# Feature index

One file per feature: scope, status, verified behaviour, stand-ins, open items (stable IDs), a findings summary that links into [docs/re/](../../re/README.md), and code and tests. Read only the section of this index for your area, then only the file for your feature, then only the docs/re entry it links. Rules: CLAUDE.md "Feature files". Work state (status, steps, log) is in [linux-port.progress.md](../linux-port.progress.md). Limits: a feature file at most 16 KB (split a larger one as `<feature>-<subtopic>.md`, one row each). `python3 tools/check_doc_links.py --fix` moves closed items (`- [x]`) to `<feature>.closed.md`; closed files are not listed here. Item format: `- [ ] ID — gap. needs: ID. blocks: ID. evidence: link`. `needs:` and `blocks:` are written on both sides. Stand-ins use `PREFIX-S<n>`. New feature: copy the section list of any file here, add it to this table. Section list: 1 Scope, 2 Status, 3 Verified behaviour, 4 Stand-ins, 5 Open items, 6 Findings summary, 7 Code and tests. Missions and enemies have no feature file yet (NOT_STARTED): mission flow needs SCRIPT-1; enemies and fighting are not recovered.

## Player and camera

| Feature | Status | Top open items |
|---|---|---|
| [player-movement](player-movement.md) | IN_PROGRESS | PLAYER-1 slide landing stun rule; PLAYER-2 slide exit; PLAYER-6 other 0x203d18 states |
| [player-traversal](player-traversal.md) | IN_PROGRESS | TRAV-1 pole climb; TRAV-2 climb wall; TRAV-3 rails and climb bars; TRAV-19 brick side jumps (zipline and bricks ported) |
| [camera](camera.md) | IN_PROGRESS | CAMERA-51 parked car after the hand-back; CAMERA-3 multi-record weights, Blocker pose; CAMERA-2 characters and vehicles not registered |
| [collision](collision.md) | IN_PROGRESS | COLL-1 broad phase stand-in; COLL-2 constructors missing (vehicles, characters) |
| [animation-characters](animation-characters.md) | IN_PROGRESS | ANIM-3 blending; ANIM-10 slot bookkeeping; ANIM-11 model folders |
| [input](input.md) | DONE (lab, mapping) | INPUT-2 readers of input slots 16/17 |
| [abilities-detective](abilities-detective.md) | IN_PROGRESS | DETECT-2 first point never enabled (Doorlock script waits); DETECT-1 use-state approach; DETECT-4 particles, fades, flight |
| [abilities-scanner](abilities-scanner.md) | IN_PROGRESS | SCAN-1 view look; SCAN-2 other zone modes; SCAN-4 real-time timing (Azahar NaN yaw) |

## Level objects

| Feature | Status | Top open items |
|---|---|---|
| [level-objects](level-objects.md) | IN_PROGRESS | LEVOBJ-3 objects frozen (needs SCRIPT-1); LEVOBJ-16 other NPC states; LEVOBJ-12/13 hidden characters, per-class visibility |
| [level-objects-buildit](level-objects-buildit.md) | IN_PROGRESS | LEVOBJ-76 studs of the build; LEVOBJ-77 two-target camera; LEVOBJ-43 built state not saved (needs SAVE-8) |
| [level-objects-carry](level-objects-carry.md) | IN_PROGRESS | LEVOBJ-75 release of the box; LEVOBJ-30 attach update; LEVOBJ-31 locomotion while carrying; LEVOBJ-32 sounds and effects |
| [doors](doors.md) | IN_PROGRESS | DOOR-1 script-requested doors stay closed (needs SCRIPT-7); DOOR-3 open-door box |
| [studs](studs.md) | IN_PROGRESS | STUD-1 magnet and multiplier; STUD-4 groups (needs SCRIPT-1) |
| [particles](particles.md) | IN_PROGRESS | PART-2 script natives and most callers start no emitters; PART-3 `particles` level object; PART-5 update trace |

## Vehicles

| Feature | Status | Top open items |
|---|---|---|
| [vehicles](vehicles.md) | IN_PROGRESS | VEH-7 driving collision; VEH-21 driving details; VEH-10 sounds |
| [vehicles-damage](vehicles-damage.md) | IN_PROGRESS | VEH-123 super bricks; VEH-125 debris line tests; VEH-121 rest of the break effects; VEH-120 rest of the return |
| [vehicles-physics](vehicles-physics.md) | IN_PROGRESS | VEH-42 inertia; VEH-50 air control; VEH-54 jump-out braking |
| [vehicles-physics-collision](vehicles-physics-collision.md) | IN_PROGRESS | VEH-102 sphere contact at a wall; VEH-70 Bullet contact details; VEH-91 vehicle activation |
| [vehicles-boarding](vehicles-boarding.md) | IN_PROGRESS | VEH-19 boarding gates and exit probes; VEH-20 boarding state reactions; VEH-80 head icons |

## Scripts and missions

| Feature | Status | Top open items |
|---|---|---|
| [script-system](script-system.md) | IN_PROGRESS | SCRIPT-1 mission flow (needs SCRIPT-7 natives, SCRIPT-75 / 76 trigger events); runner, scene start, class startups |
| [script-system-triggers](script-system-triggers.md) | IN_PROGRESS | SCRIPT-75 / 76 AI and vehicle trigger events; SCRIPT-77 / 78 object bound checks |
| [script-system-missions](script-system-missions.md) | IN_PROGRESS | SCRIPT-111 cage use spot and door lock hint; SCRIPT-113 unnamed mission scripts; SCRIPT-190 carry gaps |
| [script-system-natives](script-system-natives.md) | IN_PROGRESS | SCRIPT-7 natives in batches SCRIPT-29 to SCRIPT-39, SCRIPT-57 to SCRIPT-65 |
| [script-system-natives-world](script-system-natives-world.md) | IN_PROGRESS | SCRIPT-279 party swap; SCRIPT-280 destroy message 1 per class; SCRIPT-281 effect fade rest; SCRIPT-282 distance fade gate; SCRIPT-283 AddToSemiTransList |
| [script-system-natives-animation](script-system-natives-animation.md) | IN_PROGRESS | SCRIPT-270 player animations; SCRIPT-272 attach node changes; SCRIPT-273 / 274 states, loaded set |
| [script-system-ai-navigation](script-system-ai-navigation.md) | IN_PROGRESS | SCRIPT-79 AI movement; SCRIPT-94 active nav grid set; SCRIPT-103 AISuperJump flight, SCRIPT-104 pursuit / stalk / run-away states |
| [script-system-dcam](script-system-dcam.md) | IN_PROGRESS | SCRIPT-46 director camera commands, SCRIPT-48 / 49 / 50 / 181 / 182 details |
| [script-system-hud-natives](script-system-hud-natives.md) | IN_PROGRESS | SCRIPT-66 to SCRIPT-70 text screens; SCRIPT-71 to SCRIPT-74 objective markers, arrows, map |

## World and levels

| Feature | Status | Top open items |
|---|---|---|
| [world-streaming](world-streaming.md) | IN_PROGRESS | `chase-game` streams rooms (street level); WSTREAM-12 room position code; WSTREAM-30 loader cost; WSTREAM-4 visibility inside rooms |
| [level-transitions](level-transitions.md) | IN_PROGRESS | station exit and entry switch at run time; TRANS-3 master level (needs SCRIPT-1), TRANS-8 loading page code and look, TRANS-6 lifts |

## Front end, HUD, saves

| Feature | Status | Top open items |
|---|---|---|
| [front-end](front-end.md) | IN_PROGRESS | FE-1 existing slots and Continue (needs SAVE-2); FE-30 delete confirmation; FE-3 loading screen 3D badge |
| [hud-ui](hud-ui.md) | IN_PROGRESS | HUD-6 hearts never change; HUD-11 map levels and mission icons, HUD-60 other map icon types; HUD-8 other counters |
| [audio](audio.md) | IN_PROGRESS | AUDIO-20 duck record; AUDIO-11 action music; AUDIO-3 world sounds |
| [movies-cutscenes](movies-cutscenes.md) | IN_PROGRESS | MOVIE-33 camera of 1_1_d; MOVIE-3 short actor clips; MOVIE-1 layout values |
| [saves-options](saves-options.md) | IN_PROGRESS | SAVE-13 Continue at level 2 and later, SAVE-8 object record meaning (needs a richer save), SAVE-2 slot display (blocks FE-1) |

## Rendering and assets

| Feature | Status | Top open items |
|---|---|---|
| [assets-formats](assets-formats.md) | IN_PROGRESS | ASSET-2 streaming |
| [rendering](rendering.md) | IN_PROGRESS | RENDER-33 shadows; RENDER-66 distance fade; RENDER-2 reflections; RENDER-30 fog oracle comparison |

## Platform and tools

| Feature | Status | Top open items |
|---|---|---|
| [executable-platform](executable-platform.md) | IN_PROGRESS | EXEC-3 update rate 30/60 undecided; EXEC-4 game frame function |
| [viewer-tools](viewer-tools.md) | IN_PROGRESS | TOOLS-1 oracle controller state 255 |
