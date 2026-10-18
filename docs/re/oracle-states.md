# Runtime oracle: Save states and checkpoints

Subtopic of the Azahar runtime oracle. Start with [oracle.md](oracle.md) for setup and the fast workflow. Entries are dated; new entries go at the end.

Save states the oracle uses: which ones exist, the game progress of each, how each was made, and the New 3DS flag.

## Current state

- Procedure: start from a state with `--state NAME`; a state's `NAME.json` carries `new_3ds`, which the tool writes into the generated config (loading a New 3DS state in Old 3DS mode crashed with SIGILL); `savestate` writes the flag and `loadstate` refuses a mode mismatch. [§2026-10-13T23:20:34](#ts-20261013-232034), [§2026-10-14T11:30:36](#ts-20261014-113036)
- New 3DS mode runs the CPU faster than Old 3DS; use New 3DS states for pictures, camera, door and layout checks and repeat timing-sensitive measurements in Old 3DS mode. [§2026-10-13T23:20:34](#ts-20261013-232034)
- Use the OpenGL renderer: the software renderer crashed while loading `drv-police-car-seated` and states derived from it. [§2026-10-15T19:07:36](#ts-20261015-190736)
- Poked states (`trans-poked-*`) are measuring aids, not reference states; the heap addresses of a state are valid only for that state. [§2026-10-14T10:07:15](#ts-20261014-100715)
- States after mission 1-1 (user): `police-station`, `street-after-1-1`, `info-barrier`, `mission-1-2-start` (1-2 not started, Auburn locked), `in-vehicle` (van_dulmen, slot 5); agent-made from them: `street-at-entrance`, `station-at-exit`, `station-exit-loadingpage`, `info-box-open`. [§2026-10-14T10:20:23](#ts-20261014-102023), [§2026-10-14T11:13:12](#ts-20261014-111312), [§2026-10-14T12:04:04](#ts-20261014-120404), [§2026-10-14T19:17:30](#ts-20261014-191730), [§2026-10-14T23:10:52](#ts-20261014-231052)
- Mission 1-1 in progress (user): `m11-1-donut-shop` to `m11-9-delivery-goal`, with player positions in the table of the entry; `m11-7-box-prompt` is made from `m11-7-collect-donuts` for the carry pictures. [§2026-10-14T23:27:10](#ts-20261014-232710), [§2026-10-16T09:35:36](#ts-20261016-093536)
- Agent states with progress: `cutscene-1-1-b` (mission 1-1 started, Old 3DS), `drv-police-car-seated` (step 2, player in the police car), `bullet-trooper-at-obstacle` (from it, trooper resting at the obstacle); `savecont-slotlist-user` and `slotlist-delete-confirm` hold the user's save slot 0 after mission 1-1. [§2026-10-14T20:46:10](#ts-20261014-204610), [§2026-10-15T09:35:00](#ts-20261015-093500), [§2026-10-15T19:07:36](#ts-20261015-190736), [§2026-10-14T15:22:24](#ts-20261014-152224), [§2026-10-14T20:46:10](#ts-20261014-204610)
- Mission 1-2 in progress (user): `m12-1-mission-trigger` to `m12-8-roof-sound-scanner`, positions in the table of the entry; the sound scanner view is white in Azahar (user report), so its pictures are no reference. [§2026-10-18T10:13:06](#ts-20261018-101306)
- Rest of mission 1-2 (user, loaded and checked): `m12-9-zipline` to `m12-13-cage-trigger`, and `m12-14-mission-complete` with mission 1-2 finished; positions in the table of the entry. [§2026-10-18T12:27:39](#ts-20261018-122739)
- Vehicle brick effects (user): `veh-smash-prop` (lamp post), `veh-smash-car` (moving car ahead), `veh-break-low-hearts` (one heart left). [§2026-10-18T12:45:56](#ts-20261018-124556)
- `veh-wall-full-speed` (user): police car with four hearts at speed towards a brick wall, for VEH-128. [§2026-10-18T15:06:16](#ts-20261018-150616)
- Pole mount (user, for TRAV-1): `m12-10a-pole-foot` (on the ground in front of the drain pipe, facing away), `m12-10b-pole-side` (0.5 m left of the pipe, facing the wall). [§2026-10-18T15:46:24](#ts-20261018-154624)
- `leadcut-1_1_d-early` (from `m11-6-van-to-shop`): cutscene `1_1_d` just loaded, for its actor nodes and pictures. [§2026-10-16T14:39:21](#ts-20261016-143921)
- Open: whether mission 1-2 is active in the post-1-1 states, the level variables of the m11 states, a save with Continue byte +6 above 1, and the Old 3DS run of `savestate`.

<a id="ts-20261013-232034"></a>

## 2026-10-13T23:20:34+02:00 — police-station checkpoint (from the user's own quick save) and New 3DS states

- The user played into the police station on their own Azahar (2126.2, same build) and quick-saved in slot 1. Copied read-only from `~/.local/share/azahar-emu/states/00040000000AD600.01.cst` to `artifacts/oracle/states/police-station.cst` (24 MB, like gameplay-start).
- The user's Azahar runs in New 3DS mode (`is_new_3ds=true`); the tool's config had Old 3DS. Loading the state into Old 3DS mode crashed Azahar with SIGILL in 3 of 3 attempts. A state's `NAME.json` may now carry `"new_3ds": true`; the tool then writes `is_new_3ds=true` into the generated config (`state_needs_new_3ds`). With that, `--state police-station --display xvfb --renderer opengl --rpc` loads it: the RPC screenshot shows Chase in the lobby between the entrance pillars, facing the reception desk; the player is held (intent 0, state 0).
- **Measurement caveat:** New 3DS mode runs the CPU faster than the Old 3DS mode every earlier measurement used. Use this state for pictures, camera, door and layout checks; repeat timing-sensitive measurements in Old 3DS mode (or with a state saved there) before pinning rates.

Unknowns:
- Whether game logic timing differs between the modes for this game (frame pacing measured only in Old 3DS mode).

<a id="ts-20261014-100715"></a>

## 2026-10-14T10:07:15+02:00 — Poked transition states (no RPC)

- Two measuring-aid states, both made from `gameplay-start` with GDB `poke` and therefore **not reference states** (no test may pin original behaviour from them): `trans-poked-bound-enabled` (after `frames 3`: `poke 0x086640f6 01` enables the bound `Police_Station_Transition_Lower` of lvl_01; player node world and local x, z set to 909.0 and 1115.83 with pokes at 0x08568c3c, 0x08568c44, 0x08568bfc, 0x08568c04; y unchanged) and `trans-poked-dialog-answered` (the former plus 34 updates, `WarningDialogBox` answered with A). The heap addresses are valid only for `gameplay-start` loaded under Old 3DS mode. The way to the enabled bound without a poke is mission 1-1 (`1-1_MissionEnd`); no state after it exists yet.
- Without RPC the player is placed by poking the node translations (world `node+0x8c`, local `node+0x4c`), which is what `teleport` does over RPC. The node of `gameplay-start` is 0x08568bb0.
- A heap scan without RPC: `memtrace 1 NAME ADDR:LEN ...` with 1 MB regions (0x08500000..0x08d00000) took 170 s and found the bound records by their float centres.
- Runs from the same loaded state were not repeatable update for update (the dialog opened after 34 updates in one run and after 125 in another). Compare values only inside one run.
- Findings: [streaming.md](streaming-transitions.md#ts-20261014-100715).

<a id="ts-20261014-102023"></a>

## 2026-10-14T10:20:23+02:00 — Addition to the police-station checkpoint: game progress

Addition to [the entry of 2026-10-13T23:20](#ts-20261013-232034) (user statement, 2026-10-14): the user's quick save `police-station` was made **after the first mission (1-1, bringing the donuts to the police station)**. It is an unmodified state of the original past `1-1_MissionEnd`, so it is the reference state for everything that the mission end enables (for example the `Police_Station_Transition_*` bounds, see [streaming-transitions.md §2026-10-14T10:07](streaming-transitions.md#ts-20261014-100715)). No state that played mission 1-1 is needed; walking out of the station from this state reaches the street with the post-mission flags.

Unknowns:
- Whether mission 1-2 had already started when the user saved (the chapter / mission variables were not read).

<a id="ts-20261014-111312"></a>

## 2026-10-14T11:13:12+02:00 — Street states after mission 1-1: `street-after-1-1` (user) and `street-at-entrance`

- `street-after-1-1`: the user's own quick save (slot 2, 10:25), `new_3ds: true`. The player stands just outside the police-station entrance at world (900.55, 16.57, 1110.28), facing -x, camera yaw -1.57. Game progress: after mission 1-1 (donut delivery), the same as `police-station`. Evidence from memory (no poke): the enable bytes of `Police_Station_Transition_Lower`, `_Upper` and `Police_ExtToInt` are 1 ([streaming-transitions.md §2026-10-14T11:13](streaming-transitions.md#ts-20261014-111312)); the mission variables were not read, so whether 1-2 is active is open. User note: a mission trigger lies a few metres in front of the player; it was not found among the bounds (the bounds near the start are disabled except `Police_Station_Transition_Lower`), and it was not triggered.
- `street-at-entrance`: made from `street-after-1-1` with GDB pad injection only (no pokes, no RPC): stick (-90, -120) for 30 updates, (40, 150) for 25, idle 8. Player at (903.72, 16.57, 1113.33), 0.5 m south of Lower (z 1113.83..1117.83), `new_3ds: true` (the `savestate` command does not copy that flag: the JSON was edited by hand). Reference state for the station entry: it is unmodified past mission 1-1 and the bound fires from it by itself.
- Walking: the stick is camera relative and the camera swings while it moves (yaw 0.2 to 0.8 within 15 updates), so the player cannot be steered exactly; a short walk into the bound from `street-at-entrance` is `stick 60 100` 6 updates, `stick 140 20` 4 updates, `stick 0 100` (enters at update 14 of the scenario).
- Costs (New 3DS, OpenGL, port 24706): a `trap` update with three stops takes about 4.7 s, one without stops about 0.5 s; `loadstate` plus 3 updates about 7 s. Trap runs from this state were repeatable update for update (three full runs, same update numbers).
- A `trap` run showed stops at the UI addresses 0x2c34b4 / 0x2a85a0 / 0x2a8658 on the update of the loading-page A press although they were not in the trap list; the same values appear in an earlier agent's trace. Origin not found (see the streaming entry).

Unknowns:
- Whether mission 1-2 is active in the two states.
- The trigger the user mentioned in front of the player.

<a id="ts-20261014-113036"></a>

## 2026-10-14T11:30:36+02:00 — New 3DS flag in saved states and session (TOOLS-6)

- `tools/azahar_oracle.py` now records the console mode. The launch writes `"new_3ds"` into `session.json`; `--attach` reads it (a missing key means Old 3DS). `savestate` writes `"new_3ds"` of the running emulator into `NAME.json`. `loadstate` in a running session raises `state NAME was saved in <mode> mode; start a new emulator with --state NAME` when the state's mode differs, before it copies anything into the emulator's state folder. This closes the crash of [§2026-10-13T23:20](#ts-20261013-232034) for states made by the tool.
- Tests: `tests/test_azahar_oracle_launch.py` (`SessionModeTest`, `StateModeTest`), 11 tests pass with `python3 -m unittest tests.test_azahar_oracle_launch`.
- Real run (agent K, own port 24760, home `artifacts/oracle/agentK-home`, states copied in `agentK-states`, xvfb, opengl, no RPC): `--state police-station --keep` loaded the New 3DS state; `wait 5`, `savestate k-check`, `loadstate k-check`, `wait 3` finished with exit 0 and the emulator stayed alive. `k-check.json` was `{"file": "00040000000AD600.00.cst", "frame": 0, "new_3ds": true}` and `session.json` had `"new_3ds": true`. A second `--attach` run with `loadstate gameplay-start` (Old 3DS, no `new_3ds` key) was refused with `state gameplay-start was saved in Old 3DS mode; start a new emulator with --state gameplay-start`; the emulator was still alive afterwards. `--stop` ended the emulator and Xvfb; no `azahar` or `Xvfb` process was left.

Unknowns:
- The Old 3DS save path was not run for `savestate` in an Old 3DS session (the unit test covers the JSON only), so the Old 3DS value of the written flag is checked by the test, not by an emulator run.
- Whether a state saved in New 3DS mode loads into a New 3DS session after a crash-free start is shown for one state (`police-station`) only.

<a id="ts-20261014-120404"></a>

## 2026-10-14T12:04:04+02:00 — Agent states for the police station exit

- `station-at-exit` (New 3DS, from `police-station` with the pad: 2 idle updates, stick (0,-150) for 7 updates, 6 idle; the player stands at (-1.970, 1.424, 18.224) in the lobby, 1.9 m in front of the exit bound `Lobby_to_CherryTree`, facing +z) and `station-exit-loadingpage` (same route with the stick held about 40 updates: the loading page waits for A). Both unmodified, with a `source` key; game progress as `police-station` (after mission 1-1). Findings: [streaming-transitions.md §2026-10-14T12:02](streaming-transitions.md#ts-20261014-120258).
- With `--rpc` and OpenGL, `screenshot` works in stepped runs and returns the page that the game shows at that moment (400x480). `trap` still reports the unrequested UI stops on the A press of the loading page (0x2c34b4, 0x2a85a0, ...): they are the page's own widget code (see the streaming entry), not a tool artefact.

<a id="ts-20261014-152224"></a>

## 2026-10-14T15:22:24+02:00 — State `savecont-slotlist-user` (user save, slot list)

- `savecont-slotlist-user` (agent C, `new_3ds: false`): cold boot with a read-only copy of the user's `savegame.dat` (`artifacts/oracle/agentJ-saves/`) in the emulator's own sdmc (`artifacts/oracle/agentC-home`), A on the HOME notice and the autosave notice, stopped at the slot list before a slot is chosen. Game progress: slot 0 used, after mission 1-1 (slot byte +6 = 1, police station), slot 1 empty. Continue on slot 0 starts lvl_00_policestation at `Player1Start` ([save-format-state.md §2026-10-14T15:19](save-format-state.md#ts-20261014-151903)). The user's own Azahar saves were not touched.

Unknowns: a save with byte +6 above 1 for Continue at later levels (SAVE-13).

<a id="ts-20261014-191730"></a>

## 2026-10-14T19:17:30+02:00 — User states `info-barrier` and `mission-1-2-start`

Both are the user's own Azahar quick saves (2026-10-14 19:16), copied read-only and byte-identical to `artifacts/oracle/states/`, `new_3ds: true`, level `lvl_01_cherrytreehills`.
- `info-barrier` (user slot 3): the player stands just before the bound `No_Entry_Auburn_OnFoot` (centre 763.682 36.0 577.971) on the street from Cherry Tree Hills to Auburn, near the mission 1-2 dog house; walking forward a few steps opens the InformationBox "You need to complete more missions before you can enter this area of LEGO City." Use: HUD-26 (InformationBox look, buttons).
- `mission-1-2-start` (user slot 4): the player stands close in front of the mission 1-2 start trigger. Use: ShowIntroText heading and the mission 1-2 opening (HUD-26), mission flow (SCRIPT-1).
- Game progress (user report): mission 1-1 finished, mission 1-2 not started, Auburn locked.

Unknowns:
- Position, heading and the save slot's story bytes were not read from memory yet; the progress above is the user's report.

<a id="ts-20261014-204610"></a>

## 2026-10-14T20:46:10+02:00 — States info-box-open and slotlist-delete-confirm

- `info-box-open` (New 3DS, from `info-barrier` with the stick up for 30 updates): the InformationBox of `No_Entry_Auburn_OnFoot` is open at the player position (755.09, 25.58, 588.84). Game progress as `info-barrier`: mission 1-1 finished, mission 1-2 not started, Auburn locked. Findings: [script-text.md §2026-10-14T20:46](script-text.md#ts-20261014-204600).
- `slotlist-delete-confirm` (Old 3DS, from `savecont-slotlist-user` with Y pressed on slot 1): the delete confirmation of the used slot 1 ("Are you sure you want to delete this save file?", B Cancel, Confirm A). Game progress as `savecont-slotlist-user` (the user's slot 0: after mission 1-1, per mille 2, level 1). Findings: [front-end.md §2026-10-14T20:46](front-end.md#ts-20261014-204620).
- Both were taken in the agent's own home `artifacts/oracle/agentOC-home`; the user's saves were not touched.

- 2026-10-14T22:49:29+02:00 — Agent Q (cutscenes-in-game): state `cutscene-1-1-b` (Old 3DS, from `gameplay-start`): the player was teleported onto the donut shop waypoint (817.3, 24.0, 1071.4) and `1-1_VanBreakdown` started `1_1_b`; saved with the cutscene near its end (the fade to black and the return to the game follow within seconds). Game progress: new game, mission 1-1 started (cutscene `1_1_a` played), nothing else done. Own home `artifacts/oracle/agentQ-home`; the user saves were not touched. Findings: [cutscenes.md §2026-10-14T22:49](cutscenes.md#ts-20261014-224929).

<a id="ts-20261014-231052"></a>

## 2026-10-14T23:10:52+02:00 — State in-vehicle (user slot 5)

- The user quick-saved in their own Azahar with the player sitting in a vehicle in lvl_01, slot 5. Copied read-only from `~/.local/share/azahar-emu/states/00040000000AD600.05.cst` (2026-10-14 23:10, 25614134 bytes, identical by `cmp`) to `artifacts/oracle/states/in-vehicle.cst` with `in-vehicle.json` (New 3DS). The user's saves were not modified.
- Purpose: leave and jump-out traces of the vehicle states 0x11b / 0x11c and the driving update (VEH-4, VEH-5, VEH-9 in [vehicles.md](../plans/features/vehicles.md)).
- Game progress: mission 1-1 finished, mission 1-2 not started (the user confirmed it on 2026-10-14).
- Read from memory (2026-10-14T23:13:01+02:00, GDB, `--renderer opengl`, lead's own port 24730 and home `artifacts/oracle/lead-home`, after `frames 2`):
  - Player entity 0x085526f0, ctrl 0x08552790. Locomotion state ctrl+0x6c = 0x11c (driving), ctrl+0x130 = vehicle entity 0x08b7d640. Player at (613.370, 17.072, 1062.515), forward (-0.199, -0.002, 0.980).
  - Vehicle entity: type byte +0x12 = 0xb7 (183), owner flags +0x10 = 0x1300 (the traffic value of [vehicles-system.md](vehicles-system.md#ts-20261014-224438)), word +8 = 0x2b69676d, record +0x7c = 0x08b7d6e0 with state +2 = 5 (driven by the player).
  - `VehicleType` record+0x2ec = 37: row 37 of `tablescharacters` sheet 4 is `van_dulmen` (a van; the donut van is 36). The user took it from the traffic in the street, not a parked vehicle.
  - Vehicle node 0x08b7da40, world translation node+0x8c = (613.37, 17.97, 1062.50).

Unknowns:
- Whether word +8 is the hash of an instance name (traffic vehicles may have none).

<a id="ts-20261014-232710"></a>

## 2026-10-14T23:27:10+02:00 — Mission 1-1 step states m11-1 to m11-9 (user slots 6 to 10 and 1 to 4)

- The user played mission 1-1 in a new playthrough on their own Azahar and quick-saved before each step. Copied read-only (`cp --no-clobber`, identical by `cmp`) from `~/.local/share/azahar-emu/states/00040000000AD600.NN.cst` (saved 23:18 to 23:23) to `artifacts/oracle/states/NAME.cst` with `NAME.json` (New 3DS). The user's saves were not modified. Slots 1 to 4 were overwritten by the user since the earlier copies (`info-barrier` and `mission-1-2-start` keep their own copies).
- Game progress of all nine: mission 1-1 in progress, not finished (user report). The steps are those of [objective-hud.md §2026-10-14T21:23](objective-hud.md#ts-20261014-212357).
- Each state was loaded once by the lead (port 24730, home `artifacts/oracle/lead-home`, OpenGL, `frames 2`, `player`); all loaded. Player world position and forward vector:

| State | Slot | Point in mission 1-1 | Player position | Forward |
|---|---|---|---|---|
| `m11-1-donut-shop` | 6 | before the donut shop waypoint (`1-1_Waypoint_01` at 817.35, 23.93, 1071.20) | 819.581 23.919 1073.996 | -0.422 0 -0.907 |
| `m11-2-patrol-car` | 7 | in front of the police car waypoint (`1-1_Waypoint_02`) | 886.681 16.590 1112.930 | 0.284 0 0.959 |
| `m11-3-repair-van-marker` | 8 | in front of the broken van waypoint (`1-1_Waypoint_03`, room33) | 877.270 52.788 705.022 | -0.045 0 -0.999 |
| `m11-4-van-parts` | 9 | in front of the build-it parts (`1-1_BI_DonutVanParts`) | 876.914 52.435 700.427 | -0.768 0 -0.640 |
| `m11-5-in-donut-van` | 10 | inside the repaired donut van | 869.590 52.740 691.972 | 0.825 0.017 0.565 |
| `m11-6-van-to-shop` | 1 | in the van in front of `1-1_Waypoint_05` (bring the van to the shop) | 813.926 17.534 1046.835 | 0.997 -0.009 -0.075 |
| `m11-7-collect-donuts` | 2 | in front of `1-1_Waypoint_Donuts` | 819.964 23.920 1071.412 | -0.986 0 0.170 |
| `m11-8-donuts-in-hand` | 3 | carrying the donut box | 819.582 23.920 1074.939 | 0.773 0 0.635 |
| `m11-9-delivery-goal` | 4 | in front of the delivery target (`1-1_DonutTarget_DEMO`) | 881.396 16.597 1118.883 | 0.370 0 0.929 |

Unknowns:
- The level variables (StoryProgress, `1-1_IntroPlayed`) and which waypoints and triggers are enabled in each state; not read yet.
- The player's controller state in the van states (the `player` command prints the intent state only).

<a id="ts-20261015-093500"></a>

## 2026-10-15T09:35:00+02:00 — State drv-police-car-seated (made for the driving measurements, VEH-5)

- Made by the vehicle-drive agent from the user's `m11-2-patrol-car` (game progress: mission 1-1 step 2, the police car waits behind `1-1_Waypoint_02`; nothing of the user's save was changed): with `--rpc` writes as measuring aids the locked bit (+0x311 bit 1) of the record of `1-1_Police_Car` (entity 0x08a88440, record 0x08a884e0) was cleared and the player was teleported 1.9 m beside the car (car position plus its right axis); X was pressed, then `savestate drv-police-car-seated` (New 3DS, OpenGL renderer).
- Contents: the player sits in the police car (ctrl+0x6c = 0x11c, vehicle state (5, 5)), the forced stop byte +0x310 is still 1 (set by VanBreakdown, cleared by a script call of SetForcedStopOnMyVehicle in the first update after A is pressed), the car stands at (887.377, 17.809, 1117.318). Use: speed, brake, steering and hand brake measurements in [vehicles-driving.md §2026-10-15T09:30:35](vehicles-driving.md#ts-20261015-093035).


<a id="ts-20261015-190736"></a>

## 2026-10-15T19:07:36+02:00 — State bullet-trooper-at-obstacle (made for the vehicle box contacts, VEH-60)

- Made by agent Y (vehicle box collisions) from `drv-police-car-seated` (mission 1-1 step 2, see above): fixed dt 0.033, A held for 106 stepped updates, then `savestate`. The save ran only after the tool detached, while the game ran freely with A still held, so the state is later than the stepped frame: the trooper rests against the static Bullet box at (839.22, 22.16, 1071.55), rigid body 0x08e16f40 at (847.79, 17.25, 1077.14), awake, its manifold 0x08472cb0 has 4 points. The NAME.json frame (106) is the stepped frame, not the saved one.
- Needs the OpenGL renderer (`--renderer opengl`): with the software renderer, launches with `--state drv-police-car-seated` crashed 6 of 6 times during the state load (SIGSEGV, not the known paint race signature).

Unknowns:
- Why the save waited for the detach (the save key press is queued until the emulation thread runs).

<a id="ts-20261016-093536"></a>

## 2026-10-16T09:35:36+02:00 — State m11-7-box-prompt (made for the carry reference pictures, agent A of round 15)

- Made from the user's `m11-7-collect-donuts` (mission 1-1, donut shop, nothing of the save changed; New 3DS, OpenGL): `freestick 0 120` for 1.3 s steps into the waypoint beam (the badge icon goes, the doughnut box and its A prompt appear), 2 s wait, `savestate m11-7-box-prompt`. The player stands at 816.3 23.92 1072.5, the tip says to press A at the box. A held for 0.12 s lifts the box (A before the beam does nothing).


<a id="ts-20261016-143921"></a>

## 2026-10-16T14:39:21+02:00 — State leadcut-1_1_d-early (made for the cutscene props of 1_1_d, MOVIE-32)

- Made by the lead from the user's `m11-6-van-to-shop` (mission 1-1 step 6: the donut van driven to the shop, nothing of the save changed; New 3DS, OpenGL): `hold A` for 1.2 s drives into `1-1_Waypoint_05`, 1.3 s after the release `savestate leadcut-1_1_d-early`. In the state the cutscene `1_1_d` is loaded in slot 0 (object 0x08ffdd10) and starts about 1.2 s later; it plays to its end (frame 197) in a free run.
- Use: RPC reads of the cutscene actors (object +0x70 actor array of 0x22c-byte slots, +0x78 count; slot +0x200 node, or entity for flag 1 with the node at entity +0x3c; world matrix at node +0x5c) and `shotloop` pictures with the frame float at object +8. Heap addresses are valid for this state only.

<a id="ts-20261018-101306"></a>

## 2026-10-18T10:13:06+02:00 — Mission 1-2 step states m12-1 to m12-8 (user slots 1 to 8)

- The user played mission 1-2 on their own Azahar and quick-saved at each step (saved 09:57 to 10:05). Copied read-only (`cp --no-clobber`, identical by `cmp`, files made read-only) from `~/.local/share/azahar-emu/states/00040000000AD600.0N.cst` to `artifacts/oracle/states/`; `new_3ds: true` (the user's Azahar runs in New 3DS mode).
- Game progress of all eight: mission 1-1 finished, mission 1-2 in progress, not finished (user report). The step descriptions are the user's.
- Each state was loaded once by the lead (port 24730, home `artifacts/oracle/lead-home`, OpenGL, `frames 2`, `player`); all loaded. Player world position and forward vector:

| State | Slot | Point in mission 1-2 (user) | Player position | Forward |
|---|---|---|---|---|
| `m12-1-mission-trigger` | 1 | in front of the mission's start trigger | 881.280 16.589 1108.467 | 0.079 0 -0.997 |
| `m12-2-crying-trigger` | 2 | in front of the first trigger of the mission (the character plays the crying animation) | 743.169 31.158 643.128 | -0.383 0 -0.924 |
| `m12-3-alley-trigger` | 3 | in front of the trigger in the alleyway | 738.819 29.389 631.241 | -1.000 0 0 |
| `m12-4-detective-point` | 4 | standing on the point that activates the detective tool | 723.675 29.768 630.775 | -0.929 0 0.370 |
| `m12-5-detective-active` | 5 | detective tool activated | 720.772 29.779 630.714 | -0.926 0 0.378 |
| `m12-6-doghouse-x` | 6 | standing on the X in front of the dog house | 710.082 32.677 638.595 | -0.404 0 0.915 |
| `m12-7-detective-waypoint-2` | 7 | standing on the second detective waypoint | 720.021 37.035 654.390 | -0.942 0 0.335 |
| `m12-8-roof-sound-scanner` | 8 | on a roof, on the sound scanner trigger | 869.659 65.997 645.594 | -0.122 0 -0.993 |

- User report: in the user's Azahar the sound scanner view shows only a white screen instead of its proper picture; pictures of that view from the oracle are not a reference for how it looks (memory reads and traces are not affected by this).

Unknowns:
- The level variables (StoryProgress, mission 1-2 flags) and which waypoints and triggers are enabled in each state; not read yet.
- Whether the white sound scanner view is an emulation fault of both renderers or of the user's renderer only.

<a id="ts-20261018-122739"></a>

## 2026-10-18T12:27:39+02:00 — User states of the rest of mission 1-2

The user played mission 1-2 past the roof sound scanner and to its end and saved quick-save slots 1 to 6 (2026-10-18, 12:24 to 12:27, New 3DS). Copied read-only; each was loaded once (`--state NAME --rpc`, `wait 3`, `player`, screenshot) and shows what the user described:

| State | Slot | Where (user report) | Player position |
|---|---|---|---|
| `m12-9-zipline` | 1 | standing under the zipline (jump to use it) | 870.226 66.001 630.961 |
| `m12-10-pole-climb` | 2 | on the pole, climbing up the house | 940.542 60.181 683.410 |
| `m12-11-brick-climb` | 3 | climbing up a set of bricks | 908.942 65.698 673.170 |
| `m12-12-railing` | 4 | climbing along a railing | 908.066 70.147 669.126 |
| `m12-13-cage-trigger` | 5 | standing on the trigger in front of the dog cage | 916.702 64.431 663.724 |
| `m12-14-mission-complete` | 6 | mission 1-2 complete | 745.951 30.055 636.281 |

Progress: mission 1-1 finished; slots 1 to 5 mission 1-2 in progress (past the scanner), slot 6 mission 1-2 finished (back on the street with the dialogue "McCain, stop messing around and get back to the station."). Each `NAME.json` names the emulator load slot `00040000000AD600.00.cst` in `file`, like the other states. The user got the scanner working in Azahar; how is not recorded yet.

<a id="ts-20261018-124556"></a>

## 2026-10-18T12:45:56+02:00 — User states for vehicle smashes and the car break

The user saved quick-save slots 1 to 3 (2026-10-18, 12:44 to 12:45, New 3DS) for the brick effects of PART-2, VEH-104, VEH-112 and VEH-121. Copied read-only; each was loaded once (`--state NAME --rpc`, `wait 3`, `player`, screenshot):

| State | Slot | Where (user report) | Player position | Seen at load |
|---|---|---|---|---|
| `veh-smash-prop` | 1 | in a car in front of a smashable lamp post | 857.112 31.175 923.384 | police van, four hearts, lamp post ahead on the pavement |
| `veh-smash-car` | 2 | in a car in front of another moving car | 877.466 31.160 935.964 | police van, four hearts, a green car and a blue truck ahead |
| `veh-break-low-hearts` | 3 | in a car with one heart left, aimed at a wall a few metres away | 602.002 16.810 1095.447 | police van, one heart; straight ahead the picture shows open road, the wall is not in view |

Progress: missions 1-1 and 1-2 finished (user report). Not checked yet: which object the van hits first when A is held in `veh-break-low-hearts`.

<a id="ts-20261018-150616"></a>

## 2026-10-18T15:06:16+02:00 — User state `veh-wall-full-speed`

The user quick-saved slot 0 (2026-10-18 14:57, New 3DS) "with a police car going full speed and heading directly for a wall". Copied read-only (`cp --no-clobber`, identical by `cmp`) to `artifacts/oracle/states/veh-wall-full-speed.cst`. Loaded once by the servant r20-D (`--state veh-wall-full-speed --rpc`, read before any input): the player is in a vehicle, R+0x30c..0x30f = `6404040f` (four hearts), the picture shows the car, four hearts and a brick wall ahead. Position over three stepped updates without input: x 773.119, 774.000, 774.824 (z 784.234, 784.053, 783.883), about 0.85 to 0.88 units per update at the state's own dt.

Progress: not reported by the user; presumably the playthrough of the other vehicle states (missions 1-1 and 1-2 finished), not confirmed.

Unknowns:
- The game progress of the slot.

<a id="ts-20261018-154624"></a>

## 2026-10-18T15:46:24+02:00 — User states at the foot of the drain pipe of `m12-10`

Asked by r21-A for the pole mount (TRAV-1). The user quick-saved two slots (2026-10-18, New 3DS); copied read-only (`cp --no-clobber`, identical by `cmp`):

| State | Slot | Where (user report) |
|---|---|---|
| `m12-10a-pole-foot` | 0 (15:44) | on the ground directly in front of the drain pipe climbed in `m12-10` (`1-5_Drain_Pipe05`), facing away from it; turning 180 degrees and walking forward mounts the pipe |
| `m12-10b-pole-side` | 2 (15:45) | facing the wall about 0.5 m to the left of the same pipe |

Progress: mission 1-2 in progress, past the scanner and the zipline, as `m12-10` (user). User observation: in the original the pipe is mounted by walking into it. Not loaded yet by the lead.

