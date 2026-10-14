# Gameplay camera: records, lock and blending

Camera records, the record pose blend, the camera lock and the command stack with fades.

<a id="ts-20261013-231343"></a>

## 2026-10-13T23:13:43+02:00 — Camera lock: script native CameraRotFixedSet and the HUD icon UI_CameraLocked

Corrected by [camera-records-blending.md §2026-10-14T09:49](camera-records-blending.md#ts-20261014-094953): the police-station lock is the record-pose camera 0x298cd8

Lead, prompted by the user's observation on a real 3DS: outside the police station L/R turn the camera; inside they do not, and a crossed-out camera icon appears when L/R are pressed.
- Script native `CameraRotFixedSet`: registered at 0x16071c through 0x23912c (the native registration routine; name by `add r0, pc, #960` = string 0x160ae4, handler word at literal 0x160ae0 = **0x27bb9c**, r2 = 4, r3 = 8 as for the neighbouring registrations). Found by a scan of pc-relative ADD/SUB immediates whose target is the string; the string has no absolute-address reference.
- HUD icon `Sprites/UI_HUD/UI_CameraLocked.tga` (string 0x193a80): loaded at 0x193a5c (`add r0, pc, #28`, then `bl 0x250244`) and stored at `*(*0x3758b0) + 0x2c`; the same routine calls 0x23cb90 with r0 = 0x518218 (literal 0x193a7c), r2 = 0x1b1150 (literal 0x193a78), r3 = 6, and a float literal 0x3e99999a (0.3) at 0x193a74.
- So the lock is most likely set by level scripts through `CameraRotFixedSet`, and the HUD shows the icon when L/R are pressed while it is set. Not traced: the handler 0x27bb9c, which camera field it sets (candidates: pose flag bit 0x40000000 "no rotation" at C+0x4c+0x10, see the 14:39:58 entry, or the shoulder accumulator path 0x200ab0), which lvl_00 scripts call it, and the icon's show/hide timing (0.3 s?).

Unknowns:
- Arguments and effect of 0x27bb9c; the callers in lvl_00 scripts; the role of 0x23cb90 / 0x1b1150 / 0x518218 in the icon routine.

<a id="ts-20261014-094953"></a>

## 2026-10-14T09:49:53+02:00 — Correction: the police-station lock is not CameraRotFixedSet; it is the record-pose camera 0x298cd8 (REA Ghidra 12.1.2, llvm-objdump, Azahar oracle New 3DS state police-station)

**[Corrects camera-records-blending.md §2026-10-13T23:13:43](camera-records-blending.md#ts-20261013-231343)** (which guessed that level scripts set the lock through `CameraRotFixedSet`).
- `CameraRotFixedSet` (handler 0x27bb9c) takes (focus object, target height, target distance, height offset) (script function list, `levels/scriptfns.txt` entry id 10025). It stores the object's node (+0x3c) and three floats plus 43.0 into the preset block at 0x372d74 through 0x182060 and selects the preset object 0x372d54 through 0x226a30 (manager 0x376870 +0x28). The census `level_census` over all 30 level files finds **one** call in total, in `lvl_07_downtown_main.blvl` (script function list near `CameraFollow` / `EnableTrigger`). lvl_00 never calls it. So it is not the police-station lock, and not a generic indoor mechanism.
- What the oracle shows (state police-station, Azahar New 3DS, OpenGL, `--rpc-input`): with L held for 2 s the camera yaw (0x376894) stays -3.1416 and the HUD shows the crossed-out camera icon in the top-left (screenshot `artifacts/oracle/agentB-out/psL.png`). The preset in the manager is the default preset 0x377110 in the station and in the street (manager+0x28 = +0x2c = 0x377110, both states).
- The difference is the camera command object: `*(*(manager+0x14))+0x78` holds a function pointer. Street (gameplay-start): **0x2a8bfc** (follow controller 0x2e0bf0). Police station: **0x298cd8** (record pose blend). Read at 0x08501c38 (street) and 0x08501ce4 (station).
- Why: the default preset update 0x1b4a30 runs every frame (trap: hits at 0x1b4a30 and the record query 0x1d34ac every frame, 0x19b8c8 only when the record set changes). It calls 0x1d34ac for the player position and hands the record set to 0x19b8c8, which takes the follow path 0x1b4864 (tag 0x2a8bfc) only when exactly one record is active, it is a Room record (+0x4e) and the byte at 0x3770c8 is 0 (it is 0 in both states). Every other case (several records, a Bound record, Focus or Blocker) uses 0x298cd8. In the station the record set does not meet the follow condition, so the camera is the record pose (also the reason the oracle framing there is higher than the follow camera).
- The shoulder buttons are consumed in the player controller (0x2009e4 region of function 0x26be04 / 0x200354): the yaw accumulator at 0x376c78+0x140 adds L and R every frame; the icon routine 0x193aa4 (show UI_CameraLocked: HUD object *0x3758b0, sprite at +0x2c, fade callback 0x1b1150, 0.3 s literal at 0x193a74) is called when L or R is non-zero and `0x217b14()` is 0. 0x217b14 returns 1 only when the active command tag equals 0x2a8bfc. So: **the lock is not a flag; the camera is locked in every place where the active camera command is not the follow controller 0x2a8bfc**, and the icon shows on a shoulder press there.
- Scope: depends only on the level's `.cam` records at the player position, so other interiors with Bound or several overlapping records are locked the same way, streets with one Room record are not. Which rooms of which levels are locked was not enumerated (needs the record volumes, see data-tables-config.md 2026-10-13T09:32). The user's hypothesis (all indoor rooms) is therefore plausible but not proven.
- HUD icon timing beyond the 0.3 s literal and the yaw effect of 0x298cd8 (it ignores the accumulator: yaw stayed exactly constant) were not traced further.

Unknowns:
- The exact record set (names, Room / Bound, Focus / Blocker) at the player start in lvl_00; the full pose of 0x298cd8 (CAMERA-3).
- Which other levels or rooms select 0x298cd8; the show / hide timing of the icon (0x23cc00 argument 0.5).
- Where the byte 0x3770c8 is written (it was 0 in both states).

<a id="ts-20261014-104400"></a>

## 2026-10-14T10:44:00+02:00 — Record selection, record pose 0x298cd8, camera lock icon and lock scope (REA Ghidra 12.1.2, llvm-objdump, Azahar oracle New 3DS states police-station and street-after-1-1)

Evidence IDs: loader 0x3062e4 ev_9636e324…, query 0x1d34ac ev_a69314f3…, volume filter 0x20bf6c ev_4b20c0b7…, search 0x237f30 ev_7176d1e3…, dedupe 0x1aa734 ev_8bf13740…, binder 0x1e9290 ev_161c6427…, update 0x1b4a30 ev_20eb339d…, hand-over 0x19b8c8 ev_578623fa…, follow hand-over 0x1b4864 ev_8f15a259…, pose 0x298cd8 ev_02030dc2…, segment test 0x21c1e4 ev_ec9ce96a…, 0x23b92c ev_48c77a25…, 0x2397a0 ev_3240d814…, 0x204528 ev_16144aeb…, 0x2536c0 ev_ab3f0c52…, 0x21bb78 ev_61f11061…, icon 0x193aa4 ev_7c195249…, 0x23cc00 ev_8d8eaf0a…, 0x1b1150 ev_85050241…. Code: `src/runtime/camera_records.*`, `camera_rig.*`, `camera_lock_icon.*`; tests `camera-records`, `camera-records-real-data`.

**Records (loader 0x3062e4, binder 0x1e9290).** [Corrects data-tables-config.md §2026-10-13T09:10](data-tables-config.md#ts-20261013-0910) wording "level object": the camera records are the **level bounds** (level file section +0x24, `LevelBound`). One 0x50-byte record is made for each bound whose name starts with `Camera` and does not contain `LookAt`, in section order (lvl_00: 94, lvl_01: 332). The `.cam` `Bound` blocks fill them by name; a `Room` block fills the record of a room. Bound field +0x0c is the bounding radius (sqrt of the sum of the squared half extents, checked on lvl_00). 0x1e9290 with a node index (not 255) adds the centre to the translation of the node matrix (`level.boundMatrices`) and zeroes the centre; without a node, shapes 2 and 4 copy the radius +0x1c into +0x20 and +0x24.

**Query 0x1d34ac in detail.**
- 0x20bf6c keeps a record only when its bound is enabled (+0x0a) and the point is inside. The point is moved into the bound's space with 0x2536c0 (p minus translation, then the columns of the matrix; 0x21bb78 multiplies the bound matrix with the world matrix of the owning scene node, taken as the identity here). Shape 2: distance <= +0x1c. Shapes 4 and 6 (0x204528): 3D length of the difference < radius and |dy| < +0x20 (the length helper 0x20e968 is a plain vector length). Others: inclusive box test 0x2397a0.
- 0x1aa734 drops the later record of every pair whose smallest axis overlap (sum of half extents minus the distance of the centres plus node translations) is <= 0.5. After a removal it advances to the next index without testing the element that moved up (decompiler output).
- The first record with Focus or Blocker wins alone. For 2 or more records the weight is the XZ distance between the query point and the point where the segment from the bound centre, along (point - centre) with y = 0, length 2 x radius, leaves the box (0x21c1e4 then 0x22b62c). The slab loop of 0x21c1e4 compares t = half / delta without a sign test, so for a negative delta component it reports the face of the opposite side. The weights are divided by their sum.
- With no record inside, the set is the level's Room record (count 1, +0x4e set).

**Decision (0x1b4a30 and 0x19b8c8).** The set of the last frame (0x58d880, count 0x3770cb) is compared with the new one. If they share no record, 0x19b8c8 runs: one Room record and byte 0x3770c8 == 0 gives the follow controller (0x1b4864 registers 0x2a8bfc unless it is already active), anything else gives the record pose 0x298cd8 with the new set as its blend block (0x58d694: records +0x10, weights +0x30). If they share a record and the sets are equal, only the weights are refreshed (when more than one record). If they share a record but differ and a new record is in the blend block, the block is updated in place; otherwise 0x19b8c8 runs.

**Record pose 0x298cd8.** T is the player node position (0x58d8a0 transformed by the node; second target absent). For each record: P.x = Location.x (Type bit 0x01) or T.x + Location.x; P.y = Location.y (bit 0x02) or T.y + Location.y; P.z likewise (bit 0x04); the second-target factor 1.1 applies to x and z only. Look point: bit 0x08 / 0x10 / 0x20 clear gives T + Offset, set with the position bit clear gives T, set with the position bit set gives P + Offset; bit 0x400 overrides all three with P + Offset. FOV = byte +0x49, 3DOffset = +0x40. The pose is the weighted sum of look point, eye, FOV and 3DOffset; with more than one record the eye is rebuilt as look + normalize(eye - look) x the weighted sum of |look - eye| (0x23b92c(f, out, base, dir) = base + dir x f).

**Oracle check.**
- police-station (player node (-1.97040856, 1.43587279, 14.84306526)): command tag 0x298cd8; one record at 0x08e78640 = `Camerabound_01` (Type 0|0x80, Location (0, 5.5557, 8.3147), Offset (0, 2, 0), FOV 43, blend block count 1, weight 1); camera C+0x4c: eye (-1.9704089, 6.9915738, 23.1577549), look (-1.9704088, 3.4358723, 14.8430634), FOV 43, yaw pi. The replica gives eye (-1.9704, 6.991602, 23.15779), look (-1.9704, 3.4359, 14.8431), yaw pi (the replica input has 4 decimals). The block C+0xbc.. reads 0.1936 / 0.1296 there (its use in motion was not traced); the pose at rest equals the sum of the records.
- street-after-1-1 (player (900.549, 16.567, 1110.280)): tag 0x2a8bfc, one record = the Room record (+0x4e = 1, no bound), blend list head 0. gameplay-start was read earlier (0x2a8bfc). The replica selects the follow controller at that point.
- Pictures (local): `artifacts/oracle/agentF-out/ps_oracle.png` (oracle), `ps_cg.png` (chase-game `--gameplay --start "-1.9704 1.4359 14.8431" --facing 180`), side by side `ps_side.png`: the framing matches (eye height, distance, FOV 43); the character faces another way because the oracle player faces (-0.67, -0.74). `ps_cgL.png`: chase-game with L held for 30 updates, the camera stays and the icon shows.
- Walking in the oracle state was not possible (the player is held there, intent 0 state 0), so the pose in motion and a two-record pose were not measured. No point of lvl_00 has two records after the dedupe (scan of the lobby, y 0.5 to 6, grid 1.5).

**Lock icon (0x193a1c create, 0x193aa4 show).**
- 0x193a1c builds the object at *0x3758b0: sprite `Sprites/UI_HUD/UI_CameraLocked.tga` (archive `sprites/ui_hud/ui_cameralocked.btga`) in +0x2c, fade times +0x14 = +0x18 = 0.3 s (literal 0x3e99999a at 0x193a74), draw callback 0x1b1150. 0x193aa4 calls 0x23cc00(0.5, object, 0) when 0x21d744() and 0x236008() are both 0 (0x21d744 returns the u16 at *0x51998c + 0xf4, 0x236008 the byte at *0x51b430 + 0x4c9; meaning not traced); 0x23cc00 stores the hold time 0.5 in +0x1c.
- Read in the oracle while L was held and released (object fields polled at about 50 Hz over RPC, state byte +0x10): 0 hidden, press gives 1 fading in for 0.3 s, then 2 shown while triggers keep arriving (+0x28 holds the time of the last trigger), 0.5 s after the last trigger 3 fading out for 0.3 s, then 0 (observed 0.59 s from the release to the fade out and 0.31 s for the fade). A trigger in state 3 returns to state 1 with the start moved so that the alpha continues (0x23cc00 case 3: start = now - (1 - progress) x fade in). 0x1b1150 draws the sprite at (10, 240 - 10 - height) of the top screen with alpha = fade x 255 (confirmed by `agentB-out/psL.png`, bottom left).
- 0x217b14 returns 1 only for the follow controller, so the icon shows (and the yaw does not move) whenever the command is the record pose.

**Lock scope (replica, `tools/camera_lock_census.cpp`, target `camera-lock-census`).** The census takes every walkable cell (nav grid cell with flag bit 0 clear, at the fitted floor + 0.1) of the level's rooms and counts the cells for which the query returns something other than the Room record. All level bounds are treated as enabled, the node matrices as static, and the query order as the level order.

| Level | camera records | walkable cells | locked cells |
|---|---|---|---|
| lvl_00 police station | 94 | 41922 | 41540 (Room1 20551 of 20560, Room3 all, Room5 2205 of 2578) |
| lvl_01 Cherry Tree Hills | 332 | 125943 | 24902 |
| lvl_02 Albatross Island | 259 | 176915 | 115381 |
| lvl_03 Auburn | 200 | 514767 | 17322 |
| lvl_04 Fort Meadows | 193 | 12110 | 987 |
| lvl_05 Bluebell Wood | 202 | 156445 | 84896 |
| lvl_06 Apollo Island | 245 | 103588 | 77095 |
| lvl_07 Downtown | 297 | 85326 | 42191 |

Rooms in which every walkable cell is locked: lvl_00 Room3; lvl_01 Room34, Room35; lvl_02 Room9, Room15; lvl_04 Room41; lvl_05 Room45, 46, 48, 49, 52, 53, 56, 57; lvl_06 Room20, 27, 36; lvl_07 Room36, 40, 41, 62. In the street rooms of lvl_01 the lock covers parts only (Room15 3274 of 6667, Room18 5678 of 18057, Room10 493 of 6122), and many rooms have no camera bound at all and are never locked (lvl_01 Room3, 9, 11).

**Verdict on "all indoor rooms are locked":** not as stated. The lock is tied to the `Camera*` bounds, not to rooms or indoors: where one or more of their volumes contain the player the camera is the record pose and locked; elsewhere the Room record applies and the follow camera runs. The police station is locked almost everywhere (99 percent of its cells) and several interiors in other levels are completely locked, but outdoor areas are locked wherever a volume covers them, and rooms without such volumes stay unlocked. Which rooms are indoors was not classified, so an "indoor but unlocked" example was not searched for.

Unknowns:
- The order in which the spatial structure at 0x598300 returns the records (level order is used; it matters for the dedupe of shallow overlaps and for the first Focus / Blocker record).
- Multi-record weights and poses were not compared with the oracle (no two-record point found in lvl_00); the slab loop of 0x21c1e4 was read from decompiler output only.
- The fade between two poses (0x224094 / 0x224080, easing and the filter block), the fixed pose of a Blocker record (0x58d42c..0x58d464), the shake 0x1c607c, the second follow target, the writer of byte 0x3770c8 and of counter 0x3770e0.
- Which camera bounds are switched off at the level start (EnableBound), whether moving bound matrices (lifts, vehicles) change the selection, and what 0x21d744 / 0x236008 test.
- How 0x1b4864 seeds the follow camera when it takes over from the record pose.

<a id="ts-20261014-120746"></a>

## 2026-10-14T12:07:46+02:00 — Fade between camera commands, pose filter, stack merge (REA Ghidra 12.1.2, llvm-objdump, Azahar oracle New 3DS states police-station and street-after-1-1; CAMERA-8)

Evidence IDs: 0x224094 ev_b9877321…, 0x224080 / 0x1a3da8 ev_3ac38cda…, manager 0x23f174 ev_457e8a31…, stack update 0x1cf0b8 ev_6e8609bb…, 0x2a7af0 ev_662f11c2…, blend 0x2a00dc ev_c6d47d4f…, sum 0x2cb5dc ev_9709e610…, merge 0x2c584c ev_f29fe53d…, 0x19b8c8 ev_578623fa…, 0x1b4864 ev_8f15a259…, 0x287a90 ev_a3181492…, lerp 0x221e5c ev_0eeb0fc3…. The stack update 0x2ae250 and the curve code were read with llvm-objdump (`--mattr=+vfp3`). Replica: `CameraRig` (`src/runtime/camera_rig.*`); test `camera-blend-real-data` (traces under `artifacts/oracle/camera-blend-traces/`, scenarios `tools/oracle/camera-lobby-walk.txt` and `camera-street-teleport.txt`, converter `tools/export_camera_blend_trace.py`). Corrects the "fade between poses" unknown of the 2026-10-14T10:44:00 entry: the original does not jump to the settled pose.

**Command stack (camera object C = 0x08501ad0 in both states, manager 0x376884).**
- C+0x00 is the newest entry, C+0x04 the free list, C+0x0c the capacity (4), C+0x0d the count. Entries are chained through +0x94 (older entry); +0x00..0x38 is the entry's pose (look at +0x00, eye at +0x24, FOV +0x30, +0x34, +0x38), +0x78 the command routine (0 once the entry was merged), +0x80 its argument, +0x84 the fade flag byte, +0x88 the fade time, +0x8c the curve id (u16), +0x9c the weight, +0xa0 the progress, +0xa4 the progress per second, +0xa8 the start time.
- 0x224094 only fills a 0x1c-byte command header {routine, destructor, argument, flag byte +0x0c = 1, time +0x10, curve u16 +0x14 = 5, byte +0x18}. 0x224080 pushes it: when the flag is 0, or 1 with time 0, the whole stack is cleared (instant switch); when the count equals the capacity, 0x2c584c merges the oldest two (below); the entry is built by 0x2a7af0 (progress 0, weight 0, rate = 1 / time, or 10000 for time 0) and its routine is called once.
- Fade times: 0x19b8c8 (record pose) uses the plain mean of the records' BlendTime (+0x30) with curve 5; 0x1b4864 (follow controller) uses 0.5 s (literal 0x3f000000 in the command) with curve 5. Oracle: the PolicePK_03 entry has rate 0.625 (1.6 s), the follow entry rate 2.0.
- Per update 0x2ae250 (called from 0x1cf0b8 inside the manager update): every entry's routine is called with the current player position (old commands keep tracking), then from the newest entry down: progress += dt x rate (clamped at 1; rate 0 gives 1), weight = curve(progress) x remaining, remaining -= weight; the oldest entry gets the remaining weight. If the oldest weight is exactly 0 it is removed. Curve 5 is 0x2b5950 = x x (3 - 2x) (smoothstep). The curves are 12-byte entries at *0x376ad8 (routine pointer at +0, flags at +0xa); entry 4 is 0x1dd648 = 1 - (1 - x)^2.
- 0x2cb5dc sums the entries' stored poses times their weights (look, eye, FOV +0x30, +0x34, +0x38) and divides by the sum of the weights. So the FOV is weighted by the fade too (oracle: 40.0038 one update after the push, 43 at the end).
- **Merge 0x2c584c** (push with 4 entries on the stack): the two oldest entries get their weights normalised to a sum of 1, their stored poses are summed with 0x2cb5dc into the second-oldest entry, which keeps the weight sum, loses its routine (+0x78 = 0, so its pose stays frozen) and stays on the stack; the oldest entry is released. The oracle lobby walk over five records needed this (the fifth push reused the oldest slot 0x08501c6c); without the merge the replica was off by 0.044 after the fifth push.

**Pose filter (0x2a00dc, called at the end of 0x1cf0b8).** The weighted pose goes into C+0x10, the output pose C+0x4c is then `out = out + (new - out) x factor` per update (frame based, not time based): eye (+0x24) and +0x34 / +0x38 use the factor of slot 1 (C+0xcc), look (+0x00) the factor of slot 0 (C+0xbc), the FOV is taken from the sum. The factors are set when a command is pushed: 0x19b8c8 reads the first record's bytes +0x4c (slot 1, eye) and +0x4d (slot 0, look): byte 0 gives 0.3 (0x287a90), otherwise `1 - ease4(byte x 0.02)` = (1 - 0.02 byte)^2 (32 gives 0.1296, 28 gives 0.1936, as read in memory), set at once (rate 1.0, 0x21a538). 0x1b4864 sets both targets to 1.0 with rate 1.0, or 0.1 when the camera is near the follow position (see unknowns); in the oracle both factors read 1.0 in the street and after the hand-over from the record pose.
- Nothing else moves the pose in motion: no spring, no clamp. The record pose is re-evaluated with the new player position every update (0x298cd8) and the filter gives the lag.

**Oracle verification (GDB route, New 3DS, `dt` fixed to 0.033).**
- State `camera-lobby-walk` = police-station unmodified. `stick 0 150` for 150 updates (one record, Camerabound_01): eye, look and FOV replayed to 4.1e-6 / 2.1e-6 / 0. `stick 150 0` for 130 updates, the player walks west over five records (top entry pointers 0x8501c6c, bc0, d18, dc4, then c6c again after the merge): replay 5.9e-6 / 2.2e-6 / 0, the weights of the newest entry (+0x9c) within 3e-7 and the entry count equal in every update.
- State street-after-1-1 with the teleport command (the player node written to (933, 16.6, 1124) inside `CameraBound_PolicePK_03`, 80 updates, then back to (905, 16.6, 1110), 80 updates; one continuous run after 20 rest updates): follow to record pose and back both happen with the structure above. The replica's weights, entry count and FOV match in all 160 updates (weights within 5e-7). The poses do not match while the follow camera is a fading entry: the replica's follow controller reacts differently to the 33 m jump (first update: recorded follow eye +4.2 m, replica +3.5 m; no collision in the test) and the follow entry's seed after the push differs (recorded eye 4.59 m behind the player, y player + 2.0, look player + 1.97; the replica places it 10 m behind). The test compares poses only from update 60 of the first trace (error 0.13, shrinking) and not on the way back.
- Pictures: OpenGL screenshots of the oracle during the east walk were taken (`artifacts/oracle/agentI-out/compare/lobby_oracle_*.png`), but the RPC screenshot lets the game run during the capture, so the moment is not pinned to an update; the numeric replay above is the comparison. No side-by-side with chase-game was made. Correction (lead, merge review): the walk itself agrees. From the same start (-1.9704, 1.4359, 14.8431), `chase-game --facing 0 --script "0 0 2;150 0 130"` moves the player 15.46 units, and the oracle trace `lobby_east` moves it 15.22 units in its 130 rows. The earlier note (the player stops after 0.91 units, the original walks about 9) came from a malformed `--script` argument. It is not a player or collision difference.

Unknowns:
- The follow controller's seed when it is pushed (0x2313c8 with 0x168 or 0, then the first update of 0x2a8bfc): recorded first pose eye 4.59 m behind the player (905 -> 909.59), y 18.60 = player + 2.0, look = player + (0, 1.97, 0), growing to 7.65 m within 15 updates.
- The condition of the 0.1 factor ramp in 0x1b4864 (distance of DAT_00589ad4 to the follow position against 2 x *(DAT_00589bd0 + 4)): never seen true; both factors were 1.0 at once.
- The follow controller's reaction to a 33 m jump and its pull-in with collision are not reproduced (the replica ran without collision).
- Blocker records (0x58d42c) and the shake 0x1c607c are still not applied; the parameter `param_6` of 0x19b8c8 (no fade) is read as 0; the stack capacity 4 was read from C+0x0c only.
- Multi-record weights: the lobby has no overlapping records; the walk only shows consecutive records.

<a id="ts-20261014-133954"></a>

## 2026-10-14T13:39:54+02:00 — Details moved from feature items CAMERA-3, CAMERA-9

Moved from the feature files so that the items stay short (CLAUDE.md, "Documentation rules"). The text below is the former wording of each item.

- **CAMERA-3** (camera.md): Record selection, record pose, fade and pose filter are in; open: the multi-record weights and pose against the oracle (the lobby has no overlapping records; needs a place with two overlapping records and a free player), Blocker pose, shake, query order, pitch clamp and other presets. evidence: [camera.md §2026-10-14T10:44](camera-records-blending.md#ts-20261014-104400), [camera.md §2026-10-13T14:39](camera-follow.md#ts-20261013-143958).
- **CAMERA-9** (camera.md): The follow controller in a fade: its seed when it takes over from a record pose (original about 4.6 m behind the player, y player + 2.0), its reaction to a large jump of the player and the factor ramp 0.1 of 0x1b4864 are not reproduced, so the poses of the street hand-over replay only match late (CAMERA-S5). Needs the follow controller with collision in the test and a walked (not teleported) hand-over. evidence: [camera.md §2026-10-14T12:07](camera-records-blending.md#ts-20261014-120746).
