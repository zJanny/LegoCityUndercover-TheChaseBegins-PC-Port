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

**First (user request, 2026-10-29):** "make HUD-75 the Next Action": the font size issue [HUD-75](features/hud-ui.md) now blocks QOLSET-33, the qol settings menu drawing.

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

- 2026-10-27T23:41:17+01:00 — Agent 20261027-mission13-F (super build-it messages and save): messages 8 and 0xfe, the save byte, the restore and DisableSuperBuiltItSave ported; the message 6 handler is written but not wired. See [level-objects-buildit-super.md](features/level-objects-buildit-super.md).
- 2026-10-28T00:12:06+01:00 — Agent 20261027-mission13-A (path platforms, the lawn mowers): path platforms move along their spline and turn by the path's quaternions, attached objects turn with them; the rescue mower no longer stands still; the oracle comparison is open. See [script-system-platforms.md](features/script-system-platforms.md).
- 2026-10-28T00:41:16+01:00 — Agent 20261027-mission13-D (carry placing timing): servant rows show the port's placing already equals the oracle (release P+8, script P+10, state 1 at P+27); the first recording was numbered one update too high, feature file split ([script-system-missions-1-3-generator.md](features/script-system-missions-1-3-generator.md)).
- 2026-10-28T01:22:21+01:00 — Agent 20261027-mission13-C (push cart and crane end against the oracle): the cart's moves and the end script's timing equal the oracle, message 9 acts one update later, the player now jumps and falls at the bridge; height curve, camera shots and the landing move are open. See [script-system-missions-1-3-end.md](features/script-system-missions-1-3-end.md).
- 2026-10-28T01:30:13+01:00 — Lead (20261027-mission13 end): merged 20261027-mission13-F (super build-it helipad messages and save record), -A (path platforms, lawn mowers move), -D (carry place timing corrected, docs only), -B (platform riding, ledge lift, orient ride), -E (attached props turn with parent, generator camera compared), -C (push cart and bridge jump compared) through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [level-objects-buildit-super.md](features/level-objects-buildit-super.md), [script-system-platforms.md](features/script-system-platforms.md), [script-system-missions-1-3.md](features/script-system-missions-1-3.md), [script-system-missions-1-3-generator.md](features/script-system-missions-1-3-generator.md), [script-system-missions-1-3-end.md](features/script-system-missions-1-3-end.md)
- 2026-10-28T22:56:23+01:00 — Agent 20261028-uiport-A (map buttons): the map buttons of the bottom screen (buttons, click areas from the oracle, characters tap and hold, the fast travel page and its exit) run in chase-game; the question and the trip of the fast travel page are still open; see [ui-menus](features/ui-menus.md), [level-objects-fast-travel](features/level-objects-fast-travel.md).
- 2026-10-28T22:39:13+01:00 — Agent 20261028-uiport-B (Options page save state): a user save state of the pause Options page with changed volumes and Invert camera is registered in the local catalogue; the user confirmed mission 1-3 done with none running; no other repo files.
- 2026-10-28T22:57:18+01:00 — Agent 20261028-uiport-C (award viewer): the police station's use spot runs the native AwardViewer, which opens the award viewer: round menu with Overview and the seven levels, Back, and the bronze, silver and gold counts of the save slot; models, fades and an oracle picture are open. See [ui-menus.md](features/ui-menus.md).
- 2026-10-28T22:53:35+01:00 — Agent 20261028-uiport-D (scanner bottom screen): the map buttons slide out and in at the scanner enter and exit and a click on the back-arrow button leaves the scanner like B, read from the original and checked against servant pictures; the tip pages shown during the view are open. See [abilities-scanner.md](features/abilities-scanner.md).
- 2026-10-28T22:55:52+01:00 — Agent 20261028-uiport-B (pause Options page and saved volumes): the Options page runs as in the original and was checked against the oracle, each level load takes the saved volumes, the options are saved and Invert camera reaches the camera; the Languages page and the save check stay open ([ui-menus.md](features/ui-menus.md), [saves-options.md](features/saves-options.md)).
- 2026-10-28T23:15:49+01:00 — Lead (recorded 20261028-uiport-C): the user's quick-save on the mainframe trigger is registered in the local catalogue (mission 1-3 done, none running); it is not the award viewer spot, which is still missing.
- 2026-10-28T23:44:28+01:00 — Lead (recorded user request): the user approved three workflow speed-ups for the next round (the merge step skips unchanged checks, ccache in CMake, branches stop committing generated files); recorded in Next Action with the rule and skill text they touch; no code changed.
- 2026-10-28T23:57:17+01:00 — Agent 20261028-uiport-E (super build-it placeholders): the oracle pictures show two turning blue bricks but do not separate sprite from model icon; nothing drawn; see [hud-ui-super-bricks.md](features/hud-ui-super-bricks.md).
- 2026-10-29T00:08:23+01:00 — Lead (20261028-uiport end): merged 20261028-uiport-A (map buttons screen; fast travel question still open), 20261028-uiport-B (pause Options page and option save fields), 20261028-uiport-C (award viewer; review returned the Start-in-viewer bug, fixed), 20261028-uiport-D (scanner bottom screen buttons), 20261028-uiport-E (super build-it placeholder findings only, code waits on an open question) and three recorder branches through tools/merge_agent.py with all tests passing; ID ranges released in the merge commits; pushed main to origin/main after each merge. See [ui-menus.md](features/ui-menus.md), [ui-menus-award-viewer.md](features/ui-menus-award-viewer.md), [saves-options.md](features/saves-options.md), [abilities-scanner.md](features/abilities-scanner.md), [hud-ui-super-bricks.md](features/hud-ui-super-bricks.md), [level-objects-fast-travel.md](features/level-objects-fast-travel.md)
- 2026-10-29T07:49:48+01:00 — Agent 20261029-workflow-A (merge finish skips repeat checks): finish skips build, ctest and Python tests when the staged tree and main's HEAD equal the last passed check; see docs/commands.md "Merging an agent branch"
- 2026-10-29T07:51:37+01:00 — Agent 20261029-workflow-B (ccache launcher): CMake uses ccache automatically when installed (`-DCHASE_USE_CCACHE=OFF` switches it off), documented in docs/building.md and CONTRIBUTING.md; no feature file.
- 2026-10-29T07:51:55+01:00 — Agent 20261029-workflow-C (generated files out of branches): the commit hook refuses staged generated files off main, CTest progress-readme and priorities-current skip off main, merge_conflicts.py keeps both sides of append-style docs conflicts; rule text updated, no feature file (workflow tooling).
- 2026-10-29T08:46:10+01:00 — Agent 20261029-workflow-D (merge commit hook fix): the commit hook compares generated files with MERGE_HEAD during a merge, so merging main into a branch commits; detached HEAD counts as main; tests in tests/test_claude_hooks.py.
- 2026-10-29T08:49:47+01:00 — Lead (20261029-workflow end): merged 20261029-workflow-B (ccache as compiler launcher), 20261029-workflow-C (generated files stay out of branches, prepare merges append-only docs conflicts), 20261029-workflow-A (finish skips checks for an already verified tree), 20261029-workflow-D (hook allows merge commits of main on a branch; user-approved follow-up) through tools/merge_agent.py with all tests passing; no ID ranges reserved; pushed main to origin/main after each merge. Workflow tooling only, no feature files; see [commands.md](../commands.md)
- 2026-10-29T12:03:35+01:00 — Agent 20261029-hudmap-A (level map files): the bottom-screen map reads each level's map file through the level table as the original's loader does, the police station uses the first level's map, the unused `_2` pictures are recorded; hub centre and map switch stay open ([hud-ui](features/hud-ui.md)).
