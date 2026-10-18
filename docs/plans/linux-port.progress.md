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

**Priorities (critical path), updated 2026-10-18T22:10:30+02:00:** oracle states for every mission 1-2 step: `m12-1` to `m12-14`, `m12-10a-pole-foot`, `m12-10b-pole-side`, `m12-table-before`, `m12-table-broken`, `m12-trampoline-built`, vehicle effects: `veh-smash-prop`, `veh-smash-car`, `veh-break-low-hearts`, `veh-wall-full-speed` (docs/re/oracle-states.md); more on request from the user (new state requests deferred by the user on 2026-10-18). Rounds run three workers (user, 2026-10-18: short rounds); [priorities.md](priorities.md) ranks open items by their reach in the whole game and, when two need `--rpc`, one emulator servant that owns the only `--rpc` session.
1. Mission 1-2 playtest gaps: the flag pole jump motion, flip and idle (TRAV-39, TRAV-40, TRAV-41), then their wiring (TRAV-37, [player-traversal-rails](features/player-traversal-rails.md)); the user's look at the punch (PLAYER-19, [player-movement](features/player-movement.md)); the trampoline frame (LEVOBJ-95) and the car smash through the wobbly handler (VEH-129); the cage marker and the black screen after the dog's release (SCRIPT-292, SCRIPT-293, [script-system-missions](features/script-system-missions.md)); the zipline rope (TRAV-7); the detective marks (DETECT-14, needs the user's answer); the scanner view's look and timing now that the oracle has a motion source (SCAN-1, SCAN-4, [abilities-scanner](features/abilities-scanner.md)).
2. One worker per round on a wide-reach item from [priorities.md](priorities.md), whether or not mission 1-2 needs it. The trampoline's look and the table's wobble (LEVOBJ-S77, LEVOBJ-S79, LEVOBJ-90), attack sounds (PLAYER-17); oracle checks of the zipline A press and the pole A press (TRAV-8, TRAV-34, need user states).
3. The dog and the dog truck: animals are not created (LEVOBJ-7), the truck's moving bound and driving (SCRIPT-77, SCRIPT-43); cutscene 1_2_c and the mission end (SCRIPT-115) ([script-system-missions](features/script-system-missions.md)).
4. Open mission 1-1 gaps: the rest of the vehicle effects (VEH-121, VEH-123, VEH-124, VEH-128, PART-2), the studs and camera of the build (LEVOBJ-76, LEVOBJ-77), shadows (RENDER-33, RENDER-9), traffic and pedestrians (SCRIPT-230), camera classes (CAMERA-40, CAMERA-52).
5. Later: HUD fidelity, saves (FE-30, SAVE-14, SAVE-15), level switch screens (TRANS-13, TRANS-14), streaming (WSTREAM-30, TRANS-3), death and respawn (PLAYER-11), rendering (RENDER-60).

Oracle states (artifacts/oracle/states, see docs/re/oracle.md): `gameplay-start` (new game, Old 3DS), `police-station` and `street-after-1-1` (user saves after mission 1-1, New 3DS), `street-at-entrance`, `station-at-exit`, `station-exit-loadingpage`, `camera-lobby-walk`, `dcam-police-dialog`, `agent-doughnut-title` (agents, unmodified, New 3DS), `savecont-slotlist-user` (slot list with the user's save, Old 3DS), `trans-poked-*` (poked, measuring aids only). The real save of the user (read-only copy): `artifacts/oracle/agentJ-saves/savegame.dat`. Hard-to-reach states: ask the user (CLAUDE.md "Runtime oracle").

**Workflow notes:** oracle route and states: `tools/azahar_oracle.py` with `--display xvfb --state gameplay-start --renderer opengl --rpc --rpc-input`, route from boot `tools/oracle/to-gameplay.txt`; only one agent may use `--rpc` at a time. The lead delegates to subagents in worktrees (at most 3 in parallel, user), every brief names the CLAUDE.md rules, reviews each result against CLAUDE.md, rebuilds and tests, squash-merges one commit per branch with `tools/merge_agent.py prepare|check|finish` (it removes the worktree and branch), pushes when authorized, and checks the Graphify rebuild. Agents keep scratch files in their own subdirectory of the shared scratchpad. Worktree agents link `artifacts/` into their worktree (CLAUDE.md "Subagents"). Resume a branch with `git log main..<branch>`.

## Session Log

The newest 20 lines only; `python3 tools/check_doc_links.py --fix` moves older lines unchanged to [linux-port.progress-archive.md](linux-port.progress-archive.md). One line per session: timestamp, what was worked on, how far it got, link to the feature file. Findings, open items and stand-ins do not belong here (CLAUDE.md "Feature files"). Reverse-engineering details are in docs/re/ (see docs/re/README.md).

- 2026-10-18T16:10:43+02:00 — Agent r21-A (round 21, pole): pole and drain pipe ported (walk-in and A-press mount, climb, exits, dismount) and pinned against m12-10 / m12-10a; zipline use press found to be the A press (TRAV-12); follow-ups TRAV-11..TRAV-16 — [player-traversal.md](features/player-traversal.md)
- 2026-10-18T16:18:22+02:00 — Lead (round 21 end): r21-B (climbing bricks), r21-C (rails, climb bars, corner rail) and r21-A (pole and drain pipe, walk-in mount) merged; the player step chains rails, bricks and pole, the jump off the last brick grabs the rail as in the oracle (TRAV-30 closed, test `game-script-brick-rail`); traversal docs split by subtopic; user pole states verified — [player-traversal.md](features/player-traversal.md), [player-traversal-pole.md](features/player-traversal-pole.md), [player-traversal-rails.md](features/player-traversal-rails.md)
- 2026-10-18T16:38:47+02:00 — Agent r22-C (round 22, zipline A press, PositionObject): the zipline use press moved from B to A (TRAV-12 closed, follow-ups TRAV-34, TRAV-35) and PositionObject now moves level characters, so Holmes ends at the Doorlock teleport (SCRIPT-287 closed, SCRIPT-288 open); both checked in tests, Holmes also against the oracle by eye — [player-traversal.md](features/player-traversal.md), [script-system-missions.md](features/script-system-missions.md)
- 2026-10-18T16:43:21+02:00 — Lead (round 22 end): r22-C merged (zipline get-on with A, TRAV-12 closed; PositionObject places level characters, Holmes leaves the bin, SCRIPT-287 closed; checked by the lead on the user's real path from their save); the street lamp report corrected to cars passing through (VEH-103) — [player-traversal.md](features/player-traversal.md), [script-system-missions.md](features/script-system-missions.md)
- 2026-10-18T20:00:41+02:00 — Agent r23-E (round 23, unnamed mission 1-2 scripts): traced every script start path and trapped the m12 states; the two unnamed scripts have no starter in the original build (SCRIPT-113 closed, nothing to port), the Door_Lock_Hint object trigger is narrowed (SCRIPT-285); new oracle step `freetrap` and CTest `script-starters-real-data`. [script-system-missions.md](features/script-system-missions.md)
- 2026-10-18T20:02:49+02:00 — Agent r23-A (round 23, attack on foot): Chase punches with a Y tap; the combat states, combo, target search, hit test and the hit message 0 to the target come from the original, and the message reaches DonutTable_14 from the m12-table-before spot in tests (PLAYER-12 closed, PLAYER-13..18 open, stand-ins PLAYER-S5..S9) — [player-movement.md](features/player-movement.md)
- 2026-10-18T20:00:09+02:00 — Agent r23-B (round 23, vehicle prop smash): cars now smash mesh-kind props (all lamp posts), traffic lights and respawnable / simple destructible props, pinned to oracle frames (traffic light, tree exact, lamp post one update early); a smashed prop drops bricks, no studs (dropped studs left as STUD-11); follow-ups VEH-129..132 — [vehicles.md](features/vehicles.md)
- 2026-10-18T19:30:29+02:00 — Agent r23-C (round 23, detective glass): Chase's magnifying glass is drawn in detective mode from the original's data (hand item track 0xba1e5b53 names Ability_MagnifyingGlass and weaponRight; first shown 14 updates after the A press as in the oracle), DETECT-7 closed, look differences listed (DETECT-9..11) and hand item follow-ups DETECT-8, DETECT-12, DETECT-13 opened — [abilities-detective.md](features/abilities-detective.md)
- 2026-10-18T20:27:05+02:00 — Agent r23-D (round 23, destructible table and trampoline): a hit from any sender (Chase's punch, damage 1, health 5 for DonutTable_14) wobbles or destroys a leprop_wobblydestructible, the destroy enables Trampoline_03_Buildits through its script, and Chase bounces on the built Trampoline_03 with the original's take-off speeds and rise (oracle rows pinned); LEVOBJ-86 closed, follow-ups LEVOBJ-87..92, stand-ins LEVOBJ-S77..S80 — [level-objects.md](features/level-objects.md)
- 2026-10-18T20:40:45+02:00 — Lead (round 23 end): r23-C (detective glass), r23-E (no starter for the two unnamed mission 1-2 scripts, `freetrap`), r23-A (attack on foot), r23-B (cars smash lamp posts and other destructible props) and r23-D (destructible table, trampoline bounce) merged; the lead ran table, build and bounce once in `chase-game` and the user played it; the servant's zipline and pole presses did not fit the states (deferred), the build-it zone rotation (LEVOBJ-93) and the floating glass in a cutscene (MOVIE-34) recorded — [level-objects.md](features/level-objects.md), [level-objects-buildit.md](features/level-objects-buildit.md), [player-traversal.md](features/player-traversal.md), [movies-cutscenes.md](features/movies-cutscenes.md), [abilities-detective.md](features/abilities-detective.md)
- 2026-10-18T21:07:12+02:00 — Agent r24-B (round 24, MOVIE-34): the floating magnifying glass was the prop of cutscene `1_2_a` (clip start frame 478) drawn at its frame-0 pose from the start, not the player's glass glue; a cutscene's model of its own is now hidden until its first clip starts (stand-in MOVIE-S33, the original's gate stays open as MOVIE-35), MOVIE-34 closed — [movies-cutscenes.md](features/movies-cutscenes.md)
- 2026-10-18T21:11:07+02:00 — Agent r24-C (round 24, flag poles above the trampoline): the bar test now also runs in the fall updates (state 5 grab chain, oracle-read) so Chase grabs 1-2_FlagPole_01 from the 0.9 bounce, with the arrival and the pole centre pinned to the oracle; the acrobat pole states after the arrival stay open (TRAV-37); TRAV-36 closed, see [player-traversal-rails](features/player-traversal-rails.md).
- 2026-10-18T21:00:03+02:00 — Agent r24-A (round 24, build-it use zone): the build-it's use zone is turned with the placement's orientation as the original's query does (code read: the player is put into the node's frame before the box test); the trampoline build-it of mission 1-2 now holds the spot where DonutTable_14 stood; LEVOBJ-93 closed, LEVOBJ-94 opened for the other zone owners; unit, real-data and trampoline tests added, not checked in the oracle ([level-objects-buildit.md](features/level-objects-buildit.md))
- 2026-10-18T21:13:02+02:00 — Agent r24-D (round 24, priority ranking): `tools/priority_report.py` ranks open items and stand-ins by script calls and placed objects in the 8 main levels and by what they unblock, written to `docs/plans/priorities.md` (CTest `priorities-current`, regenerated by `tools/merge_agent.py`); optional item field `uses:`; see [docs/progress.md](../progress.md)
- 2026-10-18T21:24:00+02:00 — Lead (round 24 end): r24-A (build-it use zone turned with the placement), r24-B (cutscene props hidden until their clip starts), r24-C (flag pole grab from the trampoline bounce) and r24-D (priority ranking, `docs/plans/priorities.md`) merged; the lead ran table, build, bounce and pole grab once in `chase-game`; the user's playtest found the missing pole swing, attack animation and trampoline wall and the offset detective marks, recorded — [player-traversal-rails.md](features/player-traversal-rails.md), [player-movement.md](features/player-movement.md), [level-objects.md](features/level-objects.md), [movies-cutscenes.md](features/movies-cutscenes.md)
- 2026-10-18T22:04:05+02:00 — Agent r25-D (round 25, oracle motion source): the oracle starts a Cemuhook DSU motion server per session (`--motion-port`, script step `tilt`); with the level pose the scanner view runs and the camera yaw is no longer NaN (RPC read and screenshot, one load per pose); gyroscope axes and the NaN's origin are open. [viewer-tools](features/viewer-tools.md)
- 2026-10-18T21:39:24+02:00 — Agent r25-A (round 25, acrobat pole swing): read the acrobat pole states 93..98 from the code (idle, flip, jump to the next pole, jump to fall / land) and ported the facing on a pole and the choice of the next pole, checked against the r24-C traces; the game does not use them yet, the swing motion, the flip and the idle update are open. Feature: [player-traversal-rails.md](features/player-traversal-rails.md)
- 2026-10-18T21:41:10+02:00 — Agent r25-C (round 25, object kinds): the kind byte of every placed class is read from the class file ObjectType (`objectKind`, test `object-kinds`); ToggleUseable group case (kind 71 pushables and their kind 72 handles), EnableShadow and EnableSwitch run as stand-ins; enableobjectfade and DistanceFadeMid stay open; see [script-system-natives.md](features/script-system-natives.md)
- 2026-10-18T21:41:18+02:00 — Agent r25-B (round 25, attack animation): the combo clips' joint curves are 0xfa raw matrices that were never decoded (rig stayed in the bind pose); 0x1324f8 ported to `chase_assets` with tests, visual check of the punch still open. See [player-movement.md](features/player-movement.md).
- 2026-10-18T22:10:30+02:00 — Lead (round 25 end): r25-A (acrobat pole states read, next-pole choice and facing ported, not wired), r25-B (raw matrix joint curves decoded, punch not yet seen on screen), r25-C (object kinds, ToggleUseable group case, EnableShadow and EnableSwitch) and r25-D (virtual motion source in the oracle; the scanner view renders) merged; the servant measured the trampoline frame; the user stopped the round early (usage limit) — [player-traversal-rails.md](features/player-traversal-rails.md), [player-movement.md](features/player-movement.md), [script-system-natives-world.md](features/script-system-natives-world.md), [viewer-tools.md](features/viewer-tools.md), [abilities-scanner.md](features/abilities-scanner.md)
