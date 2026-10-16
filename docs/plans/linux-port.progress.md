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

**Priorities (critical path), updated 2026-10-16T11:25:50+02:00:** mission 1-1 runs end to end in `chase-game` with dev teleports between the stops (CTest `game-mission-1-1`). Done for mission 1-1: waypoint class with vehicle waypoints, in-engine cutscenes with the level-start intro and fades, cutscene actors, build-it with its trigger path, carrying with the code-side trigger path, vehicles created and solid, getting into and out of vehicles (also by script), the vehicle body on the original's gravity, damping, suspension, engine, gears and tyres, the vehicle camera and the hand-back after a director camera, the render near and far plane, parked bodies settling, the second floor cell search, the sphere sweep, floor-graph wheel rays, contact damage and hearts, props smashed by cars, the build animation and sounds, carry animations and hand attachment, the director camera holding the player, the music block, vehicle PositionObject, NPC idle animation. Oracle states before every step: `m11-1` to `m11-9` (docs/re/oracle-states.md).
1. Real play instead of teleports (SCRIPT-200): the car break and wreck (VEH-110, VEH-111: why the break comes 98 updates after the stop), the rest of driving collision (VEH-7, VEH-103, VEH-102); the user's invisible car (VEH-56) waits for the place from `chase-game` (F3 overlay position): [vehicles-damage](features/vehicles-damage.md), [vehicles](features/vehicles.md).
2. Open oracle measurements, best done by one agent that holds `--rpc` first: VEH-111, VEH-102, LEVOBJ-74 (place and drop), LEVOBJ-78; the exact steps are in each item.
3. Visible gaps of mission 1-1: particles for bricks and dust (PART-1, VEH-104), shadows (RENDER-33, RENDER-9), the studs and camera of the build (LEVOBJ-76, LEVOBJ-77), the van's measured collision box (VEH-105); the user still has to check the build (LEVOBJ-72) and carrying in play.
4. Mission 1-1 natives that are stand-ins: traffic and pedestrians (SCRIPT-230); the camera classes (CAMERA-40, CAMERA-52). Trigger origins: SCRIPT-190, SCRIPT-111, SCRIPT-220, SCRIPT-221, SCRIPT-35, SCRIPT-181, SCRIPT-182.
5. After mission 1-1: HUD fidelity, saves (FE-30, SAVE-14, SAVE-15), level switch screens (TRANS-13, TRANS-14), streaming (WSTREAM-30, TRANS-3), death and respawn (PLAYER-11), rendering (RENDER-60).

Oracle states (artifacts/oracle/states, see docs/re/oracle.md): `gameplay-start` (new game, Old 3DS), `police-station` and `street-after-1-1` (user saves after mission 1-1, New 3DS), `street-at-entrance`, `station-at-exit`, `station-exit-loadingpage`, `camera-lobby-walk`, `dcam-police-dialog`, `agent-doughnut-title` (agents, unmodified, New 3DS), `savecont-slotlist-user` (slot list with the user's save, Old 3DS), `trans-poked-*` (poked, measuring aids only). The real save of the user (read-only copy): `artifacts/oracle/agentJ-saves/savegame.dat`. Hard-to-reach states: ask the user (CLAUDE.md "Runtime oracle").

**Workflow notes:** oracle route and states: `tools/azahar_oracle.py` with `--display xvfb --state gameplay-start --renderer opengl --rpc --rpc-input`, route from boot `tools/oracle/to-gameplay.txt`; only one agent may use `--rpc` at a time. The lead delegates to subagents in worktrees (at most 3 in parallel, user), every brief names the CLAUDE.md rules, reviews each result against CLAUDE.md, rebuilds and tests, squash-merges one commit per branch with `tools/merge_agent.py prepare|check|finish` (it removes the worktree and branch), pushes when authorized, and checks the Graphify rebuild. Agents keep scratch files in their own subdirectory of the shared scratchpad. Worktree agents link `artifacts/` into their worktree (CLAUDE.md "Subagents"). Resume a branch with `git log main..<branch>`.

## Session Log

The newest 20 lines only; `python3 tools/check_doc_links.py --fix` moves older lines unchanged to [linux-port.progress-archive.md](linux-port.progress-archive.md). One line per session: timestamp, what was worked on, how far it got, link to the feature file. Findings, open items and stand-ins do not belong here (CLAUDE.md "Feature files"). Reverse-engineering details are in docs/re/ (see docs/re/README.md).

- 2026-10-15T18:33:20+02:00 — Agent X (prop-parts): per-node scene instances for placed props (`Scene::addModelNodes`, `prop_nodes`), build-it bricks scatter, fly and the final prop rises from the `_buildit` animation, the waypoint Badge is a class-file child (`dummymeshbouncer`) that turns and hops; LEVOBJ-40 and HUD-17 closed, LEVOBJ-60..62 and HUD-40 open, no oracle run — [level-objects-buildit.md](features/level-objects-buildit.md)
- 2026-10-15T19:07:00+02:00 — Agent Y (vehicle box collisions): VEH-60 closed: Bullet 2.78 contacts of the vehicle body with the 217 static boxes (compound child, dBoxBox2, persistent manifolds, sequential impulse solver after the floor pass; `vehicle_box_contact.*`), traced solve and collision detection replay to 1e-4, the trooper stops at the runA obstacle as traced (VEH-63 closed, VEH-56 narrowed); docs split into the collision subtopic, stand-ins VEH-S60, VEH-S62, item VEH-70 (VEH-S61 and VEH-71 closed: friction 0.5 of 0x243f90), VEH-62 narrowed (builder 0x2184a4) — [vehicles-physics-collision.md](features/vehicles-physics-collision.md)
- 2026-10-15T19:57:30+02:00 — Agent AB (map markers): the map screen's icon code read (REA 0x2db0dc, 0x2ddd64, 0x1ee7a8): waypoint = `WayPoint.tga` with two line circles, placement 0.94977 px per unit around (160, 120), edge ellipse with arrow, grow, ring clock 3 s; the map picture now equals the oracle's (HUD-16 closed, HUD-S5 and HUD-S10 removed, HUD-60 and HUD-61 opened) — [hud-ui.md](features/hud-ui.md)
- 2026-10-15T20:06:40+02:00 — Agent AC (round 13 leftovers): LEVOBJ-60 closed (unbuilt build-it bricks hop as 0x206ea0, start values 0x18275c, checked against 40 oracle rows), HUD-40 closed (waypoint column and badge fade as the node tweens 0x21b7cc, per-instance fade in the scene), SCRIPT-271 closed (skinned placed props play PlayAnimation/StopAnim/WaitForCurrentAnim); SCRIPT-272 (Attach node changes) still open; items LEVOBJ-70/71, HUD-70, stand-ins LEVOBJ-S70/S71, HUD-S70 — [level-objects-buildit.md](features/level-objects-buildit.md)
- 2026-10-15T20:29:48+02:00 — Agent AA (driving collision): vehicle against vehicle through the original's Bullet pair contacts and two-body solve, parked vehicles as sleeping bodies a hit wakes, mass scale and shape inertia (`vehicle_world.*`); traced hit solved to 1e-5; VEH-56 narrowed, VEH-7 continues with placed objects and damage (VEH-90..94 opened): [vehicles-physics-collision](features/vehicles-physics-collision.md)
- 2026-10-16T08:33:53+02:00 — Agent C (clip-planes): CAMERA-50 and RENDER-40 closed: the follow update writes (near plane test result, 260) to the render camera, the port projects and builds the fog table with them (`clip_planes.hpp`, test `clip-planes`); other writers open as CAMERA-52, see [camera.md](features/camera.md)
- 2026-10-16T08:35:33+02:00 — Agent B (round 14, VEH-33): gravity -39, sleep time 0.75 and the step record read in the original; parked vehicles start awake and rest at the oracle's poses to 0.4 mm (FP_Vehicle_03/04/05 and the police car body); VEH-33 closed, CAMERA-51 to re-measure — [vehicles-physics-collision.md](features/vehicles-physics-collision.md)
- 2026-10-16T09:01:13+02:00 — Agent A (round 14, VEH-92/94/56): VEH-94 closed (second floor cell search 0x164328 ported with 0x1aa478, 0x2384dc, 0x158cb8; an awake car teleported under an overhead floor layer is no longer thrown), VEH-S51 removed; VEH-92 answered (placed props do not stop the car, the original smashes them: VEH-98), bump pass 0x2d4af0 and the car breaking into bricks opened as VEH-99, VEH-56 narrowed (not reproduced in the port) — [vehicles-physics-collision.md](features/vehicles-physics-collision.md)
- 2026-10-16T09:41:25+02:00 — Agent B (round 15, MOVIE-31 / LEVOBJ-73): Chase no longer drawn twice in cutscene 1_1_d (placement after the jump out re-showed the hidden figure; MOVIE-31 closed); carrying plays the original's pickup / carry / put-down clips, hangs the box on the hand joint through its carryhandle node, turns to the object and walks at walk step x 2.5 (LEVOBJ-73 closed, ends of the states and the box fall open as LEVOBJ-74, LEVOBJ-75) — [level-objects-carry.md](features/level-objects-carry.md)
- 2026-10-16T09:37:21+02:00 — Agent A (round 15, LEVOBJ-72/62): oracle trace of the build's node matrices (0x22b5a0): bricks fly to the final prop (F * inverse(P), 0x2318c8), not to the parts' place; ported and pinned (LEVOBJ-62 and S61 closed, LEVOBJ-72 narrowed to the user's look check); see [level-objects-buildit.md](features/level-objects-buildit.md)
- 2026-10-16T09:55:45+02:00 — Agent C (round 15, vehicles-physics-collision): VEH-61 and VEH-97 closed (sphere sweep of the floor pass; wheel rays through the floor graph), mass scale timing from the player states, VEH-62/70/90/91 narrowed, VEH-100..102 opened; see [vehicles-physics-collision.md](features/vehicles-physics-collision.md)
- 2026-10-16T10:30:43+02:00 — Agent E (round 16, player-movement): PLAYER-4 closed (ground probe candidate order: 0x2092d8 flag 8 swap-removal and first/last overlap of 0x2023e4; 133 of 133 recorded normals agree); PLAYER-2 narrowed (constant 0.111 push at the slide exit measured), PLAYER-3 (state 8 timeout read), PLAYER-1 narrowed, PLAYER-10/11 opened; see [player-movement.md](features/player-movement.md)
- 2026-10-16T10:12:36+02:00 — Agent D (round 16, small asset files): ASSET-5 (pregen collectables and object counts decode, all 8 files parse), ASSET-6 (script-list reader tolerates the missing and extra `)`; parser adapted) and ASSET-7 (options files unused) closed, ASSET-8 opened — [assets-formats.md](features/assets-formats.md)
- 2026-10-16T10:37:24+02:00 — Agent B (round 16, VEH-93 / VEH-99): vehicle contact damage ported (`vehicle_damage.*`: closing speed 0x19b5c4 of every manifold point and floor-pass contact, hearts and health points, 0.5 s / 1.0 s cool-downs from the initialiser 0x34498c, brick-burst rule; hooks `ManifoldPointObserver`, `FloorContactObserver`, `VehicleImpactSink`); the pair flag word writer 0x271764 found and ported (VEH-90 closed, VEH-S80 replaced by VEH-S93); oracle run: a 27.9 m/s hit costs one heart, the break came 98 updates later from the water test of 0x23019c (type 6, all hearts); break 0x16cccc read, not ported (VEH-110, VEH-111, VEH-112, VEH-113, VEH-115 open) — [vehicles-damage.md](features/vehicles-damage.md)
- 2026-10-16T10:42:33+02:00 — Agent C (round 16): player build animation (state 9, clip buildit) and build sounds played, LEVOBJ-41 closed; carry release (message 0x12) at the hand, pickup ends with its clip (oracle slot trace); studs, camera pair and place/drop ends open — [level-objects-buildit.md](features/level-objects-buildit.md), [level-objects-carry.md](features/level-objects-carry.md)
- 2026-10-16T11:17:32+02:00 — Agent A (round 16, VEH-98 / VEH-62 / VEH-102): a moving vehicle smashes wobbly props with a box (vehicle update 0x16ce9c -> 0x275c00 -> prop handler 0x2a3390; `vehicle_prop_smash.*`, tests `vehicle-prop-smash` pinned to the oracle phone box pass, `game-vehicle-smash`; VEH-98 and VEH-62 closed, VEH-103..VEH-106, VEH-S88/S89 opened, VEH-102 narrowed), also oracle reads for agents B and C — [vehicles.md](features/vehicles.md)
- 2026-10-16T11:31:00+02:00 — Agent (round 17, VEH-105): the donut van takes the general shape box (entity box minus CentreOfMass), the box stand-in and VEH-S50 are removed, tests re-pinned; see [vehicles-physics-collision.md](features/vehicles-physics-collision.md).
- 2026-10-16T11:40:21+02:00 — Agent (round 17, doc tooling): feature index by area with row rules, `--next` / `--reserve` / `--release` ID allocation, future-timestamp error and `:00` warning, item field order, `Superseded by` entries moved to `.history.md` by `--fix`; CLAUDE.md rules and tests updated; see [feature index](features/README.md)
- 2026-10-16T11:47:49+02:00 — Agent (round 17, docs/re supersede triage): 100 `Corrected by` lines triaged, 0 entries superseded or moved, all kept as partial corrections (each changes one value, address or detail; the four most likely entries were read in full); see docs/re/README.md
- 2026-10-16T12:10:05+02:00 — Lead (docs workflow): `## Current state` sections written for all 77 docs/re topic files (six agents, two waves), five missing correction pairs added, every Current state now required by the checker and split with its file; see [features/README.md](features/README.md)
