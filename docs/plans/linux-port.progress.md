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

**Priorities (critical path), updated 2026-10-15T17:17:02+02:00:** mission 1-1 runs end to end in `chase-game` with dev teleports between the stops (CTest `game-mission-1-1`). Done for mission 1-1: waypoint class with vehicle waypoints, in-engine cutscenes with the level-start intro and fades, cutscene actors, build-it with its trigger path, carrying with the code-side trigger path, vehicles created and solid, getting into and out of vehicles (also by script), the vehicle body on the original's gravity, damping, suspension, engine, gears and tyres, the vehicle camera and the hand-back after a director camera, the director camera holding the player, the music block, vehicle PositionObject, NPC idle animation. Oracle states before every step: `m11-1` to `m11-9` (docs/re/oracle-states.md).
1. Real play instead of teleports (SCRIPT-200): the floor pass and the static box contacts are done (VEH-40, VEH-60); left are driving collision with other vehicles and placed objects (VEH-7, VEH-56) and the parked body (VEH-33): [vehicles-physics](features/vehicles-physics.md), [vehicles](features/vehicles.md).
2. Mission 1-1 natives that are stand-ins: traffic and pedestrians (SCRIPT-230); the camera classes (CAMERA-40) and the render near plane (CAMERA-50).
3. Trigger origins: carry gaps (SCRIPT-190), class-fired triggers (SCRIPT-111), build-it reference gaps (SCRIPT-220, SCRIPT-221); `SetPartyCharacter` (SCRIPT-35); the director camera's other readers (SCRIPT-181, SCRIPT-182).
4. Polish of the finished steps: carry and build-it stand-ins (level-objects-carry.md, level-objects-buildit.md), the police car start state (VEH-13), the enter prompt and vehicle HUD (VEH-17, VEH-18), wheel node pose (VEH-43), animation blending (ANIM-3), COLL-21, the bottom-screen panel during cutscenes (MOVIE-24); small file readers (ASSET-5, ASSET-6).
5. After mission 1-1: HUD fidelity, saves (FE-30, SAVE-14, SAVE-15), level switch screens (TRANS-13, TRANS-14), streaming (WSTREAM-30, TRANS-3), rendering (RENDER-9, RENDER-33, RENDER-60).

Oracle states (artifacts/oracle/states, see docs/re/oracle.md): `gameplay-start` (new game, Old 3DS), `police-station` and `street-after-1-1` (user saves after mission 1-1, New 3DS), `street-at-entrance`, `station-at-exit`, `station-exit-loadingpage`, `camera-lobby-walk`, `dcam-police-dialog`, `agent-doughnut-title` (agents, unmodified, New 3DS), `savecont-slotlist-user` (slot list with the user's save, Old 3DS), `trans-poked-*` (poked, measuring aids only). The real save of the user (read-only copy): `artifacts/oracle/agentJ-saves/savegame.dat`. Hard-to-reach states: ask the user (CLAUDE.md "Runtime oracle").

**Workflow notes:** oracle route and states: `tools/azahar_oracle.py` with `--display xvfb --state gameplay-start --renderer opengl --rpc --rpc-input`, route from boot `tools/oracle/to-gameplay.txt`; only one agent may use `--rpc` at a time. The lead delegates to subagents in worktrees (at most 3 in parallel, user), every brief names the CLAUDE.md rules, reviews each result against CLAUDE.md, rebuilds and tests, squash-merges one commit per branch with `tools/merge_agent.py prepare|check|finish` (it removes the worktree and branch), pushes when authorized, and checks the Graphify rebuild. Agents keep scratch files in their own subdirectory of the shared scratchpad. Worktree agents link `artifacts/` into their worktree (CLAUDE.md "Subagents"). Resume a branch with `git log main..<branch>`.

## Session Log

The newest 20 lines only; `python3 tools/check_doc_links.py --fix` moves older lines unchanged to [linux-port.progress-archive.md](linux-port.progress-archive.md). One line per session: timestamp, what was worked on, how far it got, link to the feature file. Findings, open items and stand-ins do not belong here (CLAUDE.md "Feature files"). Reverse-engineering details are in docs/re/ (see docs/re/README.md).

- 2026-10-15T13:43:26+02:00 — Agent H (vehicle-physics-2): VEH-30 and VEH-31 closed: rigid body orientation and angular step, per-wheel shock loader, wheel rays and the spring and damper force traced in the trooper and the van and transcribed (`vehicle_body.*`, `vehicle_wheels.*`); vehicles rest and tilt on their suspension, horizontal motion still kinematic; physics items split into a new feature file; [feature file](features/vehicles-physics.md)
- 2026-10-15T15:12:57+02:00 — Agent M (traffic-music-natives): OverrideMusic, AutomaticMusic, StopMusic (`natives_music.*`) and EnableTrafficAndPedestrians run as stand-ins, CAMERA-22 closed (manager update 0x23f174 switches cameras; CAMERA-40 open); SCRIPT-201 narrowed: [script-system-missions.md](features/script-system-missions.md)
- 2026-10-15T15:46:00+02:00 — Agent N (sky-models): RENDER-37 closed: sky pass 0x180fac read (camera orientation with the eye at (0, lift, 0), after clear and before world, unfogged), sky models drawn in `chase-game`, RENDER-50 open, RENDER-S8 replaced by S50 and S51 (see [rendering](features/rendering.md))
- 2026-10-15T15:23:00+02:00 — Agent L (camera-reset-obstruction): reset 0x2313c8 obstruction branches transcribed (line test, ray test, pull-in, candidate loop; CAMERA-10, 13, 20 closed), hand-back look 0.10 -> 1e-4, settled height 0.245 -> 1.6e-3, seed row still 0.64 off (CAMERA-15, CAMERA-30 open), SnapCamera uses the ray branch; screenshot pair not taken (CAMERA-21); see [camera.md](features/camera.md)
- 2026-10-15T16:25:11+02:00 — Agent K (vehicle-engine-tyres): VEH-32 closed: engine, gears, steering and tyres of 0x33aea8 transcribed (`vehicle_drivetrain.*`), fixed 1/30 s world steps, kinematic body removed; one-step replays of trooper and van traces and new free-run RPC samples match; old free-run brake samples corrected (VEH-52 closed); van escort routes use a vehicle teleport; VEH-50, VEH-51, VEH-53, VEH-54 open; [feature file](features/vehicles-physics.md)
- 2026-10-15T16:36:35+02:00 — Agent P (music-block): updater 0x240c3c, start 0x24039c and mood route 0x206c40 transcribed (`MusicBlock`), driven by the music natives and the level start; ramp replayed from an oracle memtrace; AUDIO-10 and AUDIO-2 closed, AUDIO-20/21 open — [audio.md](features/audio.md)
- 2026-10-15T16:59:26+02:00 — Agent Q (camera-handback): near plane test 0x2e0e28 and its tracker snap transcribed, StartDCam clears K+0x178 (CAMERA-15, 21, 30, 31 closed, CAMERA-S30 removed), hand-back seed rows 0.64 -> 2.4e-4, track 2.9e-3; chase-game camera after the hand-back off by the parked car's 4 cm (CAMERA-51), render near plane open (CAMERA-50); see [camera.md](features/camera.md)
- 2026-10-15T17:28:28+02:00 — Agent T (sound-natives): the ten sound natives of batch 8 transcribed (`natives_sound.*`: play, stop, volume, frequency, registry; voice controls in the mixer), mission 1-1 calls none; SCRIPT-33 and AUDIO-21 closed, stand-ins AUDIO-S30..S32, items AUDIO-30..33 — [audio.md](features/audio.md)
- 2026-10-15T17:46:00+02:00 — Agent U (update pacing): FramePacer gives level sessions 30 updates/s with dt 1/30 (EXEC-5 closed, EXEC-S10/EXEC-10 opened, EXEC-3 narrowed), model test shows 34 of 334 zero-step updates before; real-display driving not seen. [executable-platform.md](features/executable-platform.md)
- 2026-10-15T17:48:54+02:00 — Agent S (lighting): fragment lighting transcribed from 0x12d884, the light list 0x250688, the light class 0x15dd50 and the lighting init 0x115850 (LUTs D0 pow 20, DA, SP) with level light objects; registers equal the oracle command list; RENDER-1, RENDER-5 closed, RENDER-S1 removed, RENDER-60..62 and RENDER-S60/S61 open — [rendering.md](features/rendering.md)
- 2026-10-15T18:11:19+02:00 — Agent R (vehicle-collision): VEH-40 closed: the physics world is Bullet; vehicles collide with the floor in 0x1c54f8 (floor graph 0x11fd20, walk 0x23b5e8, contact and push-out impulses), sleep rule 0x30cdf8, at most 2 world steps; traced landing and drop replay exactly; vehicle-drive also runs on the lvl_01 street; VEH-53 closed; Bullet box contacts VEH-60..63 open — [vehicles-physics.md](features/vehicles-physics.md)
- 2026-10-15T18:36:38+02:00 — Agent W (animation natives): the slot player 0x245b40 / 0x24cbd4 (modes, offsets, stop, waits) drives the level characters; PlayCharAnim, PlayAnimation, the stops and the waits play through it, attach records corrected (second object under the first); van driver stuck in the mailbox (ANIM-5, ANIM-7, SCRIPT-112 closed; SCRIPT-270..274, ANIM-10/11, ANIM-S10..S12 opened). [script-system-natives-animation.md](features/script-system-natives-animation.md)
- 2026-10-15T18:37:10+02:00 — Agent Z (vehicle-visuals): wheel nodes posed as 0x33929c (travel, steer, spin; VEH-43 closed), X prompt head icon (HeadIcons, fade, alpha, projected quad; VEH-17 closed) and the seated HUD (vehicle portrait on heartsBG_generic, blue hearts with the start health; VEH-18 closed); stand-ins VEH-S70, HUD-S50, HUD-S51, items VEH-80, VEH-81, HUD-50 — [vehicles-boarding.md](features/vehicles-boarding.md)
- 2026-10-15T18:33:20+02:00 — Agent X (prop-parts): per-node scene instances for placed props (`Scene::addModelNodes`, `prop_nodes`), build-it bricks scatter, fly and the final prop rises from the `_buildit` animation, the waypoint Badge is a class-file child (`dummymeshbouncer`) that turns and hops; LEVOBJ-40 and HUD-17 closed, LEVOBJ-60..62 and HUD-40 open, no oracle run — [level-objects-buildit.md](features/level-objects-buildit.md)
- 2026-10-15T19:07:00+02:00 — Agent Y (vehicle box collisions): VEH-60 closed: Bullet 2.78 contacts of the vehicle body with the 217 static boxes (compound child, dBoxBox2, persistent manifolds, sequential impulse solver after the floor pass; `vehicle_box_contact.*`), traced solve and collision detection replay to 1e-4, the trooper stops at the runA obstacle as traced (VEH-63 closed, VEH-56 narrowed); docs split into the collision subtopic, stand-ins VEH-S60, VEH-S62, item VEH-70 (VEH-S61 and VEH-71 closed: friction 0.5 of 0x243f90), VEH-62 narrowed (builder 0x2184a4) — [vehicles-physics-collision.md](features/vehicles-physics-collision.md)
- 2026-10-15T19:57:30+02:00 — Agent AB (map markers): the map screen's icon code read (REA 0x2db0dc, 0x2ddd64, 0x1ee7a8): waypoint = `WayPoint.tga` with two line circles, placement 0.94977 px per unit around (160, 120), edge ellipse with arrow, grow, ring clock 3 s; the map picture now equals the oracle's (HUD-16 closed, HUD-S5 and HUD-S10 removed, HUD-60 and HUD-61 opened) — [hud-ui.md](features/hud-ui.md)
- 2026-10-15T20:06:40+02:00 — Agent AC (round 13 leftovers): LEVOBJ-60 closed (unbuilt build-it bricks hop as 0x206ea0, start values 0x18275c, checked against 40 oracle rows), HUD-40 closed (waypoint column and badge fade as the node tweens 0x21b7cc, per-instance fade in the scene), SCRIPT-271 closed (skinned placed props play PlayAnimation/StopAnim/WaitForCurrentAnim); SCRIPT-272 (Attach node changes) still open; items LEVOBJ-70/71, HUD-70, stand-ins LEVOBJ-S70/S71, HUD-S70 — [level-objects-buildit.md](features/level-objects-buildit.md)
- 2026-10-15T20:29:48+02:00 — Agent AA (driving collision): vehicle against vehicle through the original's Bullet pair contacts and two-body solve, parked vehicles as sleeping bodies a hit wakes, mass scale and shape inertia (`vehicle_world.*`); traced hit solved to 1e-5; VEH-56 narrowed, VEH-7 continues with placed objects and damage (VEH-90..94 opened): [vehicles-physics-collision](features/vehicles-physics-collision.md)
- 2026-10-16T08:33:53+02:00 — Agent C (clip-planes): CAMERA-50 and RENDER-40 closed: the follow update writes (near plane test result, 260) to the render camera, the port projects and builds the fog table with them (`clip_planes.hpp`, test `clip-planes`); other writers open as CAMERA-52, see [camera.md](features/camera.md)
- 2026-10-16T08:35:33+02:00 — Agent B (round 14, VEH-33): gravity -39, sleep time 0.75 and the step record read in the original; parked vehicles start awake and rest at the oracle's poses to 0.4 mm (FP_Vehicle_03/04/05 and the police car body); VEH-33 closed, CAMERA-51 to re-measure — [vehicles-physics-collision.md](features/vehicles-physics-collision.md)
