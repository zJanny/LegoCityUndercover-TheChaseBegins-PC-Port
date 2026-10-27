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

- 2026-10-25T14:15:58+01:00 — Agent 20261025-recorded-A (oracle state roles): oracle scripts load recorded states by role (`loadstate @ROLE`, `--state-param ROLE=NAME`, checked before the emulator starts), 26 scripts converted, `check_local_data.py` reads `tools/oracle`; the converted scripts are not yet run in the emulator. See [viewer-tools](features/viewer-tools.md).
- 2026-10-25T14:20:45+01:00 — Agent 20261025-recorded-B (pedestrian fade point): the pedestrian fade point is now the camera look point; its writer, the third layer table push and the node type read from the code; servant readings added; PED-13 closed, PED-16 narrowed, PED-27..29 and PED-S15/S16 opened. [pedestrians-traffic-pedestrians](features/pedestrians-traffic-pedestrians.md)
- 2026-10-25T14:54:29+01:00 — Lead (20261025-recorded end): merged 20261025-recorded-A (oracle scripts take save states by role, TOOLS-20), 20261025-recorded-B (pedestrian fade point is the camera look point, PED-13; PED-16 narrowed), 20261025-recorded-C (mission 1-3 key pickup, SCRIPT-361) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [viewer-tools.md](features/viewer-tools.md), [pedestrians-traffic-pedestrians.md](features/pedestrians-traffic-pedestrians.md), [script-system-missions-1-3.md](features/script-system-missions-1-3.md)
- 2026-10-25T16:37:35+01:00 — Agent 20261025-garage-A (switch base and garage key): the switch base (loader, stable update, state change, base handler) and the lever (kind 58) ported for kinds 38 and 58; the mission 1-3 key switch opens the garage door on the real path; SCRIPT-358 and SCRIPT-362 closed, SCRIPT-360 narrowed, SCRIPT-365..367 and SCRIPT-S292/S293 opened; the key turn follows the oracle trace RPC 20261025-garage-A-1 ([script-system-missions-reference](features/script-system-missions-reference.md), [script-system-missions-1-3](features/script-system-missions-1-3.md)).
- 2026-10-25T16:20:58+01:00 — Agent 20261025-garage-B (parts crate to base plate): the crate break, `1-3_Got_parts`, the big carry object, the base plate placement and the build already ran; the carry target's PlacedObject (the build-it `1-3_Pull_Switch_Bits`) is now hidden at the level start as in the original (post-link 0x18e0f8); CTests `game-parts-crate-1-3`, `level-start-real-data`; SCRIPT-363 closed, SCRIPT-368 opened; see [script-system-missions-1-3](features/script-system-missions-1-3.md).
- 2026-10-25T16:48:41+01:00 — Agent 20261025-garage-C (generator script natives): SetPropRocking with the kind 67 rocking, WaitForPlatform with the platform travel step, and the code's AISuperJump horizontal step ported; servant trace of the generator scene pinned (platform 103 updates, the girder guy's jump); the height of the flight stays a stand-in. Details: [script-system-missions-1-3.md](features/script-system-missions-1-3.md), [script-system-ai-navigation.md](features/script-system-ai-navigation.md).
- 2026-10-25T17:08:37+01:00 — Lead (20261025-garage end): merged 20261025-garage-B (parts crate to base plate, carry targets hide their build spot at level start), 20261025-garage-A (switch base class, key switch opens the mission 1-3 garage door), 20261025-garage-C (WaitForPlatform, SetPropRocking, AISuperJump sideways step) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [script-system-missions-1-3.md](features/script-system-missions-1-3.md), [script-system-missions-reference.md](features/script-system-missions-reference.md), [script-system-ai-navigation.md](features/script-system-ai-navigation.md)
- 2026-10-26T13:41:29+01:00 — Agent 20261026-mission13-B (generator scene to mission end): PlayParticlesGO ported, level characters follow their AI moves, the generator scene runs on the real path from the parts crate through the pull switch to the end of `1-3_Generator_Script` (CTest `game-generator-1-3`); the mission end lies behind the ledge platform, the push cart and the crane build-it and is not reached; SCRIPT-372, SCRIPT-373 and SCRIPT-S300 opened, SCRIPT-364 narrowed to the oracle check; see [script-system-missions-1-3](features/script-system-missions-1-3.md).
- 2026-10-26T14:06:07+01:00 — Agent 20261026-mission13-A (lever use): the player's lever use read and ported (USE LERP to the model's use point, state 76 with the lever's clip whose event sets the switch's use bit, multi-turn and HoldMe rules), the switch's use animation now times state 1, ToggleUseable writes the lever zone and levers show use markers; SCRIPT-366 and SCRIPT-367 closed, oracle check SCRIPT-369 requested. [script-system-missions-1-3](features/script-system-missions-1-3.md), [script-system-missions-reference](features/script-system-missions-reference.md)
- 2026-10-26T16:14:27+01:00 — Agent 20261026-mission13-D (lever use A-1 pins): the oracle's key switch and pull switch recordings (RPC 20261026-mission13-A-1) agree with the port's lever use, turn, lerp and update counts with no code change; the counts are pinned in CTests `lever-use`, `switch` and `game-generator-1-3`, the dt claim of the old entry is corrected, SCRIPT-369 and SCRIPT-S299 closed ([script-system-missions-1-3.md](features/script-system-missions-1-3.md)).
- 2026-10-26T16:02:24+01:00 — Agent 20261026-mission13-C (oracle traces, generator scene and crate placing): the generator script (658 updates, port within one update) and the parts crate placing (build-it flag 193 to 192, script two updates after the release) recorded and compared; crate hits and the pickup attach need an intact-crate state (SCRIPT-368); see [script-system-missions-1-3.md](features/script-system-missions-1-3.md).
- 2026-10-26T16:22:12+01:00 — Lead (20261026-mission13 end): merged 20261026-mission13-B (generator scene on the real path, PlayParticlesGO), 20261026-mission13-A (player lever use and pull switch), 20261026-mission13-C (oracle trace of the generator scene and crate placing) and 20261026-mission13-D (replacement for A's follow-up: A-1 timings pinned, SCRIPT-369 closed) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. Mission 1-3 still cannot be finished (platform travel, pushing, crane build-it). See [script-system-missions-1-3.md](features/script-system-missions-1-3.md)
- 2026-10-26T21:12:45+01:00 — Lead (workflow hooks): Claude Code hooks in .claude/settings.json run tools/claude_hooks.py for the commit, push, git config, merge finish, brief and worktree rules, fix and stage docs and stale generated files at each commit and report doc and local-data findings after each edit; check_doc_links.py checks needs and blocks on both sides (autofixed on all feature files), new addresses in items and new item IDs or addresses in log lines. See [CONTRIBUTING.md](../../CONTRIBUTING.md#ai-coding-agents)
- 2026-10-26T23:01:25+01:00 — Agent 20261026-crane-A (crane platform and node carry): the platform class's trigger, travel and node move and AttachObject's node hierarchy are read and ported, the crane platform of mission 1-3 lowers the girder 10.63 in 102 updates as in the oracle; paths, rotations and riding stay open ([script-system-platforms](features/script-system-platforms.md)).
- 2026-10-26T23:08:06+01:00 — Agent 20261026-crane-B (push cart and crane of mission 1-3): build-its are loaded by kind 21 so the leprop build-its (crane, push cart) build; the pushable class, its handles and the player's handle states ported; from the lawnmower scene the port breaks the cart crate, builds and pushes the cart, builds the crane and runs the mission end script; ledge platform and oracle check open; see [script-system-missions-1-3-end.md](features/script-system-missions-1-3-end.md).
- 2026-10-26T22:52:15+01:00 — Agent 20261026-crane-C (girder guy follows the girder): the level's startup script attaches the girder guy to the rocking girder (AttachObject), now followed by level characters through a node hierarchy; the kind 67 record rule ported; the rocking clock and the ground follow read; AI-moved characters take the ground height (stand-in); see [script-system-missions-1-3.md](features/script-system-missions-1-3.md).
- 2026-10-26T23:28:27+01:00 — Lead (20261026-crane end): merged 20261026-crane-C (girder guy swings with the crane), 20261026-crane-B (crane build-it and push cart), 20261026-crane-A (crane platform and girder descent) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [script-system-platforms.md](features/script-system-platforms.md), [script-system-missions-1-3.md](features/script-system-missions-1-3.md), [script-system-missions-1-3-end.md](features/script-system-missions-1-3-end.md)
- 2026-10-26T23:38:35+01:00 — Agent 20261026-superbricks-A (super-bricks option): development option --super-bricks N sets the super brick count at the start of the run (0 to the u32 maximum, wins over --save, saves follow the game); see [studs-super-bricks.md](features/studs-super-bricks.md).
- 2026-10-26T23:43:18+01:00 — Lead (20261026-superbricks end): merged 20261026-superbricks-A (--super-bricks N development option) through tools/merge_agent.py with all tests passing; no ID ranges reserved; pushed main to origin/main after the merge. See [studs-super-bricks.md](features/studs-super-bricks.md)
- 2026-10-27T18:29:22+01:00 — Agent 20261027-graphics-A (hand items for every clip): HandItems has no fixed list; every object a clip's hand item track names and levels/players.blvl lists is loaded on its first event, shown, hidden and hung on its bone (handcuffs on the zipline, hammer, whistle, tea cup); the hand item part of the graphics round is done; see [abilities-detective.md](features/abilities-detective.md).
