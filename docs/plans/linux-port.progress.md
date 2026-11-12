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

- 2026-11-12T08:06:16+01:00 — Agent 20261112-traffic-E (cop whistle connection): the D-pad Down request with extras bit 19 (`--extras`), the clip row 709, state 0x1b2 and the game clock now run in chase-game as `PlayerCopWhistle`; ctrl +0x13c is the carried object; clock unit and character row byte stay open. See [pedestrians-traffic-cars-whistle.md](features/pedestrians-traffic-cars-whistle.md).
- 2026-11-12T08:06:02+01:00 — Agent 20261112-traffic-F (pedestrian messages and order): read the pedestrian's message handler in full, ported its messages and sorted the object phase by class type with the pedestrians at 197 — [pedestrians-traffic-pedestrians.md](features/pedestrians-traffic-pedestrians.md)
- 2026-11-12T08:11:30+01:00 — Agent 20261112-traffic-B (traffic contact gather): ported the traffic contact gather (cell walk, filters, box test, cap) with its `gatherContacts` hook; chase-game does not feed it characters yet. See [pedestrians-traffic-cars-gather.md](features/pedestrians-traffic-cars-gather.md).
- 2026-11-12T09:06:54+01:00 — Agent 20261112-traffic-G (README restyle): README restyled with a header, four local SVG badges and the progress block moved below the quick start; install path re-read against the sources; docs only, see [project status](../status.md).
- 2026-11-12T09:36:15+01:00 — Lead (20261112-traffic end): merged 20261112-traffic-F (pedestrian messages and object-priority pass order), 20261112-traffic-B (traffic contact grid gather), 20261112-traffic-E (player cop whistle, --extras), 20261112-traffic-D (chase-ended event at the level root, target hearts), 20261112-traffic-G (README restyle, approved mid-round), 20261112-traffic-C (car push vector and squash), 20261112-traffic-A (player car against traffic car bodies) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [pedestrians-traffic-cars-driver.md](features/pedestrians-traffic-cars-driver.md), [pedestrians-traffic-cars-driver-body.md](features/pedestrians-traffic-cars-driver-body.md), [pedestrians-traffic-cars-push.md](features/pedestrians-traffic-cars-push.md), [pedestrians-traffic-cars-gather.md](features/pedestrians-traffic-cars-gather.md), [pedestrians-traffic-cars-whistle.md](features/pedestrians-traffic-cars-whistle.md), [pedestrians-traffic-pedestrians.md](features/pedestrians-traffic-pedestrians.md), [script-system-missions-1-4.md](features/script-system-missions-1-4.md)
- 2026-11-12T10:12:50+01:00 — Agent 20261112-trafficfeed-C (push shape tests): ported the world slide, the mesh test and the oriented test behind the push vector's shape hooks; the move write-back and the world in chase-game stay open. See [pedestrians-traffic-cars-push.md](features/pedestrians-traffic-cars-push.md).
- 2026-11-12T10:02:22+01:00 — Agent 20261112-trafficfeed-B (character feed of the traffic contact gather): the grid layout and cell lists of the traffic contact gather are ported from the original and chase-game feeds the player and the enabled pedestrians to the traffic cars; the level inputs of the build and the entry details are still stand-ins. See [pedestrians-traffic-cars-gather.md](features/pedestrians-traffic-cars-gather.md)
- 2026-11-12T10:05:15+01:00 — Agent 20261112-trafficfeed-A (chase end in play): the stolen car of mission 1-4 loses a heart per hit and its velocity is scaled by the contact factor; five hits of the player's police car end the chase in `chase-game` and the chaser leaves the car; the original's chase is not measured. See [script-system-missions-1-4.md](features/script-system-missions-1-4.md).
- 2026-11-12T10:01:57+01:00 — Agent 20261112-trafficfeed-D (disguise system map): first static map of the disguise system (classes, party, swap, requests, save, level census); eight open items, nothing ported; docs only. See [disguise.md](features/disguise.md).
- 2026-11-12T10:40:48+01:00 — Lead (20261112-trafficfeed end): merged 20261112-trafficfeed-A (mission 1-4 chase end in play), -B (traffic character feed, partial), -C (traffic push shape tests, partial), -D (disguise investigation), -E (user docs guide and rewrite) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [script-system-missions-1-4.md](features/script-system-missions-1-4.md), [pedestrians-traffic-cars-gather.md](features/pedestrians-traffic-cars-gather.md), [pedestrians-traffic-cars-push.md](features/pedestrians-traffic-cars-push.md), [disguise.md](features/disguise.md); user docs audit: declined
- 2026-11-12T11:27:56+01:00 — Agent 20261112-enemy-E (thrown goon mover and camera modes): the character mover and the throw fall of the grab and takedown states read and ported, a goon in the air flies on and feeds the FallArrest update; the throw does not move the goon yet (root and pair motion open); the camera manager's mode slots take BulletTime's restore calls. See [script-system-missions-1-4-roof.md](features/script-system-missions-1-4-roof.md).
- 2026-11-12T11:34:22+01:00 — Agent 20261112-enemy-A (enemy fight states): the decisions of AI states 2, 3, 4 / 5 and 31 and of AIRunAway, AIStalkPlayer and AIPursuit are read; states 2, 3, 4 / 5 and the hold of 31 are ported for every placed character, so the goons of mission 1-4 run up to Chase in `chase-game`; they do not strike yet. See [script-system-ai-navigation.md](features/script-system-ai-navigation.md), [combat-characters.md](features/combat-characters.md).
- 2026-11-12T11:24:19+01:00 — Agent 20261112-enemy-C (traffic push world): chase-game now registers every pedestrian slot and the player in the contact grid as the original's registration pass does (no entry removed) and supplies the push world, so a traffic car that drives into a person sends it message 45 and the hit is logged; the level inputs of the grid and the people's reaction stay stand-ins; no scene reaches a car in driver state 5 or 8 beside a person yet. See [pedestrians-traffic-cars-gather.md](features/pedestrians-traffic-cars-gather.md), [pedestrians-traffic-cars-push.md](features/pedestrians-traffic-cars-push.md)
- 2026-11-12T11:32:24+01:00 — Agent 20261112-enemy-B (disguise party manager): the party manager of the original is ported (level set-up, add, removal, cycle, swap bookkeeping) and SetPartyCharacter, UnlockCharacter and LockCharacter use it in `chase-game`; the disguise calls of every mission are listed; the swap of the player entity is still a stand-in. See [disguise.md](features/disguise.md).
- 2026-11-12T11:32:30+01:00 — Agent 20261112-enemy-D (mission 1-4 target display and chased car): read the target display callers and VehicleSetChased's table and node test and ported them, the vehicle return fills the display; partial, the draw and the hold of a VehicleSetAI car stay open; feature file split ([script-system-missions-1-4-chase](features/script-system-missions-1-4-chase.md)).
- 2026-11-12T19:40:46+01:00 — Lead (20261112-enemy end): merged 20261112-enemy-C (traffic push world supplied in chase-game), 20261112-enemy-B (disguise party manager, entity swap open), 20261112-enemy-D (mission 1-4 chase target display as fields, draw open), 20261112-enemy-A (goon fight states 2, 3, 4/5 and 31, strike open) and 20261112-enemy-E (takedown mover, throw fall and camera mode slots, throw motion open) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [traffic gather](features/pedestrians-traffic-cars-gather.md), [disguise](features/disguise.md), [1-4 chase](features/script-system-missions-1-4-chase.md), [AI navigation](features/script-system-ai-navigation.md), [combat characters](features/combat-characters.md), [1-4 roof](features/script-system-missions-1-4-roof.md)
- 2026-11-12T20:19:16+01:00 — Agent 20261112-collab-A (contributor docs): added docs/development.md, the first-pull-request path, DCO and clean-room rules, community files and templates in CONTRIBUTING.md; CLAUDE.md got an outside-agent preamble and a sync rule. No game behaviour changed. [feature index](features/README.md)
- 2026-11-12T20:30:28+01:00 — Agent 20261112-collab-C (merge tooling for outside pull requests): `tools/merge_agent.py check-pr N` checks a GitHub pull request (file refusal before any of its code runs, then build and all tests in a temporary checkout), the forbidden-file rules moved to `tools/forbidden_files.py`, merges refresh the generated-file census table, the commit hook allows README edits outside the progress section and pairs CLAUDE.md with docs/development.md, doc-links explains the duplicate-ID fix and the lead-round skill starts only on request; done, no game behaviour changed. See [commands.md](../commands.md).
- 2026-11-12T20:28:10+01:00 — Agent 20261112-collab-B (CI and checks): added the generated-file census table and a regeneration job, the DCO job, and the no-game-files and tests-data-size checks, each with unit tests; the code coverage figure is stored as scalars, not a function list, and the census table stays under 100 KB; no game behaviour changed ([commands](../commands.md)).
- 2026-11-12T20:47:07+01:00 — Lead (20261112-collab end): merged 20261112-collab-C (check-pr, tools/forbidden_files.py, hook narrowing, duplicate-ID message, lead-round trigger), 20261112-collab-B (DCO check, no-game-files and tests-data-size CTests, census table and Generated files job) and 20261112-collab-A (docs/development.md, CONTRIBUTING outside-contribution section, community files and templates) through tools/merge_agent.py with all tests passing; no ID ranges reserved; pushed main to origin/main after each merge. See [CONTRIBUTING.md](../../CONTRIBUTING.md), [development.md](../development.md)
