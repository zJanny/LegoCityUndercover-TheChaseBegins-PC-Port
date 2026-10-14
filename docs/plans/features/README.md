# Feature index

One file per feature: scope, status, verified behaviour, stand-ins, open items (stable IDs), a findings summary that links into [docs/re/](../../re/README.md), and code and tests. Read this index, then only the file for your feature, then only the docs/re entry it links. Rules: CLAUDE.md "Feature files". Work state (status, steps, log) is in [linux-port.progress.md](../linux-port.progress.md). Targets: this index at most about 40 lines, a feature file at most about 150.

Item format: `- [ ] ID — gap. needs: ID. blocks: ID. evidence: link`. `needs:` and `blocks:` are written on both sides. Stand-ins use `PREFIX-S<n>`.

| Feature | Status | Top open items |
|---|---|---|
| [player-movement](player-movement.md) | IN_PROGRESS | PLAYER-1 slide landing stun rule; PLAYER-6 other 0x203d18 states |
| [camera](camera.md) | IN_PROGRESS | CAMERA-9 follow controller seed and jump reaction in a fade; CAMERA-3 multi-record weights, Blocker pose, shake |
| [collision](collision.md) | IN_PROGRESS | COLL-1 broad phase stand-in; COLL-2 constructors missing (vehicles, characters) |
| [level-objects](level-objects.md) | IN_PROGRESS | LEVOBJ-3 objects frozen (needs SCRIPT-1); LEVOBJ-2 NPC start animation; LEVOBJ-12/13 hidden characters, per-class visibility |
| [script-system](script-system.md) | IN_PROGRESS | SCRIPT-1 mission flow (needs SCRIPT-7 natives in batches SCRIPT-29 to SCRIPT-39, SCRIPT-46 and SCRIPT-57 to SCRIPT-65, SCRIPT-11 trigger types) |
| [world-streaming](world-streaming.md) | IN_PROGRESS | `chase-game` streams rooms (street level); WSTREAM-12 room position code; WSTREAM-13 room change trigger and load time; WSTREAM-4 visibility inside rooms |
| [level-transitions](level-transitions.md) | IN_PROGRESS | station exit and entry switch at run time; TRANS-3 master level (needs SCRIPT-1), TRANS-8 loading page code and look, TRANS-6 lifts |
| [doors](doors.md) | IN_PROGRESS | DOOR-1 script-requested doors stay closed (needs SCRIPT-7); DOOR-3 open-door box |
| [studs](studs.md) | IN_PROGRESS | STUD-1 magnet and multiplier; STUD-4 groups (needs SCRIPT-1) |
| [front-end](front-end.md) | IN_PROGRESS | FE-1 existing slots and Continue (needs SAVE-2, FE-11); FE-3 loading screen 3D badge |
| [hud-ui](hud-ui.md) | IN_PROGRESS | HUD-6 hearts never change; HUD-11 map code and icons; HUD-8 other counters; HUD-13 text screens |
| [audio](audio.md) | IN_PROGRESS | AUDIO-2 level music (needs SCRIPT-1); AUDIO-3 world sounds |
| [movies-cutscenes](movies-cutscenes.md) | IN_PROGRESS | MOVIE-2 cutscenes not in game flow (blocks FE-4) |
| [animation-characters](animation-characters.md) | IN_PROGRESS | ANIM-2 NPC animation playback (blocks LEVOBJ-2) |
| [particles](particles.md) | IN_PROGRESS | PART-1 no particle rendering |
| [saves-options](saves-options.md) | IN_PROGRESS | SAVE-13 Continue at level 2 and later, SAVE-8 object record meaning (needs a richer save), SAVE-2 slot display (blocks FE-1) |
| [input](input.md) | DONE (lab, mapping) | INPUT-2 readers of input slots 16/17 |
| [executable-platform](executable-platform.md) | IN_PROGRESS | EXEC-3 update rate 30/60 undecided; EXEC-4 game frame function |
| [assets-formats](assets-formats.md) | IN_PROGRESS | ASSET-2 streaming |
| [rendering](rendering.md) | IN_PROGRESS | RENDER-1 fragment lighting; RENDER-2 reflections; RENDER-3 fog |
| [viewer-tools](viewer-tools.md) | IN_PROGRESS | TOOLS-1 oracle controller state 255 |
| missions (no file yet) | NOT_STARTED | mission flow, needs SCRIPT-1 |
| enemies (no file yet) | NOT_STARTED | enemies and fighting, not recovered |

New feature: copy the section list of any file here, add it to this table. Section list: 1 Scope, 2 Status, 3 Verified behaviour, 4 Stand-ins, 5 Open items, 6 Findings summary, 7 Code and tests.
