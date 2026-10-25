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

**Priorities (critical path), updated 2026-10-19T19:49:14+02:00:** oracle states for every mission 1-2 step: the save state in front of the mission 1-2 start trigger to the save state after mission 1-2, the save state at the foot of the drain pipe of mission 1-2, the save state beside the drain pipe of mission 1-2, the save state in front of the table of mission 1-2, the save state in front of the broken table of mission 1-2, the save state at the built trampoline of mission 1-2, vehicle effects: the save state in a police van in front of a lamp post, the save state in a police van in front of moving cars, the save state in a police van with one heart left, the save state in a police van at full speed towards a brick wall (local state catalogue next to the states); more on request from the user (new state requests deferred by the user on 2026-10-18). Rounds run three workers (user, 2026-10-18: short rounds); [priorities.md](priorities.md) ranks open items by their reach in the whole game and, when two need `--rpc`, one emulator servant that owns the only `--rpc` session.
1. Mission 1-3 (save state on the roof in front of the super brick of mission 1-3 to the save state on the fast travel pad of the built helipad, local state catalogue): the super brick pickup, the ladder and the helipad build are on main and not yet played by the user; next the super brick HUD display and flight (HUD-72, HUD-73, STUD-18, [studs](features/studs.md)), the count loaded from the save (STUD-17), the helipad's fast travel pad and menu (LEVOBJ-118, [level-objects-buildit](features/level-objects-buildit.md)), the rest of the super build-it class (LEVOBJ-117); the scanner's info boxes (SCAN-12, [abilities-scanner](features/abilities-scanner.md)); an oracle picture comparison of the objective markers and arrows (SCRIPT-330, needs `--rpc`, [script-system-hud-natives](features/script-system-hud-natives.md)). `hud-ui.md` is at the 16 KB limit: split it before adding HUD items.
2. One worker per round on a wide-reach item from [priorities.md](priorities.md), whether or not mission 1-2 needs it. The trampoline's look and the table's wobble (LEVOBJ-S77, LEVOBJ-S79, LEVOBJ-90, [level-objects-wobbly](features/level-objects-wobbly.md)); the remaining callers of dropped studs (STUD-14, STUD-15, [studs](features/studs.md)); the other map icons (HUD-60, [hud-ui](features/hud-ui.md)); the player script clip's request timing and first pose (SCRIPT-333, SCRIPT-S272, [script-system-natives-animation](features/script-system-natives-animation.md)); the mission 1-2 start one update late (SCRIPT-114, [script-system-missions](features/script-system-missions.md)); the waypoint disable fade (SCRIPT-S269, [script-system-hud-natives](features/script-system-hud-natives.md)); joints of named model nodes (MOVIE-40, [movies-cutscenes](features/movies-cutscenes.md)); saves: the level block refresh (SAVE-14) and live summary inputs (SAVE-15, [saves-options](features/saves-options.md)); oracle checks of the zipline A press and the pole A press (TRAV-8, TRAV-34, need user states).
3. The dog and the dog truck: animals are not created (LEVOBJ-7), the truck's moving bound and driving (SCRIPT-77, SCRIPT-43); cutscene 1_2_c and the mission end (SCRIPT-115) ([script-system-missions](features/script-system-missions.md)).
4. Open mission 1-1 gaps: the rest of the vehicle effects (VEH-121, VEH-123, VEH-124, VEH-128, PART-2), the studs and camera of the build (LEVOBJ-76, LEVOBJ-77), shadows (RENDER-33, RENDER-9), traffic and pedestrians (SCRIPT-230), camera classes (CAMERA-40, CAMERA-52).
5. Later: HUD fidelity, saves (FE-30, SAVE-14, SAVE-15), level switch screens (TRANS-13, TRANS-14), streaming (WSTREAM-30, TRANS-3), death and respawn (PLAYER-11), rendering (RENDER-60).

Oracle states: the list of states, their game progress and positions is the local catalogue next to the states (not in the repository); the tool and the console mode of states: `docs/re/oracle.md`, [oracle-setup.md](../re/oracle-setup.md). Hard-to-reach states: ask the user (CLAUDE.md "Runtime oracle").

**Workflow notes:** oracle route and states: `tools/azahar_oracle.py` with `--display xvfb --state gameplay-start --renderer opengl --rpc --rpc-input`, route from boot `tools/oracle/to-gameplay.txt`; only one agent may use `--rpc` at a time. The lead delegates to subagents in worktrees (at most 3 in parallel, user), every brief names the CLAUDE.md rules, reviews each result against CLAUDE.md, rebuilds and tests, squash-merges one commit per branch with `tools/merge_agent.py prepare|check|finish` (it removes the worktree and branch), pushes when authorized, and checks the Graphify rebuild. Agents keep scratch files in their own subdirectory of the shared scratchpad. Worktree agents link `artifacts/` into their worktree (CLAUDE.md "Subagents"). Resume a branch with `git log main..<branch>`.

## Session Log

The newest 20 lines only; `python3 tools/check_doc_links.py --fix` moves older lines unchanged to [linux-port.progress-archive.md](linux-port.progress-archive.md). One line per session: timestamp, what was worked on, how far it got, link to the feature file. Findings, open items and stand-ins do not belong here (CLAUDE.md "Feature files"). Reverse-engineering details are in docs/re/ (see docs/re/README.md).

- 2026-10-21T22:47:32+02:00 — Agent 20261021-audio-C (ladder sounds): the climb, dismount and clamber sounds now play from the animation sound tracks of the player's clips (`ClipSounds`); not compared with the oracle (TRAV-61, TRAV-62); see [ladders](features/player-traversal-ladders.md)
- 2026-10-21T22:23:27+02:00 — Agent 20261021-audio-B (sound owner key): script objects and doors take their sound owner from the object loader's entity id (runtime::SceneEntityIds); the kind-10 sound object stop is not ported, and the check of the ids against the emulator is still open. See [audio](features/audio.md)
- 2026-10-21T22:23:05+02:00 — Agent 20261021-audio-A (voice volume and rate): read the original's voice volume and rate path and ported it into chase_audio, with the tests voice-params and voice-params-real-data; the unread part of the path stays open. [audio.md](features/audio.md)
- 2026-10-21T22:59:49+02:00 — Lead (20261021-audio end): merged 20261021-audio-A (voice volume and pitch), 20261021-audio-B (sound owner entity ids, partial), 20261021-audio-C (ladder and clip sounds, partial) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [audio](features/audio.md), [player-traversal-ladders](features/player-traversal-ladders.md); user docs audit: declined
- 2026-10-25T11:36:10+01:00 — Agent 20261025-mission13-D (oracle touch): added the script steps touch and drag, sent through the DSU motion server as a touch point; the game's HID touch entry holds exactly the pixel for every x and y swept, with no RPC and no memory writes. [viewer-tools.md](features/viewer-tools.md)
- 2026-10-25T11:24:23+01:00 — Agent 20261025-mission13-A (fast travel exit and arrival): read the map's exit, the takeoff and the arrival state 0x183 and ported them; the exit is started by a click on the exit button. [level-objects-fast-travel.md](features/level-objects-fast-travel.md)
- 2026-10-25T11:29:32+01:00 — Agent 20261025-mission13-B (reference trigger): the step 0x23211c is ported in full (activate and deactivate, owner events ObjectSwitched / ObjectUnswitched, messages 10 / 0xff / 0xfe, nav node requests) with one list of the 18 kinds that register records, and the build-it runs on it; the switch classes, destroy, doors, platforms, topples and spawners that call it are open. See [script-system-missions](features/script-system-missions.md)
- 2026-10-25T11:14:59+01:00 — Agent 20261025-mission13-C (super brick HUD after a build): the HUD's own shown count is lowered by the helipad build, the count display stays while the super build-it prompt is shown, and the spin and tint phase are carried across level loads; read the origins of the tint hook, spin, update count and camera shift. See [hud-ui-super-bricks](features/hud-ui-super-bricks.md), [studs-super-bricks](features/studs-super-bricks.md)
- 2026-10-25T11:56:24+01:00 — Lead (20261025-mission13 end): merged 20261025-mission13-C (super brick HUD after a build), 20261025-mission13-D (oracle touch and drag steps, added mid-round on user request), 20261025-mission13-A (fast travel map exit and takeoff; arrival ported, trip open) and 20261025-mission13-B (reference-trigger step, door message dispatch) through tools/merge_agent.py with all tests passing; servant added mid-round for A-1; ID ranges released in the merge commits; pushed main to origin/main after each merge. See docs/plans/features/hud-ui-super-bricks.md, docs/plans/features/level-objects-fast-travel.md, docs/plans/features/script-system-missions.md, docs/plans/features/viewer-tools.md
- 2026-10-25T12:29:33+01:00 — Agent 20261025-traffic-B (traffic AI driver): read and ported the driver record, every state of the input provider 0x26d934, the state change 0x1cb148 (who moves +0x78) and the post step 0x1bc79c, and read how traffic cars become kinematic; the functions they call (route mover, steering, junction choice) are open and nothing is wired into chase-game yet. See [pedestrians-traffic-cars](features/pedestrians-traffic-cars.md)
- 2026-10-25T12:22:23+01:00 — Agent 20261025-traffic-A (traffic routes): the junction route list, EnableTrafficPath and its pending list, the path follower 0x2bf2ec, the route type function, the junction pairs and the junction test 0x2cdde4 read and ported as `traffic_route.*` with tests `traffic-route` and `traffic-route-real-data`; not wired. [pedestrians-traffic-cars](features/pedestrians-traffic-cars.md)
- 2026-10-25T12:40:23+01:00 — Agent 20261025-traffic-D (local-data cleanup): machine-only wording, scratch folder and trace names and save state names removed from the user docs, docs/re, feature files and test comments (provenance says what was recorded from a save state, in game terms); the oracle requirements listed; the oracle tool names a missing RPC server; check_local_data.py now rejects scratch names, distro paths and loaded state names no script saves, and warns about machine wording and user names; CLAUDE.md limit raised to 23 KiB (user approved) → [viewer-tools](features/viewer-tools.md)
- 2026-10-25T12:45:20+01:00 — Agent 20261025-traffic-C (traffic in chase-game): the traffic manager runs in chase-game through `TrafficSystem` and `game_traffic.*`: cars spawn at pregen points around the player, stand still and leave again, and EnableTrafficAndPedestrians and SetTrafficMaxVehicles reach it; the spawn height, direction, LOD class, removal radius and driver steps are read from the code and the oracle; the traffic floor cell 0x1edaac is ported (PREGEN-3). [pedestrians-traffic-cars](features/pedestrians-traffic-cars.md)
- 2026-10-25T13:19:01+01:00 — Agent 20261025-traffic-R (recorder): recorded the user's decision on the oracle scripts' state names and the servant's fade point and pedestrian byte readings; done. [viewer-tools](features/viewer-tools.md), [pedestrians-traffic-pedestrians](features/pedestrians-traffic-pedestrians.md)
- 2026-10-25T13:52:08+01:00 — Lead (20261025-traffic end): merged 20261025-traffic-A (traffic route follower), 20261025-traffic-B (AI driver input), 20261025-traffic-D (user- and machine-specific data out of the repo, stricter no-local-data check), 20261025-traffic-C (traffic cars spawn and leave in chase-game) and the recorder (TOOLS-20, PED-13/PED-16 results) through tools/merge_agent.py with all tests passing; D and C merged without re-review of their last commits at the user's request; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [pedestrians-traffic-cars.md](features/pedestrians-traffic-cars.md), [viewer-tools.md](features/viewer-tools.md)
- 2026-10-25T13:56:55+01:00 — Lead (recorded): the user's mission 1-3 report (the key cannot be picked up in the port) is recorded; open item SCRIPT-361 in [script-system-missions](features/script-system-missions.md), entry in [playtests.md](../re/playtests.md#ts-20261025-135615).
- 2026-10-25T14:40:35+01:00 — Agent 20261025-recorded-C (mission 1-3 key): the key is picked up in `chase-game` with its carry arrow moving to the keyhole as in the oracle; script-system-missions split; key use, parts crate and generator open. [script-system-missions-1-3](features/script-system-missions-1-3.md)
- 2026-10-25T14:15:58+01:00 — Agent 20261025-recorded-A (oracle state roles): oracle scripts load recorded states by role (`loadstate @ROLE`, `--state-param ROLE=NAME`, checked before the emulator starts), 26 scripts converted, `check_local_data.py` reads `tools/oracle`; the converted scripts are not yet run in the emulator. See [viewer-tools](features/viewer-tools.md).
- 2026-10-25T14:20:45+01:00 — Agent 20261025-recorded-B (pedestrian fade point): the pedestrian fade point is now the camera look point; its writer, the third layer table push and the node type read from the code; servant readings added; PED-13 closed, PED-16 narrowed, PED-27..29 and PED-S15/S16 opened. [pedestrians-traffic-pedestrians](features/pedestrians-traffic-pedestrians.md)
- 2026-10-25T14:54:29+01:00 — Lead (20261025-recorded end): merged 20261025-recorded-A (oracle scripts take save states by role, TOOLS-20), 20261025-recorded-B (pedestrian fade point is the camera look point, PED-13; PED-16 narrowed), 20261025-recorded-C (mission 1-3 key pickup, SCRIPT-361) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [viewer-tools.md](features/viewer-tools.md), [pedestrians-traffic-pedestrians.md](features/pedestrians-traffic-pedestrians.md), [script-system-missions-1-3.md](features/script-system-missions-1-3.md)
