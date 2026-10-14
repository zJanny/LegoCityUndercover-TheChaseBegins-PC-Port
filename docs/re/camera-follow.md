# Gameplay camera: follow law

The camera chain, the follow camera law, the yaw word 0x376894 and the projection.

<a id="ts-20261013-143958"></a>

## 2026-10-13T14:39:58+02:00 — Camera chain, follow camera law, yaw word 0x376894, projection

Corrected by [camera-collision-sweep.md §2026-10-13T16:19](camera-collision-sweep.md#ts-20261013-161935): sweep push factor and the timers S+0x18..0x24

Evidence IDs (shortened): manager update 0x23f174 ev_4ee0aeb7…; follow camera update 0x2e0bf0 ev_30040f4a…; strategy 0x330e4c / 0x3310d0 / 0x330f8c / 0x3313c0 ev_41be0eba…, ev_819eeed6…; tracker 0x1ed9ac, 0x2e0800, 0x2e06f4 ev_49381ce6…; single-room path 0x1b4864 ev_8f15a259…; controller 0x2a8bfc and reset 0x2313c8 ev_dc307321…; matrix 0x2ca814 ev_30885dc9…, 0x22859c ev_8a74ba9a…, 0x24c118 ev_108e70de…; mode pose step 0x181fe0 ev_ccf4afe3…, 0x2821f8 ev_21175f0e…; projection 0x23458c ev_3b3195159a…; pad side 0x26be04 ev_c048f16d….

### Writer of 0x376894 (corrects "writer not found", player.md and the 22 readers)

- 0x23f174 (camera manager update, struct at 0x376870) ends with `DAT_00376894 = atan2f(row2.x, row2.z)` of the **camera scene node's world matrix** (node 0x3768b4; row 2 = node+0x7c / +0x84, the forward axis), or 0 when both are zero (store at 0x23f410; `vstreq s16` at 0x23f400 for the zero case). It is therefore the yaw of the view direction in radians, forward = (sin yaw, cos yaw).
- The oracle watchpoint on the word stopped at 0x23f420 right after the call to 0x23c188, in this function, in the frame with the stick pushed. The recorded yaw equals atan2(look.x − eye.x, look.z − eye.z) of the pose on every row of all traces (exact to float precision).
- The player controller 0x26be04 adds `(s16)(yaw * 10430.378)` to the stick heading, so it reads the value of the previous camera update (the manager update runs after the controller in a frame). player.md already records that order.

### Manager and modes (0x376870)

- Manager fields: +0x14 camera object C (pointer at 0x376884), +0x18 optional post callback (0x376888, 0x1c607c shake in the oracle), +0x24 the yaw word, +0x28 requested mode, +0x2c current mode, 0x3768b4 / 0x3768b8 camera scene nodes (two eyes), 0x3768a8 aspect 5/3 (400/240), 0x3768ac 4.0.
- A mode is a table of function pointers, word 6 = active flag. The gameplay mode is the static object 0x377110: enter 0x1984e8, leave 0x1984e4, update 0x1b4a30 (record selection and blend set-up, see data-tables-config.md), 0x217b04, pose step 0x181fe0. 0x226a30 sets the requested mode (about 18 callers: other modes exist).
- Per frame: mode update, then `FUN_001cf0b8(C)`, which advances filters, evaluates the pose into C+0x10 and runs the registered controllers (a blend controller or the follow controller, below), then C+0x4c = pose after the "Filters" smoothing, then the mode pose step 0x181fe0 (0x2821f8 adds a shift that decays to zero unless a record has Type bit 0x100 or 0x200; see corrections), then the matrix and projection.
- Pose layout at C+0x4c (and C+0x10): look point (+0x00..), roll (+0x0c), flags (+0x10, bit 0x40000000 = no rotation), eye (+0x24..+0x2c), FOV in degrees (+0x30), shake and 3D offset (+0x34, +0x38). Matrix 0x2ca814 -> 0x22859c: row 2 = normalize(look − eye) (forward), row 0 = (fz, 0, −fx) normalised (right; for |fy| >= 0.98 it is (0, −fz, fy)), row 1 = row2 × row0, translation = eye. 0x24c118 stores it as the camera node's world matrix.
- Projection 0x23458c(fovRad, aspect, node): f = cot(fov / 2); y scale = f * 4/3, x scale = y scale / aspect. With the pose FOV 40 this is a vertical field of view of 2 atan(0.75 tan 20°) = 30.5°. Checked against gameplay-start.png: the character takes 35 % of the height in the original and 36 % in chase-game with it (27 % with a plain 40° vertical FOV).

### Which controller runs in the street

- 0x19b8c8 (new record set) hands over to **0x1b4864** when exactly one record is active, it is a Room (+0x4e) and DAT_003770c8 == 0. That is the case at gameplay-start. 0x1b4864 registers the controller **0x2a8bfc**, which calls the follow camera update 0x2e0bf0 on the camera system object `K` at 0x589a70 and copies its result into the pose: eye = K+0x34, look = K+0x94 + K+0xf4. Pose FOV in C+0x7c was 40.0 and the roll 0.
- The other path (several Room/Bound records, or DAT_003770c8 != 0) is the record pose blend 0x298cd8 of data-tables-config.md; it was not exercised.
- Filters at C+0xbc..0xd8 (current, target, rate, mode per filter) read 1.0 on this path, i.e. the "Filters" smoothing of the record path is off.

### Follow camera `K` ("OnFoot" preset), law recovered

The update 0x2e0bf0, once per game update (dt = scheduler step, `in+0x4c` = 0x23602c):

1. Preset block (static data 0x376f6c..0x376fdc, copied to `in+0x74`) values for OnFoot, offsets from the block start: distance triple (3, 10, 15) at +0x00; height triple (−1.6, 1.7, 6.4) at +0x0c; ground offset 0.25 at +0x18; look lift triple (0.3, 0.2, −0.2) at +0x1c; look depth triple (0.5, 0, 0.5) at +0x28; auto-turn gain 0 at +0x38; 1.0 at +0x3c; **yaw gain 0.6 at +0x40**; 1.5 at +0x44; **position smoothing base 0.75 at +0x48, look smoothing base 0.75 at +0x4c**; 2.0, 1.0, 5.0, 2.5, 1.25, 0.25, 0.25, 1 at +0x50..+0x6c. InVehicle starts at 0x376fdc and Skydive at 0x377344 (other values, not used here).
2. Strategy pre-step 0x330e4c (zoom S+8 = 1.0, so t = 0.5): each triple (a, b, c) gives the quadratic 0x1e3730 = a + ((4b − 3a) − c) t + ((a − 2b) + c) 2t², which is b at t = 0.5. Horizontal distance D = 10, height H = 1.7, look lift 0.2, look depth 0. Body height = entity +0x64 + +0x70 of the player = 1.15 + 0.85 = 2.0 (oracle). Look point source B = player position + (0, body − 0.25, 0) = +1.75; look offset C = (0, 0.2 + body − (body − 0.25), 0) = (0, 0.45, 0) rotated by the player matrix. Clearance multiplier S+4 = 1.
3. Three tracked points, each with a wanted position and a tracker (0x1ed9ac: tracker += (1 − f)(wanted − tracker)): A wanted camera position K+4, tracker K+0x34 = **eye**; B wanted look point K+0x64, tracker K+0x94; C look offset K+0xc4, tracker K+0xf4. **look = B tracker + C tracker.** f = pow(base, 30 dt): 0.75^0.99 = 0.7522 at dt 0.033, so the trackers move 0.2478 of the way per update (measured 0.24782 on all rows). x and z of B and C are smoothed only when a flag in the input block (`in+0xe0`) is set; it behaves as set in every trace (the code never writes it; stale stack data is possible, not verified).
4. Strategy update 0x3310d0 collects position changes for A:
   - vertical spring: dA.y = 0.3 · ((B.y + C.y + H) − A.y). The 0.3 is (0.5 − S+0x24) · 0.3 · 2 with the timer S+0x24 = 0 in free space.
   - horizontal rope: v = (player − A) with y = 0, unit u = v/|v| ((0, 0, 1) for a zero vector, 0x257ea0), dA += u · (|v| − D). A is pulled to exactly D = 10 from the player every update. The player position used is the node translation, not the tracked point.
   - yaw: b = (shoulder turn + stick turn) · dt · 0.6. A is rotated about B (raw wanted look point): (x, z) → (x cos b + z sin b, −x sin b + z cos b) relative to B. The rotation delta is added to **both A and its tracker** (so the eye turns at once), while the rope delta is added to A only (the tracker follows with the lag). Matrix 0x237c48 with angles (0, b, 0): rows (cos b, 0, −sin b), (0, 1, 0), (sin b, 0, cos b), applied as row vector times matrix (0x20b214).
   - zoom: S+8 relaxes towards 1 at 1.0 per second while the target moves, plus K+0x144 (touch zoom, not wired).
5. In 0x2e0bf0 the deltas are applied: A += rope + rotation, tracker += rotation, then the three trackers are advanced with the factors above (A uses one factor for all axes, B and C use the second base).
6. Input accumulators, written by the player controller 0x26be04 each update and cleared by the camera: K+0x13c (DAT_00589bac) += stick x term, K+0x140 (DAT_00589bb0) += shoulders, K+0x144 (DAT_00589bb4) zoom.
   - Shoulders (0x200ab0): bb0 += L · (−2) + R · 2 (L, R = slots 12 and 13), or L · 2 − R · 2 when the "invert camera" byte 0x51b493 has bit 1. R held turns the view towards +yaw (+0.0396 rad per update at dt 0.033 = 2 · 0.033 · 0.6, matches the oracle exactly); L the opposite; both cancel.
   - Stick (0x200af0..0x200b1c), only for entity types 1..5 and 0x11c (the player is type 3, read from ctrl+0x6c in the locomotion traces): slot 14 value v: (v − 0.25) · 4/3 for v > 0.25, (v + 0.25) · 4/3 for v < −0.25, else 0, and 0 while the entity state at +0x12 of 0x24255c is 0xb7. So a full sideways push adds ±1.0 and turns the camera 0.0198 rad per update with it; the camera therefore swings round when the player runs sideways.
   - Also added to bb0/bb4: touch input DAT_00375fc4 +0x34 / +0x38 (not wired). L and R together call 0x2313c8(K, 0) (camera reset).
7. Start pose: the camera starts behind the player's facing: at gameplay-start yaw −2.3501 = player facing; eye = player + (7.114, 3.9, 7.028) (10 horizontal, 3.9 above the feet = 1.75 + 0.45 + 1.7), look = player + (0, 2.2, 0), FOV 40.

Reference values: gameplay-start player (886.764, 16.589, 1111.166): eye (893.878, 20.489, 1118.194), look (886.764, 18.789, 1111.166), yaw −2.35013.

### Checks (C++ `FollowCamera`, `camera-real-data`)

`chase_runtime` `FollowCamera` replays nine oracle traces (tools/oracle/camera-trace.txt, dt 0.033 and 0.0165) with the recorded player position and the pad pipeline:
- idle, R held, release, L held, release, walking towards the camera (first 28 rows), sideways run from row 17 (after the wall push, below), rest, and a diagonal stick at dt 0.0165: eye error at most 6.1e-4, look error at most 6.1e-5, yaw error at most 6.3e-5 over 323 updates (float32 position resolution at 900 is 6e-5).

### Unknowns (not recovered; absent from `FollowCamera`)

- **Wall and player push-out.** The first 16 rows of the sideways run (camera near buildings at about x 909, z 1111) differ from the model by up to 0.2 units in the direction of an extra tangential push, and the timers S+0x18 / 0x1c (0.5 s) are set then. `0x3313c0` and `0x2e10f4` fill collision sweeps (K+0x180, 0x2d4e64, 0x2d5208, K+0x314, K+0x1ec4, values K+0x1db4..0x1dbc, K+0x3964..0x396c) and push the wanted position by in+0xd0 / in+0xd4 times the penetration. When the player walks through the camera (k_back, rows >= 29) the real camera swings round quickly (up to 0.24 rad per update) with S+0x20 / 0x24 set; the model's rope flips direction there instead. Both depend on level geometry and were not decoded.
- Pitch clamp 0x2e0a50 (keeps the player inside a vertical screen band; limits `in+0xd8` / `+0xdc` unwritten in the code read) did not act in the traces.
- Periodic re-snap: K+0x130 counts down 2 s of free space and then calls 0x2313c8(K, 0x168), which re-places the camera with ray sweeps; the counter stayed 2.0 in traces of at most 80 updates, longer runs were not checked.
- Player fade when the camera is near (0x2e08e8), K+0x14c / 0x23f168 near-plane handling, the touch camera input, the zoom input (S+8 stays 1) and the roll.
- Other presets (InVehicle at 0x376fdc, Skydive at 0x377344), the strategy objects of the vehicle (in+0x48) and the auto-turn (in+0xac, 0 for OnFoot, so the S+0x14 path was never active), the K+0x15c provider for other entities, and what DAT_003770c8 means.
- The pose FOV: 40 in C+0x7c was read at one place; how this controller sets it was not traced. The near and far planes of the camera node were not found.
- The record pose blend (0x298cd8) was not exercised in the oracle; it is the camera of rooms with several Room/Bound records.
- The assumption that in+0xe0 is set (see 3); and the "invert camera" option was not tested at runtime.

### Corrections of earlier entries

- [Corrects data-tables-config.md §2026-10-13T09:32](data-tables-config.md#ts-20261013-0932) ("Type bit 0x100: no reader found"): **0x2821f8 reads it** (`ldrh [record+0x44] & 0x100` and `& 0x200`, called from the pose step 0x181fe0 with the player controller pointer DAT_0058d6ac). With 0x100 it adds a smoothed shift (DAT_00377100 / 0x377104) that follows the controller's displacement heading (ctrl+0xa); with 0x200 a shift along the same heading scaled by the distance. Without either bit the shift decays to zero (smoothing factor 0.06). The earlier scan missed it because the record pointer was passed as an argument.
- [Corrects data-tables-config.md §2026-10-13T09:28](data-tables-config.md#ts-20261013-0928) (record path "is the per-frame camera"): in the street the record path is replaced by the follow controller above; the record path serves rooms with several records.
- [Corrects player-entity-input.md §2026-10-13T10:42](player-entity-input.md#ts-20261013-104227) ("camera yaw writer not found", "entity type unknown"): see above; the player type is 3.
