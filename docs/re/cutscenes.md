# In-engine cutscenes

Reverse-engineering findings, moved verbatim from the progress log. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis.

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
