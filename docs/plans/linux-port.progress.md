# Progress — Linux x86_64 accessible port

## Status

- Overall: IN_PROGRESS
- Current Phase: IMPLEMENT
- Current Step: S4e runtime core — M4 reached; police-station exit and entry at run time, save-aware start and camera blend merged (round H/I/J/K)
- Last Updated: 2026-10-14T12:16:46+02:00
- Branch: main
- Target Branch: Not specified
- Requested Scope: PLAN, IMPLEMENT, TEST; start reverse engineering and native port
- Plan: docs/plans/linux-port.md
- Plan Revision: 4
- Planning Baseline: no Git/source; supplied CCI only

## Summary

The executable and the full RomFS are extracted with hash verification. The FUSE asset archive, textures, sounds and model geometry decode, and each decoder has been checked against every shipped file. Levels, nav grids, skeletons, animation, cutscenes, localisation, fonts and data tables also decode in C++, and the development viewer shows levels and cutscenes. The game is not playable on PC yet: gameplay logic, rendering states, UI, scripts and saves are not recovered.

## Where things live

- This file: work state only (status, steps, decisions, blockers, next action, one log line per session).
- [features/README.md](features/README.md): one file per feature with status, open items, stand-ins and a findings summary. Look here for known gaps.
- [docs/re/README.md](../re/README.md): evidence and details (addresses, layouts, evidence IDs, unknowns).

## Phase Status

| Phase | Status | Started | Completed |
|---|---|---|---|
| PLAN | COMPLETE | 2026-10-12T14:43:19.863039+02:00 | 2026-10-12T14:43:19.863039+02:00 |
| IMPLEMENT | IN_PROGRESS | 2026-10-12T14:43:19.863039+02:00 | - |
| TEST | IN_PROGRESS | 2026-10-12T14:51:33+02:00 | - |
| COMMIT | IN_PROGRESS | 2026-10-12T15:02:00+02:00 | - |
| MERGE | SKIPPED | - | - |

Commits authorized by CLAUDE.md "User specified workflows" (commit at logical steps; push only on request). Merge not requested.

## Implementation Progress

| Step | Description | Status | Notes |
|---|---|---|---|
| S1 | Verified executable extraction and analysis ELF | COMPLETE | R1; unit tests + real extraction 2026-10-12T14:51 |
| S2 | Native accessible input laboratory | COMPLETE | R2/R3; warning-free build, CTest 2/2 |
| S3 | Focused ARM analysis | IN_PROGRESS | R1; startup, SVC stubs, service-handle helper, services, paths recorded; game frame function still open |
| S4 | Recover gameplay and replace platform | IN_PROGRESS | R4; split into S4a–S4f (plan rev 3) |
| S4a | Native asset runtime (C++) | COMPLETE | FUSE, pooled, textures, models, levels, sounds; real-data totals equal Python |
| S4b | Level viewer (SDL2 + GL) | COMPLETE | chase-viewer; offscreen smoke test; placement awaits S4c |
| S4c | Level composition (.blvl) | IN_PROGRESS | loader replicated (Py+C++), placements in viewer; materials/lighting, streaming, other sections open |
| S4d | Audio playback | IN_PROGRESS | mixer + SDL sound test with loops; sound ids (tablessoundfx) decoded, play by id; script routing partly known |
| S4e | Gameplay | IN_PROGRESS | chase_runtime replicas checked against the oracle: scheduler, pad pipeline, stick intent, locomotion, follow camera, player collision; chase-game runs them on lvl_00; scripts/AI/objects/UI not recovered |
| S4f | Saves/settings | IN_PROGRESS | chase_save: header, CRC, validate, atomic XDG storage, option fields, slot progress accessors, completion (0x1cd3dc, no cheat flags) |

## Current Work

### Current Step

Introducing the feature layer (docs/plans/features/): open items, stand-ins and findings moved out of this file. Resume from Next Action.

### What Is Being Done

Nothing else in progress.

### Files Being Modified

None.

### Expected Result

Next session picks up from Next Action.

## Completed Work

- [x] Target identity and decrypted-content probes
- [x] S1
- [x] S2
- [ ] S3
- [ ] S4

## Tests

Latest recorded verification (2026-10-13T21:17+02:00, details in the matching feature files): `cmake --build build` warning-free, `ctest --test-dir build` 38/38, `python3 -m unittest discover -s tests -p 'test_*.py'` 63 OK. S1: real extraction matched the manifest hashes (image sha256 078faf7e…59ad, `.code` sha256 f895faae…6923, ELF sha256 5152ed14…0881; [executable-platform](features/executable-platform.md)).

Failing: REA `open_binary` rejects the CCI (unsupported format); use the derived ELF. Not run: the original game outside the Azahar oracle.

## Git

Branch: main (`feature/linux-port` was the historical start branch). Early commits: `4642cf5` extraction tool (S1), `db355f9` input laboratory (S2), `a805bf3` plan, progress log and CLAUDE.md. Later commits: see `git log` (one commit per session-log entry).

## Decisions

### Decision: goal and scope (2026-10-13T17:01:07+02:00)

**Question:** what does the port have to be, and in which order. **Decision (user):** a faithful PC port that works and behaves exactly like the original 3DS game, playable start to finish, on Linux first; Windows only after the Linux version is finished and the user says so (no Windows work until then); quality-of-life features only after the game is finished for PC (no QoL and no deviations until then). Keyboard/mouse input mapping, including the `Walk` stand-in key, is part of the port, not QoL. Stand-ins are labelled and listed as open items. **Reason:** user instruction; recorded in CLAUDE.md "Goal and faithfulness" and "Platform".

### Decision: architecture and runtime oracle (2026-10-13T10:53+02:00)

**Question:** how to reach a playable game: execute the ARM code (emulation/recompilation) or reconstruct natively. **Decision:** native reconstruction in C++ (chase_runtime), with the installed Azahar emulator used only as a local oracle (tools/azahar_oracle.py) to measure the original game. **Reason:** executing the ARM code would rebuild Azahar; the oracle turns "unknown at run time" values into measurements and gives a reference for differential tests. The user's own image is only read; nothing game-derived is committed.

User approved Git init, no extra guidance, Linux devices emit keyboard/mouse. Use C++17/SDL2 and preserve ARM code as analysis artifact. Native lab is a foundation, not game runtime.

### Decision: commit authorization (2026-10-12T15:02+02:00)

**Question:** Plan section 16 left commits out of scope. **Decision:** User added to CLAUDE.md: commit at logical steps; push only when asked. **Reason:** Supersedes the no-commit instruction; merge/push remain unauthorized.

### Decision: feature layer between progress file and docs/re (2026-10-13T21:34+02:00)

**Question:** where do known gaps live so that an agent finds them without reading many files. **Decision (user):** one combined file per feature in `docs/plans/features/` (index `features/README.md`, about 18 fine-grained files, stable item IDs, `needs:`/`blocks:` between features); this file keeps work state only with one log line per session; docs/re stays the evidence layer; long old log entries shortened to one line. Plan revision 4. **Reason:** the Next Action section had grown into a mix of status, open items, stand-ins and findings. Rule in CLAUDE.md "Feature files".

### Decision: the user makes hard-to-reach oracle states (2026-10-14T10:33:57+02:00)

**Question:** how agents get to game points that need playing (missions, routes) in the oracle. **Decision (user):** an agent that would have to play there, or whose scripted attempts fail, asks the user through the lead with everything needed to play to that point; the user saves the state in their own Azahar and the lead copies it into `artifacts/oracle/states/`. **Reason:** an agent brute-forcing a route with scripted input costs hours and tokens; the user plays it in minutes (example: `street-after-1-1`). Rule in CLAUDE.md "Runtime oracle".

## Blockers

Full game logic, hardware APIs and asset formats not recovered yet.

## Deviations From Plan

- 2026-10-12: Plan section 16 said no commits. Actual: commits made per user's CLAUDE.md workflow rule. Plan revision 2 records this; no code steps reopened.

## Next Action

Read this section, CLAUDE.md (incl. "Goal and faithfulness" and "Feature files"), [features/README.md](features/README.md) and, before any emulator use, docs/re/oracle.md. Open items, stand-ins and findings are in the feature files, not here. Feature layer introduced 2026-10-13T21:34:59+02:00; earlier handoff 2026-10-13T17:53:20+02:00.

**Goal (user, 2026-10-13):** faithful PC port, playable start to finish, Linux first; Windows only when the user says so; QoL only after the PC version is finished (Decisions, CLAUDE.md).

**State:** M1 runtime loop, M2 locomotion, M3 walkable level, M4 level objects, characters, studs, falling, jumping and object collision are on main; the front end (boot to the first level) and the sound hooks are merged. `chase-game` is a development runtime, not the game ([docs/status.md](../status.md)).

**Goal of the current rounds (user, 2026-10-14):** mission 1-1 playable from the new-game start to `1-1_MissionEnd`; its steps and blockers: [objective-hud.md §2026-10-14T21:23](../re/objective-hud.md#ts-20261014-212357).

**Priorities (critical path), updated 2026-10-15T14:15:08+02:00:** mission 1-1 runs end to end in `chase-game` with dev teleports between the stops (CTest `game-mission-1-1`). Done for mission 1-1: waypoint class with vehicle waypoints, in-engine cutscenes with the level-start intro and fades, cutscene actors, build-it with its trigger path, carrying with the code-side trigger path, vehicles created and solid, getting into and out of vehicles (also by script), the vehicle body on the original's gravity, damping and suspension (speed and steering still the stand-in), the vehicle camera, the director camera holding the player, vehicle PositionObject, NPC idle animation. Oracle states before every step: `m11-1` to `m11-9` (docs/re/oracle-states.md).
1. Real play instead of teleports (SCRIPT-200): engine, gears and tyres (VEH-32), the collision pass and solver (VEH-40), then driving collision (VEH-7): [vehicles-physics](features/vehicles-physics.md), [vehicles](features/vehicles.md).
2. Camera after a director camera: the reset's obstruction branch (CAMERA-10, CAMERA-20, then CAMERA-15); mission 1-1 natives without handler (SCRIPT-201: EnableTrafficAndPedestrians, music SCRIPT-33); readers of camera manager +0x28 (CAMERA-22).
3. Trigger origins: carry gaps (SCRIPT-190), class-fired triggers (SCRIPT-111), build-it reference gaps (SCRIPT-220, SCRIPT-221); `SetPartyCharacter` (SCRIPT-35); the director camera's other readers (SCRIPT-181, SCRIPT-182).
4. Polish of the finished steps: carry and build-it stand-ins (level-objects-carry.md, level-objects-buildit.md), the police car start state (VEH-13), the enter prompt and vehicle HUD (VEH-17, VEH-18), wheel node pose (VEH-43), animation blending (ANIM-3), COLL-21, the bottom-screen panel during cutscenes (MOVIE-24); small file readers (ASSET-5, ASSET-6).
5. After mission 1-1: HUD fidelity, saves (FE-30, SAVE-14, SAVE-15), level switch screens (TRANS-13, TRANS-14), streaming (WSTREAM-30, TRANS-3), camera and rendering (CAMERA-10, RENDER-1, RENDER-9).

Oracle states (artifacts/oracle/states, see docs/re/oracle.md): `gameplay-start` (new game, Old 3DS), `police-station` and `street-after-1-1` (user saves after mission 1-1, New 3DS), `street-at-entrance`, `station-at-exit`, `station-exit-loadingpage`, `camera-lobby-walk`, `dcam-police-dialog`, `agent-doughnut-title` (agents, unmodified, New 3DS), `savecont-slotlist-user` (slot list with the user's save, Old 3DS), `trans-poked-*` (poked, measuring aids only). The real save of the user (read-only copy): `artifacts/oracle/agentJ-saves/savegame.dat`. Hard-to-reach states: ask the user (CLAUDE.md "Runtime oracle").

**Workflow notes:** oracle route and states: `tools/azahar_oracle.py` with `--display xvfb --state gameplay-start --renderer opengl --rpc --rpc-input`, route from boot `tools/oracle/to-gameplay.txt`; only one agent may use `--rpc` at a time. The lead delegates to subagents in worktrees (at most 3 in parallel, user), every brief names the CLAUDE.md rules, reviews each result against CLAUDE.md, rebuilds and tests, squash-merges one commit per branch with `tools/merge_agent.py prepare|check|finish` (it removes the worktree and branch), pushes when authorized, and checks the Graphify rebuild. Agents keep scratch files in their own subdirectory of the shared scratchpad. Worktree agents link `artifacts/` into their worktree (CLAUDE.md "Subagents"). Resume a branch with `git log main..<branch>`.

## Session Log

The newest 20 lines only; `python3 tools/check_doc_links.py --fix` moves older lines unchanged to [linux-port.progress-archive.md](linux-port.progress-archive.md). One line per session: timestamp, what was worked on, how far it got, link to the feature file. Findings, open items and stand-ins do not belong here (CLAUDE.md "Feature files"). Reverse-engineering details are in docs/re/ (see docs/re/README.md).

- 2026-10-15T08:00:00+02:00 — Agent A (enable-collision): EnableObject / EnableCollision during play now follow in collision and doors (grid keeps every candidate, owner flags decide per update; original read at 0x27e9f4, 0x2ba3f0, 0x2428c0); the police car of mission 1-1 blocks the player, oracle walk agrees roughly; COLL-20 and SCRIPT-160 closed, COLL-21 open → [collision](features/collision.md)
- 2026-10-15T08:30:00+02:00 — Agent C (vehicle-enter): boarding read from the code and measured in the oracle (X edge, search 2.0 m on x and z to the collision item box, jump in / out 2200 ticks = line plus 2.1 m arc, hidden player at node + CentreOfMass, eight exit candidates, PlayerVehicleEntered / Exiting, record states 5 / 3); ported in `src/runtime/vehicle_boarding.*`, tests vehicle-boarding(-real-data); VEH-4 closed, VEH-17..20 and VEH-S8..S10 open → [vehicles](features/vehicles.md)
- 2026-10-15T09:29:14+02:00 — Agent C (npc-anim): level characters now play the idle state (idle/idle2 once each at the stored rate, random bit 0x25b2b8 per start; slot, state 0x1d5070 and RNG read from the code and measured in the oracle m11-1-donut-shop by memory reads only); other NPC states, blend and the original picture open → [animation-characters](features/animation-characters.md)
- 2026-10-15T09:36:00+02:00 — Agent A (vehicle-drive): driving works in chase-game on a kinematic stand-in body (VEH-S3): player input 0x29cd9c, state rules 0x23019c, writers of states 5 and 3 (vehicle messages 84 and 85), forced stop, stuck rule 0x16ce9c read; speed, brake, reverse, coasting, hand brake and steering tables measured on the police trooper in the oracle and pinned in `vehicle-drive`; `game-vehicle-drive` enters, drives and jumps out; VEH-5 closed, VEH-21 opened → [vehicles](features/vehicles.md)
- 2026-10-15T09:39:09+02:00 — Agent B (dcam-hold): an active director camera holds the player (no stick, no buttons) and every character without DCamActive (no AI move) like the original: the character update 0x1835d0 picks 0x29bf30 while `*0x37689c` is the director camera; oracle m11-2-patrol-car (X ignored from the update after StartDCam to the update after the end test, 274 updates; ctrace of the call order), `game-director-hold` / `-release`, `--set-variable`; SCRIPT-47 closed, SCRIPT-181/182 open → [script-system-dcam](features/script-system-dcam.md)
- 2026-10-15T10:51:18+02:00 — Agent C (compare-shots): original-vs-port screenshots for COLL-21 (car view: port frame shows no car, COLL-22 added), MOVIE-21 (cutscene 1_1_b: different moments, MOVIE-23 added) and ANIM-6 (donut shop view: same place, ANIM-6 closed; bottom-screen cutscene panel MOVIE-24 added by the lead); side-by-sides in artifacts/oracle/agentSHOT-out → [collision](features/collision.md)
- 2026-10-15T11:22:24+02:00 — Agent D (trigger-paths): carry-fired ObjectTriggered now follows 0x244f40 (enable, message 0xff, event; TriggerPickup, TriggerObject, TriggerOnRespawn); type 21 build-it never calls it, origin of the VanFixed trigger stays open as a stand-in; details in [script-system-missions](features/script-system-missions.md).
- 2026-10-15T11:40:00+02:00 — Agent A (vehicle-waypoints): vehicle waypoints (reach test with the player's vehicle incl. jump in/out, named vehicle / VehicleType, box second test, hint, forced stop + 12 m/s clamp, release) and VehicleJumpOut / JumpIn / StopPlayerVehicle / VehicleSetTopSpeed, read from the code and measured with GDB at m11-4/m11-6; mission 1-1 steps 7 and 8 run in chase-game (enter van, drive to waypoint 05, script jumps out), VEH-9 and VEH-11 closed → [vehicles](features/vehicles.md)
- 2026-10-15T12:09:48+02:00 — Agent G (small-formats): small pregen and script-list files: nothing decoded; the lead located the code sites that use their paths (G's scan had missed them), ASSET-5 to ASSET-7 opened, [feature file](features/assets-formats.md), [data-tables-config.md](../re/data-tables-config.md).
- 2026-10-15T12:21:44+02:00 — Agent F (vehicle-body): VEH-6 part 1: rigid body layout, world step 0x30c768, gravity -39 and the speed-dependent damping measured and transcribed (`vehicle_body.*`); vehicles fall under gravity and settle on the probe ground (stand-in VEH-S20), VEH-S13 reduced to a lookup range; wheels/suspension, orientation and engine remain as VEH-30..33; see [vehicles.md](features/vehicles.md).
- 2026-10-15T12:32:42+02:00 — Agent B (vehicle-camera): follow take-over (reset seed with the stale clearance, near rule after a record pose, 0.1 filter ramp, double update; street hand-over replays 1e-3 against 1.1 before), pitch clamp 0x2e0a50, vehicle camera (InVehicle block with record floats, +10 m / +2.5, automatic turn, no stick turn, enter / leave bit-exact, drive 5e-3), director hand-back seed; CAMERA-9, CAMERA-14, VEH-8 closed, CAMERA-15 open → [camera](features/camera.md)
- 2026-10-15T12:42:00+02:00 — Agent E (mission-1-1-run): mission 1-1 runs end to end in `chase-game` (11 steps, StoryProgress 2, CTests `game-mission-1-1` and `game-mission-1-1-start`, dev step `at X Y Z`, script/cutscene log lines); MOVIE-23 and COLL-22 settled, SCRIPT-200/201 opened, [feature file](features/script-system-missions.md)
- 2026-10-15T13:08:14+02:00 — Agent J (buildit-trigger): oracle ctrace found the build-it's TriggerObject fired by the reference-trigger step 0x23211c (SCRIPT-180 closed, ported in `reference_trigger.*` with the AND-join); carry target enables PlaceObject; open: SCRIPT-220, SCRIPT-221, SCRIPT-190 narrowed, see [script-system-missions.md](features/script-system-missions.md).
- 2026-10-15T13:41:10+02:00 — Agent I (camera-handback): CAMERA-15 cause narrowed (stale clearance refuted, seed shortened by the reset's obstruction branch, CAMERA-20), hand-back test compares the whole track; CameraFollow stand-in and vehicle PositionObject (SCRIPT-201 narrowed), screenshot pair not taken, [feature file](features/camera.md), [finding](../re/camera-vehicle.md#ts-20261015-134100)
- 2026-10-15T13:43:26+02:00 — Agent H (vehicle-physics-2): VEH-30 and VEH-31 closed: rigid body orientation and angular step, per-wheel shock loader, wheel rays and the spring and damper force traced in the trooper and the van and transcribed (`vehicle_body.*`, `vehicle_wheels.*`); vehicles rest and tilt on their suspension, horizontal motion still kinematic; physics items split into a new feature file; [feature file](features/vehicles-physics.md)
- 2026-10-15T15:12:57+02:00 — Agent M (traffic-music-natives): OverrideMusic, AutomaticMusic, StopMusic (`natives_music.*`) and EnableTrafficAndPedestrians run as stand-ins, CAMERA-22 closed (manager update 0x23f174 switches cameras; CAMERA-40 open); SCRIPT-201 narrowed: [script-system-missions.md](features/script-system-missions.md)
- 2026-10-15T15:46:00+02:00 — Agent N (sky-models): RENDER-37 closed: sky pass 0x180fac read (camera orientation with the eye at (0, lift, 0), after clear and before world, unfogged), sky models drawn in `chase-game`, RENDER-50 open, RENDER-S8 replaced by S50 and S51 (see [rendering](features/rendering.md))
- 2026-10-15T15:23:00+02:00 — Agent L (camera-reset-obstruction): reset 0x2313c8 obstruction branches transcribed (line test, ray test, pull-in, candidate loop; CAMERA-10, 13, 20 closed), hand-back look 0.10 -> 1e-4, settled height 0.245 -> 1.6e-3, seed row still 0.64 off (CAMERA-15, CAMERA-30 open), SnapCamera uses the ray branch; screenshot pair not taken (CAMERA-21); see [camera.md](features/camera.md)
- 2026-10-15T16:25:11+02:00 — Agent K (vehicle-engine-tyres): VEH-32 closed: engine, gears, steering and tyres of 0x33aea8 transcribed (`vehicle_drivetrain.*`), fixed 1/30 s world steps, kinematic body removed; one-step replays of trooper and van traces and new free-run RPC samples match; old free-run brake samples corrected (VEH-52 closed); van escort routes use a vehicle teleport; VEH-50, VEH-51, VEH-53, VEH-54 open; [feature file](features/vehicles-physics.md)
- 2026-10-15T16:36:35+02:00 — Agent P (music-block): updater 0x240c3c, start 0x24039c and mood route 0x206c40 transcribed (`MusicBlock`), driven by the music natives and the level start; ramp replayed from an oracle memtrace; AUDIO-10 and AUDIO-2 closed, AUDIO-20/21 open — [audio.md](features/audio.md)
