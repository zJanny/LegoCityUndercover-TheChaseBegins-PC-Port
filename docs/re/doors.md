# Doors

Object type 42: classes `props/prop_door` and `leprops/leprop_door`. Collision of their box: [collision.md](collision.md). Level scripts that can send them requests: [scripts.md](scripts.md).

<a id="ts-20261013-211700"></a>

## 2026-10-13T21:17:00+02:00 — State machine, timing, motion and sounds (REA Ghidra 12.1.2, llvm-objdump, Azahar oracle port 24692 without RPC)

Corrected by [doors.md §2026-10-13T21:51](#ts-20261013-215117): stop events are played

Replica: `src/runtime/door.{hpp,cpp}` (`doorUpdate`, `doorMessage`, `doorPose`, `doorParams`), `Model::nodePivot`, wiring in `chase-game` (`DoorObject`). Tests: `door-core` (tests/door_test.cpp), `door-real-data` (tests/door_real_test.cpp, replays the two oracle traces below).

Evidence IDs (shortened): door constructor 0x169494 ev_18f0ca17…803b; post-link 0x163a2c ev_5e406d44…4f; update 0x1d16b8 ev_615c22b0…e6e4; state change 0x1a6b6c ev_278a1b9e…ffc; message handler 0x1749b8 ev_cd02117d…d07; motion 0x26905c ev_f634c12d…a0c; zone test 0x2397a0 ev_3240d814…001, inverse transform 0x2536c0 ev_ab3f0c52…598; easing table init 0x17ea64 (llvm-objdump), easing 0x221b48 ev_fbc0baf3…87; pivot accessor 0x20a2f4 ev_4269aad6…395; rotate about pivot 0x201f34 ev_a6af80df…a2; model bounds accessor 0x233ccc ev_bcb3fbf7…91.

### Class registration and the record

- The class descriptor of type 42 is built at 0x15c238 (`mov r0,#42; bl 0x246240`): create 0x169494, post-link 0x163a2c, destroy 0x174998, update 0x169ba0, message handler 0x1749b8, link 0x1697fc, release 0x169b70.
- Each door has a 0x40-byte record at entity+0x7c: +2 state, +4 target state, +0xc timer (frames), +0xd open/close frames, +0xe auto-close frames, +0xf flags (bit 0 two leaves, bit 1 AutoOpen, bit 2 partial opening, bits 3.. DoorType), +0x10 sound delay, +0x11 / +0x12 bounce counters, +0x13 Bounce, +0x14 rest angle (float) or slide vector (3 floats), +0x20 platform link record, +0x2c / +0x2e / +0x30 / +0x32 sound ids (SFXOpening, SFXClosing, SFXShut, SFXOpen), +0x34 DoorAnim object, +0x38 partial value, +0x3c LerpShape curve.
- Parameters (class lookup 0x217080, instance slot when the class flags have bit 1): `openclosetime` (class default 0.75), `autoclosetime` (0), `AutoOpen` (class 0), `DoorType`, `OpenAngle` (default 90 degrees, hash e4507437), `LerpShape`, `LerpShapeAccuracy`, `Bounce`, `DoorSFXOpenDelay`, `ATTR_SFXOpening` / `ATTR_SFXClosing` / `ATTR_SFXShut` / `ATTR_SFXOpen`, `DoorAnim` (class default "Prop_Door01"; no animation of that name exists in the archive, so the record's +0x34 stays 0 and the motion is procedural).
- Seconds become frames in 0x163a2c as `(byte)(int)(seconds * 30)`: 0.75 s is 22 (record +0xd = 0x16 read), 10 s is 300 and wraps to 44 (`1-8_Building_Door` +0xe = 0x2c read). The rest angle is `OpenAngle * pi / 180`, negative for types 0, 2, 4, 0x12 and positive for 1, 3, 0x13 (Police_Door_02: 0xbfc90fdb read). The slide vector of the sliding types is the half extent of the first node's mesh times 2 (x for types 5 and 6, y for 7 and 8, z for 0x11), negative for 5, 8, 9, 0xc, 0xd, 0x10.

### Update (every game update, 0x169ba0 → 0x2424d0 → 0x1d16b8, then 0x1a6b6c)

- `0x2424d0` calls the update 0x1d16b8 only while the state equals the target. The zone: the player node position (the node of `DAT_003772c0`) is moved into the door's frame (0x2536c0: subtract the door's translation, dot with the three axis rows), then tested against the entity box (centre entity+0x60) with half extents (x, y + 0.25, z + 4.0) (literals 0x1d1930 / 0x1d1934). A Police_Door box (half z 0.05) therefore reacts within 4.05 in front of and behind it and 0.76 to each side.
- State 1 (closed): AutoOpen and player in the zone → target 2. State 3 (open): when `autoclose` is not 0: player in the zone resets the timer to 0, then timer + 1; at `autoclose` frames the target becomes 4. States 2 and 4: timer + 1 per update (with `Bounce`: at the end of the movement the bounce counter halves and the timer runs backwards for `bounce * openFrames >> 9` frames); when the timer exceeds `openFrames` the target becomes 3 (opening) or 1 (closing), the timer is set to `openFrames`; a closing door with AutoOpen and the player in the zone turns around (target 2). Then the motion 0x26905c runs.
- State change 0x1a6b6c (when target ≠ state): target 1: stop the closing sound, play the shut sound, clear entity flag 0x800; target 2: timer = `openFrames - timer` coming from state 4, else 0, bounce counter = Bounce, set flag 0x800; target 3: timer 0, stop the opening sound, play the open sound; target 4: play the closing sound, timer = `openFrames - timer` coming from state 2, else 0. After the change, a door in state 2 whose timer equals the sound delay plays the opening sound (so once, at the start when the delay is 0). Sounds go through 0x23ae84 (play at the object) and 0x23ae50 (stop with -1.0).
- Messages (handler 0x1749b8): 0xff open (state 0, 1 or 4 → target 2; state 3 → timer 0), 0xfe close (state 2 or 3 → target 4), 0x29 set an opening fraction (partial mode), 0x36 open at once, 8 reset (state and target 1, pose identity). Scripts reach it through `TriggerObject` and the platform link 0x23211c, which sends 0xfe / 0xff to a linked object. The doors of lvl_00 have AutoOpen 1 (lift doors DoorType 5, double doors DoorType 0); the doors of lvl_01 (`Police_Door_01/02`, `FP_PoliceStation_UpperDoor01/02`, `1-8_Building_Door`, the lift doors) have AutoOpen 0 and wait for a request; which script sends it (mission scripts, `TriggerObject`) is not traced.

### Motion (0x26905c)

- `f = ease(timer / openFrames)` with the easing of `LerpShape` (0x221b48): 0 identity (read), 1 `(cos((t - 1) * -pi) + 1) / 2`, 2 `1 - cos(t * pi / 2)`, 3 `sin(t * pi / 2)`, 5 `t * t * (3 - 2t)` (read from the code, not measured); 4 and the custom curves (ids of 0x100 and up) are not decoded.
- Hinged types 0 and 1: the first node gets the rotation about its pivot (the float3 at node record +4, 0.815166 for the Police door) by `a = lerp(f, 0, rest)` while opening and `lerp(f, rest, 0)` otherwise; the second node of a two-leaf door gets `-a` about its own pivot. The matrix is `[[cos a, 0, -sin a], [0, 1, 0], [sin a, 0, cos a]]` with the translation `pivot - pivot * R`, in the pose array at node+0x110. Sliding types 5 to 8 and 0x11: the node's translation is the slide vector times `f` (second leaf negated). The other types (2, 3, 4, 9 to 0x13) use other rotations or add an offset; not decoded, their pose stays the identity here.
- Oracle traces (state gameplay-start, `poke` of record+4 of Police_Door_02 to 2 and later 4; memtrace of the record, the entity, the pose array and the grid entry): the door opens in 22 updates with the timer going 1 to 22 and a linear angle of 90 degrees / 22 per update, state 3 and timer 0 one update later; closing mirrors it, ends in state 1 with the timer left at 22 and the pose the identity. `door-real-data` pins the poses at timers 1, 11 and 22 for both directions to 2e-6.
- Collision while open: the grid entry of the door (mask 6, kind 0) and the entity box did not change in the trace (only flag 0x800 on the entity), so the replica keeps the closed-state box. A walk into the doorway with the door opened held the character still at (905.755, 1116.386) for 30 updates while the same walk with the door closed slid along it (stick -131 72 after a teleport to (904.4, 16.58, 1114.4), 90 settling updates and, for the open case, an open request 30 updates earlier). Why the open door blocks differently is not explained.

### Rendering

- The door's mesh vertices are drawn with the node matrix at model+0x20 (translation +0.0698476 for Police_Door_02) before the pose: `placement * pose * nodeMatrix` in chase-game. The order of the rest matrix and the pose is not verified against a picture of the original.

### Stand-ins and open items

- Only the door code was transcribed; `chase-game` has no scripts, so doors with AutoOpen 0 are never asked to open. The proximity test uses the player position of chase-game.
- Stop events (0x23ae50) are not played (the audio mixer has no stop for effect sounds); sound id 0 is skipped (assumed to mean none).
- `LerpShape` curves 4 and the custom curves; door types 2, 3, 4, 9 to 0x13 (motion); the DoorAnim path (state 2 / 4 wait for the animation object); partial opening and the bounce are transcribed but not measured.
- The collision of a door that is open (see above).

Unknowns
- Which scripts send open and close requests to the doors of lvl_01, and whether the player's "use" action does.
- The meaning of the player flag set by 0x1d16b8 in states 2 and 4 (`player ctrl +0x316 |= 0x10`).
- The difference between the open-door and closed-door walks.

<a id="ts-20261013-215117"></a>

## 2026-10-13T21:51:17+02:00 — Door sound sequence pinned (llvm-objdump, no REA; no emulator)

Addresses read again: state change 0x1a6b6c, play 0x23ae84, stop 0x23ae50 (stop details in [audio.md §2026-10-13T21:51](audio.md#ts-20261013-215117)). Tests: `door-core` (all four sounds and the delay), `door-real-data` (Police_Door_02 ids). [Corrects doors.md §2026-10-13T21:17](#ts-20261013-211700) entry ("Stop events are not played"): they are now stopped immediately. Wiring: `chase-game` plays the events at the door position with a per-door owner and stops them through `GameAudio::stop`.

- Sound call sites of the door code: only 0x1a6b6c. A scan of every `bl 0x23ae50` and `bl 0x23ae84` finds none in the update 0x1d16b8 (to 0x1d1928), the update shell 0x169ba0, the destroy 0x174998 or the message handler 0x1749b8..0x174c50 (the neighbours 0x174ef4, 0x1a6dd4 and 0x1a6e74 work on a different record at +0x48). All door sounds are positional at the door (`0x23ae84`, owner = u16 at entity+0x14):
  - target 1 (closed): stop SFXClosing (+0x2e, -1.0), then play SFXShut (+0x30); entity flag 0x800 cleared.
  - target 2 (opening): no call in the state change. Afterwards, state 2 with timer (+0xc) equal to the delay (+0x10) plays SFXOpening (+0x2c). The test runs in every update (0x1a6bfc..0x1a6c28), so delay 0 plays at the start and delay d plays d updates after it.
  - target 3 (open): link message to the platform (0x23211c), stop SFXOpening (+0x2c, -1.0), then play SFXOpen (+0x32).
  - target 4 (closing): play SFXClosing (+0x2e); link message when the state before was not 2. A close request while opening does not stop the opening sound.
- Police_Door_02 (lvl_01): SFXOpening 605 (doors/door_open_01), SFXClosing 601 (doors/door_close_01), SFXShut and SFXOpen 0, delay 0. The stop of 605 follows at the end of the opening and the stop of 601 at the end of the closing; the shut and open sounds are id 0 (empty table row, nothing plays here).

Unknowns
- Whether the original treats sound id 0 as "none" in the play and the stop (stand-in DOOR-S2).

<a id="ts-20261014-095011"></a>

## 2026-10-14T09:50:11+02:00 — Two-leaf doors: the model's two nodes are posed separately (REA Ghidra 12.1.2 ev_f634c12d84d35f04d4136c90381719005c7fb3c7b493113075d6f201c7c92a0c, chase-game)

- `Lobby_Lift_Door01` (lvl_00) uses `models/prop_policestation_doubledoors/prop_policestation_doubledoors_atlas_lvl_00_policestation_main.bfnmdl`: DoorType 0 (hinged), AutoOpen 1, openclosetime 22 frames, two nodes `Door_Right` (mesh 0, pivot -1.27201, 0, 0) and `Door_Left` (mesh 1, pivot 1.27201, 0, 0), both meshes half extents (0.6408, 1.3193, 0.166). The lift doors (`Lobby_Lift_Door`, `DOOR_lift_to_*`) use `prop_lift_doors.bfnmdl`: DoorType 5, nodes `Door_L` (mesh 0) and `Door_R` (mesh 1), slide -1.30351 in x.
- 0x26905c poses exactly the model's nodes 0 and 1 through 0x22cbbc (case 0/1: node 0 turns by `a`, node 1 by `-a` about its own pivot; cases 5 to 8 and 0x11: node 0 by the slide vector, node 1 by its negation). The replica's `doorPose` already did that; **the cause of DOOR-7 was the renderer**: `chase-game` drew the whole door model as one scene instance with only the pose of node 0, so both panels moved together (hinged) or the second leaf never had its own matrix. Fix: a two-node door gets one scene instance per node, restricted to the node's mesh (`Scene::setInstanceMesh`, `addModel(..., onlyMesh)`), each with `placement * pose[node] * rest[node]` (rest matrix of that node, `model+0x20`).
- Not verified against a picture of the original (the oracle door was not opened in this session): the lobby doors in `chase-game` were checked only by build, tests and a screenshot of the closed doors.

Unknowns:
- Whether the rest matrix of node 1 is applied in the same order as for node 0 (DOOR-4).

<a id="ts-20261014-133955"></a>

## 2026-10-14T13:39:55+02:00 — Details moved from feature items DOOR-1

Moved from the feature files so that the items stay short (CLAUDE.md, "Documentation rules"). The text below is the former wording of each item.

- **DOOR-1** (doors.md): Some doors in lvl_01 (Cherry Tree Hills, for example `Police_Door_01/02`) open only when a game script asks. `TriggerObject` requests reach the doors (0xff opens, 0xfe closes) and bound triggers start scripts, but the police front-door script `Police_ExtToInt` stops at `WarningDialogBox`, which has no handler, and its bound `Police_Station_Transition_Lower` is switched off at the start of the story (a mission script switches it on later). Whether the player's "use" action also opens doors is unknown. needs: SCRIPT-7 ([script-system.md](../plans/features/script-system.md)). evidence: [doors.md §2026-10-13T21:17](#ts-20261013-211700), [objects.md §2026-10-13T21:17](objects.md#ts-20261013-211741), [scripts.md §2026-10-14T10:06](scripts.md#ts-20261014-100640).
