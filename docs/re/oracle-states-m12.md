# Runtime oracle: Save states of mission 1-2

Subtopic of [oracle-states.md](oracle-states.md), which has the procedure, the New 3DS flag and the other states. Start with [oracle.md](oracle.md) for setup and the fast workflow. Entries are dated; new entries go at the end.

User states made in mission 1-2, the pole and trampoline states made in it, and the vehicle brick and wall states from the same days (saved with missions 1-1 and 1-2 finished, not in mission 1-2): game progress of each, positions, how each was checked.

## Current state

- Mission 1-2 in progress (user): `m12-1-mission-trigger` to `m12-8-roof-sound-scanner`, positions in the table of the entry; the sound scanner view is white in Azahar (user report), so its pictures are no reference. [§2026-10-18T10:13:06](#ts-20261018-101306)
- Rest of mission 1-2 (user, loaded and checked): `m12-9-zipline` to `m12-13-cage-trigger`, and `m12-14-mission-complete` with mission 1-2 finished; positions in the table of the entry. [§2026-10-18T12:27:39](#ts-20261018-122739)
- Vehicle brick effects (user): `veh-smash-prop` (lamp post), `veh-smash-car` (moving car ahead), `veh-break-low-hearts` (one heart left). [§2026-10-18T12:45:56](#ts-20261018-124556)
- `veh-wall-full-speed` (user): police car with four hearts at speed towards a brick wall, for VEH-128. [§2026-10-18T15:06:16](#ts-20261018-150616)
- Pole mount (user, for TRAV-1): `m12-10a-pole-foot` (0.8 m from the drain pipe, facing away; walking into the pipe mounts it), `m12-10b-pole-side` (2.4 m left of the pipe axis, facing the wall). [§2026-10-18T15:46:24](#ts-20261018-154624)
- Table and trampoline of mission 1-2 (user, for PLAYER-12 and LEVOBJ-86): `m12-table-before`, `m12-table-broken`, `m12-trampoline-built`, loaded and checked. [§2026-10-18T17:25:22](#ts-20261018-172522)
- Bars above the trampoline (user, for TRAV-36): `m12-trampoline-pole`, Chase high above the pad next to the bars; keep pressing jump to grab. [§2026-10-18T20:44:42](#ts-20261018-204442)

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

Asked by r21-A for the pole mount (TRAV-1). The user quick-saved two slots (2026-10-18, New 3DS); copied read-only (`cp --no-clobber`, identical by `cmp`). Both were loaded and checked by the servant r20-D (`--state NAME --rpc --renderer opengl`, read before any input, screenshot; a first load after a fresh start and a repeated `loadstate` gave the same values). Pole origin of `1-5_Drain_Pipe05`: 940.457 45.371 683.463.

| State | Slot | Where (user report) | Player position | Seen at load |
|---|---|---|---|---|
| `m12-10a-pole-foot` | 0 (15:44) | on the ground directly in front of the drain pipe climbed in `m12-10`, facing away from it | 941.112 58.336 683.057 | 0.77 m from the pole axis, facing (0.899, 0, -0.438), 174 degrees away from the pipe; ctrl+0x6c = 1; standing beside the blue pipe, not carrying, not on it: matches |
| `m12-10b-pole-side` | 2 (15:45) | facing the wall about 0.5 m to the left of the pipe | 939.263 58.301 681.421 | facing (-0.455, 0, 0.891), the pipe 57 degrees to the right of the facing; ctrl+0x6c = 1; seen from behind facing the wall with the pipe to his right: matches, but 2.36 m from the pole axis, not about 0.5 m |

The pole origin lies 13 below the player's feet (y 45.37 against 58.3); the player stands on the pavement in both states.

Walk check from `m12-10a-pole-foot` (one try, stepped updates with dt 0.033, stick each update towards the pole origin, no button): update 1 ctrl+0x6c 1 -> 2; update 4 -> 285 (0x11d, POLE MOUNT) and the player moves to the pipe (0.80 -> 0.10 m over updates 4 to 9); update 9 -> 288 (0x120, POLE IDLE) at 940.542 58.32 683.410, 0.1 from the pole axis (the drain pipe's snap offset); updates 9 to 60 unchanged with the stick still towards the pipe; the picture shows Chase holding the pipe at its foot. The pipe is mounted by walking into it without a button press, as the user said. State names from [player-traversal.md](player-traversal.md#ts-20261018-145230).

Progress: mission 1-2 in progress, past the scanner and the zipline, as `m12-10` (user).

Unknowns:
- Which message of the pole handler the walk-in mount uses (the pressed path of 0x191ac0 was read, the walk-in not).

<a id="ts-20261018-172522"></a>

## 2026-10-18T17:25:22+02:00 — User states at the table and the trampoline of mission 1-2

Asked for the attack on foot (PLAYER-12) and the table and trampoline (LEVOBJ-86). The user quick-saved three slots (2026-10-18 17:23, New 3DS); copied read-only (`cp --no-clobber`, identical by `cmp`). Each was loaded once by the lead (`--state NAME --display xvfb --renderer opengl --rpc`, scenario `wait 2`, `player`, `screenshot`):

| State | Slot | Where (user report) | Player position | Facing (x, z) | Seen at load |
|---|---|---|---|---|---|
| `m12-table-before` | 1 (17:23:17) | in front of the table | 715.442 32.714 641.616 | (-0.090, 0.996) | the green table with its umbrella straight ahead, 2.06 m from `DonutTable_14` (715.63 32.53 643.67), on foot, four hearts: matches |
| `m12-table-broken` | 2 (17:23:28) | in front of the broken table | 715.726 32.668 640.255 | (-0.011, 1.000) | the table is gone; a round green build-it base with grey bricks where it stood; trampoline not built: matches |
| `m12-trampoline-built` | 3 (17:23:35) | the built trampoline | 719.135 32.618 639.895 | (-0.217, -0.976) | a black trampoline with a green rim beside Chase, about 2.6 m from `Trampoline_03` (720.20 32.58 637.47): matches |

The user's earlier note gave the hit position 718.06 32.68 642.69 facing -80.2 degrees; `m12-table-before` stands at another spot of the same table and faces it from the south.

Progress: mission 1-2 in progress, past the first detective point, before the zipline (user report, implied by the step).

Unknowns:
- Whether the table in `m12-table-before` has taken any hit already (its health was not read).


<a id="ts-20261018-204442"></a>

## 2026-10-18T20:44:42+02:00 — User state above the trampoline of mission 1-2

Asked for TRAV-36 (Chase grabs the poles above the trampoline in the original). The user quick-saved in slot 0 (2026-10-18 20:43:15, New 3DS); copied read-only (`cp --no-clobber`, identical by `cmp`, file made read-only) as `m12-trampoline-pole`. The user's note: Chase is close to grabbing the pole, and jump probably has to be pressed again for him to grab it.

Loaded once by the lead (`--state m12-trampoline-pole --display xvfb --renderer opengl --rpc`, scenario `wait 1`, `player`, `screenshot`): after one second of free run Chase is at 721.362 41.459 637.579, facing (0, 0, 1), about 8.9 m above `Trampoline_03` (720.20 32.58 637.47), beside two blue and white horizontal bars on posts at the house wall; the RPC player read gives state 1. Matches the user's description.

Progress: mission 1-2 in progress, the trampoline built (user report, implied by the step).

Unknowns:
- Whether Chase already holds a bar after the one-second wait (state 1 was read; the picture shows him at the bar), and which class the bars are.
