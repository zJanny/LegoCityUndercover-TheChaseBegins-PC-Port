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

Read this section, CLAUDE.md (incl. "Plans and order of work" and "Feature files"), [docs/rules/port.md](../rules/port.md), [features/README.md](features/README.md) and, before any emulator use, docs/re/oracle.md. Open items, stand-ins and findings are in the feature files, not here. Feature layer introduced 2026-10-13T21:34:59+02:00; earlier handoff 2026-10-13T17:53:20+02:00.

**Goal (user, 2026-10-13):** faithful PC port, playable start to finish, Linux first; Windows only when the user says so; QoL only after the PC version is finished (Decisions, CLAUDE.md).

**State:** M1 runtime loop, M2 locomotion, M3 walkable level, M4 level objects, characters, studs, falling, jumping and object collision are on main; the front end (boot to the first level) and the sound hooks are merged. `chase-game` is a development runtime, not the game ([docs/status.md](../status.md)).

**Goal of the current rounds (user, 2026-10-19):** mission 1-3 next, unless the user reports more critical mission 1-2 bugs. Mission 1-1 is playable end to end (user playthrough, [scripts-missions.md §2026-10-18T10:37](../re/scripts-missions.md#ts-20261018-103746)); mission 1-2 was played through by the user ([playtests.md](../re/playtests.md)) and its four playtest bugs are fixed on main, not yet re-checked by the user; the mission 1-2 gap list is linked from [script-system-missions](features/script-system-missions.md).

**Priorities (critical path), updated 2026-10-19T19:49:14+02:00:** oracle states for every mission 1-2 step: `m12-1` to `m12-14`, `m12-10a-pole-foot`, `m12-10b-pole-side`, `m12-table-before`, `m12-table-broken`, `m12-trampoline-built`, vehicle effects: `veh-smash-prop`, `veh-smash-car`, `veh-break-low-hearts`, `veh-wall-full-speed` (docs/re/oracle-states.md); more on request from the user (new state requests deferred by the user on 2026-10-18). Rounds run three workers (user, 2026-10-18: short rounds); [priorities.md](priorities.md) ranks open items by their reach in the whole game and, when two need `--rpc`, one emulator servant that owns the only `--rpc` session.
1. Mission 1-3 (states `m13-1` to `m13-5`, [oracle-states.md](../re/oracle-states.md)): the super brick pickup, the ladder and the helipad build are on main and not yet played by the user; next the super brick HUD display and flight (HUD-72, HUD-73, STUD-18, [studs](features/studs.md)), the count loaded from the save (STUD-17), the helipad's fast travel pad and menu (LEVOBJ-118, [level-objects-buildit](features/level-objects-buildit.md)), the rest of the super build-it class (LEVOBJ-117); the scanner's info boxes (SCAN-12, [abilities-scanner](features/abilities-scanner.md)); an oracle picture comparison of the objective markers and arrows (SCRIPT-330, needs `--rpc`, [script-system-hud-natives](features/script-system-hud-natives.md)). `hud-ui.md` is at the 16 KB limit: split it before adding HUD items.
2. One worker per round on a wide-reach item from [priorities.md](priorities.md), whether or not mission 1-2 needs it. The trampoline's look and the table's wobble (LEVOBJ-S77, LEVOBJ-S79, LEVOBJ-90, [level-objects-wobbly](features/level-objects-wobbly.md)); the remaining callers of dropped studs (STUD-14, STUD-15, [studs](features/studs.md)); the other map icons (HUD-60, [hud-ui](features/hud-ui.md)); the player script clip's request timing and first pose (SCRIPT-333, SCRIPT-S272, [script-system-natives-animation](features/script-system-natives-animation.md)); the mission 1-2 start one update late (SCRIPT-114, [script-system-missions](features/script-system-missions.md)); the waypoint disable fade (SCRIPT-S269, [script-system-hud-natives](features/script-system-hud-natives.md)); joints of named model nodes (MOVIE-40, [movies-cutscenes](features/movies-cutscenes.md)); saves: the level block refresh (SAVE-14) and live summary inputs (SAVE-15, [saves-options](features/saves-options.md)); oracle checks of the zipline A press and the pole A press (TRAV-8, TRAV-34, need user states).
3. The dog and the dog truck: animals are not created (LEVOBJ-7), the truck's moving bound and driving (SCRIPT-77, SCRIPT-43); cutscene 1_2_c and the mission end (SCRIPT-115) ([script-system-missions](features/script-system-missions.md)).
4. Open mission 1-1 gaps: the rest of the vehicle effects (VEH-121, VEH-123, VEH-124, VEH-128, PART-2), the studs and camera of the build (LEVOBJ-76, LEVOBJ-77), shadows (RENDER-33, RENDER-9), traffic and pedestrians (SCRIPT-230), camera classes (CAMERA-40, CAMERA-52).
5. Later: HUD fidelity, saves (FE-30, SAVE-14, SAVE-15), level switch screens (TRANS-13, TRANS-14), streaming (WSTREAM-30, TRANS-3), death and respawn (PLAYER-11), rendering (RENDER-60).

Oracle states (artifacts/oracle/states, see docs/re/oracle.md): `gameplay-start` (new game, Old 3DS), `police-station` and `street-after-1-1` (user saves after mission 1-1, New 3DS), `street-at-entrance`, `station-at-exit`, `station-exit-loadingpage`, `camera-lobby-walk`, `dcam-police-dialog`, `agent-doughnut-title` (agents, unmodified, New 3DS), `savecont-slotlist-user` (slot list with the user's save, Old 3DS), `trans-poked-*` (poked, measuring aids only). The real save of the user (read-only copy): `artifacts/oracle/agentJ-saves/savegame.dat`. Hard-to-reach states: ask the user (CLAUDE.md "Runtime oracle").

**Workflow notes:** oracle route and states: `tools/azahar_oracle.py` with `--display xvfb --state gameplay-start --renderer opengl --rpc --rpc-input`, route from boot `tools/oracle/to-gameplay.txt`; only one agent may use `--rpc` at a time. The lead delegates to subagents in worktrees (at most 3 in parallel, user), every brief names the CLAUDE.md rules, reviews each result against CLAUDE.md, rebuilds and tests, squash-merges one commit per branch with `tools/merge_agent.py prepare|check|finish` (it removes the worktree and branch), pushes when authorized, and checks the Graphify rebuild. Agents keep scratch files in their own subdirectory of the shared scratchpad. Worktree agents link `artifacts/` into their worktree (CLAUDE.md "Subagents"). Resume a branch with `git log main..<branch>`.

## Session Log

The newest 20 lines only; `python3 tools/check_doc_links.py --fix` moves older lines unchanged to [linux-port.progress-archive.md](linux-port.progress-archive.md). One line per session: timestamp, what was worked on, how far it got, link to the feature file. Findings, open items and stand-ins do not belong here (CLAUDE.md "Feature files"). Reverse-engineering details are in docs/re/ (see docs/re/README.md).

- 2026-10-20T00:31:43+02:00 — Agent r40-A (round 40, TRAV-57): read the kind 56 ladder handler and the ladder states 33..37 and 23, measured mount, climb, clamber and exits from `m13-2-ladder` without RPC and ported them; Chase climbs the mission 1-3 ladder onto the roof in `chase-game`; TRAV-57 closed, TRAV-58..60 and TRAV-S34..35 opened ([player-traversal-ladders](features/player-traversal-ladders.md)).
- 2026-10-20T00:56:12+02:00 — Lead (recorded round 40 emulator servant tool findings: screenshot paths, inject_input after loadstate, short button presses). [oracle-rpc](../re/oracle-rpc.md#ts-20261020-005612)
- 2026-10-20T00:59:40+02:00 — Lead (round 40 end): merged r40-C (scanner info boxes and pause ticker), r40-B (helipad fast travel pad landing) and r40-A (ladders) through tools/merge_agent.py with all tests passing, plus the recorder's emulator RPC notes; released the ID ranges; pushed main. See [player-traversal-ladders.md](features/player-traversal-ladders.md), [level-objects-buildit.md](features/level-objects-buildit.md), [abilities-scanner.md](features/abilities-scanner.md), [ui-menus.md](features/ui-menus.md)
- 2026-10-20T09:23:33+02:00 — Agent r41-B (round 41, SCRIPT-343/114/S281): read the gameplay step 0x2e5158 and measured its order (entity pass, characters last), doors now update before the player through `UpdateSequence`, 1-2_MissionStart pinned at 153/305, cage script thread start traced. [script-system-natives-animation](features/script-system-natives-animation.md)
- 2026-10-20T09:33:36+02:00 — Agent r41-A (round 41, VEH-131): read the ObjectType 67 create, post-link collision rule and message 0 (0x15e408, hit machine 0x2398d8 with the hit sequence) and ported the hit rule and the kind 67 collision; cars now break the mode 1 kind 67 props, checked against the oracle grid census through the servant; VEH-131 stays open for an oracle smash run (needs a user state), VEH-134..136 and VEH-S107..108 opened; vehicles.md split. [vehicles-prop-smash](features/vehicles-prop-smash.md)
- 2026-10-20T09:16:44+02:00 — Agent r41-C (round 41, TRANS-11, TRANS-6): read manager slot +0x28 and the execution 0x303480 for a loaded destination; the same-level entry is ported for the lvl_00 lifts and the master objects, timing and oracle comparison still open. [level-transitions](features/level-transitions.md)
- 2026-10-20T09:53:38+02:00 — Lead (round 41 end): merged r41-C (same-level building entries), r41-B (gameplay step order, mission 1-2 start timing) and r41-A (cars smash ObjectType 67 props) through tools/merge_agent.py with all tests passing; r41-B landed under the subject of its ID-release commit dc76892 (chained command after a failed finish); released the ID ranges; pushed main. See [level-transitions.md](features/level-transitions.md), [script-system.md](features/script-system.md), [script-system-natives-animation.md](features/script-system-natives-animation.md), [vehicles-prop-smash.md](features/vehicles-prop-smash.md)
- 2026-10-20T10:41:40+02:00 — Agent r42-F (round 42): fixed flaky game-super-buildit-build; the once-per-wall-second updates/s line of scripted runs split the test's consecutive fast travel lines, so scripted runs no longer print it; test passed 20 of 20 repeats. [executable-platform.md](features/executable-platform.md)
- 2026-10-20T10:24:45+02:00 — Agent r42-A (round 42, SCRIPT-349): ported the carry-it waypoint global of the waypoint system with its disable branch and the sub-objective centroid key test; the vehicle waypoint steering is still open. [script-system-hud-natives](features/script-system-hud-natives.md)
- 2026-10-20T10:30:48+02:00 — Agent r42-B (round 42, SAVE-17/18, SAVE-S7/S9): read the writers of the cheat bytes (none besides the boot mask and the callback clear), the level load's HUD stud call 0x2e5fd8 and the pause mode's play tick 0x300444; the HUD now shows the loaded studs and the pause menu ticks play time; opened SAVE-21, SAVE-22. [saves-options](features/saves-options.md)
- 2026-10-20T10:31:56+02:00 — Agent r42-C (round 42, STUD-14, STUD-15): read which classes run the generic state machine 0x22c6d0 (ten kinds) and the stud callers of the player hurt and death, the hit-sequence slots and the ability props; request rules and class counts are pinned, STUD-14 closed, none of those classes runs in the port yet (STUD-25..27). [studs](features/studs.md)
- 2026-10-20T10:57:50+02:00 — Lead (round 42 end): merged r42-F (flaky game-super-buildit-build: rate line kept out of scripted runs), r42-A (carry-it waypoint global), r42-B (HUD studs at load, pause play tick) and r42-C (stud drop classes) through tools/merge_agent.py with all tests passing; released the ID ranges; pushed main. See [script-system-hud-natives.md](features/script-system-hud-natives.md), [saves-options.md](features/saves-options.md), [studs.md](features/studs.md)
- 2026-10-20T11:18:53+02:00 — Agent r43-A (round 43, pregen spawn files): read the traffic and pedestrian file loaders 0x2e02d4 / 0x2ee868, the cell readers and the lookup 0x1eb490; both formats decode in chase_assets and all 14 shipped files are pinned; the loader-derived fields and the managers are not wired yet (PREGEN-1, PREGEN-2). [pedestrians-traffic](features/pedestrians-traffic.md)
- 2026-10-20T11:20:14+02:00 — Agent r43-B (round 43, SCRIPT-230 pedestrian half): read the pedestrian manager 0x51b33c (level load, update 0x2ee5e8, spawn 0x2ee068, pedestriansystem step 0x2edc94) and corrected the swapped traffic and pedestrian managers; ported the spawn and removal rules as PedestrianManager (not wired); opened PED-1..3, PED-S1..2; the pedestrian walk is open (PED-2). [pedestrians-traffic-pedestrians](features/pedestrians-traffic-pedestrians.md)
- 2026-10-20T11:21:07+02:00 — Agent r43-C (round 43, SCRIPT-230 traffic half, VEH-12): found that object 0x527410 is the traffic manager and read its level load, room set-up and prefill, update, spawn, fade, SetTrafficMaxVehicles and the input provider choice; ported the manager as TrafficManager (not wired); AI driver 0x26d934 partly read; opened TRAFFIC-1..3, TRAFFIC-S1..2. [pedestrians-traffic-cars](features/pedestrians-traffic-cars.md)
- 2026-10-20T11:41:49+02:00 — Lead (round 43 end): merged r43-A (pregen traffic and pedestrian spawn file decoder), r43-B (pedestrian manager rules, not wired) and r43-C (traffic manager rules, not wired) through tools/merge_agent.py with all tests passing; released the ID ranges; pushed main. See [pedestrians-traffic.md](features/pedestrians-traffic.md), [pedestrians-traffic-pedestrians.md](features/pedestrians-traffic-pedestrians.md), [pedestrians-traffic-cars.md](features/pedestrians-traffic-cars.md)
- 2026-10-20T12:13:14+02:00 — Agent r44-B (round 44, pregen derived fields): ported the fields the pregen loaders derive per spawn point as deriveSpawnFields (path record from the loaded levels, unit tangent of the type 0 path sampler 0x1c17c0, room 0x239838) for both file kinds; all 6854 shipped points find path and room; the 72 closed-path positions off the sample deviate on one axis of the stored position, not in the sampler (PREGEN-1, PREGEN-2 closed; PREGEN-3 floor cell, PREGEN-4 level order and current room, PREGEN-5 path node open; stand-ins PREGEN-S2, S3). See [pedestrians-traffic.md](features/pedestrians-traffic.md).
- 2026-10-20T12:25:30+02:00 — Agent r44-A (round 44, PED-2): read the pedestrian character (create 0x1913f8, state step 0x1d7e88, walk 0x2b9138, avoidance 0x2edf08) and ported it as PedestrianWalk (not wired); closed PED-2, opened PED-4..6 and PED-S3..4; the steady updates of states 3, 4, 5 and 7 and the message slot are open. [pedestrians-traffic-pedestrians](features/pedestrians-traffic-pedestrians.md)
- 2026-10-20T12:29:41+02:00 — Agent r44-C (round 44, PED-3): read the source of Pedestrians_Max (the scene's level table through 0x2451d0), the scene enter that zeroes the countdown, the update's caller (system list, once per gameplay step) and the node, room and fade tier tests, and ported them in PedestrianManager (not wired); PED-3 closed, PED-7..9 and PED-S5 opened. See [pedestrians-traffic-pedestrians.md](features/pedestrians-traffic-pedestrians.md).
- 2026-10-20T12:45:27+02:00 — Lead (round 44 end): merged r44-A (pedestrian states and walk, PED-2), r44-B (pregen spawn point fields, PREGEN-1/2) and r44-C (Pedestrians_Max and spawn inputs, PED-3; returned once to drop duplicates of the walk's avoidance and token) through tools/merge_agent.py with all tests passing; released the ID ranges; pushed main. See [pedestrians-traffic-pedestrians.md](features/pedestrians-traffic-pedestrians.md), [pedestrians-traffic.md](features/pedestrians-traffic.md)
