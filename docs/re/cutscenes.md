# In-engine cutscenes

Reverse-engineering findings, moved verbatim from the progress log. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis.

## 2026-10-13T02:30+02:00 — Cutscene tracks, cameras and joint orientations (REA, Ghidra 12.1.2)

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
  - With rows 0 and 2 flipped for GL, "POLICE" signs read unmirrored, and 1_1_a shows the police station, the player walking up and the donut counter.
  - This is inferred; the matrix's consumer 0x241540 was not traced.
- Unknown: 113 of 397 actor animations have a different frame count from their camera; the viewer holds their last frame. The meaning of camera fields 7–11 and of fov's axis is unknown, as is the base value the actor scale is added to.
- Rule 6 violation found and fixed (next entry): Python posing (convert_anims.skin_matrices) did not apply the joint orientation and so differed from C++.
- ctest PASS 8/8 (new viewer-cutscene-smoke); Python PASS.

## 2026-10-13T03:20+02:00 — Cutscene playback rate and soundtrack

- music/csaudio/<id>.bwav exists for 57 of the 69 cutscene folders (12 kHz).
- The audio length is camera frames / 30 + 1.0 s in most cases, for example 1_1_a (405 → 14.5 s), 1_1_b, 1_1_c, 1_1_d and 2_7_a. It is exactly frames / 30 for 1_4_a and 2_6_b.
- Outliers: 6_4_a (221 frames, 23.0 s), 2_11_a and 1_4_c.
- Conclusion: cutscenes play at 30 frames per second (data evidence; the game's timer code was not traced). This matches the viewer's default `--fps 30`.
- The viewer's `--cutscene` plays the soundtrack through chase_audio and an SDL audio device. The picture clock restarts with the sound, holds the last camera frame through the tail, and loops after max(frames / fps, sound length). No sound plays with `--frame` or `--screenshot`.
- ctest PASS 8/8.

