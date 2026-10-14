# Runtime oracle: Save states and checkpoints

Subtopic of the Azahar runtime oracle. Start with [oracle.md](oracle.md) for setup and the fast workflow. Entries are dated; new entries go at the end.

Save states the oracle uses: which ones exist, the game progress of each, how each was made, and the New 3DS flag.

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
- Findings: [streaming.md](streaming.md#ts-20261014-100715).

<a id="ts-20261014-102023"></a>

## 2026-10-14T10:20:23+02:00 — Addition to the police-station checkpoint: game progress

Addition to [the entry of 2026-10-13T23:20](#ts-20261013-232034) (user statement, 2026-10-14): the user's quick save `police-station` was made **after the first mission (1-1, bringing the donuts to the police station)**. It is an unmodified state of the original past `1-1_MissionEnd`, so it is the reference state for everything that the mission end enables (for example the `Police_Station_Transition_*` bounds, see [streaming.md §2026-10-14T10:07](streaming.md#ts-20261014-100715)). No state that played mission 1-1 is needed; walking out of the station from this state reaches the street with the post-mission flags.

Unknowns:
- Whether mission 1-2 had already started when the user saved (the chapter / mission variables were not read).

<a id="ts-20261014-111312"></a>

## 2026-10-14T11:13:12+02:00 — Street states after mission 1-1: `street-after-1-1` (user) and `street-at-entrance`

- `street-after-1-1`: the user's own quick save (slot 2, 10:25), `new_3ds: true`. The player stands just outside the police-station entrance at world (900.55, 16.57, 1110.28), facing -x, camera yaw -1.57. Game progress: after mission 1-1 (donut delivery), the same as `police-station`. Evidence from memory (no poke): the enable bytes of `Police_Station_Transition_Lower`, `_Upper` and `Police_ExtToInt` are 1 ([streaming.md §2026-10-14T11:13](streaming.md#ts-20261014-111312)); the mission variables were not read, so whether 1-2 is active is open. User note: a mission trigger lies a few metres in front of the player; it was not found among the bounds (the bounds near the start are disabled except `Police_Station_Transition_Lower`), and it was not triggered.
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

- `station-at-exit` (New 3DS, from `police-station` with the pad: 2 idle updates, stick (0,-150) for 7 updates, 6 idle; the player stands at (-1.970, 1.424, 18.224) in the lobby, 1.9 m in front of the exit bound `Lobby_to_CherryTree`, facing +z) and `station-exit-loadingpage` (same route with the stick held about 40 updates: the loading page waits for A). Both unmodified, with a `source` key; game progress as `police-station` (after mission 1-1). Findings: [streaming.md §2026-10-14T12:02](streaming.md#ts-20261014-120258).
- With `--rpc` and OpenGL, `screenshot` works in stepped runs and returns the page that the game shows at that moment (400x480). `trap` still reports the unrequested UI stops on the A press of the loading page (0x2c34b4, 0x2a85a0, ...): they are the page's own widget code (see the streaming entry), not a tool artefact.

<a id="ts-20261014-152224"></a>

## 2026-10-14T15:22:24+02:00 — State `savecont-slotlist-user` (user save, slot list)

- `savecont-slotlist-user` (agent C, `new_3ds: false`): cold boot with a read-only copy of the user's `savegame.dat` (`artifacts/oracle/agentJ-saves/`) in the emulator's own sdmc (`artifacts/oracle/agentC-home`), A on the HOME notice and the autosave notice, stopped at the slot list before a slot is chosen. Game progress: slot 0 used, after mission 1-1 (slot byte +6 = 1, police station), slot 1 empty. Continue on slot 0 starts lvl_00_policestation at `Player1Start` ([save-format.md §2026-10-14T15:19](save-format.md#ts-20261014-151903)). The user's own Azahar saves were not touched.

Unknowns: a save with byte +6 above 1 for Continue at later levels (SAVE-13).
