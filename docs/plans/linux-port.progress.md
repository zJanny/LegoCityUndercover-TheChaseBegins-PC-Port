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

**Question:** how agents get to game points that need playing (missions, routes) in the oracle. **Decision (user):** an agent that would have to play there, or whose scripted attempts fail, asks the user through the lead with everything needed to play to that point; the user saves the state in their own Azahar and the lead copies it into `artifacts/oracle/states/`. **Reason:** an agent brute-forcing a route with scripted input costs hours and tokens; the user plays it in minutes (example: the save state outside the police station after mission 1-1). Rule in CLAUDE.md "Runtime oracle".

## Blockers

Full game logic, hardware APIs and asset formats not recovered yet.

## Deviations From Plan

- 2026-10-12: Plan section 16 said no commits. Actual: commits made per user's CLAUDE.md workflow rule. Plan revision 2 records this; no code steps reopened.

## Next Action

Read this section, CLAUDE.md (incl. "Plans and order of work" and "Feature files"), [docs/rules/port.md](../rules/port.md), [features/README.md](features/README.md) and, before any emulator use, docs/re/oracle.md. Open items, stand-ins and findings are in the feature files, not here. Feature layer introduced 2026-10-13T21:34:59+02:00; earlier handoff 2026-10-13T17:53:20+02:00.

**Goal (user, 2026-10-13):** faithful PC port, playable start to finish, Linux first; Windows only when the user says so; QoL only after the PC version is finished (Decisions, CLAUDE.md).

**State:** M1 runtime loop, M2 locomotion, M3 walkable level, M4 level objects, characters, studs, falling, jumping and object collision are on main; the front end (boot to the first level) and the sound hooks are merged. `chase-game` is a development runtime, not the game ([docs/status.md](../status.md)).

**Goal of the current rounds (user, 2026-10-19):** mission 1-3 next, unless the user reports more critical mission 1-2 bugs. Mission 1-1 is playable end to end (user playthrough, [scripts-missions.md §2026-10-18T10:37](../re/scripts-missions.md#ts-20261018-103746)); mission 1-2 was played through by the user ([playtests.md](../re/playtests.md)) and its four playtest bugs are fixed on main, not yet re-checked by the user; the mission 1-2 gap list is linked from [script-system-missions](features/script-system-missions.md).

**User request for the next round (2026-10-28, recorded 2026-10-28T23:44:28+01:00):** the user asked what in the workflow can be sped up (long waits on the merge agent, building and tests) and answered "Yes, plan 1 to 3 for the next round" to the lead's three points. The planner proposes a workflow-tooling round for them (tooling and skill text only, no game behaviour):
1. `tools/merge_agent.py` step `finish` reruns the full build, ctest and Python unittests even when the staged tree equals the one the last `prepare`/`check` passed; `finish` should skip the checks then.
2. Every agent worktree builds from scratch and ccache is not used; `CMakeLists.txt` should use ccache as compiler launcher when found (optional, no failure without it). The user installs ccache themselves.
3. Branches commit the generated files (README progress section, `docs/plans/priorities.md`, `docs/progress-trend.svg`), so nearly every merge conflicts; branches should not commit them, only the merge script makes them, and docs-only conflicts needing both sides are resolved on main by the script or a cheap agent, not by the resumed worker.
Text these changes touch: `CLAUDE.md` (Workflows > Git: the merge agent bullet and the `progress_report.py --update-readme` bullet; Subagents), `.claude/skills/lead-round/SKILL.md` (merge gate, the `finish` call and its failure handling), `references/worker-brief.md` (lines on `progress_report.py --update-readme` and `priority_report.py --update`), `references/code-review.md` and `references/docs-review.md` (generated-file checks), `references/planner.md` (reads `priorities.md`), `docs/commands.md` (Merging an agent branch, CTest `progress-readme` and `priorities-current`), `docs/building.md`, `CONTRIBUTING.md` (commit-time regeneration line, setup), `docs/rules/port.md` if it names the checks, and the CTest checks `progress-readme` / `priorities-current` that fail on stale generated files in a branch.

**Priorities (critical path), updated 2026-10-19T19:49:14+02:00:** oracle states for every mission 1-2 step: the save state in front of the mission 1-2 start trigger to the save state after mission 1-2, the save state at the foot of the drain pipe of mission 1-2, the save state beside the drain pipe of mission 1-2, the save state in front of the table of mission 1-2, the save state in front of the broken table of mission 1-2, the save state at the built trampoline of mission 1-2, vehicle effects: the save state in a police van in front of a lamp post, the save state in a police van in front of moving cars, the save state in a police van with one heart left, the save state in a police van at full speed towards a brick wall (local state catalogue next to the states); more on request from the user (new state requests deferred by the user on 2026-10-18). Rounds run three workers (user, 2026-10-18: short rounds); [priorities.md](priorities.md) ranks open items by their reach in the whole game and, when two need `--rpc`, one emulator servant that owns the only `--rpc` session.
1. Mission 1-3 (save state on the roof in front of the super brick of mission 1-3 to the save state on the fast travel pad of the built helipad, local state catalogue): the super brick pickup, the ladder and the helipad build are on main and not yet played by the user; next the super brick HUD display and flight (HUD-72, HUD-73, STUD-18, [studs](features/studs.md)), the count loaded from the save (STUD-17), the helipad's fast travel pad and menu (LEVOBJ-118, [level-objects-buildit](features/level-objects-buildit.md)), the rest of the super build-it class (LEVOBJ-117); the scanner's info boxes (SCAN-12, [abilities-scanner](features/abilities-scanner.md)); an oracle picture comparison of the objective markers and arrows (SCRIPT-330, needs `--rpc`, [script-system-hud-natives](features/script-system-hud-natives.md)). `hud-ui.md` is at the 16 KB limit: split it before adding HUD items.
2. One worker per round on a wide-reach item from [priorities.md](priorities.md), whether or not mission 1-2 needs it. The trampoline's look and the table's wobble (LEVOBJ-S77, LEVOBJ-S79, LEVOBJ-90, [level-objects-wobbly](features/level-objects-wobbly.md)); the remaining callers of dropped studs (STUD-14, STUD-15, [studs](features/studs.md)); the other map icons (HUD-60, [hud-ui](features/hud-ui.md)); the player script clip's request timing and first pose (SCRIPT-333, SCRIPT-S272, [script-system-natives-animation](features/script-system-natives-animation.md)); the mission 1-2 start one update late (SCRIPT-114, [script-system-missions](features/script-system-missions.md)); the waypoint disable fade (SCRIPT-S269, [script-system-hud-natives](features/script-system-hud-natives.md)); joints of named model nodes (MOVIE-40, [movies-cutscenes](features/movies-cutscenes.md)); saves: the level block refresh (SAVE-14) and live summary inputs (SAVE-15, [saves-options](features/saves-options.md)); oracle checks of the zipline A press and the pole A press (TRAV-8, TRAV-34, need user states).
3. The dog and the dog truck: animals are not created (LEVOBJ-7), the truck's moving bound and driving (SCRIPT-77, SCRIPT-43); cutscene 1_2_c and the mission end (SCRIPT-115) ([script-system-missions](features/script-system-missions.md)).
4. Open mission 1-1 gaps: the rest of the vehicle effects (VEH-121, VEH-123, VEH-124, VEH-128, PART-2), the studs and camera of the build (LEVOBJ-76, LEVOBJ-77), shadows (RENDER-72, RENDER-73), traffic and pedestrians (SCRIPT-230), camera classes (CAMERA-40, CAMERA-52).
5. Later: HUD fidelity, saves (FE-30, SAVE-14, SAVE-15), level switch screens (TRANS-13, TRANS-14), streaming (WSTREAM-30, TRANS-3), death and respawn (PLAYER-11), rendering (RENDER-60).

Oracle states: the list of states, their game progress and positions is the local catalogue next to the states (not in the repository); the tool and the console mode of states: `docs/re/oracle.md`, [oracle-setup.md](../re/oracle-setup.md). Hard-to-reach states: ask the user (CLAUDE.md "Runtime oracle").

**Workflow notes:** oracle route and states: `tools/azahar_oracle.py` with `--display xvfb --state gameplay-start --renderer opengl --rpc --rpc-input`, route from boot `tools/oracle/to-gameplay.txt`; only one agent may use `--rpc` at a time. The lead delegates to subagents in worktrees (at most 3 in parallel, user), every brief names the CLAUDE.md rules, reviews each result against CLAUDE.md, rebuilds and tests, squash-merges one commit per branch with `tools/merge_agent.py prepare|check|finish` (it removes the worktree and branch), pushes when authorized, and checks the Graphify rebuild. Agents keep scratch files in their own subdirectory of the shared scratchpad. Worktree agents link `artifacts/` into their worktree (CLAUDE.md "Subagents"). Resume a branch with `git log main..<branch>`.

## Session Log

The newest 20 lines only; `python3 tools/check_doc_links.py --fix` moves older lines unchanged to [linux-port.progress-archive.md](linux-port.progress-archive.md). One line per session: timestamp, what was worked on, how far it got, link to the feature file. Findings, open items and stand-ins do not belong here (CLAUDE.md "Feature files"). Reverse-engineering details are in docs/re/ (see docs/re/README.md).

- 2026-11-07T12:24:54+01:00 — Agent 20261107-traffic-A (traffic junction update): the caller and order of the junction update read from the original and recorded, the junction records kept from the level start, every junction's light cycle run in chase-game with a line per light change, the route functions given the road and lane paths; wiring done, the cars are not connected yet ([pedestrians-traffic-cars-movement.md](features/pedestrians-traffic-cars-movement.md)).
- 2026-11-07T12:14:12+01:00 — Agent 20261107-traffic-C (traffic driver steering): the steering controller of AI driver states 5 and 8, the contact step of the post step and the top gear speed read from the disassembly and ported as pure steps with their test; not connected to the AI driver yet ([pedestrians-traffic-cars-driver](features/pedestrians-traffic-cars-driver.md)).
- 2026-11-07T12:13:25+01:00 — Agent 20261107-traffic-D (ground ray cast, path name hash): one shared path name helper for the junction fill, DCam paths and pedestrian path count; the vertical ground ray cast and the colour message ported as traffic_ground_world; [pedestrians-traffic-cars-movement.md](features/pedestrians-traffic-cars-movement.md)
- 2026-11-07T12:15:18+01:00 — Agent 20261107-traffic-B (traffic light look): the part flags, the set-up slot quirk and the cycle's changes ported as pure steps and drawn node by node; the cycle's changes wait for the junction update; the light class update is the physical prop update; see [pedestrians-traffic-cars-movement.md](features/pedestrians-traffic-cars-movement.md)
- 2026-11-07T12:10:05+01:00 — Agent 20261107-traffic-G (BulletTime rest): restore mode calls, mode byte, named-object lookup and the time scale callback ported as pure steps with a test, the slowed process read as the gameplay client, camera shot read in part — [script-system-missions-1-4-roof.md](features/script-system-missions-1-4-roof.md)
- 2026-11-07T16:49:34+01:00 — Agent 20261107-traffic-E (goon sleep and return): ported the idle gate, the sleep branch and the wait before the return to the spawn point (AI state 15) with a unit test; the walk back (AI state 14) and the sleep clips are not ported — [script-system-missions-1-4-conduct.md](features/script-system-missions-1-4-conduct.md)
- 2026-11-07T17:29:54+01:00 — Lead (20261107-traffic end): merged 20261107-traffic-F (ShowroomArrival timing, earlier session), 20261107-traffic-A (junction light cycle), 20261107-traffic-C (AI driver steering), 20261107-traffic-D (ground ray cast, path name hash), 20261107-traffic-B (traffic light looks), 20261107-traffic-G (BulletTime restore mode calls), 20261107-traffic-E (goon sleep and return) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [pedestrians-traffic-cars-movement.md](features/pedestrians-traffic-cars-movement.md), [pedestrians-traffic-cars-driver.md](features/pedestrians-traffic-cars-driver.md), [script-system-missions-1-4-conduct.md](features/script-system-missions-1-4-conduct.md)
- 2026-11-07T18:27:36+01:00 — Agent 20261107-traffic2-C (goon walk back): ported AI state 14 (the walk to the spawn point or patrol nodes with the port's path finder, arrival to idle or state 15, the blocked retry) and the watch's PlayerCaught event with its angle test, with a unit test; the walk's obstacle pass, type 11 nodes and a real-level run are open — [script-system-missions-1-4-conduct.md](features/script-system-missions-1-4-conduct.md)
- 2026-11-07T18:14:56+01:00 — Agent 20261107-traffic2-A (traffic light changes): the junction update now hands each light cycle change to the lights' looks, the priority bytes of junctions and cars are read; the room type that decides the update list is open; feature file split into movement and junction — [pedestrians-traffic-cars-junction.md](features/pedestrians-traffic-cars-junction.md)
- 2026-11-07T18:16:52+01:00 — Agent 20261107-traffic2-B (traffic route functions): read and ported the route functions ahead and nearest of the traffic routes and lanes as pure steps with a test, set them in the route functions the AI driver gets; the driver is not connected yet — [pedestrians-traffic-cars-driver.md](features/pedestrians-traffic-cars-driver.md)
- 2026-11-07T18:49:06+01:00 — Lead (20261107-traffic2 end): merged 20261107-traffic2-A (traffic light looks follow the junction cycle), 20261107-traffic2-B (traffic route ahead/nearest functions), 20261107-traffic2-C (goon AI state 14 walk back, PlayerCaught and angle test) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [pedestrians-traffic-cars-junction.md](features/pedestrians-traffic-cars-junction.md), [pedestrians-traffic-cars-driver.md](features/pedestrians-traffic-cars-driver.md), [script-system-missions-1-4-conduct.md](features/script-system-missions-1-4-conduct.md)
- 2026-11-08T12:14:03+01:00 — Agent 20261108-traffic-C (mission 1-4 chase end): ported the chase end (contact on the last heart of a chased car in driver state 5 requests state 6, which fires VehicleChaseEnded, starting `1-4_Car_Crash`) as a pure step wired to the vehicle contact; fixed the time before the first cut of `1-4_ShowroomArrival` to the free run (Play at 0.23 s, mode enter at 0.86 s); listed what blocks a full 1-4 playthrough. See [script-system-missions-1-4.md](features/script-system-missions-1-4.md).
- 2026-11-08T12:23:49+01:00 — Agent 20261108-traffic-B (traffic driver connected): the AI driver now runs on the spawned traffic cars in chase-game, so they drive along their roads, stop at red lights, go through the junction lanes and leave at roads without junction; a removed car leaves its junction queue; states 4 to 8 and the vehicle body are open — [pedestrians-traffic-cars-driver.md](features/pedestrians-traffic-cars-driver.md)
- 2026-11-08T12:02:38+01:00 — Agent 20261108-traffic-D (traffic light prop and junction rooms): read the traffic light prop update and the junction update list skip bit, nothing to port, a measurement of the junction rooms is still needed; no code change. [pedestrians-traffic-cars-junction](features/pedestrians-traffic-cars-junction.md)
- 2026-11-08T12:14:10+01:00 — Agent 20261108-traffic-A (traffic driver calls): read the writer of the launch byte (VehicleSetRaceTrim), the body placement, the contact slide, the chase markers, the whistle stop and the brake lights of AI driver states 5 and 8 and ported them as pure steps with a test; not connected to the driver yet — [pedestrians-traffic-cars-driver.md](features/pedestrians-traffic-cars-driver.md)
- 2026-11-08T12:59:32+01:00 — Lead (20261108-traffic end): merged 20261108-traffic-D (traffic light update needs no prop physics), 20261108-traffic-A (traffic driver race trim, contact slide, brake lights, whistle stop), 20261108-traffic-B (AI driver drives the traffic cars, states 0 to 3), 20261108-traffic-C (mission 1-4 chase end event and measured cutscene start timing) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [pedestrians-traffic-cars-junction.md](features/pedestrians-traffic-cars-junction.md), [pedestrians-traffic-cars-driver.md](features/pedestrians-traffic-cars-driver.md), [script-system-missions-1-4.md](features/script-system-missions-1-4.md)
- 2026-11-12T00:12:11+01:00 — Agent 20261111-chase-C (booster chaser and character jump in): read the event 200 bindings and the boarding states every character runs, ported the script jump in and out of characters other than the player with the CharacterLeftVehicle event; the booster chaser of mission 1-4 comes out of his spawner and jumps into the stolen car, his jump out after the chase end is not run in play yet — [script-system-missions-1-4](features/script-system-missions-1-4.md), [vehicles](features/vehicles.md)
- 2026-11-12T00:06:24+01:00 — Agent 20261111-chase-F (traffic car push and cop whistle): read and ported the push of a character by a car, the characters' message 45 and the cop whistle state with the clock unit; not connected to the game, push geometry and squash left open; see [pedestrians-traffic-cars-driver.md](features/pedestrians-traffic-cars-driver.md).
- 2026-11-11T23:56:44+01:00 — Agent 20261111-chase-E (cutscenes 1_4_c and 1_4_e): both play with the cutscene natives and their port update counts are pinned; the oracle comparison is requested and open, see [features/script-system-missions-1-4.md](features/script-system-missions-1-4.md)
- 2026-11-12T00:17:11+01:00 — Agent 20261111-chase-G (traffic brake lights and culling bit): read the writers of the brake light parts and glow effect and who sets node bit 18, ported the part show, the glow start and the view-volume culling; the brake lights and the culling bit are done, with stand-ins. [pedestrians-traffic-cars-driver](features/pedestrians-traffic-cars-driver.md)
