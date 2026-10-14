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

**Priorities (critical path), updated 2026-10-14T16:54:21+02:00:**
1. Mission flow: AI movement and path finding (SCRIPT-79) and the rest of the AI (SCRIPT-42), the other AI trigger events (SCRIPT-75), `PositionPlayer` (SCRIPT-59), readers for recorded values (SCRIPT-56, SCRIPT-64), towards SCRIPT-1: [script-system](features/script-system.md), [script-system-natives](features/script-system-natives.md), [script-system-triggers](features/script-system-triggers.md); arrow and map natives (SCRIPT-72, SCRIPT-73): [script-system-hud-natives](features/script-system-hud-natives.md).
2. HUD fidelity: the other text screens and the dialogue background fade (HUD-22), heading start and box width (HUD-23), text layout inside the panels (HUD-21), map marker and waypoint animation (HUD-16, HUD-17): [hud-ui](features/hud-ui.md).
3. Saves: slot list Continue (FE-11, then FE-1), Continue at later levels (SAVE-13), level state refresh and live inputs of a save (SAVE-14, SAVE-15): [saves-options](features/saves-options.md), [front-end](features/front-end.md).
4. Streaming and transitions: current-room rule and load timing (WSTREAM-13), characters and studs of unloaded rooms (WSTREAM-14), the master level as scenes (TRANS-3): [world-streaming](features/world-streaming.md), [level-transitions](features/level-transitions.md).
5. Camera, objects, rendering: follow reset (CAMERA-10, CAMERA-9), disabled characters and sub-objects (LEVOBJ-12), vehicles (LEVOBJ-7, SCRIPT-43), player shadow (RENDER-9), fragment lights (RENDER-1), fog oracle comparison and sky (RENDER-30, RENDER-36): [camera](features/camera.md), [level-objects](features/level-objects.md), [rendering](features/rendering.md).

Oracle states (artifacts/oracle/states, see docs/re/oracle.md): `gameplay-start` (new game, Old 3DS), `police-station` and `street-after-1-1` (user saves after mission 1-1, New 3DS), `street-at-entrance`, `station-at-exit`, `station-exit-loadingpage`, `camera-lobby-walk`, `dcam-police-dialog`, `agent-doughnut-title` (agents, unmodified, New 3DS), `savecont-slotlist-user` (slot list with the user's save, Old 3DS), `trans-poked-*` (poked, measuring aids only). The real save of the user (read-only copy): `artifacts/oracle/agentJ-saves/savegame.dat`. Hard-to-reach states: ask the user (CLAUDE.md "Runtime oracle").

**Workflow notes:** oracle route and states: `tools/azahar_oracle.py` with `--display xvfb --state gameplay-start --renderer opengl --rpc --rpc-input`, route from boot `tools/oracle/to-gameplay.txt`; only one agent may use `--rpc` at a time. The lead delegates to subagents in worktrees (at most 3 in parallel, user), every brief names the CLAUDE.md rules, reviews each result against CLAUDE.md, rebuilds and tests, squash-merges one commit per branch with `tools/merge_agent.py prepare|check|finish` (it removes the worktree and branch), pushes when authorized, and checks the Graphify rebuild. Agents keep scratch files in their own subdirectory of the shared scratchpad. Worktree agents link `artifacts/` into their worktree (CLAUDE.md "Subagents"). Resume a branch with `git log main..<branch>`.

## Session Log

The newest 20 lines only; `python3 tools/check_doc_links.py --fix` moves older lines unchanged to [linux-port.progress-archive.md](linux-port.progress-archive.md). One line per session: timestamp, what was worked on, how far it got, link to the feature file. Findings, open items and stand-ins do not belong here (CLAUDE.md "Feature files"). Reverse-engineering details are in docs/re/ (see docs/re/README.md).

- 2026-10-14T14:37:10+02:00 — Agent L: director camera (StartDCam, StartDCamTrack, WaitForDCam, EndDCam, wait setters, stand-ins for the other SCRIPT-28 natives), level paths decoded; police entry timing and a two-point path match the oracle → [script-system](features/script-system.md)
- 2026-10-14T14:37:20+02:00 — Agent H: debug overlay in chase-game (F3 / DebugOverlay binding, --debug-overlay; FPS, frame time, memory, CPU, level values), tests and user doc → [viewer-tools](features/viewer-tools.md)
- 2026-10-14T14:43:22+02:00 — Agent B2: natives batch SCRIPT-27 read (21 handlers), 5 run exactly (UnlockLevel, CountRacesWon, IsRacingCarAwarded, GetCharacterUnlocked, IsRedbrickBought), 16 as stand-ins on a slot byte image → [script-system](features/script-system.md)
- 2026-10-14T15:06:09+02:00 — Agent D2: LEVOBJ-11 and LEVOBJ-1 closed: the pale disc was the disabled Police_Plaque_Floor; props the start state disables are no longer drawn; tests police-entrance-no-disc, level-start-real-data → [level-objects](features/level-objects.md)
- 2026-10-14T15:10:29+02:00 — Agent W: room node position derived from the room models (43 of 43 oracle values), RoomStreamer applies the load rule every update in chase-game with room visibility, loaded-room bound rule and debug overlay count; stand-ins for the current room and load time → [world-streaming](features/world-streaming.md)
- 2026-10-14T15:12:26+02:00 — Agent R2: all OpenGL calls moved into `src/render/` behind plain-type headers (game and viewer code no longer include epoxy or use GL types); CTest `gl-only-in-render` added; screenshots match main except the frames with live timings → [rendering](features/rendering.md)
- 2026-10-14T15:19:40+02:00 — Agent C: SAVE-9 closed: chase-game --save starts the level of the slot (byte +6) at Player1Start, verified on the oracle for level 1 (Continue chain read); FE-1 stays open (slot list page not measured, FE-11) → [saves-options](features/saves-options.md)
- 2026-10-14T15:24:20+02:00 — Agent X: batch SCRIPT-30 (16 text natives, stand-ins) and SCRIPT-41 text part: tip text and dialogue lines drawn in chase-game with the measured fade; tests script-text, tip-text-shot → [script-system](features/script-system.md)
- 2026-10-14T15:43:54+02:00 — Agent OS: docs/re/oracle.md split by subtopic (setup, RPC, states, routes, measuring) to keep it under 40 KB; the guide and links updated → [viewer-tools](features/viewer-tools.md)
- 2026-10-14T15:54:32+02:00 — Agent TT: trigger check functions of all bound types transcribed and implemented (script_triggers.*), BoundLeft / PlayerOnFootBoundEntered verified in the oracle, AI and vehicle types split off → [script-system](features/script-system.md)
- 2026-10-14T15:55:02+02:00 — Agent HD: HUD-14 closed: tip panel and dialogue box layout read from the draw code (0x15b518, 0x1cc550, 0x23c780, 0x229128), badge at natural size, dialogue fade from code; box extents equal the oracle pictures; HUD-13 not started beyond the portrait formula (HUD-19) → [hud-ui](features/hud-ui.md)
- 2026-10-14T16:01:35+02:00 — Agent OH: objective natives of batch 31 (SetActiveWayPoint, SubObjectiveGoToEnable registry), waypoint object and map marker in chase-game, measured on the oracle; group effect and arrows open → [script-system](features/script-system.md)
- 2026-10-14T16:34:42+02:00 — Agent SV: save request chain 0x21cf38 and slot commit transcribed (SaveRequestWriter, --save-out, round trip byte-exact on the user's save copy), slot row fields (play time, percent, location) decoded; level state refresh and live summary inputs open (SAVE-14, SAVE-15); SAVE-10 and SCRIPT-63 closed → [saves-options](features/saves-options.md)
- 2026-10-14T16:37:00+02:00 — Agent AI: AI core and batch SCRIPT-34 (17 AI natives as stand-ins, AIMove walks the player and fires the AIMove trigger, Police_ExtToInt runs to its SceneChange); movement, pursuit and the other AI events open → [script-system](features/script-system.md)
- 2026-10-14T16:52:00+02:00 — Agent DOCS: docs workflow rules enforced by check_doc_links --fix (log newest 20 + archive, feature files 16 KB, closed items to .closed.md, duplicate IDs), script-system and three docs/re files split by subtopic, merge_agent runs --fix → [script-system](features/script-system.md)
- 2026-10-14T16:54:00+02:00 — Agent HUD2: HUD-19 and HUD-20 closed (portrait and hexagon frame formula verified on the oracle, tip alpha writer 0x2de284 found), level heading box drawn (HUD-13 split into HUD-22/HUD-23, stand-ins HUD-S12/S13) → [hud-ui](features/hud-ui.md)
- 2026-10-14T17:04:41+02:00 — Lead: doc-links also checks that every feature file has a row in the feature index and that closed-items files have none (test_check_doc_links) → [features/README.md](features/README.md)
- 2026-10-14T17:35:00+02:00 — Agent FOG: original fog recovered (level FogStart/FogEnd/FogColour, 128-entry PICA table verified on 3 oracle captures, per-material rule) and drawn in the shader; oracle picture comparison and pop-in note open → [rendering](features/rendering.md)
- 2026-10-14T18:44:00+02:00 — Agent A (ai-move): AIFollowSimplePath ported as state 22 (arrival 0.5, index wrap, AIMove event argument 255 at 0x2cbf10); AIMove path finder, state 11 and speeds read and recorded as items → [script-system-natives](features/script-system-natives.md)
- 2026-10-14T18:40:00+02:00 — Agent C (position-flags): doors skip their update while ToggleUpdateFlag is set (reader found in the entity update loop 0x1a4098, mask corrected to 0x4); PositionPlayer ground probe and controller reset read but need run-time data, not ported → [script-system-natives](features/script-system-natives.md)
