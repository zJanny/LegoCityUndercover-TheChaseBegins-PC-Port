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

**Goal of the current rounds (user, 2026-10-18):** mission 1-2 more playable in `chase-game`. Mission 1-1 is playable end to end (user playthrough, [scripts-missions.md §2026-10-18T10:37](../re/scripts-missions.md#ts-20261018-103746)); the mission 1-2 gap list is linked from [script-system-missions](features/script-system-missions.md).

**Priorities (critical path), updated 2026-10-18T16:43:21+02:00:** oracle states for every mission 1-2 step: `m12-1` to `m12-14`, `m12-10a-pole-foot`, `m12-10b-pole-side`, vehicle effects: `veh-smash-prop`, `veh-smash-car`, `veh-break-low-hearts`, `veh-wall-full-speed` (docs/re/oracle-states.md); more on request from the user. Rounds run up to three workers and, when two need `--rpc`, one emulator servant that owns the only `--rpc` session.
1. The table and the trampoline of mission 1-2 (before the zipline on the route): an attack on foot (PLAYER-12), the destroy on foot of `leprop_wobblydestructible` and the bouncer `leprop_bouncer` (LEVOBJ-86), cars breaking the street lamps (VEH-103, [vehicles](features/vehicles.md)) ([player-movement](features/player-movement.md), [level-objects](features/level-objects.md)).
2. The magnifying glass of detective mode (DETECT-7); oracle checks of the zipline A press and the pole A press (TRAV-8, TRAV-34, need user states).
3. The dog and the dog truck: animals are not created (LEVOBJ-7), the truck's moving bound and driving (SCRIPT-77, SCRIPT-43); the unnamed scripts (SCRIPT-113); cutscene 1_2_c and the mission end (SCRIPT-115) ([script-system-missions](features/script-system-missions.md)).
4. Open mission 1-1 gaps: the rest of the vehicle effects (VEH-121, VEH-123, VEH-124, VEH-128, PART-2), the studs and camera of the build (LEVOBJ-76, LEVOBJ-77), shadows (RENDER-33, RENDER-9), traffic and pedestrians (SCRIPT-230), camera classes (CAMERA-40, CAMERA-52).
5. Later: HUD fidelity, saves (FE-30, SAVE-14, SAVE-15), level switch screens (TRANS-13, TRANS-14), streaming (WSTREAM-30, TRANS-3), death and respawn (PLAYER-11), rendering (RENDER-60).

Oracle states (artifacts/oracle/states, see docs/re/oracle.md): `gameplay-start` (new game, Old 3DS), `police-station` and `street-after-1-1` (user saves after mission 1-1, New 3DS), `street-at-entrance`, `station-at-exit`, `station-exit-loadingpage`, `camera-lobby-walk`, `dcam-police-dialog`, `agent-doughnut-title` (agents, unmodified, New 3DS), `savecont-slotlist-user` (slot list with the user's save, Old 3DS), `trans-poked-*` (poked, measuring aids only). The real save of the user (read-only copy): `artifacts/oracle/agentJ-saves/savegame.dat`. Hard-to-reach states: ask the user (CLAUDE.md "Runtime oracle").

**Workflow notes:** oracle route and states: `tools/azahar_oracle.py` with `--display xvfb --state gameplay-start --renderer opengl --rpc --rpc-input`, route from boot `tools/oracle/to-gameplay.txt`; only one agent may use `--rpc` at a time. The lead delegates to subagents in worktrees (at most 3 in parallel, user), every brief names the CLAUDE.md rules, reviews each result against CLAUDE.md, rebuilds and tests, squash-merges one commit per branch with `tools/merge_agent.py prepare|check|finish` (it removes the worktree and branch), pushes when authorized, and checks the Graphify rebuild. Agents keep scratch files in their own subdirectory of the shared scratchpad. Worktree agents link `artifacts/` into their worktree (CLAUDE.md "Subagents"). Resume a branch with `git log main..<branch>`.

## Session Log

The newest 20 lines only; `python3 tools/check_doc_links.py --fix` moves older lines unchanged to [linux-port.progress-archive.md](linux-port.progress-archive.md). One line per session: timestamp, what was worked on, how far it got, link to the feature file. Findings, open items and stand-ins do not belong here (CLAUDE.md "Feature files"). Reverse-engineering details are in docs/re/ (see docs/re/README.md).

- 2026-10-18T09:44:27+02:00 — Agent r18-C (round 18, PART-1): particle runtime ported from the read start entry, update, init, step, stop and quad builders (`src/runtime/particles.*`), drawn in `chase-game` with the definition's blend (`src/render/particles.*`, `--particle` development option); init matches an oracle siren-light emitter bit for bit, emitter limit is 25; PART-1 closed, callers PART-2 and PART-3 open — [particles.md](features/particles.md)
- 2026-10-18T10:13:45+02:00 — Lead (mission 1-2 states): the user's eight step states of mission 1-2 copied read-only as `m12-1-mission-trigger` to `m12-8-roof-sound-scanner`, each loaded once, recorded with positions and the user's note on the white sound scanner view; SCRIPT-115 narrowed — [script-system-missions.md](features/script-system-missions.md)
- 2026-10-18T11:28:45+02:00 — Agent r19-C (round 19, sound scanner): audio scanner zone, scannables, enter / view / zoom / lock / scan / exit and their ObjectTriggered events ported (`src/runtime/scanner.*`, `src/game_scanner.cpp`), tests `scanner` and `scanner-real-data`; view look and real-time timing open: [abilities-scanner.md](features/abilities-scanner.md).
- 2026-10-18T11:15:40+02:00 — Agent r19-B (round 19, mission 1-2 natives): DestroyProp / KillObject, EffectFadeObject, EnableDistanceFade, AddToSemiTransNearCameraList, SetPartyCharacter and StreamRoomIn run from the read handlers (the port run of mission 1-2 no longer reports them), the room request reaches the streamer, SCRIPT-58 closed, SCRIPT-113 narrowed (names not found in any text), RENDER-66 / 67 opened for the distance fade found on the way; see [script-system-natives-world.md](features/script-system-natives-world.md)
- 2026-10-18T10:38:02+02:00 — Lead (SCRIPT-200): the user played mission 1-1 end to end in `chase-game`; recorded and SCRIPT-200 closed — [script-system-missions.md](features/script-system-missions.md)
- 2026-10-18T10:41:22+02:00 — Lead (LEVOBJ-72): the user confirmed the look of the donut van build in `chase-game`; LEVOBJ-72 closed, its open switch-time comparison is LEVOBJ-85 — [level-objects-buildit.md](features/level-objects-buildit.md)
- 2026-10-18T10:43:27+02:00 — Agent r18-A (round 18, oracle measurements): VEH-111 closed (stuck-timer trigger traced), LEVOBJ-74 closed and ported (`carryTiming`, LEVOBJ-S30 and LEVOBJ-S72 removed, carrying findings split), LEVOBJ-78 closed (port already matched; LEVOBJ-79 opened), VEH-102 narrowed (no sphere contact at the pillar) — [level-objects-carry.md](features/level-objects-carry.md), [vehicles-damage.md](features/vehicles-damage.md), [level-objects-buildit.md](features/level-objects-buildit.md), [vehicles-physics-collision.md](features/vehicles-physics-collision.md)
- 2026-10-18T11:26:21+02:00 — Agent r19-A (round 19, detective tool): detective spot, reveal spot and reveal prop ported with oracle-pinned timing; from the second detective point chase-game reaches 1-2_SO_Dspot_02_X; first point blocked by the Doorlock script (DETECT-2) — [abilities-detective.md](features/abilities-detective.md)
- 2026-10-18T11:36:58+02:00 — Lead (round 19 end): r18-A, r19-B (mission 1-2 world natives), r19-A (detective tool) and r19-C (sound scanner) merged; SCRIPT-111 narrowed to the cage use spot and the door lock hint; stale index rows refreshed — [script-system-missions.md](features/script-system-missions.md), [README.md](features/README.md)
- 2026-10-18T14:29:15+02:00 — Agent r20-B (round 20, Doorlock script and cage use spot): DETECT-2 closed (PlayCharAnim state 0x50 keeps the idle away, the Doorlock script ends, the first detective point is enabled), `prop_simpleuse` ported and confirmed in the oracle, SCRIPT-111 closed with SCRIPT-284..286 open — [abilities-detective.md](features/abilities-detective.md), [script-system-missions.md](features/script-system-missions.md)
- 2026-10-18T14:52:18+02:00 — Agent r20-C (round 20, vehicle brick effects): brick debris of vehicle contacts, destroyed props and the car break, the hit flash and the vehicle smoke ported; brick motion and flash colours pinned to oracle traces; VEH-112 and VEH-104 closed, PART-2 and VEH-121 narrowed — [vehicles-damage.md](features/vehicles-damage.md)
- 2026-10-18T14:56:25+02:00 — Agent r20-A (round 20, climbing moves of mission 1-2): new feature player-traversal; zipline ported from the original (grab, slide, dismount) with the m12-9 ride pinned in tests; pole read, pole, bricks, climb wall, rails and ledge surveyed as TRAV-1..TRAV-4 — [player-traversal.md](features/player-traversal.md)
- 2026-10-18T15:06:39+02:00 — Lead (round 20 end): r20-B (Doorlock scene, cage use spot), r20-C (vehicle brick debris, flash, smoke) and r20-A (zipline, climbing survey) merged; user state `veh-wall-full-speed` recorded, VEH-128 narrowed, SCRIPT-287 opened from the lead's check of the Doorlock scene — [player-traversal.md](features/player-traversal.md), [vehicles-damage.md](features/vehicles-damage.md), [script-system-missions.md](features/script-system-missions.md)
- 2026-10-18T15:59:19+02:00 — Agent r21-B (round 21, climb wall): the m12-11 climb identified as the climbing bricks (states 141..152), ported with grab, reach, jump, drop and jump-off and pinned to oracle runs; climb wall entry found, TRAV-2 narrowed to the wall, TRAV-19..22 and TRAV-S10..S13 opened — [player-traversal.md](features/player-traversal.md)
- 2026-10-18T16:06:52+02:00 — Agent r21-C (round 21, climb bars): rails and the corner rail of mission 1-2 ported (grab, arrival, shuffle with clip root motion, corner spline, A drop, B jump) and replayed from m12-12; ledge states 23 / 24 narrowed to the `Ledge` bar variant; TRAV-3 and TRAV-4 closed, variants TRAV-27..33 — [player-traversal](features/player-traversal.md)
- 2026-10-18T16:10:43+02:00 — Agent r21-A (round 21, pole): pole and drain pipe ported (walk-in and A-press mount, climb, exits, dismount) and pinned against m12-10 / m12-10a; zipline use press found to be the A press (TRAV-12); follow-ups TRAV-11..TRAV-16 — [player-traversal.md](features/player-traversal.md)
- 2026-10-18T16:18:22+02:00 — Lead (round 21 end): r21-B (climbing bricks), r21-C (rails, climb bars, corner rail) and r21-A (pole and drain pipe, walk-in mount) merged; the player step chains rails, bricks and pole, the jump off the last brick grabs the rail as in the oracle (TRAV-30 closed, test `game-script-brick-rail`); traversal docs split by subtopic; user pole states verified — [player-traversal.md](features/player-traversal.md), [player-traversal-pole.md](features/player-traversal-pole.md), [player-traversal-rails.md](features/player-traversal-rails.md)
- 2026-10-18T16:38:47+02:00 — Agent r22-C (round 22, zipline A press, PositionObject): the zipline use press moved from B to A (TRAV-12 closed, follow-ups TRAV-34, TRAV-35) and PositionObject now moves level characters, so Holmes ends at the Doorlock teleport (SCRIPT-287 closed, SCRIPT-288 open); both checked in tests, Holmes also against the oracle by eye — [player-traversal.md](features/player-traversal.md), [script-system-missions.md](features/script-system-missions.md)
- 2026-10-18T16:43:21+02:00 — Lead (round 22 end): r22-C merged (zipline get-on with A, TRAV-12 closed; PositionObject places level characters, Holmes leaves the bin, SCRIPT-287 closed; checked by the lead on the user's real path from their save); the street lamp report corrected to cars passing through (VEH-103) — [player-traversal.md](features/player-traversal.md), [script-system-missions.md](features/script-system-missions.md)
- 2026-10-18T19:30:29+02:00 — Agent r23-C (round 23, detective glass): Chase's magnifying glass is drawn in detective mode from the original's data (hand item track 0xba1e5b53 names Ability_MagnifyingGlass and weaponRight; first shown 14 updates after the A press as in the oracle), DETECT-7 closed, look differences listed (DETECT-9..11) and hand item follow-ups DETECT-8, DETECT-12, DETECT-13 opened — [abilities-detective.md](features/abilities-detective.md)
