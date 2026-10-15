# Audio

## 1. Scope

Sound effects and music: the CWAV decode and mixer, sound ids (`tablessoundfx`), the play routine 0x23fa54 with positional rules, level music, and the game hooks (footsteps, jump, landing, studs). Not covered: movie sound ([movies-cutscenes.md](movies-cutscenes.md)); doors play their sounds through this layer ([doors.md](doors.md)).

## 2. Status

IN_PROGRESS, updated 2026-10-15T16:35:00+02:00. `chase_audio` plays the player's and the studs' sounds in `chase-game`; music runs through the original's block (`MusicBlock`: start, queue, ramp, mood route) from the script natives and the level start.

## 3. Verified behaviour

- CWAV DSP-ADPCM decode, mixer with loops (`chase-soundtest`); 757/757 sound effect names resolve to `sounds/<name>.bwav`; music sheet 1 (39 tracks).
- Jump, footsteps, landings and studs play the original ids with the original's distance and pan rules: `audio-real-data` (8 recorded scenarios). Footstep trigger: track hash 0x68cc6917 sampled as a step function, event 50, slot 5.
- Effect sounds can be stopped by (sound id, owner): the voice that id started for that owner ends at once when the table row's fade (column 16) is 0, as for every door sound; `audio-core`. Details: [audio.md §2026-10-13T21:51](../../re/audio.md#ts-20261013-215117).
- Music block 0x376b30: ramp `step * 30 * dt` per update, clamp to the target, stop at 0 and start of the queued track, crossfade queue, mood route; ramp rows replayed from an oracle memtrace: `music-block`. Details: [script-natives-audio.md §2026-10-15T16:33](../../re/script-natives-audio.md#ts-20261015-163319).
- Level start calls the mood route (fade 1.0, mood 0, loop, crossfade): lvl_01 starts no music (ambient entry 255, nothing playing).
- Level music rule: lvl_00 starts none; lvl_01 (set 2 = (255, 5)) has ambient none and action `act_fight_music_1`.

## 4. Stand-ins

- AUDIO-S1 Water and object-11 landing flags are fixed to off.
- AUDIO-S2 Sprint phase transfer: floor of the proportional frame (9 of 10 recorded run footsteps exact, one off by one update).
- AUDIO-S3 No blend weights (a new animation slot cannot fire in its first update).
- AUDIO-S4 Distance volume is linear only as read from code (not measured end to end); Doppler assumed 1.0.
- AUDIO-S5 The 24-voice limit is not enforced.
- AUDIO-S6 Mix matrix channel order: left first assumed.
- AUDIO-S20 The music block assumes: lock hword 0x376b36 = 0 (no writer found), update rate 30 (0x23ca4c's rate object unread), block byte +0 starts 0, the byte +0x185 gate of the level-start call is 0, no duck record, no reopen of a finished looping stream, the loop flag only reaches the stream open (the mixer loops by the file's own loop flag). needs: AUDIO-20. blocks: SCRIPT-33.
- AUDIO-S11 AutomaticMusic clears the override byte 0x376b31 and runs the evaluation 0x204fe0 only as far as the mood-0 exit (the port has no combat, so the mood never changes). needs: AUDIO-11.
- AUDIO-S7 A stop of a sound with a fade time (column 16 above 0, 136 rows, none of them a door sound) fades the voice out linearly over that many seconds; the original's ramp is not read.

## 5. Open items

- [ ] AUDIO-1 — Sprint footstep phase: how the original maps the run phase to the sprint animation; run-to-sprint phase transfer. evidence: [audio.md §2026-10-13T17:54](../../re/audio.md#ts-20261013-175424).
- [ ] AUDIO-11 — Action music: the combat scan of 0x204fe0 (table 0x58deb0), OverrideMusicMood, PauseMusic, DuckMusicFor, VehicleEnableDrivingMusic and the level-start mood route are not wired. needs: SCRIPT-42. blocks: AUDIO-S11, AUDIO-20. evidence: [script-natives-audio.md §2026-10-15T15:10](../../re/script-natives-audio.md#ts-20261015-151038)
- [ ] AUDIO-20 — Duck record 0x5862d0 in the updater (flag, countdown, step 0.05), the reopen branch of a finished looping stream, 0x23ca4c's rate object, the lock writers and the block +0 start value are not read. needs: AUDIO-11. blocks: AUDIO-S20. evidence: [script-natives-audio.md §2026-10-15T16:33](../../re/script-natives-audio.md#ts-20261015-163319)
- [ ] AUDIO-21 — Of the ten sound natives of SCRIPT-33 the effect play and stop family (PlaySound, PlaySoundGO, PlaySoundAtObject, StopSound, StopSoundGO) can use the existing play 0x23fa54 and stop 0x23ae50 (handlers unread); registersound, ScaleSoundFrequency and SetSoundVolume need per-voice handles; vehiclestopsounds and VehicleEnableDrivingMusic need the vehicle music (AUDIO-11). Nothing implemented. blocks: SCRIPT-33. evidence: [script-natives-audio.md §2026-10-15T16:33](../../re/script-natives-audio.md#ts-20261015-163319)
- [ ] AUDIO-3 — Idle and jump animation sound tracks; world sounds of NPCs, vehicles and objects (ambient emitter 0x2c05dc plays ids like 210/296 from level objects). evidence: [audio.md §2026-10-13T17:54](../../re/audio.md#ts-20261013-175424).
- [ ] AUDIO-5 — Stud collect sound: silver (id 64) is replayed from a recorded pickup (`audio-real-data`); blue and purple (id 63) have no recorded pickup, and gold (id 64) none either. see STUD-3. evidence: [audio.md §2026-10-13T17:54](../../re/audio.md#ts-20261013-175424).
- [ ] AUDIO-6 — Which condition sets ctrl+0x317 bit 3 and ground type 11; the options volume source in a loaded save (0x2c19a4 sets defaults 8 and 10); mode 3 id remap of 0x23fa54; ducking (record flag bit 3). evidence: [audio.md §2026-10-13T17:54](../../re/audio.md#ts-20261013-175424).
- [ ] AUDIO-7 — Music, and the sounds of other people, animals, vehicles and objects, are not played (the character and the coins are).

## 6. Findings summary

- Sound container and mixer: [audio.md §2026-10-12T15:45](../../re/audio.md#ts-20261012-1545), [audio.md §2026-10-12T18:25](../../re/audio.md#ts-20261012-1825).
- Play entry 0x23fa54(id, pos, ...), ids from `tables/tablessoundfx.bxls`: [audio.md §2026-10-13T09:33](../../re/audio.md#ts-20261013-093338), [audio.md §2026-10-13T09:39](../../re/audio.md#ts-20261013-093936).
- Music routing and level music rule: [audio.md §2026-10-13T09:42](../../re/audio.md#ts-20261013-094203), [audio.md §2026-10-13T17:54](../../re/audio.md#ts-20261013-175424).
- Positional gains (0x107fa0, 0x25c388) and footsteps: [audio.md §2026-10-13T17:54](../../re/audio.md#ts-20261013-175424).
- Stop of effect sounds 0x23ae50 (owner key, fade from the table): [audio.md §2026-10-13T21:51](../../re/audio.md#ts-20261013-215117).

## 7. Code and tests

- `src/audio/` (`chase_audio`: mixer, soundplayer, effectvoices, musicplayer, spatial), `src/runtime/music.*`, `src/runtime/music_block.*`, `src/game_audio.hpp`, `chase-soundtest` (`--id`).
- CTest: `music-block`, `natives-music`, `audio-mixer`, `audio-core`, `audio-real-data`, `soundtest-real-smoke`, `soundtest-id-smoke`. Trace dir `CHASE_AUDIO_TRACE_DIR`. Scenarios `tools/oracle/audio-events*.txt`; `tools/export_sound_trace.py`.
