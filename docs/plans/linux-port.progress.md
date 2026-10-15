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

**Priorities (critical path), updated 2026-10-15T10:01:37+02:00:** done for mission 1-1: waypoint class, in-engine cutscenes with the level-start intro and fades, cutscene actors with their looks, build-it, carrying, vehicles created and solid, collision of objects enabled during play, getting into and out of vehicles, driving with a stand-in body, the director camera holding the player, NPC idle animation. Oracle states before every step: `m11-1` to `m11-9` (docs/re/oracle-states.md).
1. Vehicles on the mission path: vehicle waypoints (VEH-11, SCRIPT-141), jump out natives (VEH-9), vehicle camera (VEH-8, needs CAMERA-9), driving collision (VEH-7), then the physics body (VEH-6): [vehicles](features/vehicles.md).
2. Original comparison pictures that need an RPC slot (COLL-21, MOVIE-21, ANIM-6).
3. Class-fired triggers and their call paths (SCRIPT-111, SCRIPT-170, SCRIPT-180), `SetPartyCharacter` (SCRIPT-35), music (SCRIPT-33), the director camera's other readers (SCRIPT-181, SCRIPT-182).
4. Polish of the finished steps: carry and build-it stand-ins (level-objects-carry.md, level-objects-buildit.md), the police car start state (VEH-13), the enter prompt and vehicle HUD (VEH-17, VEH-18), animation blending (ANIM-3).
5. After mission 1-1: HUD fidelity, saves (FE-30, SAVE-14, SAVE-15), level switch screens (TRANS-13, TRANS-14), streaming (WSTREAM-30, TRANS-3), camera and rendering (CAMERA-10, RENDER-1, RENDER-9).

Oracle states (artifacts/oracle/states, see docs/re/oracle.md): `gameplay-start` (new game, Old 3DS), `police-station` and `street-after-1-1` (user saves after mission 1-1, New 3DS), `street-at-entrance`, `station-at-exit`, `station-exit-loadingpage`, `camera-lobby-walk`, `dcam-police-dialog`, `agent-doughnut-title` (agents, unmodified, New 3DS), `savecont-slotlist-user` (slot list with the user's save, Old 3DS), `trans-poked-*` (poked, measuring aids only). The real save of the user (read-only copy): `artifacts/oracle/agentJ-saves/savegame.dat`. Hard-to-reach states: ask the user (CLAUDE.md "Runtime oracle").

**Workflow notes:** oracle route and states: `tools/azahar_oracle.py` with `--display xvfb --state gameplay-start --renderer opengl --rpc --rpc-input`, route from boot `tools/oracle/to-gameplay.txt`; only one agent may use `--rpc` at a time. The lead delegates to subagents in worktrees (at most 3 in parallel, user), every brief names the CLAUDE.md rules, reviews each result against CLAUDE.md, rebuilds and tests, squash-merges one commit per branch with `tools/merge_agent.py prepare|check|finish` (it removes the worktree and branch), pushes when authorized, and checks the Graphify rebuild. Agents keep scratch files in their own subdirectory of the shared scratchpad. Worktree agents link `artifacts/` into their worktree (CLAUDE.md "Subagents"). Resume a branch with `git log main..<branch>`.

## Session Log

The newest 20 lines only; `python3 tools/check_doc_links.py --fix` moves older lines unchanged to [linux-port.progress-archive.md](linux-port.progress-archive.md). One line per session: timestamp, what was worked on, how far it got, link to the feature file. Findings, open items and stand-ins do not belong here (CLAUDE.md "Feature files"). Reverse-engineering details are in docs/re/ (see docs/re/README.md).

- 2026-10-14T21:42:00+02:00 — Agent N (mission-1-2): mission 1-2 gap analysis (13 of 65 functions without handler, 12 of 22 triggers class-fired, waypoint class starts MissionStart) with an oracle executor trace (`ctrace`) of the start, batch SCRIPT-32 (13 animation and attach natives) as stand-ins; [script-system-missions.md](features/script-system-missions.md), [script-system-natives.md](features/script-system-natives.md).
- 2026-10-14T21:42:55+02:00 — Agent M (oracle-visual): HUD-31 closed (test set-up artefacts, no port change), RENDER-41 closed (pop-in faint or off-screen, oracle shots in a free run), RENDER-30 narrowed (pose compared, RENDER-42 near-ground lighting offset); breakpoint-free stick, `shotloop`, `--hide-room` → [rendering.md](features/rendering.md)
- 2026-10-14T22:49:18+02:00 — Agent P (waypoint-class): waypoint class (init 0x17aa34, update 0x17acbc) ported in src/runtime/waypoints.*: TriggerBound cylinder hit, script thread 14 updates later, self switch-off, TakePlayerControl; oracle hit update 36 / start update 50 reproduced; SCRIPT-110 closed, SCRIPT-141/142 open; [feature file](features/script-system-missions.md)
- 2026-10-14T22:50:12+02:00 — Agent Q (cutscenes-in-game): DCamCutScene natives, PlayCutscene and in-game cutscene and movie playback; `1_1_b` compared with the oracle; MOVIE-2 closed, MOVIE-10 to MOVIE-13 and SCRIPT-150 open → [movies-cutscenes.md](features/movies-cutscenes.md)
- 2026-10-14T22:54:16+02:00 — Agent R (vehicles-survey): vehicle system mapped (class 183, handling data in class files, update chain, enter via X / event 200 / states 0x11a, 0x11c, natives of mission 1-1 are flag writes), oracle-confirmed; 12 ordered items VEH-1..12 with sizes → [vehicles](features/vehicles.md), [vehicles-system.md](../re/vehicles-system.md#ts-20261014-224438)
- 2026-10-14T23:52:19+02:00 — Agent S (new-game-start): startup mission choice per StoryProgress pinned by test; level start plays intro movie and 1_1_a (MOVIE-13), cutscene fades measured in the oracle and drawn (MOVIE-10); MOVIE-20, SCRIPT-160 open → [movies-cutscenes.md](features/movies-cutscenes.md)
- 2026-10-15T00:00:07+02:00 — Agent V (build-it): build-it class (type 0x15) read and measured in the oracle (state m11-4-van-parts: 12 updates per brick, six bricks, built 85 updates after the first A press, only A starts it, a started brick finishes without the button); ported in src/runtime/buildit*.cpp: use zone, state machine, final prop shown and hidden parts, ObjectTriggered of TriggerObject queued → 1-1_SO_VanFixed runs; LEVOBJ-15 closed, LEVOBJ-40..43 and SCRIPT-180 open → [level-objects-buildit](features/level-objects-buildit.md)
- 2026-10-15T00:01:08+02:00 — Agent U (vehicles-1): vehicle records, wheels, collision item (mask 6) and the five mission 1-1 flag natives; oracle-checked records and grid entries; VEH-1..3, COLL-10, LEVOBJ-20 closed, VEH-13..16 open → [vehicles](features/vehicles.md)
- 2026-10-15T00:18:34+02:00 — Agent T (carrying): carry objects (type 122) and targets (type 123) read from the code and measured in the oracle (m11-7/8/9: A pickup, use zones, place release 8 updates, target effect at update 9); `CarrySet` + glue, mission 1-1 steps 9 and 10 run in a real-data test; LEVOBJ-14 closed, LEVOBJ-30..34 and stand-ins S30..S34 open, SCRIPT-170 → [level-objects-carry](features/level-objects-carry.md)
- 2026-10-15T07:56:00+02:00 — Agent B (cutscene-actors): the original cutscene cast are the level's own figures (`CSCharacter` parameter, `Player1.Player01`), found in 0x2d1ddc; chase-game draws actors as copies of those characters and hides the originals, so Chase and the donut man have the right look and appear once (MOVIE-11, MOVIE-14 closed; MOVIE-21, MOVIE-22 open; original pictures not compared) → [movies-cutscenes.md](features/movies-cutscenes.md)
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
