# Audio

## 1. Scope

Sound effects and music: the CWAV decode and mixer, sound ids (`tablessoundfx`), the play routine 0x23fa54 with positional rules, level music, and the game hooks (footsteps, jump, landing, studs). Not covered: movie sound ([movies-cutscenes.md](movies-cutscenes.md)); doors play their sounds through this layer ([doors.md](doors.md)).

## 2. Status

IN_PROGRESS, updated 2026-10-21T22:53:57+02:00. The ten sound natives of batch 8 play, stop and set volume and rate of effect voices (`SoundNativeState`, stand-ins AUDIO-S30, S32); script objects and doors key their sounds by the entity id of the object loader (`SceneEntityIds`, AUDIO-S36). `chase_audio` plays the player's and the studs' sounds in `chase-game`; music runs through the original's block (`MusicBlock`: start, queue, ramp, mood route) from the script natives and the level start.

## 3. Verified behaviour

- CWAV DSP-ADPCM decode, mixer with loops (`chase-soundtest`); 757/757 sound effect names resolve to `sounds/<name>.bwav`; music sheet 1 (39 tracks).
- Jump, footsteps, landings and studs play the original ids with the original's distance and pan rules: `audio-real-data` (8 recorded scenarios). Footstep trigger: track hash 0x68cc6917 sampled as a step function, event 50, slot 5.
- Effect sounds can be stopped by (sound id, owner): the voice that id started for that owner ends at once when the table row's fade (column 16) is 0, as for every door sound; `audio-core`. Details: [audio.md §2026-10-13T21:51](../../re/audio.md#ts-20261013-215117).
- The owner of a script object's or a door's sounds is its entity id +0x14, the creation index of the object loader 0x1a4130 (root 0, each object before the objects of its class file, record byte +0xc bits 0 and 1); a play and a stop on one object agree, and a script uid equal to an id shares its key: `natives-sound`, `entity-ids-real-data`. The absolute ids are not verified against the oracle yet (AUDIO-S36, AUDIO-37). Details: [audio.md §2026-10-21T22:21:40](../../re/audio.md#ts-20261021-222140).
- Voice volume and rate: SetSoundVolume replaces the table volume (distance factor and master volume multiply it), ScaleSoundFrequency stores (int)(scale * source rate) and the pitch is rate * Doppler / source rate; the rate boost of record bit 3 does not occur in the sound table; formulas pinned in `voice-params` and `voice-params-real-data`. Details: [audio.md §2026-10-21T22:19:39](../../re/audio.md#ts-20261021-221939).
- Music block 0x376b30: ramp `step * 30 * dt` per update, clamp to the target, stop at 0 and start of the queued track, crossfade queue, mood route; ramp rows replayed from an oracle memtrace: `music-block`. Details: [script-natives-audio.md §2026-10-15T16:33](../../re/script-natives-audio.md#ts-20261015-163319).
- Level start calls the mood route (fade 1.0, mood 0, loop, crossfade): lvl_01 starts no music (ambient entry 255, nothing playing).
- Animation sound tracks of the player's clips (pulse record with sound id, character sound slot) play through `ClipSounds` (`ladder-sounds`, `ladder-sounds-real-data`); the ladder climb plays 112 and 288, not compared with the oracle yet (TRAV-61). Details: [player-traversal-ladders.md §2026-10-21T22:44:32](../../re/player-traversal-ladders.md#ts-20261021-224432).
- Level music rule: lvl_00 starts none; lvl_01 (set 2 = (255, 5)) has ambient none and action `act_fight_music_1`.

## 4. Stand-ins

- AUDIO-S1 Water and object-11 landing flags are fixed to off.
- AUDIO-S2 Sprint phase transfer: floor of the proportional frame (9 of 10 recorded run footsteps exact, one off by one update).
- AUDIO-S3 No blend weights (a new animation slot cannot fire in its first update).
- AUDIO-S4 Distance volume is linear only as read from code (not measured end to end).
- AUDIO-S5 The 24-voice limit is not enforced.
- AUDIO-S6 Mix matrix channel order: left first assumed.
- AUDIO-S20 The music block assumes: lock hword 0x376b36 = 0 (no writer found), update rate 30 (0x23ca4c's rate object unread), block byte +0 starts 0, the byte +0x185 gate of the level-start call is 0, no duck record, no reopen of a finished looping stream, the loop flag only reaches the stream open (the mixer loops by the file's own loop flag). needs: AUDIO-20. blocks: SCRIPT-33.
- AUDIO-S11 AutomaticMusic clears the override byte 0x376b31 and runs the evaluation 0x204fe0 only as far as the mood-0 exit (the port has no combat, so the mood never changes). needs: AUDIO-11.
- AUDIO-S7 A stop of a sound with a fade time (column 16 above 0, 136 rows, none of them a door sound) fades the voice out linearly over that many seconds; the original's ramp is not read.

- AUDIO-S30 A name that is no object of the loaded level file (Player1, an entity of lvl_the_city; entities made at run time) gets a consecutive sound owner from 0x4000 instead of an entity id. evidence: [audio.md §2026-10-21T22:21:40](../../re/audio.md#ts-20261021-222140)
- AUDIO-S36 Every object of the level file that the record flags allow gets an entity id: class creates that return no entity and the loader's filter are not modelled, and the loaded level is taken to be a world level. The oracle's recorded ids and counts disagree with the rule (AUDIO-37). needs: AUDIO-37. evidence: [audio.md §2026-10-21T22:21:40](../../re/audio.md#ts-20261021-222140)
- AUDIO-S34 Voice rate and volume assumptions: the byte 0x376ce6 that enables the rate boost of sounds with record bit 3 is off (`SoundPlayer::setRateBoostByte`; no row of tablessoundfx has the bit, the only code found clears the byte); sound object +0xc and +0x10 are both the decoded sound's rate; a positional voice with volume 0 or below plays silence, where the original keeps its pan. needs: AUDIO-36. evidence: [audio.md §2026-10-21T22:19:39](../../re/audio.md#ts-20261021-221939)
- AUDIO-S35 The Doppler factor of positional voices (formula read) uses emitter and listener velocity zero, so it is 1.0. needs: AUDIO-35. evidence: [audio.md §2026-10-21T22:19:39](../../re/audio.md#ts-20261021-221939)
- AUDIO-S32 registersound, VehicleEnableDrivingMusic and vehiclestopsounds only record their arguments (no preload, no vehicle music, no vehicle sound ids). needs: AUDIO-32.

## 5. Open items

- [ ] AUDIO-1 — Sprint footstep phase: how the original maps the run phase to the sprint animation; run-to-sprint phase transfer. evidence: [audio.md §2026-10-13T17:54](../../re/audio.md#ts-20261013-175424).
- [ ] AUDIO-11 — Action music: the combat scan of 0x204fe0 (table 0x58deb0), OverrideMusicMood, PauseMusic, DuckMusicFor, VehicleEnableDrivingMusic and the level-start mood route are not wired. needs: SCRIPT-42. blocks: AUDIO-S11, AUDIO-20, AUDIO-32. evidence: [script-natives-audio.md §2026-10-15T15:10](../../re/script-natives-audio.md#ts-20261015-151038)
- [ ] AUDIO-20 — Duck record 0x5862d0 in the updater (flag, countdown, step 0.05), the reopen branch of a finished looping stream, 0x23ca4c's rate object, the lock writers and the block +0 start value are not read. needs: AUDIO-11. blocks: AUDIO-S20. evidence: [script-natives-audio.md §2026-10-15T16:33](../../re/script-natives-audio.md#ts-20261015-163319)
- [ ] AUDIO-35 — Sources of the emitter velocity (emitter slot +0x14) and the listener velocity (0x52ac50) that the Doppler factor of the voice synchroniser reads. blocks: AUDIO-S35. evidence: [audio.md §2026-10-21T22:19:39](../../re/audio.md#ts-20261021-221939)
- [ ] AUDIO-36 — Unread around voice volume and rate: a code path that sets byte 0x376ce6, whether sound object +0xc and +0x10 (rate fields) ever differ, a positional voice with volume 0 or below (the original keeps its pan, the port is silent), the flat-path branch of 0x23f82c for record bit 1. blocks: AUDIO-S34. evidence: [audio.md §2026-10-21T22:19:39](../../re/audio.md#ts-20261021-221939)
- [ ] AUDIO-32 — Readers of the registered sound pairs (0x593a60), the vehicle sound ids at component +0x1c.., the callee 0x2b429c and the reader of vehicle byte +0x312 bit 2. needs: AUDIO-11. blocks: AUDIO-S32. evidence: [script-natives-audio.md §2026-10-15T17:27](../../re/script-natives-audio.md#ts-20261015-172705)
- [ ] AUDIO-33 — StopSoundGO on a kind-10 sound object (`lesoundobject`, `soundobject`) stops the object's own sound and sets a component bit; the port creates no kind-10 objects, and their create and update are not read. needs: AUDIO-3. evidence: [audio.md §2026-10-21T22:21:40](../../re/audio.md#ts-20261021-222140)
- [ ] AUDIO-37 — Oracle check of the entity ids: the recorded ids and object counts of the lvl_01 scene disagree with the loader rule; read +0x14 and the name hash of the first entities, and find which objects get no entity. blocks: AUDIO-S36. evidence: [audio.md §2026-10-21T22:21:40](../../re/audio.md#ts-20261021-222140)
- [ ] AUDIO-38 — The detective (DETECT-S4), build-it (LEVOBJ-S74) and scanner (AUDIO-S33) sounds still use owner keys of their own instead of the entity id of `SceneEntityIds`. evidence: [audio.md §2026-10-21T22:21:40](../../re/audio.md#ts-20261021-222140)
- [ ] AUDIO-34 — The attack's weapon-table sounds are all id 0 (silent); compare an audio recording of a punch in the oracle: the `combo_*` clips carry sound pulses 295 and 530 that `ClipSounds` now plays ([player-traversal-ladders.md §2026-10-21T22:44:32](../../re/player-traversal-ladders.md#ts-20261021-224432)). blocks: PLAYER-24. evidence: [player-attack.md §2026-10-19T12:54:52](../../re/player-attack.md#ts-20261019-125452).
- [ ] AUDIO-3 — World sounds of NPCs, vehicles and objects (ambient emitter 0x2c05dc plays ids like 210/296 from level objects); the sound tracks of the player's clips are played by `ClipSounds` ([player-traversal-ladders.md §2026-10-21T22:44:32](../../re/player-traversal-ladders.md#ts-20261021-224432), TRAV-61, TRAV-62). blocks: AUDIO-33, VEH-10. evidence: [audio.md §2026-10-13T17:54](../../re/audio.md#ts-20261013-175424).
- [ ] AUDIO-5 — Stud collect sound: silver (id 64) is replayed from a recorded pickup (`audio-real-data`); blue and purple (id 63) have no recorded pickup, and gold (id 64) none either. see STUD-3. evidence: [audio.md §2026-10-13T17:54](../../re/audio.md#ts-20261013-175424).
- [ ] AUDIO-6 — Which condition sets ctrl+0x317 bit 3 and ground type 11; the options volume source in a loaded save (0x2c19a4 sets defaults 8 and 10); mode 3 id remap of 0x23fa54; ducking (record flag bit 3). evidence: [audio.md §2026-10-13T17:54](../../re/audio.md#ts-20261013-175424).
- [ ] AUDIO-7 — Music, and the sounds of other people, animals, vehicles and objects, are not played (the character and the coins are).

## 6. Findings summary

- Sound container and mixer: [audio.md §2026-10-12T15:45](../../re/audio.md#ts-20261012-1545), [audio.md §2026-10-12T18:25](../../re/audio.md#ts-20261012-1825).
- Play entry 0x23fa54(id, pos, ...), ids from `tables/tablessoundfx.bxls`: [audio.md §2026-10-13T09:33](../../re/audio.md#ts-20261013-093338), [audio.md §2026-10-13T09:39](../../re/audio.md#ts-20261013-093936).
- Music routing and level music rule: [audio.md §2026-10-13T09:42](../../re/audio.md#ts-20261013-094203), [audio.md §2026-10-13T17:54](../../re/audio.md#ts-20261013-175424).
- Positional gains (0x107fa0, 0x25c388) and footsteps: [audio.md §2026-10-13T17:54](../../re/audio.md#ts-20261013-175424).
- The ten sound natives (play, stop, volume, frequency, registry): [script-natives-audio.md §2026-10-15T17:27](../../re/script-natives-audio.md#ts-20261015-172705).
- Voice volume and rate fields, their synchroniser 0x25c388 and the DSP path, the Doppler formula and the rate boost: [audio.md §2026-10-21T22:19:39](../../re/audio.md#ts-20261021-221939).
- Stop of effect sounds 0x23ae50 (owner key, fade from the table): [audio.md §2026-10-13T21:51](../../re/audio.md#ts-20261013-215117).
- The owner key is the entity id of the object loader 0x1a4130; the kind-10 sound object stop 0x22c58c: [audio.md §2026-10-21T22:21:40](../../re/audio.md#ts-20261021-222140).

## 7. Code and tests

- `src/audio/` (`chase_audio`: mixer, soundplayer, effectvoices, musicplayer, spatial, voice_params), `src/runtime/music.*`, `src/runtime/music_block.*`, `src/game_audio.hpp`, `chase-soundtest` (`--id`).
- `src/runtime/natives_sound.*` (the ten sound natives), `src/runtime/entity_ids.*` (entity ids, the owner key). CTest: `natives-sound`, `entity-ids-real-data`, `music-block`, `natives-music`, `audio-mixer`, `audio-core`, `audio-real-data`, `voice-params`, `voice-params-real-data`, `soundtest-real-smoke`, `soundtest-id-smoke`. The recorded sound calls are pinned in `tests/audio_real_test.cpp`. Scenarios `tools/oracle/audio-events*.txt`; `tools/export_sound_trace.py`.
