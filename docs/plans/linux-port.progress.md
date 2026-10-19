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

- 2026-10-19T15:35:48+02:00 — Lead (round 34 end, started alongside r33-A on user request): merged r34-A (EnableBound in camera records, ShakeScreen), r34-C (null script calls stop, seated PositionPlayer, placement effects) and r34-B (player state change order) with all tests passing; released their ID ranges; pushed main ([camera](features/camera.md), [script-system](features/script-system.md), [script-system-natives-animation](features/script-system-natives-animation.md)).
- 2026-10-19T15:55:10+02:00 — Agent r35-C (round 35, batch 35 natives and the transition scripts): split the flag and progress natives out of the natives plan; read all 18 batch 35 handlers (UsePet and HACK_PlayDowntownIntro run, the other 15 record their effect in `PartyNativeState`), SCRIPT-35 closed; TRANS-5 rewritten (only five unhandled natives remain in the transition scripts); tests `natives-party` and `natives-party-real-data` ([script-system-natives-party.md](features/script-system-natives-party.md)).
- 2026-10-19T16:00:15+02:00 — Lead (round 35 end, one worker on user request): merged r35-C (feature file split, TRANS-5 brought up to date, batch SCRIPT-35 handlers) through `tools/merge_agent.py` with all tests passing; released its ID ranges; pushed main ([script-system-natives-party](features/script-system-natives-party.md), [level-transitions](features/level-transitions.md)).
- 2026-10-19T17:50:50+02:00 — Lead: the user checked the dog footprints in detective mode and found them correct; DETECT-16 and DETECT-17 closed and the save state request withdrawn ([abilities-detective](features/abilities-detective.md)).
- 2026-10-19T17:56:41+02:00 — Lead: recorded the user's mission 1-2 playthrough (flag poles, trampoline, scanner look, dog release and punch confirmed; PLAYER-19 closed) and its four bugs as TRAV-53, SCAN-14, SCRIPT-341 and SCRIPT-342 ([script-system-missions](features/script-system-missions.md)).
- 2026-10-19T18:02:53+02:00 — Lead: recorded the user's five mission 1-3 save states (`m13-1-roof-superbrick` to `m13-5-helipad-built-menu`, read-only copies, not loaded yet) and the ladder seen in the port as RENDER-71 ([rendering](features/rendering.md)).
- 2026-10-19T18:17:18+02:00 — Agent r36-A (round 36, zipline poles): the poles were placed but drawn 25 m above the anchors because the static draw ignored the pole model's node matrix; now drawn with it and the link rows (`zipline_pole`, CTests `zipline-pole`, `zipline-pole-real-data`), poles seen in chase-game; TRAV-53 closed, TRAV-54 and LEVOBJ-113 open, details in [player-traversal.md](features/player-traversal.md).
- 2026-10-19T18:21:29+02:00 — Agent r36-B (round 36, SCAN-14): Chase now holds the scanner device (`Ability_Scanner` on `weaponRight` from the hand item track of `scanner_out` / `scanner_in`, shared with the glass in `game_hand_items`); pose not compared with the oracle ([abilities-scanner](features/abilities-scanner.md)).
- 2026-10-19T18:31:18+02:00 — Agent r36-C (round 36, SCRIPT-341 and SCRIPT-342): the dog of `1_2_c` was overwritten by its own prop clip and the dog truck lacked the node matrix of skinned meshes; both fixed and closed, the matrix rule is a data fit (MOVIE-S35, MOVIE-39) ([movies-cutscenes.md](features/movies-cutscenes.md)).
- 2026-10-19T18:38:41+02:00 — Lead (round 36 end, mission 1-2 playtest bugs): merged r36-A (zipline poles), r36-B (scanner device in hand) and r36-C (dog and dog truck in the end cutscene) through `tools/merge_agent.py` with all tests passing; released their ID ranges; pushed main; Next Action now points at mission 1-3 ([player-traversal](features/player-traversal.md), [abilities-scanner](features/abilities-scanner.md), [movies-cutscenes](features/movies-cutscenes.md)).
- 2026-10-19T19:25:55+02:00 — Agent r37-C (round 37, the helipad super build-it of mission 1-3, LEVOBJ-117..119): the super build-it class (kind 98) read with REA and llvm-objdump and measured from m13-3 to m13-5 over RPC; the prompt page, the confirmation, the movie and the build ported (`SuperBuilditSet`, `drawSuperBuildPrompt`), the count is the super brick pickups' after the merge of main (19:38:50: spend and save request), the fast travel menu recorded only; CTests `super-buildit`, `super-buildit-real-data`, `game-super-buildit-prompt`, `game-super-buildit-build`. [level-objects-buildit.md](features/level-objects-buildit.md), [hud-ui.md](features/hud-ui.md)
- 2026-10-19T19:24:08+02:00 — Agent r37-B (round 37, STUD-16..18): placed super build bricks (`pickup_sbbrick*`, pickuptype 103) are picked up in `chase-game` with the 1.0 s countdown, the count at 0x593430 (slot +0x5190), the UI sound and the save request; the roof brick of mission 1-3 is `1-3_Rainbow_02` (200); HUD display and flight open ([studs](features/studs.md), [hud-ui](features/hud-ui.md)).
- 2026-10-19T19:28:53+02:00 — Agent r37-A (round 37, LEVOBJ-113, MOVIE-39, RENDER-71): read the model draw (0x205160 / 0x10e028): every node's mesh is drawn with its node matrix before the node world, skinned meshes after their skinning; one rule in `Scene::addModel` replaces the zipline, skinned-mesh and footprint special cases, and the mission 1-3 ladder's upper half (a nested class object) is drawn; closed LEVOBJ-113, MOVIE-39, RENDER-71 ([level-objects](features/level-objects.md), [movies-cutscenes](features/movies-cutscenes.md)).
- 2026-10-19T19:49:14+02:00 — Lead (round 37 end, mission 1-3): merged r37-A (universal node matrix draw, nested class-file objects incl. the roof ladder), r37-B (placed super bricks and their count) and r37-C (helipad super build-it, wired to the count) through `tools/merge_agent.py` with all tests passing; fixed the slot file of the m13 state JSONs and made the oracle tool refuse a slot its quick load cannot read; released the ID ranges; pushed main ([level-objects](features/level-objects.md), [studs](features/studs.md), [level-objects-buildit](features/level-objects-buildit.md)).
- 2026-10-19T20:35:43+02:00 — Agent r38-A (round 38, SAVE-11, SAVE-15, STUD-17, SAVE-14 partly): `chase-game` writes save requests to `--save-out` or the default XDG save file; a new game starts from the original default slot, the stud counter and super brick count load from the slot and survive scene changes, the play time ticks and the level variables and StoryProgress go into the level block; SAVE-11, SAVE-15 and STUD-17 closed, SAVE-14 narrowed ([saves-options](features/saves-options.md)).
- 2026-10-19T20:40:04+02:00 — Agent r38-B (round 38, UI census and pause menu): census of all 55 layouts and the code-drawn screens, new feature file ui-menus (UI-1..UI-19, HUD-5 closed), pause screen opened by Start with its round menu, D-pad, taps and swipes, Resume / B / Start close; Options, Credits, Exit open. [ui-menus](features/ui-menus.md)
- 2026-10-19T20:38:05+02:00 — Agent r38-C (round 38, HUD-72, STUD-18): `chase-game` shows the super brick count display at a pickup (fader 0x5198b0, yellow number, spinning brick icon as a fitted CPU stand-in, 80 updates), the collected brick flies to 6.0 ahead of the camera and the idle bricks take the rainbow tint; oracle pictures compared (digits exact, icon IoU 0.897); [hud-ui-super-bricks](features/hud-ui-super-bricks.md), [studs](features/studs.md).
- 2026-10-19T20:49:27+02:00 — Lead (round 38 end, stopped early for the usage limit): merged r38-A (saving to the port's save file, new-game slot, counters and level state from the slot), r38-B (UI census, pause menu, lower-screen clicks as touches) and r38-C (super brick display, flight and tint) through `tools/merge_agent.py` with all tests passing; recorded user reports TRAV-55..57; released the ID ranges; pushed main ([saves-options](features/saves-options.md), [ui-menus](features/ui-menus.md), [hud-ui-super-bricks](features/hud-ui-super-bricks.md)).
- 2026-10-19T23:07:18+02:00 — Agent r39-C (round 39, waypoint disable and placement message): ported the SetActiveWayPoint disable path and the player node show of a placement; the rest of both stays open; [script-system-hud-natives](features/script-system-hud-natives.md)
- 2026-10-19T23:18:27+02:00 — Agent r39-B (round 39, SCRIPT-310, HUD-S9): FadeScreen takes its screens from the ShowDialogBG byte and is transcribed (tween, end test, blend read); the dialogue background fades with the shared panel alpha that tips and BlackoutMap raise; SCRIPT-13 not started. [script-system-missions](features/script-system-missions.md), [hud-ui-text](features/hud-ui-text.md)
