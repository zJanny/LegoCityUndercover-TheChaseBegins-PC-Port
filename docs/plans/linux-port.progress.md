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

**Priorities (critical path), updated 2026-10-19T10:27:58+02:00:** oracle states for every mission 1-2 step: `m12-1` to `m12-14`, `m12-10a-pole-foot`, `m12-10b-pole-side`, `m12-table-before`, `m12-table-broken`, `m12-trampoline-built`, vehicle effects: `veh-smash-prop`, `veh-smash-car`, `veh-break-low-hearts`, `veh-wall-full-speed` (docs/re/oracle-states.md); more on request from the user (new state requests deferred by the user on 2026-10-18). Rounds run three workers (user, 2026-10-18: short rounds); [priorities.md](priorities.md) ranks open items by their reach in the whole game and, when two need `--rpc`, one emulator servant that owns the only `--rpc` session.
1. Mission 1-2 playtest gaps: the user's playtest of mission 1-2 (flag poles, trampoline frame, zipline rope, scanner overlay and markers, detective footprints, the dog at the cage and its release, the mission end); the user's look at the punch (PLAYER-19, [player-movement](features/player-movement.md)); the footprints at the T-shirt spot (DETECT-16, DETECT-17: wait for a save state the user will provide, user 2026-10-19; remind the user in every round report until it arrives, [abilities-detective](features/abilities-detective.md)); the released dog's PositionObject (SCRIPT-288, LEVOBJ-100) and the cats (LEVOBJ-99, [level-objects](features/level-objects.md)); the scanner's cyan view and green dog (SCAN-8, [abilities-scanner](features/abilities-scanner.md)); character deaths (SCRIPT-305).
2. One worker per round on a wide-reach item from [priorities.md](priorities.md), whether or not mission 1-2 needs it. The trampoline's look and the table's wobble (LEVOBJ-S77, LEVOBJ-S79, LEVOBJ-90), attack sounds (PLAYER-17); oracle checks of the zipline A press and the pole A press (TRAV-8, TRAV-34, need user states).
3. The dog and the dog truck: animals are not created (LEVOBJ-7), the truck's moving bound and driving (SCRIPT-77, SCRIPT-43); cutscene 1_2_c and the mission end (SCRIPT-115) ([script-system-missions](features/script-system-missions.md)).
4. Open mission 1-1 gaps: the rest of the vehicle effects (VEH-121, VEH-123, VEH-124, VEH-128, PART-2), the studs and camera of the build (LEVOBJ-76, LEVOBJ-77), shadows (RENDER-33, RENDER-9), traffic and pedestrians (SCRIPT-230), camera classes (CAMERA-40, CAMERA-52).
5. Later: HUD fidelity, saves (FE-30, SAVE-14, SAVE-15), level switch screens (TRANS-13, TRANS-14), streaming (WSTREAM-30, TRANS-3), death and respawn (PLAYER-11), rendering (RENDER-60).

Oracle states (artifacts/oracle/states, see docs/re/oracle.md): `gameplay-start` (new game, Old 3DS), `police-station` and `street-after-1-1` (user saves after mission 1-1, New 3DS), `street-at-entrance`, `station-at-exit`, `station-exit-loadingpage`, `camera-lobby-walk`, `dcam-police-dialog`, `agent-doughnut-title` (agents, unmodified, New 3DS), `savecont-slotlist-user` (slot list with the user's save, Old 3DS), `trans-poked-*` (poked, measuring aids only). The real save of the user (read-only copy): `artifacts/oracle/agentJ-saves/savegame.dat`. Hard-to-reach states: ask the user (CLAUDE.md "Runtime oracle").

**Workflow notes:** oracle route and states: `tools/azahar_oracle.py` with `--display xvfb --state gameplay-start --renderer opengl --rpc --rpc-input`, route from boot `tools/oracle/to-gameplay.txt`; only one agent may use `--rpc` at a time. The lead delegates to subagents in worktrees (at most 3 in parallel, user), every brief names the CLAUDE.md rules, reviews each result against CLAUDE.md, rebuilds and tests, squash-merges one commit per branch with `tools/merge_agent.py prepare|check|finish` (it removes the worktree and branch), pushes when authorized, and checks the Graphify rebuild. Agents keep scratch files in their own subdirectory of the shared scratchpad. Worktree agents link `artifacts/` into their worktree (CLAUDE.md "Subagents"). Resume a branch with `git log main..<branch>`.

## Session Log

The newest 20 lines only; `python3 tools/check_doc_links.py --fix` moves older lines unchanged to [linux-port.progress-archive.md](linux-port.progress-archive.md). One line per session: timestamp, what was worked on, how far it got, link to the feature file. Findings, open items and stand-ins do not belong here (CLAUDE.md "Feature files"). Reverse-engineering details are in docs/re/ (see docs/re/README.md).

- 2026-10-18T22:10:30+02:00 — Lead (round 25 end): r25-A (acrobat pole states read, next-pole choice and facing ported, not wired), r25-B (raw matrix joint curves decoded, punch not yet seen on screen), r25-C (object kinds, ToggleUseable group case, EnableShadow and EnableSwitch) and r25-D (virtual motion source in the oracle; the scanner view renders) merged; the servant measured the trampoline frame; the user stopped the round early (usage limit) — [player-traversal-rails.md](features/player-traversal-rails.md), [player-movement.md](features/player-movement.md), [script-system-natives-world.md](features/script-system-natives-world.md), [viewer-tools.md](features/viewer-tools.md), [abilities-scanner.md](features/abilities-scanner.md)
- 2026-10-19T07:58:13+02:00 — Agent r26-C (round 26, kind gates): the use-zone registry has one adder (0x23e130, 52 kinds), so ToggleUseable and EnableSwitch write the flag only for those kinds; the EnableShadow kind list and bit 14 are traced; no handler of the 13 kinds of enableobjectfade answers message 24, so it ends like EnableObject (kinds 27 and 102 answer it, not read). See [script-system-natives-world.md](features/script-system-natives-world.md)
- 2026-10-19T08:03:47+02:00 — Agent r26-B (round 26, trampoline frame and punch root step): a placed object's wall triangle blocks only while the player's box centre is in front of its plane (0x209a98), so the built trampoline frame stops a run from outside and lets a walk from inside cross (floor-only stand-in removed, test `prop-wall-side`); the combo root step is the extra joint curve (stand-in track replaced). See [level-objects.md](features/level-objects.md) and [player-movement.md](features/player-movement.md).
- 2026-10-19T08:12:59+02:00 — Agent r26-A (round 26, acrobat poles): the flag pole states 93..98 (idle, flip, jumps, jump to the fall and to the landing point) ported in `src/runtime/acrobat_pole.cpp` and wired into `chase-game` after the arrival; replays of t1, t2 and a new flip trace pass and a scripted run crosses the four poles of mission 1-2; TRAV-37/39/40/41 closed, leftovers TRAV-42..45; [player-traversal-rails.md](features/player-traversal-rails.md).
- 2026-10-19T08:20:57+02:00 — Lead (round 26 end): r26-C (use-zone and shadow kind gates, enableobjectfade), r26-B (placed-object walls block only from the front, the trampoline frame stops a walk from outside; combo root step from the extra joint curve) and r26-A (flag pole states 93..98 wired into `chase-game`; the lead ran the real path from the trampoline bounce to the landing point) merged — [player-traversal-rails.md](features/player-traversal-rails.md), [level-objects.md](features/level-objects.md), [player-movement.md](features/player-movement.md), [script-system-natives-world.md](features/script-system-natives-world.md)
- 2026-10-19T09:11:41+02:00 — Agent r27-E (round 27, detective footprints): footprints now carry the path orientation (0x270b70, 0x248870) and stand on the ray test's ground instead of the player probe (0.24 too high on the dog house stairs); oracle traps E-1 and E-2 give identity rotation for the dog paths and 13 hit points the port reproduces; DETECT-14 closed, DETECT-S3 removed, DETECT-S9 / DETECT-15 / DETECT-16 opened ([abilities-detective.md](features/abilities-detective.md)).
- 2026-10-19T08:52:07+02:00 — Agent r27-D (round 27, zipline rope): the rope of the zipline (descriptor draw 0x1a9dc4, batch 0x297b7c, strip 0x1f9590) is drawn in `chase-game` as a straight textured ribbon with the fade near the camera; TRAV-7 closed, facing and render states open as TRAV-46 / TRAV-47, tightrope ropes TRAV-48; see [player-traversal.md](features/player-traversal.md).
- 2026-10-19T08:53:59+02:00 — Agent r27-B (round 27, scanner timing and look): real-time scanner counts measured with the motion source (state 0x157 +49 updates, enter callback +20, camera +56, lock 20, scan 39, script exit 57; no hang) and ported; the audio scanner overlay (frame, zoom meter, headphone icon, reticle, scan meter) drawn from the DRC_Scanner layouts; SCAN-4 and SCAN-1 closed, SCAN-7..9 open, see [abilities-scanner.md](features/abilities-scanner.md).
- 2026-10-19T09:05:37+02:00 — Agent r27-C (round 27, destroy message): no class handler returns non-zero for message 1, so DestroyProp / KillObject destroy every kind and hide only the classes that disable (kind table `destroy_message.*`, Dead event and prop TriggerObject, EnableObject re-shows); a car smash uses the same `ScriptRunner::destroyEntity`, SCRIPT-280 and VEH-129 closed, SCRIPT-302..304 and the unread kinds open: [script-system-natives-world.md](features/script-system-natives-world.md)
- 2026-10-19T09:30:43+02:00 — Agent r27-A (round 27, dog cage of mission 1-2): the black screen after the dog release is fixed (one screen fade shared by FadeScreen and the cutscene mode, gdb-traced from `m12-13-cage-trigger`), the release runs on to `1-2_MissionEnd` and `1-3_MissionStart`; the use marker on the ground is ported for use spots; SCRIPT-292 and SCRIPT-293 closed, SCRIPT-298 and SCRIPT-299 open: [script-system-missions.md](features/script-system-missions.md).
- 2026-10-19T09:36:51+02:00 — Lead (round 27 end, 5 workers + servant): r27-D (zipline rope drawn), r27-B (scanner real-time timing and overlay), r27-C (destroy per class, car smashes fire TriggerObject), r27-E (footprints on the ground ray, turn ported; the wrong way at the later spot stays open) and r27-A (shared screen fade: no black screen after the dog release, mission 1-2 ends; use marker drawn) merged — [script-system-missions.md](features/script-system-missions.md), [abilities-scanner.md](features/abilities-scanner.md), [abilities-detective.md](features/abilities-detective.md), [player-traversal.md](features/player-traversal.md), [script-system-natives-world.md](features/script-system-natives-world.md)
- 2026-10-19T10:00:50+02:00 — Agent r28-D (round 28, footprint card and rope): the dog footprint card is placed with its model's node matrix (it was drawn about 3 m off; DETECT-16 narrowed, the on-screen check waits for a user state), the rope's facing row 0x529fec is the camera's view direction and its render state is read (TRAV-46, TRAV-47 closed; sampler and fog open as TRAV-49, TRAV-50); see [abilities-detective.md](features/abilities-detective.md) and [player-traversal.md](features/player-traversal.md).
- 2026-10-19T10:01:53+02:00 — Agent r28-B (round 28, scanner view): sound markers (the large green rings, per-object centre pip and three pulses) and the audio / exit buttons of the map screen are drawn in `chase-game`; exit by B measured (57 updates, SCAN-7 closed); the cyan wash was searched for and not found (SCAN-8 narrowed). See [abilities-scanner.md](features/abilities-scanner.md).
- 2026-10-19T10:06:35+02:00 — Agent r28-C (round 28, destroy effects per kind): message 1 of the hit machine (67, 204), the pushable (71) and the default handler (64, 132) read and ported (explosion definition, bricks, destroy sound; objects without a collision entry take the model bounds), kinds 27 / 242 / 102 read but not ported, the second and third Dead broadcast have no consumer in any level file; SCRIPT-303 closed, SCRIPT-305..307 and SCRIPT-S250..S252 opened, see [script-system-natives-world.md](features/script-system-natives-world.md).
- 2026-10-19T10:13:24+02:00 — Agent r28-E (round 28, screen fade draw and cutscene end fades): the fade draw 0x24bda4 (black quad of opacity |1 - value|), its screen gate 0x1087ec and the screen bytes of every fade are read and ported (cutscene fades and FadeScreen cover the top screen only), and the cutscene end runs the code's fade out, mode-switch wait and fade in instead of the measured veil; SCRIPT-298 closed, SCRIPT-S244 and MOVIE-S20 replaced by SCRIPT-S253/S254 and MOVIE-S34, open: SCRIPT-308..310, MOVIE-36, MOVIE-37 (see [movies-cutscenes.md](features/movies-cutscenes.md) and [script-system-missions.md](features/script-system-missions.md)).
- 2026-10-19T10:18:46+02:00 — Agent r28-A (round 28, mission 1-2 dog): the dogs, fireman cats and crow of lvl_01 are drawn as props (their kinds' creates build the model like any prop), and props with a playing clip move by its root motion, so the released dog jumps out of the cage and ends where a GDB trap of the original has it; LEVOBJ-7 narrowed to ride_pig and sharks, LEVOBJ-98 closed, LEVOBJ-99..102 and LEVOBJ-S82..S84 opened. See [level-objects.md](features/level-objects.md).
- 2026-10-19T10:27:58+02:00 — Lead (round 28 end, 5 workers + servant): r28-D (dog footprint card placed by its node matrix; rope facing and render state measured), r28-B (scanner sound pulse markers, map-screen buttons, exit by B), r28-C (destroy effects per handler kind), r28-E (screen fade drawn from the code, cutscene end fades) and r28-A (the dogs, cats and crow drawn as props; props move by root motion, the released dog ends where the original puts it) merged — [level-objects.md](features/level-objects.md), [abilities-scanner.md](features/abilities-scanner.md), [abilities-detective.md](features/abilities-detective.md), [script-system-natives-world.md](features/script-system-natives-world.md), [movies-cutscenes.md](features/movies-cutscenes.md)
- 2026-10-19T10:38:36+02:00 — Agent r29-E (round 29, priorities links): each item link in priorities.md opens the source view of its feature file at the item line, checked and followed by check_doc_links; rendered landing stays open (TOOLS-17), see [viewer-tools.md](features/viewer-tools.md).
- 2026-10-19T10:38:41+02:00 — Agent r29-C (round 29, fog on the zipline rope): the rope is drawn with the scene's fog through a fog switch in the shared particle program (particles unchanged), lvl_01 fog values pinned for the rope; the double draw and an oracle picture stay open. [player-traversal.md](features/player-traversal.md)
- 2026-10-19T10:40:06+02:00 — Agent r29-A (round 29, PositionObject of props): placed props and objects take the target's matrix (scene node, room, root-motion base) so the released dog is placed at the dog house; collision and physics follow-up open (SCRIPT-311); SCRIPT-288 closed, [script-system-missions.md](features/script-system-missions.md).
