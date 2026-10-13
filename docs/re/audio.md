# Sound container, CWAV and mixer

Reverse-engineering findings, moved verbatim from the progress log. Entries are chronological; later entries may correct earlier ones. Evidence IDs are shortened with an ellipsis.

## 2026-10-12T15:45+02:00 — Sound container decoded (.bwav)

- The .bwav wrapper starts with u32 version 0x400 or 0x600.
  - v4 (1263 files): one standard CWAV at 0x44.
  - v6 (98 files, all music): one mono CWAV per channel, back to back from 0x4c, with the second CWAV starting exactly where the first ends.
- The other wrapper fields (rate copy, sample/loop counts, a 4-byte value before CWAV that may be a hash) are not interpreted. The CWAV structure is the public BCWAV layout.
- Added tools/convert_audio.py (PCM8/PCM16/DSP-ADPCM/IMA-ADPCM to WAV, v6 parts merged as channels, loop points kept in audio-manifest.json) and tests/test_convert_audio.py (4 tests: DSP frame arithmetic including floor shift, PCM8/16, IMA, two-part merge + WAV output, malformed inputs).
- `python3 tools/convert_audio.py --census artifacts/fib/files` — 1361/1361 parsed; all DSP-ADPCM: 12000 Hz 1068 (+98 two-part), 11999 69, 12001 75; the rest are between 7025 and 12003 Hz.
- `python3 tools/convert_audio.py artifacts/fib/files --output artifacts/audio` — PASS, 1361 WAVs, 0 failures, 40.4 s.
- Checks:
  - The first DSP frame header equals the context pred/scale byte (0x39 in pickup_positive_1_notloop), which confirms data alignment at DATA+0x20.
  - ffmpeg astats: voice-over and music peak at about −0.2 to −2.4 dBFS with RMS between −10 and −23 dB, which reads as normalized audio rather than noise. Zero-crossing rate is 0.06 for the music sample.
  - music/act_01 decodes to silence because its frames are genuinely all zero, so it is a placeholder.
- `python3 -m unittest discover -s tests -p 'test_*.py' -v` — PASS 30/30.

## 2026-10-12T18:25+02:00 — S4a complete (CWAV in C++), S4d mixer and sound test

- Added src/assets/audio.cpp: decodeSound splits the .bwav through parsePooled (0x2c record plus CWAV parts) and decodes PCM8/PCM16/DSP-ADPCM/IMA. The DSP sum is computed in 64-bit and floor-shifted, as in the reference.
- assets-core gains a synthetic DSP vector ([112, −16, −16], matching the Python test) and a malformed-record rejection.
- assets-real-data: 1361 sounds / 122501647 samples / 123 looping, equal to audio-manifest.json. The FNV-1a over act_bass_break's interleaved PCM is 0xdcbf1f85, bit-identical to the Python WAV output.
- Added the chase_audio library (src/audio/mixer.{hpp,cpp}, no SDL): voices with linear resampling, mono→stereo, gain, loop wrap to loopStart, and one-shot expiry.
- Added CTest `audio-mixer` (tests/mixer_test.cpp).
- Added chase-soundtest (SDL audio callback at 48 kHz float stereo). It plays archive sounds and keeps loops going for 10 s by default or `--seconds`.
- Added the opt-in CTest `soundtest-real-smoke` (SDL_AUDIODRIVER=dummy).
- `ctest --test-dir build --output-on-failure` — PASS 7/7; 0 warnings.

## 2026-10-13T09:33:38+02:00 — S4d play-sound entry point (REA Ghidra 12.1.2; llvm-objdump; read-only, partial)

- Binary: artifacts/game/game.arm.elf (sha256 5152ed14…0881). Script command addresses come from local scan of the `bl 0x23912c` registration calls (name via `add r0,pc`, handler via `ldr r1,[pc]`); local evidence, not REA.
- Sound thread: init at 0x103400 creates the thread named `fnaSound` (string 0x1034a8, xref 0x103444, ev_dd91ebb5…), stack 0x1000, entry 0x108ae0, then an object named `sound`. Thread body not read.
- Script commands (registered via 0x23912c; handler = 2nd argument; the args array is `param_2`, with value pointers at +0x0, +0x4, +0xc):
  - `RegisterSound` (name 0x160bc4, handler 0x1cdbdc, ev_efaa1c6c…): `FUN_00281fc4(*(u32*)arg[1] & 0xffff, (int)*(float*)arg[3] & 0xff)`. Appends a (u16 sound id, u8 value) pair to a dedup list at 0x593a60, count at 0x377598. It does not start a voice; it reads as a preload or usage list.
  - `PlaySoundAtObject` (name 0x161260, registered at 0x160ecc, handler 0x27bd48, disassembly only): calls 0x23ae84(sound id = `**(arg[3])`, object = `*arg[0]`).
  - Other handlers found, not read: `PlayCharacterSound` 0x285f94, `SetSoundVolumeGO` 0x1da758, `ScaleSoundFrequencyGO` 0x2915f4, `PauseMusic` 0x1a3020, `StopMusic` 0x195054, `OverrideMusic` 0x1cdb28, `OverrideMusicMood` 0x27bcb0, `AutomaticMusic` 0x1d9c70, `VehicleEnableDrivingMusic` 0x2bf2b4, `VehicleStopSounds` 0x27c148.
- 0x23ae84 (ev_aa513b99…): `FUN_0023fb78(obj, vec12)` gets the object position, then calls the core below with the sound id, that position, the u16 at obj+0x14 and 0.
- Core play function 0x23fa54(id, pos*, u16 param, flag) (ev_90aa6512…). The id is a plain integer, not a name or hash.
  - If `DAT_00376ce5 == 3` and `DAT_00376cf4 != 0`, the id is first remapped through a table of 0x10-byte entries: `id = u16 at (DAT_00376cf4 + id*0x10 + 4)`. Meaning of mode 3 unknown.
  - It walks a linked list of sound banks starting at `DAT_0058974c`. A bank (node+8 points to the bank struct) has: record array at +0x10 with 0x10 bytes per sound id, highest valid id (u16) at +0x20, a bit set of valid ids at +0x24. The first bank whose bit is set plays the sound.
  - `pos == 0` (non-positional) calls 0x23f82c(volume = rec[9]*0.01, bank, id, param, 1). `pos != 0` calls 0x23f940(rec[10], rec[11], bank, id, param, pos).
- Voice start 0x23f82c and 0x23f940 (ev_d1036049…, ev_5a119b87…): allocate a voice with 0x1b4420(bank, id) (handle at voice+8, not read), then use 0x227ff4 (set volume), 0x25c604 (set loop or similar from rec[6] bit 0), 0x227fac (pos/space), 0x227f78 (rec[10], rec[11] as two parameters, likely attenuation range; unverified), 0x227f4c (ducking value from 0x23fc98, when rec[6] bit 3 and `DAT_00376ce6`).
  - Record bytes used: +6 flags (bit0 passed to 0x25c604; bit1 gates the position setup in the non-positional path; bit3 duck; bit4 sets a global state at 0x5862d0..0x5862e4), +9 volume percent, +10 and +11 two byte parameters.
- Shipped table: `tables/tablessoundfx.bxls` (build/chase-tabledump: 758 rows, sheet 0). Each row has a path-like name without extension (for example `ui/menuselect`, `lego_sfx/red_button`), then trailing integers. For the rows checked (1, 5, 9, 21) the last three integers equal record bytes +9/+10/+11 (80/20/30, 100/1/100). That fits the volume percent in the table being the record's volume, and the row index being the sound id used by scripts. Row 0 has an empty name.
- Unknowns:
  - How the bank record array is built from the table and from which .bwav the name resolves to (the archive path under `sounds/`, whether names map directly).
  - Which table column feeds record +6 flags and the 16-bit field at +4, and the real meaning of the two bytes at +10/+11 (listed as range/pitch candidates only).
  - Real names of 0x227ff4, 0x25c604, 0x227fac, 0x227f78, 0x227f4c, 0x1b4420 and the voice object at voice+8, and the sound thread body 0x108ae0 (the actual mixing/streaming).
  - Callers other than script commands (gameplay C++ code, UI, `ATTR_SFX*` attributes), 3D positioning maths, pitch scaling (`ScaleSoundFrequencyGO`), and how music is started.


## 2026-10-13T09:39:36+02:00 — Sound effect name → file (chase_assets parseSoundEffects; real archive)

- tablessoundfx.bxls sheet 0 column 1 holds the effect name; `sounds/<name>.bwav` exists in the FUSE archive for all 757 named rows (row 0 is unnamed). Pinned in assets-real-test. This resolves the 09:33:38 unknown "how .bwav files map to table names".
- Column 13 equals record byte +9 (volume percent). chase-soundtest applies it as a linear gain (80 → 0.8); the game's volume curve is not recovered, so that gain is an assumption.
- Unknown: columns 2, 3, 11, 12, 14, 15, 16 (paramA/paramB = record bytes +10/+11).

## 2026-10-13T09:42:03+02:00 — S4d music routing (REA Ghidra 12.1.2 pseudo-code; llvm-objdump; script docs; partial)

- Script argument docs come from `levels/scriptfns.txt` in the extracted archive (shipped text, not code evidence): `OverrideMusic(1: enum "Music Track ID Number", Tables/TablesSoundFX.xls:1)`, `OverrideMusicMood(1: number, 0 = ambient, 1 = action)`, `AutomaticMusic()`, `PauseMusic(1 = pause, 0 = unpause)`, `StopMusic(0 = stop now, >0 = seconds to fade out)`, `DuckMusicFor(fade to, for how long)`, `VehicleEnableDrivingMusic(vehicle, 1/0)`.
- Music table: `tables/tablessoundfx.bxls` sheet 1, 39 rows, columns `[row index, "-", name]`. Row 0 is empty. Rows 1..38 are names without path or extension, for example 1 `tit_01`, 4 `act_bass_break`, 8 `amb_prison`, 30 `credits`. Rows 1 and 3 are both `tit_01`. All 38 names exist as `music/<name>.bwav` in the archive (checked against the csv). The row index is the "Music Track ID Number" used by `OverrideMusic`. Not in the exe: no music file names appear in the executable strings.
- Other sheets of the same file (not read in the code): sheet 2 has 14 rows of two integers, sheet 3 has 9 rows of 34 integers, sheet 4 has 34 rows of one value, sheet 5 has 8 rows with one integer each (values 6, 15, 16, 17, 20, 12, 4, 13, all valid music row indices). Sheet 5 may be a list of music rows (hypothesis). Sheet 0 is the effects table.
- Handlers (disassembly, VFP decoded with `--mattr=+vfp2`; args array is `r1`, `[r1+4]` is the first argument pointer):
  - `OverrideMusic` 0x1cdb28: `FUN_00240ec8(id = (u16)*(u32*)arg1, 1, 1, fade = 0.5f)`.
  - `OverrideMusicMood` 0x27bcb0: `FUN_00206c40(fade = 1.0f, mood = (u8)(int)arg1, 1, 1)`.
  - `AutomaticMusic` 0x1d9c70: calls 0x1c29ac and 0x204fe0 (not read).
  - `StopMusic` 0x195054: if arg == 0.0 calls 0x21cc04(1); otherwise 0x240f6c(1.0 / (30.0 * arg)) then 0x22b660(0.0). Stored at 0x376b50 (`FUN_00240f6c` clamps to a constant if the value is <= 0). The 30.0 suggests a per-frame rate at 30 fps (inference).
  - `PauseMusic` 0x1a3020: calls 0x15bdc8 with `0.0 < arg` as a bool (body not read).
  - `DuckMusicFor`, `VehicleEnableDrivingMusic`: not read.
- Track by id, 0x240ec8 (disassembly only): `name = ((char**)DAT_00376b40)[id]`, stores id (u16) at 0x376b38, the two flags at 0x376b34/0x376b35 and the fade at 0x376b58, copies the name to 0x5862e8, then calls 0x24039c. `DAT_00376b40` is the name pointer array, presumably filled from sheet 1 above; the filler was not located.
- Mood route, 0x206c40 (ev_3f83fc98…): `DAT_00376b54` is a per-level byte array indexed by mood (0, 1, ...). 0xff means no music for that mood (stops the current track). Value 0 on the requested mood swaps to the other mood (`mood ^ 1`). Otherwise the track id is `byte[mood]`, the name is `((char**)DAT_00376b40)[id]`, and it calls 0x24039c(fade, name, id, loop flag, crossfade flag) if the id differs from the current one (`DAT_00586220`); with the crossfade flag 0 it first stops the current stream with 0x21cc04(0). The current mood is kept in `DAT_00376b33`. Nothing runs while `DAT_00376b36` or `DAT_00376b31` is set. The content of the byte array (a level attribute) was not found.
- Stream start, 0x24039c(fade, name, id, loop, crossfade) (ev_38cc810e…): builds `<name>.wav` (suffix literal ".wav" at 0x240574), pushes the path prefix `Music/` while opening it with 0x22be4c(path, mode = loop ? 2 : 0, 0), so the file is `Music/<name>.wav`. The archive holds it as `music/<name>.bwav` (same name; the .wav to .bwav mapping is shared with `Music/CSAudio/%s.wav` at 0x359658 for `music/csaudio/*.bwav`). Resulting stream handle is `DAT_00376b4c`; current name is stored at 0x5861e0, id at 0x586220, loop flag at 0x58622c.
  - Loop: the loop argument selects open mode 2 versus 0. Whether file-internal loop points are used was not checked.
  - Volume and fade: with crossfade 0 the stream starts at volume `fade * DAT_00376b3c` (`DAT_00376b3c`, a float presumably the master music volume; unverified) via 0x220f6c, and `DAT_00586228 = fade`. With crossfade 1 it starts at volume 0, stores the target in `DAT_00586224` and sets `DAT_00376b32 = 1`, so a per-frame update (not read) ramps the volume up. If a previous stream still plays and crossfade is set, the new name/id/target is queued in 0x586230, 0x586270, 0x586274 and `DAT_00376b30 = 1` instead of starting. The ramp itself and the ducking are unread.
  - A "sudden start" bit `DAT_005862e4 & 1` forces the start fade to 1.0.
- Vehicle music: the vehicle spawn function 0x23ed0c reads attribute `MusicTrack` (u16 stored at vehicle data +0x2ee, xref string 0x23efc8, ev_e567d15a…) and `NoMusic` (clears or sets bit 2 of byte +0x312, the inverse). `MusicTrack` is presumably a music table row; the consumer of +0x2ee was not read.
- Unknowns:
  - Who fills `DAT_00376b40` (the name pointer array) and `DAT_00376b54` (the mood-to-track bytes). Likely load-time from `TablesSoundFX.xls` sheet 1 and level attributes, unverified.
  - Meaning of sheets 2-5 of tablessoundfx.
  - The volume ramp updater (reads `DAT_00376b32`), `DuckMusicFor`, `PauseMusic` (0x15bdc8), `AutomaticMusic` internals, and `VehicleEnableDrivingMusic`.
  - Which file-internal loop info the stream uses for looping tracks such as `tension_short_loop` versus `tension_not_looped`.
  - Whether `AIActionMusicExclude` (string 0x1a4adc) and `levels/aiactionmusicexclusion.blvl` relate to action mood switching.
