# In-engine cutscenes

Reverse-engineering findings, moved verbatim from the progress log. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis.

## Current state

- Track records are 0x18 bytes (u16 key count at +4); 1954 animations, 62293 keys decode. [§2026-10-13T02:30](#ts-20261013-0230) Keys are evaluated by 0x252a7c: u linear in time, the handle values weighted by interval / handle delta (clamped 0.01 to 100), a key with out delta 0 and its own out value holds until the next (hard cuts). [§2026-10-27T18:38:05](#ts-20261027-183805)
- Cutscene folders `csdata/models/<id>` (69) list their cast in `manifest.bstxt`; the camera handler 0x1c1fbc reads position, rotation and the lens: focal length (mm, default 35) and horizontal aperture (inches, default 1.41732) give the pose FOV 2 atan(aperture · 12.7 / focal) · 4 / (3 · screen aspect) (0x2cd650), projected by 0x23458c; this was the cause of the wide shots in `1_1_d` (MOVIE-33). [§2026-10-27T18:38:05](#ts-20261027-183805) The oracle confirms the pose FOV, the projection and the eye of `1_1_d` and `1_1_b` (screen aspect 5/3). [§2026-10-27T18:50:42](#ts-20261027-185042)
- The camera matrix (0x20e850) negates x; the world is left-handed, so the row flip alone mirrors the signs. [Corrected by levels.md §2026-10-13T12:49](levels.md#ts-20261013-1249)
- Joint orientation: after sampling, the skeleton record's +0x88 quaternion is multiplied in (Hamilton product, 0x2485d8). [§2026-10-13T02:30](#ts-20261013-0230)
- Cutscenes play at 30 frames per second (data evidence); soundtrack `music/csaudio/<id>.bwav` exists for 57 of 69. [§2026-10-13T03:20](#ts-20261013-0320)
- Natives work on 8 slots of 0x24 bytes at 0x58976c: Load takes a free slot and builds the object, Play starts the cutscene mode without blocking, Wait returns `!busy(name)`, Free clears the object. [§2026-10-14T22:49](#ts-20261014-224929)
- One screen fade (block 0x529e30) serves FadeScreen and the cutscene mode; the DCamCutScenePlay handler 0x26fa18 fades to black (0.5 s with flag a, else at once), the mode enter 0x2fadb8 fades in 0.5 s, its update 0x2fae68 fades out 0.5 s at the end and the exit 0x2fac7c fades in 0.5 s (not after `7_9_A`). [§2026-10-19T09:12:06](#ts-20261019-091206) The draw 0x24bda4 paints a black quad of opacity |1 - value| (blend source alpha / one minus source alpha, [script-text-bottom-screen.md §2026-10-19T23:15:01](script-text-bottom-screen.md#ts-20261019-231501)); the screen byte of the start (cutscene fades: 1 = top screen only; FadeScreen: the ShowDialogBG byte) is gated by 0x1087ec; the mode switch waits for the running fade and two render ticks before the exit. [§2026-10-19T10:04:29](#ts-20261019-100429)
- The level start plays `01_GameIntro` and then `1_1_a` through the `startup` script. [§2026-10-14T23:52](#ts-20261014-235214)
- The cutscene object update 0x2d19cc reports the end when the camera clip's slot state is 6. [§2026-10-27T18:38:05](#ts-20261027-183805)
- Open: Load's retry field `args[0]+0x44`, flags b, c, d, the push path of the mode switch, 113 actor animations with a frame count differing from the camera, the source of the screen size at 0x529dc8 that the pose FOV divides (measured 5/3), the readers of pose +0x34 / +0x38.

<a id="ts-20261013-0230"></a>

## 2026-10-13T02:30+02:00 — Cutscene tracks, cameras and joint orientations (REA, Ghidra 12.1.2)

Corrected by [cutscenes.md §2026-10-27T18:38:05](#ts-20261027-183805): keys are evaluated by 0x252a7c (linear time, weighted handles, step keys), not a Bezier solved for time; 0xc251f0c2 is the focal length and the fields 7 to 11 are lens apertures and pose words
Corrected by [levels.md §2026-10-13T12:49](levels.md#ts-20261013-1249): the row flip alone leaves the POLICE signs mirrored; the world is left-handed

- Track records (0x18, loader 0x253764, ev_8cc8ebc4…997f): u16 +4 key count, u16 +6 extra entry blocks. Blocks: name hash, pointer array, entry blocks, key times (f32), key values (5 × f32: in-delta, in-value, value, out-delta, out-value). The earlier draft read +4 as u32, which overlaps +6; fixed.
  - Keys are evaluated as cubic Bezier curves in normalised time 0..1 (time solved by bisection). The curve shape is a data fit and has not been traced in code.
  - Census: 1954 animations, 3804 sub-animations, 14564 tracks, 62293 keys; every key block matches its count.
- Cutscene folders csdata/models/<id> (69): manifest.bstxt is one pooled text block of "Group/file.fnmdl|fnskl|fnanm" lines. 397 actors. Only one listed file is missing from the archive: 2_7_c/laundrycart/laundrycart.bfnskl.
- Camera handler 0x1c1fbc (ev_45b783ec…1e4b): position hashes 0x191a8df3..5, rotation 0x66c4d914..6, fov 0xc251f0c2 (default 35), plus five unknown fields with defaults 1.41732, 0.945, -, 8.0, 0.035. These were checked against the switch constants.
  - 0x20e850 (ev_8238b1ac…4559) builds the camera matrix from sin/cos (0x249038 → 0x247ac8 = sin, ev_4caeb96f…a49a) with the x position negated. cameraMatrix replicates it.
- Actor transforms: event 0xb945fcd8 in 0x287ca4 (ev_14988793…b8ae). Its absolute branch sets position and yaw directly. Its accumulating branch subtracts x and yaw, adds y and z, and adds weighted scale to a base. actorPose uses the accumulating signs. Check: actors and camera both land near (815, 24, 1068) in 1_1_a, in Cherry Tree Hills.
- Joint orientation: after sampling, 0x12a2b4 (ev_210a33ed…3420) calls 0x2485d8 (ev_6c72e2e4…a73d) with the skeleton record's +0x88 quaternion. If −1 < w < 1, the sampled rotation becomes pose ⊗ q (0x247f3c, ev_75f6f15b…ab5f, Hamilton product) and mask bits 0–2 are set.
  - The bear's orientations are identity (its references are unchanged). Minifigs and the squirrel carry real orientations.
  - Before the fix, minifigs posed with detached heads and twisted limbs, and the squirrel's ears lay flat. With it, both look right. New squirrel and minifig references are recorded in assets-real-test.
  - The extra curve (anim+8 bit 28) is sampled after the last joint (flag 0x20 path in 0x12a2b4). Its use (root motion?) is not recovered.
- Viewer `--cutscene`: plays the cast and camera. The game camera looks along +row 2:
  - In 1_2_a and 2_7_a the actors lie along +row 2 (dot product 0.97).
  - With rows 0 and 2 flipped for GL, 1_1_a shows the police station, the player walking up and the donut counter.
  - This is inferred; the matrix's consumer 0x241540 was not traced.
  - Correction (2026-10-13, see levels.md): the "POLICE" signs read mirrored with this flip alone; the world is left-handed and the renderer's projection now mirrors x. The row flip itself stays.
- Unknown: 113 of 397 actor animations have a different frame count from their camera; the viewer holds their last frame. The meaning of camera fields 7–11 and of fov's axis is unknown, as is the base value the actor scale is added to.
- Rule 6 violation found and fixed (next entry): Python posing (convert_anims.skin_matrices) did not apply the joint orientation and so differed from C++.
- ctest PASS 8/8 (new viewer-cutscene-smoke); Python PASS.

<a id="ts-20261013-0320"></a>

## 2026-10-13T03:20+02:00 — Cutscene playback rate and soundtrack

- music/csaudio/<id>.bwav exists for 57 of the 69 cutscene folders (12 kHz).
- The audio length is camera frames / 30 + 1.0 s in most cases, for example 1_1_a (405 → 14.5 s), 1_1_b, 1_1_c, 1_1_d and 2_7_a. It is exactly frames / 30 for 1_4_a and 2_6_b.
- Outliers: 6_4_a (221 frames, 23.0 s), 2_11_a and 1_4_c.
- Conclusion: cutscenes play at 30 frames per second (data evidence; the game's timer code was not traced). This matches the viewer's default `--fps 30`.
- The viewer's `--cutscene` plays the soundtrack through chase_audio and an SDL audio device. The picture clock restarts with the sound, holds the last camera frame through the tail, and loops after max(frames / fps, sound length). No sound plays with `--frame` or `--screenshot`.
- ctest PASS 8/8.


<a id="ts-20261014-224929"></a>

## 2026-10-14T22:49:29+02:00 — Cutscene natives, slot table and the oracle run of 1_1_b (llvm-objdump on `artifacts/game/game.arm.elf`, oracle run on `gameplay-start`; REA was not used)

Corrected by [cutscenes.md §2026-10-19T09:12:06](#ts-20261019-091206): the handler bound to DCamCutScenePlay is 0x26fa18, which fades and sets byte 0x376d18 before it calls 0x2889e4, so scripts always start mode 4.

Handlers read: DCamCutSceneLoad 0x2889a4 (loader 0x196390), DCamCutScenePlay 0x2889e4 (0x1964dc), DCamCutSceneWait 0x288a40 (busy test 0x2297b0), DCamCutSceneFree 0x28898c (0x196144), PlayCutscene 0x1bf9dc, PlayCutsceneNoFadeBackIn 0x2b9dac, DCamCutsceneFadeDisable 0x2c09ec, cutscene end 0x2fac7c.

- Slot table: 8 slots of 0x24 bytes at 0x58976c; a 32-byte name at +0, the cutscene object pointer at +0x20. Names compare with 0x25c8ac. The block at 0x58988c holds the active object at +0x28 (0x5898b4), c at +0x2c and d at +0x30.
- Load: when the name is in a slot it does nothing. Otherwise it takes the first free slot, copies the name, builds `DCSData/models/%s/manifest.bstxt`, looks it up (0x21bbb0), allocates 672 bytes (0x255254), initialises them (0x2d236c) and stores the pointer at +0x20. The native first returns 0 (the script retries) while `*(args[0] + 0x44)` is 0; what that field is is not known.
- Play: finds the slot by name, stores the object as active (+0x28), c and d as 0.5 or 0 (+0x2c, +0x30), stores `byte 0x37751d / 10.0` at object +0, and starts the cutscene mode with 0x25d26c (mode 4 when byte 0x376d18 is set, else 6) with a and b as 0.5 or 0 (the two floats). It does not block. Oracle after `1_1_a` (flags 0 1 1 1): +0x2c and +0x30 hold 0.5.
- Wait returns `!busy(name)`; busy is true when a slot named `name` (any slot for an empty name) holds the active object. WaitForDCam's busy test 0x236104 includes it with an empty name.
- Free ends the slot: object freed, slot object 0, the name stays in the slot memory (oracle: slot 0 still reads "1_1_a" with object 0 after Free).
- PlayCutscene returns 1 once the movie mode reached state 3, and its re-entry is the thread's blocked flag (+0x14 bit 4). The second argument of the shared body is 1 (fade back in) for PlayCutscene and 0 for NoFadeBackIn, which first sets byte 0x5163e8 + 0x184. DCamCutsceneFadeDisable sets byte 0x529e30 + 0x39d.
- Oracle (`gameplay-start`, OpenGL, `freestick`, `shotloop`): standing within 0.62 m of the waypoint (817.35, 1071.20) starts `1-1_VanBreakdown` (1.0 and 1.1 m did not; a teleport onto the waypoint does). The gameplay picture fades to black (about 0.6 s; `1_1_b` is played with a = 1), then the cutscene shows: top screen without HUD and with the dialogue box, bottom screen the hexagon dialogue background (ShowDialogBG). At the end the picture fades to black, then the game camera is behind the player again, the HUD is back and the tip "Head back to the police station..." follows. The active pointer is non-zero during the cutscene and 0 afterwards.
- Port: `ScriptRunner` handles the natives (`natives_cutscene`), `chase-game` loads the cast into the level scene, poses it from the cutscene time, plays the soundtrack, uses the cutscene camera, hides the HUD and holds the player. Frames of `1_1_b` at 0.5, 1.7, 3.1 and 3.7 s match the oracle in camera and dialogue box. Differences: the port's actors have no faces and wrong clothes, the bottom screen background differs, and there is no fade.
- Save state at the end of the cutscene 1_1_b (agent Q, Old 3DS, from `gameplay-start`: the player was teleported onto the donut shop waypoint (817.3, 24.0, 1071.4), `1-1_VanBreakdown` started `1_1_b`, saved with the cutscene near its end; game progress: new game, mission 1-1 started).

Unknowns:
- What `*(args[0] + 0x44)` of Load is, the meaning of Play's flags a, b, c, d (a = fade out before the start fits the oracle), what ends the cutscene object (the port ends at max(camera frames / 30, soundtrack)), and the cutscene fades at the end.
- Whether Free of a playing cutscene ends it, and what Play of a cutscene that was not loaded does.
- The oracle length of `1_1_b` was not measured (the saved state is near its end).

<a id="ts-20261014-235214"></a>

## 2026-10-14T23:52:14+02:00 — Cutscene fades measured in the oracle, the level start plays the intro (llvm-objdump on `artifacts/game/game.arm.elf`; Azahar OpenGL, RPC, port 24711, save state at the end of the cutscene 1_1_b and `gameplay-start`; REA was not used)

Corrected by [cutscenes.md §2026-10-19T09:12:06](#ts-20261019-091206): 0x25ad90 is a string compare that is non-zero when the strings differ, so 0x2fac7c fades in after every cutscene except `7_9_A`; the fades of the cutscene mode are read there.

- Cutscene end 0x2fac7c (the cutscene object's teardown): when byte 0x376d18 is set it compares the object's path with the string `DCSData/models/7_9_A/` (0x2fad98) and only for that cutscene starts a fade of 0.5 s (0x252068(colour 0x1166bc, 1, 0, s0 = 0.5), a wrapper of 0x256a14 with s1 = 0, s2 = 1); then it frees the object and clears +0x28 / +0x50. So the end fade of `1_1_b` does not come from this function; the generic fade belongs to the cutscene mode that 0x25d26c starts (a pushed event record in the table at 0x58613c, 12 bytes each, with the float a as its value) and was not traced.
- Oracle `endfade` (save state at the end of the cutscene 1_1_b, `shotloop` back to back, 274 shots, mean brightness of the top screen; shots not kept): the top picture is clear until vblank 118, falls linearly to black at vblank 148 (30 vblanks, 0.5 s), stays black for about 4 vblanks, the active cutscene pointer (0x5898b4) becomes 0 at vblank 149, then the game picture (camera behind the player) fades in linearly over about 43 vblanks (0.7 s) up to vblank 194. The bottom screen does not fade; it changes to the map at vblanks 174 to 194.
- Oracle `startfade` (state `gameplay-start`, teleport onto the waypoint at vblank 20): the active pointer is set at vblank 57 and the top picture falls from 60.5 to 25.7 over the next 32 vblanks (0.5 s, linear) with the HUD still drawn at the start; then the cutscene picture appears. `1_1_b` is played with flags 1 1 1 1; `1_1_a` with 0 1 1 1, so flag a is the only difference and fits the fade before the start (a = 0 has none).
- Port: `CutsceneNativeState` (`src/runtime/natives_cutscene.*`) delays the start by 0.5 s when flag a is set (the script's Wait blocks through it, the picture stays the game picture), ramps the top-screen veil to black over the last 0.5 s of the cutscene, keeps it black 4 vblanks and fades in over 43 vblanks. The times are measured values (stand-in MOVIE-S20). The end fade applies to every cutscene because no flag is known to control it (b, c, d are 1 in both known calls).
- Level start: the `startup` run now yields as soon as it starts a movie or an in-engine cutscene, and the game loop continues it, so a plain `chase-game --level lvl_01` plays `01_GameIntro` (PlayCutscene, `1-1_IntroPlayed` guard in the script) and then `1_1_a`, then `Wait 1` and the first tip, as the script orders them. The front end path still plays the movie before the level and skips it in the level (MOVIE-S12). `--no-start-cutscenes` keeps the old preroll for development runs (the tests use it).

Unknowns:
- The generic fade code of the cutscene mode (the consumer of the 0x25d26c record), what flags b, c and d control, whether `1_1_a` fades at its end like `1_1_b`, and whether the fade-in after the end is part of the cutscene mode or of the camera return.
- The 0.7 s fade-in may include the camera blend back to the game camera.

<a id="ts-20261019-091206"></a>

## 2026-10-19T09:12:06+02:00 — One screen fade for FadeScreen and the cutscene mode, the DCamCutScenePlay handler 0x26fa18 (llvm-objdump on `artifacts/game/game.arm.elf`; Azahar GDB `freetrap`, OpenGL, port 24771, save state at the dog cage trigger of mission 1-2, no RPC; REA was not used)

[Corrects §2026-10-14T22:49:29](#ts-20261014-224929) (the handler bound to DCamCutScenePlay), [Corrects §2026-10-14T23:52:14](#ts-20261014-235214) (the fade of the cutscene end 0x2fac7c) and [Corrects script-natives.md §2026-10-14T13:33:39](script-natives.md#ts-20261014-133339) (the two handler sources agree).

**The screen fade (block 0x529e30).** The start 0x256a14(kind r0, byte r1, colour r2, s0 = seconds, s1 = start, s2 = rate) stores the kind (a draw function: 0x1166bc for the black fade, 0x1198d8, and 0x1166c0 = no fade, set by the init at 0x105c98) at +0x114, r1 at +0x118, the colour at +0x12c (r2's vector, else white for kind 0x1198d8 and black otherwise, alpha 1) and sets the timer at +0x11c: duration = seconds in clock ticks (0x114aa4), position = start x duration (0x1150e0), rate (0x1140a0). The position (0x1202f8) is (clock - start tick) x rate, clamped to [0, duration]; the value 0x252880 is position / duration, and 0 when the duration is 0. 0x2520e8 = start 1, rate -1 (to black); 0x252068 = start 0, rate +1 (from black). The test 0x25207c returns 0 for kind 0x1166c0, else 1 while the value is below 1 (rate >= 0) or above 0 (rate < 0). FadeScreen 0x1a2e8c starts 0x2520e8 for 0 and 0x252068 otherwise, kind 0x1166bc, s0 = 0.5, r1 = 0x1d9584() != 0, and returns `!0x25207c()`. Every start replaces the fade before; there is one fade for the whole game.

**DCamCutScenePlay is 0x26fa18.** The registration at 0x1625e8 (`bl 0x23912c`, name at 0x16290c, 5 arguments) binds `DCamCutScenePlay` to 0x26fa18; the static table entry at 0x359528 holds 0x2889e4, which 0x26fa18 tail-calls. Arguments are 8 bytes apart (argument 0 at +4, the next at +0xc; the same in 0x2889e4). 0x26fa18 sets byte 0x376d18 to 1, clears bytes +0x23 and +0x24 of the block 0x5163e8, and then: flag a set: 0x2520e8 with 0.5 s; flag a clear: when byte +0x10 of the current game mode (0x2369c4) is 1, byte 0x374600 = 1 and no fade, else 0x2520e8 with 0 s (black at once). Then 0x2889e4 → 0x1964dc, which pushes mode 4 (byte 0x376d18 set) through 0x25d26c.

**The cutscene mode (vtable 0x363134: +8 enter 0x2fadb8, +0x1c update 0x2fae68, +0x20 exit 0x2fac7c).** Enter: with byte 0x376d18 set, 0x252068 with 0.5 s (from black). Update: when 0x2d19cc(cutscene) returns 0 and the byte is set, 0x2520e8 with 0.5 s, then 0x197134(1, the floats +0x2c / +0x30 of 0x58988c). Exit: with the byte set, `0x25ad90(object path, "DCSData/models/7_9_A/")` (a case-insensitive string compare, non-zero when different) decides a 0x252068 with 0.5 s, so every cutscene except `7_9_A` fades in after its end; then the byte is cleared.

**Oracle (round 27, agent A; trace file not kept).** `hold A`, `frames 3`, `hold -`, `frames 2`, then `freetrap 75` at 0x256a14, 0x25d26c, 0x22ce9c and 0x2fac7c (seconds are wall clock including the stops): 3.78 s 0x256a14 kind 0x1166bc from FadeScreen (lr 0x1a2ee8, r1 0); 7.96 s from 0x26fa18 (lr 0x26faa0: the flag-a-clear path of `DCamCutScenePlay "1_2_c" 0 1 1 1`); 8.27 s 0x25d26c mode 4 (lr 0x1966f0); 8.97 s kind 0x1166bc, r1 1, r2 0 through the virtual call at 0x1098bc (the enter 0x2fadb8, the only `b 0x252068` with r1 = 1); 28.83 s from the update (lr 0x2faf38); 29.76 s exit 0x2fac7c (lr 0x10967c); 30.03 s the exit's fade in (lr 0x2face8); 30.51 s SpawnScript `1-2_MissionEnd` (lr 0x1c3748); 50.74 s and 51.09 s `1-3_MissionStart` and `1-3_Animations`.

**Port.** `src/runtime/screen_fade.{hpp,cpp}` (`ScreenFade`) holds the fade; `ScriptRunner` advances it every update, FadeScreen starts it and blocks while it runs, DCamCutScenePlay fades to black (0.5 s with flag a, else 0 s) when the cutscene plays, and `CutsceneNativeState::takeModeEntered` starts the 0.5 s fade in at the end of the flag-a delay. `fadeLevel()` is the value. The veil of the cutscene end (MOVIE-S20) is unchanged; the fade before the start is now the screen fade, not the veil. Test `natives-cutscene` (synthetic) and `game-cage-release-1-2` (chase-game from the cage).

Unknowns:
- Byte +0x10 of the game mode records (0x2369c4) in the game modes other than the one seen here; the mode switch of 0x25d26c and when the enter runs relative to the fade before it; what 0x2d19cc tests at the end and what 0x197134 does with the two floats.
- The draw function 0x24bda4 behind kind 0x1166bc (how the value becomes the picture, and on which screen: the end fade measured in §2026-10-14T23:52:14 did not darken the bottom screen); r1 of the start (+0x118).
- Other names whose static-table handler differs from the registered one were not checked.

<a id="ts-20261019-100429"></a>

## 2026-10-19T10:04:29+02:00 — The fade draw 0x24bda4, its screen gate 0x1087ec and the end fades of the cutscene mode (llvm-objdump with `--mattr=+vfp3,+neon` on `artifacts/game/game.arm.elf`; Azahar GDB `ctrace`/`memtrace`/`read`, OpenGL, port 24785, save state at the end of the cutscene 1_1_b and `gameplay-start`, no RPC; REA was not used)

**The draw.** The kind 0x1166bc is a thunk (`b 0x24bda4`); the frame code 0x1087ec calls the kind at +0x114 with s0 = 0x252880(+0x11c) and s1..s4 = the colour at +0x12c (black, alpha 1) when byte +0x118 (the screen byte r1 of the start) and the render state allow it. 0x24bda4 returns at once for value 1.0. Otherwise it sets a quad pipeline (0x25a55c with (-1, 1, -1, 1); the scissor rectangle of the current screen from the table at 0x529e30 + 0x34 + 0x18 * [+0x3c8]; a render state record built by 0x254350 and applied by 0x25511c, bytes [8] = 4, [9] = 5, [0x13] = (1 - value) * 255 for value <= 1 and (value - 1) * 255 above) and draws the rectangle (0, 0) to (w, h) = the floats at 0x529e30 + 0x10c / 0x110 (240.0 at +0x110 in gameplay) through 0x129bdc with the colour packed from s1..s4 * 255. The measured linear brightness fall (30 vblanks for 0.5 s) fits an opacity of |1 - value|; the blend itself (what the bytes 4 and 5 and [0x13] select) is not read.

**The screen gate 0x1087ec** (called from the frame end 0x25a700 at 0x25a7f0 once per screen pass): byte +0x118 = 0 draws on every pass; the render mode byte +0x3c4 (set by 0x1f0598: 0 top only, 1 bottom only, 2 both, with +0x3c8 = the screen index 0 top / 1 bottom, flipped each pass in mode 2 at 0x25ac14) selects the rest: in mode 2, byte 1 draws on screen 0 (top) and byte 2 on screen 1 (bottom); in mode 1, byte 1 draws and byte 2 does not. Oracle `read` at `gameplay-start`: +0x3c4 = 2, +0x118 = 0 after the last fade there, +0x110 = 240.0.

**Who passes which byte.** FadeScreen 0x1a2e8c passes the signed byte at 0x51a224 + 153 != 0 (0x1d9584; `read 0x51a2b8` at `gameplay-start` gives 1 at 0x51a2bd; the writer was not found), so FadeScreen fades the top screen only. DCamCutScenePlay 0x26fa18, the cutscene mode update 0x2fae68 and the exit 0x2fac7c all start with r1 = 1 (top only; `ctrace` below). The mode switch fades 0x10f628 (fade in, s0 = t) and 0x10f6e8 (fade out, then a wait loop) pass r1 = 0 (both screens) but only when the mode info byte +0x23 (out) or +0x24 (in) is set; 0x26fa18 clears both bytes of 0x5163e8 and the exit 0x2fac7c sets both.

**The end of a cutscene.** The update 0x2fae68 asks 0x2d19cc(object) first; when it returns 0 and byte 0x376d18 is set it starts 0x2520e8 (0.5 s, r1 = 1) and calls 0x197134(1, a, d), which queues the pop of the mode through 0x25ca0c (kind 1, time = 0.5 s when flag d is set). The mode switch (0x1096c0 .. 0x1097b0) starts a generic fade only when the info byte +0x23 is set, then loops `while 0x25207c() { 0x257620(...) }` (a sleep, 0x257620 converts seconds to milliseconds for 0x259484), sleeps once more, waits until the render counter at 0x529e30 + 0x3d4 has advanced by 2 (the scheduler frame does not advance meanwhile: the `ctrace` stops of the update fade, the exit and the exit fade all carry the same frame number), and calls the exit 0x2fac7c, which sets the bytes +0x23 / +0x24 and fades in 0.5 s (r1 = 1) unless the path is `7_9_A`. So the fade out ends before the exit, the black between them is the sleep plus two render ticks (measured 4 vblanks: two ticks of 1/30 s), and the fade in is 0.5 s long by the code. Oracle `ctrace 0x256a14,0x2fac7c,0x25d26c` from the save state at the end of the cutscene 1_1_b (trace output not kept): three stops in 170 updates: 0x256a14 from lr 0x2faf38 (r1 = 1, kind 0x1166bc), 0x2fac7c from lr 0x10967c, 0x256a14 from lr 0x2face8 (r1 = 1); no other fade starts and no further mode push.

**Port.** `ScreenFade` keeps the screen byte and gives `opacityOn(screen)` (|1 - value|, the screen gate for both screens rendered); `game_screen_fade::draw` paints the black quads; FadeScreen and every fade of the cutscene mode pass the top screen. `CutsceneNativeState` now runs the end from the code: at its length the cutscene asks for the 0.5 s fade out (the picture stays the cutscene, held on its last time), waits until the screen fade has ended, waits the two render ticks (2 / 30 s, stand-in MOVIE-S34), ends and asks for the 0.5 s fade in unless the name is `7_9_a`. The measured veil (MOVIE-S20) is gone. Tests: `screen-fade`, `natives-cutscene`, `natives-cutscene-real-data`.

Unknowns:
- The writer of the byte at 0x51a2bd (FadeScreen's screen byte); the blend of the quad (render state 0x254350 / 0x25511c); the colour of kind 0x1198d8 (white).
- What the byte +0x10 of the mode object (0x2369c4, tested by 0x26fa18) holds in each situation, and the frames between a mode push and its enter (the pop path was read, the push path shares the wait loop but was not traced).
- Why the oracle measured a fade in of about 43 vblanks (§2026-10-14T23:52:14) while the code fades in over 0.5 s (30 vblanks): the game clock may be capped (dt maximum 0.07 s) while the shots slowed the guest; not isolated.
- What 0x2d19cc tests for the end of a cutscene object (the port still ends at max(camera frames / 30, soundtrack)).

<a id="ts-20261027-183805"></a>

## 2026-10-27T18:38:05+01:00 — The key evaluator 0x252a7c and the lens of the cutscene camera (llvm-objdump `--triple=armv6k-none-eabi --mattr=+vfp2` on `artifacts/game/game.arm.elf`; data probes with `chase_assets`; agent 20261027-graphics-F; REA was not used)

[Corrects §2026-10-13T02:30](#ts-20261013-0230) (the curve shape was a data fit solved for time by bisection, and the camera field 0xc251f0c2 was taken as a field of view in degrees with fields 7 to 11 unknown).

**Track sampler 0x257458(track, t).** One key: its value. Before the first key time or from the last key time on, the extrapolation mode of the track's byte 0 (bits 0-2 before, bits 3-5 after; mode 0 is the first or last value; the other modes repeat or mirror the keys and are not ported, cutscene times stay inside 0..1). Inside the keys it calls the evaluator 0x252a7c. The camera handler's event data is filled by 0x259774: per track the sampled value at +0xc and the track's name word at +0x8c, which is what 0x1c1fbc reads.

**Key evaluator 0x252a7c(track, t).** B is the first key with a time greater than t, A the one before. When A's out delta is 0 and A's out value equals A's value, A's value holds until B (a hard cut: the camera of `1_1_b` holds and then jumps at camera frames 117, 175 and 268 this way). A key time equal to t gives that key's value. Otherwise u = (t - tA) / (tB - tA), linear in time, and the value is A (1-u)^3 + wA · A.out · (1-u)^2 u + wB · B.in · (1-u) u^2 + B u^3, with wA = (tB - tA) / A's out delta and wB = (tB - tA) / B's in delta, each clamped to 100 above and 0.01 below (a compare of the float bits, so a negative or zero delta gives 0.01 or 100). With one-third handles (wA = wB = 3) this is the plain cubic Bezier the port had; it differs for other handles. The port solved the curve for time and interpolated the cut keys; over all 69 cutscenes 916 of 3405 tracks (405 camera tracks) differ by more than 0.001 somewhere.

**The camera record.** 0x1c1fbc stores the tracks into the camera entity's record (entity +0x7c): position +0x00, rotation +0x0c, 0xc251f0c2 +0x18 (default 35), 0x45218801 +0x1c (1.41732), 0x00b68d33 +0x20 (0.945), 0x89afeba4 +0x24 (no default), 0x041a48a8 +0x28 (8.0), 0x7d690130 +0x2c (0.035), sets byte +0x30 = 1 and builds the node matrix (0x20e850, 0x241540).

**Who uses it.** The cutscene start 0x2d1800 starts the camera clip on the camera entity through 0x2a79d4, which pushes a director camera command with the routine 0x2663c4 (context +0x24 the entity, +0x1c the clip, +0x5c its handle, clip speed 1.0). 0x2663c4 sets pose flag 0x40000000, takes eye and axes from the entity's node matrix (0x1dd308, 0x248618), and writes pose +0x30 = 0x2cd650(entity) × [[0x3768c0] + 0x1c] (the factor the director camera applies to its 43 too; the port's director camera takes it as 1), pose +0x38 = record +0x28 (0x2a04fc) and pose +0x34 = record +0x2c (0x2c8b7c); before the entity is read, the FOV is 43 or the hook at 0x3768dc. It returns done when the clip slot state (0x245f84) is 6.

**The pose FOV 0x2cd650.** With byte +0x30 set: 2 · atan(record +0x1c · 25.4 · 0.5 / record +0x18) · 180 / π, times 4 / (3 · w / h), where w and h are the u32 at 0x529dc8 +4 and +8 converted to float. So +0x18 is the focal length in millimetres and +0x1c the horizontal film aperture in inches (1.41732 in = 36 mm; +0x20, 0.945 in = 24 mm, is the vertical aperture, read by nothing found). The camera manager 0x23f174 projects pose +0x30 like every pose (0x23458c: y scale cot(FOV / 2) · 4/3, x scale / (5/3)).

**MOVIE-33.** The lens tracks of `1_1_d` (focal length 55.0210 and aperture 1.6630, each two keys of the same value with zero handle values and deltas 0 / 1 / -1) evaluate to (1-u)^3 + u^3 of their value, but their ratio and so the FOV stay the same: 42.0 degrees across, 33.60 degrees of pose FOV, a vertical field of 25.52 degrees over the whole cutscene. The port took the focal length as a vertical field in degrees and its bisection gave 55.0 at the ends, 13.9 at frame 30, 33.7 at frame 120 and about 38 at frame 137: so at frames 120 and 137 it showed a wider shot than the original, as reported. The frame count was not the cause (the camera clip runs at its header rate, 30 per second). The port now evaluates every track with 0x252a7c and gives the cutscene camera the pose FOV of 0x2cd650 through the projection 0x23458c (`runtime::cutsceneVerticalFieldOfView`); `chase-viewer --cutscene` at frame 120 shows the close shot of the van front.

**Named FOV curve.** The cutscene object update 0x2d19cc also asks the camera clip for a named per-frame curve `yFieldOfView` (0x26557c: the entries at clip +0x18, 24 bytes, name at +0xc, value of key int(frame) by 0x1886cc) and, when it is greater than 0, sets the camera node's projection with 0x1ce568 (y scale cot(value), x scale divided by the screen w / h of 0x529dc8). No cutscene camera clip has such an entry (their only object is `ExportCam`), so this path does nothing in the shipped data. 0x2d19cc returns 0 when the camera clip slot state (0x245f84) is 6, else 1: that is what ends the cutscene object (MOVIE-36); state 6 itself was not read.

Unknowns:
- The value of w / h at 0x529dc8 +4 / +8 (the port takes 400 / 240, stand-in MOVIE-S36) and of the factor at [[0x3768c0] + 0x1c] during a cutscene.
- What reads pose +0x34 and +0x38 (record +0x2c and +0x28; the director camera writes 0.035 and 8.0 there), what record +0x24 is, and when clip state 6 is reached.

<a id="ts-20261027-185042"></a>

## 2026-10-27T18:50:42+01:00 — Oracle check of the cutscene camera (Azahar OpenGL, RPC request F-1 run by agent 20261027-graphics-servant for 20261027-graphics-F; recorded from a save state just after the cutscene 1_1_d loaded, New 3DS, and from a save state late in the cutscene 1_1_b, Old 3DS)

Checks [§2026-10-27T18:38:05](#ts-20261027-183805).

- `1_1_d`, stepped memtrace with dt 1/30 from camera frame 22.09 to the end (update 175, camera frame about 197; the frame at O+8 advances by 1.00 per update): pose FOV C+0x7c = 33.5993 in every update, the camera node's projection scales N+0xbc / +0xc0 = 2.64979 / 4.41631 (ratio 5/3), C+0x80 = 0.035 and C+0x84 = 13.32 (the record's +0x2c and +0x28 of `1_1_d`), current camera 0x3768e8 throughout (words 0x17e544, 0x17e478, 0x196784). The port's rule gives 33.5993 and the y scale cot(16.80 degrees) · 4/3 = 4.4163, so the screen size of 0x2cd650 gives the aspect 5/3 and the director camera's factor is 1.
- Eye C+0x70 and the node's world matrix row 2 (N+0x5c) at camera frames 22.09, 30.09, 100.09, 120.09, 137.09, 180.09 and 196.09 equal the port's cameraMatrix of the tracks evaluated with 0x252a7c to 4 decimals (for example frame 137.09: eye (827.3733, 20.3415, 1043.3774), row 2 (-0.5608, -0.1798, 0.8082)). C+0x4c holds a small vector (frame 22: (-0.0533, 0.2475, -0.0136)), not a look point, as the pose flag 0x40000000 of 0x2663c4 leads one to expect.
- `1_1_b` from camera frame 298 to its end at about 391 (94 updates): pose FOV 29.499 and scales 3.0387 / 5.0645 constant (lens 60 mm, 1.575 in: the rule gives 29.4994); eye (862.842, 29.976, 1082.018) at frame 298 and (864.297, 29.348, 1083.876) at the end, as the port evaluates.
- Pictures of `1_1_d` from a free run at camera frames 0, about 28, 99, 120, 137 and 179 (not kept): the close shot of the van front stays from 99 to 179.
- After the cutscene the follow camera is back: pose FOV 40.0, scales 2.198 / 3.663.
- Pinned in `cutscene-camera` (FOV and y scale of both cutscenes) and `assets-real-data` (eye and row 2).

Unknowns:
- The source of the 5/3 aspect (the words at 0x529dc8 +4 / +8 were not read) and of the factor 1, so MOVIE-S36 stays a measured stand-in; frames 0 to 297 of `1_1_b` and its cuts at 117, 175 and 268 were not reached (no save state at its start).
