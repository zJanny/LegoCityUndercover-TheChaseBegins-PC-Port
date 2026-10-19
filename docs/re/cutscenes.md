# In-engine cutscenes

Reverse-engineering findings, moved verbatim from the progress log. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis.

## Current state

- Track records are 0x18 bytes (u16 key count at +4); keys are cubic Bezier curves in normalised time; 1954 animations, 62293 keys decode. [§2026-10-13T02:30](#ts-20261013-0230)
- Cutscene folders `csdata/models/<id>` (69) list their cast in `manifest.bstxt`; the camera handler 0x1c1fbc reads position, rotation and fov (default 35). [§2026-10-13T02:30](#ts-20261013-0230)
- The camera matrix (0x20e850) negates x; the world is left-handed, so the row flip alone mirrors the signs. [Corrected by levels.md §2026-10-13T12:49](levels.md#ts-20261013-1249)
- Joint orientation: after sampling, the skeleton record's +0x88 quaternion is multiplied in (Hamilton product, 0x2485d8). [§2026-10-13T02:30](#ts-20261013-0230)
- Cutscenes play at 30 frames per second (data evidence); soundtrack `music/csaudio/<id>.bwav` exists for 57 of 69. [§2026-10-13T03:20](#ts-20261013-0320)
- Natives work on 8 slots of 0x24 bytes at 0x58976c: Load takes a free slot and builds the object, Play starts the cutscene mode without blocking, Wait returns `!busy(name)`, Free clears the object. [§2026-10-14T22:49](#ts-20261014-224929)
- One screen fade (block 0x529e30) serves FadeScreen and the cutscene mode; the DCamCutScenePlay handler 0x26fa18 fades to black (0.5 s with flag a, else at once), the mode enter 0x2fadb8 fades in 0.5 s, its update fades out 0.5 s at the end and the exit fades in 0.5 s (not after `7_9_A`). [§2026-10-19T09:12:06](#ts-20261019-091206) The end fade measured with `1_1_b` (0.5 s out, 4 vblanks black, 0.7 s in) is what the port still uses. [§2026-10-14T23:52](#ts-20261014-235214)
- The level start plays `01_GameIntro` and then `1_1_a` through the `startup` script. [§2026-10-14T23:52](#ts-20261014-235214)
- Open: Load's retry field `args[0]+0x44`, flags b, c, d, the fade draw function 0x24bda4 and the mode switch timing, 113 actor animations with a frame count differing from the camera, camera fields 7-11.

<a id="ts-20261013-0230"></a>

## 2026-10-13T02:30+02:00 — Cutscene tracks, cameras and joint orientations (REA, Ghidra 12.1.2)

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
- State `cutscene-1-1-b` (see [oracle-states.md](oracle-states.md)).

Unknowns:
- What `*(args[0] + 0x44)` of Load is, the meaning of Play's flags a, b, c, d (a = fade out before the start fits the oracle), what ends the cutscene object (the port ends at max(camera frames / 30, soundtrack)), and the cutscene fades at the end.
- Whether Free of a playing cutscene ends it, and what Play of a cutscene that was not loaded does.
- The oracle length of `1_1_b` was not measured (the saved state is near its end).

<a id="ts-20261014-235214"></a>

## 2026-10-14T23:52:14+02:00 — Cutscene fades measured in the oracle, the level start plays the intro (llvm-objdump on `artifacts/game/game.arm.elf`; Azahar OpenGL, RPC, port 24711, states `cutscene-1-1-b` and `gameplay-start`; REA was not used)

Corrected by [cutscenes.md §2026-10-19T09:12:06](#ts-20261019-091206): 0x25ad90 is a string compare that is non-zero when the strings differ, so 0x2fac7c fades in after every cutscene except `7_9_A`; the fades of the cutscene mode are read there.

- Cutscene end 0x2fac7c (the cutscene object's teardown): when byte 0x376d18 is set it compares the object's path with the string `DCSData/models/7_9_A/` (0x2fad98) and only for that cutscene starts a fade of 0.5 s (0x252068(colour 0x1166bc, 1, 0, s0 = 0.5), a wrapper of 0x256a14 with s1 = 0, s2 = 1); then it frees the object and clears +0x28 / +0x50. So the end fade of `1_1_b` does not come from this function; the generic fade belongs to the cutscene mode that 0x25d26c starts (a pushed event record in the table at 0x58613c, 12 bytes each, with the float a as its value) and was not traced.
- Oracle `endfade` (state `cutscene-1-1-b`, `shotloop` back to back, 274 shots, mean brightness of the top screen; `artifacts/oracle/agentS-out/endfade_*`, local): the top picture is clear until vblank 118, falls linearly to black at vblank 148 (30 vblanks, 0.5 s), stays black for about 4 vblanks, the active cutscene pointer (0x5898b4) becomes 0 at vblank 149, then the game picture (camera behind the player) fades in linearly over about 43 vblanks (0.7 s) up to vblank 194. The bottom screen does not fade; it changes to the map at vblanks 174 to 194.
- Oracle `startfade` (state `gameplay-start`, teleport onto the waypoint at vblank 20): the active pointer is set at vblank 57 and the top picture falls from 60.5 to 25.7 over the next 32 vblanks (0.5 s, linear) with the HUD still drawn at the start; then the cutscene picture appears. `1_1_b` is played with flags 1 1 1 1; `1_1_a` with 0 1 1 1, so flag a is the only difference and fits the fade before the start (a = 0 has none).
- Port: `CutsceneNativeState` (`src/runtime/natives_cutscene.*`) delays the start by 0.5 s when flag a is set (the script's Wait blocks through it, the picture stays the game picture), ramps the top-screen veil to black over the last 0.5 s of the cutscene, keeps it black 4 vblanks and fades in over 43 vblanks. The times are measured values (stand-in MOVIE-S20). The end fade applies to every cutscene because no flag is known to control it (b, c, d are 1 in both known calls).
- Level start: the `startup` run now yields as soon as it starts a movie or an in-engine cutscene, and the game loop continues it, so a plain `chase-game --level lvl_01` plays `01_GameIntro` (PlayCutscene, `1-1_IntroPlayed` guard in the script) and then `1_1_a`, then `Wait 1` and the first tip, as the script orders them. The front end path still plays the movie before the level and skips it in the level (MOVIE-S12). `--no-start-cutscenes` keeps the old preroll for development runs (the tests use it).

Unknowns:
- The generic fade code of the cutscene mode (the consumer of the 0x25d26c record), what flags b, c and d control, whether `1_1_a` fades at its end like `1_1_b`, and whether the fade-in after the end is part of the cutscene mode or of the camera return.
- The 0.7 s fade-in may include the camera blend back to the game camera.

<a id="ts-20261019-091206"></a>

## 2026-10-19T09:12:06+02:00 — One screen fade for FadeScreen and the cutscene mode, the DCamCutScenePlay handler 0x26fa18 (llvm-objdump on `artifacts/game/game.arm.elf`; Azahar GDB `freetrap`, OpenGL, port 24771, state `m12-13-cage-trigger`, no RPC; REA was not used)

[Corrects §2026-10-14T22:49:29](#ts-20261014-224929) (the handler bound to DCamCutScenePlay), [Corrects §2026-10-14T23:52:14](#ts-20261014-235214) (the fade of the cutscene end 0x2fac7c) and [Corrects script-natives.md §2026-10-14T13:33:39](script-natives.md#ts-20261014-133339) (the two handler sources agree).

**The screen fade (block 0x529e30).** The start 0x256a14(kind r0, byte r1, colour r2, s0 = seconds, s1 = start, s2 = rate) stores the kind (a draw function: 0x1166bc for the black fade, 0x1198d8, and 0x1166c0 = no fade, set by the init at 0x105c98) at +0x114, r1 at +0x118, the colour at +0x12c (r2's vector, else white for kind 0x1198d8 and black otherwise, alpha 1) and sets the timer at +0x11c: duration = seconds in clock ticks (0x114aa4), position = start x duration (0x1150e0), rate (0x1140a0). The position (0x1202f8) is (clock - start tick) x rate, clamped to [0, duration]; the value 0x252880 is position / duration, and 0 when the duration is 0. 0x2520e8 = start 1, rate -1 (to black); 0x252068 = start 0, rate +1 (from black). The test 0x25207c returns 0 for kind 0x1166c0, else 1 while the value is below 1 (rate >= 0) or above 0 (rate < 0). FadeScreen 0x1a2e8c starts 0x2520e8 for 0 and 0x252068 otherwise, kind 0x1166bc, s0 = 0.5, r1 = 0x1d9584() != 0, and returns `!0x25207c()`. Every start replaces the fade before; there is one fade for the whole game.

**DCamCutScenePlay is 0x26fa18.** The registration at 0x1625e8 (`bl 0x23912c`, name at 0x16290c, 5 arguments) binds `DCamCutScenePlay` to 0x26fa18; the static table entry at 0x359528 holds 0x2889e4, which 0x26fa18 tail-calls. Arguments are 8 bytes apart (argument 0 at +4, the next at +0xc; the same in 0x2889e4). 0x26fa18 sets byte 0x376d18 to 1, clears bytes +0x23 and +0x24 of the block 0x5163e8, and then: flag a set: 0x2520e8 with 0.5 s; flag a clear: when byte +0x10 of the current game mode (0x2369c4) is 1, byte 0x374600 = 1 and no fade, else 0x2520e8 with 0 s (black at once). Then 0x2889e4 → 0x1964dc, which pushes mode 4 (byte 0x376d18 set) through 0x25d26c.

**The cutscene mode (vtable 0x363134: +8 enter 0x2fadb8, +0x1c update 0x2fae68, +0x20 exit 0x2fac7c).** Enter: with byte 0x376d18 set, 0x252068 with 0.5 s (from black). Update: when 0x2d19cc(cutscene) returns 0 and the byte is set, 0x2520e8 with 0.5 s, then 0x197134(1, the floats +0x2c / +0x30 of 0x58988c). Exit: with the byte set, `0x25ad90(object path, "DCSData/models/7_9_A/")` (a case-insensitive string compare, non-zero when different) decides a 0x252068 with 0.5 s, so every cutscene except `7_9_A` fades in after its end; then the byte is cleared.

**Oracle (`artifacts/scratch/r27-A/cage_fade.txt` → `artifacts/oracle/r27A-out/cagefade.txt`).** `hold A`, `frames 3`, `hold -`, `frames 2`, then `freetrap 75` at 0x256a14, 0x25d26c, 0x22ce9c and 0x2fac7c (seconds are wall clock including the stops): 3.78 s 0x256a14 kind 0x1166bc from FadeScreen (lr 0x1a2ee8, r1 0); 7.96 s from 0x26fa18 (lr 0x26faa0: the flag-a-clear path of `DCamCutScenePlay "1_2_c" 0 1 1 1`); 8.27 s 0x25d26c mode 4 (lr 0x1966f0); 8.97 s kind 0x1166bc, r1 1, r2 0 through the virtual call at 0x1098bc (the enter 0x2fadb8, the only `b 0x252068` with r1 = 1); 28.83 s from the update (lr 0x2faf38); 29.76 s exit 0x2fac7c (lr 0x10967c); 30.03 s the exit's fade in (lr 0x2face8); 30.51 s SpawnScript `1-2_MissionEnd` (lr 0x1c3748); 50.74 s and 51.09 s `1-3_MissionStart` and `1-3_Animations`.

**Port.** `src/runtime/screen_fade.{hpp,cpp}` (`ScreenFade`) holds the fade; `ScriptRunner` advances it every update, FadeScreen starts it and blocks while it runs, DCamCutScenePlay fades to black (0.5 s with flag a, else 0 s) when the cutscene plays, and `CutsceneNativeState::takeModeEntered` starts the 0.5 s fade in at the end of the flag-a delay. `fadeLevel()` is the value. The veil of the cutscene end (MOVIE-S20) is unchanged; the fade before the start is now the screen fade, not the veil. Test `natives-cutscene` (synthetic) and `game-cage-release-1-2` (chase-game from the cage).

Unknowns:
- Byte +0x10 of the game mode records (0x2369c4) in the game modes other than the one seen here; the mode switch of 0x25d26c and when the enter runs relative to the fade before it; what 0x2d19cc tests at the end and what 0x197134 does with the two floats.
- The draw function 0x24bda4 behind kind 0x1166bc (how the value becomes the picture, and on which screen: the end fade measured in §2026-10-14T23:52:14 did not darken the bottom screen); r1 of the start (+0x118).
- Other names whose static-table handler differs from the registered one were not checked.
