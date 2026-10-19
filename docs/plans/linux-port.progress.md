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

**Priorities (critical path), updated 2026-10-19T15:35:47+02:00:** oracle states for every mission 1-2 step: `m12-1` to `m12-14`, `m12-10a-pole-foot`, `m12-10b-pole-side`, `m12-table-before`, `m12-table-broken`, `m12-trampoline-built`, vehicle effects: `veh-smash-prop`, `veh-smash-car`, `veh-break-low-hearts`, `veh-wall-full-speed` (docs/re/oracle-states.md); more on request from the user (new state requests deferred by the user on 2026-10-18). Rounds run three workers (user, 2026-10-18: short rounds); [priorities.md](priorities.md) ranks open items by their reach in the whole game and, when two need `--rpc`, one emulator servant that owns the only `--rpc` session.
1. Mission 1-2 playtest gaps: the user's playtest of mission 1-2 (flag poles, trampoline frame, zipline rope, scanner overlay and markers, detective footprints, the dog at the cage and its release, the mission end); the user's look at the punch (PLAYER-19, [player-movement](features/player-movement.md)); the footprints at the T-shirt spot (DETECT-16, DETECT-17: wait for a save state the user will provide, user 2026-10-19; remind the user in every round report until it arrives, [abilities-detective](features/abilities-detective.md)); the scanner's info boxes (SCAN-12, [abilities-scanner](features/abilities-scanner.md)); an oracle picture comparison of the objective markers and arrows (SCRIPT-330, needs `--rpc`, [script-system-hud-natives](features/script-system-hud-natives.md)); an oracle run of a turning script clip on the player (SCRIPT-324, [script-system-natives-animation](features/script-system-natives-animation.md)).
2. One worker per round on a wide-reach item from [priorities.md](priorities.md), whether or not mission 1-2 needs it. The trampoline's look and the table's wobble (LEVOBJ-S77, LEVOBJ-S79, LEVOBJ-90, [level-objects-wobbly](features/level-objects-wobbly.md)); the remaining callers of dropped studs (STUD-14, STUD-15, [studs](features/studs.md)); the other map icons (HUD-60, [hud-ui](features/hud-ui.md)); split [script-system-natives](features/script-system-natives.md) before its next item (near the 16 KB limit); oracle checks of the zipline A press and the pole A press (TRAV-8, TRAV-34, need user states).
3. The dog and the dog truck: animals are not created (LEVOBJ-7), the truck's moving bound and driving (SCRIPT-77, SCRIPT-43); cutscene 1_2_c and the mission end (SCRIPT-115) ([script-system-missions](features/script-system-missions.md)).
4. Open mission 1-1 gaps: the rest of the vehicle effects (VEH-121, VEH-123, VEH-124, VEH-128, PART-2), the studs and camera of the build (LEVOBJ-76, LEVOBJ-77), shadows (RENDER-33, RENDER-9), traffic and pedestrians (SCRIPT-230), camera classes (CAMERA-40, CAMERA-52).
5. Later: HUD fidelity, saves (FE-30, SAVE-14, SAVE-15), level switch screens (TRANS-13, TRANS-14), streaming (WSTREAM-30, TRANS-3), death and respawn (PLAYER-11), rendering (RENDER-60).

Oracle states (artifacts/oracle/states, see docs/re/oracle.md): `gameplay-start` (new game, Old 3DS), `police-station` and `street-after-1-1` (user saves after mission 1-1, New 3DS), `street-at-entrance`, `station-at-exit`, `station-exit-loadingpage`, `camera-lobby-walk`, `dcam-police-dialog`, `agent-doughnut-title` (agents, unmodified, New 3DS), `savecont-slotlist-user` (slot list with the user's save, Old 3DS), `trans-poked-*` (poked, measuring aids only). The real save of the user (read-only copy): `artifacts/oracle/agentJ-saves/savegame.dat`. Hard-to-reach states: ask the user (CLAUDE.md "Runtime oracle").

**Workflow notes:** oracle route and states: `tools/azahar_oracle.py` with `--display xvfb --state gameplay-start --renderer opengl --rpc --rpc-input`, route from boot `tools/oracle/to-gameplay.txt`; only one agent may use `--rpc` at a time. The lead delegates to subagents in worktrees (at most 3 in parallel, user), every brief names the CLAUDE.md rules, reviews each result against CLAUDE.md, rebuilds and tests, squash-merges one commit per branch with `tools/merge_agent.py prepare|check|finish` (it removes the worktree and branch), pushes when authorized, and checks the Graphify rebuild. Agents keep scratch files in their own subdirectory of the shared scratchpad. Worktree agents link `artifacts/` into their worktree (CLAUDE.md "Subagents"). Resume a branch with `git log main..<branch>`.

## Session Log

The newest 20 lines only; `python3 tools/check_doc_links.py --fix` moves older lines unchanged to [linux-port.progress-archive.md](linux-port.progress-archive.md). One line per session: timestamp, what was worked on, how far it got, link to the feature file. Findings, open items and stand-ins do not belong here (CLAUDE.md "Feature files"). Reverse-engineering details are in docs/re/ (see docs/re/README.md).

- 2026-10-19T11:21:22+02:00 — Lead (round 30 end, quick round, 3 Sonnet workers, no emulator): r30-C (every textured zipline line is drawn from both anchors, as the original's draw gate allows), r30-A (a prop moved by PositionObject collides at its new place) and r30-B (kinds 64 and 132 answer ResetObject with handlers of their own: pickups restore their placement, platforms only re-enable; the destroy releases the skeleton, not a collision body) merged — [player-traversal.md](features/player-traversal.md), [script-system-missions.md](features/script-system-missions.md), [script-system-natives-world.md](features/script-system-natives-world.md)
- 2026-10-19T12:56:21+02:00 — Agent r31-C (round 31, script animations on the player): the user-animation state 0x50 (update 0x2a47b0, enter 0x25fbec, StopCharAnim 0x1bfb84) read; PlayCharAnim, StopCharAnim and the character wait on Player1 now pose the script clip and hand back to locomotion; SCRIPT-270 closed, SCRIPT-319, SCRIPT-320 opened; see [script-system-natives-animation.md](features/script-system-natives-animation.md).
- 2026-10-19T12:55:51+02:00 — Agent r31-E (round 31, attack sounds): read the swing (enter 0x307e7c) and hit (event 0x319ffc) sound calls, both from the weapon table row of the character; the ids are 0 for every character, so the attack is silent; ported as data-driven `attack_sounds`; PLAYER-17 closed, PLAYER-23/24, PLAYER-S11, AUDIO-34 opened. See [player-movement.md](features/player-movement.md).
- 2026-10-19T12:59:04+02:00 — Agent r31-B (round 31, fireman cats): read the cat rescue use spot (create 0x1829ac, clips 0x182a5c, states 0x1c6aa8, message 0x18e1b8); the four cats of lvl_01 now play `idle_sat` looping and the spot's state machine is ported (`CatRescueSet`); LEVOBJ-99 closed, LEVOBJ-108 and LEVOBJ-109 open (rescue needs the fireman ability), LEVOBJ-S88 and LEVOBJ-S89 opened. See [level-objects.md](features/level-objects.md).
- 2026-10-19T13:01:38+02:00 — Agent r31-A (round 31, the dog of cutscene 1_2_c): the actor builder 0x2d1ddc binds any entity with a matching CSCharacter, props included, so the cutscene poses the placed prop `1-2_Dog` and the port no longer draws a dog of its own; props without a scene key of their own keep a model of their own (LEVOBJ-S86); LEVOBJ-100 closed, LEVOBJ-106 and LEVOBJ-107 open; [level-objects.md](features/level-objects.md).
- 2026-10-19T13:25:20+02:00 — Agent r31-D (round 31, character death): message 1 of the character handler (kinds 27, 121, 242) read and the fall-apart body (pool, parts, motion, lifetime) and the bricks ported; a killed goon of lvl_02 now falls apart; SCRIPT-305 closed, SCRIPT-S246 narrowed, SCRIPT-321..323 and SCRIPT-S264, SCRIPT-S265 opened, see [script-system-natives-world-death.md](features/script-system-natives-world-death.md).
- 2026-10-19T13:31:51+02:00 — Lead (round 31 end, 5 workers by the user's exception, 4 Sonnet + 1 Opus, no emulator): r31-C (script clips on Player1 through the user-animation state), r31-E (attack sounds ported from the weapon table; all ids 0, the attack stays silent), r31-B (fireman cats sit on their rescue spots), r31-A (cutscene 1_2_c poses the level prop 1-2_Dog; level-objects split, wobbly props and bouncers in their own file) and r31-D (killed minifig characters fall apart) merged — [script-system-natives-animation.md](features/script-system-natives-animation.md), [player-movement.md](features/player-movement.md), [level-objects.md](features/level-objects.md), [level-objects-wobbly.md](features/level-objects-wobbly.md), [script-system-natives-world-death.md](features/script-system-natives-world-death.md)
- 2026-10-19T13:44:20+02:00 — Agent r32-A (round 32, root motion of script clips on Player1): read 0x215804, the mover 0x20cd30 with flag 1 and 0x2c9770; a script clip's root step now moves and turns the player (no heading step for the player, no gravity function in state 0x50); SCRIPT-319 closed, SCRIPT-324 / 325 and stand-ins S266 / S267 open; see [script-system-natives-animation.md](features/script-system-natives-animation.md).
- 2026-10-19T13:48:01+02:00 — Agent r32-B (round 32, cutscene actor entity): the cutscene setup 0x2d1524 enables a disabled actor and saves its node matrix, the teardown 0x2d135c restores both, and entity byte +0x18 = 5 is a room-activity class; the port holds actor entities (CutsceneHold, drawn while held, prop matrix restored); LEVOBJ-105 and LEVOBJ-107 closed, LEVOBJ-110 and LEVOBJ-111 open; [level-objects.md](features/level-objects.md).
- 2026-10-19T14:06:37+02:00 — Agent r32-C (round 32, dropped studs and the respawn tail): the stud drop 0x21e2e4 modes 0..2 / 0x19913c with its update, draw rule and the 0x2149ac request ported as `StudDropField` (STUD-11 closed, callers STUD-12); the dead character's drop is mode 1 with count 0 and drops nothing (SCRIPT-321 closed); state DEAD, its spawn counter and the after-death 0x21e218 ported as far as read (SCRIPT-322 closed, SCRIPT-326..328 open); CTest `stud-drop`. [studs.md](features/studs.md), [script-system-natives-world-death.md](features/script-system-natives-world-death.md)
- 2026-10-19T14:14:40+02:00 — Agent r32-E (round 32, scanner look): the cyan wash is the material override 0x530834 that the audio mode switches on (with no fog); it is ported through the texenv stages and `Scene::setMaterialOverride`, checked in the oracle on m12-8, and there is no green dog; overlay leftovers read; SCAN-8 and SCAN-9 closed, SCAN-12, SCAN-13, SCAN-S9 and RENDER-68 (glow post-process) open; see [abilities-scanner.md](features/abilities-scanner.md).
- 2026-10-19T14:20:50+02:00 — Lead (round 32 end, 5 workers by the user's request, 3 Sonnet + 2 Opus, emulator with `--rpc` for r32-E only): r32-D (README rewritten as a player's quick start, CLAUDE.md rule), r32-A (script clips move the player by their root motion), r32-B (cutscene actors held and restored at the end), r32-C (dropped studs ported; a dead character drops none, friendly ones return to their placement) and r32-E (the scanner view washed cyan by the material override) merged; the lead corrected the README's version wording (other regions untested, not unsupported) — [script-system-natives-animation.md](features/script-system-natives-animation.md), [level-objects.md](features/level-objects.md), [studs.md](features/studs.md), [script-system-natives-world-death.md](features/script-system-natives-world-death.md), [abilities-scanner.md](features/abilities-scanner.md)
- 2026-10-19T14:37:16+02:00 — Agent r33-C (round 33, first callers of the dropped studs): 0x1aad90 is the build-it state change, not the car break (which and the forcing 0x22ca88 callers drop no stud); the wobble hit 0x2bc370 and the finished build-it now drop studs through `StudDropField`; STUD-12 and LEVOBJ-88 closed, STUD-14, STUD-15, LEVOBJ-112 open; see [studs.md](features/studs.md).
- 2026-10-19T14:45:21+02:00 — Agent r33-B (round 33, glow post-process): 0x2bf830 read (copies to 256 x 128 and 128 x 64, three blurs with table modes 0xc / 0xd, add with mode 0x12 and blend one / one) with its tables and strengths in the oracle, ported as `src/runtime/glow.*` and `src/render/glow.*` with `glow.{vert,frag}.glsl` for the scanner view; mean glow delta of the port (6.0, 49.6, 23.0) against the oracle (6.3, 48.2, 21.0); RENDER-68 closed, RENDER-69 / 70 open (see docs/plans/features/rendering.md)
- 2026-10-19T15:05:12+02:00 — Agent r33-A (round 33, objective markers and arrows): ported the SetActiveWayPoint chain 0x229c98 (disable fade), the sub-objective manager update 0x2ed8d8 / 0x2be934 with the group map slots and target icons, and the arrow natives drawn as world quads; SCRIPT-71, SCRIPT-72 and SCRIPT-74 closed, HUD-60 narrowed; no oracle pictures (no `--rpc` this round): [script-system-hud-natives.md](features/script-system-hud-natives.md), [hud-ui.md](features/hud-ui.md)
- 2026-10-19T15:09:12+02:00 — Agent r34-A (round 34, camera quick items): the camera records now skip bounds switched off by EnableBound (volume filter 0x20bf6c reads bound +0xa; CAMERA-7 closed) and ShakeScreen shakes the gameplay camera's eye (start 0x22c500, manager update 0x23f174, `ScreenShake`); open: moving bound matrices, the per-record shake 0x1c607c, other 0x22c500 callers. See [camera.md](features/camera.md), [script-system-dcam.md](features/script-system-dcam.md).
- 2026-10-19T15:13:37+02:00 — Agent r34-C (round 34, null script calls and placement effects): CallScript / SpawnScript fault on a null script in the original and all 894 calls of the main levels resolve, so the runner stops with an error (SCRIPT-S5 closed); message 8 to the player and ctrl+0x315 bits 0x14 read and recorded as `PlacementEffects`, a seated PositionPlayer moves the vehicle, props have no physics body (SCRIPT-98, SCRIPT-S255 closed); consumers open as SCRIPT-335, SCRIPT-336, PLAYER-25; see [script-system-natives.md](features/script-system-natives.md).
- 2026-10-19T15:30:57+02:00 — Agent r34-B (round 34, state change order on Player1): a ctrace of a jump shows a request is applied at the end of the next update (exit and enter by 0x226100) after the previous state's update, the new state's update one update later; the hand-back enter 0x1abbb0 clears ctrl+0x2b4/0x2b8 and starts the idle at blend 0.1; PlayerScriptAnimation, its glue and tests follow that order (SCRIPT-286 and SCRIPT-320 closed, SCRIPT-333, SCRIPT-334, SCRIPT-S272 open); the 0.1 crossfade stays ANIM-S13 with the new ANIM-14: [script-system-natives-animation.md](features/script-system-natives-animation.md).
- 2026-10-19T15:35:47+02:00 — Lead (round 33 end): merged r33-C (dropped studs from wobble hits and finished build-its), r33-B (glow post-process of the scanner view) and r33-A (objective markers, arrows and group map icons) through `tools/merge_agent.py` with all tests passing; released their ID ranges; pushed main ([studs](features/studs.md), [rendering](features/rendering.md), [script-system-hud-natives](features/script-system-hud-natives.md)).
- 2026-10-19T15:35:48+02:00 — Lead (round 34 end, started alongside r33-A on user request): merged r34-A (EnableBound in camera records, ShakeScreen), r34-C (null script calls stop, seated PositionPlayer, placement effects) and r34-B (player state change order) with all tests passing; released their ID ranges; pushed main ([camera](features/camera.md), [script-system](features/script-system.md), [script-system-natives-animation](features/script-system-natives-animation.md)).
